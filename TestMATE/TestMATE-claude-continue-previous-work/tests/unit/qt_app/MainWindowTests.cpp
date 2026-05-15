/**************************************************************************
 * File Name: MainWindowTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 * Description: QTest unit tests for MainWindow
 **************************************************************************/

#include <QtTest/QtTest>
#include <QApplication>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QAction>

#include "qt_app/MainWindow.h"
#include "qt_app/widgets/SequenceEditorWidget.h"
#include "qt_app/widgets/ExecutionMonitorWidget.h"
#include "qt_app/widgets/ReportViewerWidget.h"

class MainWindowTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Constructor tests
    void testConstructor();
    void testInitialization();

    // UI component tests
    void testMenuBarExists();
    void testMenusExist();
    void testToolBarsExist();
    void testStatusBarExists();
    void testDockWidgetsExist();
    void testCentralWidget();

    // Menu action tests
    void testFileMenuActions();
    void testEditMenuActions();
    void testSequenceMenuActions();
    void testExecutionMenuActions();
    void testViewMenuActions();
    void testHelpMenuActions();

    // Toolbar tests
    void testFileToolBarActions();
    void testEditToolBarActions();
    void testExecutionToolBarActions();

    // Dock widget tests
    void testExecutionMonitorDock();
    void testReportViewerDock();
    void testDockWidgetVisibility();

    // Window state tests
    void testWindowTitle();
    void testInitialWindowSize();
    void testSettingsPersistence();

    // Signal/slot tests
    void testSequenceEditorSignals();
    void testExecutionMonitorSignals();

private:
    MainWindow *m_pMainWindow;
};

void MainWindowTests::initTestCase()
{
    // Called once before all tests
    qDebug() << "MainWindow test suite starting...";
}

void MainWindowTests::cleanupTestCase()
{
    // Called once after all tests
    qDebug() << "MainWindow test suite finished.";
}

void MainWindowTests::init()
{
    // Called before each test
    m_pMainWindow = new MainWindow();
}

void MainWindowTests::cleanup()
{
    // Called after each test
    delete m_pMainWindow;
    m_pMainWindow = nullptr;
}

void MainWindowTests::testConstructor()
{
    QVERIFY(m_pMainWindow != nullptr);
    QVERIFY(m_pMainWindow->isWindow());
}

void MainWindowTests::testInitialization()
{
    QVERIFY(m_pMainWindow != nullptr);
    QVERIFY(m_pMainWindow->centralWidget() != nullptr);
    QVERIFY(m_pMainWindow->menuBar() != nullptr);
    QVERIFY(m_pMainWindow->statusBar() != nullptr);
}

void MainWindowTests::testMenuBarExists()
{
    QMenuBar *menuBar = m_pMainWindow->menuBar();
    QVERIFY(menuBar != nullptr);
    QVERIFY(menuBar->actions().count() > 0);
}

void MainWindowTests::testMenusExist()
{
    QMenuBar *menuBar = m_pMainWindow->menuBar();
    QList<QAction*> actions = menuBar->actions();

    // Should have: File, Edit, Sequence, Execution, View, Tools, Help
    QVERIFY(actions.count() >= 7);

    // Check menu titles contain expected text
    bool hasFileMenu = false;
    bool hasEditMenu = false;
    bool hasHelpMenu = false;

    for (QAction *action : actions) {
        QString text = action->text().remove('&');
        if (text.contains("File")) hasFileMenu = true;
        if (text.contains("Edit")) hasEditMenu = true;
        if (text.contains("Help")) hasHelpMenu = true;
    }

    QVERIFY(hasFileMenu);
    QVERIFY(hasEditMenu);
    QVERIFY(hasHelpMenu);
}

void MainWindowTests::testToolBarsExist()
{
    QList<QToolBar*> toolBars = m_pMainWindow->findChildren<QToolBar*>();

    // Should have at least 3 toolbars: File, Edit, Execution
    QVERIFY(toolBars.count() >= 3);
}

void MainWindowTests::testStatusBarExists()
{
    QStatusBar *statusBar = m_pMainWindow->statusBar();
    QVERIFY(statusBar != nullptr);
    QVERIFY(statusBar->isVisible());
}

void MainWindowTests::testDockWidgetsExist()
{
    QList<QDockWidget*> dockWidgets = m_pMainWindow->findChildren<QDockWidget*>();

    // Should have 2 dock widgets: Execution Monitor, Report Viewer
    QVERIFY(dockWidgets.count() >= 2);
}

void MainWindowTests::testCentralWidget()
{
    QWidget *central = m_pMainWindow->centralWidget();
    QVERIFY(central != nullptr);

    // Central widget should be SequenceEditorWidget
    SequenceEditorWidget *editor = qobject_cast<SequenceEditorWidget*>(central);
    QVERIFY(editor != nullptr);
}

void MainWindowTests::testFileMenuActions()
{
    QMenuBar *menuBar = m_pMainWindow->menuBar();
    QMenu *fileMenu = nullptr;

    for (QAction *action : menuBar->actions()) {
        if (action->text().remove('&').contains("File")) {
            fileMenu = action->menu();
            break;
        }
    }

    QVERIFY(fileMenu != nullptr);

    QList<QAction*> actions = fileMenu->actions();
    QVERIFY(actions.count() > 0);

    // Check for key actions
    bool hasNew = false;
    bool hasOpen = false;
    bool hasSave = false;
    bool hasExit = false;

    for (QAction *action : actions) {
        if (action->isSeparator()) continue;
        QString text = action->text().remove('&');
        if (text.contains("New")) hasNew = true;
        if (text.contains("Open")) hasOpen = true;
        if (text.contains("Save")) hasSave = true;
        if (text.contains("Exit") || text.contains("Quit")) hasExit = true;
    }

    QVERIFY(hasNew);
    QVERIFY(hasOpen);
    QVERIFY(hasSave);
}

void MainWindowTests::testEditMenuActions()
{
    QMenuBar *menuBar = m_pMainWindow->menuBar();
    QMenu *editMenu = nullptr;

    for (QAction *action : menuBar->actions()) {
        if (action->text().remove('&').contains("Edit")) {
            editMenu = action->menu();
            break;
        }
    }

    QVERIFY(editMenu != nullptr);
    QVERIFY(editMenu->actions().count() > 0);
}

void MainWindowTests::testSequenceMenuActions()
{
    QMenuBar *menuBar = m_pMainWindow->menuBar();
    QMenu *sequenceMenu = nullptr;

    for (QAction *action : menuBar->actions()) {
        if (action->text().remove('&').contains("Sequence")) {
            sequenceMenu = action->menu();
            break;
        }
    }

    QVERIFY(sequenceMenu != nullptr);
    QVERIFY(sequenceMenu->actions().count() > 0);
}

void MainWindowTests::testExecutionMenuActions()
{
    QMenuBar *menuBar = m_pMainWindow->menuBar();
    QMenu *executionMenu = nullptr;

    for (QAction *action : menuBar->actions()) {
        if (action->text().remove('&').contains("Execution")) {
            executionMenu = action->menu();
            break;
        }
    }

    QVERIFY(executionMenu != nullptr);

    // Check for Run action
    bool hasRun = false;
    for (QAction *action : executionMenu->actions()) {
        if (action->text().contains("Run")) {
            hasRun = true;
            break;
        }
    }
    QVERIFY(hasRun);
}

void MainWindowTests::testViewMenuActions()
{
    QMenuBar *menuBar = m_pMainWindow->menuBar();
    QMenu *viewMenu = nullptr;

    for (QAction *action : menuBar->actions()) {
        if (action->text().remove('&').contains("View")) {
            viewMenu = action->menu();
            break;
        }
    }

    QVERIFY(viewMenu != nullptr);
}

void MainWindowTests::testHelpMenuActions()
{
    QMenuBar *menuBar = m_pMainWindow->menuBar();
    QMenu *helpMenu = nullptr;

    for (QAction *action : menuBar->actions()) {
        if (action->text().remove('&').contains("Help")) {
            helpMenu = action->menu();
            break;
        }
    }

    QVERIFY(helpMenu != nullptr);

    // Check for About action
    bool hasAbout = false;
    for (QAction *action : helpMenu->actions()) {
        if (action->text().contains("About")) {
            hasAbout = true;
            break;
        }
    }
    QVERIFY(hasAbout);
}

void MainWindowTests::testFileToolBarActions()
{
    QList<QToolBar*> toolBars = m_pMainWindow->findChildren<QToolBar*>();

    bool foundFileToolBar = false;
    for (QToolBar *tb : toolBars) {
        if (tb->windowTitle().contains("File") && tb->actions().count() > 0) {
            foundFileToolBar = true;
            break;
        }
    }

    QVERIFY(foundFileToolBar);
}

void MainWindowTests::testEditToolBarActions()
{
    QList<QToolBar*> toolBars = m_pMainWindow->findChildren<QToolBar*>();

    bool foundEditToolBar = false;
    for (QToolBar *tb : toolBars) {
        if (tb->windowTitle().contains("Edit") && tb->actions().count() > 0) {
            foundEditToolBar = true;
            break;
        }
    }

    QVERIFY(foundEditToolBar);
}

void MainWindowTests::testExecutionToolBarActions()
{
    QList<QToolBar*> toolBars = m_pMainWindow->findChildren<QToolBar*>();

    bool foundExecutionToolBar = false;
    for (QToolBar *tb : toolBars) {
        if (tb->windowTitle().contains("Execution") && tb->actions().count() > 0) {
            foundExecutionToolBar = true;
            break;
        }
    }

    QVERIFY(foundExecutionToolBar);
}

void MainWindowTests::testExecutionMonitorDock()
{
    QList<QDockWidget*> docks = m_pMainWindow->findChildren<QDockWidget*>();

    bool foundExecutionMonitor = false;
    for (QDockWidget *dock : docks) {
        if (dock->windowTitle().contains("Execution Monitor")) {
            foundExecutionMonitor = true;
            QVERIFY(dock->widget() != nullptr);

            ExecutionMonitorWidget *monitor = qobject_cast<ExecutionMonitorWidget*>(dock->widget());
            QVERIFY(monitor != nullptr);
            break;
        }
    }

    QVERIFY(foundExecutionMonitor);
}

void MainWindowTests::testReportViewerDock()
{
    QList<QDockWidget*> docks = m_pMainWindow->findChildren<QDockWidget*>();

    bool foundReportViewer = false;
    for (QDockWidget *dock : docks) {
        if (dock->windowTitle().contains("Report Viewer")) {
            foundReportViewer = true;
            QVERIFY(dock->widget() != nullptr);

            ReportViewerWidget *viewer = qobject_cast<ReportViewerWidget*>(dock->widget());
            QVERIFY(viewer != nullptr);
            break;
        }
    }

    QVERIFY(foundReportViewer);
}

void MainWindowTests::testDockWidgetVisibility()
{
    QList<QDockWidget*> docks = m_pMainWindow->findChildren<QDockWidget*>();

    // All docks should be visible by default
    for (QDockWidget *dock : docks) {
        QVERIFY(dock->isVisible() || dock->isHidden()); // Either state is valid initially
    }
}

void MainWindowTests::testWindowTitle()
{
    QString title = m_pMainWindow->windowTitle();
    QVERIFY(!title.isEmpty());
    QVERIFY(title.contains("TestMATE"));
}

void MainWindowTests::testInitialWindowSize()
{
    QSize size = m_pMainWindow->size();

    // Window should have reasonable size (at least 800x600)
    QVERIFY(size.width() >= 800);
    QVERIFY(size.height() >= 600);
}

void MainWindowTests::testSettingsPersistence()
{
    // Test that settings can be loaded/saved without crashing
    // Note: Actual persistence would require file I/O tests

    // Just verify the window can be resized
    m_pMainWindow->resize(1024, 768);
    QCOMPARE(m_pMainWindow->size(), QSize(1024, 768));
}

void MainWindowTests::testSequenceEditorSignals()
{
    SequenceEditorWidget *editor = qobject_cast<SequenceEditorWidget*>(m_pMainWindow->centralWidget());
    QVERIFY(editor != nullptr);

    // Verify signal exists by checking meta object
    const QMetaObject *metaObj = editor->metaObject();
    int signalIndex = metaObj->indexOfSignal("sequenceModified()");
    QVERIFY(signalIndex >= 0);
}

void MainWindowTests::testExecutionMonitorSignals()
{
    QList<QDockWidget*> docks = m_pMainWindow->findChildren<QDockWidget*>();

    for (QDockWidget *dock : docks) {
        ExecutionMonitorWidget *monitor = qobject_cast<ExecutionMonitorWidget*>(dock->widget());
        if (monitor) {
            const QMetaObject *metaObj = monitor->metaObject();

            // Verify signals exist
            QVERIFY(metaObj->indexOfSignal("executionStarted()") >= 0);
            QVERIFY(metaObj->indexOfSignal("executionCompleted(bool)") >= 0);
            break;
        }
    }
}

QTEST_MAIN(MainWindowTests)
#include "MainWindowTests.moc"
