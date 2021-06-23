/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeHeatmapItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTreeHeatmapItem.h"
#include "svtkDendrogramItem.h"
#include "svtkHeatmapItem.h"

#include "svtkBitArray.h"
#include "svtkDataSetAttributes.h"
#include "svtkObjectFactory.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTree.h"

#include <algorithm>

svtkStandardNewMacro(svtkTreeHeatmapItem);

//-----------------------------------------------------------------------------
svtkTreeHeatmapItem::svtkTreeHeatmapItem()
{
  this->Interactive = true;
  this->Orientation = svtkDendrogramItem::LEFT_TO_RIGHT;
  this->TreeHeatmapBuildTime = 0;

  this->Dendrogram = svtkSmartPointer<svtkDendrogramItem>::New();
  this->Dendrogram->ExtendLeafNodesOn();
  this->Dendrogram->SetVisible(false);
  this->AddItem(this->Dendrogram);

  this->ColumnDendrogram = svtkSmartPointer<svtkDendrogramItem>::New();
  this->ColumnDendrogram->ExtendLeafNodesOn();
  this->ColumnDendrogram->SetVisible(false);
  this->ColumnDendrogram->SetDrawLabels(false);
  this->AddItem(this->ColumnDendrogram);

  this->Heatmap = svtkSmartPointer<svtkHeatmapItem>::New();
  this->Heatmap->SetVisible(false);
  this->AddItem(this->Heatmap);

  this->ColumnDendrogram->SetLeafSpacing(this->Heatmap->GetCellWidth());
}

//-----------------------------------------------------------------------------
svtkTreeHeatmapItem::~svtkTreeHeatmapItem() = default;

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::SetTree(svtkTree* tree)
{
  this->Dendrogram->SetTree(tree);
  if (tree == nullptr)
  {
    return;
  }

  if (this->GetTable() != nullptr && this->GetTable()->GetNumberOfRows() != 0)
  {
    this->Dendrogram->SetDrawLabels(false);
  }
  this->Dendrogram->SetVisible(true);

  // rearrange our table to match the order of the leaf nodes in this tree.
  if (this->GetTable() != nullptr && this->GetTable()->GetNumberOfRows() != 0)
  {
    this->ReorderTable();
  }
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::SetTable(svtkTable* table)
{
  this->Heatmap->SetTable(table);
  if (table == nullptr)
  {
    return;
  }

  if (this->Dendrogram->GetTree() != nullptr &&
    this->Dendrogram->GetTree()->GetNumberOfVertices() != 0)
  {
    this->Dendrogram->SetDrawLabels(false);
  }
  this->Heatmap->SetVisible(true);

  // rearrange our table to match the order of the leaf nodes in this tree.
  if (this->GetTree() != nullptr && this->GetTree()->GetNumberOfVertices() != 0)
  {
    this->ReorderTable();
  }

  // add an array to this table's field data to keep track of collapsed rows
  // (unless it already has the array)
  svtkBitArray* existingRowsArray =
    svtkArrayDownCast<svtkBitArray>(this->GetTable()->GetFieldData()->GetArray("collapsed rows"));
  if (existingRowsArray)
  {
    for (svtkIdType row = 0; row < this->GetTable()->GetNumberOfRows(); ++row)
    {
      existingRowsArray->SetValue(row, 0);
    }
  }
  else
  {
    svtkSmartPointer<svtkBitArray> collapsedRowsArray = svtkSmartPointer<svtkBitArray>::New();
    collapsedRowsArray->SetNumberOfComponents(1);
    collapsedRowsArray->SetName("collapsed rows");
    for (svtkIdType row = 0; row < this->GetTable()->GetNumberOfRows(); ++row)
    {
      collapsedRowsArray->InsertNextValue(0);
    }
    this->GetTable()->GetFieldData()->AddArray(collapsedRowsArray);
  }

  // add an array to this table's field data to keep track of collapsed columns
  // (unless it already has the array)
  svtkBitArray* existingColumnsArray =
    svtkArrayDownCast<svtkBitArray>(this->GetTable()->GetFieldData()->GetArray("collapsed columns"));
  if (existingColumnsArray)
  {
    for (svtkIdType col = 0; col < this->GetTable()->GetNumberOfColumns(); ++col)
    {
      existingColumnsArray->SetValue(col, 0);
    }
  }
  else
  {
    svtkSmartPointer<svtkBitArray> collapsedColumnsArray = svtkSmartPointer<svtkBitArray>::New();
    collapsedColumnsArray->SetNumberOfComponents(1);
    collapsedColumnsArray->SetName("collapsed columns");
    for (svtkIdType col = 0; col < this->GetTable()->GetNumberOfColumns(); ++col)
    {
      collapsedColumnsArray->InsertNextValue(0);
    }
    this->GetTable()->GetFieldData()->AddArray(collapsedColumnsArray);
  }
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::SetColumnTree(svtkTree* tree)
{
  this->ColumnDendrogram->SetTree(tree);
  if (tree == nullptr)
  {
    return;
  }

  if (this->Orientation == svtkDendrogramItem::LEFT_TO_RIGHT ||
    this->Orientation == svtkDendrogramItem::RIGHT_TO_LEFT)
  {
    this->ColumnDendrogram->SetOrientation(svtkDendrogramItem::UP_TO_DOWN);
  }
  else
  {
    this->ColumnDendrogram->SetOrientation(svtkDendrogramItem::RIGHT_TO_LEFT);
  }

  this->ColumnDendrogram->SetVisible(true);
}

//-----------------------------------------------------------------------------
svtkTree* svtkTreeHeatmapItem::GetColumnTree()
{
  return this->ColumnDendrogram->GetTree();
}

//-----------------------------------------------------------------------------
svtkDendrogramItem* svtkTreeHeatmapItem::GetDendrogram()
{
  return this->Dendrogram;
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::SetDendrogram(svtkDendrogramItem* dendrogram)
{
  this->Dendrogram = dendrogram;
}

//-----------------------------------------------------------------------------
svtkHeatmapItem* svtkTreeHeatmapItem::GetHeatmap()
{
  return this->Heatmap;
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::SetHeatmap(svtkHeatmapItem* heatmap)
{
  this->Heatmap = heatmap;
}

//-----------------------------------------------------------------------------
svtkTree* svtkTreeHeatmapItem::GetTree()
{
  return this->Dendrogram->GetTree();
}

//-----------------------------------------------------------------------------
svtkTable* svtkTreeHeatmapItem::GetTable()
{
  return this->Heatmap->GetTable();
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::ReorderTable()
{
  // make a copy of our table.
  svtkNew<svtkTable> tableCopy;
  tableCopy->DeepCopy(this->GetTable());

  // grab a separate copy of the row names column.
  svtkNew<svtkStringArray> rowNames;
  rowNames->DeepCopy(this->Heatmap->GetRowNames());

  // we also need to know which number column it is
  svtkIdType rowNamesColNum = 0;
  for (svtkIdType col = 0; col < this->GetTable()->GetNumberOfColumns(); ++col)
  {
    if (this->GetTable()->GetColumn(col) == this->Heatmap->GetRowNames())
    {
      rowNamesColNum = col;
      break;
    }
  }

  // empty out our original table.
  for (svtkIdType row = this->GetTable()->GetNumberOfRows() - 1; row > -1; --row)
  {
    this->GetTable()->RemoveRow(row);
  }

  // get the names of the vertices in our tree.
  svtkStringArray* vertexNames = svtkArrayDownCast<svtkStringArray>(
    this->GetTree()->GetVertexData()->GetAbstractArray("node name"));

  for (svtkIdType vertex = 0; vertex < this->GetTree()->GetNumberOfVertices(); ++vertex)
  {
    if (!this->GetTree()->IsLeaf(vertex))
    {
      continue;
    }

    // find the row in the table that corresponds to this vertex
    std::string vertexName = vertexNames->GetValue(vertex);
    svtkIdType tableRow = rowNames->LookupValue(vertexName);
    if (tableRow < 0)
    {
      svtkIdType newRowNum = this->GetTable()->InsertNextBlankRow();
      this->GetTable()->SetValue(newRowNum, rowNamesColNum, svtkVariant(vertexName));
      this->Heatmap->MarkRowAsBlank(vertexName);
      continue;
    }

    // copy it back into our original table
    this->GetTable()->InsertNextRow(tableCopy->GetRow(tableRow));
  }

  if (this->Orientation == svtkDendrogramItem::DOWN_TO_UP ||
    this->Orientation == svtkDendrogramItem::UP_TO_DOWN)
  {
    this->ReverseTableColumns();
  }
  if (this->Orientation == svtkDendrogramItem::RIGHT_TO_LEFT ||
    this->Orientation == svtkDendrogramItem::DOWN_TO_UP)
  {
    this->ReverseTableRows();
  }
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::ReverseTableRows()
{
  // make a copy of our table and then empty out the original.
  svtkNew<svtkTable> tableCopy;
  tableCopy->DeepCopy(this->GetTable());
  for (svtkIdType row = 0; row < tableCopy->GetNumberOfRows(); ++row)
  {
    this->GetTable()->RemoveRow(row);
  }

  // re-insert the rows back into our original table in reverse order
  for (svtkIdType tableRow = tableCopy->GetNumberOfRows() - 1; tableRow >= 0; --tableRow)
  {
    this->GetTable()->InsertNextRow(tableCopy->GetRow(tableRow));
  }
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::ReverseTableColumns()
{
  // make a copy of our table and then empty out the original.
  svtkNew<svtkTable> tableCopy;
  tableCopy->DeepCopy(this->GetTable());
  for (svtkIdType col = tableCopy->GetNumberOfColumns() - 1; col > 0; --col)
  {
    this->GetTable()->RemoveColumn(col);
  }

  // re-insert the columns back into our original table in reverse order
  for (svtkIdType col = tableCopy->GetNumberOfColumns() - 1; col >= 1; --col)
  {
    this->GetTable()->AddColumn(tableCopy->GetColumn(col));
  }
}

//-----------------------------------------------------------------------------
bool svtkTreeHeatmapItem::Paint(svtkContext2D* painter)
{
  this->Dendrogram->Paint(painter);

  double treeBounds[4];
  this->Dendrogram->GetBounds(treeBounds);
  double spacing = this->Dendrogram->GetLeafSpacing() / 2.0;

  double heatmapStartX, heatmapStartY;

  switch (this->Orientation)
  {
    case svtkDendrogramItem::UP_TO_DOWN:
      heatmapStartX = treeBounds[0] - spacing;
      heatmapStartY = treeBounds[2] -
        (this->GetTable()->GetNumberOfColumns() - 1) * this->Heatmap->GetCellWidth() - spacing;
      break;
    case svtkDendrogramItem::DOWN_TO_UP:
      heatmapStartX = treeBounds[0] - spacing;
      heatmapStartY = treeBounds[3] + spacing;
      break;
    case svtkDendrogramItem::RIGHT_TO_LEFT:
      heatmapStartX = treeBounds[0] -
        (this->GetTable()->GetNumberOfColumns() - 1) * this->Heatmap->GetCellWidth() - spacing;
      heatmapStartY = treeBounds[2] - spacing;
      break;
    case svtkDendrogramItem::LEFT_TO_RIGHT:
    default:
      heatmapStartX = treeBounds[1] + spacing;
      heatmapStartY = treeBounds[2] - spacing;
      break;
  }
  this->Heatmap->SetPosition(heatmapStartX, heatmapStartY);
  this->Heatmap->Paint(painter);

  if (this->ColumnDendrogram->GetVisible())
  {
    double columnTreeStartX, columnTreeStartY;

    double heatmapBounds[4];
    this->Heatmap->GetBounds(heatmapBounds);

    this->ColumnDendrogram->PrepareToPaint(painter);
    this->ColumnDendrogram->GetBounds(treeBounds);

    float offset = 0.0;
    if (this->Heatmap->GetRowLabelWidth() > 0.0)
    {
      offset = this->Heatmap->GetRowLabelWidth() + spacing;
    }
    switch (this->Orientation)
    {
      case svtkDendrogramItem::UP_TO_DOWN:
        columnTreeStartX = heatmapBounds[1] + (treeBounds[1] - treeBounds[0]) + spacing;
        columnTreeStartY = heatmapBounds[3] - this->ColumnDendrogram->GetLeafSpacing() / 2.0;
        break;
      case svtkDendrogramItem::DOWN_TO_UP:
        columnTreeStartX = heatmapBounds[1] + (treeBounds[1] - treeBounds[0]) + spacing;
        columnTreeStartY =
          heatmapBounds[3] - offset - this->ColumnDendrogram->GetLeafSpacing() / 2.0;
        break;
      case svtkDendrogramItem::RIGHT_TO_LEFT:
        columnTreeStartX =
          heatmapBounds[0] + offset + this->ColumnDendrogram->GetLeafSpacing() / 2.0;
        columnTreeStartY = heatmapBounds[3] + spacing + (treeBounds[3] - treeBounds[2]);
        break;
      case svtkDendrogramItem::LEFT_TO_RIGHT:
      default:
        columnTreeStartX = heatmapBounds[0] + this->ColumnDendrogram->GetLeafSpacing() / 2.0;
        columnTreeStartY = heatmapBounds[3] + spacing + (treeBounds[3] - treeBounds[2]);
        break;
    }

    this->ColumnDendrogram->SetPosition(columnTreeStartX, columnTreeStartY);
    this->ColumnDendrogram->Paint(painter);
  }

  return true;
}

//-----------------------------------------------------------------------------
bool svtkTreeHeatmapItem::MouseDoubleClickEvent(const svtkContextMouseEvent& event)
{
  bool treeChanged = this->Dendrogram->MouseDoubleClickEvent(event);

  // update the heatmap if a subtree just collapsed or expanded.
  if (treeChanged)
  {
    this->CollapseHeatmapRows();
  }
  else
  {
    treeChanged = this->ColumnDendrogram->MouseDoubleClickEvent(event);
    if (treeChanged)
    {
      this->CollapseHeatmapColumns();
    }
  }
  return treeChanged;
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::CollapseHeatmapRows()
{
  svtkBitArray* collapsedRowsArray =
    svtkArrayDownCast<svtkBitArray>(this->GetTable()->GetFieldData()->GetArray("collapsed rows"));

  svtkStringArray* vertexNames = svtkArrayDownCast<svtkStringArray>(
    this->Dendrogram->GetPrunedTree()->GetVertexData()->GetAbstractArray("node name"));

  svtkStringArray* rowNames = this->Heatmap->GetRowNames();
  if (!rowNames)
  {
    return;
  }

  for (svtkIdType row = 0; row < this->GetTable()->GetNumberOfRows(); ++row)
  {
    std::string name = rowNames->GetValue(row);
    // if we can't find this name in the layout tree, then the corresponding
    // row in the heatmap should be marked as collapsed.
    if (vertexNames->LookupValue(name) == -1)
    {
      collapsedRowsArray->SetValue(row, 1);
    }
    else
    {
      collapsedRowsArray->SetValue(row, 0);
    }
  }
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::CollapseHeatmapColumns()
{
  svtkBitArray* collapsedColumnsArray =
    svtkArrayDownCast<svtkBitArray>(this->GetTable()->GetFieldData()->GetArray("collapsed columns"));

  svtkStringArray* vertexNames = svtkArrayDownCast<svtkStringArray>(
    this->ColumnDendrogram->GetPrunedTree()->GetVertexData()->GetAbstractArray("node name"));

  for (svtkIdType col = 1; col < this->GetTable()->GetNumberOfColumns(); ++col)
  {
    std::string name = this->GetTable()->GetColumn(col)->GetName();

    // if we can't find this name in the layout tree, then the corresponding
    // column in the heatmap should be marked as collapsed.
    if (vertexNames->LookupValue(name) == -1)
    {
      collapsedColumnsArray->SetValue(col, 1);
    }
    else
    {
      collapsedColumnsArray->SetValue(col, 0);
    }
  }
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::SetOrientation(int orientation)
{
  int previousOrientation = this->Orientation;
  this->Orientation = orientation;
  this->Dendrogram->SetOrientation(this->Orientation);
  this->Heatmap->SetOrientation(this->Orientation);

  if (this->Orientation == svtkDendrogramItem::LEFT_TO_RIGHT ||
    this->Orientation == svtkDendrogramItem::RIGHT_TO_LEFT)
  {
    this->ColumnDendrogram->SetOrientation(svtkDendrogramItem::UP_TO_DOWN);
  }
  else
  {
    this->ColumnDendrogram->SetOrientation(svtkDendrogramItem::RIGHT_TO_LEFT);
  }

  // reverse our table if we're changing from a "not backwards" orientation
  // to one that it backwards.
  if ((this->Orientation == svtkDendrogramItem::UP_TO_DOWN ||
        this->Orientation == svtkDendrogramItem::DOWN_TO_UP) &&
    (previousOrientation != svtkDendrogramItem::UP_TO_DOWN &&
      previousOrientation != svtkDendrogramItem::DOWN_TO_UP))
  {
    this->ReverseTableColumns();
  }
  if ((this->Orientation == svtkDendrogramItem::RIGHT_TO_LEFT ||
        this->Orientation == svtkDendrogramItem::DOWN_TO_UP) &&
    (previousOrientation != svtkDendrogramItem::RIGHT_TO_LEFT &&
      previousOrientation != svtkDendrogramItem::DOWN_TO_UP))
  {
    this->ReverseTableRows();
  }
}

//-----------------------------------------------------------------------------
int svtkTreeHeatmapItem::GetOrientation()
{
  return this->Orientation;
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::GetBounds(double bounds[4])
{
  double treeBounds[4] = { SVTK_DOUBLE_MAX, SVTK_DOUBLE_MIN, SVTK_DOUBLE_MAX, SVTK_DOUBLE_MIN };
  if (this->GetTree()->GetNumberOfVertices() > 0)
  {
    this->Dendrogram->GetBounds(treeBounds);
  }

  double tableBounds[4] = { SVTK_DOUBLE_MAX, SVTK_DOUBLE_MIN, SVTK_DOUBLE_MAX, SVTK_DOUBLE_MIN };
  if (this->GetTable()->GetNumberOfRows() > 0)
  {
    this->Heatmap->GetBounds(tableBounds);
  }

  double columnTreeBounds[4] = { SVTK_DOUBLE_MAX, SVTK_DOUBLE_MIN, SVTK_DOUBLE_MAX, SVTK_DOUBLE_MIN };
  if (this->ColumnDendrogram->GetTree() != nullptr)
  {
    this->ColumnDendrogram->GetBounds(columnTreeBounds);
  }

  double xMin, xMax, yMin, yMax;

  xMin = std::min(std::min(treeBounds[0], tableBounds[0]), columnTreeBounds[0]);
  xMax = std::max(std::max(treeBounds[1], tableBounds[1]), columnTreeBounds[1]);
  yMin = std::min(std::min(treeBounds[2], tableBounds[2]), columnTreeBounds[2]);
  yMax = std::max(std::max(treeBounds[3], tableBounds[3]), columnTreeBounds[3]);

  bounds[0] = xMin;
  bounds[1] = xMax;
  bounds[2] = yMin;
  bounds[3] = yMax;
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::GetCenter(double center[2])
{
  double bounds[4];
  this->GetBounds(bounds);

  center[0] = bounds[0] + (bounds[1] - bounds[0]) / 2.0;
  center[1] = bounds[2] + (bounds[3] - bounds[2]) / 2.0;
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::GetSize(double size[2])
{
  double bounds[4];
  this->GetBounds(bounds);

  size[0] = fabs(bounds[1] - bounds[0]);
  size[1] = fabs(bounds[3] - bounds[2]);
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::SetTreeColorArray(const char* arrayName)
{
  this->Dendrogram->SetColorArray(arrayName);
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::CollapseToNumberOfLeafNodes(unsigned int n)
{
  this->Dendrogram->CollapseToNumberOfLeafNodes(n);
  this->CollapseHeatmapRows();
}

//-----------------------------------------------------------------------------
float svtkTreeHeatmapItem::GetTreeLineWidth()
{
  return this->Dendrogram->GetLineWidth();
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::SetTreeLineWidth(float width)
{
  this->Dendrogram->SetLineWidth(width);
  this->ColumnDendrogram->SetLineWidth(width);
}

//-----------------------------------------------------------------------------
svtkTree* svtkTreeHeatmapItem::GetPrunedTree()
{
  return this->Dendrogram->GetPrunedTree();
}

//-----------------------------------------------------------------------------
bool svtkTreeHeatmapItem::Hit(const svtkContextMouseEvent& svtkNotUsed(mouse))
{
  // If we are interactive, we want to catch anything that propagates to the
  // background, otherwise we do not want any mouse events.
  return this->Interactive;
}

//-----------------------------------------------------------------------------
void svtkTreeHeatmapItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  this->Dendrogram->PrintSelf(os, indent);
  this->Heatmap->PrintSelf(os, indent);
}
