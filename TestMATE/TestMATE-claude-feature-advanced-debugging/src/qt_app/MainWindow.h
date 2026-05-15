/**************************************************************************
 * File Name: MainWindow.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 * Description: Main window for TestMATE application
 **************************************************************************/

#pragma once

#include <QMainWindow>
#include <QTimer>
#include <memory>

// Forward declarations
class QDockWidget;
class QTabWidget;
class QToolBar;
class QStatusBar;
class QLabel;
class QProgressBar;

namespace TestMATE {
    class CTestMATECore;
}

namespace Ui {
class MainWindow;
}

class SequenceEditorWidget;
class ExecutionMonitorWidget;
class ReportViewerWidget;
class ConfigurationDialog;
class CStepPaletteWidget;

/**************************************************************************
 * Class: MainWindow
 * Description: Main application window with dockable widgets
 **************************************************************************/
class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    explicit MainWindow(QWidget *parent = nullptr);
    ~MainWindow();

protected:
    void closeEvent(QCloseEvent *event) override;

private slots:
    // File menu
    void onNewSequence();
    void onOpenSequence();
    void onSaveSequence();
    void onSaveSequenceAs();
    void onExit();

    // Edit menu
    void onUndo();
    void onRedo();
    void onCut();
    void onCopy();
    void onPaste();
    void onPreferences();

    // Sequence menu
    void onAddStep();
    void onEditStep();
    void onDeleteStep();
    void onMoveStepUp();
    void onMoveStepDown();
    void onValidateSequence();

    // Execution menu
    void onRun();
    void onPause();
    void onStop();
    void onAbort();
    void onRunWithDebug();

    // View menu
    void onToggleSequenceEditor();
    void onToggleExecutionMonitor();
    void onToggleReportViewer();
    void onToggleStepPalette();
    void onToggleToolbar();
    void onToggleStatusBar();

    // Tools menu
    void onOpenConfiguration();
    void onOpenDatabase();
    void onExportReport();
    void onImportLimits();
    void onPluginManager();

    // Help menu
    void onDocumentation();
    void onAbout();
    void onCheckUpdates();

    // Execution callbacks
    void onExecutionStarted();
    void onExecutionProgress(double progress, const QString& message);
    void onExecutionCompleted(bool success);
    void onStepCompleted(const QString& stepName, const QString& result);

    // Update UI
    void updateWindowTitle();
    void updateRecentFiles();
    void updateStatusBar();

private:
    void setupUI();
    void createActions();
    void createMenus();
    void createToolbars();
    void createDockWidgets();
    void createStatusBar();
    void connectSignals();
    void loadSettings();
    void saveSettings();

    bool maybeSave();
    void setCurrentFile(const QString& fileName);
    void addRecentFile(const QString& fileName);

private:
    Ui::MainWindow *ui;

    // Core engine
    std::unique_ptr<TestMATE::CTestMATECore> m_pCore;

    // Widgets
    SequenceEditorWidget *m_pSequenceEditor;
    ExecutionMonitorWidget *m_pExecutionMonitor;
    ReportViewerWidget *m_pReportViewer;
    CStepPaletteWidget *m_pStepPalette;

    // Dock widgets
    QDockWidget *m_pSequenceEditorDock;
    QDockWidget *m_pExecutionMonitorDock;
    QDockWidget *m_pReportViewerDock;
    QDockWidget *m_pStepPaletteDock;

    // Dialogs
    ConfigurationDialog *m_pConfigDialog;

    // Toolbars
    QToolBar *m_pFileToolBar;
    QToolBar *m_pEditToolBar;
    QToolBar *m_pExecutionToolBar;

    // Status bar widgets
    QLabel *m_pStatusLabel;
    QLabel *m_pExecutionStateLabel;
    QProgressBar *m_pProgressBar;

    // Current state
    QString m_strCurrentFile;
    QStringList m_recentFiles;
    bool m_bModified;
    bool m_bRunning;

    // Timer for UI updates
    QTimer m_updateTimer;
};
