/**************************************************************************
 * File Name: ExecutionMonitorWidget.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 * Description: Real-time execution monitoring widget
 **************************************************************************/

#pragma once

#include <QWidget>

namespace Ui {
class ExecutionMonitorWidget;
}

class ResultsTableModel;

class ExecutionMonitorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit ExecutionMonitorWidget(QWidget *parent = nullptr);
    ~ExecutionMonitorWidget();

    void startExecution(bool debugMode = false);
    void pauseExecution();
    void stopExecution();
    void abortExecution();

    /// Append a finished step to the results table and emit stepCompleted.
    void addStepResult(const QString &stepName, const QString &verdict,
                       const QString &detail = QString());

    /// Update the progress bar (0.0 - 1.0).
    void setProgress(double progress);

signals:
    void executionStarted();
    void executionCompleted(bool success);
    void stepCompleted(const QString& stepName, const QString& result);

private:
    Ui::ExecutionMonitorWidget *ui;
    ResultsTableModel *m_pResultsModel;
    bool m_bRunning;
};
