#include "ResultsTableModel.h"
#include <QColor>

ResultsTableModel::ResultsTableModel(QObject *parent)
    : QAbstractTableModel(parent)
{
}

int ResultsTableModel::rowCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : static_cast<int>(m_rows.size());
}

int ResultsTableModel::columnCount(const QModelIndex &parent) const
{
    return parent.isValid() ? 0 : 3;
}

QVariant ResultsTableModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || index.row() < 0 ||
        index.row() >= static_cast<int>(m_rows.size())) {
        return QVariant();
    }

    const SRow &row = m_rows[static_cast<size_t>(index.row())];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return row.stepName;
            case 1: return row.verdict;
            case 2: return row.detail;
        }
    } else if (role == Qt::ForegroundRole && index.column() == 1) {
        const QString v = row.verdict.toLower();
        if (v == QLatin1String("pass")) {
            return QColor(0x27, 0xae, 0x60);   // green
        }
        if (v == QLatin1String("fail") || v == QLatin1String("error")) {
            return QColor(0xe7, 0x4c, 0x3c);   // red
        }
    }

    return QVariant();
}

QVariant ResultsTableModel::headerData(int section, Qt::Orientation orientation,
                                       int role) const
{
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return QVariant();
    }
    switch (section) {
        case 0: return tr("Step");
        case 1: return tr("Verdict");
        case 2: return tr("Detail");
    }
    return QVariant();
}

void ResultsTableModel::addResult(const QString &stepName, const QString &verdict,
                                  const QString &detail)
{
    const int newRow = static_cast<int>(m_rows.size());
    beginInsertRows(QModelIndex(), newRow, newRow);
    m_rows.push_back(SRow{stepName, verdict, detail});
    endInsertRows();
}

void ResultsTableModel::clearResults()
{
    if (m_rows.empty()) {
        return;
    }
    beginResetModel();
    m_rows.clear();
    endResetModel();
}
