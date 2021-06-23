/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtTreeView.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

#include "svtkQtTreeView.h"

#include "QFilterTreeProxyModel.h"
#include <QAbstractItemView>
#include <QColumnView>
#include <QHeaderView>
#include <QItemSelection>
#include <QItemSelectionModel>
#include <QSortFilterProxyModel>
#include <QTreeView>
#include <QVBoxLayout>

#include "svtkAlgorithm.h"
#include "svtkAlgorithmOutput.h"
#include "svtkAnnotation.h"
#include "svtkAnnotationLayers.h"
#include "svtkAnnotationLink.h"
#include "svtkApplyColors.h"
#include "svtkConvertSelection.h"
#include "svtkDataRepresentation.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkQtTreeModelAdapter.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkTree.h"
#include "svtkViewTheme.h"

svtkStandardNewMacro(svtkQtTreeView);

//----------------------------------------------------------------------------
svtkQtTreeView::svtkQtTreeView()
{
  this->ApplyColors = svtkSmartPointer<svtkApplyColors>::New();

  this->Widget = new QWidget();
  this->TreeView = new QTreeView();
  this->ColumnView = new QColumnView();
  this->TreeAdapter = new svtkQtTreeModelAdapter();
  this->TreeFilter = new QFilterTreeProxyModel();
  this->TreeFilter->setSourceModel(this->TreeAdapter);
  this->TreeFilter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  this->TreeView->setModel(this->TreeFilter);
  this->ColumnView->setModel(this->TreeFilter);
  this->SelectionModel = new QItemSelectionModel(this->TreeAdapter);
  this->TreeView->setSelectionModel(this->SelectionModel);
  this->ColumnView->setSelectionModel(this->SelectionModel);
  this->Layout = new QVBoxLayout(this->GetWidget());
  this->Layout->setContentsMargins(0, 0, 0, 0);

  // Add both widgets to the layout and then hide one
  this->Layout->addWidget(this->TreeView);
  this->Layout->addWidget(this->ColumnView);
  this->ColumnView->hide();

  // Set up some default properties
  this->TreeView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->TreeView->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->ColumnView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->ColumnView->setSelectionBehavior(QAbstractItemView::SelectRows);
  this->SetUseColumnView(false);
  this->SetAlternatingRowColors(false);
  this->SetShowRootNode(false);
  this->CurrentSelectionMTime = 0;
  this->ColorArrayNameInternal = nullptr;
  double defCol[3] = { 0.827, 0.827, 0.827 };
  this->ApplyColors->SetDefaultPointColor(defCol);
  this->ApplyColors->SetUseCurrentAnnotationColor(true);
  this->LastInputMTime = 0;

  // Drag/Drop parameters - defaults to off
  this->TreeView->setDragEnabled(false);
  this->TreeView->setDragDropMode(QAbstractItemView::DragOnly);
  this->TreeView->setDragDropOverwriteMode(false);
  this->TreeView->setAcceptDrops(false);
  this->TreeView->setDropIndicatorShown(false);

  this->ColumnView->setDragEnabled(false);
  this->ColumnView->setDragDropMode(QAbstractItemView::DragOnly);
  this->ColumnView->setDragDropOverwriteMode(false);
  this->ColumnView->setAcceptDrops(false);
  this->ColumnView->setDropIndicatorShown(false);

  QObject::connect(this->TreeView, SIGNAL(expanded(const QModelIndex&)), this,
    SIGNAL(expanded(const QModelIndex&)));
  QObject::connect(this->TreeView, SIGNAL(collapsed(const QModelIndex&)), this,
    SIGNAL(collapsed(const QModelIndex&)));

  QObject::connect(this->SelectionModel,
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SLOT(slotQtSelectionChanged(const QItemSelection&, const QItemSelection&)));

  QObject::connect(this->ColumnView, SIGNAL(updatePreviewWidget(const QModelIndex&)), this,
    SIGNAL(updatePreviewWidget(const QModelIndex&)));
}

//----------------------------------------------------------------------------
svtkQtTreeView::~svtkQtTreeView()
{
  delete this->TreeView;
  delete this->ColumnView;
  delete this->Layout;
  delete this->Widget;
  delete this->SelectionModel;
  delete this->TreeAdapter;
  delete this->TreeFilter;
}

//----------------------------------------------------------------------------
void svtkQtTreeView::SetUseColumnView(int state)
{
  if (state)
  {
    this->ColumnView->show();
    this->TreeView->hide();
    this->View = qobject_cast<QAbstractItemView*>(this->ColumnView);
  }
  else
  {
    this->ColumnView->hide();
    this->TreeView->show();
    this->View = qobject_cast<QAbstractItemView*>(this->TreeView);
  }

  // Probably a good idea to make sure the container widget is refreshed
  this->Widget->update();
}

//----------------------------------------------------------------------------
QWidget* svtkQtTreeView::GetWidget()
{
  return this->Widget;
}

//----------------------------------------------------------------------------
void svtkQtTreeView::SetShowHeaders(bool state)
{
  if (state)
  {
    this->TreeView->header()->show();
  }
  else
  {
    this->TreeView->header()->hide();
  }
}

//----------------------------------------------------------------------------
void svtkQtTreeView::SetAlternatingRowColors(bool state)
{
  this->TreeView->setAlternatingRowColors(state);
  this->ColumnView->setAlternatingRowColors(state);
}

//----------------------------------------------------------------------------
void svtkQtTreeView::SetEnableDragDrop(bool state)
{
  this->TreeView->setDragEnabled(state);
  this->ColumnView->setDragEnabled(state);
}

//----------------------------------------------------------------------------
void svtkQtTreeView::SetShowRootNode(bool state)
{
  if (!state)
  {
    this->TreeView->setRootIndex(this->TreeView->model()->index(0, 0));
    this->ColumnView->setRootIndex(this->TreeView->model()->index(0, 0));
  }
  else
  {
    this->TreeView->setRootIndex(QModelIndex());
    this->ColumnView->setRootIndex(QModelIndex());
  }
}

//----------------------------------------------------------------------------
void svtkQtTreeView::HideColumn(int i)
{
  this->TreeView->hideColumn(i);
  this->HiddenColumns << i;
}

//----------------------------------------------------------------------------
void svtkQtTreeView::ShowColumn(int i)
{
  this->TreeView->showColumn(i);
  this->HiddenColumns.removeAll(i);
}

//----------------------------------------------------------------------------
void svtkQtTreeView::HideAllButFirstColumn()
{
  this->HiddenColumns.clear();
  this->TreeView->showColumn(0);
  for (int j = 1; j < this->TreeAdapter->columnCount(); ++j)
  {
    this->TreeView->hideColumn(j);
    this->HiddenColumns << j;
  }
}

//----------------------------------------------------------------------------
void svtkQtTreeView::SetFilterColumn(int i)
{
  this->TreeFilter->setFilterKeyColumn(i);
}

//----------------------------------------------------------------------------
void svtkQtTreeView::SetFilterRegExp(const QRegExp& pattern)
{
  this->TreeFilter->setFilterRegExp(pattern);
}

//----------------------------------------------------------------------------
void svtkQtTreeView::SetFilterTreeLevel(int level)
{
  this->TreeFilter->setFilterTreeLevel(level);
}

void svtkQtTreeView::AddRepresentationInternal(svtkDataRepresentation* rep)
{
  svtkAlgorithmOutput *annConn, *conn;
  conn = rep->GetInputConnection();
  annConn = rep->GetInternalAnnotationOutputPort();

  this->ApplyColors->SetInputConnection(0, conn);

  if (annConn)
  {
    this->ApplyColors->SetInputConnection(1, annConn);
  }
}

void svtkQtTreeView::RemoveRepresentationInternal(svtkDataRepresentation* rep)
{
  svtkAlgorithmOutput *annConn, *conn;
  conn = rep->GetInputConnection();
  annConn = rep->GetInternalAnnotationOutputPort();

  this->ApplyColors->RemoveInputConnection(0, conn);
  this->ApplyColors->RemoveInputConnection(1, annConn);
  this->TreeAdapter->SetSVTKDataObject(nullptr);
}

//----------------------------------------------------------------------------
void svtkQtTreeView::SetItemDelegate(QAbstractItemDelegate* delegate)
{
  this->TreeView->setItemDelegate(delegate);
  this->ColumnView->setItemDelegate(delegate);
}

void svtkQtTreeView::SetColorByArray(bool b)
{
  this->ApplyColors->SetUsePointLookupTable(b);
}

bool svtkQtTreeView::GetColorByArray()
{
  return this->ApplyColors->GetUsePointLookupTable();
}

void svtkQtTreeView::SetColorArrayName(const char* name)
{
  this->SetColorArrayNameInternal(name);
  this->ApplyColors->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_VERTICES, name);
}

const char* svtkQtTreeView::GetColorArrayName()
{
  return this->GetColorArrayNameInternal();
}

//----------------------------------------------------------------------------
void svtkQtTreeView::slotQtSelectionChanged(
  const QItemSelection& svtkNotUsed(s1), const QItemSelection& svtkNotUsed(s2))
{
  // Convert from a QModelIndexList to an index based svtkSelection
  const QModelIndexList qmil = this->View->selectionModel()->selectedRows();
  QModelIndexList origRows;
  for (int i = 0; i < qmil.size(); ++i)
  {
    origRows.push_back(this->TreeFilter->mapToSource(qmil[i]));
  }

  // If in column view mode, don't propagate a selection of a non-leaf node
  // since such a selection is used to expand the next column.
  if (this->ColumnView->isVisible())
  {
    bool leafNodeSelected = false;
    for (int i = 0; i < origRows.size(); ++i)
    {
      if (!this->TreeAdapter->hasChildren(origRows[i]))
      {
        leafNodeSelected = true;
        break;
      }
    }
    if (!leafNodeSelected)
    {
      return;
    }
  }

  svtkSelection* SVTKIndexSelectList =
    this->TreeAdapter->QModelIndexListToSVTKIndexSelection(origRows);

  // Convert to the correct type of selection
  svtkDataRepresentation* rep = this->GetRepresentation();
  svtkDataObject* data = this->TreeAdapter->GetSVTKDataObject();
  svtkSmartPointer<svtkSelection> converted;
  converted.TakeReference(svtkConvertSelection::ToSelectionType(
    SVTKIndexSelectList, data, rep->GetSelectionType(), rep->GetSelectionArrayNames()));

  // Call select on the representation (all 'linked' views will receive this selection)
  rep->Select(this, converted);

  // Delete the selection list
  SVTKIndexSelectList->Delete();

  // Store the selection mtime
  this->CurrentSelectionMTime = rep->GetAnnotationLink()->GetCurrentSelection()->GetMTime();
}

//----------------------------------------------------------------------------
void svtkQtTreeView::SetSVTKSelection()
{
  // Check to see we actually have data
  svtkDataObject* d = this->TreeAdapter->GetSVTKDataObject();
  if (!d)
  {
    return;
  }

  // See if the selection has changed in any way
  svtkDataRepresentation* rep = this->GetRepresentation();
  svtkAlgorithmOutput* annConn = rep->GetInternalAnnotationOutputPort();
  svtkAnnotationLayers* a =
    svtkAnnotationLayers::SafeDownCast(annConn->GetProducer()->GetOutputDataObject(0));
  svtkSelection* s = a->GetCurrentAnnotation()->GetSelection();

  svtkSmartPointer<svtkSelection> selection;
  selection.TakeReference(svtkConvertSelection::ToSelectionType(
    s, d, svtkSelectionNode::INDICES, nullptr, svtkSelectionNode::VERTEX));

  QItemSelection qisList = this->TreeAdapter->SVTKIndexSelectionToQItemSelection(selection);
  QItemSelection filteredSel = this->TreeFilter->mapSelectionFromSource(qisList);

  // Here we want the qt model to have it's selection changed
  // but we don't want to emit the selection.
  QObject::disconnect(this->View->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SLOT(slotQtSelectionChanged(const QItemSelection&, const QItemSelection&)));

  this->View->selectionModel()->select(
    filteredSel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

  QObject::connect(this->View->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SLOT(slotQtSelectionChanged(const QItemSelection&, const QItemSelection&)));

  // Make sure selected items are visible
  // FIXME: Should really recurse up all levels of the tree, this just does one.
  for (int i = 0; i < filteredSel.size(); ++i)
  {
    this->TreeView->setExpanded(filteredSel[i].parent(), true);
  }
}

//----------------------------------------------------------------------------
void svtkQtTreeView::Update()
{
  svtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
  {
    // Remove SVTK data from the adapter
    this->TreeAdapter->SetSVTKDataObject(nullptr);
    this->View->update();
    return;
  }
  rep->Update();

  // Make the data current
  svtkAlgorithm* alg = rep->GetInputConnection()->GetProducer();
  alg->Update();
  svtkDataObject* d = alg->GetOutputDataObject(0);
  svtkTree* tree = svtkTree::SafeDownCast(d);

  // Special-case: if our input is missing or not-a-tree, or empty then quietly exit.
  if (!tree || !tree->GetNumberOfVertices())
  {
    return;
  }

  svtkAlgorithmOutput* annConn = rep->GetInternalAnnotationOutputPort();
  if (annConn)
  {
    annConn->GetProducer()->Update();
  }

  this->ApplyColors->Update();

  if (tree->GetMTime() > this->LastInputMTime)
  {
    // Reset the model
    this->TreeAdapter->SetSVTKDataObject(nullptr);
    this->TreeAdapter->SetSVTKDataObject(this->ApplyColors->GetOutput());

    if (this->GetColorByArray())
    {
      this->TreeAdapter->SetColorColumnName("svtkApplyColors color");
    }
    else
    {
      this->TreeAdapter->SetColorColumnName("");
    }

    this->TreeView->resizeColumnToContents(0);
    this->TreeView->collapseAll();
    // Reset show root node if it was false.
    if (this->TreeView->rootIndex() != QModelIndex())
    {
      this->SetShowRootNode(false);
    }

    this->LastInputMTime = tree->GetMTime();
  }

  svtkMTimeType atime = rep->GetAnnotationLink()->GetMTime();
  if (atime > this->CurrentSelectionMTime)
  {
    this->SetSVTKSelection();
    this->CurrentSelectionMTime = atime;
  }

  // Re-hide the hidden columns
  int col = 0;
  foreach (col, this->HiddenColumns)
  {
    this->TreeView->hideColumn(col);
  }

  for (int j = 0; j < this->TreeAdapter->columnCount(); ++j)
  {
    QString colName = this->TreeAdapter->headerData(j, Qt::Horizontal).toString();
    if (colName == "svtkApplyColors color")
    {
      this->TreeView->hideColumn(j);
    }
  }

  // Redraw the view
  this->TreeView->update();
  this->ColumnView->update();
}

//----------------------------------------------------------------------------
void svtkQtTreeView::ApplyViewTheme(svtkViewTheme* theme)
{
  this->Superclass::ApplyViewTheme(theme);

  this->ApplyColors->SetPointLookupTable(theme->GetPointLookupTable());
  this->ApplyColors->SetDefaultPointColor(theme->GetPointColor());
  this->ApplyColors->SetDefaultPointOpacity(theme->GetPointOpacity());
  this->ApplyColors->SetDefaultCellColor(theme->GetCellColor());
  this->ApplyColors->SetDefaultCellOpacity(theme->GetCellOpacity());
  this->ApplyColors->SetSelectedPointColor(theme->GetSelectedPointColor());
  this->ApplyColors->SetSelectedPointOpacity(theme->GetSelectedPointOpacity());
  this->ApplyColors->SetSelectedCellColor(theme->GetSelectedCellColor());
  this->ApplyColors->SetSelectedCellOpacity(theme->GetSelectedCellOpacity());
  this->ApplyColors->SetScalePointLookupTable(theme->GetScalePointLookupTable());
  this->ApplyColors->SetScaleCellLookupTable(theme->GetScaleCellLookupTable());
}

//----------------------------------------------------------------------------
void svtkQtTreeView::Collapse(const QModelIndex& index)
{
  this->TreeView->collapse(index);
}

//----------------------------------------------------------------------------
void svtkQtTreeView::CollapseAll()
{
  this->TreeView->collapseAll();
}

//----------------------------------------------------------------------------
void svtkQtTreeView::Expand(const QModelIndex& index)
{
  this->TreeView->expand(index);
}

//----------------------------------------------------------------------------
void svtkQtTreeView::ExpandAll()
{
  this->TreeView->expandAll();
}

//----------------------------------------------------------------------------
void svtkQtTreeView::ExpandToDepth(int depth)
{
  this->TreeView->expandToDepth(depth);
}

//----------------------------------------------------------------------------
void svtkQtTreeView::ResizeColumnToContents(int column)
{
  this->TreeView->resizeColumnToContents(column);
}

//----------------------------------------------------------------------------
void svtkQtTreeView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
