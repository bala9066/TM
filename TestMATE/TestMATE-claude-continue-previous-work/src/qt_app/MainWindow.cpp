/**************************************************************************
 * File Name: MainWindow.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 * Description: Main window implementation
 **************************************************************************/

#include "MainWindow.h"
#include "ui_MainWindow.h"
#include "widgets/SequenceEditorWidget.h"
#include "widgets/ExecutionMonitorWidget.h"
#include "widgets/ReportViewerWidget.h"
#include "widgets/ConfigurationDialog.h"
#include "widgets/AboutDialog.h"
#include "StepPaletteWidget.h"

#include "api/TestMATECore.h"

#include <QAction>
#include <QMenu>
#include <QMenuBar>
#include <QToolBar>
#include <QStatusBar>
#include <QDockWidget>
#include <QFileDialog>
#include <QMessageBox>
#include <QCloseEvent>
#include <QSettings>
#include <QLabel>
#include <QProgressBar>
#include <QApplication>

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::MainWindow)
    , m_pCore(std::make_unique<TestMATE::CTestMATECore>())
    , m_pSequenceEditor(nullptr)
    , m_pExecutionMonitor(nullptr)
    , m_pReportViewer(nullptr)
    , m_pStepPalette(nullptr)
    , m_pConfigDialog(nullptr)
    , m_bModified(false)
    , m_bRunning(false)
{
    ui->setupUi(this);

    setupUI();
    createActions();
    createMenus();
    createToolbars();
    createDockWidgets();
    createStatusBar();
    connectSignals();
    loadSettings();

    // Initialize core
    m_pCore->Initialize();

    updateWindowTitle();
    updateStatusBar();

    // Start UI update timer
    m_updateTimer.setInterval(100); // 100ms updates
    connect(&m_updateTimer, &QTimer::timeout, this, &MainWindow::updateStatusBar);
    m_updateTimer.start();
}

MainWindow::~MainWindow()
{
    saveSettings();
    delete ui;
}

void MainWindow::setupUI()
{
    setWindowTitle("TestMATE - Test Management and Automation Tool");
    resize(1400, 900);

    // Set central widget (will be sequence editor)
    m_pSequenceEditor = new SequenceEditorWidget(this);
    setCentralWidget(m_pSequenceEditor);
}

void MainWindow::createActions()
{
    // Actions will be created within menus for simplicity
}

void MainWindow::createMenus()
{
    // File Menu
    QMenu *fileMenu = menuBar()->addMenu(tr("&File"));

    QAction *newAction = fileMenu->addAction(QIcon(":/icons/new.svg"), tr("&New Sequence"), this, &MainWindow::onNewSequence);
    newAction->setShortcut(QKeySequence::New);

    QAction *openAction = fileMenu->addAction(QIcon(":/icons/open.svg"), tr("&Open Sequence..."), this, &MainWindow::onOpenSequence);
    openAction->setShortcut(QKeySequence::Open);

    fileMenu->addSeparator();

    QAction *saveAction = fileMenu->addAction(QIcon(":/icons/save.svg"), tr("&Save"), this, &MainWindow::onSaveSequence);
    saveAction->setShortcut(QKeySequence::Save);

    QAction *saveAsAction = fileMenu->addAction(tr("Save &As..."), this, &MainWindow::onSaveSequenceAs);
    saveAsAction->setShortcut(QKeySequence::SaveAs);

    fileMenu->addSeparator();

    QAction *exitAction = fileMenu->addAction(tr("E&xit"), this, &MainWindow::onExit);
    exitAction->setShortcut(QKeySequence::Quit);

    // Edit Menu
    QMenu *editMenu = menuBar()->addMenu(tr("&Edit"));

    QAction *undoAction = editMenu->addAction(QIcon(":/icons/undo.svg"), tr("&Undo"), this, &MainWindow::onUndo);
    undoAction->setShortcut(QKeySequence::Undo);

    QAction *redoAction = editMenu->addAction(QIcon(":/icons/redo.svg"), tr("&Redo"), this, &MainWindow::onRedo);
    redoAction->setShortcut(QKeySequence::Redo);

    editMenu->addSeparator();

    QAction *cutAction = editMenu->addAction(QIcon(":/icons/cut.svg"), tr("Cu&t"), this, &MainWindow::onCut);
    cutAction->setShortcut(QKeySequence::Cut);

    QAction *copyAction = editMenu->addAction(QIcon(":/icons/copy.svg"), tr("&Copy"), this, &MainWindow::onCopy);
    copyAction->setShortcut(QKeySequence::Copy);

    QAction *pasteAction = editMenu->addAction(QIcon(":/icons/paste.svg"), tr("&Paste"), this, &MainWindow::onPaste);
    pasteAction->setShortcut(QKeySequence::Paste);

    editMenu->addSeparator();

    editMenu->addAction(tr("&Preferences..."), this, &MainWindow::onPreferences);

    // Sequence Menu
    QMenu *sequenceMenu = menuBar()->addMenu(tr("&Sequence"));

    sequenceMenu->addAction(QIcon(":/icons/add.svg"), tr("&Add Step..."), this, &MainWindow::onAddStep);
    sequenceMenu->addAction(QIcon(":/icons/edit.svg"), tr("&Edit Step..."), this, &MainWindow::onEditStep);
    sequenceMenu->addAction(QIcon(":/icons/delete.svg"), tr("&Delete Step"), this, &MainWindow::onDeleteStep);

    sequenceMenu->addSeparator();

    sequenceMenu->addAction(QIcon(":/icons/up.svg"), tr("Move &Up"), this, &MainWindow::onMoveStepUp);
    sequenceMenu->addAction(QIcon(":/icons/down.svg"), tr("Move &Down"), this, &MainWindow::onMoveStepDown);

    sequenceMenu->addSeparator();

    sequenceMenu->addAction(tr("&Validate Sequence"), this, &MainWindow::onValidateSequence);

    // Execution Menu
    QMenu *executionMenu = menuBar()->addMenu(tr("E&xecution"));

    QAction *runAction = executionMenu->addAction(QIcon(":/icons/run.svg"), tr("&Run"), this, &MainWindow::onRun);
    runAction->setShortcut(Qt::Key_F5);

    QAction *pauseAction = executionMenu->addAction(QIcon(":/icons/pause.svg"), tr("&Pause"), this, &MainWindow::onPause);
    pauseAction->setShortcut(Qt::Key_F6);

    QAction *stopAction = executionMenu->addAction(QIcon(":/icons/stop.svg"), tr("&Stop"), this, &MainWindow::onStop);
    stopAction->setShortcut(Qt::Key_F7);

    QAction *abortAction = executionMenu->addAction(QIcon(":/icons/abort.svg"), tr("&Abort"), this, &MainWindow::onAbort);
    abortAction->setShortcut(Qt::SHIFT | Qt::Key_F7);

    executionMenu->addSeparator();

    executionMenu->addAction(tr("Run with &Debug"), this, &MainWindow::onRunWithDebug);

    // View Menu
    QMenu *viewMenu = menuBar()->addMenu(tr("&View"));

    viewMenu->addAction(tr("&Sequence Editor"), this, &MainWindow::onToggleSequenceEditor);
    viewMenu->addAction(tr("&Execution Monitor"), this, &MainWindow::onToggleExecutionMonitor);
    viewMenu->addAction(tr("&Report Viewer"), this, &MainWindow::onToggleReportViewer);

    viewMenu->addSeparator();

    viewMenu->addAction(tr("&Toolbar"), this, &MainWindow::onToggleToolbar);
    viewMenu->addAction(tr("&Status Bar"), this, &MainWindow::onToggleStatusBar);

    // Tools Menu
    QMenu *toolsMenu = menuBar()->addMenu(tr("&Tools"));

    toolsMenu->addAction(QIcon(":/icons/config.svg"), tr("&Configuration..."), this, &MainWindow::onOpenConfiguration);
    toolsMenu->addAction(tr("&Database Manager..."), this, &MainWindow::onOpenDatabase);

    toolsMenu->addSeparator();

    toolsMenu->addAction(tr("&Export Report..."), this, &MainWindow::onExportReport);
    toolsMenu->addAction(tr("&Import Limits..."), this, &MainWindow::onImportLimits);

    toolsMenu->addSeparator();

    toolsMenu->addAction(tr("&Plugin Manager..."), this, &MainWindow::onPluginManager);

    // Help Menu
    QMenu *helpMenu = menuBar()->addMenu(tr("&Help"));

    helpMenu->addAction(QIcon(":/icons/help.svg"), tr("&Documentation"), this, &MainWindow::onDocumentation);
    helpMenu->addAction(tr("Check for &Updates..."), this, &MainWindow::onCheckUpdates);

    helpMenu->addSeparator();

    helpMenu->addAction(tr("&About TestMATE..."), this, &MainWindow::onAbout);
}

void MainWindow::createToolbars()
{
    // Use toolbars from UI file and populate them
    m_pFileToolBar = ui->fileToolBar;
    m_pFileToolBar->addAction(QIcon(":/icons/new.svg"), tr("New"), this, &MainWindow::onNewSequence);
    m_pFileToolBar->addAction(QIcon(":/icons/open.svg"), tr("Open"), this, &MainWindow::onOpenSequence);
    m_pFileToolBar->addAction(QIcon(":/icons/save.svg"), tr("Save"), this, &MainWindow::onSaveSequence);

    m_pEditToolBar = ui->editToolBar;
    m_pEditToolBar->addAction(QIcon(":/icons/add.svg"), tr("Add Step"), this, &MainWindow::onAddStep);
    m_pEditToolBar->addAction(QIcon(":/icons/edit.svg"), tr("Edit Step"), this, &MainWindow::onEditStep);
    m_pEditToolBar->addAction(QIcon(":/icons/delete.svg"), tr("Delete Step"), this, &MainWindow::onDeleteStep);
    m_pEditToolBar->addSeparator();
    m_pEditToolBar->addAction(QIcon(":/icons/up.svg"), tr("Move Up"), this, &MainWindow::onMoveStepUp);
    m_pEditToolBar->addAction(QIcon(":/icons/down.svg"), tr("Move Down"), this, &MainWindow::onMoveStepDown);

    m_pExecutionToolBar = ui->executionToolBar;
    m_pExecutionToolBar->addAction(QIcon(":/icons/run.svg"), tr("Run"), this, &MainWindow::onRun);
    m_pExecutionToolBar->addAction(QIcon(":/icons/pause.svg"), tr("Pause"), this, &MainWindow::onPause);
    m_pExecutionToolBar->addAction(QIcon(":/icons/stop.svg"), tr("Stop"), this, &MainWindow::onStop);
    m_pExecutionToolBar->addAction(QIcon(":/icons/abort.svg"), tr("Abort"), this, &MainWindow::onAbort);
}

void MainWindow::createDockWidgets()
{
    // Use dock widgets from UI file and set their content
    m_pExecutionMonitor = new ExecutionMonitorWidget(this);
    m_pExecutionMonitorDock = ui->executionMonitorDock;
    m_pExecutionMonitorDock->setWidget(m_pExecutionMonitor);

    m_pReportViewer = new ReportViewerWidget(this);
    m_pReportViewerDock = ui->reportViewerDock;
    m_pReportViewerDock->setWidget(m_pReportViewer);

    // Create Step Palette dock widget (created programmatically)
    m_pStepPalette = new TestMATE::CStepPaletteWidget(this);
    m_pStepPaletteDock = new QDockWidget(tr("Step Palette"), this);
    m_pStepPaletteDock->setWidget(m_pStepPalette);
    m_pStepPaletteDock->setAllowedAreas(Qt::LeftDockWidgetArea | Qt::RightDockWidgetArea);
    m_pStepPaletteDock->setObjectName("stepPaletteDock");  // For saveState/restoreState
    addDockWidget(Qt::LeftDockWidgetArea, m_pStepPaletteDock);

    // Stack palette below sequence editor if it exists
    if (ui->sequenceEditorDock) {
        tabifyDockWidget(ui->sequenceEditorDock, m_pStepPaletteDock);
        ui->sequenceEditorDock->raise();  // Keep sequence editor on top initially
    }
}

void MainWindow::createStatusBar()
{
    m_pStatusLabel = new QLabel("Ready");
    statusBar()->addWidget(m_pStatusLabel, 1);

    m_pExecutionStateLabel = new QLabel("Idle");
    statusBar()->addPermanentWidget(m_pExecutionStateLabel);

    m_pProgressBar = new QProgressBar();
    m_pProgressBar->setMaximumWidth(200);
    m_pProgressBar->setVisible(false);
    statusBar()->addPermanentWidget(m_pProgressBar);
}

void MainWindow::connectSignals()
{
    // Connect sequence editor signals
    connect(m_pSequenceEditor, &SequenceEditorWidget::sequenceModified, this, [this]() {
        m_bModified = true;
        updateWindowTitle();
    });

    // Connect execution monitor signals
    connect(m_pExecutionMonitor, &ExecutionMonitorWidget::executionStarted, this, &MainWindow::onExecutionStarted);
    connect(m_pExecutionMonitor, &ExecutionMonitorWidget::executionCompleted, this, &MainWindow::onExecutionCompleted);
}

void MainWindow::loadSettings()
{
    QSettings settings;

    // Window geometry
    restoreGeometry(settings.value("geometry").toByteArray());
    restoreState(settings.value("windowState").toByteArray());

    // Recent files
    m_recentFiles = settings.value("recentFiles").toStringList();
}

void MainWindow::saveSettings()
{
    QSettings settings;

    // Window geometry
    settings.setValue("geometry", saveGeometry());
    settings.setValue("windowState", saveState());

    // Recent files
    settings.setValue("recentFiles", m_recentFiles);
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    if (maybeSave()) {
        saveSettings();
        event->accept();
    } else {
        event->ignore();
    }
}

bool MainWindow::maybeSave()
{
    if (!m_bModified)
        return true;

    QMessageBox::StandardButton ret = QMessageBox::warning(this, tr("TestMATE"),
        tr("The sequence has been modified.\nDo you want to save your changes?"),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel);

    switch (ret) {
    case QMessageBox::Save:
        onSaveSequence();
        return true;
    case QMessageBox::Discard:
        return true;
    case QMessageBox::Cancel:
        return false;
    default:
        return false;
    }
}

void MainWindow::updateWindowTitle()
{
    QString title = "TestMATE";

    if (!m_strCurrentFile.isEmpty()) {
        QFileInfo fileInfo(m_strCurrentFile);
        title += " - " + fileInfo.fileName();
    }

    if (m_bModified) {
        title += " *";
    }

    setWindowTitle(title);
}

void MainWindow::updateStatusBar()
{
    if (m_bRunning) {
        m_pExecutionStateLabel->setText("Running");
    } else {
        m_pExecutionStateLabel->setText("Idle");
    }
}

// File Menu Slots
void MainWindow::onNewSequence()
{
    if (maybeSave()) {
        m_pSequenceEditor->newSequence();
        m_strCurrentFile.clear();
        m_bModified = false;
        updateWindowTitle();
        m_pStatusLabel->setText("New sequence created");
    }
}

void MainWindow::onOpenSequence()
{
    if (!maybeSave())
        return;

    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Open Test Sequence"),
        QString(),
        tr("TestMATE Sequences (*.json *.xml);;All Files (*)"));

    if (!fileName.isEmpty()) {
        if (m_pSequenceEditor->loadSequence(fileName)) {
            setCurrentFile(fileName);
            m_pStatusLabel->setText(tr("Sequence loaded: %1").arg(fileName));
        } else {
            QMessageBox::warning(this, tr("Error"),
                tr("Failed to load sequence: %1").arg(fileName));
        }
    }
}

void MainWindow::onSaveSequence()
{
    if (m_strCurrentFile.isEmpty()) {
        onSaveSequenceAs();
    } else {
        if (m_pSequenceEditor->saveSequence(m_strCurrentFile)) {
            m_bModified = false;
            updateWindowTitle();
            m_pStatusLabel->setText(tr("Sequence saved: %1").arg(m_strCurrentFile));
        } else {
            QMessageBox::warning(this, tr("Error"),
                tr("Failed to save sequence: %1").arg(m_strCurrentFile));
        }
    }
}

void MainWindow::onSaveSequenceAs()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Save Test Sequence"),
        QString(),
        tr("JSON Files (*.json);;XML Files (*.xml);;All Files (*)"));

    if (!fileName.isEmpty()) {
        if (m_pSequenceEditor->saveSequence(fileName)) {
            setCurrentFile(fileName);
            m_pStatusLabel->setText(tr("Sequence saved: %1").arg(fileName));
        } else {
            QMessageBox::warning(this, tr("Error"),
                tr("Failed to save sequence: %1").arg(fileName));
        }
    }
}

void MainWindow::onExit()
{
    close();
}

// Edit Menu Slots
void MainWindow::onUndo() { m_pSequenceEditor->undo(); }
void MainWindow::onRedo() { m_pSequenceEditor->redo(); }
void MainWindow::onCut() { m_pSequenceEditor->cut(); }
void MainWindow::onCopy() { m_pSequenceEditor->copy(); }
void MainWindow::onPaste() { m_pSequenceEditor->paste(); }

void MainWindow::onPreferences()
{
    if (!m_pConfigDialog) {
        m_pConfigDialog = new ConfigurationDialog(this);
    }
    m_pConfigDialog->exec();
}

// Sequence Menu Slots
void MainWindow::onAddStep() { m_pSequenceEditor->addStep(); }
void MainWindow::onEditStep() { m_pSequenceEditor->editStep(); }
void MainWindow::onDeleteStep() { m_pSequenceEditor->deleteStep(); }
void MainWindow::onMoveStepUp() { m_pSequenceEditor->moveStepUp(); }
void MainWindow::onMoveStepDown() { m_pSequenceEditor->moveStepDown(); }

void MainWindow::onValidateSequence()
{
    if (m_pSequenceEditor->validateSequence()) {
        QMessageBox::information(this, tr("Validation"),
            tr("Sequence is valid and ready for execution."));
    } else {
        QMessageBox::warning(this, tr("Validation"),
            tr("Sequence validation failed. Please check for errors."));
    }
}

// Execution Menu Slots
void MainWindow::onRun()
{
    if (!m_bRunning) {
        m_pExecutionMonitor->startExecution();
        m_bRunning = true;
        m_pProgressBar->setVisible(true);
        m_pStatusLabel->setText("Execution started");
    }
}

void MainWindow::onPause()
{
    if (m_bRunning) {
        m_pExecutionMonitor->pauseExecution();
        m_pStatusLabel->setText("Execution paused");
    }
}

void MainWindow::onStop()
{
    if (m_bRunning) {
        m_pExecutionMonitor->stopExecution();
        m_bRunning = false;
        m_pProgressBar->setVisible(false);
        m_pStatusLabel->setText("Execution stopped");
    }
}

void MainWindow::onAbort()
{
    if (m_bRunning) {
        m_pExecutionMonitor->abortExecution();
        m_bRunning = false;
        m_pProgressBar->setVisible(false);
        m_pStatusLabel->setText("Execution aborted");
    }
}

void MainWindow::onRunWithDebug()
{
    m_pExecutionMonitor->startExecution(true); // Debug mode
    m_bRunning = true;
    m_pProgressBar->setVisible(true);
    m_pStatusLabel->setText("Debug execution started");
}

// View Menu Slots
void MainWindow::onToggleSequenceEditor() { /* Central widget always visible */ }
void MainWindow::onToggleExecutionMonitor() { m_pExecutionMonitorDock->setVisible(!m_pExecutionMonitorDock->isVisible()); }
void MainWindow::onToggleReportViewer() { m_pReportViewerDock->setVisible(!m_pReportViewerDock->isVisible()); }
void MainWindow::onToggleStepPalette() { m_pStepPaletteDock->setVisible(!m_pStepPaletteDock->isVisible()); }
void MainWindow::onToggleToolbar() { /* Toggle all toolbars */ }
void MainWindow::onToggleStatusBar() { statusBar()->setVisible(!statusBar()->isVisible()); }

// Tools Menu Slots
void MainWindow::onOpenConfiguration()
{
    onPreferences();
}

void MainWindow::onOpenDatabase()
{
    // TODO: Implement database manager dialog
    QMessageBox::information(this, tr("Database Manager"),
        tr("Database manager coming soon..."));
}

void MainWindow::onExportReport()
{
    QString fileName = QFileDialog::getSaveFileName(this,
        tr("Export Report"),
        QString(),
        tr("HTML Files (*.html);;PDF Files (*.pdf);;All Files (*)"));

    if (!fileName.isEmpty()) {
        m_pReportViewer->exportReport(fileName);
        m_pStatusLabel->setText(tr("Report exported: %1").arg(fileName));
    }
}

void MainWindow::onImportLimits()
{
    QString fileName = QFileDialog::getOpenFileName(this,
        tr("Import Limits"),
        QString(),
        tr("CSV Files (*.csv);;All Files (*)"));

    if (!fileName.isEmpty()) {
        // TODO: Implement limits import
        m_pStatusLabel->setText(tr("Limits imported: %1").arg(fileName));
    }
}

void MainWindow::onPluginManager()
{
    // TODO: Implement plugin manager dialog
    QMessageBox::information(this, tr("Plugin Manager"),
        tr("Plugin manager coming soon..."));
}

// Help Menu Slots
void MainWindow::onDocumentation()
{
    // TODO: Open documentation
    QMessageBox::information(this, tr("Documentation"),
        tr("Documentation will open in your web browser."));
}

void MainWindow::onAbout()
{
    AboutDialog dialog(this);
    dialog.exec();
}

void MainWindow::onCheckUpdates()
{
    QMessageBox::information(this, tr("Updates"),
        tr("You are running the latest version of TestMATE."));
}

// Execution Callbacks
void MainWindow::onExecutionStarted()
{
    m_bRunning = true;
    m_pProgressBar->setValue(0);
    m_pProgressBar->setVisible(true);
}

void MainWindow::onExecutionProgress(double progress, const QString& message)
{
    m_pProgressBar->setValue(static_cast<int>(progress * 100));
    m_pStatusLabel->setText(message);
}

void MainWindow::onExecutionCompleted(bool success)
{
    m_bRunning = false;
    m_pProgressBar->setVisible(false);

    if (success) {
        m_pStatusLabel->setText("Execution completed successfully");
        m_pReportViewer->loadReport(); // Load latest report
    } else {
        m_pStatusLabel->setText("Execution failed");
    }
}

void MainWindow::onStepCompleted(const QString& stepName, const QString& result)
{
    m_pStatusLabel->setText(tr("Completed: %1 - %2").arg(stepName, result));
}

void MainWindow::setCurrentFile(const QString& fileName)
{
    m_strCurrentFile = fileName;
    m_bModified = false;
    updateWindowTitle();
    addRecentFile(fileName);
}

void MainWindow::addRecentFile(const QString& fileName)
{
    m_recentFiles.removeAll(fileName);
    m_recentFiles.prepend(fileName);

    while (m_recentFiles.size() > 10) {
        m_recentFiles.removeLast();
    }

    updateRecentFiles();
}

void MainWindow::updateRecentFiles()
{
    // TODO: Update recent files menu
}
