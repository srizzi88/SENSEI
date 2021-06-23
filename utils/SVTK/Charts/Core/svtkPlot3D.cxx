/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPlot3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkPlot3D.h"

#include "svtkChartXYZ.h"
#include "svtkDataArray.h"
#include "svtkIdTypeArray.h"
#include "svtkLookupTable.h"
#include "svtkObjectFactory.h"
#include "svtkPen.h"
#include "svtkStringArray.h"
#include "svtkTable.h"

namespace
{
// FIXME: Put this in a central header, as it is used across several classes.
// Copy the two arrays into the points array
template <class A>
void CopyToPoints(float* data, A* input, size_t offset, size_t n)
{
  for (size_t i = 0; i < n; ++i)
  {
    data[3 * i + offset] = *(input++);
  }
}

}

//-----------------------------------------------------------------------------
svtkPlot3D::svtkPlot3D()
{
  this->Pen = svtkSmartPointer<svtkPen>::New();
  this->Pen->SetWidth(2.0);
  this->SelectionPen = svtkSmartPointer<svtkPen>::New();
  this->SelectionPen->SetColor(255, 50, 0, 150);
  this->SelectionPen->SetWidth(4.0);
  this->NumberOfComponents = 0;
  this->Chart = nullptr;
}

//-----------------------------------------------------------------------------
svtkPlot3D::~svtkPlot3D() = default;

//-----------------------------------------------------------------------------
void svtkPlot3D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
void svtkPlot3D::SetPen(svtkPen* pen)
{
  if (this->Pen != pen)
  {
    this->Pen = pen;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkPen* svtkPlot3D::GetSelectionPen()
{
  return this->SelectionPen;
}

//-----------------------------------------------------------------------------
void svtkPlot3D::SetSelectionPen(svtkPen* pen)
{
  if (this->SelectionPen != pen)
  {
    this->SelectionPen = pen;
    this->Modified();
  }
}

//-----------------------------------------------------------------------------
svtkPen* svtkPlot3D::GetPen()
{
  return this->Pen;
}

//-----------------------------------------------------------------------------
void svtkPlot3D::SetInputData(svtkTable* input)
{
  assert(input->GetNumberOfColumns() >= 3);

  // assume the 4th column is color info if available
  if (input->GetNumberOfColumns() > 3)
  {
    this->SetInputData(input, input->GetColumnName(0), input->GetColumnName(1),
      input->GetColumnName(2), input->GetColumnName(3));
  }
  else
  {
    this->SetInputData(
      input, input->GetColumnName(0), input->GetColumnName(1), input->GetColumnName(2));
  }
}

//-----------------------------------------------------------------------------
void svtkPlot3D::SetInputData(
  svtkTable* input, svtkIdType xColumn, svtkIdType yColumn, svtkIdType zColumn)
{
  this->SetInputData(input, input->GetColumnName(xColumn), input->GetColumnName(yColumn),
    input->GetColumnName(zColumn));
}

//-----------------------------------------------------------------------------
void svtkPlot3D::SetInputData(
  svtkTable* input, const svtkStdString& xName, const svtkStdString& yName, const svtkStdString& zName)
{
  // Copy the points into our data structure for rendering - pack x, y, z...
  svtkDataArray* xArr = svtkArrayDownCast<svtkDataArray>(input->GetColumnByName(xName.c_str()));
  svtkDataArray* yArr = svtkArrayDownCast<svtkDataArray>(input->GetColumnByName(yName.c_str()));
  svtkDataArray* zArr = svtkArrayDownCast<svtkDataArray>(input->GetColumnByName(zName.c_str()));

  // Ensure that we have valid data arrays, and that they are of the same length.
  assert(xArr);
  assert(yArr);
  assert(zArr);
  assert(xArr->GetNumberOfTuples() == yArr->GetNumberOfTuples() &&
    xArr->GetNumberOfTuples() == zArr->GetNumberOfTuples());

  size_t n = xArr->GetNumberOfTuples();
  this->Points.resize(n);
  float* data = this->Points[0].GetData();

  switch (xArr->GetDataType())
  {
    svtkTemplateMacro(CopyToPoints(data, static_cast<SVTK_TT*>(xArr->GetVoidPointer(0)), 0, n));
  }
  switch (yArr->GetDataType())
  {
    svtkTemplateMacro(CopyToPoints(data, static_cast<SVTK_TT*>(yArr->GetVoidPointer(0)), 1, n));
  }
  switch (zArr->GetDataType())
  {
    svtkTemplateMacro(CopyToPoints(data, static_cast<SVTK_TT*>(zArr->GetVoidPointer(0)), 2, n));
  }
  this->PointsBuildTime.Modified();

  // This removes the colors from our points.
  // They will be (re-)added by SetColors if necessary.
  this->NumberOfComponents = 0;

  this->XAxisLabel = xName;
  this->YAxisLabel = yName;
  this->ZAxisLabel = zName;
  this->ComputeDataBounds();
}

//-----------------------------------------------------------------------------
void svtkPlot3D::SetInputData(svtkTable* input, const svtkStdString& xName, const svtkStdString& yName,
  const svtkStdString& zName, const svtkStdString& colorName)
{
  this->SetInputData(input, xName, yName, zName);

  svtkDataArray* colorArr =
    svtkArrayDownCast<svtkDataArray>(input->GetColumnByName(colorName.c_str()));
  this->SetColors(colorArr);
}

//-----------------------------------------------------------------------------
void svtkPlot3D::SetColors(svtkDataArray* colorArr)
{
  assert(colorArr);
  assert((unsigned int)colorArr->GetNumberOfTuples() == this->Points.size());

  this->NumberOfComponents = 3;

  // generate a color lookup table
  svtkNew<svtkLookupTable> lookupTable;
  double min = SVTK_DOUBLE_MAX;
  double max = SVTK_DOUBLE_MIN;

  for (unsigned int i = 0; i < this->Points.size(); ++i)
  {
    double value = colorArr->GetComponent(i, 0);
    if (value > max)
    {
      max = value;
    }
    if (value < min)
    {
      min = value;
    }
  }

  lookupTable->SetNumberOfTableValues(256);
  lookupTable->SetRange(min, max);
  lookupTable->Build();
  this->Colors->Reset();

  for (unsigned int i = 0; i < this->Points.size(); ++i)
  {
    double value = colorArr->GetComponent(i, 0);
    const unsigned char* rgb = lookupTable->MapValue(value);
    this->Colors->InsertNextTypedTuple(&rgb[0]);
    this->Colors->InsertNextTypedTuple(&rgb[1]);
    this->Colors->InsertNextTypedTuple(&rgb[2]);
  }

  this->Modified();
}

//-----------------------------------------------------------------------------
void svtkPlot3D::ComputeDataBounds()
{
  double xMin = SVTK_DOUBLE_MAX;
  double xMax = SVTK_DOUBLE_MIN;
  double yMin = SVTK_DOUBLE_MAX;
  double yMax = SVTK_DOUBLE_MIN;
  double zMin = SVTK_DOUBLE_MAX;
  double zMax = SVTK_DOUBLE_MIN;

  for (unsigned int i = 0; i < this->Points.size(); ++i)
  {
    float* point = this->Points[i].GetData();
    if (point[0] < xMin)
    {
      xMin = point[0];
    }
    if (point[0] > xMax)
    {
      xMax = point[0];
    }
    if (point[1] < yMin)
    {
      yMin = point[1];
    }
    if (point[1] > yMax)
    {
      yMax = point[1];
    }
    if (point[2] < zMin)
    {
      zMin = point[2];
    }
    if (point[2] > zMax)
    {
      zMax = point[2];
    }
  }

  this->DataBounds.clear();
  this->DataBounds.resize(8);
  float* data = this->DataBounds[0].GetData();

  // point 1: xMin, yMin, zMin
  data[0] = xMin;
  data[1] = yMin;
  data[2] = zMin;

  // point 2: xMin, yMin, zMax
  data[3] = xMin;
  data[4] = yMin;
  data[5] = zMax;

  // point 3: xMin, yMax, zMin
  data[6] = xMin;
  data[7] = yMax;
  data[8] = zMin;

  // point 4: xMin, yMax, zMax
  data[9] = xMin;
  data[10] = yMax;
  data[11] = zMax;

  // point 5: xMax, yMin, zMin
  data[12] = xMax;
  data[13] = yMin;
  data[14] = zMin;

  // point 6: xMax, yMin, zMax
  data[15] = xMax;
  data[16] = yMin;
  data[17] = zMax;

  // point 7: xMax, yMax, zMin
  data[18] = xMax;
  data[19] = yMax;
  data[20] = zMin;

  // point 8: xMax, yMax, zMax
  data[21] = xMax;
  data[22] = yMax;
  data[23] = zMax;
}

// ----------------------------------------------------------------------------
void svtkPlot3D::SetChart(svtkChartXYZ* chart)
{
  this->Chart = chart;
}

// ----------------------------------------------------------------------------
std::string svtkPlot3D::GetXAxisLabel()
{
  return this->XAxisLabel;
}

// ----------------------------------------------------------------------------
std::string svtkPlot3D::GetYAxisLabel()
{
  return this->YAxisLabel;
}

// ----------------------------------------------------------------------------
std::string svtkPlot3D::GetZAxisLabel()
{
  return this->ZAxisLabel;
}

// ----------------------------------------------------------------------------
void svtkPlot3D::SetSelection(svtkIdTypeArray* id)
{
  if (id == this->Selection)
  {
    return;
  }
  this->Selection = id;
  this->Modified();
}

// ----------------------------------------------------------------------------
svtkIdTypeArray* svtkPlot3D::GetSelection()
{
  return this->Selection;
}

// ----------------------------------------------------------------------------
std::vector<svtkVector3f> svtkPlot3D::GetPoints()
{
  return this->Points;
}
