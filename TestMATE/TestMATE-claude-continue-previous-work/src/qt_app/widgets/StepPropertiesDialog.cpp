#include "StepPropertiesDialog.h"
#include "ui_StepPropertiesDialog.h"

StepPropertiesDialog::StepPropertiesDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::StepPropertiesDialog)
{
    ui->setupUi(this);
}

StepPropertiesDialog::~StepPropertiesDialog()
{
    delete ui;
}
