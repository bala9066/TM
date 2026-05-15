/**************************************************************************
 * File Name: ExecutionMonitorWidgetTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 * Description: QTest unit tests for ExecutionMonitorWidget
 **************************************************************************/

#include <QtTest/QtTest>
#include <QProgressBar>
#include <QLabel>
#include <QPushButton>
#include <QTableView>
#include <QSignalSpy>

#include "qt_app/widgets/ExecutionMonitorWidget.h"

class ExecutionMonitorWidgetTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Constructor tests
    void testConstructor();
    void testUIInitialization();

    // UI component tests
    void testStatusLabelExists();
    void testProgressBarExists();
    void testControlButtonsExist();
    void testResultsTableExists();

    // Functionality tests
    void testStartExecution();
    void testPauseExecution();
    void testStopExecution();
    void testAbortExecution();
    void testDebugMode();

    // Signal tests
    void testExecutionStartedSignal();
    void testExecutionCompletedSignal();
    void testStepCompletedSignal();

    // State tests
    void testInitialState();
    void testRunningState();
    void testPausedState();
    void testStoppedState();

    // Button state tests
    void testButtonStatesWhenIdle();
    void testButtonStatesWhenRunning();

private:
    ExecutionMonitorWidget *m_pWidget;
};

void ExecutionMonitorWidgetTests::initTestCase()
{
    qDebug() << "ExecutionMonitorWidget test suite starting...";
}

void ExecutionMonitorWidgetTests::cleanupTestCase()
{
    qDebug() << "ExecutionMonitorWidget test suite finished.";
}

void ExecutionMonitorWidgetTests::init()
{
    m_pWidget = new ExecutionMonitorWidget();
}

void ExecutionMonitorWidgetTests::cleanup()
{
    delete m_pWidget;
    m_pWidget = nullptr;
}

void ExecutionMonitorWidgetTests::testConstructor()
{
    QVERIFY(m_pWidget != nullptr);
    QVERIFY(m_pWidget->isWidgetType());
}

void ExecutionMonitorWidgetTests::testUIInitialization()
{
    QVERIFY(m_pWidget->layout() != nullptr);
}

void ExecutionMonitorWidgetTests::testStatusLabelExists()
{
    QLabel *statusLabel = m_pWidget->findChild<QLabel*>("statusLabel");
    QVERIFY(statusLabel != nullptr);
    QVERIFY(!statusLabel->text().isEmpty());
}

void ExecutionMonitorWidgetTests::testProgressBarExists()
{
    QProgressBar *progressBar = m_pWidget->findChild<QProgressBar*>("progressBar");
    QVERIFY(progressBar != nullptr);
    QCOMPARE(progressBar->value(), 0); // Should start at 0
}

void ExecutionMonitorWidgetTests::testControlButtonsExist()
{
    QPushButton *pauseButton = m_pWidget->findChild<QPushButton*>("pauseButton");
    QVERIFY(pauseButton != nullptr);

    QPushButton *stopButton = m_pWidget->findChild<QPushButton*>("stopButton");
    QVERIFY(stopButton != nullptr);
}

void ExecutionMonitorWidgetTests::testResultsTableExists()
{
    QTableView *resultsTable = m_pWidget->findChild<QTableView*>("resultsTable");
    QVERIFY(resultsTable != nullptr);
}

void ExecutionMonitorWidgetTests::testStartExecution()
{
    QSignalSpy spy(m_pWidget, SIGNAL(executionStarted()));

    m_pWidget->startExecution(false);

    // Signal should be emitted
    QCOMPARE(spy.count(), 1);

    // Status label should change
    QLabel *statusLabel = m_pWidget->findChild<QLabel*>("statusLabel");
    if (statusLabel) {
        QVERIFY(statusLabel->text().contains("Running") ||
                statusLabel->text().contains("started", Qt::CaseInsensitive));
    }

    // Progress bar should be reset
    QProgressBar *progressBar = m_pWidget->findChild<QProgressBar*>("progressBar");
    if (progressBar) {
        QCOMPARE(progressBar->value(), 0);
    }

    // Buttons should be enabled
    QPushButton *pauseButton = m_pWidget->findChild<QPushButton*>("pauseButton");
    QPushButton *stopButton = m_pWidget->findChild<QPushButton*>("stopButton");

    if (pauseButton) QVERIFY(pauseButton->isEnabled());
    if (stopButton) QVERIFY(stopButton->isEnabled());
}

void ExecutionMonitorWidgetTests::testPauseExecution()
{
    // Start first
    m_pWidget->startExecution(false);

    // Then pause
    m_pWidget->pauseExecution();

    QLabel *statusLabel = m_pWidget->findChild<QLabel*>("statusLabel");
    if (statusLabel) {
        QVERIFY(statusLabel->text().contains("Pause", Qt::CaseInsensitive));
    }
}

void ExecutionMonitorWidgetTests::testStopExecution()
{
    QSignalSpy spy(m_pWidget, SIGNAL(executionCompleted(bool)));

    // Start first
    m_pWidget->startExecution(false);

    // Then stop
    m_pWidget->stopExecution();

    // Signal should be emitted with success=true
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == true);

    // Buttons should be disabled
    QPushButton *pauseButton = m_pWidget->findChild<QPushButton*>("pauseButton");
    QPushButton *stopButton = m_pWidget->findChild<QPushButton*>("stopButton");

    if (pauseButton) QVERIFY(!pauseButton->isEnabled());
    if (stopButton) QVERIFY(!stopButton->isEnabled());
}

void ExecutionMonitorWidgetTests::testAbortExecution()
{
    QSignalSpy spy(m_pWidget, SIGNAL(executionCompleted(bool)));

    // Start first
    m_pWidget->startExecution(false);

    // Then abort
    m_pWidget->abortExecution();

    // Signal should be emitted with success=false
    QCOMPARE(spy.count(), 1);
    QList<QVariant> arguments = spy.takeFirst();
    QVERIFY(arguments.at(0).toBool() == false);

    QLabel *statusLabel = m_pWidget->findChild<QLabel*>("statusLabel");
    if (statusLabel) {
        QVERIFY(statusLabel->text().contains("Abort", Qt::CaseInsensitive));
    }
}

void ExecutionMonitorWidgetTests::testDebugMode()
{
    QSignalSpy spy(m_pWidget, SIGNAL(executionStarted()));

    m_pWidget->startExecution(true); // Debug mode

    QCOMPARE(spy.count(), 1);

    QLabel *statusLabel = m_pWidget->findChild<QLabel*>("statusLabel");
    if (statusLabel) {
        QVERIFY(statusLabel->text().contains("Debug", Qt::CaseInsensitive));
    }
}

void ExecutionMonitorWidgetTests::testExecutionStartedSignal()
{
    const QMetaObject *metaObj = m_pWidget->metaObject();
    int signalIndex = metaObj->indexOfSignal("executionStarted()");
    QVERIFY(signalIndex >= 0);
}

void ExecutionMonitorWidgetTests::testExecutionCompletedSignal()
{
    const QMetaObject *metaObj = m_pWidget->metaObject();
    int signalIndex = metaObj->indexOfSignal("executionCompleted(bool)");
    QVERIFY(signalIndex >= 0);
}

void ExecutionMonitorWidgetTests::testStepCompletedSignal()
{
    const QMetaObject *metaObj = m_pWidget->metaObject();
    int signalIndex = metaObj->indexOfSignal("stepCompleted(QString,QString)");
    QVERIFY(signalIndex >= 0);
}

void ExecutionMonitorWidgetTests::testInitialState()
{
    QLabel *statusLabel = m_pWidget->findChild<QLabel*>("statusLabel");
    if (statusLabel) {
        QVERIFY(statusLabel->text().contains("Ready", Qt::CaseInsensitive) ||
                statusLabel->text().contains("Idle", Qt::CaseInsensitive));
    }

    QProgressBar *progressBar = m_pWidget->findChild<QProgressBar*>("progressBar");
    if (progressBar) {
        QCOMPARE(progressBar->value(), 0);
    }
}

void ExecutionMonitorWidgetTests::testRunningState()
{
    m_pWidget->startExecution(false);

    QPushButton *pauseButton = m_pWidget->findChild<QPushButton*>("pauseButton");
    QPushButton *stopButton = m_pWidget->findChild<QPushButton*>("stopButton");

    if (pauseButton) QVERIFY(pauseButton->isEnabled());
    if (stopButton) QVERIFY(stopButton->isEnabled());
}

void ExecutionMonitorWidgetTests::testPausedState()
{
    m_pWidget->startExecution(false);
    m_pWidget->pauseExecution();

    QLabel *statusLabel = m_pWidget->findChild<QLabel*>("statusLabel");
    if (statusLabel) {
        QString text = statusLabel->text();
        QVERIFY(text.contains("Pause", Qt::CaseInsensitive));
    }
}

void ExecutionMonitorWidgetTests::testStoppedState()
{
    m_pWidget->startExecution(false);
    m_pWidget->stopExecution();

    QPushButton *pauseButton = m_pWidget->findChild<QPushButton*>("pauseButton");
    QPushButton *stopButton = m_pWidget->findChild<QPushButton*>("stopButton");

    if (pauseButton) QVERIFY(!pauseButton->isEnabled());
    if (stopButton) QVERIFY(!stopButton->isEnabled());
}

void ExecutionMonitorWidgetTests::testButtonStatesWhenIdle()
{
    QPushButton *pauseButton = m_pWidget->findChild<QPushButton*>("pauseButton");
    QPushButton *stopButton = m_pWidget->findChild<QPushButton*>("stopButton");

    // Buttons should be disabled when idle
    if (pauseButton) QVERIFY(!pauseButton->isEnabled());
    if (stopButton) QVERIFY(!stopButton->isEnabled());
}

void ExecutionMonitorWidgetTests::testButtonStatesWhenRunning()
{
    m_pWidget->startExecution(false);

    QPushButton *pauseButton = m_pWidget->findChild<QPushButton*>("pauseButton");
    QPushButton *stopButton = m_pWidget->findChild<QPushButton*>("stopButton");

    // Buttons should be enabled when running
    if (pauseButton) QVERIFY(pauseButton->isEnabled());
    if (stopButton) QVERIFY(stopButton->isEnabled());
}

QTEST_MAIN(ExecutionMonitorWidgetTests)
#include "ExecutionMonitorWidgetTests.moc"
