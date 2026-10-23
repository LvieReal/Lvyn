#include "score_handler.hpp"

#include <algorithm>
#include <cmath>

void ScoreHandler::initialize() {
    s_table.fill(0.0f);
    s_table[0] = 0.0f;
    for (int i = 1; i < maxPlayerLevel; i++) {
        const float add = (40.0f / 9.0f) * std::pow(1.06f, static_cast<float>(i - 1)) * static_cast<float>(std::min(31, i));
        s_table[static_cast<size_t>(i)] = s_table[static_cast<size_t>(i - 1)] + add;
    }
    s_inited = true;
}

float ScoreHandler::levelToScore(int level) {
    if (!s_inited)
        initialize();
    if (level >= maxPlayerLevel)
        return s_table[static_cast<size_t>(maxPlayerLevel - 1)];
    if (level <= 0)
        return 0.0f;
    return s_table[static_cast<size_t>(level - 1)];
}

int ScoreHandler::scoreToLevel(float score) {
    if (!s_inited)
        initialize();
    score = std::max(0.0f, score);
    int lvl = 1;
    for (int i = 1; i <= maxPlayerLevel; i++) {
        if (score >= levelToScore(i))
            lvl = i;
        else
            break;
    }
    return lvl;
}

