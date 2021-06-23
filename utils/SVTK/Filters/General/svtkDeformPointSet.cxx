/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDeformPointSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDeformPointSet.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMeanValueCoordinatesInterpolator.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"

svtkStandardNewMacro(svtkDeformPointSet);

//----------------------------------------------------------------------------
svtkDeformPointSet::svtkDeformPointSet()
{
  this->InitializeWeights = 0;
  this->SetNumberOfInputPorts(2);

  // Prepare cached data
  this->InitialNumberOfControlMeshPoints = 0;
  this->InitialNumberOfControlMeshCells = 0;
  this->InitialNumberOfPointSetPoints = 0;
  this->InitialNumberOfPointSetCells = 0;
  this->Weights = svtkSmartPointer<svtkDoubleArray>::New();
}

//----------------------------------------------------------------------------
svtkDeformPointSet::~svtkDeformPointSet() = default;

//----------------------------------------------------------------------------
void svtkDeformPointSet::SetControlMeshConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void svtkDeformPointSet::SetControlMeshData(svtkPolyData* input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
svtkPolyData* svtkDeformPointSet::GetControlMeshData()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return svtkPolyData::SafeDownCast(this->GetInputDataObject(1, 0));
}

//----------------------------------------------------------------------------
int svtkDeformPointSet::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* cmeshInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPointSet* output = svtkPointSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!cmeshInfo)
  {
    return 0;
  }
  svtkPolyData* cmesh = svtkPolyData::SafeDownCast(cmeshInfo->Get(svtkDataObject::DATA_OBJECT()));
  if (!cmesh)
  {
    return 0;
  }

  // Pass the input attributes to the output
  output->CopyStructure(input);
  output->GetPointData()->PassData(input->GetPointData());
  output->GetCellData()->PassData(input->GetCellData());

  // Gather initial information
  svtkIdType numberOfPointSetPoints = input->GetNumberOfPoints();
  svtkIdType numberOfPointSetCells = input->GetNumberOfCells();
  svtkPoints* inPts = input->GetPoints();
  svtkPoints* cmeshPts = cmesh->GetPoints();
  if (!inPts || !cmeshPts)
  {
    return 0;
  }
  svtkCellArray* cmeshPolys = cmesh->GetPolys();
  svtkIdType numberOfControlMeshPoints = cmeshPts->GetNumberOfPoints();
  svtkIdType numberOfControlMeshCells = cmeshPolys->GetNumberOfCells();
  svtkIdType numTriangles = cmeshPolys->GetNumberOfConnectivityIds() / 3;
  if (numTriangles != numberOfControlMeshCells)
  {
    svtkErrorMacro("Control mesh must be a closed, manifold triangular mesh");
    return 0;
  }

  // We will be modifying the points
  svtkPoints* outPts = input->GetPoints()->NewInstance();
  outPts->SetDataType(input->GetPoints()->GetDataType());
  outPts->SetNumberOfPoints(numberOfPointSetPoints);
  output->SetPoints(outPts);

  // Start by determining whether weights must be computed or not
  int abort = 0;
  svtkIdType progressInterval = (numberOfPointSetPoints / 10 + 1);
  int workLoad = 1;
  double x[3], *weights;
  svtkIdType ptId, pid;
  if (this->InitializeWeights ||
    this->InitialNumberOfControlMeshPoints != numberOfControlMeshPoints ||
    this->InitialNumberOfControlMeshCells != numberOfControlMeshCells ||
    this->InitialNumberOfPointSetPoints != numberOfPointSetPoints ||
    this->InitialNumberOfPointSetCells != numberOfPointSetCells)
  {
    workLoad = 2;
    // reallocate the weights
    this->Weights->Reset();
    this->Weights->SetNumberOfComponents(numberOfControlMeshPoints);
    this->Weights->SetNumberOfTuples(numberOfPointSetPoints);

    // compute the interpolation weights
    for (ptId = 0; ptId < numberOfPointSetPoints && !abort; ++ptId)
    {
      if (!(ptId % progressInterval))
      {
        svtkDebugMacro(<< "Processing #" << ptId);
        this->UpdateProgress(ptId / (workLoad * numberOfPointSetPoints));
        abort = this->GetAbortExecute();
      }

      inPts->GetPoint(ptId, x);
      weights = this->Weights->GetPointer(ptId * numberOfControlMeshPoints);
      svtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeights(
        x, cmeshPts, cmeshPolys, weights);
    }

    // prepare for next execution
    this->InitializeWeights = 0;
    this->InitialNumberOfControlMeshPoints = numberOfControlMeshPoints;
    this->InitialNumberOfControlMeshCells = numberOfControlMeshCells;
    this->InitialNumberOfPointSetPoints = numberOfPointSetPoints;
    this->InitialNumberOfPointSetCells = numberOfPointSetCells;
  }

  // Okay weights are computed, now interpolate
  double xx[3];
  for (ptId = 0; ptId < numberOfPointSetPoints && !abort; ++ptId)
  {
    if (!(ptId % progressInterval))
    {
      svtkDebugMacro(<< "Processing #" << ptId);
      this->UpdateProgress(ptId / (workLoad * numberOfPointSetPoints));
      abort = this->GetAbortExecute();
    }

    weights = this->Weights->GetPointer(ptId * numberOfControlMeshPoints);

    x[0] = x[1] = x[2] = 0.0;
    for (pid = 0; pid < numberOfControlMeshPoints; ++pid)
    {
      cmeshPts->GetPoint(pid, xx);
      x[0] += weights[pid] * xx[0];
      x[1] += weights[pid] * xx[1];
      x[2] += weights[pid] * xx[2];
    }
    outPts->SetPoint(ptId, x);
  }

  // clean up and get out
  outPts->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void svtkDeformPointSet::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  svtkDataObject* cmesh = this->GetControlMeshData();
  os << indent << "Control Mesh: " << cmesh << "\n";

  os << indent << "Initialize Weights: " << (this->InitializeWeights ? "true" : "false") << "\n";
}
