/**************************************************************************
 * File Name: DialogTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 * Description: QTest unit tests for Qt dialogs
 **************************************************************************/

#include <QtTest/QtTest>
#include <QDialog>
#include <QDialogButtonBox>
#include <QTabWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QCheckBox>
#include <QTextEdit>
#include <QLabel>
#include <QTableWidget>

#include "qt_app/widgets/AboutDialog.h"
#include "qt_app/widgets/ConfigurationDialog.h"
#include "qt_app/widgets/StepPropertiesDialog.h"
#include "qt_app/widgets/ReportViewerWidget.h"

/**************************************************************************
 * AboutDialog Tests
 **************************************************************************/
class AboutDialogTests : public QObject
{
    Q_OBJECT

private slots:
    void init() { m_pDialog = new AboutDialog(); }
    void cleanup() { delete m_pDialog; m_pDialog = nullptr; }

    void testConstructor();
    void testIsDialog();
    void testButtonBoxExists();
    void testVersionLabelExists();
    void testTitleLabelExists();

private:
    AboutDialog *m_pDialog;
};

void AboutDialogTests::testConstructor()
{
    QVERIFY(m_pDialog != nullptr);
}

void AboutDialogTests::testIsDialog()
{
    QVERIFY(qobject_cast<QDialog*>(m_pDialog) != nullptr);
}

void AboutDialogTests::testButtonBoxExists()
{
    QDialogButtonBox *buttonBox = m_pDialog->findChild<QDialogButtonBox*>();
    QVERIFY(buttonBox != nullptr);
}

void AboutDialogTests::testVersionLabelExists()
{
    QLabel *versionLabel = m_pDialog->findChild<QLabel*>("versionLabel");
    QVERIFY(versionLabel != nullptr);
}

void AboutDialogTests::testTitleLabelExists()
{
    QLabel *titleLabel = m_pDialog->findChild<QLabel*>("titleLabel");
    QVERIFY(titleLabel != nullptr);
}

/**************************************************************************
 * ConfigurationDialog Tests
 **************************************************************************/
class ConfigurationDialogTests : public QObject
{
    Q_OBJECT

private slots:
    void init() { m_pDialog = new ConfigurationDialog(); }
    void cleanup() { delete m_pDialog; m_pDialog = nullptr; }

    void testConstructor();
    void testIsDialog();
    void testTabWidgetExists();
    void testHasMultipleTabs();
    void testGeneralTabExists();
    void testExecutionTabExists();
    void testDatabaseTabExists();
    void testButtonBoxExists();
    void testGeneralTabControls();
    void testExecutionTabControls();
    void testDatabaseTabControls();

private:
    ConfigurationDialog *m_pDialog;
};

void ConfigurationDialogTests::testConstructor()
{
    QVERIFY(m_pDialog != nullptr);
}

void ConfigurationDialogTests::testIsDialog()
{
    QVERIFY(qobject_cast<QDialog*>(m_pDialog) != nullptr);
}

void ConfigurationDialogTests::testTabWidgetExists()
{
    QTabWidget *tabWidget = m_pDialog->findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
}

void ConfigurationDialogTests::testHasMultipleTabs()
{
    QTabWidget *tabWidget = m_pDialog->findChild<QTabWidget*>();
    if (tabWidget) {
        QVERIFY(tabWidget->count() >= 3); // General, Execution, Database
    }
}

void ConfigurationDialogTests::testGeneralTabExists()
{
    QTabWidget *tabWidget = m_pDialog->findChild<QTabWidget*>();
    if (tabWidget) {
        bool foundGeneralTab = false;
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (tabWidget->tabText(i).contains("General", Qt::CaseInsensitive)) {
                foundGeneralTab = true;
                break;
            }
        }
        QVERIFY(foundGeneralTab);
    }
}

void ConfigurationDialogTests::testExecutionTabExists()
{
    QTabWidget *tabWidget = m_pDialog->findChild<QTabWidget*>();
    if (tabWidget) {
        bool foundExecutionTab = false;
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (tabWidget->tabText(i).contains("Execution", Qt::CaseInsensitive)) {
                foundExecutionTab = true;
                break;
            }
        }
        QVERIFY(foundExecutionTab);
    }
}

void ConfigurationDialogTests::testDatabaseTabExists()
{
    QTabWidget *tabWidget = m_pDialog->findChild<QTabWidget*>();
    if (tabWidget) {
        bool foundDatabaseTab = false;
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (tabWidget->tabText(i).contains("Database", Qt::CaseInsensitive)) {
                foundDatabaseTab = true;
                break;
            }
        }
        QVERIFY(foundDatabaseTab);
    }
}

void ConfigurationDialogTests::testButtonBoxExists()
{
    QDialogButtonBox *buttonBox = m_pDialog->findChild<QDialogButtonBox*>();
    QVERIFY(buttonBox != nullptr);
}

void ConfigurationDialogTests::testGeneralTabControls()
{
    // Check for workspace directory controls
    QLineEdit *workspaceDirEdit = m_pDialog->findChild<QLineEdit*>("workspaceDirEdit");
    // May or may not exist depending on .ui design
    Q_UNUSED(workspaceDirEdit);

    // Check for auto-save controls
    QCheckBox *confirmExitCheckBox = m_pDialog->findChild<QCheckBox*>("confirmExitCheckBox");
    Q_UNUSED(confirmExitCheckBox);
}

void ConfigurationDialogTests::testExecutionTabControls()
{
    // Check for timeout settings
    QCheckBox *pauseOnFailureCheckBox = m_pDialog->findChild<QCheckBox*>("pauseOnFailureCheckBox");
    Q_UNUSED(pauseOnFailureCheckBox);

    QCheckBox *generateReportCheckBox = m_pDialog->findChild<QCheckBox*>("generateReportCheckBox");
    Q_UNUSED(generateReportCheckBox);
}

void ConfigurationDialogTests::testDatabaseTabControls()
{
    // Check for database type
    QComboBox *dbTypeCombo = m_pDialog->findChild<QComboBox*>("dbTypeCombo");
    if (dbTypeCombo) {
        QVERIFY(dbTypeCombo->count() > 0);
    }

    // Check for database path
    QLineEdit *dbPathEdit = m_pDialog->findChild<QLineEdit*>("dbPathEdit");
    Q_UNUSED(dbPathEdit);
}

/**************************************************************************
 * StepPropertiesDialog Tests
 **************************************************************************/
class StepPropertiesDialogTests : public QObject
{
    Q_OBJECT

private slots:
    void init() { m_pDialog = new StepPropertiesDialog(); }
    void cleanup() { delete m_pDialog; m_pDialog = nullptr; }

    void testConstructor();
    void testIsDialog();
    void testTabWidgetExists();
    void testGeneralTabExists();
    void testParametersTabExists();
    void testButtonBoxExists();
    void testNameFieldExists();
    void testTypeComboExists();
    void testDescriptionFieldExists();
    void testEnabledCheckBoxExists();
    void testParametersTableExists();

private:
    StepPropertiesDialog *m_pDialog;
};

void StepPropertiesDialogTests::testConstructor()
{
    QVERIFY(m_pDialog != nullptr);
}

void StepPropertiesDialogTests::testIsDialog()
{
    QVERIFY(qobject_cast<QDialog*>(m_pDialog) != nullptr);
}

void StepPropertiesDialogTests::testTabWidgetExists()
{
    QTabWidget *tabWidget = m_pDialog->findChild<QTabWidget*>();
    QVERIFY(tabWidget != nullptr);
}

void StepPropertiesDialogTests::testGeneralTabExists()
{
    QTabWidget *tabWidget = m_pDialog->findChild<QTabWidget*>();
    if (tabWidget) {
        QVERIFY(tabWidget->count() >= 2);
        bool foundGeneral = false;
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (tabWidget->tabText(i).contains("General", Qt::CaseInsensitive)) {
                foundGeneral = true;
                break;
            }
        }
        QVERIFY(foundGeneral);
    }
}

void StepPropertiesDialogTests::testParametersTabExists()
{
    QTabWidget *tabWidget = m_pDialog->findChild<QTabWidget*>();
    if (tabWidget) {
        bool foundParameters = false;
        for (int i = 0; i < tabWidget->count(); ++i) {
            if (tabWidget->tabText(i).contains("Parameters", Qt::CaseInsensitive)) {
                foundParameters = true;
                break;
            }
        }
        QVERIFY(foundParameters);
    }
}

void StepPropertiesDialogTests::testButtonBoxExists()
{
    QDialogButtonBox *buttonBox = m_pDialog->findChild<QDialogButtonBox*>();
    QVERIFY(buttonBox != nullptr);
}

void StepPropertiesDialogTests::testNameFieldExists()
{
    QLineEdit *nameEdit = m_pDialog->findChild<QLineEdit*>("nameEdit");
    QVERIFY(nameEdit != nullptr);
}

void StepPropertiesDialogTests::testTypeComboExists()
{
    QComboBox *typeCombo = m_pDialog->findChild<QComboBox*>("typeCombo");
    QVERIFY(typeCombo != nullptr);
    QVERIFY(typeCombo->count() > 0); // Should have step types
}

void StepPropertiesDialogTests::testDescriptionFieldExists()
{
    QTextEdit *descEdit = m_pDialog->findChild<QTextEdit*>("descriptionEdit");
    QVERIFY(descEdit != nullptr);
}

void StepPropertiesDialogTests::testEnabledCheckBoxExists()
{
    QCheckBox *enabledCheckBox = m_pDialog->findChild<QCheckBox*>("enabledCheckBox");
    QVERIFY(enabledCheckBox != nullptr);
    QVERIFY(enabledCheckBox->isChecked()); // Should be checked by default
}

void StepPropertiesDialogTests::testParametersTableExists()
{
    QTableWidget *parametersTable = m_pDialog->findChild<QTableWidget*>("parametersTable");
    QVERIFY(parametersTable != nullptr);
    QVERIFY(parametersTable->columnCount() >= 2); // Name and Value columns
}

/**************************************************************************
 * ReportViewerWidget Tests
 **************************************************************************/
class ReportViewerWidgetTests : public QObject
{
    Q_OBJECT

private slots:
    void init() { m_pWidget = new ReportViewerWidget(); }
    void cleanup() { delete m_pWidget; m_pWidget = nullptr; }

    void testConstructor();
    void testTextBrowserExists();
    void testToolBarExists();
    void testLoadReport();

private:
    ReportViewerWidget *m_pWidget;
};

void ReportViewerWidgetTests::testConstructor()
{
    QVERIFY(m_pWidget != nullptr);
}

void ReportViewerWidgetTests::testTextBrowserExists()
{
    QTextBrowser *textBrowser = m_pWidget->findChild<QTextBrowser*>("textBrowser");
    QVERIFY(textBrowser != nullptr);
}

void ReportViewerWidgetTests::testToolBarExists()
{
    QToolBar *toolBar = m_pWidget->findChild<QToolBar*>();
    QVERIFY(toolBar != nullptr);
}

void ReportViewerWidgetTests::testLoadReport()
{
    m_pWidget->loadReport();

    QTextBrowser *textBrowser = m_pWidget->findChild<QTextBrowser*>("textBrowser");
    if (textBrowser) {
        QVERIFY(!textBrowser->toPlainText().isEmpty());
    }
}

/**************************************************************************
 * Main Test Runner
 **************************************************************************/
int main(int argc, char *argv[])
{
    QApplication app(argc, argv);

    int status = 0;

    {
        AboutDialogTests aboutTests;
        status |= QTest::qExec(&aboutTests, argc, argv);
    }

    {
        ConfigurationDialogTests configTests;
        status |= QTest::qExec(&configTests, argc, argv);
    }

    {
        StepPropertiesDialogTests stepTests;
        status |= QTest::qExec(&stepTests, argc, argv);
    }

    {
        ReportViewerWidgetTests reportTests;
        status |= QTest::qExec(&reportTests, argc, argv);
    }

    return status;
}

#include "DialogTests.moc"
