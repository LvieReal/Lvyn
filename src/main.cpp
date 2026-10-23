#include <QApplication>
#include <QSurfaceFormat>
#include <QtGlobal>
#include <QFontDatabase>

#include "window.hpp"
#include "game_manager.hpp"
#include "arena.hpp"
#include "settings.hpp"
#include "score_handler.hpp"

int main(int argc, char *argv[]) {
    // Disable v-sync (swap interval) so rendering cadence is controlled by Settings::renderRateMS.
    // Note: this must be set before creating any Qt GUI objects that create a surface.
    qputenv("QSG_NO_VSYNC", "1");
    QSurfaceFormat fmt = QSurfaceFormat::defaultFormat();
    fmt.setSwapInterval(0);
    QSurfaceFormat::setDefaultFormat(fmt);

    auto app = QApplication(argc, argv);
    app.setApplicationName("TBD");

    auto fontId = QFontDatabase::addApplicationFont(":/assets/fonts/Ubuntu.TTF");
    if (fontId != -1)
        QApplication::setFont(QFont(QFontDatabase::applicationFontFamilies(fontId)[0]));

    {
        QString err;
        if (!Settings::loadFromFile("config/Settings.json", &err)) {
            qWarning() << "Settings load failed:" << err;
        }
    }

    ScoreHandler::initialize();

    GameManager gameManager;

    Arena arena(gameManager);

    auto window = Window(gameManager, &arena);
    window.showMaximized();

    return app.exec();
}
