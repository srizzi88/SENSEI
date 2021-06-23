/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtListView.cxx

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

#include "svtkQtListView.h"

#include <QItemSelection>
#include <QListView>
#include <QSortFilterProxyModel>

#include "svtkAbstractArray.h"
#include "svtkAddMembershipArray.h"
#include "svtkAlgorithm.h"
#include "svtkAlgorithmOutput.h"
#include "svtkAnnotation.h"
#include "svtkAnnotationLayers.h"
#include "svtkAnnotationLink.h"
#include "svtkApplyColors.h"
#include "svtkConvertSelection.h"
#include "svtkDataObjectToTable.h"
#include "svtkDataRepresentation.h"
#include "svtkDataSetAttributes.h"
#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkOutEdgeIterator.h"
#include "svtkQtTableModelAdapter.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkTable.h"
#include "svtkViewTheme.h"

svtkStandardNewMacro(svtkQtListView);

//----------------------------------------------------------------------------
svtkQtListView::svtkQtListView()
{
  this->ApplyColors = svtkSmartPointer<svtkApplyColors>::New();
  this->DataObjectToTable = svtkSmartPointer<svtkDataObjectToTable>::New();
  this->ApplyColors->SetInputConnection(0, this->DataObjectToTable->GetOutputPort(0));

  this->DataObjectToTable->SetFieldType(svtkDataObjectToTable::VERTEX_DATA);
  this->FieldType = svtkQtListView::VERTEX_DATA;

  this->ListView = new QListView();
  this->TableAdapter = new svtkQtTableModelAdapter();
  this->TableAdapter->SetDecorationLocation(svtkQtTableModelAdapter::ITEM);
  this->TableSorter = new QSortFilterProxyModel();
  this->TableSorter->setFilterCaseSensitivity(Qt::CaseInsensitive);
  this->TableSorter->setFilterRole(Qt::DisplayRole);
  this->TableSorter->setSourceModel(this->TableAdapter);
  this->ListView->setModel(this->TableSorter);
  this->ListView->setModelColumn(0);
  this->TableSorter->setFilterKeyColumn(0);
  this->TableAdapter->SetColorColumnName("svtkApplyColors color");

  // Set up some default properties
  this->ListView->setSelectionMode(QAbstractItemView::ExtendedSelection);
  this->ListView->setSelectionBehavior(QAbstractItemView::SelectRows);

  this->LastSelectionMTime = 0;
  this->LastInputMTime = 0;
  this->LastMTime = 0;
  this->ApplyRowColors = false;
  this->VisibleColumn = 0;
  this->TableAdapter->SetDecorationStrategy(svtkQtTableModelAdapter::NONE);

  this->ColorArrayNameInternal = nullptr;
  this->IconIndexArrayNameInternal = nullptr;
  double defCol[3] = { 0.827, 0.827, 0.827 };
  this->ApplyColors->SetDefaultPointColor(defCol);
  this->ApplyColors->SetUseCurrentAnnotationColor(true);

  QObject::connect(this->ListView->selectionModel(),
    SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
    SLOT(slotQtSelectionChanged(const QItemSelection&, const QItemSelection&)));
}

//----------------------------------------------------------------------------
svtkQtListView::~svtkQtListView()
{
  delete this->ListView;
  delete this->TableAdapter;
  delete this->TableSorter;
}

//----------------------------------------------------------------------------
QWidget* svtkQtListView::GetWidget()
{
  return this->ListView;
}

//----------------------------------------------------------------------------
void svtkQtListView::SetAlternatingRowColors(bool state)
{
  this->ListView->setAlternatingRowColors(state);
}

//----------------------------------------------------------------------------
void svtkQtListView::SetEnableDragDrop(bool state)
{
  this->ListView->setDragEnabled(state);
}

//----------------------------------------------------------------------------
void svtkQtListView::SetFieldType(int type)
{
  this->DataObjectToTable->SetFieldType(type);
  if (this->FieldType != type)
  {
    this->FieldType = type;
    this->Modified();
  }
}

void svtkQtListView::SetIconSheet(QImage sheet)
{
  this->TableAdapter->SetIconSheet(sheet);
}

void svtkQtListView::SetIconSheetSize(int w, int h)
{
  this->TableAdapter->SetIconSheetSize(w, h);
}

void svtkQtListView::SetIconSize(int w, int h)
{
  this->TableAdapter->SetIconSize(w, h);
}

void svtkQtListView::SetIconArrayName(const char* name)
{
  this->SetIconIndexArrayNameInternal(name);
  this->TableAdapter->SetIconIndexColumnName(name);
}

void svtkQtListView::SetDecorationStrategy(int value)
{
  this->TableAdapter->SetDecorationStrategy(value);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkQtListView::SetFilterRegExp(const QRegExp& pattern)
{
  this->ListView->selectionModel()->clearSelection();
  this->TableSorter->setFilterRegExp(pattern);
}

void svtkQtListView::SetColorByArray(bool b)
{
  this->ApplyColors->SetUsePointLookupTable(b);
}

bool svtkQtListView::GetColorByArray()
{
  return this->ApplyColors->GetUsePointLookupTable();
}

void svtkQtListView::SetColorArrayName(const char* name)
{
  this->SetColorArrayNameInternal(name);
  this->ApplyColors->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_ROWS, name);
}

const char* svtkQtListView::GetColorArrayName()
{
  return this->GetColorArrayNameInternal();
}

void svtkQtListView::SetVisibleColumn(int col)
{
  this->ListView->setModelColumn(col);
  this->TableSorter->setFilterKeyColumn(col);
  this->VisibleColumn = col;
}

void svtkQtListView::AddRepresentationInternal(svtkDataRepresentation* rep)
{
  svtkAlgorithmOutput *annConn, *conn;
  conn = rep->GetInputConnection();
  annConn = rep->GetInternalAnnotationOutputPort();

  this->DataObjectToTable->SetInputConnection(0, conn);

  if (annConn)
  {
    this->ApplyColors->SetInputConnection(1, annConn);
  }
}

void svtkQtListView::RemoveRepresentationInternal(svtkDataRepresentation* rep)
{
  svtkAlgorithmOutput *annConn, *conn;
  conn = rep->GetInputConnection();
  annConn = rep->GetInternalAnnotationOutputPort();

  this->DataObjectToTable->RemoveInputConnection(0, conn);
  this->ApplyColors->RemoveInputConnection(1, annConn);
  this->TableAdapter->SetSVTKDataObject(nullptr);
}

//----------------------------------------------------------------------------
void svtkQtListView::slotQtSelectionChanged(
  const QItemSelection& svtkNotUsed(s1), const QItemSelection& svtkNotUsed(s2))
{
  // Convert to the correct type of selection
  svtkDataObject* data = this->TableAdapter->GetSVTKDataObject();
  if (!data)
    return;

  // Map the selected rows through the sorter map before sending to model
  const QModelIndexList selectedRows = this->ListView->selectionModel()->selectedRows();
  QModelIndexList origRows;
  for (int i = 0; i < selectedRows.size(); ++i)
  {
    origRows.push_back(this->TableSorter->mapToSource(selectedRows[i]));
  }

  svtkSelection* SVTKIndexSelectList =
    this->TableAdapter->QModelIndexListToSVTKIndexSelection(origRows);

  // Convert to the correct type of selection
  svtkDataRepresentation* rep = this->GetRepresentation();
  svtkSmartPointer<svtkSelection> converted;
  converted.TakeReference(svtkConvertSelection::ToSelectionType(
    SVTKIndexSelectList, data, rep->GetSelectionType(), nullptr));

  // Call select on the representation
  rep->Select(this, converted);

  // Delete the selection list
  SVTKIndexSelectList->Delete();

  this->LastSelectionMTime = rep->GetAnnotationLink()->GetMTime();
}

//----------------------------------------------------------------------------
void svtkQtListView::SetSVTKSelection()
{
  svtkDataRepresentation* rep = this->GetRepresentation();
  svtkDataObject* d = this->TableAdapter->GetSVTKDataObject();
  svtkAlgorithmOutput* annConn = rep->GetInternalAnnotationOutputPort();
  svtkAnnotationLayers* a =
    svtkAnnotationLayers::SafeDownCast(annConn->GetProducer()->GetOutputDataObject(0));
  svtkSelection* s = a->GetCurrentAnnotation()->GetSelection();

  svtkSmartPointer<svtkSelection> selection;
  selection.TakeReference(svtkConvertSelection::ToSelectionType(
    s, d, svtkSelectionNode::INDICES, nullptr, svtkSelectionNode::ROW));

  if (!selection || selection->GetNumberOfNodes() == 0)
  {
    return;
  }

  if (selection->GetNode(0)->GetSelectionList()->GetNumberOfTuples())
  {
    QItemSelection qisList = this->TableAdapter->SVTKIndexSelectionToQItemSelection(selection);
    QItemSelection sortedSel = this->TableSorter->mapSelectionFromSource(qisList);

    // Here we want the qt model to have it's selection changed
    // but we don't want to emit the selection.
    QObject::disconnect(this->ListView->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
      SLOT(slotQtSelectionChanged(const QItemSelection&, const QItemSelection&)));

    this->ListView->selectionModel()->select(
      sortedSel, QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    QObject::connect(this->ListView->selectionModel(),
      SIGNAL(selectionChanged(const QItemSelection&, const QItemSelection&)), this,
      SLOT(slotQtSelectionChanged(const QItemSelection&, const QItemSelection&)));
  }
}

//----------------------------------------------------------------------------
void svtkQtListView::Update()
{
  svtkDataRepresentation* rep = this->GetRepresentation();
  if (!rep)
  {
    // Remove SVTK data from the adapter
    this->TableAdapter->SetSVTKDataObject(nullptr);
    this->ListView->update();
    return;
  }
  rep->Update();

  // Make the data current
  svtkAlgorithmOutput *selConn, *annConn, *conn;
  conn = rep->GetInputConnection();
  conn->GetProducer()->Update();
  annConn = rep->GetInternalAnnotationOutputPort();
  annConn->GetProducer()->Update();
  selConn = rep->GetInternalSelectionOutputPort();
  selConn->GetProducer()->Update();

  svtkDataObject* d = conn->GetProducer()->GetOutputDataObject(0);
  svtkMTimeType atime = rep->GetAnnotationLink()->GetMTime();
  if (d->GetMTime() > this->LastInputMTime || this->GetMTime() > this->LastMTime ||
    atime > this->LastSelectionMTime)
  {
    this->DataObjectToTable->Update();
    this->ApplyColors->Update();
    this->TableAdapter->SetSVTKDataObject(nullptr);
    this->TableAdapter->SetSVTKDataObject(this->ApplyColors->GetOutput());

    this->TableAdapter->SetColorColumnName("svtkApplyColors color");
    this->TableAdapter->SetIconIndexColumnName(this->IconIndexArrayNameInternal);

    if (atime > this->LastSelectionMTime)
    {
      this->SetSVTKSelection();
    }

    this->ListView->setModelColumn(this->VisibleColumn);

    this->LastSelectionMTime = atime;
    this->LastInputMTime = d->GetMTime();
    this->LastMTime = this->GetMTime();
  }

  this->ListView->update();
}

void svtkQtListView::ApplyViewTheme(svtkViewTheme* theme)
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
}

//----------------------------------------------------------------------------
void svtkQtListView::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "ApplyRowColors: " << (this->ApplyRowColors ? "true" : "false") << endl;
}
