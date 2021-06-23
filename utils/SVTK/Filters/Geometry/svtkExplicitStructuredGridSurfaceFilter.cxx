/*=========================================================================
 Copyright (c) Kitware SAS 2014
 All rights reserved.
 More information http://www.kitware.fr
=========================================================================*/
#include "svtkExplicitStructuredGridSurfaceFilter.h"

#include "svtkCellData.h"
#include "svtkExplicitStructuredGrid.h"
#include "svtkGenericCell.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkUnsignedCharArray.h"

#include <vector>

svtkStandardNewMacro(svtkExplicitStructuredGridSurfaceFilter);

//----------------------------------------------------------------------------
svtkExplicitStructuredGridSurfaceFilter::svtkExplicitStructuredGridSurfaceFilter()
{
  this->PieceInvariant = 0;

  this->PassThroughCellIds = 0;
  this->PassThroughPointIds = 0;

  this->OriginalCellIdsName = nullptr;
  this->SetOriginalCellIdsName("svtkOriginalCellIds");

  this->OriginalPointIdsName = nullptr;
  this->SetOriginalPointIdsName("svtkOriginalPointIds");
}

//----------------------------------------------------------------------------
svtkExplicitStructuredGridSurfaceFilter::~svtkExplicitStructuredGridSurfaceFilter()
{
  this->SetOriginalCellIdsName(nullptr);
  this->SetOriginalPointIdsName(nullptr);
}

// ----------------------------------------------------------------------------
int svtkExplicitStructuredGridSurfaceFilter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  inputVector[0]->GetInformationObject(0)->Get(
    svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent);
  return 1;
}

//----------------------------------------------------------------------------
int svtkExplicitStructuredGridSurfaceFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int ghostLevels;
  ghostLevels = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  ghostLevels = std::max(ghostLevels, 1);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);

  return 1;
}

//----------------------------------------------------------------------------
int svtkExplicitStructuredGridSurfaceFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  // Get the input and output
  svtkExplicitStructuredGrid* input = svtkExplicitStructuredGrid::GetData(inputVector[0], 0);
  svtkPolyData* output = svtkPolyData::GetData(outputVector, 0);

  svtkIdType numCells = input->GetNumberOfCells();
  if (input->CheckAttributes() || numCells == 0)
  {
    return 1;
  }

  inputVector[0]->GetInformationObject(0)->Get(
    svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), this->WholeExtent);

  return this->ExtractSurface(input, output);
}

static int hexaFaces[6][4] = {
  { 0, 4, 7, 3 },
  { 1, 2, 6, 5 },
  { 0, 1, 5, 4 },
  { 3, 7, 6, 2 },
  { 0, 3, 2, 1 },
  { 4, 5, 6, 7 },
};

//----------------------------------------------------------------------------
int svtkExplicitStructuredGridSurfaceFilter::ExtractSurface(
  svtkExplicitStructuredGrid* input, svtkPolyData* output)
{
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();

  if (numCells == 0)
  {
    return 1;
  }

  svtkDebugMacro(<< "Executing explicit structured grid surface filter");

  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();

  svtkNew<svtkIdTypeArray> originalCellIds;
  if (this->PassThroughCellIds)
  {
    originalCellIds->SetName(this->GetOriginalCellIdsName());
    originalCellIds->SetNumberOfComponents(1);
    originalCellIds->Allocate(numCells);
    outputCD->AddArray(originalCellIds.GetPointer());
  }
  svtkNew<svtkIdTypeArray> originalPointIds;
  if (this->PassThroughPointIds)
  {
    originalPointIds->SetName(this->GetOriginalPointIdsName());
    originalPointIds->SetNumberOfComponents(1);
    originalPointIds->Allocate(numPts);
    outputPD->AddArray(originalPointIds.GetPointer());
  }

  svtkNew<svtkIdList> cellIds;
  svtkUnsignedCharArray* connectivityFlags = nullptr;

  char* facesConnectivityFlagsArrayName = input->GetFacesConnectivityFlagsArrayName();
  if (facesConnectivityFlagsArrayName)
  {
    connectivityFlags = svtkUnsignedCharArray::SafeDownCast(
      input->GetCellData()->GetAbstractArray(facesConnectivityFlagsArrayName));
    if (!connectivityFlags)
    {
      svtkErrorMacro("Make sure Connectivity Flags have been computed before using this filter");
      return 0;
    }
  }

  svtkPoints* points = input->GetPoints();
  svtkCellArray* cells = input->GetCells();
  if (!points || !cells)
  {
    return 1;
  }

  // Allocate
  svtkNew<svtkPoints> newPts;
  newPts->SetDataType(points->GetDataType());
  newPts->Allocate(numPts, numPts / 2);
  output->SetPoints(newPts);

  svtkNew<svtkCellArray> newCells;
  newCells->AllocateEstimate(numCells / 10, 4);
  output->SetPolys(newCells);

  outputPD->CopyGlobalIdsOn();
  outputPD->CopyAllocate(pd, numPts);
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(cd, numCells);

  // Traverse cells to extract geometry
  int abort = 0;
  svtkIdType progressInterval = numCells / 20 + 1;
  svtkIdType npts;
  const svtkIdType* pts;
  cells->InitTraversal();
  std::vector<svtkIdType> pointIdVector(numPts, -1);

  bool mayBlank = input->HasAnyBlankCells();
  bool mayBlankOrGhost = mayBlank || input->HasAnyGhostCells();
  for (svtkIdType cellId = 0; cells->GetNextCell(npts, pts) && !abort; cellId++)
  {
    // Progress and abort method support
    if (!(cellId % progressInterval))
    {
      svtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }

    // Ignore blank cells and ghost cells
    if (mayBlankOrGhost && input->GetCellGhostArray()->GetValue(cellId) > 0)
    {
      continue;
    }

    svtkIdType neighbors[6];
    input->GetCellNeighbors(cellId, neighbors);
    unsigned char cflag = connectivityFlags->GetValue(cellId);
    // Traverse hexahedron cell faces
    for (int f = 0; f < 6; f++)
    {
      bool nonBlankNeighbor =
        !mayBlank || (neighbors[f] >= 0 && input->IsCellVisible(neighbors[f]));

      // Connected faces with non blank neighbor are skipped
      if (cflag & (1 << f) && nonBlankNeighbor)
      {
        continue;
      }

      svtkIdType facePtIds[4];
      for (int p = 0; p < 4; p++)
      {
        svtkIdType ptId = pts[hexaFaces[f][p]];
        svtkIdType pt = pointIdVector[ptId];
        if (pt == -1)
        {
          double x[3];
          points->GetPoint(ptId, x);
          pt = newPts->InsertNextPoint(x);
          pointIdVector[ptId] = pt;
          outputPD->CopyData(pd, ptId, pt);
          if (this->PassThroughPointIds)
          {
            originalPointIds->InsertValue(pt, ptId);
          }
        }
        facePtIds[p] = pt;
      }
      svtkIdType newCellId = newCells->InsertNextCell(4, facePtIds);
      outputCD->CopyData(cd, cellId, newCellId);
      if (this->PassThroughCellIds)
      {
        originalCellIds->InsertValue(newCellId, cellId);
      }

    } // for all faces
  }   // for all cells
  // free storage
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
int svtkExplicitStructuredGridSurfaceFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkExplicitStructuredGrid");
  return 1;
}

//----------------------------------------------------------------------------
void svtkExplicitStructuredGridSurfaceFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PieceInvariant: " << this->PieceInvariant << endl;
  os << indent << "PassThroughCellIds: " << (this->PassThroughCellIds ? "On\n" : "Off\n");
  os << indent << "PassThroughPointIds: " << (this->PassThroughPointIds ? "On\n" : "Off\n");

  os << indent << "OriginalCellIdsName: " << this->GetOriginalCellIdsName() << endl;
  os << indent << "OriginalPointIdsName: " << this->GetOriginalPointIdsName() << endl;
}
