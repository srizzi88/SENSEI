/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWarpScalar.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkWarpScalar.h"

#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDataSetAttributes.h"
#include "svtkImageData.h"
#include "svtkImageDataToPointSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridToPointSet.h"

#include "svtkNew.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkWarpScalar);

//----------------------------------------------------------------------------
svtkWarpScalar::svtkWarpScalar()
{
  this->ScaleFactor = 1.0;
  this->UseNormal = 0;
  this->Normal[0] = 0.0;
  this->Normal[1] = 0.0;
  this->Normal[2] = 1.0;
  this->XYPlane = 0;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
}

//----------------------------------------------------------------------------
svtkWarpScalar::~svtkWarpScalar() = default;

//----------------------------------------------------------------------------
double* svtkWarpScalar::DataNormal(svtkIdType id, svtkDataArray* normals)
{
  return normals->GetTuple(id);
}

//----------------------------------------------------------------------------
double* svtkWarpScalar::InstanceNormal(svtkIdType svtkNotUsed(id), svtkDataArray* svtkNotUsed(normals))
{
  return this->Normal;
}

//----------------------------------------------------------------------------
double* svtkWarpScalar::ZNormal(svtkIdType svtkNotUsed(id), svtkDataArray* svtkNotUsed(normals))
{
  static double zNormal[3] = { 0.0, 0.0, 1.0 };
  return zNormal;
}

//----------------------------------------------------------------------------
int svtkWarpScalar::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}

//----------------------------------------------------------------------------
int svtkWarpScalar::RequestDataObject(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkImageData* inImage = svtkImageData::GetData(inputVector[0]);
  svtkRectilinearGrid* inRect = svtkRectilinearGrid::GetData(inputVector[0]);

  if (inImage || inRect)
  {
    svtkStructuredGrid* output = svtkStructuredGrid::GetData(outputVector);
    if (!output)
    {
      svtkNew<svtkStructuredGrid> newOutput;
      outputVector->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), newOutput);
    }
    return 1;
  }
  else
  {
    return this->Superclass::RequestDataObject(request, inputVector, outputVector);
  }
}

//----------------------------------------------------------------------------
int svtkWarpScalar::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkSmartPointer<svtkPointSet> input = svtkPointSet::GetData(inputVector[0]);
  svtkPointSet* output = svtkPointSet::GetData(outputVector);

  if (!input)
  {
    // Try converting image data.
    svtkImageData* inImage = svtkImageData::GetData(inputVector[0]);
    if (inImage)
    {
      svtkNew<svtkImageDataToPointSet> image2points;
      image2points->SetInputData(inImage);
      image2points->Update();
      input = image2points->GetOutput();
    }
  }

  if (!input)
  {
    // Try converting rectilinear grid.
    svtkRectilinearGrid* inRect = svtkRectilinearGrid::GetData(inputVector[0]);
    if (inRect)
    {
      svtkNew<svtkRectilinearGridToPointSet> rect2points;
      rect2points->SetInputData(inRect);
      rect2points->Update();
      input = rect2points->GetOutput();
    }
  }

  if (!input)
  {
    svtkErrorMacro(<< "Invalid or missing input");
    return 0;
  }

  svtkPoints* inPts;
  svtkDataArray* inNormals;
  svtkDataArray* inScalars;
  svtkPoints* newPts;
  svtkPointData* pd;
  int i;
  svtkIdType ptId, numPts;
  double x[3], *n, s, newX[3];

  svtkDebugMacro(<< "Warping data with scalars");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  inPts = input->GetPoints();
  pd = input->GetPointData();
  inNormals = pd->GetNormals();

  inScalars = this->GetInputArrayToProcess(0, inputVector);
  if (!inPts || !inScalars)
  {
    svtkDebugMacro(<< "No data to warp");
    return 1;
  }

  numPts = inPts->GetNumberOfPoints();

  if (inNormals && !this->UseNormal)
  {
    this->PointNormal = &svtkWarpScalar::DataNormal;
    svtkDebugMacro(<< "Using data normals");
  }
  else if (this->XYPlane)
  {
    this->PointNormal = &svtkWarpScalar::ZNormal;
    svtkDebugMacro(<< "Using x-y plane normal");
  }
  else
  {
    this->PointNormal = &svtkWarpScalar::InstanceNormal;
    svtkDebugMacro(<< "Using Normal instance variable");
  }

  newPts = svtkPoints::New();
  newPts->SetNumberOfPoints(numPts);

  // Loop over all points, adjusting locations
  //
  for (ptId = 0; ptId < numPts; ptId++)
  {
    if (!(ptId % 10000))
    {
      this->UpdateProgress((double)ptId / numPts);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    inPts->GetPoint(ptId, x);
    n = (this->*(this->PointNormal))(ptId, inNormals);
    if (this->XYPlane)
    {
      s = x[2];
    }
    else
    {
      s = inScalars->GetComponent(ptId, 0);
    }
    for (i = 0; i < 3; i++)
    {
      newX[i] = x[i] + this->ScaleFactor * s * n[i];
    }
    newPts->SetPoint(ptId, newX);
  }

  // Update ourselves and release memory
  //
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->CopyNormalsOff(); // distorted geometry
  output->GetCellData()->PassData(input->GetCellData());

  output->SetPoints(newPts);
  newPts->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void svtkWarpScalar::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
  os << indent << "Use Normal: " << (this->UseNormal ? "On\n" : "Off\n");
  os << indent << "Normal: (" << this->Normal[0] << ", " << this->Normal[1] << ", "
     << this->Normal[2] << ")\n";
  os << indent << "XY Plane: " << (this->XYPlane ? "On\n" : "Off\n");
}
