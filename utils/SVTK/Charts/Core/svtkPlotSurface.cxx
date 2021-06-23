/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlotSurface.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPlotSurface.h"
#include "svtkChartXYZ.h"
#include "svtkContext2D.h"
#include "svtkContext3D.h"
#include "svtkLookupTable.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkTable.h"
#include "svtkUnsignedCharArray.h"

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkPlotSurface);

//-----------------------------------------------------------------------------
svtkPlotSurface::svtkPlotSurface()
{
  this->NumberOfRows = 0;
  this->NumberOfColumns = 0;
  this->NumberOfVertices = 0;
  this->ColorComponents = 0;
  this->XAxisLabel = "X";
  this->YAxisLabel = "Y";
  this->ZAxisLabel = "Z";
  this->XMinimum = this->XMaximum = this->YMinimum = this->YMaximum = 0.0;
  this->DataHasBeenRescaled = true;
}

//-----------------------------------------------------------------------------
svtkPlotSurface::~svtkPlotSurface() = default;

//-----------------------------------------------------------------------------
void svtkPlotSurface::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool svtkPlotSurface::Paint(svtkContext2D* painter)
{
  if (!this->Visible)
  {
    return false;
  }

  if (!this->DataHasBeenRescaled)
  {
    this->RescaleData();
  }

  // Get the 3D context.
  svtkContext3D* context = painter->GetContext3D();

  if (!context)
  {
    return false;
  }

  context->ApplyPen(this->Pen);

  // draw the surface
  if (!this->Surface.empty())
  {
    context->DrawTriangleMesh(this->Surface[0].GetData(), static_cast<int>(this->Surface.size()),
      this->Colors->GetPointer(0), this->ColorComponents);
  }

  return true;
}

//-----------------------------------------------------------------------------
void svtkPlotSurface::SetInputData(svtkTable* input)
{
  this->InputTable = input;
  this->NumberOfRows = input->GetNumberOfRows();
  this->NumberOfColumns = input->GetNumberOfColumns();
  this->NumberOfVertices = (this->NumberOfRows - 1) * (this->NumberOfColumns - 1) * 6;

  // initialize data ranges to row and column indices if they are not
  // already set.
  if (this->XMinimum == 0 && this->XMaximum == 0)
  {
    this->XMaximum = this->NumberOfColumns - 1;
  }
  if (this->YMinimum == 0 && this->YMaximum == 0)
  {
    this->YMaximum = this->NumberOfRows - 1;
  }

  this->Points.clear();
  this->Points.resize(this->NumberOfRows * this->NumberOfColumns);
  float* data = this->Points[0].GetData();
  int pos = 0;
  float surfaceMin = SVTK_FLOAT_MAX;
  float surfaceMax = SVTK_FLOAT_MIN;
  for (int i = 0; i < this->NumberOfRows; ++i)
  {
    for (int j = 0; j < this->NumberOfColumns; ++j)
    {
      // X (columns)
      data[pos] = this->ColumnToX(j);
      ++pos;

      // Y (rows)
      data[pos] = this->RowToY(i);
      ++pos;

      // Z (cell value)
      float k = input->GetValue(i, j).ToFloat();
      data[pos] = k;
      ++pos;

      if (k < surfaceMin)
      {
        surfaceMin = k;
      }
      if (k > surfaceMax)
      {
        surfaceMax = k;
      }
    }
  }

  if (this->Chart)
  {
    this->Chart->RecalculateBounds();
  }
  this->ComputeDataBounds();

  // setup lookup table
  this->LookupTable->SetNumberOfTableValues(256);
  this->LookupTable->SetRange(surfaceMin, surfaceMax);
  this->LookupTable->Build();
  this->ColorComponents = 3;

  // generate the surface that is used for rendering
  this->GenerateSurface();

  this->DataHasBeenRescaled = true;
}

//-----------------------------------------------------------------------------
void svtkPlotSurface::SetInputData(svtkTable* input, const svtkStdString& svtkNotUsed(xName),
  const svtkStdString& svtkNotUsed(yName), const svtkStdString& svtkNotUsed(zName))
{
  svtkWarningMacro("Warning: parameters beyond svtkTable are ignored");
  this->SetInputData(input);
}

//-----------------------------------------------------------------------------
void svtkPlotSurface::SetInputData(svtkTable* input, const svtkStdString& svtkNotUsed(xName),
  const svtkStdString& svtkNotUsed(yName), const svtkStdString& svtkNotUsed(zName),
  const svtkStdString& svtkNotUsed(colorName))
{
  svtkWarningMacro("Warning: parameters beyond svtkTable are ignored");
  this->SetInputData(input);
}

//-----------------------------------------------------------------------------
void svtkPlotSurface::SetInputData(svtkTable* input, svtkIdType svtkNotUsed(xColumn),
  svtkIdType svtkNotUsed(yColumn), svtkIdType svtkNotUsed(zColumn))
{
  svtkWarningMacro("Warning: parameters beyond svtkTable are ignored");
  this->SetInputData(input);
}

//-----------------------------------------------------------------------------
void svtkPlotSurface::GenerateSurface()
{
  // clear out and initialize our surface & colors
  this->Surface.clear();
  this->Surface.resize(this->NumberOfVertices);
  this->Colors->Reset();
  this->Colors->Allocate(this->NumberOfVertices * 3);

  // collect vertices of triangles
  float* data = this->Surface[0].GetData();
  int pos = 0;
  for (int i = 0; i < this->NumberOfRows - 1; ++i)
  {
    for (int j = 0; j < this->NumberOfColumns - 1; ++j)
    {
      float value1 = this->InputTable->GetValue(i, j).ToFloat();
      float value2 = this->InputTable->GetValue(i, j + 1).ToFloat();
      float value3 = this->InputTable->GetValue(i + 1, j + 1).ToFloat();
      float value4 = this->InputTable->GetValue(i + 1, j).ToFloat();

      // bottom right triangle
      this->InsertSurfaceVertex(data, value1, i, j, pos);
      this->InsertSurfaceVertex(data, value2, i, j + 1, pos);
      this->InsertSurfaceVertex(data, value3, i + 1, j + 1, pos);

      // upper left triangle
      this->InsertSurfaceVertex(data, value1, i, j, pos);
      this->InsertSurfaceVertex(data, value3, i + 1, j + 1, pos);
      this->InsertSurfaceVertex(data, value4, i + 1, j, pos);
    }
  }
}

//-----------------------------------------------------------------------------
void svtkPlotSurface::InsertSurfaceVertex(float* data, float value, int i, int j, int& pos)
{
  data[pos] = this->ColumnToX(j);
  ++pos;
  data[pos] = this->RowToY(i);
  ++pos;
  data[pos] = value;
  ++pos;

  const unsigned char* rgb = this->LookupTable->MapValue(data[pos - 1]);
  this->Colors->InsertNextTypedTuple(&rgb[0]);
  this->Colors->InsertNextTypedTuple(&rgb[1]);
  this->Colors->InsertNextTypedTuple(&rgb[2]);
}

//-----------------------------------------------------------------------------
void svtkPlotSurface::SetXRange(float min, float max)
{
  this->XMinimum = min;
  this->XMaximum = max;
  this->DataHasBeenRescaled = false;
}

//-----------------------------------------------------------------------------
void svtkPlotSurface::SetYRange(float min, float max)
{
  this->YMinimum = min;
  this->YMaximum = max;
  this->DataHasBeenRescaled = false;
}

//-----------------------------------------------------------------------------
void svtkPlotSurface::RescaleData()
{
  float* data = this->Points[0].GetData();

  // rescale Points (used by ChartXYZ to generate axes scales).
  int pos = 0;
  for (int i = 0; i < this->NumberOfRows; ++i)
  {
    for (int j = 0; j < this->NumberOfColumns; ++j)
    {
      // X (columns)
      data[pos] = this->ColumnToX(j);
      ++pos;

      // Y (rows)
      data[pos] = this->RowToY(i);
      ++pos;

      // Z value doesn't change
      ++pos;
    }
  }
  this->Chart->RecalculateBounds();
  this->ComputeDataBounds();
  this->DataHasBeenRescaled = true;
}

//-----------------------------------------------------------------------------
float svtkPlotSurface::ColumnToX(int columnIndex)
{
  float newRange = this->XMaximum - this->XMinimum;
  return static_cast<float>(columnIndex) * (newRange / this->NumberOfColumns) + this->XMinimum;
}

//-----------------------------------------------------------------------------
float svtkPlotSurface::RowToY(int rowIndex)
{
  float newRange = this->YMaximum - this->YMinimum;
  return static_cast<float>(rowIndex) * (newRange / this->NumberOfRows) + this->YMinimum;
}
