/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBrownianPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkBrownianPoints.h"

#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkFieldData.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkBrownianPoints);

svtkBrownianPoints::svtkBrownianPoints()
{
  this->MinimumSpeed = 0.0;
  this->MaximumSpeed = 1.0;
}

int svtkBrownianPoints::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType i, numPts;
  int j;
  svtkFloatArray* newVectors;
  double v[3], norm, speed;

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  svtkDebugMacro(<< "Executing Brownian filter");

  if (((numPts = input->GetNumberOfPoints()) < 1))
  {
    svtkDebugMacro(<< "No input!\n");
    return 1;
  }

  newVectors = svtkFloatArray::New();
  newVectors->SetNumberOfComponents(3);
  newVectors->SetNumberOfTuples(numPts);
  newVectors->SetName("BrownianVectors");

  // Check consistency of minimum and maximum speed
  //
  if (this->MinimumSpeed > this->MaximumSpeed)
  {
    svtkErrorMacro(<< " Minimum speed > maximum speed; reset to (0,1).");
    this->MinimumSpeed = 0.0;
    this->MaximumSpeed = 1.0;
  }

  int tenth = numPts / 10 + 1;
  for (i = 0; i < numPts; i++)
  {
    if (!(i % tenth))
    {
      this->UpdateProgress(static_cast<double>(i) / numPts);
      if (this->GetAbortExecute())
      {
        break;
      }
    }

    speed = svtkMath::Random(this->MinimumSpeed, this->MaximumSpeed);
    if (speed != 0.0)
    {
      for (j = 0; j < 3; j++)
      {
        v[j] = svtkMath::Random(-1.0, 1.0);
      }
      norm = svtkMath::Norm(v);
      for (j = 0; j < 3; j++)
      {
        v[j] *= (speed / norm);
      }
    }
    else
    {
      v[0] = 0.0;
      v[1] = 0.0;
      v[2] = 0.0;
    }

    newVectors->SetTuple(i, v);
  }

  // Update ourselves
  //
  output->GetPointData()->CopyVectorsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());
  output->GetFieldData()->PassData(input->GetFieldData());

  output->GetPointData()->SetVectors(newVectors);
  newVectors->Delete();

  return 1;
}

void svtkBrownianPoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Minimum Speed: " << this->MinimumSpeed << "\n";
  os << indent << "Maximum Speed: " << this->MaximumSpeed << "\n";
}
