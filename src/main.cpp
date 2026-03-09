#include <QApplication>
#include <QtWebEngineQuick/QtWebEngineQuick>
#include "MainWindow.h"

int main(int argc, char *argv[])
{
    QtWebEngineQuick::initialize();
    QApplication app(argc, argv);
    app.setApplicationName("TextEd");
    app.setOrganizationName("TextEd");

    MainWindow w;
    w.resize(1200, 800);
    w.show();

    if (argc > 1)
        w.openFile(QString::fromLocal8Bit(argv[1]));

    return app.exec();
}
