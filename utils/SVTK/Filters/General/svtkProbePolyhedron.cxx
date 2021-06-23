/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProbePolyhedron.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProbePolyhedron.h"

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

svtkStandardNewMacro(svtkProbePolyhedron);

//----------------------------------------------------------------------------
svtkProbePolyhedron::svtkProbePolyhedron()
{
  this->ProbePointData = 1;
  this->ProbeCellData = 0;

  this->SetNumberOfInputPorts(2);
}

//----------------------------------------------------------------------------
svtkProbePolyhedron::~svtkProbePolyhedron() = default;

//----------------------------------------------------------------------------
void svtkProbePolyhedron::SetSourceConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
void svtkProbePolyhedron::SetSourceData(svtkPolyData* input)
{
  this->SetInputData(1, input);
}

//----------------------------------------------------------------------------
svtkPolyData* svtkProbePolyhedron::GetSource()
{
  if (this->GetNumberOfInputConnections(1) < 1)
  {
    return nullptr;
  }

  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
int svtkProbePolyhedron::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* source = svtkPolyData::SafeDownCast(sourceInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* output = svtkDataSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!source)
  {
    return 0;
  }

  // Make sure that the mesh consists of triangles. Bail out if not.
  svtkIdType numPolys = source->GetNumberOfPolys();
  svtkCellArray* srcPolys = source->GetPolys();

  if (!numPolys || !srcPolys)
  {
    svtkErrorMacro("Probe polyhedron filter requires a non-empty mesh");
    return 0;
  }

  // Set up attribute interpolation. The input structure is passed to the
  // output.
  svtkIdType numInputPts = input->GetNumberOfPoints();
  svtkIdType numSrcPts = source->GetNumberOfPoints();
  svtkIdType numInputCells = input->GetNumberOfCells();
  output->CopyStructure(input);
  svtkPointData* outPD = output->GetPointData();
  svtkCellData* outCD = output->GetCellData();
  svtkPointData* srcPD = source->GetPointData();
  outPD->InterpolateAllocate(srcPD, numInputPts, 1);
  outCD->InterpolateAllocate(srcPD, numInputCells, 1);

  // Okay probe the polyhedral mesh. Have to loop over all points and compute
  // each points interpolation weights. These weights are used to perform the
  // interpolation.
  svtkPoints* srcPts = source->GetPoints();
  svtkDoubleArray* weights = svtkDoubleArray::New();
  weights->SetNumberOfComponents(1);
  weights->SetNumberOfTuples(numSrcPts);
  double* wPtr = weights->GetPointer(0);

  // InterpolatePoint requires knowing which points to interpolate from.
  svtkIdType ptId, cellId;
  svtkIdList* srcIds = svtkIdList::New();
  srcIds->SetNumberOfIds(numSrcPts);
  for (ptId = 0; ptId < numSrcPts; ++ptId)
  {
    srcIds->SetId(ptId, ptId);
  }

  // Interpolate the point data (if requested)
  double x[3];
  int abort = 0;
  svtkIdType idx = 0, progressInterval = (numInputCells + numInputPts) / 10 + 1;
  if (this->ProbePointData)
  {
    for (ptId = 0; ptId < numInputPts && !abort; ++ptId, ++idx)
    {
      if (!(idx % progressInterval))
      {
        svtkDebugMacro(<< "Processing #" << idx);
        this->UpdateProgress(static_cast<double>(idx) / (numInputCells + numInputPts));
        abort = this->GetAbortExecute();
      }

      input->GetPoint(ptId, x);
      svtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeights(x, srcPts, srcPolys, wPtr);

      outPD->InterpolatePoint(srcPD, ptId, srcIds, wPtr);
    }
  }

  // Interpolate the cell data (if requested)
  // Compute point value at the cell's parametric center.
  if (this->ProbeCellData)
  {
    svtkCell* cell;
    int subId;
    double pcoords[3];
    x[0] = x[1] = x[2] = 0.0;

    for (cellId = 0; cellId < numInputCells && !abort; ++cellId, ++idx)
    {
      if (!(idx % progressInterval))
      {
        svtkDebugMacro(<< "Processing #" << idx);
        this->UpdateProgress(static_cast<double>(idx) / (numInputCells + numInputPts));
        abort = this->GetAbortExecute();
      }

      cell = input->GetCell(cellId);
      if (cell->GetCellType() != SVTK_EMPTY_CELL)
      {
        subId = cell->GetParametricCenter(pcoords);
        cell->EvaluateLocation(subId, pcoords, x, wPtr);
      }
      svtkMeanValueCoordinatesInterpolator::ComputeInterpolationWeights(x, srcPts, srcPolys, wPtr);

      outCD->InterpolatePoint(srcPD, cellId, srcIds, wPtr);
    }
  }

  // clean up
  srcIds->Delete();
  weights->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int svtkProbePolyhedron::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* sourceInfo = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  outInfo->CopyEntry(sourceInfo, svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->CopyEntry(sourceInfo, svtkStreamingDemandDrivenPipeline::TIME_RANGE());

  outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(),
    inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT()), 6);

  return 1;
}

//----------------------------------------------------------------------------
int svtkProbePolyhedron::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int usePiece = 0;

  // What ever happened to CopyUpdateExtent in svtkDataObject?
  // Copying both piece and extent could be bad.  Setting the piece
  // of a structured data set will affect the extent.
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());
  if (output &&
    (!strcmp(output->GetClassName(), "svtkUnstructuredGrid") ||
      !strcmp(output->GetClassName(), "svtkPolyData")))
  {
    usePiece = 1;
  }

  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  if (usePiece)
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()));
  }
  else
  {
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()), 6);
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkProbePolyhedron::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  svtkDataObject* source = this->GetSource();
  os << indent << "Source: " << source << "\n";

  os << indent << "Probe Point Data: " << (this->ProbePointData ? "true" : "false") << "\n";

  os << indent << "Probe Cell Data: " << (this->ProbeCellData ? "true" : "false") << "\n";
}
