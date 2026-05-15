#pragma once
#include <QAbstractTableModel>
#include <QString>
#include <vector>

/**
 * @brief Table model holding per-step execution results (step, verdict,
 *        detail). Rows are appended as steps complete.
 */
class ResultsTableModel : public QAbstractTableModel
{
    Q_OBJECT
public:
    explicit ResultsTableModel(QObject *parent = nullptr);

    int rowCount(const QModelIndex &parent = QModelIndex()) const override;
    int columnCount(const QModelIndex &parent = QModelIndex()) const override;
    QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation,
                        int role = Qt::DisplayRole) const override;

    /// Append one step-result row.
    void addResult(const QString &stepName, const QString &verdict,
                   const QString &detail = QString());

    /// Remove all rows.
    void clearResults();

private:
    struct SRow {
        QString stepName;
        QString verdict;
        QString detail;
    };
    std::vector<SRow> m_rows;
};
