/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotParallelCoordinates.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPlotParallelCoordinates.h"

#include "svtkAxis.h"
#include "svtkChartParallelCoordinates.h"
#include "svtkContext2D.h"
#include "svtkContextDevice2D.h"
#include "svtkContextMapper2D.h"
#include "svtkDataArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkLookupTable.h"
#include "svtkPen.h"
#include "svtkSmartPointer.h"
#include "svtkStringArray.h"
#include "svtkTable.h"
#include "svtkTimeStamp.h"
#include "svtkTransform2D.h"
#include "svtkUnsignedCharArray.h"
#include "svtkVector.h"

// Need to turn some arrays of strings into categories
#include "svtkStringToCategory.h"

#include "svtkObjectFactory.h"

#include <algorithm>
#include <vector>

class svtkPlotParallelCoordinates::Private : public std::vector<std::vector<float> >
{
public:
  Private() { this->SelectionInitialized = false; }

  std::vector<float> AxisPos;
  bool SelectionInitialized;
};

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPlotParallelCoordinates);

//-----------------------------------------------------------------------------
svtkPlotParallelCoordinates::svtkPlotParallelCoordinates()
{
  this->Storage = new svtkPlotParallelCoordinates::Private;
  this->Pen->SetColor(0, 0, 0, 25);

  this->LookupTable = nullptr;
  this->Colors = nullptr;
  this->ScalarVisibility = 0;
}

//-----------------------------------------------------------------------------
svtkPlotParallelCoordinates::~svtkPlotParallelCoordinates()
{
  delete this->Storage;
  if (this->LookupTable)
  {
    this->LookupTable->UnRegister(this);
  }
  if (this->Colors != nullptr)
  {
    this->Colors->UnRegister(this);
  }
}

//-----------------------------------------------------------------------------
void svtkPlotParallelCoordinates::Update()
{
  if (!this->Visible)
  {
    return;
  }
  // Check if we have an input
  svtkTable* table = this->Data->GetInput();
  if (!table)
  {
    svtkDebugMacro(<< "Update event called with no input table set.");
    return;
  }

  this->UpdateTableCache(table);
}

//-----------------------------------------------------------------------------
bool svtkPlotParallelCoordinates::Paint(svtkContext2D* painter)
{
  // This is where everything should be drawn, or dispatched to other methods.
  svtkDebugMacro(<< "Paint event called in svtkPlotParallelCoordinates.");

  if (!this->Visible)
  {
    return false;
  }

  painter->ApplyPen(this->Pen);

  if (this->Storage->empty())
  {
    return false;
  }

  size_t cols = this->Storage->size();
  size_t rows = this->Storage->at(0).size();
  std::vector<svtkVector2f> line(cols);

  // Update the axis positions
  svtkChartParallelCoordinates* parent = svtkChartParallelCoordinates::SafeDownCast(this->Parent);

  for (size_t i = 0; i < cols; ++i)
  {
    this->Storage->AxisPos[i] =
      parent->GetAxis(int(i)) ? parent->GetAxis(int(i))->GetPoint1()[0] : 0;
  }

  svtkIdType selection = 0;
  svtkIdType id = 0;
  if (this->Selection)
  {
    svtkIdType selectionSize = this->Selection->GetNumberOfTuples();
    if (selectionSize)
    {
      this->Selection->GetTypedTuple(selection, &id);
    }
  }

  // Draw all of the lines
  painter->ApplyPen(this->Pen);
  int ncComps(0);
  if (this->ScalarVisibility && this->Colors)
  {
    ncComps = static_cast<int>(this->Colors->GetNumberOfComponents());
  }
  if (this->ScalarVisibility && this->Colors && ncComps == 4)
  {
    for (size_t i = 0, nc = 0; i < rows; ++i, nc += ncComps)
    {
      for (size_t j = 0; j < cols; ++j)
      {
        line[j].Set(this->Storage->AxisPos[j], (*this->Storage)[j][i]);
      }
      painter->GetPen()->SetColor(this->Colors->GetPointer(static_cast<svtkIdType>(nc)));
      painter->DrawPoly(line[0].GetData(), static_cast<int>(cols));
    }
  }
  else
  {
    for (size_t i = 0; i < rows; ++i)
    {
      for (size_t j = 0; j < cols; ++j)
      {
        line[j].Set(this->Storage->AxisPos[j], (*this->Storage)[j][i]);
      }
      painter->DrawPoly(line[0].GetData(), static_cast<int>(cols));
    }
  }

  // Now draw the selected lines
  if (this->Selection)
  {
    painter->GetPen()->SetColor(255, 0, 0, 100);
    for (svtkIdType i = 0; i < this->Selection->GetNumberOfTuples(); ++i)
    {
      for (size_t j = 0; j < cols; ++j)
      {
        this->Selection->GetTypedTuple(i, &id);
        line[j].Set(this->Storage->AxisPos[j], (*this->Storage)[j][id]);
      }
      painter->DrawPoly(line[0].GetData(), static_cast<int>(cols));
    }
  }

  return true;
}

//-----------------------------------------------------------------------------
bool svtkPlotParallelCoordinates::PaintLegend(svtkContext2D* painter, const svtkRectf& rect, int)
{
  painter->ApplyPen(this->Pen);
  painter->DrawLine(rect[0], rect[1] + 0.5 * rect[3], rect[0] + rect[2], rect[1] + 0.5 * rect[3]);
  return true;
}

//-----------------------------------------------------------------------------
void svtkPlotParallelCoordinates::GetBounds(double*) {}

//-----------------------------------------------------------------------------
bool svtkPlotParallelCoordinates::SetSelectionRange(int axis, float low, float high)
{
  if (!this->Selection)
  {
    this->Storage->SelectionInitialized = false;
    this->Selection = svtkIdTypeArray::New();
  }

  if (this->Storage->SelectionInitialized)
  {
    // Further refine the selection that has already been made
    svtkIdTypeArray* array = svtkIdTypeArray::New();
    std::vector<float>& col = this->Storage->at(axis);
    for (svtkIdType i = 0; i < this->Selection->GetNumberOfTuples(); ++i)
    {
      svtkIdType id = 0;
      this->Selection->GetTypedTuple(i, &id);
      if (col[id] >= low && col[id] <= high)
      {
        // Remove this point - no longer selected
        array->InsertNextValue(id);
      }
    }
    this->Selection->DeepCopy(array);
    array->Delete();
  }
  else
  {
    // First run - ensure the selection list is empty and build it up
    std::vector<float>& col = this->Storage->at(axis);
    for (size_t i = 0; i < col.size(); ++i)
    {
      if (col[i] >= low && col[i] <= high)
      {
        // Remove this point - no longer selected
        this->Selection->InsertNextValue(static_cast<svtkIdType>(i));
      }
    }
    this->Storage->SelectionInitialized = true;
  }
  return true;
}

//-----------------------------------------------------------------------------
bool svtkPlotParallelCoordinates::ResetSelectionRange()
{
  this->Storage->SelectionInitialized = false;
  if (this->Selection)
  {
    this->Selection->SetNumberOfTuples(0);
  }
  return true;
}

//-----------------------------------------------------------------------------
void svtkPlotParallelCoordinates::SetInputData(svtkTable* table)
{
  if (table == this->Data->GetInput() && (!table || table->GetMTime() < this->BuildTime))
  {
    return;
  }

  bool updateVisibility = table != this->Data->GetInput();
  this->svtkPlot::SetInputData(table);
  svtkChartParallelCoordinates* parent = svtkChartParallelCoordinates::SafeDownCast(this->Parent);

  if (parent && table && updateVisibility)
  {
    parent->SetColumnVisibilityAll(false);
    // By default make the first 10 columns visible in a plot.
    for (svtkIdType i = 0; i < table->GetNumberOfColumns() && i < 10; ++i)
    {
      parent->SetColumnVisibility(table->GetColumnName(i), true);
    }
  }
  else if (parent && updateVisibility)
  {
    // No table, therefore no visible columns
    parent->GetVisibleColumns()->SetNumberOfTuples(0);
  }
}

//-----------------------------------------------------------------------------
bool svtkPlotParallelCoordinates::UpdateTableCache(svtkTable* table)
{
  // Each axis is a column in our storage array, they are scaled from 0.0 to 1.0
  svtkChartParallelCoordinates* parent = svtkChartParallelCoordinates::SafeDownCast(this->Parent);
  if (!parent || !table || table->GetNumberOfColumns() == 0)
  {
    return false;
  }

  svtkStringArray* cols = parent->GetVisibleColumns();

  this->Storage->resize(cols->GetNumberOfTuples());
  this->Storage->AxisPos.resize(cols->GetNumberOfTuples());
  svtkIdType rows = table->GetNumberOfRows();

  for (svtkIdType i = 0; i < cols->GetNumberOfTuples(); ++i)
  {
    std::vector<float>& col = this->Storage->at(i);
    svtkAxis* axis = parent->GetAxis(i);
    col.resize(rows);
    svtkSmartPointer<svtkDataArray> data =
      svtkArrayDownCast<svtkDataArray>(table->GetColumnByName(cols->GetValue(i)));
    if (!data)
    {
      if (table->GetColumnByName(cols->GetValue(i))->IsA("svtkStringArray"))
      {
        // We have a different kind of column - attempt to make it into an enum
        svtkStringToCategory* stoc = svtkStringToCategory::New();
        stoc->SetInputData(table);
        stoc->SetInputArrayToProcess(
          0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_ROWS, cols->GetValue(i));
        stoc->SetCategoryArrayName("enumPC");
        stoc->Update();
        svtkTable* table2 = svtkTable::SafeDownCast(stoc->GetOutput());
        svtkTable* stringTable = svtkTable::SafeDownCast(stoc->GetOutput(1));
        if (table2)
        {
          data = svtkArrayDownCast<svtkDataArray>(table2->GetColumnByName("enumPC"));
        }
        if (stringTable && stringTable->GetColumnByName("Strings"))
        {
          svtkStringArray* strings =
            svtkArrayDownCast<svtkStringArray>(stringTable->GetColumnByName("Strings"));
          svtkSmartPointer<svtkDoubleArray> arr = svtkSmartPointer<svtkDoubleArray>::New();
          for (svtkIdType j = 0; j < strings->GetNumberOfTuples(); ++j)
          {
            arr->InsertNextValue(j);
          }
          // Now we need to set the range on the string axis
          axis->SetCustomTickPositions(arr, strings);
          if (strings->GetNumberOfTuples() > 1)
          {
            axis->SetUnscaledRange(0.0, strings->GetNumberOfTuples() - 1);
          }
          else
          {
            axis->SetUnscaledRange(-0.1, 0.1);
          }
          axis->Update();
        }
        stoc->Delete();
      }
      // If we still don't have a valid data array then skip this column.
      if (!data)
      {
        continue;
      }
    }

    // Also need the range from the appropriate axis, to normalize points
    double min = axis->GetUnscaledMinimum();
    double max = axis->GetUnscaledMaximum();
    double scale = 1.0f / (max - min);

    for (svtkIdType j = 0; j < rows; ++j)
    {
      col[j] = (data->GetTuple1(j) - min) * scale;
    }
  }

  // Additions for color mapping
  if (this->ScalarVisibility && !this->ColorArrayName.empty())
  {
    svtkDataArray* c = svtkArrayDownCast<svtkDataArray>(table->GetColumnByName(this->ColorArrayName));
    // TODO: Should add support for categorical coloring & try enum lookup
    if (this->Colors)
    {
      this->Colors->UnRegister(this);
      this->Colors = nullptr;
    }
    if (c)
    {
      if (!this->LookupTable)
      {
        this->CreateDefaultLookupTable();
      }
      this->Colors = this->LookupTable->MapScalars(c, SVTK_COLOR_MODE_MAP_SCALARS, -1);
      // Consistent register and unregisters
      this->Colors->Register(this);
      this->Colors->Delete();
    }
  }

  this->BuildTime.Modified();
  return true;
}

//-----------------------------------------------------------------------------
void svtkPlotParallelCoordinates::SetLookupTable(svtkScalarsToColors* lut)
{
  if (this->LookupTable != lut)
  {
    if (this->LookupTable)
    {
      this->LookupTable->UnRegister(this);
    }
    this->LookupTable = lut;
    if (lut)
    {
      lut->Register(this);
    }
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkScalarsToColors* svtkPlotParallelCoordinates::GetLookupTable()
{
  if (this->LookupTable == nullptr)
  {
    this->CreateDefaultLookupTable();
  }
  return this->LookupTable;
}

//-----------------------------------------------------------------------------
void svtkPlotParallelCoordinates::CreateDefaultLookupTable()
{
  if (this->LookupTable)
  {
    this->LookupTable->UnRegister(this);
  }
  this->LookupTable = svtkLookupTable::New();
  // Consistent Register/UnRegisters.
  this->LookupTable->Register(this);
  this->LookupTable->Delete();
}

//-----------------------------------------------------------------------------
void svtkPlotParallelCoordinates::SelectColorArray(const svtkStdString& arrayName)
{
  svtkTable* table = this->Data->GetInput();
  if (!table)
  {
    svtkDebugMacro(<< "SelectColorArray called with no input table set.");
    return;
  }
  if (this->ColorArrayName == arrayName)
  {
    return;
  }
  for (svtkIdType c = 0; c < table->GetNumberOfColumns(); ++c)
  {
    if (table->GetColumnName(c) == arrayName)
    {
      this->ColorArrayName = arrayName;
      this->Modified();
      return;
    }
  }
  svtkDebugMacro(<< "SelectColorArray called with invalid column name.");
  this->ColorArrayName = "";
  this->Modified();
}

//-----------------------------------------------------------------------------
svtkStdString svtkPlotParallelCoordinates::GetColorArrayName()
{
  return this->ColorArrayName;
}

//-----------------------------------------------------------------------------
void svtkPlotParallelCoordinates::SelectColorArray(svtkIdType arrayNum)
{
  svtkTable* table = this->Data->GetInput();
  if (!table)
  {
    svtkDebugMacro(<< "SelectColorArray called with no input table set.");
    return;
  }
  svtkDataArray* col = svtkArrayDownCast<svtkDataArray>(table->GetColumn(arrayNum));
  // TODO: Should add support for categorical coloring & try enum lookup
  if (!col)
  {
    svtkDebugMacro(<< "SelectColorArray called with invalid column index");
    return;
  }
  else
  {
    if (this->ColorArrayName == table->GetColumnName(arrayNum))
    {
      return;
    }
    else
    {
      this->ColorArrayName = table->GetColumnName(arrayNum);
      this->Modified();
    }
  }
}

//-----------------------------------------------------------------------------
void svtkPlotParallelCoordinates::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
