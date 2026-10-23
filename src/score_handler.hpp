#pragma once

#include <array>

class ScoreHandler {
public:
    static inline constexpr int maxPlayerLevel = 45;

    static void initialize();

    static float levelToScore(int level);

    static int scoreToLevel(float score);

private:
    static inline bool s_inited = false;
    static inline std::array<float, maxPlayerLevel> s_table{};
};

