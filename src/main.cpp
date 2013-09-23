#include "mainwindow.h"
#include <QtWidgets/QApplication>
#include <QtWidgets/QDesktopWidget.h>

int main(int argc, char *argv[])
{
    QApplication a(argc, argv);
    MainWindow w;

	QRect screenGeometry = QApplication::desktop()->screenGeometry();
	int x = (screenGeometry.width()-w.width()) / 2;
	//int y = (screenGeometry.height()-w.height()) / 2;
	int y = 30;
	w.move(x, y);
    w.show();

    return a.exec();
}