/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRSliceFilter.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/

#include "svtkAMRSliceFilter.h"
#include "svtkAMRBox.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkDataArray.h"
#include "svtkDataObject.h"
#include "svtkInformation.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkParallelAMRUtilities.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredData.h"
#include "svtkTimerLog.h"
#include "svtkUniformGrid.h"
#include "svtkUniformGridAMRDataIterator.h"
#include "svtkUnsignedCharArray.h"

#include <algorithm>
#include <cassert>
#include <sstream>

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkAMRSliceFilter);

//-----------------------------------------------------------------------------
svtkAMRSliceFilter::svtkAMRSliceFilter()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->OffsetFromOrigin = 0.0;
  this->Normal = X_NORMAL;
  this->Controller = svtkMultiProcessController::GetGlobalController();
  this->MaxResolution = 1;
}

//-----------------------------------------------------------------------------
void svtkAMRSliceFilter::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
int svtkAMRSliceFilter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkOverlappingAMR");
  return 1;
}

//-----------------------------------------------------------------------------
int svtkAMRSliceFilter::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkOverlappingAMR");
  return 1;
}

//-----------------------------------------------------------------------------
bool svtkAMRSliceFilter::IsAMRData2D(svtkOverlappingAMR* input)
{
  assert("pre: Input AMR dataset is nullptr" && (input != nullptr));

  if (input->GetGridDescription() != SVTK_XYZ_GRID)
  {
    return true;
  }

  return false;
}

//-----------------------------------------------------------------------------
svtkPlane* svtkAMRSliceFilter::GetCutPlane(svtkOverlappingAMR* inp)
{
  assert("pre: AMR dataset should not be nullptr" && (inp != nullptr));

  svtkTimerLog::MarkStartEvent("AMRSlice::GetCutPlane");

  svtkPlane* pl = svtkPlane::New();

  // Get global bounds
  double minBounds[3];
  double maxBounds[3];
  inp->GetMin(minBounds);
  inp->GetMax(maxBounds);

  double porigin[3];
  for (int i = 0; i < 3; ++i)
    porigin[i] = minBounds[i];

  auto offset = svtkMath::ClampValue(
    this->OffsetFromOrigin, 0.0, maxBounds[this->Normal / 2] - minBounds[this->Normal / 2]);

  switch (this->Normal)
  {
    case X_NORMAL:
      pl->SetNormal(1.0, 0.0, 0.0);
      porigin[0] += offset;
      break;
    case Y_NORMAL:
      pl->SetNormal(0.0, 1.0, 0.0);
      porigin[1] += offset;
      break;
    case Z_NORMAL:
      pl->SetNormal(0.0, 0.0, 1.0);
      porigin[2] += offset;
      break;
    default:
      svtkErrorMacro("Undefined plane normal");
  }
  pl->SetOrigin(porigin);

  svtkTimerLog::MarkEndEvent("AMRSlice::GetCutPlane");
  return (pl);
}

//-----------------------------------------------------------------------------
svtkUniformGrid* svtkAMRSliceFilter::GetSlice(
  double origin[3], int* dims, double* gorigin, double* spacing)
{
  //  svtkTimerLog::MarkStartEvent( "AMRSlice::GetSliceForBlock" );

  svtkUniformGrid* slice = svtkUniformGrid::New();

  // Storage for dimensions of the 2-D slice grid & its origin
  int sliceDims[3];
  double sliceOrigin[3];

  switch (this->Normal)
  {
    case X_NORMAL: // -- YZ plane
      sliceDims[0] = 1;
      sliceDims[1] = dims[1];
      sliceDims[2] = dims[2];

      sliceOrigin[0] = origin[0];
      sliceOrigin[1] = gorigin[1];
      sliceOrigin[2] = gorigin[2];

      slice->SetOrigin(sliceOrigin);
      slice->SetDimensions(sliceDims);
      slice->SetSpacing(spacing);
      assert(slice->GetGridDescription() == SVTK_YZ_PLANE);
      break;
    case Y_NORMAL: // -- XZ plane
      sliceDims[0] = dims[0];
      sliceDims[1] = 1;
      sliceDims[2] = dims[2];

      sliceOrigin[0] = gorigin[0];
      sliceOrigin[1] = origin[1];
      sliceOrigin[2] = gorigin[2];

      slice->SetOrigin(sliceOrigin);
      slice->SetDimensions(sliceDims);
      slice->SetSpacing(spacing);
      assert(slice->GetGridDescription() == SVTK_XZ_PLANE);
      break;
    case Z_NORMAL: // -- XY plane
      sliceDims[0] = dims[0];
      sliceDims[1] = dims[1];
      sliceDims[2] = 1;

      sliceOrigin[0] = gorigin[0];
      sliceOrigin[1] = gorigin[1];
      sliceOrigin[2] = origin[2];

      slice->SetOrigin(sliceOrigin);
      slice->SetDimensions(sliceDims);
      slice->SetSpacing(spacing);
      assert(slice->GetGridDescription() == SVTK_XY_PLANE);
      break;
    default:
      svtkErrorMacro("Undefined normal");
  }

  //  svtkTimerLog::MarkEndEvent( "AMRSlice::GetSliceForBlock" );

  return (slice);
}

//-----------------------------------------------------------------------------
bool svtkAMRSliceFilter::PlaneIntersectsAMRBox(double plane[4], double bounds[6])
{
  bool lowPnt = false;
  bool highPnt = false;

  for (int i = 0; i < 8; ++i)
  {
    // Get box coordinates
    double x = (i & 1) ? bounds[1] : bounds[0];
    double y = (i & 2) ? bounds[3] : bounds[2];
    double z = (i & 4) ? bounds[5] : bounds[4];

    // Plug-in coordinates to the plane equation
    double v = plane[3] - plane[0] * x - plane[1] * y - plane[2] * z;

    if (v == 0.0) // Point is on a plane
    {
      return true;
    }

    if (v < 0.0)
    {
      lowPnt = true;
    }
    else
    {
      highPnt = true;
    }
    if (lowPnt && highPnt)
    {
      return true;
    }
  }
  return false;
}

//-----------------------------------------------------------------------------
void svtkAMRSliceFilter::ComputeAMRBlocksToLoad(svtkPlane* p, svtkOverlappingAMR* metadata)
{
  assert("pre: plane object is nullptr" && (p != nullptr));
  assert("pre: metadata object is nullptr" && (metadata != nullptr));

  svtkTimerLog::MarkStartEvent("AMRSlice::ComputeAMRBlocksToLoad");

  // Store A,B,C,D from the plane equation
  double plane[4];
  plane[0] = p->GetNormal()[0];
  plane[1] = p->GetNormal()[1];
  plane[2] = p->GetNormal()[2];
  plane[3] = p->GetNormal()[0] * p->GetOrigin()[0] + p->GetNormal()[1] * p->GetOrigin()[1] +
    p->GetNormal()[2] * p->GetOrigin()[2];

  svtkSmartPointer<svtkUniformGridAMRDataIterator> iter;
  iter.TakeReference(svtkUniformGridAMRDataIterator::SafeDownCast(metadata->NewIterator()));
  iter->SetSkipEmptyNodes(false);
  for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
  {
    if (iter->GetCurrentLevel() <= this->MaxResolution)
    {
      double* bounds = iter->GetCurrentMetaData()->Get(svtkDataObject::BOUNDING_BOX());
      if (this->PlaneIntersectsAMRBox(plane, bounds))
      {
        unsigned int amrGridIdx = iter->GetCurrentFlatIndex();
        this->BlocksToLoad.push_back(amrGridIdx);
      }
    }
  }
}

//-----------------------------------------------------------------------------
void svtkAMRSliceFilter::GetAMRSliceInPlane(
  svtkPlane* p, svtkOverlappingAMR* inp, svtkOverlappingAMR* out)
{
  assert("pre: input AMR dataset is nullptr" && (inp != nullptr));
  assert("pre: output AMR dataset is nullptr" && (out != nullptr));
  assert("pre: cut plane is nullptr" && (p != nullptr));

  int description = 0;
  switch (this->Normal)
  {
    case X_NORMAL:
      description = SVTK_YZ_PLANE;
      break;
    case Y_NORMAL:
      description = SVTK_XZ_PLANE;
      break;
    case Z_NORMAL:
      description = SVTK_XY_PLANE;
      break;
    default:
      svtkErrorMacro("Undefined normal");
  }

  if (this->BlocksToLoad.empty())
  {
    this->ComputeAMRBlocksToLoad(p, inp);
  }

  auto numLevels = svtkMath::Min(this->MaxResolution + 1, inp->GetNumberOfLevels());
  std::vector<int> blocksPerLevel(numLevels, 0);
  for (unsigned int i = 0; i < this->BlocksToLoad.size(); i++)
  {
    unsigned int flatIndex = this->BlocksToLoad[i];
    unsigned int level;
    unsigned int dataIdx;
    inp->GetLevelAndIndex(flatIndex, level, dataIdx);
    assert(level < numLevels);
    blocksPerLevel[level]++;
  }

  for (int i = static_cast<int>(blocksPerLevel.size() - 1); i >= 0; i--)
  {
    if (blocksPerLevel[i] == 0)
    {
      blocksPerLevel.pop_back();
    }
    else
    {
      break;
    }
  }

  out->Initialize(static_cast<int>(blocksPerLevel.size()), &blocksPerLevel[0]);
  out->SetGridDescription(description);
  out->SetOrigin(p->GetOrigin());
  svtkTimerLog::MarkStartEvent("AMRSlice::GetAMRSliceInPlane");

  std::vector<int> dataIndices(out->GetNumberOfLevels(), 0);
  for (unsigned int i = 0; i < this->BlocksToLoad.size(); i++)
  {
    int flatIndex = this->BlocksToLoad[i];
    unsigned int level;
    unsigned int dataIdx;
    inp->GetLevelAndIndex(flatIndex, level, dataIdx);
    svtkUniformGrid* grid = inp->GetDataSet(level, dataIdx);
    svtkUniformGrid* slice = nullptr;

    if (grid)
    {
      // Get the 3-D Grid dimensions
      int dims[3];
      grid->GetDimensions(dims);
      slice = this->GetSlice(p->GetOrigin(), dims, grid->GetOrigin(), grid->GetSpacing());
      assert("Dimension of slice must be 2-D" && (slice->GetDataDimension() == 2));
      assert("2-D slice is nullptr" && (slice != nullptr));
      this->GetSliceCellData(slice, grid);
      this->GetSlicePointData(slice, grid);
    }
    else
    {
      int dims[3];
      double spacing[3];
      double origin[3];
      inp->GetSpacing(level, spacing);
      inp->GetAMRBox(level, dataIdx).GetNumberOfNodes(dims);
      inp->GetOrigin(level, dataIdx, origin);
      slice = this->GetSlice(p->GetOrigin(), dims, origin, spacing);
    }

    svtkAMRBox box(slice->GetOrigin(), slice->GetDimensions(), slice->GetSpacing(), out->GetOrigin(),
      out->GetGridDescription());
    out->SetSpacing(level, slice->GetSpacing());
    out->SetAMRBox(level, dataIndices[level], box);
    if (grid)
    {
      out->SetDataSet(level, dataIndices[level], slice);
    }
    slice->Delete();
    dataIndices[level]++;
  }

  svtkTimerLog::MarkEndEvent("AMRSlice::GetAMRSliceInPlane");

  svtkTimerLog::MarkStartEvent("AMRSlice::Generate Blanking");
  svtkParallelAMRUtilities::BlankCells(out, this->Controller);
  svtkTimerLog::MarkEndEvent("AMRSlice::Generate Blanking");
}

//-----------------------------------------------------------------------------
void svtkAMRSliceFilter::ComputeCellCenter(svtkUniformGrid* ug, const int cellIdx, double centroid[3])
{
  assert("pre: Input grid is nullptr" && (ug != nullptr));
  assert(
    "pre: cell index out-of-bounds!" && ((cellIdx >= 0) && (cellIdx < ug->GetNumberOfCells())));

  svtkCell* myCell = ug->GetCell(cellIdx);
  assert("post: cell is nullptr" && (myCell != nullptr));

  double pCenter[3];
  double weights[8];
  int subId = myCell->GetParametricCenter(pCenter);
  myCell->EvaluateLocation(subId, pCenter, centroid, weights);
}

//-----------------------------------------------------------------------------
int svtkAMRSliceFilter::GetDonorCellIdx(double x[3], svtkUniformGrid* ug)
{
  const double* x0 = ug->GetOrigin();
  const double* h = ug->GetSpacing();
  int* dims = ug->GetDimensions();

  int ijk[3];
  for (int i = 0; i < 3; ++i)
  {
    ijk[i] = static_cast<int>(floor((x[i] - x0[i]) / h[i]));
    ijk[i] = svtkMath::ClampValue(ijk[i], 0, svtkMath::Max(1, dims[i] - 1) - 1);
  }

  return (svtkStructuredData::ComputeCellId(dims, ijk));
}

//----------------------------------------------------------------------------
int svtkAMRSliceFilter::GetDonorPointIdx(double x[3], svtkUniformGrid* ug)
{
  const double* x0 = ug->GetOrigin();
  const double* h = ug->GetSpacing();
  int* dims = ug->GetDimensions();

  int ijk[3];
  for (int i = 0; i < 3; ++i)
  {
    ijk[i] = std::floor((x[i] - x0[i]) / h[i]);
    ijk[i] = svtkMath::ClampValue(ijk[i], 0, svtkMath::Max(1, dims[i] - 1));
  }

  return svtkStructuredData::ComputePointId(dims, ijk);
}

//-----------------------------------------------------------------------------
void svtkAMRSliceFilter::GetSliceCellData(svtkUniformGrid* slice, svtkUniformGrid* grid3D)
{
  // STEP 1: Allocate data-structures
  svtkCellData* sourceCD = grid3D->GetCellData();
  svtkCellData* targetCD = slice->GetCellData();

  if (sourceCD->GetNumberOfArrays() == 0)
  {
    // nothing to do here
    return;
  }

  // NOTE:
  // Essentially the same as CopyAllocate
  // However CopyAllocate causes visual errors in the slice
  // if ghost cells are present
  svtkIdType numCells = slice->GetNumberOfCells();
  for (int arrayIdx = 0; arrayIdx < sourceCD->GetNumberOfArrays(); ++arrayIdx)
  {
    svtkDataArray* array = sourceCD->GetArray(arrayIdx)->NewInstance();
    array->Initialize();
    array->SetName(sourceCD->GetArray(arrayIdx)->GetName());
    array->SetNumberOfComponents(sourceCD->GetArray(arrayIdx)->GetNumberOfComponents());
    array->SetNumberOfTuples(numCells);
    targetCD->AddArray(array);
    svtkUnsignedCharArray* uca = svtkArrayDownCast<svtkUnsignedCharArray>(array);
    if (uca != nullptr && uca == slice->GetCellGhostArray())
    {
      // initiallize the ghost array
      memset(uca->WritePointer(0, numCells), 0, numCells);
    }
    array->Delete();
  } // END for all arrays

  // STEP 2: Fill in slice data-arrays
  for (int cellIdx = 0; cellIdx < numCells; ++cellIdx)
  {
    double probePnt[3];
    this->ComputeCellCenter(slice, cellIdx, probePnt);
    int sourceCellIdx = this->GetDonorCellIdx(probePnt, grid3D);

    // NOTE:
    // Essentially the same as CopyData, but since CopyAllocate is not
    // working properly the loop has to stay for now.
    for (int arrayIdx = 0; arrayIdx < sourceCD->GetNumberOfArrays(); ++arrayIdx)
    {
      svtkDataArray* sourceArray = sourceCD->GetArray(arrayIdx);
      const char* name = sourceArray->GetName();
      svtkDataArray* targetArray = targetCD->GetArray(name);
      targetArray->SetTuple(cellIdx, sourceCellIdx, sourceArray);
    }
  }
}

//-----------------------------------------------------------------------------
void svtkAMRSliceFilter::GetSlicePointData(svtkUniformGrid* slice, svtkUniformGrid* grid3D)
{
  // STEP 1: Allocate data-structures
  svtkPointData* sourcePD = grid3D->GetPointData();
  svtkPointData* targetPD = slice->GetPointData();

  if (sourcePD->GetNumberOfArrays() == 0)
  {
    // nothing to do here
    return;
  }

  // NOTE:
  // Essentially the same as CopyAllocate
  // For the same reasons as with cell data above,
  // this code is used instead as a precaution, for now.
  svtkIdType numPoints = slice->GetNumberOfPoints();
  for (int arrayIdx = 0; arrayIdx < sourcePD->GetNumberOfArrays(); ++arrayIdx)
  {
    svtkDataArray* array = sourcePD->GetArray(arrayIdx)->NewInstance();
    array->Initialize();
    array->SetName(sourcePD->GetArray(arrayIdx)->GetName());
    array->SetNumberOfComponents(sourcePD->GetArray(arrayIdx)->GetNumberOfComponents());
    array->SetNumberOfTuples(numPoints);
    targetPD->AddArray(array);
    svtkUnsignedCharArray* uca = svtkArrayDownCast<svtkUnsignedCharArray>(array);
    if (uca != nullptr && uca == slice->GetPointGhostArray())
    {
      // initialize the ghost array
      memset(uca->WritePointer(0, numPoints), 0, numPoints);
    }
    array->Delete();
  }

  // STEP 2: Fill in slice data-arrays
  for (int pointIdx = 0; pointIdx < numPoints; ++pointIdx)
  {
    double point[3];
    slice->GetPoint(pointIdx, point);
    int sourcePointIdx = this->GetDonorPointIdx(point, grid3D);

    // NOTE:
    // Essentially the same as CopyData, but since CopyAllocate is not
    // working properly the loop has to stay for now.
    for (int arrayIdx = 0; arrayIdx < sourcePD->GetNumberOfArrays(); ++arrayIdx)
    {
      svtkDataArray* sourceArray = sourcePD->GetArray(arrayIdx);
      const char* name = sourceArray->GetName();
      svtkDataArray* targetArray = targetPD->GetArray(name);
      targetArray->SetTuple(pointIdx, sourcePointIdx, sourceArray);
    }
  }
}

//-----------------------------------------------------------------------------
int svtkAMRSliceFilter::RequestInformation(svtkInformation* svtkNotUsed(rqst),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  this->BlocksToLoad.clear();

  svtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: input information object is nullptr" && (input != nullptr));

  // Check if metadata are passed downstream
  if (input->Has(svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()))
  {
    svtkOverlappingAMR* metadata = svtkOverlappingAMR::SafeDownCast(
      input->Get(svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()));

    svtkPlane* cutPlane = this->GetCutPlane(metadata);
    assert("Cut plane is nullptr" && (cutPlane != nullptr));

    this->ComputeAMRBlocksToLoad(cutPlane, metadata);
    cutPlane->Delete();
  }

  return 1;
}

//-----------------------------------------------------------------------------
int svtkAMRSliceFilter::RequestUpdateExtent(svtkInformation*, svtkInformationVector** inputVector,
  svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  assert("pre: inInfo is nullptr" && (inInfo != nullptr));

  // Send upstream request for higher resolution
  if (this->BlocksToLoad.size() > 0)
  {
    inInfo->Set(svtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), &this->BlocksToLoad[0],
      static_cast<int>(this->BlocksToLoad.size()));
  }
  return 1;
}

//-----------------------------------------------------------------------------
int svtkAMRSliceFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  std::ostringstream oss;
  oss.clear();
  oss << "AMRSlice::Request-" << this->MaxResolution;

  std::string eventName = oss.str();
  svtkTimerLog::MarkStartEvent(eventName.c_str());

  // STEP 0: Get input object
  svtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: input information object is nullptr" && (input != nullptr));

  svtkOverlappingAMR* inputAMR =
    svtkOverlappingAMR::SafeDownCast(input->Get(svtkDataObject::DATA_OBJECT()));

  // STEP 1: Get output object
  svtkInformation* output = outputVector->GetInformationObject(0);
  assert("pre: output information object is nullptr" && (output != nullptr));
  svtkOverlappingAMR* outputAMR =
    svtkOverlappingAMR::SafeDownCast(output->Get(svtkDataObject::DATA_OBJECT()));

  if (this->IsAMRData2D(inputAMR))
  {
    outputAMR->ShallowCopy(inputAMR);
    return 1;
  }

  // STEP 2: Compute global origin
  svtkPlane* cutPlane = this->GetCutPlane(inputAMR);
  assert("Cut plane is nullptr" && (cutPlane != nullptr));

  // STEP 3: Get the AMR slice
  this->GetAMRSliceInPlane(cutPlane, inputAMR, outputAMR);
  cutPlane->Delete();

  svtkTimerLog::MarkEndEvent(eventName.c_str());
  return 1;
}
