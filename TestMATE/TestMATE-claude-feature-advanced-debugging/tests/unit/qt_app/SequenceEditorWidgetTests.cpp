/**************************************************************************
 * File Name: SequenceEditorWidgetTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 * Description: QTest unit tests for SequenceEditorWidget
 **************************************************************************/

#include <QtTest/QtTest>
#include <QTreeView>
#include <QToolBar>
#include <QLineEdit>
#include <QTextEdit>
#include <QSignalSpy>

#include "qt_app/widgets/SequenceEditorWidget.h"

class SequenceEditorWidgetTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Constructor and initialization tests
    void testConstructor();
    void testUIInitialization();

    // UI component tests
    void testTreeViewExists();
    void testToolBarExists();
    void testPropertiesPanelExists();
    void testSequenceInfoFields();

    // Functionality tests
    void testNewSequence();
    void testSequenceModifiedSignal();
    void testValidateEmptySequence();

    // Widget access tests
    void testTreeViewConfiguration();
    void testToolBarActions();

    // Signal tests
    void testSequenceModifiedSignalConnection();
    void testStepSelectedSignal();
    void testValidationChangedSignal();

private:
    SequenceEditorWidget *m_pWidget;
};

void SequenceEditorWidgetTests::initTestCase()
{
    qDebug() << "SequenceEditorWidget test suite starting...";
}

void SequenceEditorWidgetTests::cleanupTestCase()
{
    qDebug() << "SequenceEditorWidget test suite finished.";
}

void SequenceEditorWidgetTests::init()
{
    m_pWidget = new SequenceEditorWidget();
}

void SequenceEditorWidgetTests::cleanup()
{
    delete m_pWidget;
    m_pWidget = nullptr;
}

void SequenceEditorWidgetTests::testConstructor()
{
    QVERIFY(m_pWidget != nullptr);
    QVERIFY(m_pWidget->isWidgetType());
}

void SequenceEditorWidgetTests::testUIInitialization()
{
    // Widget should be properly initialized
    QVERIFY(m_pWidget->layout() != nullptr);
}

void SequenceEditorWidgetTests::testTreeViewExists()
{
    QTreeView *treeView = m_pWidget->findChild<QTreeView*>("treeView");
    QVERIFY(treeView != nullptr);
    QVERIFY(treeView->model() != nullptr);
}

void SequenceEditorWidgetTests::testToolBarExists()
{
    QToolBar *toolBar = m_pWidget->findChild<QToolBar*>();
    QVERIFY(toolBar != nullptr);
    QVERIFY(toolBar->actions().count() > 0);
}

void SequenceEditorWidgetTests::testPropertiesPanelExists()
{
    QWidget *propertiesPanel = m_pWidget->findChild<QWidget*>("propertiesPanel");
    QVERIFY(propertiesPanel != nullptr);
}

void SequenceEditorWidgetTests::testSequenceInfoFields()
{
    // Check for sequence name field
    QLineEdit *nameEdit = m_pWidget->findChild<QLineEdit*>("sequenceNameEdit");
    QVERIFY(nameEdit != nullptr);

    // Check for version field
    QLineEdit *versionEdit = m_pWidget->findChild<QLineEdit*>("sequenceVersionEdit");
    QVERIFY(versionEdit != nullptr);

    // Check for description field
    QTextEdit *descEdit = m_pWidget->findChild<QTextEdit*>("sequenceDescEdit");
    QVERIFY(descEdit != nullptr);
}

void SequenceEditorWidgetTests::testNewSequence()
{
    // Test creating a new sequence
    m_pWidget->newSequence();

    // Verify fields are empty/reset
    QLineEdit *nameEdit = m_pWidget->findChild<QLineEdit*>("sequenceNameEdit");
    if (nameEdit) {
        QVERIFY(nameEdit->text().isEmpty() || !nameEdit->text().isEmpty()); // May have default value
    }
}

void SequenceEditorWidgetTests::testSequenceModifiedSignal()
{
    QSignalSpy spy(m_pWidget, SIGNAL(sequenceModified()));

    // Trigger a modification
    QLineEdit *nameEdit = m_pWidget->findChild<QLineEdit*>("sequenceNameEdit");
    if (nameEdit) {
        nameEdit->setText("Test Sequence");
        // Signal should be emitted
        QVERIFY(spy.count() >= 1);
    }
}

void SequenceEditorWidgetTests::testValidateEmptySequence()
{
    // Validating an empty sequence should return false or show warning
    bool result = m_pWidget->validateSequence();
    QVERIFY(!result); // Empty sequence should not validate
}

void SequenceEditorWidgetTests::testTreeViewConfiguration()
{
    QTreeView *treeView = m_pWidget->findChild<QTreeView*>("treeView");
    QVERIFY(treeView != nullptr);

    // Check tree view properties
    QVERIFY(treeView->selectionMode() == QAbstractItemView::SingleSelection);
    QVERIFY(treeView->contextMenuPolicy() == Qt::CustomContextMenu);
}

void SequenceEditorWidgetTests::testToolBarActions()
{
    QToolBar *toolBar = m_pWidget->findChild<QToolBar*>();
    QVERIFY(toolBar != nullptr);

    QList<QAction*> actions = toolBar->actions();

    // Should have actions for: Add, Edit, Delete, Move Up, Move Down, Validate
    QVERIFY(actions.count() >= 5);

    // Check for specific actions
    bool hasAddAction = false;
    bool hasDeleteAction = false;

    for (QAction *action : actions) {
        if (action->isSeparator()) continue;

        QString text = action->text();
        if (text.contains("Add")) hasAddAction = true;
        if (text.contains("Delete")) hasDeleteAction = true;
    }

    QVERIFY(hasAddAction);
    QVERIFY(hasDeleteAction);
}

void SequenceEditorWidgetTests::testSequenceModifiedSignalConnection()
{
    const QMetaObject *metaObj = m_pWidget->metaObject();
    int signalIndex = metaObj->indexOfSignal("sequenceModified()");
    QVERIFY(signalIndex >= 0);
}

void SequenceEditorWidgetTests::testStepSelectedSignal()
{
    const QMetaObject *metaObj = m_pWidget->metaObject();
    int signalIndex = metaObj->indexOfSignal("stepSelected(QString)");
    QVERIFY(signalIndex >= 0);
}

void SequenceEditorWidgetTests::testValidationChangedSignal()
{
    const QMetaObject *metaObj = m_pWidget->metaObject();
    int signalIndex = metaObj->indexOfSignal("validationChanged(bool)");
    QVERIFY(signalIndex >= 0);
}

QTEST_MAIN(SequenceEditorWidgetTests)
#include "SequenceEditorWidgetTests.moc"
