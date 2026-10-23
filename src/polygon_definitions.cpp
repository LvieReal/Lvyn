#include "polygon_definitions.hpp"

#include <algorithm>
#include <cmath>

#include <QFile>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>

namespace {

QString missingField(const QString& path, const QString& field) {
    return QString("%1: missing required field '%2'").arg(path, field);
}

bool readNumber(const QJsonObject& obj, const char* key, double* out) {
    const auto v = obj.value(QLatin1StringView(key));
    if (!v.isDouble())
        return false;
    *out = v.toDouble();
    return true;
}

bool readInt(const QJsonObject& obj, const char* key, int* out) {
    const auto v = obj.value(QLatin1StringView(key));
    if (!v.isDouble())
        return false;
    *out = v.toInt();
    return true;
}

std::optional<QColor> readColor(const QJsonValue& v) {
    if (!v.isArray())
        return std::nullopt;
    const auto a = v.toArray();
    if (a.size() < 3)
        return std::nullopt;
    const int r = a.at(0).toInt(-1);
    const int g = a.at(1).toInt(-1);
    const int b = a.at(2).toInt(-1);
    const int alpha = (a.size() >= 4) ? a.at(3).toInt(255) : 255;
    if (r < 0 || g < 0 || b < 0 || alpha < 0)
        return std::nullopt;
    return QColor(r, g, b, alpha);
}

std::optional<QPolygonF> readVertices(const QJsonValue& v) {
    if (!v.isArray())
        return std::nullopt;
    const auto arr = v.toArray();
    QPolygonF poly;
    poly.reserve(arr.size());
    for (const auto& p : arr) {
        if (!p.isArray())
            return std::nullopt;
        const auto xy = p.toArray();
        if (xy.size() != 2 || !xy.at(0).isDouble() || !xy.at(1).isDouble())
            return std::nullopt;
        poly.push_back(QPointF(xy.at(0).toDouble(), xy.at(1).toDouble()));
    }
    return poly;
}

std::optional<QPair<float, float>> readRange2(const QJsonValue& v) {
    if (!v.isArray())
        return std::nullopt;
    const auto a = v.toArray();
    if (a.size() != 2 || !a.at(0).isDouble() || !a.at(1).isDouble())
        return std::nullopt;
    return QPair<float, float>(static_cast<float>(a.at(0).toDouble()), static_cast<float>(a.at(1).toDouble()));
}

} // namespace

std::optional<PolygonDefinitions> loadPolygonDefinitionsFromFile(const QString& filePath, QString* outError) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (outError)
            *outError = QString("Failed to open '%1'").arg(filePath);
        return std::nullopt;
    }

    const auto bytes = f.readAll();
    QJsonParseError parseErr{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseErr);
    if (doc.isNull() || !doc.isObject()) {
        if (outError)
            *outError = QString("Invalid JSON in '%1': %2").arg(filePath, parseErr.errorString());
        return std::nullopt;
    }

    const QJsonObject root = doc.object();

    PolygonDefinitions defs;

    // spawner
    if (!root.value("spawner").isObject()) {
        if (outError)
            *outError = missingField("root", "spawner");
        return std::nullopt;
    }
    {
        const QJsonObject sp = root.value("spawner").toObject();

        double d = 0.0;
        if (!readNumber(sp, "nestSideDivisor", &d)) {
            if (outError)
                *outError = missingField("spawner", "nestSideDivisor");
            return std::nullopt;
        }
        defs.spawner.nestSideDivisor = d;

        if (readNumber(sp, "crasherSideMultiplier", &d))
            defs.spawner.crasherSideMultiplier = d;

        if (!readNumber(sp, "polygonAreaDivisor", &d)) {
            if (outError)
                *outError = missingField("spawner", "polygonAreaDivisor");
            return std::nullopt;
        }
        defs.spawner.polygonAreaDivisor = d;

        if (!readNumber(sp, "nestAreaDivisor", &d)) {
            if (outError)
                *outError = missingField("spawner", "nestAreaDivisor");
            return std::nullopt;
        }
        defs.spawner.nestAreaDivisor = d;

        if (readNumber(sp, "avoidPlayerDistance", &d))
            defs.spawner.avoidPlayerDistance = d;

        const QString nestId = sp.value("nestPolygonId").toString();
        if (nestId.isEmpty()) {
            if (outError)
                *outError = missingField("spawner", "nestPolygonId");
            return std::nullopt;
        }
        defs.spawner.nestPolygonId = nestId;

        const QString alphaNestId = sp.value("alphaNestPolygonId").toString();
        if (alphaNestId.isEmpty()) {
            if (outError)
                *outError = missingField("spawner", "alphaNestPolygonId");
            return std::nullopt;
        }
        defs.spawner.alphaNestPolygonId = alphaNestId;

        if (readNumber(sp, "alphaPentagonChance", &d))
            defs.spawner.alphaPentagonChance = d;

        int i = 0;
        if (readInt(sp, "alphaPentagonMax", &i))
            defs.spawner.alphaPentagonMax = i;

        const QJsonValue weightsV = sp.value("randomPolygonWeights");
        if (!weightsV.isArray()) {
            if (outError)
                *outError = missingField("spawner", "randomPolygonWeights");
            return std::nullopt;
        }

        const QJsonArray weightsA = weightsV.toArray();
        for (int idx = 0; idx < weightsA.size(); idx++) {
            if (!weightsA.at(idx).isObject()) {
                if (outError)
                    *outError = QString("spawner.randomPolygonWeights[%1] must be an object").arg(idx);
                return std::nullopt;
            }
            const QJsonObject w = weightsA.at(idx).toObject();
            const QString id = w.value("id").toString();
            if (id.isEmpty()) {
                if (outError)
                    *outError = QString("spawner.randomPolygonWeights[%1].id is required").arg(idx);
                return std::nullopt;
            }
            if (!readNumber(w, "weight", &d) || d <= 0.0) {
                if (outError)
                    *outError = QString("spawner.randomPolygonWeights[%1].weight must be > 0").arg(idx);
                return std::nullopt;
            }
            defs.spawner.randomPolygonWeights.push_back(WeightedPolygonId{ id, d });
        }
    }

    // polygons
    const QJsonValue polygonsV = root.value("polygons");
    if (!polygonsV.isArray()) {
        if (outError)
            *outError = missingField("root", "polygons");
        return std::nullopt;
    }

    const QJsonArray polygonsA = polygonsV.toArray();
    for (int idx = 0; idx < polygonsA.size(); idx++) {
        if (!polygonsA.at(idx).isObject()) {
            if (outError)
                *outError = QString("polygons[%1] must be an object").arg(idx);
            return std::nullopt;
        }

        const QJsonObject o = polygonsA.at(idx).toObject();
        PolygonDefinition def;

        def.id = o.value("id").toString();
        if (def.id.isEmpty()) {
            if (outError)
                *outError = QString("polygons[%1].id is required").arg(idx);
            return std::nullopt;
        }
        def.display = o.value("display").toString(def.id);

        int sides = 0;
        if (!readInt(o, "sides", &sides) || sides < 3) {
            if (outError)
                *outError = QString("polygons[%1].sides must be >= 3").arg(idx);
            return std::nullopt;
        }
        def.sides = sides;

        double d = 0.0;
        if (readNumber(o, "radius", &d))
            def.radius = static_cast<float>(d);

        if (readNumber(o, "zIndex", &d))
            def.zIndex = static_cast<float>(d);

        const auto fill = readColor(o.value("fillColor"));
        if (!fill) {
            if (outError)
                *outError = QString("polygons[%1] must contain valid fillColor array").arg(idx);
            return std::nullopt;
        }
        def.fillColor = *fill;

        if (!readNumber(o, "maxHealth", &d)) {
            if (outError)
                *outError = missingField(QString("polygons[%1]").arg(idx), "maxHealth");
            return std::nullopt;
        }
        def.maxHealth = static_cast<float>(d);

        if (!readNumber(o, "damage", &d)) {
            if (outError)
                *outError = missingField(QString("polygons[%1]").arg(idx), "damage");
            return std::nullopt;
        }
        def.damage = static_cast<float>(d);

        if (readNumber(o, "scoreReward", &d))
            def.scoreReward = static_cast<float>(d);

        if (readNumber(o, "friction", &d))
            def.friction = static_cast<float>(d);

        if (readNumber(o, "absorptionFactor", &d))
            def.absorptionFactor = static_cast<float>(d);

        if (readNumber(o, "pushFactor", &d))
            def.pushFactor = static_cast<float>(d);

        if (readNumber(o, "baseAcceleration", &d))
            def.baseAcceleration = static_cast<float>(d);

        if (readNumber(o, "orbitRate", &d))
            def.orbitRate = static_cast<float>(d);

        if (const auto range = readRange2(o.value("orbitRateRandomRange"))) {
            def.orbitRateRandomMin = range->first;
            def.orbitRateRandomMax = range->second;
        }

        if (readNumber(o, "rotationRate", &d))
            def.rotationRate = static_cast<float>(d);

        if (readNumber(o, "edgeBuffer", &d))
            def.edgeBuffer = static_cast<float>(d);

        if (const auto poly = readVertices(o.value("vertices")))
            def.localPolygon = *poly;

        if (def.radius <= 0.0f) {
            if (!def.localPolygon.isEmpty()) {
                float r = 0.0f;
                for (const auto& p : def.localPolygon)
                    r = std::max(r, static_cast<float>(std::hypot(p.x(), p.y())));
                def.radius = r;
            } else {
                if (outError)
                    *outError = QString("polygons[%1] must contain 'radius' or non-empty 'vertices'").arg(idx);
                return std::nullopt;
            }
        }

        if (defs.polygons.contains(def.id)) {
            if (outError)
                *outError = QString("Duplicate polygon id '%1'").arg(def.id);
            return std::nullopt;
        }
        defs.polygons.insert(def.id, def);
    }

    return defs;
}
