/**************************************************************************
 * File Name: main.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 * Description: Main entry point for TestMATE Qt application
 **************************************************************************/

#include "MainWindow.h"
#include <QApplication>
#include <QStyleFactory>
#include <QIcon>

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    // Set application metadata
    QApplication::setApplicationName("TestMATE");
    QApplication::setApplicationVersion("1.0.0");
    QApplication::setOrganizationName("TestMATE");
    QApplication::setOrganizationDomain("testmate.org");

    // Set modern style
    QApplication::setStyle(QStyleFactory::create("Fusion"));

    // Set application icon
    QApplication::setWindowIcon(QIcon(":/icons/app_icon.svg"));

    // Create and show main window
    MainWindow mainWindow;
    mainWindow.show();

    return app.exec();
}
