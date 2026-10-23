#include "tank_definitions.hpp"

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

} // namespace

std::optional<TankDefinitions> loadTankDefinitionsFromFile(const QString& filePath, QString* outError) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (outError)
            *outError = QString("Failed to open '%1'").arg(filePath);
        return std::nullopt;
    }

    const auto bytes = f.readAll();
    QJsonParseError parseErr{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseErr);
    if (doc.isNull() || !doc.isArray()) {
        if (outError)
            *outError = QString("Invalid JSON array in '%1': %2").arg(filePath, parseErr.errorString());
        return std::nullopt;
    }

    TankDefinitions defs;

    const QJsonArray arr = doc.array();
    for (int idx = 0; idx < arr.size(); idx++) {
        if (!arr.at(idx).isObject()) {
            if (outError)
                *outError = QString("TankDefinitions[%1] must be an object").arg(idx);
            return std::nullopt;
        }

        const QJsonObject o = arr.at(idx).toObject();
        TankDefinition def;

        int id = 0;
        if (readInt(o, "id", &id))
            def.id = id;

        def.name = o.value("name").toString();
        if (def.name.isEmpty()) {
            if (outError)
                *outError = QString("TankDefinitions[%1].name is required").arg(idx);
            return std::nullopt;
        }

        // flags
        if (o.value("flags").isObject()) {
            const QJsonObject fl = o.value("flags").toObject();
            def.flags.invisibility = fl.value("invisibility").toBool(false);
            def.flags.zoomAbility = fl.value("zoomAbility").toBool(false);
            def.flags.canClaimSquares = fl.value("canClaimSquares").toBool(false);
            def.flags.devOnly = fl.value("devOnly").toBool(false);
        }

        double d = 0.0;
        if (readNumber(o, "fieldFactor", &d))
            def.fieldFactor = static_cast<float>(d);
        if (readNumber(o, "absorbtionFactor", &d))
            def.absorptionFactor = static_cast<float>(d);
        if (readNumber(o, "speed", &d))
            def.speed = static_cast<float>(d);
        if (readNumber(o, "maxHealth", &d))
            def.maxHealth = static_cast<float>(d);

        if (readNumber(o, "visibilityRateShooting", &d))
            def.visibilityRateShooting = static_cast<float>(d);
        if (readNumber(o, "visibilityRateMoving", &d))
            def.visibilityRateMoving = static_cast<float>(d);
        if (readNumber(o, "invisibilityRate", &d))
            def.invisibilityRate = static_cast<float>(d);

        int i = 0;
        if (readInt(o, "sides", &i))
            def.sides = i;
        if (readNumber(o, "borderWidth", &d))
            def.borderWidth = static_cast<float>(d);

        // barrels
        const QJsonValue barrelsV = o.value("barrels");
        if (!barrelsV.isArray()) {
            if (outError)
                *outError = missingField(QString("TankDefinitions[%1]").arg(idx), "barrels");
            return std::nullopt;
        }

        const QJsonArray barrelsA = barrelsV.toArray();
        def.barrels.reserve(barrelsA.size());
        for (int bi = 0; bi < barrelsA.size(); bi++) {
            if (!barrelsA.at(bi).isObject()) {
                if (outError)
                    *outError = QString("TankDefinitions[%1].barrels[%2] must be an object").arg(idx).arg(bi);
                return std::nullopt;
            }

            const QJsonObject b = barrelsA.at(bi).toObject();
            TankBarrelDefinition barrel;

            if (readNumber(b, "angle", &d))
                barrel.angleRadians = d;
            if (readNumber(b, "offset", &d))
                barrel.offset = static_cast<float>(d);
            if (readNumber(b, "size", &d))
                barrel.size = static_cast<float>(d);
            if (readNumber(b, "width", &d))
                barrel.width = static_cast<float>(d);

            if (readNumber(b, "delay", &d))
                barrel.delayFactor = static_cast<float>(d);
            if (readNumber(b, "reload", &d))
                barrel.reloadFactor = static_cast<float>(d);
            if (readNumber(b, "recoil", &d))
                barrel.recoil = static_cast<float>(d);

            barrel.isTrapezoid = b.value("isTrapezoid").toBool(false);
            if (readNumber(b, "trapezoidDirection", &d))
                barrel.trapezoidDirectionRadians = d;

            if (!b.value("addon").isNull())
                barrel.addon = b.value("addon").toString();

            // bullet
            if (!b.value("bullet").isObject()) {
                if (outError)
                    *outError = missingField(QString("TankDefinitions[%1].barrels[%2]").arg(idx).arg(bi), "bullet");
                return std::nullopt;
            }
            const QJsonObject blt = b.value("bullet").toObject();
            barrel.bullet.type = blt.value("type").toString();
            if (barrel.bullet.type.isEmpty()) {
                if (outError)
                    *outError = QString("TankDefinitions[%1].barrels[%2].bullet.type is required").arg(idx).arg(bi);
                return std::nullopt;
            }
            if (readNumber(blt, "sizeRatio", &d))
                barrel.bullet.sizeRatio = static_cast<float>(d);
            if (readNumber(blt, "health", &d))
                barrel.bullet.health = static_cast<float>(d);
            if (readNumber(blt, "damage", &d))
                barrel.bullet.damage = static_cast<float>(d);
            if (readNumber(blt, "speed", &d))
                barrel.bullet.speed = static_cast<float>(d);
            if (readNumber(blt, "scatterRate", &d))
                barrel.bullet.scatterRate = static_cast<float>(d);
            if (readNumber(blt, "lifeLength", &d))
                barrel.bullet.lifeLength = static_cast<float>(d);
            if (readNumber(blt, "absorbtionFactor", &d))
                barrel.bullet.absorptionFactor = static_cast<float>(d);

            def.barrels.push_back(barrel);
        }

        // stats
        const QJsonValue statsV = o.value("stats");
        if (statsV.isArray()) {
            const QJsonArray statsA = statsV.toArray();
            def.stats.reserve(statsA.size());
            for (int si = 0; si < statsA.size(); si++) {
                if (!statsA.at(si).isObject())
                    continue;
                const QJsonObject so = statsA.at(si).toObject();
                TankStatCap sc;
                sc.name = so.value("name").toString();
                if (readInt(so, "max", &i))
                    sc.max = i;
                if (!sc.name.isEmpty())
                    def.stats.push_back(sc);
            }
        }
        if (def.stats.isEmpty()) {
            // Default stat caps.
            def.stats = QList<TankStatCap>{
                {"Movement Speed", 7},
                {"Reload", 7},
                {"Bullet Damage", 7},
                {"Bullet Penetration", 7},
                {"Bullet Speed", 7},
                {"Body Damage", 7},
                {"Max Health", 7},
                {"Health Regen", 7},
            };
        }

        const QString key = def.name.trimmed().toLower();
        if (defs.byNameLower.contains(key)) {
            if (outError)
                *outError = QString("Duplicate tank definition name '%1'").arg(def.name);
            return std::nullopt;
        }
        defs.byNameLower.insert(key, def);
    }

    return defs;
}
