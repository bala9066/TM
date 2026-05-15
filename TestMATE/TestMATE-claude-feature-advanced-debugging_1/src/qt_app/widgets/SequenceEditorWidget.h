/**************************************************************************
 * File Name: SequenceEditorWidget.h
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 * Description: Widget for editing test sequences
 **************************************************************************/

#pragma once

#include <QWidget>
#include <QString>
#include <memory>

namespace TestMATE {
    class CTestSequence;
}

namespace Ui {
class SequenceEditorWidget;
}

class SequenceTreeModel;

/**************************************************************************
 * Class: SequenceEditorWidget
 * Description: Tree-based editor for test sequences with step management
 **************************************************************************/
class SequenceEditorWidget : public QWidget
{
    Q_OBJECT

public:
    explicit SequenceEditorWidget(QWidget *parent = nullptr);
    ~SequenceEditorWidget();

    bool loadSequence(const QString& fileName);
    bool saveSequence(const QString& fileName);
    void newSequence();
    bool validateSequence();

    // Edit operations
    void addStep();
    void editStep();
    void deleteStep();
    void moveStepUp();
    void moveStepDown();

    void undo();
    void redo();
    void cut();
    void copy();
    void paste();

signals:
    void sequenceModified();
    void stepSelected(const QString& stepId);
    void validationChanged(bool isValid);

private slots:
    void onTreeSelectionChanged();
    void onStepDoubleClicked(const QModelIndex& index);
    void onContextMenuRequested(const QPoint& pos);
    void onSequenceInfoChanged();

private:
    void createToolbar();
    void updateSequenceInfo();
    void updateStepProperties();

private:
    // UI
    Ui::SequenceEditorWidget *ui;

    // Data model
    SequenceTreeModel *m_pModel;

    // Sequence data
    std::shared_ptr<TestMATE::CTestSequence> m_pSequence;

    // State
    bool m_bModified;
};
