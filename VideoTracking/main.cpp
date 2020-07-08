#include "zmainui.h"
#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    ZMainUI w;
    if(!w.ZDoInit())
    {
        return -1;
    }
    w.show();

    return a.exec();
}
