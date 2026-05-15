#pragma once
#include <QAbstractItemModel>
#include <QMimeData>

namespace TestMATE {
    class CTestSequence;
    class ITestStep;
}

class SequenceTreeModel : public QAbstractItemModel
{
    Q_OBJECT
public:
    explicit SequenceTreeModel(TestMATE::CTestSequence *sequence, QObject *parent = nullptr);

    // Basic model interface
    QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const override;
    QModelIndex parent(const QModelIndex &child) const override;
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    // Drag-and-drop support
    Qt::ItemFlags flags(const QModelIndex &index) const override;
    Qt::DropActions supportedDropActions() const override;
    QStringList mimeTypes() const override;
    QMimeData *mimeData(const QModelIndexList &indexes) const override;
    bool canDropMimeData(const QMimeData *data, Qt::DropAction action,
                         int row, int column, const QModelIndex &parent) const override;
    bool dropMimeData(const QMimeData *data, Qt::DropAction action,
                      int row, int column, const QModelIndex &parent) override;

    void setSequence(TestMATE::CTestSequence *sequence);
    void refresh();

    // Get step at index
    TestMATE::ITestStep* getStep(const QModelIndex &index) const;

signals:
    void stepAdded(int row);
    void stepMoved(int fromRow, int toRow);

private:
    TestMATE::CTestSequence *m_pSequence;

    // Helper methods for drag-and-drop
    bool insertStep(int row, const QString &stepId, const QString &className);
    bool moveStep(int fromRow, int toRow);
};
