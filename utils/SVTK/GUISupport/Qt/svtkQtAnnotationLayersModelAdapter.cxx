/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtAnnotationLayersModelAdapter.cxx

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
#include "svtkQtAnnotationLayersModelAdapter.h"

#include "svtkAnnotation.h"
#include "svtkAnnotationLayers.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkVariant.h"

#include <QHash>
#include <QIcon>
#include <QMap>
#include <QPixmap>

//----------------------------------------------------------------------------
svtkQtAnnotationLayersModelAdapter::svtkQtAnnotationLayersModelAdapter(QObject* p)
  : svtkQtAbstractModelAdapter(p)
{
  this->Annotations = nullptr;
}

//----------------------------------------------------------------------------
svtkQtAnnotationLayersModelAdapter::svtkQtAnnotationLayersModelAdapter(
  svtkAnnotationLayers* t, QObject* p)
  : svtkQtAbstractModelAdapter(p)
  , Annotations(t)
{
  if (this->Annotations != nullptr)
  {
    this->Annotations->Register(nullptr);
  }
}

//----------------------------------------------------------------------------
svtkQtAnnotationLayersModelAdapter::~svtkQtAnnotationLayersModelAdapter()
{
  if (this->Annotations != nullptr)
  {
    this->Annotations->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkQtAnnotationLayersModelAdapter::SetKeyColumnName(const char* svtkNotUsed(name))
{
  /*
    if (name == 0)
      {
      this->KeyColumn = -1;
      }
    else
      {
      this->KeyColumn = -1;
      for (int i = 0; i < static_cast<int>(this->Annotations->GetNumberOfColumns()); i++)
        {
        if (!strcmp(name, this->Annotations->GetColumn(i)->GetName()))
          {
          this->KeyColumn = i;
          break;
          }
        }
      }
      */
}

// ----------------------------------------------------------------------------
void svtkQtAnnotationLayersModelAdapter::SetColorColumnName(const char* svtkNotUsed(name)) {}

//----------------------------------------------------------------------------
void svtkQtAnnotationLayersModelAdapter::SetSVTKDataObject(svtkDataObject* obj)
{
  svtkAnnotationLayers* t = svtkAnnotationLayers::SafeDownCast(obj);
  if (obj && !t)
  {
    qWarning("svtkQtAnnotationLayersModelAdapter needs a svtkAnnotationLayers for SetSVTKDataObject");
    return;
  }

  // Okay it's a table so set it :)
  this->setAnnotationLayers(t);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkQtAnnotationLayersModelAdapter::GetSVTKDataObject() const
{
  return this->Annotations;
}

//----------------------------------------------------------------------------
void svtkQtAnnotationLayersModelAdapter::setAnnotationLayers(svtkAnnotationLayers* t)
{
  if (this->Annotations != nullptr)
  {
    this->Annotations->Delete();
  }
  this->Annotations = t;
  if (this->Annotations != nullptr)
  {
    this->Annotations->Register(nullptr);

    // When setting a table, update the QHash tables for column mapping.
    // If SplitMultiComponentColumns is disabled, this call will just clear
    // the tables and return.
    // this->updateModelColumnHashTables();

    // We will assume the table is totally
    // new and any views should update completely
    emit this->reset();
  }
}

//----------------------------------------------------------------------------
bool svtkQtAnnotationLayersModelAdapter::noAnnotationsCheck() const
{
  if (this->Annotations == nullptr)
  {
    // It's not necessarily an error to have a null pointer for the
    // table.  It just means that the model is empty.
    return true;
  }
  if (this->Annotations->GetNumberOfAnnotations() == 0)
  {
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
// Description:
// Selection conversion from SVTK land to Qt land
svtkAnnotationLayers* svtkQtAnnotationLayersModelAdapter::QModelIndexListToSVTKAnnotationLayers(
  const QModelIndexList qmil) const
{
  // Create svtk index selection
  svtkAnnotationLayers* annotations = svtkAnnotationLayers::New(); // Caller needs to delete

  // Run through the QModelIndexList pulling out svtk indexes
  for (int i = 0; i < qmil.size(); i++)
  {
    svtkIdType svtk_index = qmil.at(i).internalId();
    // annotations->AddLayer();
    annotations->AddAnnotation(this->Annotations->GetAnnotation(svtk_index));
  }
  return annotations;
}

//----------------------------------------------------------------------------
QItemSelection svtkQtAnnotationLayersModelAdapter::SVTKAnnotationLayersToQItemSelection(
  svtkAnnotationLayers* svtkNotUsed(svtkann)) const
{

  QItemSelection qis_list;
  /*
  svtkSelectionNode* node = svtksel->GetNode(0);
  if (node)
    {
    svtkIdTypeArray* arr = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());
    if (arr)
      {
      for (svtkIdType i = 0; i < arr->GetNumberOfTuples(); i++)
        {
        svtkIdType svtk_index = arr->GetValue(i);
        QModelIndex qmodel_index =
          this->createIndex(svtk_index, 0, static_cast<int>(svtk_index));
        qis_list.select(qmodel_index, qmodel_index);
        }
      }
    }
  */
  return qis_list;
}

//----------------------------------------------------------------------------
// Description:
// Selection conversion from SVTK land to Qt land
svtkSelection* svtkQtAnnotationLayersModelAdapter::QModelIndexListToSVTKIndexSelection(
  const QModelIndexList svtkNotUsed(qmil)) const
{
  /*
    // Create svtk index selection
    svtkSelection* IndexSelection = svtkSelection::New(); // Caller needs to delete
    svtkSmartPointer<svtkSelectionNode> node =
      svtkSmartPointer<svtkSelectionNode>::New();
    node->SetContentType(svtkSelectionNode::INDICES);
    node->SetFieldType(svtkSelectionNode::ROW);
    svtkSmartPointer<svtkIdTypeArray> index_arr =
      svtkSmartPointer<svtkIdTypeArray>::New();
    node->SetSelectionList(index_arr);
    IndexSelection->AddNode(node);

    // Run through the QModelIndexList pulling out svtk indexes
    for (int i = 0; i < qmil.size(); i++)
      {
      svtkIdType svtk_index = qmil.at(i).internalId();
      index_arr->InsertNextValue(svtk_index);
      }
    return IndexSelection;
    */
  return nullptr;
}

//----------------------------------------------------------------------------
QItemSelection svtkQtAnnotationLayersModelAdapter::SVTKIndexSelectionToQItemSelection(
  svtkSelection* svtkNotUsed(svtksel)) const
{

  QItemSelection qis_list;
  /*
  svtkSelectionNode* node = svtksel->GetNode(0);
  if (node)
    {
    svtkIdTypeArray* arr = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());
    if (arr)
      {
      for (svtkIdType i = 0; i < arr->GetNumberOfTuples(); i++)
        {
        svtkIdType svtk_index = arr->GetValue(i);
        QModelIndex qmodel_index =
          this->createIndex(svtk_index, 0, static_cast<int>(svtk_index));
        qis_list.select(qmodel_index, qmodel_index);
        }
      }
    }
    */
  return qis_list;
}

//----------------------------------------------------------------------------
QVariant svtkQtAnnotationLayersModelAdapter::data(const QModelIndex& idx, int role) const
{
  if (this->noAnnotationsCheck())
  {
    return QVariant();
  }
  if (!idx.isValid())
  {
    return QVariant();
  }
  if (idx.row() >= static_cast<int>(this->Annotations->GetNumberOfAnnotations()))
  {
    return QVariant();
  }

  svtkAnnotation* a = this->Annotations->GetAnnotation(idx.row());
  int numItems = 0;
  svtkSelection* s = a->GetSelection();
  if (s)
  {
    for (unsigned int i = 0; i < s->GetNumberOfNodes(); ++i)
    {
      numItems += s->GetNode(i)->GetSelectionList()->GetNumberOfTuples();
    }
  }

  double* color = a->GetInformation()->Get(svtkAnnotation::COLOR());
  int annColor[3];
  annColor[0] = static_cast<int>(255 * color[0]);
  annColor[1] = static_cast<int>(255 * color[1]);
  annColor[2] = static_cast<int>(255 * color[2]);

  if (role == Qt::DisplayRole)
  {
    switch (idx.column())
    {
      case 1:
        return QVariant(numItems);
      case 2:
        return QVariant(a->GetInformation()->Get(svtkAnnotation::LABEL()));
      default:
        return QVariant();
    }
  }
  else if (role == Qt::DecorationRole)
  {
    switch (idx.column())
    {
      case 0:
        return QColor(annColor[0], annColor[1], annColor[2]);
      default:
        return QVariant();
    }
  }

  return QVariant();
}

//----------------------------------------------------------------------------
bool svtkQtAnnotationLayersModelAdapter::setData(
  const QModelIndex& svtkNotUsed(idx), const QVariant& svtkNotUsed(value), int svtkNotUsed(role))
{
  /*
    if (role == Qt::DecorationRole)
      {
      this->Internal->IndexToDecoration[idx] = value;
      emit this->dataChanged(idx, idx);
      return true;
      }
   */
  return false;
}

//----------------------------------------------------------------------------
Qt::ItemFlags svtkQtAnnotationLayersModelAdapter::flags(const QModelIndex& idx) const
{
  if (!idx.isValid())
  {
    return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
  }

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
}

//----------------------------------------------------------------------------
QVariant svtkQtAnnotationLayersModelAdapter::headerData(
  int section, Qt::Orientation orientation, int role) const
{
  if (this->noAnnotationsCheck())
  {
    return QVariant();
  }

  if (orientation == Qt::Horizontal && role == Qt::DisplayRole)
  {
    switch (section)
    {
      case 0:
        return QVariant("C");
      case 1:
        return QVariant("# Items");
      case 2:
        return QVariant("Label");
      default:
        return QVariant();
    }
  }

  return QVariant();
}

//----------------------------------------------------------------------------
QModelIndex svtkQtAnnotationLayersModelAdapter::index(
  int row, int column, const QModelIndex& svtkNotUsed(parentIdx)) const
{
  return createIndex(row, column, row);
}

//----------------------------------------------------------------------------
QModelIndex svtkQtAnnotationLayersModelAdapter::parent(const QModelIndex& svtkNotUsed(idx)) const
{
  return QModelIndex();
}

//----------------------------------------------------------------------------
int svtkQtAnnotationLayersModelAdapter::rowCount(const QModelIndex& mIndex) const
{
  if (this->noAnnotationsCheck())
  {
    return 0;
  }
  if (mIndex == QModelIndex())
  {
    return this->Annotations->GetNumberOfAnnotations();
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkQtAnnotationLayersModelAdapter::columnCount(const QModelIndex&) const
{
  if (this->noAnnotationsCheck())
  {
    return 0;
  }

  return 3;
}
/*
Qt::DropActions svtkQtAnnotationLayersModelAdapter::supportedDropActions() const
{
   return Qt::MoveAction;
}

Qt::DropActions svtkQtAnnotationLayersModelAdapter::supportedDragActions() const
{
   return Qt::MoveAction;
}

bool svtkQtAnnotationLayersModelAdapter::insertRows(int row, int count, const QModelIndex &p)
{
  emit this->beginInsertRows(p,row,row+count-1);
  for(int i=0; i<count; ++i)
    {
    this->Annotations->InsertLayer(row);
    }
  emit this->endInsertRows();

  return true;
}

bool svtkQtAnnotationLayersModelAdapter::removeRows(int row, int count, const QModelIndex &p)
{
  emit this->beginRemoveRows(p,row,row+count-1);
  for(int i=0; i<count; ++i)
    {
    this->Annotations->RemoveAnnotation(this->Annotations->GetAnnotation(row));
    }
  emit this->endRemoveRows();

  return true;
}

bool svtkQtAnnotationLayersModelAdapter::dropMimeData(const QMimeData *data,
     Qt::DropAction action, int row, int column, const QModelIndex &parent)
{
  if (action == Qt::IgnoreAction)
    return true;

  if (!data->hasFormat("application/vnd.text.list"))
    return false;

  if (column > 0)
    return false;


}

QStringList svtkQtAnnotationLayersModelAdapter::mimeTypes() const
{
  QStringList types;
  types << "application/x-color" << "text/plain";
  return types;
}

QMimeData *svtkQtAnnotationLayersModelAdapter::mimeData(const QModelIndexList &indexes) const
{
  QMimeData *mimeData = new QMimeData();
  QByteArray encodedData;

  QDataStream stream(&encodedData, QIODevice::WriteOnly);

  foreach (QModelIndex index, indexes) {
     if (index.isValid()) {
         stream << data(index, Qt::DisplayRole).toByteArray();
     }
  }

  mimeData->setData("application/vnd.text.list", encodedData);
  return mimeData;
}
*/
