#include "ReportViewerWidget.h"
#include "ui_ReportViewerWidget.h"
#include <QFile>
#include <QTextStream>

ReportViewerWidget::ReportViewerWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::ReportViewerWidget)
{
    ui->setupUi(this);
}

ReportViewerWidget::~ReportViewerWidget()
{
    delete ui;
}

void ReportViewerWidget::loadReport()
{
    ui->textBrowser->setHtml("<h1>Test Report</h1><p>Report content will appear here.</p>");
}

void ReportViewerWidget::exportReport(const QString& fileName)
{
    if (fileName.isEmpty()) {
        return;
    }

    // Get the current HTML content from the report viewer
    QString htmlContent = ui->textBrowser->toHtml();

    // Export to file
    QFile file(fileName);
    if (file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        QTextStream out(&file);
        out << htmlContent;
        file.close();
    }
    // Note: Production code would handle errors and show user feedback
}
