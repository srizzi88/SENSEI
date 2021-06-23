/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractTensorComponents.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractTensorComponents.h"

#include "svtkDataSet.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

svtkStandardNewMacro(svtkExtractTensorComponents);

//---------------------------------------------------------------------------
// Construct object to extract nothing and to not pass tensor data
// through the pipeline.
svtkExtractTensorComponents::svtkExtractTensorComponents()
{
  this->PassTensorsToOutput = 0;

  this->ExtractScalars = 0;
  this->ExtractVectors = 0;
  this->ExtractNormals = 0;
  this->ExtractTCoords = 0;

  this->ScalarMode = SVTK_EXTRACT_COMPONENT;
  this->ScalarComponents[0] = this->ScalarComponents[1] = 0;

  this->VectorComponents[0] = 0;
  this->VectorComponents[1] = 0;
  this->VectorComponents[2] = 1;
  this->VectorComponents[3] = 0;
  this->VectorComponents[4] = 2;
  this->VectorComponents[5] = 0;

  this->NormalizeNormals = 1;
  this->NormalComponents[0] = 0;
  this->NormalComponents[1] = 1;
  this->NormalComponents[2] = 1;
  this->NormalComponents[3] = 1;
  this->NormalComponents[4] = 2;
  this->NormalComponents[5] = 1;

  this->NumberOfTCoords = 2;
  this->TCoordComponents[0] = 0;
  this->TCoordComponents[1] = 2;
  this->TCoordComponents[2] = 1;
  this->TCoordComponents[3] = 2;
  this->TCoordComponents[4] = 2;
  this->TCoordComponents[5] = 2;
}

//---------------------------------------------------------------------------
// Extract data from tensors.
//
int svtkExtractTensorComponents::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDataArray* inTensors;
  double tensor[9];
  svtkPointData* pd = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  double s = 0.0;
  double v[3];
  svtkFloatArray* newScalars = nullptr;
  svtkFloatArray* newVectors = nullptr;
  svtkFloatArray* newNormals = nullptr;
  svtkFloatArray* newTCoords = nullptr;
  svtkIdType ptId, numPts;
  double sx, sy, sz, txy, tyz, txz;

  // Initialize
  //
  svtkDebugMacro(<< "Extracting vector components!");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  inTensors = pd->GetTensors();
  numPts = input->GetNumberOfPoints();

  if (!inTensors || numPts < 1)
  {
    svtkErrorMacro(<< "No data to extract!");
    return 1;
  }

  if (!this->ExtractScalars && !this->ExtractVectors && !this->ExtractNormals &&
    !this->ExtractTCoords)
  {
    svtkWarningMacro(<< "No data is being extracted");
  }

  outPD->CopyAllOn();
  if (!this->PassTensorsToOutput)
  {
    outPD->CopyTensorsOff();
  }
  if (this->ExtractScalars)
  {
    outPD->CopyScalarsOff();
    newScalars = svtkFloatArray::New();
    newScalars->SetNumberOfTuples(numPts);
  }
  if (this->ExtractVectors)
  {
    outPD->CopyVectorsOff();
    newVectors = svtkFloatArray::New();
    newVectors->SetNumberOfComponents(3);
    newVectors->SetNumberOfTuples(numPts);
  }
  if (this->ExtractNormals)
  {
    outPD->CopyNormalsOff();
    newNormals = svtkFloatArray::New();
    newNormals->SetNumberOfComponents(3);
    newNormals->SetNumberOfTuples(numPts);
  }
  if (this->ExtractTCoords)
  {
    outPD->CopyTCoordsOff();
    newTCoords = svtkFloatArray::New();
    newTCoords->SetNumberOfComponents(2);
    newTCoords->SetNumberOfTuples(numPts);
  }
  outPD->PassData(pd);

  // Loop over all points extracting components of tensor
  //
  for (ptId = 0; ptId < numPts; ptId++)
  {
    inTensors->GetTuple(ptId, tensor);
    if (inTensors->GetNumberOfComponents() == 6)
    {
      svtkMath::TensorFromSymmetricTensor(tensor);
    }

    if (this->ExtractScalars)
    {
      if (this->ScalarMode == SVTK_EXTRACT_EFFECTIVE_STRESS)
      {
        sx = tensor[0];
        sy = tensor[4];
        sz = tensor[8];
        txy = tensor[3];
        tyz = tensor[7];
        txz = tensor[6];

        s = sqrt(0.16666667 *
          ((sx - sy) * (sx - sy) + (sy - sz) * (sy - sz) + (sz - sx) * (sz - sx) +
            6.0 * (txy * txy + tyz * tyz + txz * txz)));
      }

      else if (this->ScalarMode == SVTK_EXTRACT_COMPONENT)
      {
        s = tensor[this->ScalarComponents[0] + 3 * this->ScalarComponents[1]];
      }

      else // SVTK_EXTRACT_EFFECTIVE_DETERMINANT
      {
        s = tensor[0] * tensor[4] * tensor[8] - tensor[0] * tensor[5] * tensor[7] -
          tensor[1] * tensor[3] * tensor[8] + tensor[1] * tensor[5] * tensor[6] +
          tensor[2] * tensor[3] * tensor[7] - tensor[2] * tensor[4] * tensor[6];
      }
      newScalars->SetTuple(ptId, &s);
    } // if extract scalars

    if (this->ExtractVectors)
    {
      v[0] = tensor[this->VectorComponents[0] + 3 * this->VectorComponents[1]];
      v[1] = tensor[this->VectorComponents[2] + 3 * this->VectorComponents[3]];
      v[2] = tensor[this->VectorComponents[4] + 3 * this->VectorComponents[5]];
      newVectors->SetTuple(ptId, v);
    }

    if (this->ExtractNormals)
    {
      v[0] = tensor[this->NormalComponents[0] + 3 * this->NormalComponents[1]];
      v[1] = tensor[this->NormalComponents[2] + 3 * this->NormalComponents[3]];
      v[2] = tensor[this->NormalComponents[4] + 3 * this->NormalComponents[5]];
      newNormals->SetTuple(ptId, v);
    }

    if (this->ExtractTCoords)
    {
      for (int i = 0; i < this->NumberOfTCoords; i++)
      {
        v[i] = tensor[this->TCoordComponents[2 * i] + 3 * this->TCoordComponents[2 * i + 1]];
      }
      newTCoords->SetTuple(ptId, v);
    }

  } // for all points

  // Send data to output
  //
  if (this->ExtractScalars)
  {
    int idx = outPD->AddArray(newScalars);
    outPD->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
    newScalars->Delete();
  }
  if (this->ExtractVectors)
  {
    outPD->SetVectors(newVectors);
    newVectors->Delete();
  }
  if (this->ExtractNormals)
  {
    outPD->SetNormals(newNormals);
    newNormals->Delete();
  }
  if (this->ExtractTCoords)
  {
    outPD->SetTCoords(newTCoords);
    newTCoords->Delete();
  }

  return 1;
}

//---------------------------------------------------------------------------
void svtkExtractTensorComponents::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Pass Tensors To Output: " << (this->PassTensorsToOutput ? "On\n" : "Off\n");

  os << indent << "Extract Scalars: " << (this->ExtractScalars ? "On\n" : "Off\n");

  os << indent << "Scalar Extraction Mode: ";

  if (this->ScalarMode == SVTK_EXTRACT_COMPONENT)
  {
    os << "SVTK_EXTRACT_COMPONENT\n";
  }
  else if (this->ScalarMode == SVTK_EXTRACT_EFFECTIVE_STRESS)
  {
    os << "SVTK_EXTRACT_EFFECTIVE_STRESS\n";
  }
  else
  {
    os << "SVTK_EXTRACT_DETERMINANT\n";
  }

  os << indent << "Scalar Components: \n";
  os << indent << "  (row,column): (" << this->ScalarComponents[0] << ", "
     << this->ScalarComponents[1] << ")\n";

  os << indent << "Extract Vectors: " << (this->ExtractVectors ? "On\n" : "Off\n");
  os << indent << "Vector Components: \n";
  os << indent << "  (row,column)0: (" << this->VectorComponents[0] << ", "
     << this->VectorComponents[1] << ")\n";
  os << indent << "  (row,column)1: (" << this->VectorComponents[2] << ", "
     << this->VectorComponents[3] << ")\n";
  os << indent << "  (row,column)2: (" << this->VectorComponents[4] << ", "
     << this->VectorComponents[5] << ")\n";

  os << indent << "Extract Normals: " << (this->ExtractNormals ? "On\n" : "Off\n");
  os << indent << "Normalize Normals: " << (this->NormalizeNormals ? "On\n" : "Off\n");
  os << indent << "Normal Components: \n";
  os << indent << "  (row,column)0: (" << this->NormalComponents[0] << ", "
     << this->NormalComponents[1] << ")\n";
  os << indent << "  (row,column)1: (" << this->NormalComponents[2] << ", "
     << this->NormalComponents[3] << ")\n";
  os << indent << "  (row,column)2: (" << this->NormalComponents[4] << ", "
     << this->NormalComponents[5] << ")\n";

  os << indent << "Extract TCoords: " << (this->ExtractTCoords ? "On\n" : "Off\n");
  os << indent << "Number Of TCoords: (" << this->NumberOfTCoords << ")\n";
  os << indent << "TCoord Components: \n";
  os << indent << "  (row,column)0: (" << this->TCoordComponents[0] << ", "
     << this->TCoordComponents[1] << ")\n";
  os << indent << "  (row,column)1: (" << this->TCoordComponents[2] << ", "
     << this->TCoordComponents[3] << ")\n";
  os << indent << "  (row,column)2: (" << this->TCoordComponents[4] << ", "
     << this->TCoordComponents[5] << ")\n";
}
