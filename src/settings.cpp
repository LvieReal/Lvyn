#include "settings.hpp"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>

namespace {

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

void Settings::recomputeDerived() {
    renderRateMS = renderRate * 1000.0f;
    physicsRateMS = physicsRate * 1000.0f;
    arenaBoundsPad = arenaGridSize * 10;
}

bool Settings::loadFromFile(const QString& filePath, QString* outError) {
    QFile f(filePath);
    if (!f.open(QIODevice::ReadOnly)) {
        if (outError)
            *outError = QString("Failed to open '%1'").arg(filePath);
        recomputeDerived();
        return false;
    }

    const auto bytes = f.readAll();
    QJsonParseError parseErr{};
    const QJsonDocument doc = QJsonDocument::fromJson(bytes, &parseErr);
    if (doc.isNull() || !doc.isObject()) {
        if (outError)
            *outError = QString("Invalid JSON in '%1': %2").arg(filePath, parseErr.errorString());
        recomputeDerived();
        return false;
    }

    const QJsonObject root = doc.object();

    double d = 0.0;
    int i = 0;

    if (root.value("timing").isObject()) {
        const QJsonObject t = root.value("timing").toObject();
        if (readNumber(t, "renderRate", &d))
            renderRate = static_cast<float>(d);
        if (readNumber(t, "physicsRate", &d))
            physicsRate = static_cast<float>(d);
    }

    if (root.value("arena").isObject()) {
        const QJsonObject a = root.value("arena").toObject();
        if (readInt(a, "width", &i))
            arenaWidth = i;
        if (readInt(a, "height", &i))
            arenaHeight = i;
        if (readInt(a, "gridSize", &i))
            arenaGridSize = i;
        if (readNumber(a, "gridLineWidth", &d))
            arenaGridLineWidth = static_cast<float>(d);
    }

    if (root.value("colors").isObject()) {
        const QJsonObject c = root.value("colors").toObject();

        if (const auto col = readColor(c.value("arenaColor")))
            arenaColor = *col;
        if (const auto col = readColor(c.value("arenaGridColor")))
            arenaGridColor = *col;
        if (const auto col = readColor(c.value("backgroundColor")))
            backgroundColor = *col;
        if (const auto col = readColor(c.value("globalStrokeColor")))
            globalStrokeColor = *col;

        const QJsonValue palV = c.value("palette");
        if (palV.isObject()) {
            palette.clear();
            const QJsonObject pal = palV.toObject();
            for (auto it = pal.begin(); it != pal.end(); ++it) {
                const auto col = readColor(it.value());
                if (!col)
                    continue;
                palette.insert(it.key(), *col);
            }
        }
    }

    recomputeDerived();
    return true;
}

