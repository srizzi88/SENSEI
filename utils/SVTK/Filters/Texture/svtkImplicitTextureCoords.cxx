/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImplicitTextureCoords.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkImplicitTextureCoords.h"

#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkImplicitFunction.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkImplicitTextureCoords);
svtkCxxSetObjectMacro(svtkImplicitTextureCoords, SFunction, svtkImplicitFunction);
svtkCxxSetObjectMacro(svtkImplicitTextureCoords, RFunction, svtkImplicitFunction);
svtkCxxSetObjectMacro(svtkImplicitTextureCoords, TFunction, svtkImplicitFunction);

// Create object with texture dimension=2 and no r-s-t implicit functions
// defined and FlipTexture turned off.
svtkImplicitTextureCoords::svtkImplicitTextureCoords()
{
  this->RFunction = nullptr;
  this->SFunction = nullptr;
  this->TFunction = nullptr;

  this->FlipTexture = 0;
}

svtkImplicitTextureCoords::~svtkImplicitTextureCoords()
{
  this->SetRFunction(nullptr);
  this->SetSFunction(nullptr);
  this->SetTFunction(nullptr);
}

int svtkImplicitTextureCoords::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType ptId, numPts;
  int tcoordDim;
  svtkFloatArray* newTCoords;
  double min[3], max[3], scale[3];
  double tCoord[3], tc[3], x[3];
  int i;

  // Initialize
  //
  svtkDebugMacro(<< "Generating texture coordinates from implicit functions...");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  if (((numPts = input->GetNumberOfPoints()) < 1))
  {
    svtkErrorMacro(<< "No input points!");
    return 1;
  }

  if (this->RFunction == nullptr)
  {
    svtkErrorMacro(<< "No implicit functions defined!");
    return 1;
  }

  tcoordDim = 1;
  if (this->SFunction != nullptr)
  {
    tcoordDim++;
    if (this->TFunction != nullptr)
    {
      tcoordDim++;
    }
  }
  //
  // Allocate
  //
  tCoord[0] = tCoord[1] = tCoord[2] = 0.0;

  newTCoords = svtkFloatArray::New();
  if (tcoordDim == 1) // force 2D map to be created
  {
    newTCoords->SetNumberOfComponents(2);
    newTCoords->Allocate(2 * numPts);
  }
  else
  {
    newTCoords->SetNumberOfComponents(tcoordDim);
    newTCoords->Allocate(tcoordDim * numPts);
  }
  //
  // Compute implicit function values -> insert as initial texture coordinate
  //
  for (i = 0; i < 3; i++) // initialize min/max values array
  {
    min[i] = SVTK_DOUBLE_MAX;
    max[i] = -SVTK_DOUBLE_MAX;
  }
  for (ptId = 0; ptId < numPts; ptId++) // compute texture coordinates
  {
    input->GetPoint(ptId, x);
    tCoord[0] = this->RFunction->FunctionValue(x);
    if (this->SFunction)
    {
      tCoord[1] = this->SFunction->FunctionValue(x);
    }
    if (this->TFunction)
    {
      tCoord[2] = this->TFunction->FunctionValue(x);
    }

    for (i = 0; i < tcoordDim; i++)
    {
      if (tCoord[i] < min[i])
      {
        min[i] = tCoord[i];
      }
      if (tCoord[i] > max[i])
      {
        max[i] = tCoord[i];
      }
    }

    newTCoords->InsertTuple(ptId, tCoord);
  }
  //
  // Scale and shift texture coordinates into (0,1) range, with 0.0 implicit
  // function value equal to texture coordinate value of 0.5
  //
  for (i = 0; i < tcoordDim; i++)
  {
    scale[i] = 1.0;
    if (max[i] > 0.0 && min[i] < 0.0) // have positive & negative numbers
    {
      if (max[i] > (-min[i]))
      {
        scale[i] = 0.499 / max[i]; // scale into 0.5->1
      }
      else
      {
        scale[i] = -0.499 / min[i]; // scale into 0->0.5
      }
    }
    else if (max[i] > 0.0) // have positive numbers only
    {
      scale[i] = 0.499 / max[i]; // scale into 0.5->1.0
    }
    else if (min[i] < 0.0) // have negative numbers only
    {
      scale[i] = -0.499 / min[i]; // scale into 0.0->0.5
    }
  }

  if (this->FlipTexture)
  {
    for (i = 0; i < tcoordDim; i++)
    {
      scale[i] *= (-1.0);
    }
  }
  for (ptId = 0; ptId < numPts; ptId++)
  {
    newTCoords->GetTuple(ptId, tc);
    for (i = 0; i < tcoordDim; i++)
    {
      tCoord[i] = 0.5 + scale[i] * tc[i];
    }
    newTCoords->InsertTuple(ptId, tCoord);
  }
  //
  // Update self
  //
  output->GetPointData()->CopyTCoordsOff();
  output->GetPointData()->PassData(input->GetPointData());

  output->GetPointData()->SetTCoords(newTCoords);
  newTCoords->Delete();

  return 1;
}

void svtkImplicitTextureCoords::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Flip Texture: " << this->FlipTexture << "\n";

  if (this->RFunction != nullptr)
  {
    if (this->SFunction != nullptr)
    {
      if (this->TFunction != nullptr)
      {
        os << indent << "R, S, and T Functions defined\n";
      }
    }
    else
    {
      os << indent << "R and S Functions defined\n";
    }
  }
  else
  {
    os << indent << "R Function defined\n";
  }
}
