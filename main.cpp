#include "can_paddle.h"

#include <QApplication>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    CanPaddle w;
    w.showMaximized();
    return a.exec();
}
