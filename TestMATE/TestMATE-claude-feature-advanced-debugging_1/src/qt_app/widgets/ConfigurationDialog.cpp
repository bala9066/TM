#include "ConfigurationDialog.h"
#include "ui_ConfigurationDialog.h"

ConfigurationDialog::ConfigurationDialog(QWidget *parent)
    : QDialog(parent)
    , ui(new Ui::ConfigurationDialog)
{
    ui->setupUi(this);
}

ConfigurationDialog::~ConfigurationDialog()
{
    delete ui;
}
