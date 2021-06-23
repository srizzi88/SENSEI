/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWarpTo.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkWarpTo.h"

#include "svtkImageData.h"
#include "svtkImageDataToPointSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridToPointSet.h"
#include "svtkStructuredGrid.h"

#include "svtkNew.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkWarpTo);

svtkWarpTo::svtkWarpTo()
{
  this->ScaleFactor = 0.5;
  this->Absolute = 0;
  this->Position[0] = this->Position[1] = this->Position[2] = 0.0;
}

int svtkWarpTo::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkRectilinearGrid");
  return 1;
}

int svtkWarpTo::RequestDataObject(
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

int svtkWarpTo::RequestData(svtkInformation* svtkNotUsed(request), svtkInformationVector** inputVector,
  svtkInformationVector* outputVector)
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
  svtkPoints* newPts;
  svtkIdType ptId, numPts;
  int i;
  double x[3], newX[3];
  double mag;
  double minMag = 0;

  svtkDebugMacro(<< "Warping data to a point");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  inPts = input->GetPoints();

  if (!inPts)
  {
    svtkErrorMacro(<< "No input data");
    return 1;
  }

  numPts = inPts->GetNumberOfPoints();
  newPts = svtkPoints::New();
  newPts->SetNumberOfPoints(numPts);

  if (this->Absolute)
  {
    minMag = 1.0e10;
    for (ptId = 0; ptId < numPts; ptId++)
    {
      inPts->GetPoint(ptId, x);
      mag = sqrt(svtkMath::Distance2BetweenPoints(this->Position, x));
      if (mag < minMag)
      {
        minMag = mag;
      }
    }
  }

  //
  // Loop over all points, adjusting locations
  //
  for (ptId = 0; ptId < numPts; ptId++)
  {
    inPts->GetPoint(ptId, x);
    if (this->Absolute)
    {
      mag = sqrt(svtkMath::Distance2BetweenPoints(this->Position, x));
      for (i = 0; i < 3; i++)
      {
        newX[i] =
          this->ScaleFactor * (this->Position[i] + minMag * (x[i] - this->Position[i]) / mag) +
          (1.0 - this->ScaleFactor) * x[i];
      }
    }
    else
    {
      for (i = 0; i < 3; i++)
      {
        newX[i] = (1.0 - this->ScaleFactor) * x[i] + this->ScaleFactor * this->Position[i];
      }
    }
    newPts->SetPoint(ptId, newX);
  }
  //
  // Update ourselves and release memory
  //
  output->GetPointData()->CopyNormalsOff(); // distorted geometry
  output->GetPointData()->PassData(input->GetPointData());

  output->SetPoints(newPts);
  newPts->Delete();

  return 1;
}

void svtkWarpTo::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Absolute: " << (this->Absolute ? "On\n" : "Off\n");

  os << indent << "Position: (" << this->Position[0] << ", " << this->Position[1] << ", "
     << this->Position[2] << ")\n";
  os << indent << "Scale Factor: " << this->ScaleFactor << "\n";
}
