/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLineSource.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLineSource.h"

#include "svtkCellArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkVector.h"
#include "svtkVectorOperators.h"

#include <cmath>

svtkStandardNewMacro(svtkLineSource);
svtkCxxSetObjectMacro(svtkLineSource, Points, svtkPoints);

// ----------------------------------------------------------------------
svtkLineSource::svtkLineSource(int res)
{
  this->Point1[0] = -.5;
  this->Point1[1] = .0;
  this->Point1[2] = .0;

  this->Point2[0] = .5;
  this->Point2[1] = .0;
  this->Point2[2] = .0;

  this->Points = nullptr;

  this->Resolution = (res < 1 ? 1 : res);
  this->OutputPointsPrecision = SINGLE_PRECISION;
  this->UseRegularRefinement = true;
  this->SetNumberOfInputPorts(0);
}

// ----------------------------------------------------------------------
svtkLineSource::~svtkLineSource()
{
  this->SetPoints(nullptr);
}

// ----------------------------------------------------------------------
void svtkLineSource::SetNumberOfRefinementRatios(int val)
{
  if (val < 0)
  {
    svtkErrorMacro("Value cannot be negative: " << val);
  }
  else if (static_cast<int>(this->RefinementRatios.size()) != val)
  {
    this->RefinementRatios.resize(val);
    this->Modified();
  }
}

// ----------------------------------------------------------------------
void svtkLineSource::SetRefinementRatio(int index, double value)
{
  if (index >= 0 && index < static_cast<int>(this->RefinementRatios.size()))
  {
    if (this->RefinementRatios[index] != value)
    {
      this->RefinementRatios[index] = value;
      this->Modified();
    }
  }
  else
  {
    svtkErrorMacro("Invalid index: " << index);
  }
}

// ----------------------------------------------------------------------
int svtkLineSource::GetNumberOfRefinementRatios()
{
  return static_cast<int>(this->RefinementRatios.size());
}

// ----------------------------------------------------------------------
double svtkLineSource::GetRefinementRatio(int index)
{
  if (index >= 0 && index < static_cast<int>(this->RefinementRatios.size()))
  {
    return this->RefinementRatios[index];
  }
  svtkErrorMacro("Invalid index: " << index);
  return 0.0;
}

// ----------------------------------------------------------------------
int svtkLineSource::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);
  return 1;
}

// ----------------------------------------------------------------------
int svtkLineSource::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** svtkNotUsed(inputVector), svtkInformationVector* outputVector)
{
  // Reject meaningless parameterizations
  const svtkIdType nSegments = this->Points ? this->Points->GetNumberOfPoints() - 1 : 1;
  if (nSegments < 1)
  {
    svtkWarningMacro(<< "Cannot define a broken line with given input.");
    return 0;
  }

  // get the info object
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  if (outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) > 0)
  {
    // we'll only produce data for piece 0, and produce empty datasets on
    // others since splitting a line source into pieces is generally not what's
    // expected.
    return 1;
  }

  // get the output
  svtkPolyData* output = svtkPolyData::GetData(outInfo);

  // This is a vector giving the positions of intermediate points. Thus, if empty, only the
  // end points for each line segment are generated.
  std::vector<double> refinements;
  if (this->UseRegularRefinement)
  {
    assert(this->Resolution >= 1);
    refinements.reserve(this->Resolution + 1);
    for (int cc = 0; cc < this->Resolution; ++cc)
    {
      refinements.push_back(static_cast<double>(cc) / this->Resolution);
    }
    refinements.push_back(1.0);
  }
  else
  {
    refinements = this->RefinementRatios;
  }

  svtkSmartPointer<svtkPoints> pts = this->Points;
  if (this->Points == nullptr)
  {
    // using end points.
    pts = svtkSmartPointer<svtkPoints>::New();
    pts->SetDataType(SVTK_DOUBLE);
    pts->SetNumberOfPoints(2);
    pts->SetPoint(0, this->Point1);
    pts->SetPoint(1, this->Point2);
  }

  // Create and allocate lines
  svtkIdType numPts = nSegments * static_cast<svtkIdType>(refinements.size());

  // Create and allocate points
  svtkNew<svtkPoints> newPoints;

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPoints->SetDataType(SVTK_DOUBLE);
  }
  else
  {
    newPoints->SetDataType(SVTK_FLOAT);
  }
  newPoints->Allocate(numPts);

  // Generate points

  // Point index offset for fast insertion
  svtkIdType offset = 0;

  // Iterate over segments
  for (svtkIdType seg = 0; seg < nSegments; ++seg)
  {
    assert((seg + 1) < pts->GetNumberOfPoints());

    // Get coordinates of endpoints
    svtkVector3d point1, point2;

    pts->GetPoint(seg, point1.GetData());
    pts->GetPoint(seg + 1, point2.GetData());

    // Calculate segment vector
    const svtkVector3d v = point2 - point1;

    // Generate points along segment
    for (size_t i = 0; i < refinements.size(); ++i)
    {
      if (seg > 0 && i == 0 && refinements.front() == 0.0 && refinements.back() == 1.0)
      {
        // skip adding first point in the segment if it is same as the last point
        // from previously added segment.
        continue;
      }
      const svtkVector3d pt = point1 + refinements[i] * v;
      newPoints->InsertPoint(offset, pt.GetData());
      ++offset;
    }
  } // seg

  // update number of points estimate.
  numPts = offset;

  //  Generate lines
  svtkNew<svtkCellArray> newLines;
  newLines->AllocateEstimate(1, numPts);
  newLines->InsertNextCell(numPts);
  for (svtkIdType i = 0; i < numPts; ++i)
  {
    newLines->InsertCellPoint(i);
  }

  // Generate texture coordinates
  svtkNew<svtkFloatArray> newTCoords;
  newTCoords->SetNumberOfComponents(2);
  newTCoords->SetNumberOfTuples(numPts);
  newTCoords->SetName("Texture Coordinates");
  newTCoords->FillValue(0.0f);

  float length_sum = 0.0f;
  for (svtkIdType cc = 1; cc < numPts; ++cc)
  {
    svtkVector3d p1, p2;
    newPoints->GetPoint(cc - 1, p1.GetData());
    newPoints->GetPoint(cc, p2.GetData());

    length_sum += static_cast<float>((p2 - p1).Norm());
    newTCoords->SetTypedComponent(cc, 0, length_sum);
  }

  // now normalize the tcoord
  if (length_sum)
  {
    for (svtkIdType cc = 1; cc < numPts; ++cc)
    {
      newTCoords->SetTypedComponent(cc, 0, newTCoords->GetTypedComponent(cc, 0) / length_sum);
    }
  }

  // Update ourselves and release memory
  output->SetPoints(newPoints);
  output->GetPointData()->SetTCoords(newTCoords);
  output->SetLines(newLines);
  return 1;
}

// ----------------------------------------------------------------------
void svtkLineSource::SetPoint1(float point1f[3])
{
  double point1d[3];
  point1d[0] = point1f[0];
  point1d[1] = point1f[1];
  point1d[2] = point1f[2];
  SetPoint1(point1d);
}

// ----------------------------------------------------------------------
void svtkLineSource::SetPoint2(float point2f[3])
{
  double point2d[3];
  point2d[0] = point2f[0];
  point2d[1] = point2f[1];
  point2d[2] = point2f[2];
  SetPoint2(point2d);
}

// ----------------------------------------------------------------------
void svtkLineSource::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Resolution: " << this->Resolution << "\n";

  os << indent << "Point 1: (" << this->Point1[0] << ", " << this->Point1[1] << ", "
     << this->Point1[2] << ")\n";

  os << indent << "Point 2: (" << this->Point2[0] << ", " << this->Point2[1] << ", "
     << this->Point2[2] << ")\n";

  os << indent << "Points: ";
  if (this->Points)
  {
    this->Points->PrintSelf(os, indent);
  }
  else
  {
    os << "(none)" << endl;
  }
  os << indent << "UseRegularRefinement: " << this->UseRegularRefinement << endl;
  os << indent << "RefinementRatios: [";
  for (const auto& r : this->RefinementRatios)
  {
    os << r << " ";
  }
  os << "]" << endl;

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
