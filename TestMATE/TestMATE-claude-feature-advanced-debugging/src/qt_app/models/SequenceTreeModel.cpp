#include "SequenceTreeModel.h"
#include "core/test_sequence/TestSequence.h"
#include "core/test_sequence/ITestStep.h"
#include <QDebug>

namespace {
QString stepTypeToString(TestMATE::EStepType type)
{
    switch (type) {
        case TestMATE::EStepType::kAction:      return QStringLiteral("Action");
        case TestMATE::EStepType::kValidation:  return QStringLiteral("Validation");
        case TestMATE::EStepType::kMeasurement: return QStringLiteral("Measurement");
        case TestMATE::EStepType::kSequence:    return QStringLiteral("Sequence");
        case TestMATE::EStepType::kConditional: return QStringLiteral("Conditional");
        case TestMATE::EStepType::kLoop:        return QStringLiteral("Loop");
        case TestMATE::EStepType::kCall:        return QStringLiteral("Call");
        case TestMATE::EStepType::kWait:        return QStringLiteral("Wait");
        case TestMATE::EStepType::kSync:        return QStringLiteral("Sync");
        case TestMATE::EStepType::kCustom:      return QStringLiteral("Custom");
    }
    return QStringLiteral("Unknown");
}
} // namespace

SequenceTreeModel::SequenceTreeModel(TestMATE::CTestSequence *sequence, QObject *parent)
    : QAbstractItemModel(parent), m_pSequence(sequence)
{
}

void SequenceTreeModel::setSequence(TestMATE::CTestSequence *sequence)
{
    beginResetModel();
    m_pSequence = sequence;
    endResetModel();
}

void SequenceTreeModel::refresh()
{
    beginResetModel();
    endResetModel();
}

QModelIndex SequenceTreeModel::index(int row, int column, const QModelIndex &parent) const
{
    if (!hasIndex(row, column, parent))
        return QModelIndex();

    return createIndex(row, column);
}

QModelIndex SequenceTreeModel::parent(const QModelIndex &child) const
{
    Q_UNUSED(child);
    return QModelIndex();
}

int SequenceTreeModel::rowCount(const QModelIndex &parent) const
{
    if (parent.isValid())
        return 0;

    return m_pSequence ? m_pSequence->GetStepCount() : 0;
}

int SequenceTreeModel::columnCount(const QModelIndex &parent) const
{
    Q_UNUSED(parent);
    return 3; // Name, Type, Status
}

QVariant SequenceTreeModel::data(const QModelIndex &index, int role) const
{
    if (!index.isValid() || !m_pSequence)
        return QVariant();

    TestMATE::ITestStep *step = getStep(index);
    if (!step)
        return QVariant();

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case 0: return QString::fromStdString(step->GetName());
            case 1: return stepTypeToString(step->GetType());
            case 2: return step->IsEnabled() ? QStringLiteral("Enabled")
                                             : QStringLiteral("Disabled");
        }
    } else if (role == Qt::ToolTipRole && index.column() == 0) {
        return QString::fromStdString(step->GetDescription());
    }

    return QVariant();
}

QVariant SequenceTreeModel::headerData(int section, Qt::Orientation orientation, int role) const
{
    if (orientation == Qt::Horizontal && role == Qt::DisplayRole) {
        switch (section) {
            case 0: return QString("Step Name");
            case 1: return QString("Type");
            case 2: return QString("Status");
        }
    }
    return QVariant();
}

/**************************************************************************
 * Drag-and-Drop Support
 **************************************************************************/

Qt::ItemFlags SequenceTreeModel::flags(const QModelIndex &index) const
{
    Qt::ItemFlags defaultFlags = QAbstractItemModel::flags(index);

    if (index.isValid()) {
        // Items are draggable and can be drop targets
        return defaultFlags | Qt::ItemIsDragEnabled | Qt::ItemIsDropEnabled;
    } else {
        // Empty space can accept drops
        return defaultFlags | Qt::ItemIsDropEnabled;
    }
}

Qt::DropActions SequenceTreeModel::supportedDropActions() const
{
    return Qt::CopyAction | Qt::MoveAction;
}

QStringList SequenceTreeModel::mimeTypes() const
{
    QStringList types;
    types << "application/x-testmate-step";      // From palette
    types << "application/x-testmate-step-move";  // Internal reordering
    return types;
}

QMimeData *SequenceTreeModel::mimeData(const QModelIndexList &indexes) const
{
    if (indexes.isEmpty())
        return nullptr;

    QMimeData *mimeData = new QMimeData();

    // Get first index (we only support single selection for now)
    QModelIndex index = indexes.first();
    if (!index.isValid())
        return mimeData;

    // Create MIME data for internal move
    // Format: "row=<row>"
    QString mimeText = QString("row=%1").arg(index.row());
    mimeData->setData("application/x-testmate-step-move", mimeText.toUtf8());
    mimeData->setText(mimeText);

    return mimeData;
}

bool SequenceTreeModel::canDropMimeData(const QMimeData *data, Qt::DropAction action,
                                         int row, int column, const QModelIndex &parent) const
{
    Q_UNUSED(action);
    Q_UNUSED(parent);

    if (!m_pSequence)
        return false;

    // Don't drop on specific column
    if (column > 0)
        return false;

    // Check if we have supported MIME type
    if (!data->hasFormat("application/x-testmate-step") &&
        !data->hasFormat("application/x-testmate-step-move")) {
        return false;
    }

    // Can always drop
    return true;
}

bool SequenceTreeModel::dropMimeData(const QMimeData *data, Qt::DropAction action,
                                      int row, int column, const QModelIndex &parent)
{
    if (!canDropMimeData(data, action, row, column, parent))
        return false;

    if (!m_pSequence)
        return false;

    // Determine drop row
    int dropRow;
    if (row != -1) {
        // Dropped between items
        dropRow = row;
    } else if (parent.isValid()) {
        // Dropped on an item
        dropRow = parent.row() + 1;
    } else {
        // Dropped on empty space (append)
        dropRow = rowCount();
    }

    // Handle internal move
    if (data->hasFormat("application/x-testmate-step-move")) {
        QString mimeText = QString::fromUtf8(data->data("application/x-testmate-step-move"));

        // Parse source row
        if (mimeText.startsWith("row=")) {
            int sourceRow = mimeText.mid(4).toInt();

            // Adjust drop row if moving down
            if (sourceRow < dropRow) {
                dropRow--;
            }

            return moveStep(sourceRow, dropRow);
        }
    }

    // Handle drop from palette
    if (data->hasFormat("application/x-testmate-step")) {
        QString mimeText = QString::fromUtf8(data->data("application/x-testmate-step"));

        // Parse MIME data: "testmate/step;id=<id>;class=<class>;name=<name>"
        QStringList parts = mimeText.split(';');
        if (parts.size() < 3)
            return false;

        QString stepId;
        QString className;

        for (const QString& part : parts) {
            if (part.startsWith("id=")) {
                stepId = part.mid(3);
            } else if (part.startsWith("class=")) {
                className = part.mid(6);
            }
        }

        if (stepId.isEmpty() || className.isEmpty())
            return false;

        return insertStep(dropRow, stepId, className);
    }

    return false;
}

TestMATE::ITestStep* SequenceTreeModel::getStep(const QModelIndex &index) const
{
    if (!index.isValid() || !m_pSequence)
        return nullptr;

    const auto& steps = m_pSequence->GetSteps();
    if (index.row() < 0 || index.row() >= static_cast<int>(steps.size()))
        return nullptr;

    return steps[index.row()].get();
}

/**************************************************************************
 * Helper Methods
 **************************************************************************/

bool SequenceTreeModel::insertStep(int row, const QString &stepId, const QString &className)
{
    if (!m_pSequence)
        return false;

    // TODO: Create actual step instance based on className
    // For now, this is a placeholder that would need integration with
    // the step factory/plugin system

    qDebug() << "Would insert step:" << stepId << "class:" << className << "at row:" << row;

    // Signal that we added a step
    emit stepAdded(row);

    // Refresh model (in real implementation, use beginInsertRows/endInsertRows)
    refresh();

    return true;
}

bool SequenceTreeModel::moveStep(int fromRow, int toRow)
{
    if (!m_pSequence)
        return false;

    if (fromRow == toRow)
        return true;  // No-op

    const int count = rowCount();
    if (fromRow < 0 || fromRow >= count)
        return false;
    if (toRow < 0 || toRow >= count)
        return false;

    // A full reset is used (rather than beginMoveRows) because it is
    // unconditionally correct; the cost is losing selection/expansion.
    beginResetModel();
    auto result = m_pSequence->MoveStep(static_cast<TestMATE::TUInt32>(fromRow),
                                        static_cast<TestMATE::TUInt32>(toRow));
    endResetModel();

    if (result.IsFailure())
        return false;

    emit stepMoved(fromRow, toRow);
    return true;
}
