#include "qt_hikvision.h"
#include <QtWidgets/QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    qt_hikvision w;
    w.show();
    return a.exec();
}
