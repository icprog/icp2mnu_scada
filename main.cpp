#include "mainwindow.h"
#include <QApplication>
#include <QProxyStyle>
#include <QStyleFactory>
#include <QMessageBox>
#include <SingleApplication/singleapplication.h>


int main(int argc, char *argv[])
{
//    QApplication a(argc, argv);  - replaced by SingleApplication

    QApplication::setApplicationName("icp2mnu_scada");
    QApplication::setOrganizationName("MNU_SCADA-casey}");

    SingleApplication a(argc, argv);


    //hash

    //if (!MainWindow::CheckHash())
    //{
    //    QMessageBox::information(NULL,"MNU SCADA error","Что-то пошло не так...");
    //    return 0;
    //}


    //style

    //"windows", "motif", "cde", "plastique" and "cleanlooks" "fusion"

    QStyle *style = new QProxyStyle(QStyleFactory::create("fusion"));
    a.setStyle(style);


    MainWindow w;
    w.show();

    QObject::connect(&a, &SingleApplication::showUp, [&]
    {
        w.showMinimized();
        //w.show();
        w.raise();
        w.activateWindow();
        w.showNormal();
    }); //

    return a.exec();
}
