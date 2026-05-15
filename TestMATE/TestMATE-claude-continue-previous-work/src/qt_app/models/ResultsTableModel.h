#pragma once
#include <QAbstractTableModel>

class ResultsTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit ResultsTableModel(QObject *parent = nullptr);
    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
};
