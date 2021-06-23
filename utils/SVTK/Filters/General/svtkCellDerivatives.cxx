/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellDerivatives.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkCellDerivatives.h"

#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkDataSet.h"
#include "svtkDoubleArray.h"
#include "svtkGenericCell.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"

#include <cmath>

svtkStandardNewMacro(svtkCellDerivatives);

svtkCellDerivatives::svtkCellDerivatives()
{
  this->VectorMode = SVTK_VECTOR_MODE_COMPUTE_GRADIENT;
  this->TensorMode = SVTK_TENSOR_MODE_COMPUTE_GRADIENT;

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);

  // by default process active point vectors
  this->SetInputArrayToProcess(
    1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::VECTORS);
}

int svtkCellDerivatives::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPointData *pd = input->GetPointData(), *outPD = output->GetPointData();
  svtkCellData *cd = input->GetCellData(), *outCD = output->GetCellData();
  svtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
  svtkDataArray* inVectors = this->GetInputArrayToProcess(1, inputVector);
  svtkDoubleArray* outGradients = nullptr;
  svtkDoubleArray* outVorticity = nullptr;
  svtkDoubleArray* outTensors = nullptr;
  svtkIdType numCells = input->GetNumberOfCells();
  int computeScalarDerivs = 1, computeVectorDerivs = 1, computeVorticity = 1, subId;

  // Initialize
  svtkDebugMacro(<< "Computing cell derivatives");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  // Check input
  if (numCells < 1)
  {
    svtkErrorMacro("No cells to generate derivatives from");
    return 1;
  }

  // Figure out what to compute
  if (inScalars && this->VectorMode == SVTK_VECTOR_MODE_COMPUTE_GRADIENT)
  {
    outGradients = svtkDoubleArray::New();
    outGradients->SetNumberOfComponents(3);
    outGradients->SetNumberOfTuples(numCells);
    outGradients->SetName("ScalarGradient");
  }
  else
  {
    computeScalarDerivs = 0;
  }

  if (inVectors && this->VectorMode == SVTK_VECTOR_MODE_COMPUTE_VORTICITY)
  {
    outVorticity = svtkDoubleArray::New();
    outVorticity->SetNumberOfComponents(3);
    outVorticity->SetNumberOfTuples(numCells);
    outVorticity->SetName("Vorticity");
  }
  else
  {
    computeVorticity = 0;
  }

  if (inVectors &&
    (this->TensorMode == SVTK_TENSOR_MODE_COMPUTE_GRADIENT ||
      this->TensorMode == SVTK_TENSOR_MODE_COMPUTE_STRAIN ||
      this->TensorMode == SVTK_TENSOR_MODE_COMPUTE_GREEN_LAGRANGE_STRAIN))
  {
    outTensors = svtkDoubleArray::New();
    outTensors->SetNumberOfComponents(9);
    outTensors->SetNumberOfTuples(numCells);
    if (this->TensorMode == SVTK_TENSOR_MODE_COMPUTE_STRAIN)
    {
      outTensors->SetName("Strain");
    }
    else if (this->TensorMode == SVTK_TENSOR_MODE_COMPUTE_GREEN_LAGRANGE_STRAIN)
    {
      outTensors->SetName("GreenLagrangeStrain");
    }
    else
    {
      outTensors->SetName("VectorGradient");
    }
  }
  else
  {
    computeVectorDerivs = 0;
  }

  // If just passing data forget the loop
  if (computeScalarDerivs || computeVectorDerivs || computeVorticity)
  {
    double pcoords[3], derivs[9], tens[9], w[3], *scalars, *vectors;
    svtkGenericCell* cell = svtkGenericCell::New();
    svtkIdType cellId;
    svtkDoubleArray* cellScalars = svtkDoubleArray::New();
    if (computeScalarDerivs)
    {
      cellScalars->SetNumberOfComponents(inScalars->GetNumberOfComponents());
      cellScalars->Allocate(cellScalars->GetNumberOfComponents() * SVTK_CELL_SIZE);
      cellScalars->SetName("Scalars");
    }
    svtkDoubleArray* cellVectors = svtkDoubleArray::New();
    cellVectors->SetNumberOfComponents(3);
    cellVectors->Allocate(3 * SVTK_CELL_SIZE);
    cellVectors->SetName("Vectors");

    // Loop over all cells computing derivatives
    svtkIdType progressInterval = numCells / 20 + 1;
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (!(cellId % progressInterval))
      {
        svtkDebugMacro(<< "Computing cell #" << cellId);
        this->UpdateProgress(static_cast<double>(cellId) / numCells);
      }

      input->GetCell(cellId, cell);
      subId = cell->GetParametricCenter(pcoords);

      if (computeScalarDerivs)
      {
        inScalars->GetTuples(cell->PointIds, cellScalars);
        scalars = cellScalars->GetPointer(0);
        cell->Derivatives(subId, pcoords, scalars, 1, derivs);
        outGradients->SetTuple(cellId, derivs);
      }

      if (computeVectorDerivs || computeVorticity)
      {
        inVectors->GetTuples(cell->PointIds, cellVectors);
        vectors = cellVectors->GetPointer(0);
        cell->Derivatives(0, pcoords, vectors, 3, derivs);

        // Insert appropriate tensor
        if (this->TensorMode == SVTK_TENSOR_MODE_COMPUTE_GRADIENT)
        {
          outTensors->InsertTuple(cellId, derivs);
        }
        else if (this->TensorMode == SVTK_TENSOR_MODE_COMPUTE_STRAIN)
        {
          tens[0] = 0.5 * (derivs[0] + derivs[0]);
          tens[1] = 0.5 * (derivs[1] + derivs[3]);
          tens[2] = 0.5 * (derivs[2] + derivs[6]);
          tens[3] = 0.5 * (derivs[3] + derivs[1]);
          tens[4] = 0.5 * (derivs[4] + derivs[4]);
          tens[5] = 0.5 * (derivs[5] + derivs[7]);
          tens[6] = 0.5 * (derivs[6] + derivs[2]);
          tens[7] = 0.5 * (derivs[7] + derivs[5]);
          tens[8] = 0.5 * (derivs[8] + derivs[8]);

          outTensors->InsertTuple(cellId, tens);
        }
        else if (this->TensorMode == SVTK_TENSOR_MODE_COMPUTE_GREEN_LAGRANGE_STRAIN)
        {
          tens[0] = 0.5 *
            (derivs[0] + derivs[0] + derivs[0] * derivs[0] + derivs[3] * derivs[3] +
              derivs[6] * derivs[6]);
          tens[1] = 0.5 *
            (derivs[1] + derivs[3] + derivs[0] * derivs[1] + derivs[3] * derivs[4] +
              derivs[6] * derivs[7]);
          tens[2] = 0.5 *
            (derivs[2] + derivs[6] + derivs[0] * derivs[2] + derivs[3] * derivs[5] +
              derivs[6] * derivs[8]);
          tens[3] = 0.5 *
            (derivs[3] + derivs[1] + derivs[1] * derivs[0] + derivs[4] * derivs[3] +
              derivs[7] * derivs[6]);
          tens[4] = 0.5 *
            (derivs[4] + derivs[4] + derivs[1] * derivs[1] + derivs[4] * derivs[4] +
              derivs[7] * derivs[7]);
          tens[5] = 0.5 *
            (derivs[5] + derivs[7] + derivs[1] * derivs[2] + derivs[4] * derivs[5] +
              derivs[7] * derivs[8]);
          tens[6] = 0.5 *
            (derivs[6] + derivs[2] + derivs[2] * derivs[0] + derivs[5] * derivs[3] +
              derivs[8] * derivs[6]);
          tens[7] = 0.5 *
            (derivs[7] + derivs[5] + derivs[2] * derivs[1] + derivs[5] * derivs[4] +
              derivs[8] * derivs[7]);
          tens[8] = 0.5 *
            (derivs[8] + derivs[8] + derivs[2] * derivs[2] + derivs[5] * derivs[5] +
              derivs[8] * derivs[8]);

          outTensors->InsertTuple(cellId, tens);
        }
        else if (this->TensorMode == SVTK_TENSOR_MODE_PASS_TENSORS)
        {
          // do nothing.
        }

        if (computeVorticity)
        {
          w[0] = derivs[7] - derivs[5];
          w[1] = derivs[2] - derivs[6];
          w[2] = derivs[3] - derivs[1];
          outVorticity->SetTuple(cellId, w);
        }
      }
    } // for all cells

    cell->Delete();
    cellScalars->Delete();
    cellVectors->Delete();
  } // if something to compute

  // Pass appropriate data through to output
  outPD->PassData(pd);
  outCD->PassData(cd);
  if (outGradients)
  {
    outCD->SetVectors(outGradients);
    outGradients->Delete();
  }
  if (outVorticity)
  {
    outCD->SetVectors(outVorticity);
    outVorticity->Delete();
  }
  if (outTensors)
  {
    outCD->SetTensors(outTensors);
    outTensors->Delete();
  }

  return 1;
}

const char* svtkCellDerivatives::GetVectorModeAsString()
{
  if (this->VectorMode == SVTK_VECTOR_MODE_PASS_VECTORS)
  {
    return "PassVectors";
  }
  else if (this->VectorMode == SVTK_VECTOR_MODE_COMPUTE_GRADIENT)
  {
    return "ComputeGradient";
  }
  else // SVTK_VECTOR_MODE_COMPUTE_VORTICITY
  {
    return "ComputeVorticity";
  }
}

const char* svtkCellDerivatives::GetTensorModeAsString()
{
  if (this->TensorMode == SVTK_TENSOR_MODE_PASS_TENSORS)
  {
    return "PassTensors";
  }
  else if (this->TensorMode == SVTK_TENSOR_MODE_COMPUTE_GRADIENT)
  {
    return "ComputeGradient";
  }
  else if (this->TensorMode == SVTK_TENSOR_MODE_COMPUTE_STRAIN)
  {
    return "ComputeStrain";
  }
  else // SVTK_TENSOR_MODE_COMPUTE_GREEN_LAGRANGE_STRAIN
  {
    return "ComputeGreenLagrangeStrain";
  }
}

void svtkCellDerivatives::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Vector Mode: " << this->GetVectorModeAsString() << endl;

  os << indent << "Tensor Mode: " << this->GetTensorModeAsString() << endl;
}
