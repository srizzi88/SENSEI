/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtTreeModelAdapter.cxx

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
#include "svtkQtTreeModelAdapter.h"

#include "svtkAdjacentVertexIterator.h"
#include "svtkAnnotation.h"
#include "svtkArrayIteratorIncludes.h"
#include "svtkConvertSelection.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkPointData.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkStringArray.h"
#include "svtkTree.h"
#include "svtkUnicodeStringArray.h"
#include "svtkUnsignedCharArray.h"
#include "svtkVariantArray.h"

#include <QBrush>
#include <QIcon>
#include <QMimeData>
#include <QPainter>
#include <QPen>
#include <QPixmap>
#include <algorithm>

#include <set>
#include <sstream>

svtkQtTreeModelAdapter::svtkQtTreeModelAdapter(QObject* p, svtkTree* t)
  : svtkQtAbstractModelAdapter(p)
{
  this->TreeMTime = 0;
  this->Tree = nullptr;
  this->setTree(t);
  this->ChildIterator = svtkAdjacentVertexIterator::New();
}

svtkQtTreeModelAdapter::~svtkQtTreeModelAdapter()
{
  if (this->Tree)
  {
    this->Tree->Delete();
  }
  this->ChildIterator->Delete();
}

void svtkQtTreeModelAdapter::SetColorColumnName(const char* name)
{
  if (name == nullptr)
  {
    this->ColorColumn = -1;
  }
  else
  {
    this->ColorColumn = -1;
    for (int i = 0; i < this->Tree->GetVertexData()->GetNumberOfArrays(); i++)
    {
      if (!strcmp(name, this->Tree->GetVertexData()->GetAbstractArray(i)->GetName()))
      {
        this->ColorColumn = i;
        break;
      }
    }
  }
}

void svtkQtTreeModelAdapter::SetKeyColumnName(const char* name)
{
  if (name == nullptr)
  {
    this->KeyColumn = -1;
  }
  else
  {
    this->KeyColumn = -1;
    for (int i = 0; i < this->Tree->GetVertexData()->GetNumberOfArrays(); i++)
    {
      if (!strcmp(name, this->Tree->GetVertexData()->GetAbstractArray(i)->GetName()))
      {
        this->KeyColumn = i;
        break;
      }
    }
  }
}

void svtkQtTreeModelAdapter::SetSVTKDataObject(svtkDataObject* obj)
{
  svtkTree* t = svtkTree::SafeDownCast(obj);
  if (obj && !t)
  {
    cerr << "svtkQtTreeModelAdapter needs a svtkTree for SetSVTKDataObject" << endl;
    return;
  }

  // Okay it's a tree so set it :)
  this->setTree(t);
}

svtkDataObject* svtkQtTreeModelAdapter::GetSVTKDataObject() const
{
  return this->Tree;
}

svtkMTimeType svtkQtTreeModelAdapter::GetSVTKDataObjectMTime() const
{
  return this->TreeMTime;
}

void svtkQtTreeModelAdapter::setTree(svtkTree* t)
{
  if (!t || (t != this->Tree))
  {
    svtkTree* tempSGMacroVar = this->Tree;
    this->Tree = t;
    if (this->Tree != nullptr)
    {
      this->Tree->Register(nullptr);
      svtkIdType root = this->Tree->GetRoot();
      this->SVTKIndexToQtModelIndex.clear();
      this->SVTKIndexToQtModelIndex.resize(this->Tree->GetNumberOfVertices());
      if (root >= 0)
      {
        this->GenerateSVTKIndexToQtModelIndex(root, this->createIndex(0, 0, static_cast<int>(root)));
      }
      this->TreeMTime = this->Tree->GetMTime();
    }
    if (tempSGMacroVar != nullptr)
    {
      tempSGMacroVar->UnRegister(nullptr);
    }
    emit reset();
  }

  // Okay it's the same pointer but the contents
  // of the tree might have been modified so
  // check for that condition
  else if (this->Tree->GetMTime() != this->TreeMTime)
  {
    this->treeModified();
  }
}

void svtkQtTreeModelAdapter::treeModified()
{
  this->SVTKIndexToQtModelIndex.clear();
  if (this->Tree->GetNumberOfVertices() > 0)
  {
    svtkIdType root = this->Tree->GetRoot();
    this->SVTKIndexToQtModelIndex.resize(this->Tree->GetNumberOfVertices());
    this->GenerateSVTKIndexToQtModelIndex(root, this->createIndex(0, 0, static_cast<int>(root)));
  }
  this->TreeMTime = this->Tree->GetMTime();
  emit reset();
}

// Description:
// Selection conversion from SVTK land to Qt land
svtkSelection* svtkQtTreeModelAdapter::QModelIndexListToSVTKIndexSelection(
  const QModelIndexList qmil) const
{
  // Create svtk index selection
  svtkSelection* IndexSelection = svtkSelection::New(); // Caller needs to delete
  svtkSmartPointer<svtkSelectionNode> node = svtkSmartPointer<svtkSelectionNode>::New();
  node->SetContentType(svtkSelectionNode::INDICES);
  node->SetFieldType(svtkSelectionNode::VERTEX);
  svtkSmartPointer<svtkIdTypeArray> index_arr = svtkSmartPointer<svtkIdTypeArray>::New();
  node->SetSelectionList(index_arr);
  IndexSelection->AddNode(node);

  // Run through the QModelIndexList pulling out svtk indexes
  std::set<int> unique_ids;
  for (int i = 0; i < qmil.size(); i++)
  {
    unique_ids.insert(qmil.at(i).internalId());
  }

  std::set<int>::iterator iter;
  for (iter = unique_ids.begin(); iter != unique_ids.end(); ++iter)
  {
    index_arr->InsertNextValue(*iter);
  }

  return IndexSelection;
}

QItemSelection svtkQtTreeModelAdapter::SVTKIndexSelectionToQItemSelection(svtkSelection* svtksel) const
{
  QItemSelection qis_list;
  for (unsigned int j = 0; j < svtksel->GetNumberOfNodes(); ++j)
  {
    svtkSelectionNode* node = svtksel->GetNode(j);
    if (node && node->GetFieldType() == svtkSelectionNode::VERTEX)
    {
      svtkIdTypeArray* arr = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());
      if (arr)
      {
        for (svtkIdType i = 0; i < arr->GetNumberOfTuples(); i++)
        {
          svtkIdType svtk_index = arr->GetValue(i);
          QModelIndex qmodel_index = this->SVTKIndexToQtModelIndex[svtk_index];
          qis_list.select(qmodel_index, qmodel_index);
        }
      }
    }
  }
  return qis_list;
}

void svtkQtTreeModelAdapter::GenerateSVTKIndexToQtModelIndex(
  svtkIdType svtk_index, QModelIndex qmodel_index)
{

  // Store the QModelIndex for selection conversions later
  this->SVTKIndexToQtModelIndex.replace(svtk_index, qmodel_index);

  // Iterate through the children of this tree nodes
  svtkAdjacentVertexIterator* it = svtkAdjacentVertexIterator::New();
  this->Tree->GetChildren(svtk_index, it);
  int i = 0;
  while (it->HasNext())
  {
    svtkIdType svtk_child_index = it->Next();
    this->GenerateSVTKIndexToQtModelIndex(
      svtk_child_index, this->createIndex(i, 0, static_cast<int>(svtk_child_index)));
    ++i;
  }
  it->Delete();
}

//----------------------------------------------------------------------------
QVariant svtkQtTreeModelAdapterArrayValue(svtkAbstractArray* arr, svtkIdType i, svtkIdType j)
{
  int comps = arr->GetNumberOfComponents();
  if (svtkDataArray* const data = svtkArrayDownCast<svtkDataArray>(arr))
  {
    return QVariant(data->GetComponent(i, j));
  }

  if (svtkStringArray* const data = svtkArrayDownCast<svtkStringArray>(arr))
  {
    return QVariant(data->GetValue(i * comps + j));
  }

  if (svtkUnicodeStringArray* const data = svtkArrayDownCast<svtkUnicodeStringArray>(arr))
  {
    return QVariant(QString::fromUtf8(data->GetValue(i * comps + j).utf8_str()));
  }

  if (svtkVariantArray* const data = svtkArrayDownCast<svtkVariantArray>(arr))
  {
    return QVariant(QString(data->GetValue(i * comps + j).ToString().c_str()));
  }

  svtkGenericWarningMacro("Unknown array type in svtkQtTreeModelAdapterArrayValue.");
  return QVariant();
}

QVariant svtkQtTreeModelAdapter::data(const QModelIndex& idx, int role) const
{
  if (!this->Tree)
  {
    return QVariant();
  }

  if (!idx.isValid())
  {
    return QVariant();
  }

  // if (role == Qt::DecorationRole)
  //  {
  //  return this->IndexToDecoration[idx];
  //  }

  svtkIdType vertex = static_cast<svtkIdType>(idx.internalId());
  int column = this->ModelColumnToFieldDataColumn(idx.column());
  svtkAbstractArray* arr = this->Tree->GetVertexData()->GetAbstractArray(column);
  if (role == Qt::DisplayRole)
  {
    return QString::fromUtf8(arr->GetVariantValue(vertex).ToUnicodeString().utf8_str()).trimmed();
  }
  else if (role == Qt::UserRole)
  {
    return svtkQtTreeModelAdapterArrayValue(arr, vertex, 0);
  }

  if (this->ColorColumn >= 0)
  {
    int colorColumn = this->ModelColumnToFieldDataColumn(this->ColorColumn);
    svtkUnsignedCharArray* colors = svtkArrayDownCast<svtkUnsignedCharArray>(
      this->Tree->GetVertexData()->GetAbstractArray(colorColumn));
    if (!colors)
    {
      return QVariant();
    }

    const int nComponents = colors->GetNumberOfComponents();
    if (nComponents < 3)
    {
      return QVariant();
    }

    unsigned char rgba[4];
    colors->GetTypedTuple(vertex, rgba);
    int rgb[3];
    rgb[0] = static_cast<int>(0x0ff & rgba[0]);
    rgb[1] = static_cast<int>(0x0ff & rgba[1]);
    rgb[2] = static_cast<int>(0x0ff & rgba[2]);

    if (role == Qt::DecorationRole)
    {
      QPixmap pixmap(12, 12);
      pixmap.fill(QColor(0, 0, 0, 0));
      QPainter painter(&pixmap);
      painter.setRenderHint(QPainter::Antialiasing);
      painter.setPen(Qt::NoPen);
      painter.setBrush(QBrush(QColor(rgb[0], rgb[1], rgb[2])));
      if (this->rowCount(idx) > 0)
      {
        painter.drawEllipse(0, 0, 11, 11);
      }
      else
      {
        painter.drawEllipse(2, 2, 7, 7);
      }
      return QVariant(pixmap);
    }
    else if (role == Qt::TextColorRole)
    {
      // return QVariant(QColor(rgb[0],rgb[1],rgb[2]));
    }
  }

  return QVariant();
}

bool svtkQtTreeModelAdapter::setData(const QModelIndex& idx, const QVariant& value, int role)
{
  if (role == Qt::DecorationRole)
  {
    this->IndexToDecoration[idx] = value;
    emit this->dataChanged(idx, idx);
    return true;
  }
  return false;
}

Qt::ItemFlags svtkQtTreeModelAdapter::flags(const QModelIndex& idx) const
{
  if (!idx.isValid())
  {
    return Qt::ItemIsEnabled;
  }

  Qt::ItemFlags itemFlags = Qt::ItemIsEnabled | Qt::ItemIsSelectable;

  if (!this->hasChildren(idx))
  {
    itemFlags = itemFlags | Qt::ItemIsDragEnabled;
  }

  return itemFlags;
}

QVariant svtkQtTreeModelAdapter::headerData(int section, Qt::Orientation orientation, int role) const
{

  // For horizontal headers, try to convert the column names to double.
  // If it doesn't work, return a string.
  if (orientation == Qt::Horizontal && (role == Qt::DisplayRole || role == Qt::UserRole))
  {
    section = this->ModelColumnToFieldDataColumn(section);
    QVariant svar(this->Tree->GetVertexData()->GetArrayName(section));
    bool ok;
    double value = svar.toDouble(&ok);
    if (ok)
    {
      return QVariant(value);
    }
    return svar;
  }

  // For vertical headers, return values in the key column if
  // KeyColumn is valid.
  if (orientation == Qt::Vertical && this->KeyColumn != -1 &&
    (role == Qt::DisplayRole || role == Qt::UserRole))
  {
    return QVariant(this->Tree->GetVertexData()->GetArrayName(this->KeyColumn));
  }

  return QVariant();
}

QModelIndex svtkQtTreeModelAdapter::index(int row, int column, const QModelIndex& parentIdx) const
{
  if (!this->Tree)
  {
    return QModelIndex();
  }

  svtkIdType parentItem;

  if (!parentIdx.isValid())
  {
    if (row == 0)
    {
      return createIndex(row, column, static_cast<int>(this->Tree->GetRoot()));
    }
    else
    {
      return QModelIndex();
    }
  }
  else
  {
    parentItem = static_cast<svtkIdType>(parentIdx.internalId());
  }

  this->Tree->GetChildren(parentItem, this->ChildIterator);
  if (row < this->Tree->GetNumberOfChildren(parentItem))
  {
    svtkIdType child = this->ChildIterator->Next();
    for (int i = 0; i < row; ++i)
    {
      child = this->ChildIterator->Next();
    }
    return createIndex(row, column, static_cast<int>(child));
  }
  else
  {
    return QModelIndex();
  }
}

QModelIndex svtkQtTreeModelAdapter::parent(const QModelIndex& idx) const
{
  if (!this->Tree)
  {
    return QModelIndex();
  }

  if (!idx.isValid())
  {
    return QModelIndex();
  }

  svtkIdType child = static_cast<svtkIdType>(idx.internalId());

  if (child == this->Tree->GetRoot())
  {
    return QModelIndex();
  }

  svtkIdType parentId = this->Tree->GetParent(child);

  if (parentId == this->Tree->GetRoot())
  {
    return createIndex(0, 0, static_cast<int>(parentId));
  }

  svtkIdType grandparentId = this->Tree->GetParent(parentId);

  svtkIdType row = -1;
  this->Tree->GetChildren(grandparentId, this->ChildIterator);
  int i = 0;
  while (this->ChildIterator->HasNext())
  {
    if (this->ChildIterator->Next() == parentId)
    {
      row = i;
      break;
    }
    ++i;
  }

  return createIndex(row, 0, static_cast<int>(parentId));
}

int svtkQtTreeModelAdapter::rowCount(const QModelIndex& idx) const
{
  if (!this->Tree)
  {
    return 1;
  }

  if (!idx.isValid())
  {
    return 1;
  }

  svtkIdType parentId = static_cast<svtkIdType>(idx.internalId());
  return this->Tree->GetNumberOfChildren(parentId);
}

int svtkQtTreeModelAdapter::columnCount(const QModelIndex& svtkNotUsed(parentIdx)) const
{
  if (!this->Tree)
  {
    return 0;
  }

  int numArrays = this->Tree->GetVertexData()->GetNumberOfArrays();
  int numDataArrays = this->DataEndColumn - this->DataStartColumn + 1;
  switch (this->ViewType)
  {
    case FULL_VIEW:
      return numArrays;
    case DATA_VIEW:
      return numDataArrays;
    default:
      svtkGenericWarningMacro("svtkQtTreeModelAdapter: Bad view type.");
  };
  return 0;
}

QStringList svtkQtTreeModelAdapter::mimeTypes() const
{
  QStringList types;
  types << "svtk/selection";
  return types;
}

QMimeData* svtkQtTreeModelAdapter::mimeData(const QModelIndexList& indexes) const
{
  // Only supports dragging single item right now ...

  if (indexes.size() == 0)
  {
    return nullptr;
  }

  svtkSmartPointer<svtkSelection> indexSelection = svtkSmartPointer<svtkSelection>::New();
  indexSelection.TakeReference(QModelIndexListToSVTKIndexSelection(indexes));

  svtkSmartPointer<svtkSelection> pedigreeIdSelection = svtkSmartPointer<svtkSelection>::New();
  pedigreeIdSelection.TakeReference(svtkConvertSelection::ToSelectionType(
    indexSelection, this->Tree, svtkSelectionNode::PEDIGREEIDS));

  if (pedigreeIdSelection->GetNode(0) == nullptr ||
    pedigreeIdSelection->GetNode(0)->GetSelectionList()->GetNumberOfTuples() == 0)
  {
    return nullptr;
  }

  std::ostringstream buffer;
  buffer << pedigreeIdSelection;

  QMimeData* mime_data = new QMimeData();
  mime_data->setData("svtk/selection", buffer.str().c_str());

  return mime_data;
}

Qt::DropActions svtkQtTreeModelAdapter::supportedDragActions() const
{
  return Qt::CopyAction;
}
