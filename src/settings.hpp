#pragma once

#include <optional>

#include <QColor>
#include <QHash>
#include <QString>

class Settings {
public:
    // Call once at startup (before constructing game objects that read Settings).
    // If the file can't be loaded, defaults remain in effect.
    static bool loadFromFile(const QString& filePath, QString* outError);

    // Timing (seconds)
    static inline float renderRate;
    static inline float renderRateMS;

    static inline float physicsRate;
    static inline float physicsRateMS;
    static inline float physicsRateCounter;

    // Arena
    static inline int arenaWidth;
    static inline int arenaHeight;

    static inline int arenaGridSize;
    static inline float arenaGridLineWidth;

    static inline int arenaBoundsPad;

    // Core colors
    static inline QColor arenaColor;
    static inline QColor arenaGridColor;
    static inline QColor backgroundColor;

    // Stroke is global across objects.
    static inline QColor globalStrokeColor;

    static inline QHash<QString, QColor> palette;

private:
    static void recomputeDerived();
};

