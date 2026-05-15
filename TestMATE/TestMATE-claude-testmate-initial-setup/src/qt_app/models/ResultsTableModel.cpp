#include "ResultsTableModel.h"

ResultsTableModel::ResultsTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int ResultsTableModel::rowCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 0;
}

int ResultsTableModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3;
}

QVariant ResultsTableModel::data(const QModelIndex &index, int role) const
{
    Q_UNUSED(index);
    Q_UNUSED(role);
    return QVariant();
}
