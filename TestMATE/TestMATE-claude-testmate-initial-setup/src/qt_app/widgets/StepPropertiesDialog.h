#pragma once
#include <QDialog>

namespace Ui {
class StepPropertiesDialog;
}

class StepPropertiesDialog : public QDialog
{
    Q_OBJECT
public:
    explicit StepPropertiesDialog(QWidget *parent = nullptr);
    ~StepPropertiesDialog();

private:
    Ui::StepPropertiesDialog *ui;
};
