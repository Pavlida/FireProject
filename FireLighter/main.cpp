#include "mainwindow.h"

#include <QApplication>
int main(int argc, char* argv[])
{
    QApplication app(argc, argv);
    UserImage *view = new UserImage();
    MainWindow w(nullptr, view);

    w.show();
    return app.exec();
}
