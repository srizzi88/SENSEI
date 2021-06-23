/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtTableModelAdapter.cxx

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
#include "svtkQtTableModelAdapter.h"

#include "svtkConvertSelection.h"
#include "svtkDataSetAttributes.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStdString.h"
#include "svtkTable.h"
#include "svtkUnsignedCharArray.h"
#include "svtkVariant.h"

#include <QColor>
#include <QHash>
#include <QIcon>
#include <QImage>
#include <QMap>
#include <QMimeData>
#include <QPainter>
#include <QPair>
#include <QPixmap>

#include <set>
#include <sstream>

//----------------------------------------------------------------------------
class svtkQtTableModelAdapter::svtkInternal
{
public:
  svtkInternal() {}
  ~svtkInternal() {}

  QHash<QModelIndex, QVariant> IndexToDecoration;
  QHash<int, QPair<svtkIdType, int> > ModelColumnToTableColumn;
  QHash<int, QString> ModelColumnNames;
  QHash<svtkIdType, svtkSmartPointer<svtkDoubleArray> > MagnitudeColumns;
};

//----------------------------------------------------------------------------
svtkQtTableModelAdapter::svtkQtTableModelAdapter(QObject* p)
  : svtkQtAbstractModelAdapter(p)
{
  this->Internal = new svtkInternal;
  this->Table = nullptr;
  this->SplitMultiComponentColumns = false;
  this->DecorationLocation = svtkQtTableModelAdapter::HEADER;
  this->DecorationStrategy = svtkQtTableModelAdapter::NONE;
  this->ColorColumn = -1;
  this->IconIndexColumn = -1;
  this->IconSheetSize[0] = this->IconSheetSize[1] = 0;
  this->IconSize[0] = this->IconSize[1] = 0;
}

//----------------------------------------------------------------------------
svtkQtTableModelAdapter::svtkQtTableModelAdapter(svtkTable* t, QObject* p)
  : svtkQtAbstractModelAdapter(p)
  , Table(t)
{
  this->Internal = new svtkInternal;
  this->SplitMultiComponentColumns = false;
  this->DecorationLocation = svtkQtTableModelAdapter::HEADER;
  this->DecorationStrategy = svtkQtTableModelAdapter::NONE;
  this->ColorColumn = -1;
  this->IconIndexColumn = -1;
  this->IconSheetSize[0] = this->IconSheetSize[1] = 0;
  this->IconSize[0] = this->IconSize[1] = 0;
  if (this->Table != nullptr)
  {
    this->Table->Register(nullptr);
  }
}

//----------------------------------------------------------------------------
svtkQtTableModelAdapter::~svtkQtTableModelAdapter()
{
  if (this->Table != nullptr)
  {
    this->Table->Delete();
  }
  delete this->Internal;
}

//----------------------------------------------------------------------------
void svtkQtTableModelAdapter::SetColorColumnName(const char* name)
{
  int color_column = this->ColorColumn;
  if (name == nullptr || !this->Table)
  {
    this->ColorColumn = -1;
  }
  else if (this->SplitMultiComponentColumns)
  {
    this->ColorColumn = -1;
    int color_index = 0;
    foreach (QString columnname, this->Internal->ModelColumnNames)
    {
      if (columnname == name)
      {
        this->ColorColumn = color_index;
        break;
      }
      color_index++;
    }
  }
  else
  {
    this->ColorColumn = -1;
    for (int i = 0; i < static_cast<int>(this->Table->GetNumberOfColumns()); i++)
    {
      if (!strcmp(name, this->Table->GetColumn(i)->GetName()))
      {
        this->ColorColumn = i;
        break;
      }
    }
  }
  if (this->ColorColumn != color_column)
  {
    emit this->reset();
  }
}

//----------------------------------------------------------------------------
void svtkQtTableModelAdapter::SetIconIndexColumnName(const char* name)
{
  int color_column = this->IconIndexColumn;
  if (name == nullptr || !this->Table)
  {
    this->IconIndexColumn = -1;
  }
  else if (this->SplitMultiComponentColumns)
  {
    this->IconIndexColumn = -1;
    int color_index = 0;
    foreach (QString columnname, this->Internal->ModelColumnNames)
    {
      if (columnname == name)
      {
        this->IconIndexColumn = color_index;
        break;
      }
      color_index++;
    }
  }
  else
  {
    this->IconIndexColumn = -1;
    for (int i = 0; i < static_cast<int>(this->Table->GetNumberOfColumns()); i++)
    {
      if (!strcmp(name, this->Table->GetColumn(i)->GetName()))
      {
        this->IconIndexColumn = i;
        break;
      }
    }
  }
  if (this->IconIndexColumn != color_column)
  {
    emit this->reset();
  }
}

//----------------------------------------------------------------------------
void svtkQtTableModelAdapter::SetKeyColumnName(const char* name)
{
  int key_column = this->KeyColumn;
  if (name == nullptr || !this->Table)
  {
    this->KeyColumn = -1;
  }
  else if (this->SplitMultiComponentColumns)
  {
    this->KeyColumn = -1;
    int key_index = 0;
    foreach (QString columnname, this->Internal->ModelColumnNames)
    {
      if (columnname == name)
      {
        this->KeyColumn = key_index;
        break;
      }
      key_index++;
    }
  }
  else
  {
    this->KeyColumn = -1;
    for (int i = 0; i < static_cast<int>(this->Table->GetNumberOfColumns()); i++)
    {
      if (!strcmp(name, this->Table->GetColumn(i)->GetName()))
      {
        this->KeyColumn = i;
        break;
      }
    }
  }
  if (this->KeyColumn != key_column)
  {
    emit this->reset();
  }
}

//----------------------------------------------------------------------------
void svtkQtTableModelAdapter::SetSVTKDataObject(svtkDataObject* obj)
{
  svtkTable* t = svtkTable::SafeDownCast(obj);
  if (obj && !t)
  {
    qWarning("svtkQtTableModelAdapter needs a svtkTable for SetSVTKDataObject");
    return;
  }

  // Okay it's a table so set it :)
  this->setTable(t);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkQtTableModelAdapter::GetSVTKDataObject() const
{
  return this->Table;
}

//----------------------------------------------------------------------------
void svtkQtTableModelAdapter::updateModelColumnHashTables()
{
  // Clear the hash tables
  this->Internal->ModelColumnToTableColumn.clear();
  this->Internal->ModelColumnNames.clear();
  this->Internal->MagnitudeColumns.clear();

  // Do not continue if SplitMultiComponentColumns is false
  // or our table is null.
  if (!this->SplitMultiComponentColumns || !this->Table)
  {
    return;
  }

  // Get the start and end columns.
  int startColumn = 0;
  int endColumn = this->Table->GetNumberOfColumns() - 1;
  if (this->GetViewType() == DATA_VIEW)
  {
    startColumn = this->DataStartColumn;
    endColumn = this->DataEndColumn;
  }

  // Double check to make sure startColumn and endColumn are within bounds
  int maxColumn = this->Table->GetNumberOfColumns() - 1;
  if ((startColumn < 0 || startColumn > maxColumn) || (endColumn < 0 || endColumn > maxColumn))
  {
    return;
  }

  // For each column in the svtkTable, iterate over the column's number of
  // components to construct a mapping from qt model columns to
  // svtkTable columns-component pairs.  Also generate qt model column names.
  int modelColumn = 0;
  for (int tableColumn = startColumn; tableColumn <= endColumn; ++tableColumn)
  {
    const int nComponents = this->Table->GetColumn(tableColumn)->GetNumberOfComponents();
    for (int c = 0; c < nComponents; ++c)
    {
      QString columnName = this->Table->GetColumnName(tableColumn);
      if (nComponents != 1)
      {
        columnName = QString("%1 (%2)").arg(columnName).arg(c);
      }
      this->Internal->ModelColumnNames[modelColumn] = columnName;
      this->Internal->ModelColumnToTableColumn[modelColumn++] =
        QPair<svtkIdType, int>(tableColumn, c);
    }

    // If number of components is greater than 1, create a new column for Magnitude
    svtkDataArray* dataArray = svtkArrayDownCast<svtkDataArray>(this->Table->GetColumn(tableColumn));
    if (nComponents > 1 && dataArray)
    {
      svtkSmartPointer<svtkDoubleArray> magArray = svtkSmartPointer<svtkDoubleArray>::New();
      magArray->SetNumberOfComponents(1);
      const svtkIdType nTuples = dataArray->GetNumberOfTuples();
      for (svtkIdType i = 0; i < nTuples; ++i)
      {
        double mag = 0;
        for (int j = 0; j < nComponents; ++j)
        {
          double tmp = dataArray->GetComponent(i, j);
          mag += tmp * tmp;
        }
        mag = sqrt(mag);
        magArray->InsertNextValue(mag);
      }

      // Create a name for this column and add it to the ModelColumnNames map
      QString columnName = this->Table->GetColumnName(tableColumn);
      columnName = QString("%1 (Magnitude)").arg(columnName);
      this->Internal->ModelColumnNames[modelColumn] = columnName;

      // Store the magnitude column mapped to its corresponding column in the svtkTable
      this->Internal->MagnitudeColumns[tableColumn] = magArray;

      // Add a pair that has the svtkTable column and component, but use a component value
      // that is out of range to signal that this column represents magnitude
      this->Internal->ModelColumnToTableColumn[modelColumn++] =
        QPair<svtkIdType, int>(tableColumn, nComponents);
    }
  }
}

//----------------------------------------------------------------------------
void svtkQtTableModelAdapter::setTable(svtkTable* t)
{
  if (this->Table != nullptr)
  {
    this->Table->Delete();
  }
  this->Table = t;
  if (this->Table != nullptr)
  {
    this->Table->Register(nullptr);

    // When setting a table, update the QHash tables for column mapping.
    // If SplitMultiComponentColumns is disabled, this call will just clear
    // the tables and return.
    this->updateModelColumnHashTables();

    // We will assume the table is totally
    // new and any views should update completely
    emit this->reset();
  }
}

//----------------------------------------------------------------------------
bool svtkQtTableModelAdapter::noTableCheck() const
{
  if (this->Table == nullptr)
  {
    // It's not necessarily an error to have a null pointer for the
    // table.  It just means that the model is empty.
    return true;
  }
  if (this->Table->GetNumberOfRows() == 0)
  {
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
// Description:
// Selection conversion from SVTK land to Qt land
svtkSelection* svtkQtTableModelAdapter::QModelIndexListToSVTKIndexSelection(
  const QModelIndexList qmil) const
{
  // Create svtk index selection
  svtkSelection* IndexSelection = svtkSelection::New(); // Caller needs to delete
  svtkSmartPointer<svtkSelectionNode> node = svtkSmartPointer<svtkSelectionNode>::New();
  node->SetContentType(svtkSelectionNode::INDICES);
  node->SetFieldType(svtkSelectionNode::ROW);
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

//----------------------------------------------------------------------------
QItemSelection svtkQtTableModelAdapter::SVTKIndexSelectionToQItemSelection(svtkSelection* svtksel) const
{

  QItemSelection qis_list;
  svtkSelectionNode* node = svtksel->GetNode(0);
  if (node)
  {
    svtkIdTypeArray* arr = svtkArrayDownCast<svtkIdTypeArray>(node->GetSelectionList());
    if (arr)
    {
      for (svtkIdType i = 0; i < arr->GetNumberOfTuples(); i++)
      {
        svtkIdType svtk_index = arr->GetValue(i);
        QModelIndex qmodel_index = this->createIndex(svtk_index, 0, static_cast<int>(svtk_index));
        qis_list.select(qmodel_index, qmodel_index);
      }
    }
  }
  return qis_list;
}

//----------------------------------------------------------------------------
bool svtkQtTableModelAdapter::GetSplitMultiComponentColumns() const
{
  return this->SplitMultiComponentColumns;
}

//----------------------------------------------------------------------------
void svtkQtTableModelAdapter::SetSplitMultiComponentColumns(bool value)
{
  if (value != this->SplitMultiComponentColumns)
  {
    this->SplitMultiComponentColumns = value;
    this->updateModelColumnHashTables();
  }
}

//----------------------------------------------------------------------------
void svtkQtTableModelAdapter::SetDecorationStrategy(int s)
{
  if (s != this->DecorationStrategy)
  {
    this->DecorationStrategy = s;
  }
}

//----------------------------------------------------------------------------
void svtkQtTableModelAdapter::SetDecorationLocation(int s)
{
  if (s != this->DecorationLocation)
  {
    this->DecorationLocation = s;
  }
}

//----------------------------------------------------------------------------
QVariant svtkQtTableModelAdapter::data(const QModelIndex& idx, int role) const
{
  if (this->noTableCheck())
  {
    return QVariant();
  }
  if (!idx.isValid())
  {
    return QVariant();
  }

  // Map the qt model column to a column in the svtk table and
  // get the value from the table as a svtkVariant
  svtkVariant v;
  this->getValue(idx.row(), idx.column(), v);

  // Return a string if they ask for a display role
  if (role == Qt::DisplayRole)
  {
    bool ok;
    double value = v.ToDouble(&ok);
    if (ok)
    {
      return QVariant(value);
    }
    else
    {
      return QString::fromUtf8(v.ToUnicodeString().utf8_str()).trimmed();
    }
  }

  // Return a byte array if they ask for a decorate role
  if (role == Qt::DecorationRole)
  {
    if (this->DecorationStrategy == svtkQtTableModelAdapter::COLORS &&
      this->DecorationLocation == svtkQtTableModelAdapter::ITEM && this->ColorColumn >= 0)
    {
      return this->getColorIcon(idx.row());
    }
    else if (this->DecorationStrategy == svtkQtTableModelAdapter::ICONS &&
      this->DecorationLocation == svtkQtTableModelAdapter::ITEM && this->IconIndexColumn >= 0)
    {
      return this->getIcon(idx.row());
    }
    return this->Internal->IndexToDecoration[idx];
  }

  // Return a raw value if they ask for a user role
  if (role == Qt::UserRole)
  {
    if (v.IsNumeric())
    {
      return QVariant(v.ToDouble());
    }
    return QVariant(v.ToString().c_str());
  }

  // Hmmm... if the role isn't decorate, user or display
  // then just punt and return a empty qvariant
  return QVariant();
}

//----------------------------------------------------------------------------
bool svtkQtTableModelAdapter::setData(const QModelIndex& idx, const QVariant& value, int role)
{
  if (role == Qt::DecorationRole)
  {
    this->Internal->IndexToDecoration[idx] = value;
    emit this->dataChanged(idx, idx);
    return true;
  }
  return false;
}

//----------------------------------------------------------------------------
Qt::ItemFlags svtkQtTableModelAdapter::flags(const QModelIndex& idx) const
{
  if (!idx.isValid())
  {
    return Qt::ItemIsEnabled | Qt::ItemIsDropEnabled;
  }

  return Qt::ItemIsEnabled | Qt::ItemIsSelectable | Qt::ItemIsDropEnabled | Qt::ItemIsDragEnabled;
}

//----------------------------------------------------------------------------
QVariant svtkQtTableModelAdapter::headerData(
  int section, Qt::Orientation orientation, int role) const
{
  if (this->noTableCheck())
  {
    return QVariant();
  }

  // For horizontal headers, try to convert the column names to double.
  // If it doesn't work, return a string.
  if (orientation == Qt::Horizontal && (role == Qt::DisplayRole || role == Qt::UserRole))
  {

    QString columnName;
    if (this->GetSplitMultiComponentColumns())
    {
      columnName = this->Internal->ModelColumnNames[section];
    }
    else
    {
      int column = this->ModelColumnToFieldDataColumn(section);
      columnName = this->Table->GetColumnName(column);
    }

    QVariant svar(columnName);
    bool ok;
    double value = svar.toDouble(&ok);
    if (ok)
    {
      return QVariant(value);
    }
    return svar;
  }

  // For vertical headers, return values in the first column if
  // KeyColumn is valid.
  if (orientation == Qt::Vertical)
  {
    if (role == Qt::DisplayRole || role == Qt::UserRole)
    {
      if (this->KeyColumn >= 0)
      {
        svtkVariant v;
        this->getValue(section, this->KeyColumn, v);
        if (v.IsNumeric())
        {
          return QVariant(v.ToDouble());
        }
        return QVariant(v.ToString().c_str());
      }
    }
    else if (role == Qt::DecorationRole &&
      this->DecorationStrategy == svtkQtTableModelAdapter::ICONS &&
      this->DecorationLocation == svtkQtTableModelAdapter::ITEM && this->IconIndexColumn >= 0)
    {
      return this->getIcon(section);
    }
  }

  return QVariant();
}

//----------------------------------------------------------------------------
void svtkQtTableModelAdapter::getValue(int row, int in_column, svtkVariant& v) const
{
  // Map the qt model column to a column in the svtk table
  int column;
  if (this->GetSplitMultiComponentColumns())
  {
    column = this->Internal->ModelColumnToTableColumn[in_column].first;
  }
  else
  {
    column = this->ModelColumnToFieldDataColumn(in_column);
  }

  // Get the value from the table as a svtkVariant
  // We don't use svtkTable::GetValue() since for multi-component arrays, it can
  // be slow due to the use of svtkDataArray in the svtkVariant.
  svtkAbstractArray* array = this->Table->GetColumn(column);
  if (!array)
  {
    return;
  }

  const int nComponents = array->GetNumberOfComponents();

  // Special case- if the variant is an array it means the column data
  // has multiple components
  if (nComponents == 1)
  {
    v = array->GetVariantValue(row);
  }
  else if (nComponents > 1)
  {
    if (this->GetSplitMultiComponentColumns())
    {
      // Map the qt model column to the corresponding component in the svtkTable column
      const int component = this->Internal->ModelColumnToTableColumn[in_column].second;
      if (component < nComponents)
      {
        // If component is in range, then fetch the component's value
        v = array->GetVariantValue(nComponents * row + component);
      }
      else
      {
        // If component is out of range this signals that we should return the
        // value from the magnitude column
        v = this->Internal->MagnitudeColumns[column]->GetValue(row);
      }
    }
    else // don't split columns.
    {
      QString strValue;
      for (int i = 0; i < nComponents; ++i)
      {
        strValue.append(
          QString("%1, ").arg(array->GetVariantValue(row * nComponents + i).ToString().c_str()));
      }
      strValue = strValue.remove(strValue.size() - 2, 2); // remove the last comma

      // Reconstruct the variant using this string value
      v = svtkVariant(strValue.toLatin1().data());
    }
  }
}

//----------------------------------------------------------------------------
QModelIndex svtkQtTableModelAdapter::index(
  int row, int column, const QModelIndex& svtkNotUsed(parentIdx)) const
{
  return createIndex(row, column, row);
}

//----------------------------------------------------------------------------
QModelIndex svtkQtTableModelAdapter::parent(const QModelIndex& svtkNotUsed(idx)) const
{
  return QModelIndex();
}

//----------------------------------------------------------------------------
int svtkQtTableModelAdapter::rowCount(const QModelIndex& mIndex) const
{
  if (this->noTableCheck())
  {
    return 0;
  }
  if (mIndex == QModelIndex())
  {
    return this->Table->GetNumberOfRows();
  }
  return 0;
}

//----------------------------------------------------------------------------
int svtkQtTableModelAdapter::columnCount(const QModelIndex&) const
{
  if (this->noTableCheck())
  {
    return 0;
  }

  // If we are splitting multi-component columns, then just return the
  // number of entries in the QHash map that stores column names.
  if (this->GetSplitMultiComponentColumns())
  {
    return this->Internal->ModelColumnNames.size();
  }

  // The number of columns in the qt model depends on the current ViewType
  switch (this->ViewType)
  {
    case FULL_VIEW:
      return this->Table->GetNumberOfColumns();
    case DATA_VIEW:
      return this->DataEndColumn - this->DataStartColumn + 1;
    default:
      svtkGenericWarningMacro("svtkQtTreeModelAdapter: Bad view type.");
  };
  return 0;
}

bool svtkQtTableModelAdapter::dropMimeData(const QMimeData* d, Qt::DropAction action,
  int svtkNotUsed(row), int svtkNotUsed(column), const QModelIndex& svtkNotUsed(parent))
{
  if (action == Qt::IgnoreAction)
    return true;

  if (!d->hasFormat("svtk/selection"))
    return false;

  void* temp = nullptr;
  std::istringstream buffer(d->data("svtk/selection").data());
  buffer >> temp;
  svtkSelection* s = reinterpret_cast<svtkSelection*>(temp);

  emit this->selectionDropped(s);

  return true;
}

QStringList svtkQtTableModelAdapter::mimeTypes() const
{
  QStringList types;
  types << "svtk/selection";
  return types;
}

Qt::DropActions svtkQtTableModelAdapter::supportedDropActions() const
{
  return Qt::CopyAction;
}

QMimeData* svtkQtTableModelAdapter::mimeData(const QModelIndexList& indexes) const
{
  // Only supports dragging single item right now ...

  if (indexes.size() == 0)
  {
    return nullptr;
  }

  svtkSmartPointer<svtkSelection> indexSelection =
    svtkSmartPointer<svtkSelection>::Take(QModelIndexListToSVTKIndexSelection(indexes));
  // svtkSmartPointer<svtkSelection> pedigreeIdSelection =
  // svtkSmartPointer<svtkSelection>::Take(svtkConvertSelection::ToSelectionType(indexSelection,
  // this->Table, svtkSelectionNode::PEDIGREEIDS));

  // This is a memory-leak, we need to serialize its contents as a string, instead of serializing a
  // pointer to the object
  svtkSelection* pedigreeIdSelection = svtkConvertSelection::ToSelectionType(
    indexSelection, this->Table, svtkSelectionNode::PEDIGREEIDS);

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

QVariant svtkQtTableModelAdapter::getColorIcon(int row) const
{
  int column;
  if (this->GetSplitMultiComponentColumns())
  {
    column = this->Internal->ModelColumnToTableColumn[this->ColorColumn].first;
  }
  else
  {
    column = this->ModelColumnToFieldDataColumn(this->ColorColumn);
  }
  svtkUnsignedCharArray* colors =
    svtkArrayDownCast<svtkUnsignedCharArray>(this->Table->GetColumn(column));
  if (!colors)
  {
    return QVariant();
  }

  const int nComponents = colors->GetNumberOfComponents();
  if (nComponents >= 3)
  {
    unsigned char rgba[4];
    colors->GetTypedTuple(row, rgba);
    int rgb[3];
    rgb[0] = static_cast<int>(0x0ff & rgba[0]);
    rgb[1] = static_cast<int>(0x0ff & rgba[1]);
    rgb[2] = static_cast<int>(0x0ff & rgba[2]);

    QPixmap pixmap(16, 16);
    pixmap.fill(QColor(0, 0, 0, 0));
    QPainter painter(&pixmap);
    painter.setRenderHint(QPainter::Antialiasing);
    painter.setPen(Qt::NoPen);
    painter.setBrush(QBrush(QColor(rgb[0], rgb[1], rgb[2])));
    painter.drawEllipse(4, 4, 7, 7);
    return QVariant(pixmap);
  }

  return QVariant();
}

QVariant svtkQtTableModelAdapter::getIcon(int row) const
{
  int column;
  if (this->GetSplitMultiComponentColumns())
  {
    column = this->Internal->ModelColumnToTableColumn[this->IconIndexColumn].first;
  }
  else
  {
    column = this->ModelColumnToFieldDataColumn(this->IconIndexColumn);
  }
  svtkIntArray* icon_indices = svtkArrayDownCast<svtkIntArray>(this->Table->GetColumn(column));
  if (!icon_indices)
  {
    return QVariant();
  }

  int icon_idx = icon_indices->GetValue(row);
  int x, y;
  int dimX = this->IconSheetSize[0] / this->IconSize[0];
  x = (icon_idx >= dimX) ? icon_idx % dimX : icon_idx;
  x *= this->IconSize[0];
  y = (icon_idx >= dimX) ? static_cast<int>(icon_idx / dimX) : 0;
  y *= this->IconSize[1];

  return this->IconSheet.copy(x, y, this->IconSize[0], this->IconSize[1]);
}

void svtkQtTableModelAdapter::SetIconSheet(QImage sheet)
{
  this->IconSheet = sheet;
}
void svtkQtTableModelAdapter::SetIconSheetSize(int w, int h)
{
  this->IconSheetSize[0] = w;
  this->IconSheetSize[1] = h;
}
void svtkQtTableModelAdapter::SetIconSize(int w, int h)
{
  this->IconSize[0] = w;
  this->IconSize[1] = h;
}
