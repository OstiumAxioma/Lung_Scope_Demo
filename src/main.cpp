#include <QApplication>
#include <QStyleFactory>
#include <QDebug>
#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // 设置应用程序信息
    app.setApplicationName("VTK Qt 项目");
    app.setApplicationVersion("1.0");
    app.setOrganizationName("VTK Qt 项目组");

    // 设置应用程序样式
    app.setStyle(QStyleFactory::create("Fusion"));

    qDebug() << "Qt应用程序初始化成功";

    // 创建并显示主窗口
    MainWindow window;
    qDebug() << "主窗口创建成功";

    window.show();
    qDebug() << "主窗口显示成功";

    // 运行应用程序
    return app.exec();
}
