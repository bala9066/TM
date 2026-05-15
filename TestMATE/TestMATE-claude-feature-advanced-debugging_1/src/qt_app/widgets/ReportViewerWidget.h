#pragma once
#include <QWidget>

namespace Ui {
class ReportViewerWidget;
}

class ReportViewerWidget : public QWidget
{
    Q_OBJECT
public:
    explicit ReportViewerWidget(QWidget *parent = nullptr);
    ~ReportViewerWidget();
    void loadReport();
    void exportReport(const QString& fileName);
private:
    Ui::ReportViewerWidget *ui;
};
