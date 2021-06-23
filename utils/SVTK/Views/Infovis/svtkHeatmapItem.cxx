/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHeatmapItem.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkHeatmapItem.h"

#include "svtkBitArray.h"
#include "svtkBrush.h"
#include "svtkCategoryLegend.h"
#include "svtkColorLegend.h"
#include "svtkColorSeries.h"
#include "svtkContext2D.h"
#include "svtkContextMouseEvent.h"
#include "svtkContextScene.h"
#include "svtkFieldData.h"
#include "svtkIntArray.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkRect.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTextProperty.h"
#include "svtkTooltipItem.h"
#include "svtkTransform2D.h"
#include "svtkVariantArray.h"

#include <sstream>

svtkStandardNewMacro(svtkHeatmapItem);

//-----------------------------------------------------------------------------
svtkHeatmapItem::svtkHeatmapItem()
  : PositionVector(0, 0)
{
  this->Position = this->PositionVector.GetData();
  this->Interactive = true;
  this->HeatmapBuildTime = 0;
  this->Table = svtkSmartPointer<svtkTable>::New();
  this->NameColumn = "name";
  this->RowNames = nullptr;

  this->CollapsedRowsArray = nullptr;
  this->CollapsedColumnsArray = nullptr;

  /* initialize bounds so that the mouse cursor is never considered
   * "inside" the heatmap */
  this->MinX = 1.0;
  this->MinY = 1.0;
  this->MaxX = 0.0;
  this->MaxY = 0.0;

  this->RowLabelWidth = 0.0;
  this->ColumnLabelWidth = 0.0;

  this->CellHeight = 18.0;
  this->CellWidth = this->CellHeight * 2.0;

  this->CategoryLegend->SetVisible(false);
  this->CategoryLegend->CacheBoundsOff();
  this->AddItem(this->CategoryLegend);

  this->ColorLegend->SetVisible(false);
  this->ColorLegend->DrawBorderOn();
  this->ColorLegend->CacheBoundsOff();
  this->AddItem(this->ColorLegend);

  this->LegendPositionSet = false;

  this->Tooltip->SetVisible(false);
  this->AddItem(this->Tooltip);
}

//-----------------------------------------------------------------------------
svtkHeatmapItem::~svtkHeatmapItem() = default;

//-----------------------------------------------------------------------------
void svtkHeatmapItem::SetPosition(const svtkVector2f& pos)
{
  this->PositionVector = pos;
}

//-----------------------------------------------------------------------------
svtkVector2f svtkHeatmapItem::GetPositionVector()
{
  return this->PositionVector;
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::SetTable(svtkTable* table)
{
  if (table == nullptr || table->GetNumberOfRows() == 0)
  {
    this->Table = svtkSmartPointer<svtkTable>::New();
    return;
  }
  this->Table = table;

  // get the row names for this table
  svtkStringArray* rowNames =
    svtkArrayDownCast<svtkStringArray>(this->Table->GetColumnByName(this->NameColumn));
  if (rowNames == nullptr)
  {
    rowNames = svtkArrayDownCast<svtkStringArray>(this->Table->GetColumn(0));
  }
  if (rowNames == nullptr)
  {
    svtkWarningMacro("Could not determine row name column."
                    "Try calling svtkHeatmapItem::SetNameColumn(svtkStdString)");
    this->RowNames = nullptr;
  }
  else
  {
    this->RowNames = rowNames;
  }
}

//-----------------------------------------------------------------------------
svtkTable* svtkHeatmapItem::GetTable()
{
  return this->Table;
}

//-----------------------------------------------------------------------------
svtkStringArray* svtkHeatmapItem::GetRowNames()
{
  return this->RowNames;
}

//-----------------------------------------------------------------------------
bool svtkHeatmapItem::Paint(svtkContext2D* painter)
{
  if (this->Table->GetNumberOfRows() == 0)
  {
    return true;
  }

  if (this->IsDirty())
  {
    this->RebuildBuffers();
  }

  this->PaintBuffers(painter);
  this->PaintChildren(painter);
  return true;
}

//-----------------------------------------------------------------------------
bool svtkHeatmapItem::IsDirty()
{
  if (this->Table->GetNumberOfRows() == 0)
  {
    return false;
  }
  if (this->Table->GetMTime() > this->HeatmapBuildTime)
  {
    return true;
  }
  return false;
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::RebuildBuffers()
{
  if (this->Table->GetNumberOfRows() == 0)
  {
    return;
  }

  this->InitializeLookupTables();

  this->CollapsedRowsArray =
    svtkArrayDownCast<svtkBitArray>(this->Table->GetFieldData()->GetArray("collapsed rows"));
  this->CollapsedColumnsArray =
    svtkArrayDownCast<svtkBitArray>(this->Table->GetFieldData()->GetArray("collapsed columns"));

  this->HeatmapBuildTime = this->Table->GetMTime();
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::InitializeLookupTables()
{
  this->ColumnRanges.clear();
  this->CategoricalDataValues->Reset();

  for (svtkIdType column = 0; column < this->Table->GetNumberOfColumns(); ++column)
  {
    if (this->Table->GetColumn(column) == this->GetRowNames())
    {
      continue;
    }
    if (this->Table->GetValue(0, column).IsString())
    {
      this->AccumulateProminentCategoricalDataValues(column);
      continue;
    }
    double min = SVTK_DOUBLE_MAX;
    double max = SVTK_DOUBLE_MIN;
    for (svtkIdType row = 0; row < this->Table->GetNumberOfRows(); ++row)
    {
      double value = this->Table->GetValue(row, column).ToDouble();
      if (value > max)
      {
        max = value;
      }
      if (value < min)
      {
        min = value;
      }
    }
    this->ColumnRanges[column] = std::pair<double, double>(min, max);
  }

  this->GenerateCategoricalDataLookupTable();
  this->GenerateContinuousDataLookupTable();
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::GenerateContinuousDataLookupTable()
{
  this->ContinuousDataLookupTable->SetNumberOfTableValues(255);
  this->ContinuousDataLookupTable->Build();
  this->ContinuousDataLookupTable->SetRange(0, 255);
  this->ContinuousDataLookupTable->SetNanColor(0.75, 0.75, 0.75, 1.0);

  // black to red
  for (int i = 0; i < 85; ++i)
  {
    float f = static_cast<float>(i) / 84.0;
    this->ContinuousDataLookupTable->SetTableValue(i, f, 0, 0);
  }

  // red to yellow
  for (int i = 0; i < 85; ++i)
  {
    float f = static_cast<float>(i) / 84.0;
    this->ContinuousDataLookupTable->SetTableValue(85 + i, 1.0, f, 0);
  }

  // yellow to white
  for (int i = 0; i < 85; ++i)
  {
    float f = static_cast<float>(i) / 84.0;
    this->ContinuousDataLookupTable->SetTableValue(170 + i, 1.0, 1.0, f);
  }

  this->ColorLegendLookupTable->DeepCopy(this->ContinuousDataLookupTable);
  this->ColorLegend->SetTransferFunction(this->ColorLegendLookupTable);
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::AccumulateProminentCategoricalDataValues(svtkIdType column)
{
  svtkStringArray* stringColumn = svtkArrayDownCast<svtkStringArray>(this->Table->GetColumn(column));

  // search for values that occur more than once
  svtkNew<svtkStringArray> repeatedValues;
  std::map<std::string, int> CountMap;
  std::map<std::string, int>::iterator mapItr;
  for (int i = 0; i < stringColumn->GetNumberOfTuples(); ++i)
  {
    CountMap[stringColumn->GetValue(i)]++;
  }

  for (mapItr = CountMap.begin(); mapItr != CountMap.end(); ++mapItr)
  {
    if (mapItr->second > 1)
    {
      repeatedValues->InsertNextValue(mapItr->first);
    }
  }

  // add each distinct, repeated value from this column to our master list
  for (int i = 0; i < repeatedValues->GetNumberOfTuples(); ++i)
  {
    svtkVariant v = repeatedValues->GetVariantValue(i);
    if (this->CategoricalDataValues->LookupValue(v) == -1)
    {
      this->CategoricalDataValues->InsertNextValue(v.ToString());
    }
  }
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::GenerateCategoricalDataLookupTable()
{
  this->CategoricalDataLookupTable->ResetAnnotations();
  this->CategoricalDataLookupTable->SetNanColor(0.75, 0.75, 0.75, 1.0);

  // make each distinct categorical value an index into our lookup table
  for (int i = 0; i < this->CategoricalDataValues->GetNumberOfTuples(); ++i)
  {
    this->CategoricalDataLookupTable->SetAnnotation(
      this->CategoricalDataValues->GetValue(i), this->CategoricalDataValues->GetValue(i));
  }

  svtkNew<svtkColorSeries> colorSeries;
  colorSeries->SetColorScheme(svtkColorSeries::BREWER_QUALITATIVE_SET3);
  colorSeries->BuildLookupTable(this->CategoricalDataLookupTable);

  this->CategoryLegend->SetScalarsToColors(this->CategoricalDataLookupTable);
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::PaintBuffers(svtkContext2D* painter)
{
  // Calculate the extent of the data that is visible within the window.
  this->UpdateVisibleSceneExtent(painter);

  // Compute the bounds of the heatmap (excluding text labels)
  this->ComputeBounds();

  // leave a small amount of space between the heatmap and the row/column
  // labels
  double spacing = this->CellWidth * 0.25;

  // variables used to calculate the positions of elements drawn on screen.
  double cellStartX = 0.0;
  double cellStartY = 0.0;
  double labelStartX = 0.0;
  double labelStartY = 0.0;

  bool currentlyCollapsingRows = false;

  bool currentlyCollapsingColumns = false;

  // this map helps us display information about the correct row & column
  // in our tooltips
  this->SceneRowToTableRowMap.clear();
  this->SceneRowToTableRowMap.assign(this->Table->GetNumberOfRows(), -1);
  this->SceneColumnToTableColumnMap.clear();
  this->SceneColumnToTableColumnMap.assign(this->Table->GetNumberOfColumns(), -1);

  // Setup text property & calculate an appropriate font size for this zoom
  // level.  "Igq" was selected for the range of height of its characters.
  painter->GetTextProp()->SetColor(0.0, 0.0, 0.0);
  painter->GetTextProp()->SetVerticalJustificationToCentered();
  painter->GetTextProp()->SetJustificationToLeft();
  painter->GetTextProp()->SetOrientation(0.0);
  int fontSize = painter->ComputeFontSizeForBoundedString("Igq", SVTK_FLOAT_MAX, this->CellHeight);

  // canDrawText is set to false if we're too zoomed out to draw legible text.
  bool canDrawText = true;
  if (fontSize < 8)
  {
    canDrawText = false;
  }
  bool drawRowLabels = canDrawText;
  bool drawColumnLabels = canDrawText;

  int orientation = this->GetOrientation();

  // Detect if our row or column labels would be currently visible on screen.
  if (canDrawText)
  {
    switch (orientation)
    {
      case svtkHeatmapItem::DOWN_TO_UP:
        if (this->SceneBottomLeft[1] > this->MaxY + spacing ||
          this->SceneTopRight[1] < this->MaxY + spacing)
        {
          drawRowLabels = false;
        }
        if (this->SceneBottomLeft[0] > this->MaxX + spacing ||
          this->SceneTopRight[0] < this->MaxX + spacing)
        {
          drawColumnLabels = false;
        }
        break;

      case svtkHeatmapItem::RIGHT_TO_LEFT:
        if (this->SceneBottomLeft[0] > this->MinX - spacing ||
          this->SceneTopRight[0] < this->MinX - spacing)
        {
          drawRowLabels = false;
        }
        else
        {
          painter->GetTextProp()->SetJustificationToRight();
        }
        if (this->SceneBottomLeft[1] > this->MaxY + spacing &&
          this->SceneTopRight[1] < this->MaxY + spacing)
        {
          drawColumnLabels = false;
        }
        break;

      case svtkHeatmapItem::UP_TO_DOWN:
        if (this->SceneBottomLeft[1] > this->MinY - spacing ||
          this->SceneTopRight[1] < this->MinY - spacing)
        {
          drawRowLabels = false;
        }
        else
        {
          painter->GetTextProp()->SetJustificationToRight();
        }
        if (this->SceneBottomLeft[0] > this->MaxX + spacing ||
          this->SceneTopRight[0] < this->MaxX + spacing)
        {
          drawColumnLabels = false;
        }
        break;

      case svtkHeatmapItem::LEFT_TO_RIGHT:
      default:
        if (this->SceneBottomLeft[0] > this->MaxX + spacing ||
          this->SceneTopRight[0] < this->MaxX + spacing)
        {
          drawRowLabels = false;
        }
        if (this->SceneBottomLeft[1] > this->MaxY + spacing &&
          this->SceneTopRight[1] < this->MaxY + spacing)
        {
          drawColumnLabels = false;
        }
        break;
    }
  }

  // set the orientation of our text property to draw row names
  if (drawRowLabels)
  {
    painter->GetTextProp()->SetOrientation(this->GetTextAngleForOrientation(orientation));
  }

  // keep track of what row & column we're drawing next
  svtkIdType rowToDraw = 0;
  svtkIdType columnToDraw = 0;
  bool columnMapSet = false;

  for (svtkIdType row = 0; row != this->Table->GetNumberOfRows(); ++row)
  {
    // check if this row has been collapsed or not
    if (this->CollapsedRowsArray && this->CollapsedRowsArray->GetValue(row) == 1)
    {
      // a contiguous block of collapsed rows is represented as a single blank
      // row by this item.
      if (!currentlyCollapsingRows)
      {
        this->SceneRowToTableRowMap[rowToDraw] = -1;
        ++rowToDraw;
        currentlyCollapsingRows = true;
      }
      continue;
    }
    currentlyCollapsingRows = false;

    // get the name of this row
    std::string name;
    if (this->RowNames)
    {
      name = this->RowNames->GetValue(row);
    }

    // only draw the cells of this row if it isn't explicitly marked as blank
    if (this->BlankRows.find(name) == this->BlankRows.end())
    {
      columnToDraw = 0;
      for (svtkIdType column = 0; column < this->Table->GetNumberOfColumns(); ++column)
      {
        // don't draw the name column as part of the heatmap
        // (it's used later to label the rows instead)
        if (this->Table->GetColumn(column) == this->GetRowNames())
        {
          continue;
        }

        // check if this column has been collapsed or not
        if (this->CollapsedColumnsArray && this->CollapsedColumnsArray->GetValue(column) == 1)
        {
          // a contiguous block of collapsed columns is represented as a single blank
          // column by this item.
          if (!currentlyCollapsingColumns)
          {
            this->SceneColumnToTableColumnMap[columnToDraw] = -1;
            ++columnToDraw;
            currentlyCollapsingColumns = true;
          }
          continue;
        }
        currentlyCollapsingColumns = false;

        // get the color for this cell from the lookup table
        double color[4];
        svtkVariant value = this->Table->GetValue(row, column);
        if (value.IsString())
        {
          this->CategoricalDataLookupTable->GetAnnotationColor(value, color);
        }
        else
        {
          // set the range on our continuous lookup table for this column
          this->ContinuousDataLookupTable->SetRange(
            this->ColumnRanges[column].first, this->ColumnRanges[column].second);

          // get the color for this value
          this->ContinuousDataLookupTable->GetColor(value.ToDouble(), color);
        }
        painter->GetBrush()->SetColorF(color[0], color[1], color[2]);

        // draw this cell of the table
        double w = 0.0;
        double h = 0.0;
        switch (orientation)
        {
          case svtkHeatmapItem::DOWN_TO_UP:
            cellStartX = this->Position[0] + this->CellHeight * rowToDraw;
            cellStartY = this->MinY + this->CellWidth * columnToDraw;
            h = this->CellWidth;
            w = this->CellHeight;
            break;

          case svtkHeatmapItem::RIGHT_TO_LEFT:
            cellStartX = this->MinX + this->CellWidth * columnToDraw;
            cellStartY = this->Position[1] + this->CellHeight * rowToDraw;
            w = this->CellWidth;
            h = this->CellHeight;
            break;

          case svtkHeatmapItem::UP_TO_DOWN:
            cellStartX = this->Position[0] + this->CellHeight * rowToDraw;
            cellStartY = this->MinY + this->CellWidth * columnToDraw;
            h = this->CellWidth;
            w = this->CellHeight;
            break;

          case svtkHeatmapItem::LEFT_TO_RIGHT:
          default:
            cellStartX = this->MinX + this->CellWidth * columnToDraw;
            cellStartY = this->Position[1] + this->CellHeight * rowToDraw;
            w = this->CellWidth;
            h = this->CellHeight;
            break;
        }

        if (this->LineIsVisible(cellStartX, cellStartY, cellStartX + this->CellWidth,
              cellStartY + this->CellHeight) ||
          this->LineIsVisible(
            cellStartX, cellStartY + this->CellHeight, cellStartX + this->CellWidth, cellStartY))
        {
          painter->DrawRect(cellStartX, cellStartY, w, h);
        }

        if (!columnMapSet)
        {
          this->SceneColumnToTableColumnMap[columnToDraw] = column;
        }

        ++columnToDraw;
      }
      columnMapSet = true;
    }

    this->SceneRowToTableRowMap[rowToDraw] = row;
    ++rowToDraw;

    // draw this row's label if it would be visible
    if (!drawRowLabels)
    {
      continue;
    }

    switch (orientation)
    {
      case svtkHeatmapItem::DOWN_TO_UP:
        labelStartX = cellStartX + this->CellHeight / 2.0;
        labelStartY = this->MaxY + spacing;
        break;
      case svtkHeatmapItem::RIGHT_TO_LEFT:
        labelStartX = this->MinX - spacing;
        labelStartY = cellStartY + this->CellHeight / 2.0;
        break;
      case svtkHeatmapItem::UP_TO_DOWN:
        labelStartX = cellStartX + this->CellHeight / 2.0;
        labelStartY = this->MinY - spacing;
        break;
      case svtkHeatmapItem::LEFT_TO_RIGHT:
      default:
        labelStartX = this->MaxX + spacing;
        labelStartY = cellStartY + this->CellHeight / 2.0;
        break;
    }

    if (!name.empty() && this->SceneBottomLeft[0] < labelStartX &&
      this->SceneTopRight[0] > labelStartX && this->SceneBottomLeft[1] < labelStartY &&
      this->SceneTopRight[1] > labelStartY)
    {
      painter->DrawString(labelStartX, labelStartY, name);
    }
  }

  // draw column labels
  if (!canDrawText)
  {
    this->RowLabelWidth = 0.0;
    this->ColumnLabelWidth = 0.0;
    return;
  }

  if (!drawColumnLabels)
  {
    this->ComputeLabelWidth(painter);
    this->ColumnLabelWidth = 0.0;
    return;
  }

  // set up our text property to draw column labels appropriately for
  // the current orientation.
  switch (orientation)
  {
    case svtkHeatmapItem::DOWN_TO_UP:
    case svtkHeatmapItem::UP_TO_DOWN:
      painter->GetTextProp()->SetOrientation(0);
      break;

    case svtkHeatmapItem::RIGHT_TO_LEFT:
    case svtkHeatmapItem::LEFT_TO_RIGHT:
    default:
      painter->GetTextProp()->SetOrientation(90);
      break;
  }

  painter->GetTextProp()->SetJustificationToLeft();

  columnToDraw = 1;
  for (svtkIdType column = 0; column < this->Table->GetNumberOfColumns(); ++column)
  {
    // don't draw the name column as part of the heatmap
    // (it's used later to label the rows instead)
    if (this->Table->GetColumn(column) == this->GetRowNames())
    {
      continue;
    }

    // check if this column has been collapsed or not
    if (this->CollapsedColumnsArray && this->CollapsedColumnsArray->GetValue(column) == 1)
    {
      // a contiguous block of collapsed columns is represented as a single blank
      // column by this item.
      if (!currentlyCollapsingColumns)
      {
        ++columnToDraw;
        currentlyCollapsingColumns = true;
      }
      continue;
    }
    currentlyCollapsingColumns = false;

    switch (orientation)
    {
      case svtkHeatmapItem::DOWN_TO_UP:
      case svtkHeatmapItem::UP_TO_DOWN:
        labelStartX = this->MaxX + spacing;
        labelStartY = this->MinY + this->CellWidth * columnToDraw - this->CellWidth / 2;
        break;

      case svtkHeatmapItem::RIGHT_TO_LEFT:
      case svtkHeatmapItem::LEFT_TO_RIGHT:
      default:
        labelStartX = this->MinX + this->CellWidth * columnToDraw - this->CellWidth / 2;
        labelStartY = this->MaxY + spacing;
        break;
    }

    std::string columnName = this->Table->GetColumn(column)->GetName();
    if (this->SceneBottomLeft[0] < labelStartX && this->SceneTopRight[0] > labelStartX &&
      this->SceneBottomLeft[1] < labelStartY && this->SceneTopRight[1] > labelStartY)
    {
      painter->DrawString(labelStartX, labelStartY, columnName);
    }
    ++columnToDraw;
  }

  // update the size of our labels
  this->ComputeLabelWidth(painter);
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::UpdateVisibleSceneExtent(svtkContext2D* painter)
{
  float position[2];
  painter->GetTransform()->GetPosition(position);
  this->SceneBottomLeft[0] = -position[0];
  this->SceneBottomLeft[1] = -position[1];
  this->SceneBottomLeft[2] = 0.0;

  this->SceneTopRight[0] = static_cast<double>(this->GetScene()->GetSceneWidth() - position[0]);
  this->SceneTopRight[1] = static_cast<double>(this->GetScene()->GetSceneHeight() - position[1]);
  this->SceneTopRight[2] = 0.0;
  svtkNew<svtkMatrix3x3> inverse;
  painter->GetTransform()->GetInverse(inverse);
  inverse->MultiplyPoint(this->SceneBottomLeft, this->SceneBottomLeft);
  inverse->MultiplyPoint(this->SceneTopRight, this->SceneTopRight);
}

//-----------------------------------------------------------------------------
bool svtkHeatmapItem::LineIsVisible(double x0, double y0, double x1, double y1)
{
  // use local variables to improve readability
  double xMinScene = this->SceneBottomLeft[0];
  double yMinScene = this->SceneBottomLeft[1];
  double xMaxScene = this->SceneTopRight[0];
  double yMaxScene = this->SceneTopRight[1];

  // if either end point of the line segment falls within the screen,
  // then the line segment is visible.
  if ((xMinScene <= x0 && xMaxScene >= x0 && yMinScene <= y0 && yMaxScene >= y0) ||
    (xMinScene <= x1 && xMaxScene >= x1 && yMinScene <= y1 && yMaxScene >= y1))
  {
    return true;
  }

  // figure out which end point is "greater" than the other in both dimensions
  double xMinLine, xMaxLine, yMinLine, yMaxLine;
  if (x0 < x1)
  {
    xMinLine = x0;
    xMaxLine = x1;
  }
  else
  {
    xMinLine = x1;
    xMaxLine = x0;
  }
  if (y0 < y1)
  {
    yMinLine = y0;
    yMaxLine = y1;
  }
  else
  {
    yMinLine = y1;
    yMaxLine = y0;
  }

  // case where the Y range of the line falls within the visible scene
  // and the X range of the line contains the entire visible scene
  if (yMinScene <= yMinLine && yMaxScene >= yMinLine && yMinScene <= yMaxLine &&
    yMaxScene >= yMaxLine && xMinLine <= xMinScene && xMaxLine >= xMaxScene)
  {
    return true;
  }

  // case where the X range of the line falls within the visible scene
  // and the Y range of the line contains the entire visible scene
  if (xMinScene <= xMinLine && xMaxScene >= xMinLine && xMinScene <= xMaxLine &&
    xMaxScene >= xMaxLine && yMinLine <= yMinScene && yMaxLine >= yMaxScene)
  {
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
bool svtkHeatmapItem::MouseMoveEvent(const svtkContextMouseEvent& event)
{
  if (event.GetButton() == svtkContextMouseEvent::NO_BUTTON)
  {
    float pos[3];
    svtkNew<svtkMatrix3x3> inverse;
    pos[0] = event.GetPos().GetX();
    pos[1] = event.GetPos().GetY();
    pos[2] = 0;
    this->GetScene()->GetTransform()->GetInverse(inverse);
    inverse->MultiplyPoint(pos, pos);
    if (pos[0] <= this->MaxX && pos[0] >= this->MinX && pos[1] <= this->MaxY &&
      pos[1] >= this->MinY)
    {
      this->Tooltip->SetPosition(pos[0], pos[1]);

      std::string tooltipText = this->GetTooltipText(pos[0], pos[1]);
      if (tooltipText.compare("") != 0)
      {
        this->Tooltip->SetText(tooltipText);
        this->Tooltip->SetVisible(true);
        this->Scene->SetDirty(true);
        return true;
      }
    }
    bool shouldRepaint = this->Tooltip->GetVisible();
    this->Tooltip->SetVisible(false);
    if (shouldRepaint)
    {
      this->Scene->SetDirty(true);
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
std::string svtkHeatmapItem::GetTooltipText(float x, float y)
{
  int sceneRow = 0;
  int sceneColumn = 0;
  int orientation = this->GetOrientation();
  if (orientation == svtkHeatmapItem::UP_TO_DOWN || orientation == svtkHeatmapItem::DOWN_TO_UP)
  {
    sceneRow = static_cast<int>(floor(fabs(x - this->Position[0]) / this->CellHeight));
    sceneColumn = static_cast<int>(floor((y - this->MinY) / this->CellWidth));
  }
  else
  {
    sceneRow = static_cast<int>(floor(fabs(y - this->Position[1]) / this->CellHeight));
    sceneColumn = static_cast<int>(floor((x - this->MinX) / this->CellWidth));
  }

  svtkIdType row = -1;
  if (static_cast<unsigned int>(sceneRow) < this->SceneRowToTableRowMap.size())
  {
    row = this->SceneRowToTableRowMap[sceneRow];
  }
  svtkIdType column = -1;
  if (static_cast<unsigned int>(sceneColumn) < this->SceneColumnToTableColumnMap.size())
  {
    column = this->SceneColumnToTableColumnMap[sceneColumn];
  }

  if (row > -1 && column > -1)
  {
    std::string rowName;
    if (this->RowNames)
    {
      rowName = this->RowNames->GetValue(row);
    }
    else
    {
      std::stringstream ss;
      ss << row;
      rowName = ss.str();
    }
    if (this->BlankRows.find(rowName) != this->BlankRows.end())
    {
      return "";
    }

    std::string columnName = this->Table->GetColumn(column)->GetName();

    std::string tooltipText = "(";
    tooltipText += rowName;
    tooltipText += ", ";
    tooltipText += columnName;
    tooltipText += ")\n";
    tooltipText += this->Table->GetValue(row, column).ToString();

    return tooltipText;
  }
  return "";
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::SetOrientation(int orientation)
{
  svtkIntArray* existingArray =
    svtkArrayDownCast<svtkIntArray>(this->Table->GetFieldData()->GetArray("orientation"));
  if (existingArray)
  {
    existingArray->SetValue(0, orientation);
  }
  else
  {
    svtkSmartPointer<svtkIntArray> orientationArray = svtkSmartPointer<svtkIntArray>::New();
    orientationArray->SetNumberOfComponents(1);
    orientationArray->SetName("orientation");
    orientationArray->InsertNextValue(orientation);
    this->Table->GetFieldData()->AddArray(orientationArray);
  }

  // reposition the legends
  this->PositionLegends(orientation);
}

//-----------------------------------------------------------------------------
int svtkHeatmapItem::GetOrientation()
{
  svtkIntArray* orientationArray =
    svtkArrayDownCast<svtkIntArray>(this->Table->GetFieldData()->GetArray("orientation"));
  if (orientationArray)
  {
    return orientationArray->GetValue(0);
  }
  return svtkHeatmapItem::LEFT_TO_RIGHT;
}

//-----------------------------------------------------------------------------
double svtkHeatmapItem::GetTextAngleForOrientation(int orientation)
{
  switch (orientation)
  {
    case svtkHeatmapItem::DOWN_TO_UP:
      return 90.0;

    case svtkHeatmapItem::RIGHT_TO_LEFT:
      return 0.0;

    case svtkHeatmapItem::UP_TO_DOWN:
      return 270.0;

    case svtkHeatmapItem::LEFT_TO_RIGHT:
    default:
      return 0.0;
  }
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::ComputeLabelWidth(svtkContext2D* painter)
{
  this->RowLabelWidth = 0.0;
  this->ColumnLabelWidth = 0.0;

  int fontSize = painter->ComputeFontSizeForBoundedString("Igq", SVTK_FLOAT_MAX, this->CellHeight);
  if (fontSize < 8)
  {
    return;
  }

  // temporarily set text to default orientation
  double orientation = painter->GetTextProp()->GetOrientation();
  painter->GetTextProp()->SetOrientation(0.0);

  float bounds[4];
  // find the longest row label
  if (this->RowNames)
  {

    for (svtkIdType row = 0; row != this->Table->GetNumberOfRows(); ++row)
    {
      if (this->CollapsedRowsArray && this->CollapsedRowsArray->GetValue(row) == 1)
      {
        continue;
      }
      std::string name = this->RowNames->GetValue(row);
      painter->ComputeStringBounds(name, bounds);
      if (bounds[2] > this->RowLabelWidth)
      {
        this->RowLabelWidth = bounds[2];
      }
    }
  }

  // find the longest column label
  for (svtkIdType col = 0; col != this->Table->GetNumberOfColumns(); ++col)
  {
    if (this->Table->GetColumn(col) == this->GetRowNames())
    {
      continue;
    }
    if (this->CollapsedColumnsArray && this->CollapsedColumnsArray->GetValue(col) == 1)
    {
      continue;
    }
    std::string name = this->Table->GetColumn(col)->GetName();
    painter->ComputeStringBounds(name, bounds);
    if (bounds[2] > this->ColumnLabelWidth)
    {
      this->ColumnLabelWidth = bounds[2];
    }
  }

  // restore orientation
  painter->GetTextProp()->SetOrientation(orientation);
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::ComputeBounds()
{
  // figure out how many actual rows will be drawn
  bool currentlyCollapsingRows = false;
  int numRows = 0;
  for (svtkIdType row = 0; row != this->Table->GetNumberOfRows(); ++row)
  {
    if (this->CollapsedRowsArray && this->CollapsedRowsArray->GetValue(row) == 1)
    {
      // a contiguous block of collapsed rows is represented as a single blank
      // row by this item.
      if (!currentlyCollapsingRows)
      {
        ++numRows;
        currentlyCollapsingRows = true;
      }
      continue;
    }
    currentlyCollapsingRows = false;
    ++numRows;
  }

  // figure out how many actual columns will be drawn
  bool currentlyCollapsingColumns = false;
  int numColumns = 0;
  for (svtkIdType col = 0; col != this->Table->GetNumberOfColumns(); ++col)
  {
    if (this->Table->GetColumn(col) == this->GetRowNames())
    {
      continue;
    }
    if (this->CollapsedColumnsArray && this->CollapsedColumnsArray->GetValue(col) == 1)
    {
      // a contiguous block of collapsed columns is represented as a single blank
      // column by this item.
      if (!currentlyCollapsingColumns)
      {
        ++numColumns;
        currentlyCollapsingColumns = true;
      }
      continue;
    }
    currentlyCollapsingColumns = false;
    ++numColumns;
  }

  this->MinX = this->Position[0];
  this->MinY = this->Position[1];
  switch (this->GetOrientation())
  {
    case svtkHeatmapItem::RIGHT_TO_LEFT:
    case svtkHeatmapItem::LEFT_TO_RIGHT:
    default:
      this->MaxX = this->MinX + this->CellWidth * numColumns;
      this->MaxY = this->MinY + this->CellHeight * numRows;
      break;

    case svtkHeatmapItem::UP_TO_DOWN:
    case svtkHeatmapItem::DOWN_TO_UP:
      this->MaxX = this->MinX + this->CellHeight * numRows;
      this->MaxY = this->MinY + this->CellWidth * numColumns;
      break;
  }
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::GetBounds(double bounds[4])
{
  bounds[0] = this->MinX;
  bounds[1] = this->MaxX;
  bounds[2] = this->MinY;
  bounds[3] = this->MaxY;

  if (this->RowLabelWidth == 0.0 && this->ColumnLabelWidth == 0.0)
  {
    return;
  }

  double spacing = this->CellWidth * 0.25;

  switch (this->GetOrientation())
  {
    case svtkHeatmapItem::LEFT_TO_RIGHT:
    default:
      bounds[1] += spacing + this->RowLabelWidth;
      bounds[3] += spacing + this->ColumnLabelWidth;
      break;

    case svtkHeatmapItem::UP_TO_DOWN:
      bounds[1] += spacing + this->ColumnLabelWidth;
      bounds[2] -= spacing + this->RowLabelWidth;
      break;

    case svtkHeatmapItem::RIGHT_TO_LEFT:
      bounds[0] -= spacing + this->RowLabelWidth;
      bounds[3] += spacing + this->ColumnLabelWidth;
      break;

    case svtkHeatmapItem::DOWN_TO_UP:
      bounds[1] += spacing + this->ColumnLabelWidth;
      bounds[3] += spacing + this->RowLabelWidth;
      break;
  }
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::MarkRowAsBlank(const std::string& rowName)
{
  this->BlankRows.insert(rowName);
}

//-----------------------------------------------------------------------------
bool svtkHeatmapItem::MouseDoubleClickEvent(const svtkContextMouseEvent& event)
{
  // get the position of the double click and convert it to scene coordinates
  double pos[3];
  svtkNew<svtkMatrix3x3> inverse;
  pos[0] = event.GetPos().GetX();
  pos[1] = event.GetPos().GetY();
  pos[2] = 0;
  this->GetScene()->GetTransform()->GetInverse(inverse);
  inverse->MultiplyPoint(pos, pos);
  if (pos[0] <= this->MaxX && pos[0] >= this->MinX && pos[1] <= this->MaxY && pos[1] >= this->MinY)
  {
    svtkIdType column = 0;
    int orientation = this->GetOrientation();
    if (orientation == svtkHeatmapItem::UP_TO_DOWN || orientation == svtkHeatmapItem::DOWN_TO_UP)
    {
      column = static_cast<svtkIdType>(floor((pos[1] - this->MinY) / this->CellWidth));
    }
    else
    {
      column = static_cast<svtkIdType>(floor((pos[0] - this->MinX) / this->CellWidth));
    }
    ++column;

    if (!this->LegendPositionSet)
    {
      this->PositionLegends(this->GetOrientation());
    }

    if (this->Table->GetValue(0, column).IsString())
    {
      // categorical data
      // generate an array of distinct values from this column
      svtkStringArray* stringColumn =
        svtkArrayDownCast<svtkStringArray>(this->Table->GetColumn(column));
      this->CategoryLegendValues->Reset();
      this->CategoryLegendValues->Squeeze();
      stringColumn->SetMaxDiscreteValues(stringColumn->GetNumberOfTuples() - 1);
      stringColumn->GetProminentComponentValues(0, this->CategoryLegendValues);
      this->CategoryLegendValues->Modified();

      // these distinct values become the input to our categorical legend
      this->CategoryLegend->SetValues(this->CategoryLegendValues);
      this->CategoryLegend->SetTitle(this->Table->GetColumn(column)->GetName());
      this->CategoryLegend->SetVisible(true);
      this->ColorLegend->SetVisible(false);
      this->Scene->SetDirty(true);
      return true;
    }
    else
    {
      // continuous data
      // set up the scalar bar legend
      this->ColorLegend->GetTransferFunction()->SetRange(
        this->ColumnRanges[column].first, this->ColumnRanges[column].second);

      this->ColorLegend->SetTitle(this->Table->GetColumn(column)->GetName());

      this->ColorLegend->Update();
      this->ColorLegend->SetVisible(true);
      this->CategoryLegend->SetVisible(false);
      this->Scene->SetDirty(true);
      return true;
    }
  }
  bool shouldRepaint = this->ColorLegend->GetVisible() || this->CategoryLegend->GetVisible();
  this->CategoryLegend->SetVisible(false);
  this->ColorLegend->SetVisible(false);
  if (shouldRepaint)
  {
    this->Scene->SetDirty(true);
  }

  return false;
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::PositionLegends(int orientation)
{
  // bail out early if we don't have meaningful bounds yet.
  if (this->MinX > this->MaxX || this->MinY > this->MaxY)
  {
    return;
  }

  switch (orientation)
  {
    case svtkHeatmapItem::DOWN_TO_UP:
    case svtkHeatmapItem::UP_TO_DOWN:

      this->CategoryLegend->SetHorizontalAlignment(svtkChartLegend::RIGHT);
      this->CategoryLegend->SetVerticalAlignment(svtkChartLegend::CENTER);
      this->CategoryLegend->SetPoint(
        this->MinX - this->CellHeight, this->MinY + (this->MaxY - this->MinY) / 2.0);

      this->ColorLegend->SetHorizontalAlignment(svtkChartLegend::RIGHT);
      this->ColorLegend->SetVerticalAlignment(svtkChartLegend::CENTER);
      this->ColorLegend->SetOrientation(svtkColorLegend::VERTICAL);
      this->ColorLegend->SetPoint(
        this->MinX - this->CellHeight, this->MinY + (this->MaxY - this->MinY) / 2.0);
      this->ColorLegend->SetTextureSize(
        this->ColorLegend->GetSymbolWidth(), this->MaxY - this->MinY);
      break;

    case svtkHeatmapItem::RIGHT_TO_LEFT:
    case svtkHeatmapItem::LEFT_TO_RIGHT:
    default:

      this->CategoryLegend->SetHorizontalAlignment(svtkChartLegend::CENTER);
      this->CategoryLegend->SetVerticalAlignment(svtkChartLegend::TOP);
      this->CategoryLegend->SetPoint(
        this->MinX + (this->MaxX - this->MinX) / 2.0, this->MinY - this->CellHeight);

      this->ColorLegend->SetHorizontalAlignment(svtkChartLegend::CENTER);
      this->ColorLegend->SetVerticalAlignment(svtkChartLegend::TOP);
      this->ColorLegend->SetOrientation(svtkColorLegend::HORIZONTAL);
      this->ColorLegend->SetPoint(
        this->MinX + (this->MaxX - this->MinX) / 2.0, this->MinY - this->CellHeight);
      this->ColorLegend->SetTextureSize(
        this->MaxX - this->MinX, this->ColorLegend->GetSymbolWidth());
      break;
  }
  this->LegendPositionSet = true;
}

//-----------------------------------------------------------------------------
bool svtkHeatmapItem::Hit(const svtkContextMouseEvent& svtkNotUsed(mouse))
{
  // If we are interactive, we want to catch anything that propagates to the
  // background, otherwise we do not want any mouse events.
  return this->Interactive;
}

//-----------------------------------------------------------------------------
void svtkHeatmapItem::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << "Table: " << (this->Table ? "" : "(null)") << std::endl;
  if (this->Table->GetNumberOfRows() > 0)
  {
    this->Table->PrintSelf(os, indent.GetNextIndent());
  }
}
