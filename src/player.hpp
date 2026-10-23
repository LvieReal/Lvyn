#pragma once

#include "tank.hpp"
#include "settings.hpp"

class Player : public Tank {
public:
    Player() : Tank() {
        fillColor = Settings::palette.value("BLUE");
    }

    QString username = "unnamed";
};
