#include <QApplication>
#include "mainwindow.h"

int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    app.setStyle("Fusion");
    app.setApplicationName("ZU Documentation");
    app.setApplicationVersion("1.0");

    MainWindow window;
    window.showFullScreen();

    return app.exec();
}
