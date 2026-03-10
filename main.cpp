#include <QApplication>
#include <QScreen>
#include <QDebug>
#include "TopBar.h"

int main(int argc, char *argv[])
{
    // Enable high DPI scaling
    QApplication::setAttribute(Qt::AA_EnableHighDpiScaling);
    QApplication::setAttribute(Qt::AA_UseHighDpiPixmaps);

    QApplication app(argc, argv);
    app.setApplicationName("OBK TopBar");

    // Create the toolbar
    TopBar topBar;

    // Connect signals for demonstration
    QObject::connect(&topBar, &TopBar::languageSwitcherClicked, []() {
        qDebug() << "Language switcher clicked";
    });

    QObject::connect(&topBar, &TopBar::monitorSettingsClicked, []() {
        qDebug() << "Monitor settings clicked";
    });

    QObject::connect(&topBar, &TopBar::inputSettingsClicked, []() {
        qDebug() << "Input settings clicked";
    });

    QObject::connect(&topBar, &TopBar::settingsClicked, []() {
        qDebug() << "Settings clicked";
    });

    QObject::connect(&topBar, &TopBar::powerClicked, []() {
        qDebug() << "Power clicked";
        QApplication::quit();
    });

    // Position the toolbar at the center-bottom of the primary screen
    QScreen* screen = QApplication::primaryScreen();
    QRect screenGeometry = screen->availableGeometry();

    int x = screenGeometry.x() + (screenGeometry.width() - topBar.width()) / 2;
    int y = screenGeometry.y() + screenGeometry.height() - topBar.height() - 50;

    topBar.move(x, y);
    topBar.show();

    // Enable acrylic blur AFTER show() so the platform window is fully created.
    // Calling winId() before show() with FramelessWindowHint|Tool can cause
    // recursive CreateWindowEx failures.
    topBar.enableBlur();

    return app.exec();
}
