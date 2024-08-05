#include <QApplication>
#include "main_window.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    MainWindow window;
    window.setFixedSize(600, 600);
    window.show();

    return app.exec();
}