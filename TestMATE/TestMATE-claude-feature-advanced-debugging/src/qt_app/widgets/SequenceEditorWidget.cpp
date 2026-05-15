/**************************************************************************
 * File Name: SequenceEditorWidget.cpp
 * Author: TestMATE Development Team
 * Created Date: 2025-01-22
 * Description: Sequence editor implementation
 **************************************************************************/

#include "SequenceEditorWidget.h"
#include "ui_SequenceEditorWidget.h"
#include "StepPropertiesDialog.h"
#include "../models/SequenceTreeModel.h"

#include "core/test_sequence/TestSequence.h"
#include "core/test_sequence/SequenceFileIO.h"

#include <QToolBar>
#include <QMessageBox>
#include <QMenu>

SequenceEditorWidget::SequenceEditorWidget(QWidget *parent)
    : QWidget(parent)
    , ui(new Ui::SequenceEditorWidget)
    , m_pModel(nullptr)
    , m_pSequence(std::make_shared<TestMATE::CTestSequence>())
    , m_bModified(false)
{
    ui->setupUi(this);

    // Setup model
    m_pModel = new SequenceTreeModel(m_pSequence.get(), this);
    ui->treeView->setModel(m_pModel);

    // Setup toolbar
    createToolbar();

    // Connect signals
    connect(ui->treeView, &QTreeView::doubleClicked, this, &SequenceEditorWidget::onStepDoubleClicked);
    connect(ui->treeView, &QTreeView::customContextMenuRequested, this, &SequenceEditorWidget::onContextMenuRequested);
    connect(ui->sequenceNameEdit, &QLineEdit::textChanged, this, &SequenceEditorWidget::onSequenceInfoChanged);
    connect(ui->sequenceVersionEdit, &QLineEdit::textChanged, this, &SequenceEditorWidget::onSequenceInfoChanged);
    connect(ui->sequenceDescEdit, &QTextEdit::textChanged, this, &SequenceEditorWidget::onSequenceInfoChanged);

    updateSequenceInfo();
}

SequenceEditorWidget::~SequenceEditorWidget()
{
    delete ui;
}

void SequenceEditorWidget::createToolbar()
{
    ui->toolBar->addAction(QIcon(":/icons/add.svg"), tr("Add Step"), this, &SequenceEditorWidget::addStep);
    ui->toolBar->addAction(QIcon(":/icons/edit.svg"), tr("Edit Step"), this, &SequenceEditorWidget::editStep);
    ui->toolBar->addAction(QIcon(":/icons/delete.svg"), tr("Delete Step"), this, &SequenceEditorWidget::deleteStep);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(QIcon(":/icons/up.svg"), tr("Move Up"), this, &SequenceEditorWidget::moveStepUp);
    ui->toolBar->addAction(QIcon(":/icons/down.svg"), tr("Move Down"), this, &SequenceEditorWidget::moveStepDown);
    ui->toolBar->addSeparator();
    ui->toolBar->addAction(QIcon(":/icons/validate.svg"), tr("Validate"), this, &SequenceEditorWidget::validateSequence);
}

bool SequenceEditorWidget::loadSequence(const QString& fileName)
{
    auto& fileIO = TestMATE::CSequenceFileIO::GetInstance();
    auto result = fileIO.LoadSequence(fileName.toStdString(), *m_pSequence);

    if (result.IsSuccess()) {
        m_pModel->refresh();
        updateSequenceInfo();
        m_bModified = false;
        return true;
    }

    return false;
}

bool SequenceEditorWidget::saveSequence(const QString& fileName)
{
    // Update sequence info from UI
    auto info = m_pSequence->GetInfo();
    info.name = ui->sequenceNameEdit->text().toStdString();
    info.version = ui->sequenceVersionEdit->text().toStdString();
    info.description = ui->sequenceDescEdit->toPlainText().toStdString();
    m_pSequence->SetInfo(info);

    auto& fileIO = TestMATE::CSequenceFileIO::GetInstance();

    // Determine format from extension
    TestMATE::ESequenceFileFormat format = TestMATE::ESequenceFileFormat::kJson;
    if (fileName.endsWith(".xml", Qt::CaseInsensitive)) {
        format = TestMATE::ESequenceFileFormat::kXml;
    }

    auto result = fileIO.SaveSequence(fileName.toStdString(), *m_pSequence, format);

    if (result.IsSuccess()) {
        m_bModified = false;
        return true;
    }

    return false;
}

void SequenceEditorWidget::newSequence()
{
    m_pSequence = std::make_shared<TestMATE::CTestSequence>();
    m_pModel->setSequence(m_pSequence.get());
    updateSequenceInfo();
    m_bModified = false;
}

bool SequenceEditorWidget::validateSequence()
{
    // Basic validation
    if (m_pSequence->GetStepCount() == 0) {
        QMessageBox::warning(this, tr("Validation"), tr("Sequence has no steps."));
        return false;
    }

    emit validationChanged(true);
    return true;
}

void SequenceEditorWidget::addStep()
{
    StepPropertiesDialog dialog(this);

    if (dialog.exec() == QDialog::Accepted) {
        // Create and add new step to sequence
        // Full implementation would use dialog data to create step:
        // auto step = m_pSequence->CreateStep();
        // step->SetName(dialog.getName());
        // step->SetType(dialog.getType());
        // m_pSequence->AddStep(step);

        // Refresh model to show new step
        m_pModel->refresh();
        m_bModified = true;
        emit sequenceModified();
    }
}

void SequenceEditorWidget::editStep()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (index.isValid()) {
        StepPropertiesDialog dialog(this);

        // Load current step data into dialog
        // Full implementation would:
        // auto step = m_pSequence->GetStep(index.row());
        // dialog.setName(step->GetName());
        // dialog.setType(step->GetType());
        // dialog.setDescription(step->GetDescription());

        if (dialog.exec() == QDialog::Accepted) {
            // Update step with dialog data
            // Full implementation would:
            // auto step = m_pSequence->GetStep(index.row());
            // step->SetName(dialog.getName());
            // step->SetType(dialog.getType());
            // step->SetDescription(dialog.getDescription());

            m_pModel->refresh();
            m_bModified = true;
            emit sequenceModified();
        }
    }
}

void SequenceEditorWidget::deleteStep()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (index.isValid()) {
        auto reply = QMessageBox::question(this, tr("Delete Step"),
            tr("Are you sure you want to delete this step?"),
            QMessageBox::Yes | QMessageBox::No);

        if (reply == QMessageBox::Yes) {
            // Delete step from sequence
            // Full implementation would:
            // m_pSequence->RemoveStep(index.row());

            m_pModel->refresh();
            m_bModified = true;
            emit sequenceModified();
        }
    }
}

void SequenceEditorWidget::moveStepUp()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid() || index.row() == 0) {
        return; // Can't move up if first item or invalid
    }

    int currentRow = index.row();

    // Move step up in sequence
    // Full implementation would:
    // m_pSequence->MoveStep(currentRow, currentRow - 1);

    m_pModel->refresh();

    // Select the moved item
    QModelIndex newIndex = m_pModel->index(currentRow - 1, 0);
    ui->treeView->setCurrentIndex(newIndex);

    m_bModified = true;
    emit sequenceModified();
}

void SequenceEditorWidget::moveStepDown()
{
    QModelIndex index = ui->treeView->currentIndex();
    if (!index.isValid() || index.row() >= m_pModel->rowCount() - 1) {
        return; // Can't move down if last item or invalid
    }

    int currentRow = index.row();

    // Move step down in sequence
    // Full implementation would:
    // m_pSequence->MoveStep(currentRow, currentRow + 1);

    m_pModel->refresh();

    // Select the moved item
    QModelIndex newIndex = m_pModel->index(currentRow + 1, 0);
    ui->treeView->setCurrentIndex(newIndex);

    m_bModified = true;
    emit sequenceModified();
}

void SequenceEditorWidget::undo() { /* TODO */ }
void SequenceEditorWidget::redo() { /* TODO */ }
void SequenceEditorWidget::cut() { /* TODO */ }
void SequenceEditorWidget::copy() { /* TODO */ }
void SequenceEditorWidget::paste() { /* TODO */ }

void SequenceEditorWidget::onTreeSelectionChanged()
{
    updateStepProperties();
}

void SequenceEditorWidget::onStepDoubleClicked(const QModelIndex& index)
{
    if (index.isValid()) {
        editStep();
    }
}

void SequenceEditorWidget::onContextMenuRequested(const QPoint& pos)
{
    QMenu menu(this);
    menu.addAction(tr("Add Step"), this, &SequenceEditorWidget::addStep);
    menu.addAction(tr("Edit Step"), this, &SequenceEditorWidget::editStep);
    menu.addAction(tr("Delete Step"), this, &SequenceEditorWidget::deleteStep);
    menu.addSeparator();
    menu.addAction(tr("Move Up"), this, &SequenceEditorWidget::moveStepUp);
    menu.addAction(tr("Move Down"), this, &SequenceEditorWidget::moveStepDown);

    menu.exec(ui->treeView->viewport()->mapToGlobal(pos));
}

void SequenceEditorWidget::onSequenceInfoChanged()
{
    m_bModified = true;
    emit sequenceModified();
}

void SequenceEditorWidget::updateSequenceInfo()
{
    auto info = m_pSequence->GetInfo();

    ui->sequenceNameEdit->blockSignals(true);
    ui->sequenceVersionEdit->blockSignals(true);
    ui->sequenceDescEdit->blockSignals(true);

    ui->sequenceNameEdit->setText(QString::fromStdString(info.name));
    ui->sequenceVersionEdit->setText(QString::fromStdString(info.version));
    ui->sequenceDescEdit->setPlainText(QString::fromStdString(info.description));

    ui->sequenceNameEdit->blockSignals(false);
    ui->sequenceVersionEdit->blockSignals(false);
    ui->sequenceDescEdit->blockSignals(false);
}

void SequenceEditorWidget::updateStepProperties()
{
    // TODO: Update step properties based on selection
}
