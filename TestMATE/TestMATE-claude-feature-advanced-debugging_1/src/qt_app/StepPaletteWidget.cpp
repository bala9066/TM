/**************************************************************************
 * File Name: StepPaletteWidget.cpp
 * Description: Implementation of step palette widget
 **************************************************************************/

#include "StepPaletteWidget.h"
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QIcon>
#include <QDrag>
#include <QMimeData>
#include <QApplication>

namespace TestMATE {

/**************************************************************************
 * CStepPaletteWidget Implementation
 **************************************************************************/

CStepPaletteWidget::CStepPaletteWidget(QWidget* parent)
    : QWidget(parent)
    , m_pMainLayout(nullptr)
    , m_pTitleLabel(nullptr)
    , m_pCategoryCombo(nullptr)
    , m_pSearchEdit(nullptr)
    , m_pListWidget(nullptr)
{
    SetupUI();
    SetupConnections();
    PopulateDefaultSteps();
    UpdateDisplay();
}

void CStepPaletteWidget::SetupUI() {
    // Main layout
    m_pMainLayout = new QVBoxLayout(this);
    m_pMainLayout->setContentsMargins(5, 5, 5, 5);
    m_pMainLayout->setSpacing(5);

    // Title
    m_pTitleLabel = new QLabel(tr("Test Step Palette"), this);
    QFont titleFont = m_pTitleLabel->font();
    titleFont.setBold(true);
    titleFont.setPointSize(titleFont.pointSize() + 1);
    m_pTitleLabel->setFont(titleFont);
    m_pMainLayout->addWidget(m_pTitleLabel);

    // Category filter
    QHBoxLayout* filterLayout = new QHBoxLayout();
    QLabel* categoryLabel = new QLabel(tr("Category:"), this);
    m_pCategoryCombo = new QComboBox(this);
    m_pCategoryCombo->addItem(tr("All Steps"), static_cast<int>(EStepCategory::kAll));
    m_pCategoryCombo->addItem(tr("Delay"), static_cast<int>(EStepCategory::kDelay));
    m_pCategoryCombo->addItem(tr("Validation"), static_cast<int>(EStepCategory::kValidation));
    m_pCategoryCombo->addItem(tr("Measurement"), static_cast<int>(EStepCategory::kMeasurement));
    m_pCategoryCombo->addItem(tr("Action"), static_cast<int>(EStepCategory::kAction));
    m_pCategoryCombo->addItem(tr("Custom"), static_cast<int>(EStepCategory::kCustom));
    filterLayout->addWidget(categoryLabel);
    filterLayout->addWidget(m_pCategoryCombo, 1);
    m_pMainLayout->addLayout(filterLayout);

    // Search box
    m_pSearchEdit = new QLineEdit(this);
    m_pSearchEdit->setPlaceholderText(tr("Search steps..."));
    m_pSearchEdit->setClearButtonEnabled(true);
    m_pMainLayout->addWidget(m_pSearchEdit);

    // List widget with custom drag support
    m_pListWidget = new CStepListWidget(this);
    m_pListWidget->setSelectionMode(QAbstractItemView::SingleSelection);
    m_pListWidget->setDragEnabled(true);
    m_pListWidget->setDefaultDropAction(Qt::CopyAction);
    m_pListWidget->setAlternatingRowColors(true);
    m_pMainLayout->addWidget(m_pListWidget);

    // Help text
    QLabel* helpLabel = new QLabel(
        tr("Drag steps into sequence editor\nor double-click to add at end"), this);
    helpLabel->setWordWrap(true);
    helpLabel->setStyleSheet("QLabel { color: gray; font-size: 9pt; }");
    helpLabel->setAlignment(Qt::AlignCenter);
    m_pMainLayout->addWidget(helpLabel);
}

void CStepPaletteWidget::SetupConnections() {
    connect(m_pCategoryCombo, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &CStepPaletteWidget::OnCategoryChanged);

    connect(m_pSearchEdit, &QLineEdit::textChanged,
            this, &CStepPaletteWidget::OnSearchTextChanged);

    connect(m_pListWidget, &QListWidget::itemDoubleClicked,
            this, &CStepPaletteWidget::OnItemDoubleClicked);

    connect(static_cast<CStepListWidget*>(m_pListWidget), &CStepListWidget::dragStarted,
            this, &CStepPaletteWidget::stepDragStarted);
}

void CStepPaletteWidget::PopulateDefaultSteps() {
    // Wait Step
    SStepPaletteItem waitStep;
    waitStep.id = "wait";
    waitStep.name = tr("Wait");
    waitStep.description = tr("Time delay with abort capability");
    waitStep.icon = ":/icons/wait.svg";
    waitStep.category = EStepCategory::kDelay;
    waitStep.className = "CWaitStep";
    AddStepType(waitStep);

    // Limit Check Step
    SStepPaletteItem limitStep;
    limitStep.id = "limit_check";
    limitStep.name = tr("Limit Check");
    limitStep.description = tr("Validate measurement against limits");
    limitStep.icon = ":/icons/validate.svg";
    limitStep.category = EStepCategory::kValidation;
    limitStep.className = "CLimitCheckStep";
    AddStepType(limitStep);

    // Calculation Step
    SStepPaletteItem calcStep;
    calcStep.id = "calculation";
    calcStep.name = tr("Calculation");
    calcStep.description = tr("Mathematical operations on values");
    calcStep.icon = ":/icons/edit.svg";
    calcStep.category = EStepCategory::kMeasurement;
    calcStep.className = "CCalculationStep";
    AddStepType(calcStep);

    // Instrument Measure Step
    SStepPaletteItem measureStep;
    measureStep.id = "instrument_measure";
    measureStep.name = tr("Instrument Measure");
    measureStep.description = tr("Take measurement from SCPI instrument");
    measureStep.icon = ":/icons/edit.svg";
    measureStep.category = EStepCategory::kMeasurement;
    measureStep.className = "CInstrumentMeasureStep";
    AddStepType(measureStep);

    // Serial Command Step
    SStepPaletteItem serialStep;
    serialStep.id = "serial_command";
    serialStep.name = tr("Serial Command");
    serialStep.description = tr("Send command via serial port");
    serialStep.icon = ":/icons/run.svg";
    serialStep.category = EStepCategory::kAction;
    serialStep.className = "CSerialCommandStep";
    AddStepType(serialStep);

    // File Operation Step
    SStepPaletteItem fileStep;
    fileStep.id = "file_operation";
    fileStep.name = tr("File Operation");
    fileStep.description = tr("Read, write, or manipulate files");
    fileStep.icon = ":/icons/save.svg";
    fileStep.category = EStepCategory::kAction;
    fileStep.className = "CFileOperationStep";
    AddStepType(fileStep);
}

void CStepPaletteWidget::AddStepType(const SStepPaletteItem& item) {
    // Check if already exists
    for (const auto& existing : m_stepTypes) {
        if (existing.id == item.id) {
            return; // Already exists
        }
    }

    m_stepTypes.append(item);
    UpdateDisplay();
}

void CStepPaletteWidget::RemoveStepType(const QString& stepId) {
    for (int i = 0; i < m_stepTypes.size(); ++i) {
        if (m_stepTypes[i].id == stepId) {
            m_stepTypes.removeAt(i);
            break;
        }
    }
    UpdateDisplay();
}

void CStepPaletteWidget::Clear() {
    m_stepTypes.clear();
    m_pListWidget->clear();
}

QList<SStepPaletteItem> CStepPaletteWidget::GetStepTypes() const {
    return m_stepTypes;
}

void CStepPaletteWidget::Refresh() {
    // TODO: Discover plugins and add them to palette
    // For now, just refresh display
    UpdateDisplay();
}

void CStepPaletteWidget::OnCategoryChanged(int index) {
    Q_UNUSED(index);
    UpdateDisplay();
}

void CStepPaletteWidget::OnSearchTextChanged(const QString& text) {
    Q_UNUSED(text);
    UpdateDisplay();
}

void CStepPaletteWidget::OnItemDoubleClicked(QListWidgetItem* item) {
    if (!item) return;

    QString stepId = item->data(Qt::UserRole).toString();
    emit stepDoubleClicked(stepId);
}

void CStepPaletteWidget::UpdateDisplay() {
    m_pListWidget->clear();

    // Get current filter
    EStepCategory selectedCategory = static_cast<EStepCategory>(
        m_pCategoryCombo->currentData().toInt());
    QString searchText = m_pSearchEdit->text().toLower();

    // Filter and display steps
    for (const auto& step : m_stepTypes) {
        // Category filter
        if (selectedCategory != EStepCategory::kAll &&
            step.category != selectedCategory) {
            continue;
        }

        // Search filter
        if (!searchText.isEmpty()) {
            if (!step.name.toLower().contains(searchText) &&
                !step.description.toLower().contains(searchText)) {
                continue;
            }
        }

        // Create list item
        QListWidgetItem* item = new QListWidgetItem(m_pListWidget);
        item->setText(step.name);
        item->setToolTip(QString("%1\n\n%2").arg(step.name, step.description));
        item->setData(Qt::UserRole, step.id);
        item->setData(Qt::UserRole + 1, step.className);

        // Set icon if available
        QIcon icon(step.icon);
        if (!icon.isNull()) {
            item->setIcon(icon);
        }

        m_pListWidget->addItem(item);
    }

    // Update count in title
    int visibleCount = m_pListWidget->count();
    int totalCount = m_stepTypes.size();
    if (visibleCount < totalCount) {
        m_pTitleLabel->setText(
            tr("Test Step Palette (%1 of %2)").arg(visibleCount).arg(totalCount));
    } else {
        m_pTitleLabel->setText(tr("Test Step Palette (%1)").arg(totalCount));
    }
}

QString CStepPaletteWidget::CategoryToString(EStepCategory category) const {
    switch (category) {
        case EStepCategory::kAll: return tr("All");
        case EStepCategory::kDelay: return tr("Delay");
        case EStepCategory::kValidation: return tr("Validation");
        case EStepCategory::kMeasurement: return tr("Measurement");
        case EStepCategory::kAction: return tr("Action");
        case EStepCategory::kControl: return tr("Control");
        case EStepCategory::kCustom: return tr("Custom");
        default: return tr("Unknown");
    }
}

EStepCategory CStepPaletteWidget::StringToCategory(const QString& str) const {
    if (str == tr("Delay")) return EStepCategory::kDelay;
    if (str == tr("Validation")) return EStepCategory::kValidation;
    if (str == tr("Measurement")) return EStepCategory::kMeasurement;
    if (str == tr("Action")) return EStepCategory::kAction;
    if (str == tr("Control")) return EStepCategory::kControl;
    if (str == tr("Custom")) return EStepCategory::kCustom;
    return EStepCategory::kAll;
}

/**************************************************************************
 * CStepListWidget Implementation
 **************************************************************************/

CStepPaletteWidget::CStepListWidget::CStepListWidget(QWidget* parent)
    : QListWidget(parent)
{
    setDragEnabled(true);
    setDragDropMode(QAbstractItemView::DragOnly);
}

void CStepPaletteWidget::CStepListWidget::startDrag(Qt::DropActions supportedActions) {
    QListWidgetItem* item = currentItem();
    if (!item) return;

    QString stepId = item->data(Qt::UserRole).toString();
    emit dragStarted(stepId);

    // Call base implementation
    QListWidget::startDrag(supportedActions);
}

QMimeData* CStepPaletteWidget::CStepListWidget::mimeData(
    const QList<QListWidgetItem*> items) const {

    if (items.isEmpty()) return nullptr;

    QMimeData* mimeData = new QMimeData();

    // Get first item
    QListWidgetItem* item = items.first();
    QString stepId = item->data(Qt::UserRole).toString();
    QString className = item->data(Qt::UserRole + 1).toString();
    QString stepName = item->text();

    // Create MIME data for test step
    // Format: "testmate/step;id=<id>;class=<class>;name=<name>"
    QString mimeText = QString("testmate/step;id=%1;class=%2;name=%3")
        .arg(stepId, className, stepName);

    mimeData->setText(mimeText);
    mimeData->setData("application/x-testmate-step", mimeText.toUtf8());

    return mimeData;
}

QStringList CStepPaletteWidget::CStepListWidget::mimeTypes() const {
    QStringList types;
    types << "application/x-testmate-step";
    types << "text/plain";
    return types;
}

} // namespace TestMATE
