/**************************************************************************
 * File Name: ExecutionMonitorWidget.cpp
 **************************************************************************/

#include "ExecutionMonitorWidget.h"
#include "ui_ExecutionMonitorWidget.h"

ExecutionMonitorWidget::ExecutionMonitorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ExecutionMonitorWidget)
    , m_bRunning(false)
{
    ui->setupUi(this);
}

ExecutionMonitorWidget::~ExecutionMonitorWidget()
{
    delete ui;
}

void ExecutionMonitorWidget::startExecution(bool debugMode)
{
    m_bRunning = true;
    ui->statusLabel->setText(debugMode ? tr("Debug Mode Running") : tr("Running"));
    ui->pauseButton->setEnabled(true);
    ui->stopButton->setEnabled(true);
    ui->progressBar->setValue(0);
    emit executionStarted();
}

void ExecutionMonitorWidget::pauseExecution()
{
    ui->statusLabel->setText(tr("Paused"));
}

void ExecutionMonitorWidget::stopExecution()
{
    m_bRunning = false;
    ui->statusLabel->setText(tr("Stopped"));
    ui->pauseButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
    emit executionCompleted(true);
}

void ExecutionMonitorWidget::abortExecution()
{
    m_bRunning = false;
    ui->statusLabel->setText(tr("Aborted"));
    ui->pauseButton->setEnabled(false);
    ui->stopButton->setEnabled(false);
    emit executionCompleted(false);
}

void ExecutionMonitorWidget::updateProgress(double progress)
{
    ui->progressBar->setValue(static_cast<int>(progress * 100));
}
