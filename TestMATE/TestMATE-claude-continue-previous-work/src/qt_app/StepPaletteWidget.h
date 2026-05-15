/**************************************************************************
 * File Name: StepPaletteWidget.h
 * Description: Drag-and-drop palette of available test step types
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 *
 * Purpose:
 *   Provides a visual palette showing all available test step types that
 *   can be dragged into test sequences. Supports categorization, search,
 *   and automatic plugin discovery.
 **************************************************************************/

#pragma once

#include <QWidget>
#include <QListWidget>
#include <QLineEdit>
#include <QComboBox>
#include <QString>
#include <QVBoxLayout>
#include <QLabel>
#include <QMimeData>

namespace TestMATE {

/**************************************************************************
 * Enum: EStepCategory
 * Description: Categories for organizing test steps
 **************************************************************************/
enum class EStepCategory {
    kAll,           // Show all steps
    kDelay,         // Time delays (Wait)
    kValidation,    // Limit checks, comparisons
    kMeasurement,   // Measurements, calculations
    kAction,        // Commands, file operations
    kControl,       // Loops, conditionals (future)
    kCustom         // User plugins
};

/**************************************************************************
 * Struct: SStepPaletteItem
 * Description: Represents a test step type in the palette
 **************************************************************************/
struct SStepPaletteItem {
    QString id;              // Step type ID (e.g., "wait", "limit_check")
    QString name;            // Display name
    QString description;     // Short description
    QString icon;            // Icon resource path
    EStepCategory category;  // Category
    QString className;       // C++ class name for instantiation
};

/**************************************************************************
 * Class: CStepPaletteWidget
 * Description: Widget showing draggable test step palette
 *
 * Features:
 *   - Categorized list of available test steps
 *   - Drag-and-drop to sequence editor
 *   - Search/filter functionality
 *   - Plugin discovery
 *   - Tooltips with detailed descriptions
 *
 * Usage:
 *   auto* palette = new CStepPaletteWidget(this);
 *   addDockWidget(Qt::LeftDockWidgetArea, palette);
 **************************************************************************/
class CStepPaletteWidget : public QWidget {
    Q_OBJECT

public:
    explicit CStepPaletteWidget(QWidget* parent = nullptr);
    ~CStepPaletteWidget() override = default;

    // Add step type to palette
    void AddStepType(const SStepPaletteItem& item);

    // Remove step type
    void RemoveStepType(const QString& stepId);

    // Clear all step types
    void Clear();

    // Get all step types
    QList<SStepPaletteItem> GetStepTypes() const;

    // Refresh palette (discover plugins)
    void Refresh();

signals:
    // Emitted when user wants to add a step (double-click)
    void stepDoubleClicked(const QString& stepId);

    // Emitted when drag starts
    void stepDragStarted(const QString& stepId);

private slots:
    // Category filter changed
    void OnCategoryChanged(int index);

    // Search text changed
    void OnSearchTextChanged(const QString& text);

    // Item double-clicked
    void OnItemDoubleClicked(QListWidgetItem* item);

private:
    // UI Components
    QVBoxLayout* m_pMainLayout;
    QLabel* m_pTitleLabel;
    QComboBox* m_pCategoryCombo;
    QLineEdit* m_pSearchEdit;
    QListWidget* m_pListWidget;

    // Data
    QList<SStepPaletteItem> m_stepTypes;

    // Helper methods
    void SetupUI();
    void SetupConnections();
    void PopulateDefaultSteps();
    void UpdateDisplay();
    QString CategoryToString(EStepCategory category) const;
    EStepCategory StringToCategory(const QString& str) const;

    // Custom list widget with drag support
    class CStepListWidget;
};

/**************************************************************************
 * Class: CStepListWidget
 * Description: Custom QListWidget with drag-and-drop support
 **************************************************************************/
class CStepPaletteWidget::CStepListWidget : public QListWidget {
    Q_OBJECT

public:
    explicit CStepListWidget(QWidget* parent = nullptr);

protected:
    // Drag-and-drop support
    void startDrag(Qt::DropActions supportedActions) override;
    QMimeData* mimeData(const QList<QListWidgetItem*> items) const override;
    QStringList mimeTypes() const override;

signals:
    void dragStarted(const QString& stepId);
};

} // namespace TestMATE
