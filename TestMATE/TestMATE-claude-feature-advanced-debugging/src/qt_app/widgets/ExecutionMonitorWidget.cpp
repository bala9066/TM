/**************************************************************************
 * File Name: ExecutionMonitorWidget.cpp
 **************************************************************************/

#include "ExecutionMonitorWidget.h"
#include "ui_ExecutionMonitorWidget.h"
#include "models/ResultsTableModel.h"

ExecutionMonitorWidget::ExecutionMonitorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ExecutionMonitorWidget)
    , m_pResultsModel(new ResultsTableModel(this))
    , m_bRunning(false)
{
    ui->setupUi(this);
    ui->resultsTable->setModel(m_pResultsModel);
}

ExecutionMonitorWidget::~ExecutionMonitorWidget()
{
    delete ui;
}

void ExecutionMonitorWidget::startExecution(bool debugMode)
{
    m_bRunning = true;
    m_pResultsModel->clearResults();
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

void ExecutionMonitorWidget::addStepResult(const QString &stepName,
                                           const QString &verdict,
                                           const QString &detail)
{
    m_pResultsModel->addResult(stepName, verdict, detail);
    emit stepCompleted(stepName, verdict);
}

void ExecutionMonitorWidget::setProgress(double progress)
{
    ui->progressBar->setValue(static_cast<int>(progress * 100));
}
