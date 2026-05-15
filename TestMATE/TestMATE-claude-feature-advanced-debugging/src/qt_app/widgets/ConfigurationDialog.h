#pragma once
#include <QDialog>

namespace Ui {
class ConfigurationDialog;
}

class ConfigurationDialog : public QDialog
{
    Q_OBJECT
public:
    explicit ConfigurationDialog(QWidget *parent = nullptr);
    ~ConfigurationDialog();

private:
    Ui::ConfigurationDialog *ui;
};
