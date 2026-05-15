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

signals:
    void executionStarted();
    void executionCompleted(bool success);
    void stepCompleted(const QString& stepName, const QString& result);

private:
    void updateProgress(double progress);

private:
    Ui::ExecutionMonitorWidget *ui;
    bool m_bRunning;
};
