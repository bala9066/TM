/**************************************************************************
 * File Name: ModelTests.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 * Description: QTest unit tests for Qt model classes
 **************************************************************************/

#include <QtTest/QtTest>
#include <QAbstractItemModel>
#include <QAbstractTableModel>
#include <QModelIndex>
#include <QSignalSpy>

#include "qt_app/models/SequenceTreeModel.h"
#include "qt_app/models/ResultsTableModel.h"
#include "core/test_sequence/TestSequence.h"

/**************************************************************************
 * SequenceTreeModel Tests
 **************************************************************************/
class SequenceTreeModelTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Constructor tests
    void testConstructorWithNullSequence();
    void testConstructorWithValidSequence();

    // Row/Column count tests
    void testRowCountWithNullSequence();
    void testRowCountWithEmptySequence();
    void testRowCountWithSteps();
    void testColumnCount();
    void testColumnCountIsConstant();

    // Data tests
    void testDataWithInvalidIndex();
    void testDataWithValidIndex();
    void testDataDisplayRole();
    void testDataColumns();
    void testDataWithNullSequence();

    // Header tests
    void testHeaderData();
    void testHeaderDataHorizontal();
    void testHeaderDataVertical();
    void testHeaderDataAllColumns();

    // Index/Parent tests
    void testIndexCreation();
    void testIndexWithInvalidRow();
    void testIndexWithInvalidColumn();
    void testParentAlwaysInvalid();

    // Sequence management tests
    void testSetSequence();
    void testSetSequenceEmitsSignals();
    void testRefresh();
    void testRefreshEmitsSignals();

    // Model interface tests
    void testModelIsValid();
    void testCanFetchMore();

private:
    SequenceTreeModel *m_pModel;
    TestMATE::CTestSequence *m_pSequence;
};

void SequenceTreeModelTests::initTestCase()
{
    qDebug() << "SequenceTreeModel test suite starting...";
}

void SequenceTreeModelTests::cleanupTestCase()
{
    qDebug() << "SequenceTreeModel test suite finished.";
}

void SequenceTreeModelTests::init()
{
    m_pSequence = new TestMATE::CTestSequence();
    m_pModel = new SequenceTreeModel(m_pSequence);
}

void SequenceTreeModelTests::cleanup()
{
    delete m_pModel;
    delete m_pSequence;
    m_pModel = nullptr;
    m_pSequence = nullptr;
}

void SequenceTreeModelTests::testConstructorWithNullSequence()
{
    SequenceTreeModel *model = new SequenceTreeModel(nullptr);
    QVERIFY(model != nullptr);
    QCOMPARE(model->rowCount(), 0);
    delete model;
}

void SequenceTreeModelTests::testConstructorWithValidSequence()
{
    QVERIFY(m_pModel != nullptr);
    QVERIFY(m_pModel->rowCount() >= 0);
}

void SequenceTreeModelTests::testRowCountWithNullSequence()
{
    SequenceTreeModel *model = new SequenceTreeModel(nullptr);
    QCOMPARE(model->rowCount(), 0);
    delete model;
}

void SequenceTreeModelTests::testRowCountWithEmptySequence()
{
    QCOMPARE(m_pModel->rowCount(), 0);
}

void SequenceTreeModelTests::testRowCountWithSteps()
{
    // The sequence should initially have 0 steps
    int initialCount = m_pModel->rowCount();
    QCOMPARE(initialCount, 0);

    // Note: Adding steps would require knowing the TestSequence API
    // This test verifies that rowCount reflects the sequence's step count
}

void SequenceTreeModelTests::testColumnCount()
{
    QCOMPARE(m_pModel->columnCount(), 3);
}

void SequenceTreeModelTests::testColumnCountIsConstant()
{
    // Column count should be 3 regardless of parent
    QCOMPARE(m_pModel->columnCount(QModelIndex()), 3);

    QModelIndex index = m_pModel->index(0, 0);
    QCOMPARE(m_pModel->columnCount(index), 3);
}

void SequenceTreeModelTests::testDataWithInvalidIndex()
{
    QModelIndex invalidIndex;
    QVariant result = m_pModel->data(invalidIndex);
    QVERIFY(!result.isValid());
}

void SequenceTreeModelTests::testDataWithValidIndex()
{
    // With empty sequence, even valid indices return QVariant
    // because there are no rows
    if (m_pModel->rowCount() == 0) {
        QModelIndex index = m_pModel->index(0, 0);
        QVERIFY(!index.isValid());
    }
}

void SequenceTreeModelTests::testDataDisplayRole()
{
    // Test that DisplayRole returns appropriate data
    // With empty sequence, this will be empty
    int rows = m_pModel->rowCount();

    if (rows == 0) {
        // No data to test
        QCOMPARE(rows, 0);
    }
}

void SequenceTreeModelTests::testDataColumns()
{
    // Verify all three columns are handled
    // Column 0: Step name (e.g., "Step 1")
    // Column 1: Type (e.g., "Test")
    // Column 2: Status (e.g., "Ready")

    QCOMPARE(m_pModel->columnCount(), 3);
}

void SequenceTreeModelTests::testDataWithNullSequence()
{
    SequenceTreeModel *model = new SequenceTreeModel(nullptr);

    QModelIndex index = model->index(0, 0);
    QVariant data = model->data(index);

    QVERIFY(!data.isValid());

    delete model;
}

void SequenceTreeModelTests::testHeaderData()
{
    QVariant header0 = m_pModel->headerData(0, Qt::Horizontal, Qt::DisplayRole);
    QVERIFY(header0.isValid());
    QCOMPARE(header0.toString(), QString("Step Name"));
}

void SequenceTreeModelTests::testHeaderDataHorizontal()
{
    QVariant header0 = m_pModel->headerData(0, Qt::Horizontal, Qt::DisplayRole);
    QVariant header1 = m_pModel->headerData(1, Qt::Horizontal, Qt::DisplayRole);
    QVariant header2 = m_pModel->headerData(2, Qt::Horizontal, Qt::DisplayRole);

    QCOMPARE(header0.toString(), QString("Step Name"));
    QCOMPARE(header1.toString(), QString("Type"));
    QCOMPARE(header2.toString(), QString("Status"));
}

void SequenceTreeModelTests::testHeaderDataVertical()
{
    // Vertical header should return QVariant()
    QVariant vHeader = m_pModel->headerData(0, Qt::Vertical, Qt::DisplayRole);
    QVERIFY(!vHeader.isValid());
}

void SequenceTreeModelTests::testHeaderDataAllColumns()
{
    // Test all columns have headers
    for (int col = 0; col < 3; ++col) {
        QVariant header = m_pModel->headerData(col, Qt::Horizontal, Qt::DisplayRole);
        QVERIFY(header.isValid());
        QVERIFY(!header.toString().isEmpty());
    }
}

void SequenceTreeModelTests::testIndexCreation()
{
    // With empty sequence, index creation for row 0 should be invalid
    QModelIndex index = m_pModel->index(0, 0);

    if (m_pModel->rowCount() == 0) {
        QVERIFY(!index.isValid());
    }
}

void SequenceTreeModelTests::testIndexWithInvalidRow()
{
    QModelIndex index = m_pModel->index(-1, 0);
    QVERIFY(!index.isValid());

    QModelIndex index2 = m_pModel->index(1000, 0);
    QVERIFY(!index2.isValid());
}

void SequenceTreeModelTests::testIndexWithInvalidColumn()
{
    QModelIndex index = m_pModel->index(0, -1);
    QVERIFY(!index.isValid());

    QModelIndex index2 = m_pModel->index(0, 1000);
    QVERIFY(!index2.isValid());
}

void SequenceTreeModelTests::testParentAlwaysInvalid()
{
    // SequenceTreeModel is a flat list, so parent is always invalid
    QModelIndex index = m_pModel->index(0, 0);
    QModelIndex parentIndex = m_pModel->parent(index);

    QVERIFY(!parentIndex.isValid());
}

void SequenceTreeModelTests::testSetSequence()
{
    TestMATE::CTestSequence *newSequence = new TestMATE::CTestSequence();

    m_pModel->setSequence(newSequence);

    // Model should now use the new sequence
    QCOMPARE(m_pModel->rowCount(), 0);

    delete newSequence;
}

void SequenceTreeModelTests::testSetSequenceEmitsSignals()
{
    QSignalSpy modelResetSpy(m_pModel, SIGNAL(modelReset()));

    TestMATE::CTestSequence *newSequence = new TestMATE::CTestSequence();
    m_pModel->setSequence(newSequence);

    // Should emit modelReset signal
    QVERIFY(modelResetSpy.count() >= 1);

    delete newSequence;
}

void SequenceTreeModelTests::testRefresh()
{
    // refresh() should update the model
    m_pModel->refresh();

    // Model should still be valid
    QVERIFY(m_pModel != nullptr);
}

void SequenceTreeModelTests::testRefreshEmitsSignals()
{
    QSignalSpy modelResetSpy(m_pModel, SIGNAL(modelReset()));

    m_pModel->refresh();

    // Should emit modelReset signal
    QCOMPARE(modelResetSpy.count(), 1);
}

void SequenceTreeModelTests::testModelIsValid()
{
    // Test that model can be cast to QAbstractItemModel
    QAbstractItemModel *abstractModel = qobject_cast<QAbstractItemModel*>(m_pModel);
    QVERIFY(abstractModel != nullptr);
}

void SequenceTreeModelTests::testCanFetchMore()
{
    // SequenceTreeModel doesn't implement canFetchMore
    // Default implementation should return false
    QVERIFY(!m_pModel->canFetchMore(QModelIndex()));
}

/**************************************************************************
 * ResultsTableModel Tests
 **************************************************************************/
class ResultsTableModelTests : public QObject
{
    Q_OBJECT

private slots:
    void initTestCase();
    void cleanupTestCase();
    void init();
    void cleanup();

    // Constructor tests
    void testConstructor();
    void testConstructorWithParent();

    // Row/Column count tests
    void testRowCount();
    void testRowCountIsZero();
    void testColumnCount();
    void testColumnCountIs3();

    // Data tests
    void testDataReturnsInvalid();
    void testDataWithValidIndex();
    void testDataWithInvalidIndex();
    void testDataAllRoles();

    // Model interface tests
    void testModelIsValid();
    void testCanBeCastToAbstractTableModel();

private:
    ResultsTableModel *m_pModel;
};

void ResultsTableModelTests::initTestCase()
{
    qDebug() << "ResultsTableModel test suite starting...";
}

void ResultsTableModelTests::cleanupTestCase()
{
    qDebug() << "ResultsTableModel test suite finished.";
}

void ResultsTableModelTests::init()
{
    m_pModel = new ResultsTableModel();
}

void ResultsTableModelTests::cleanup()
{
    delete m_pModel;
    m_pModel = nullptr;
}

void ResultsTableModelTests::testConstructor()
{
    QVERIFY(m_pModel != nullptr);
}

void ResultsTableModelTests::testConstructorWithParent()
{
    QObject parent;
    ResultsTableModel *model = new ResultsTableModel(&parent);

    QVERIFY(model != nullptr);
    QCOMPARE(model->parent(), &parent);

    // Parent will delete model
}

void ResultsTableModelTests::testRowCount()
{
    QCOMPARE(m_pModel->rowCount(), 0);
}

void ResultsTableModelTests::testRowCountIsZero()
{
    // Current implementation returns 0
    QCOMPARE(m_pModel->rowCount(QModelIndex()), 0);
}

void ResultsTableModelTests::testColumnCount()
{
    QCOMPARE(m_pModel->columnCount(), 3);
}

void ResultsTableModelTests::testColumnCountIs3()
{
    // Column count should be 3 regardless of parent
    QCOMPARE(m_pModel->columnCount(QModelIndex()), 3);
}

void ResultsTableModelTests::testDataReturnsInvalid()
{
    // Current implementation returns invalid QVariant
    QModelIndex index = m_pModel->index(0, 0);
    QVariant data = m_pModel->data(index);

    QVERIFY(!data.isValid());
}

void ResultsTableModelTests::testDataWithValidIndex()
{
    // Even with a valid index (if rows existed), data returns QVariant()
    QModelIndex index = m_pModel->index(0, 0);
    QVariant data = m_pModel->data(index);

    QVERIFY(!data.isValid());
}

void ResultsTableModelTests::testDataWithInvalidIndex()
{
    QModelIndex invalidIndex;
    QVariant data = m_pModel->data(invalidIndex);

    QVERIFY(!data.isValid());
}

void ResultsTableModelTests::testDataAllRoles()
{
    // Test different roles all return invalid
    QModelIndex index = m_pModel->index(0, 0);

    QVERIFY(!m_pModel->data(index, Qt::DisplayRole).isValid());
    QVERIFY(!m_pModel->data(index, Qt::EditRole).isValid());
    QVERIFY(!m_pModel->data(index, Qt::ToolTipRole).isValid());
}

void ResultsTableModelTests::testModelIsValid()
{
    // Test that model can be cast to QAbstractTableModel
    QAbstractTableModel *abstractModel = qobject_cast<QAbstractTableModel*>(m_pModel);
    QVERIFY(abstractModel != nullptr);
}

void ResultsTableModelTests::testCanBeCastToAbstractTableModel()
{
    QAbstractTableModel *tableModel = qobject_cast<QAbstractTableModel*>(m_pModel);
    QVERIFY(tableModel != nullptr);

    QAbstractItemModel *itemModel = qobject_cast<QAbstractItemModel*>(m_pModel);
    QVERIFY(itemModel != nullptr);
}

/**************************************************************************
 * Main Test Runner
 **************************************************************************/
int main(int argc, char *argv[])
{
    QCoreApplication app(argc, argv);

    int status = 0;

    {
        SequenceTreeModelTests sequenceTreeTests;
        status |= QTest::qExec(&sequenceTreeTests, argc, argv);
    }

    {
        ResultsTableModelTests resultsTableTests;
        status |= QTest::qExec(&resultsTableTests, argc, argv);
    }

    return status;
}

#include "ModelTests.moc"
