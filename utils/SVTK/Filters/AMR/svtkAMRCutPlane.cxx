/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkAMRCutPlane.cxx

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
#include "svtkAMRCutPlane.h"
#include "svtkAMRUtilities.h"
#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCutter.h"
#include "svtkDoubleArray.h"
#include "svtkIdList.h"
#include "svtkIndent.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkObjectFactory.h"
#include "svtkOverlappingAMR.h"
#include "svtkPlane.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkUniformGrid.h"
#include "svtkUnstructuredGrid.h"

#include <algorithm>
#include <cassert>

svtkStandardNewMacro(svtkAMRCutPlane);

//------------------------------------------------------------------------------
svtkAMRCutPlane::svtkAMRCutPlane()
{
  this->SetNumberOfInputPorts(1);
  this->SetNumberOfOutputPorts(1);
  this->LevelOfResolution = 0;
  this->initialRequest = true;
  for (int i = 0; i < 3; ++i)
  {
    this->Center[i] = 0.0;
    this->Normal[i] = 0.0;
  }
  this->Controller = svtkMultiProcessController::GetGlobalController();
  this->UseNativeCutter = 1;
}

//------------------------------------------------------------------------------
svtkAMRCutPlane::~svtkAMRCutPlane()
{
  this->BlocksToLoad.clear();
}

//------------------------------------------------------------------------------
void svtkAMRCutPlane::PrintSelf(std::ostream& oss, svtkIndent indent)
{
  this->Superclass::PrintSelf(oss, indent);
  oss << indent << "LevelOfResolution: " << this->LevelOfResolution << endl;
  oss << indent << "UseNativeCutter: " << this->UseNativeCutter << endl;
  oss << indent << "Controller: " << this->Controller << endl;
  oss << indent << "Center: ";
  for (int i = 0; i < 3; ++i)
  {
    oss << this->Center[i] << " ";
  }
  oss << endl;
  oss << indent << "Normal: ";
  for (int i = 0; i < 3; ++i)
  {
    oss << this->Normal[i] << " ";
  }
  oss << endl;
}

//------------------------------------------------------------------------------
int svtkAMRCutPlane::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  assert("pre: information object is nullptr!" && (info != nullptr));
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkOverlappingAMR");
  return 1;
}

//------------------------------------------------------------------------------
int svtkAMRCutPlane::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  assert("pre: information object is nullptr!" && (info != nullptr));
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
  return 1;
}

//------------------------------------------------------------------------------
int svtkAMRCutPlane::RequestInformation(svtkInformation* svtkNotUsed(rqst),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  this->BlocksToLoad.clear();

  svtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: input information object is nullptr" && (input != nullptr));

  if (input->Has(svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()))
  {
    svtkOverlappingAMR* metadata = svtkOverlappingAMR::SafeDownCast(
      input->Get(svtkCompositeDataPipeline::COMPOSITE_DATA_META_DATA()));

    svtkPlane* cutPlane = this->GetCutPlane(metadata);
    assert("Cut plane is nullptr" && (cutPlane != nullptr));

    this->ComputeAMRBlocksToLoad(cutPlane, metadata);
    cutPlane->Delete();
  }

  this->Modified();
  return 1;
}

//------------------------------------------------------------------------------
int svtkAMRCutPlane::RequestUpdateExtent(svtkInformation* svtkNotUsed(rqst),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  assert("pre: inInfo is nullptr" && (inInfo != nullptr));

  inInfo->Set(svtkCompositeDataPipeline::UPDATE_COMPOSITE_INDICES(), &this->BlocksToLoad[0],
    static_cast<int>(this->BlocksToLoad.size()));
  return 1;
}

//------------------------------------------------------------------------------
int svtkAMRCutPlane::RequestData(svtkInformation* svtkNotUsed(rqst),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // STEP 0: Get input object
  svtkInformation* input = inputVector[0]->GetInformationObject(0);
  assert("pre: input information object is nullptr" && (input != nullptr));
  svtkOverlappingAMR* inputAMR =
    svtkOverlappingAMR::SafeDownCast(input->Get(svtkDataObject::DATA_OBJECT()));
  assert("pre: input AMR dataset is nullptr!" && (inputAMR != nullptr));

  // STEP 1: Get output object
  svtkInformation* output = outputVector->GetInformationObject(0);
  assert("pre: output information is nullptr" && (output != nullptr));
  svtkMultiBlockDataSet* mbds =
    svtkMultiBlockDataSet::SafeDownCast(output->Get(svtkDataObject::DATA_OBJECT()));
  assert("pre: output multi-block dataset is nullptr" && (mbds != nullptr));

  if (this->IsAMRData2D(inputAMR))
  {
    // Return an empty multi-block, we cannot cut a 2-D dataset
    return 1;
  }

  svtkPlane* cutPlane = this->GetCutPlane(inputAMR);
  assert("pre: cutPlane should not be nullptr!" && (cutPlane != nullptr));

  unsigned int blockIdx = 0;
  unsigned int level = 0;
  for (; level < inputAMR->GetNumberOfLevels(); ++level)
  {
    unsigned int dataIdx = 0;
    for (; dataIdx < inputAMR->GetNumberOfDataSets(level); ++dataIdx)
    {
      svtkUniformGrid* grid = inputAMR->GetDataSet(level, dataIdx);
      if (this->UseNativeCutter == 1)
      {
        if (grid != nullptr)
        {
          svtkCutter* myCutter = svtkCutter::New();
          myCutter->SetInputData(grid);
          myCutter->SetCutFunction(cutPlane);
          myCutter->Update();
          mbds->SetBlock(blockIdx, myCutter->GetOutput());
          ++blockIdx;
          myCutter->Delete();
        }
        else
        {
          mbds->SetBlock(blockIdx, nullptr);
          ++blockIdx;
        }
      }
      else
      {
        if (grid != nullptr)
        {
          this->CutAMRBlock(cutPlane, blockIdx, grid, mbds);
          ++blockIdx;
        }
        else
        {
          mbds->SetBlock(blockIdx, nullptr);
          ++blockIdx;
        }
      }
    } // END for all data
  }   // END for all levels

  cutPlane->Delete();
  return 1;
}

//------------------------------------------------------------------------------
void svtkAMRCutPlane::CutAMRBlock(
  svtkPlane* cutPlane, unsigned int blockIdx, svtkUniformGrid* grid, svtkMultiBlockDataSet* output)
{
  assert("pre: multiblock output object is nullptr!" && (output != nullptr));
  assert("pre: grid is nullptr" && (grid != nullptr));

  svtkUnstructuredGrid* mesh = svtkUnstructuredGrid::New();
  svtkPoints* meshPts = svtkPoints::New();
  meshPts->SetDataTypeToDouble();
  svtkCellArray* cells = svtkCellArray::New();

  // Maps points from the input grid to the output grid
  std::map<svtkIdType, svtkIdType> grdPntMapping;
  std::vector<svtkIdType> extractedCells;

  svtkIdType cellIdx = 0;
  for (; cellIdx < grid->GetNumberOfCells(); ++cellIdx)
  {
    if (grid->IsCellVisible(cellIdx) && this->PlaneIntersectsCell(cutPlane, grid->GetCell(cellIdx)))
    {
      extractedCells.push_back(cellIdx);
      this->ExtractCellFromGrid(grid, grid->GetCell(cellIdx), grdPntMapping, meshPts, cells);
    } // END if
  }   // END for all cells

  // Sanity checks
  assert("post: Number of mesh points should match map size!" &&
    (static_cast<int>(grdPntMapping.size()) == meshPts->GetNumberOfPoints()));
  assert("post: Number of cells mismatch" &&
    (cells->GetNumberOfCells() == static_cast<int>(extractedCells.size())));

  // Insert the points
  mesh->SetPoints(meshPts);
  meshPts->Delete();

  std::vector<int> types;
  if (grid->GetDataDimension() == 3)
  {
    types.resize(cells->GetNumberOfCells(), SVTK_VOXEL);
  }
  else
  {
    svtkErrorMacro("Cannot cut a grid of dimension=" << grid->GetDataDimension());
    output->SetBlock(blockIdx, nullptr);
    return;
  }

  // Insert the cells
  mesh->SetCells(&types[0], cells);
  cells->Delete();

  // Extract fields
  this->ExtractPointDataFromGrid(
    grid, grdPntMapping, mesh->GetNumberOfPoints(), mesh->GetPointData());
  this->ExtractCellDataFromGrid(grid, extractedCells, mesh->GetCellData());

  output->SetBlock(blockIdx, mesh);
  mesh->Delete();
  grdPntMapping.clear();
  extractedCells.clear();
}

//------------------------------------------------------------------------------
void svtkAMRCutPlane::ExtractCellFromGrid(svtkUniformGrid* grid, svtkCell* cell,
  std::map<svtkIdType, svtkIdType>& grdPntMapping, svtkPoints* nodes, svtkCellArray* cells)
{
  assert("pre: grid is nullptr" && (grid != nullptr));
  assert("pre: cell is nullptr" && (cell != nullptr));
  assert("pre: cells is nullptr" && (cells != nullptr));

  cells->InsertNextCell(cell->GetNumberOfPoints());
  svtkIdType nodeIdx = 0;
  for (; nodeIdx < cell->GetNumberOfPoints(); ++nodeIdx)
  {
    // Get the point ID w.r.t. the grid
    svtkIdType meshPntIdx = cell->GetPointId(nodeIdx);
    assert("pre: mesh point ID should within grid range point ID" &&
      (meshPntIdx < grid->GetNumberOfPoints()));

    if (grdPntMapping.find(meshPntIdx) != grdPntMapping.end())
    {
      // Point already exists in nodes
      cells->InsertCellPoint(grdPntMapping[meshPntIdx]);
    }
    else
    {
      // Push point to the end of the list
      svtkIdType nidx = nodes->GetNumberOfPoints();
      double* pnt = grid->GetPoint(meshPntIdx);
      nodes->InsertPoint(nidx, pnt);
      assert("post: number of points should be increased by 1" &&
        (nodes->GetNumberOfPoints() == (nidx + 1)));
      grdPntMapping[meshPntIdx] = nidx;
      cells->InsertCellPoint(nidx);
    }
  } // END for all nodes
}

//------------------------------------------------------------------------------
void svtkAMRCutPlane::ExtractPointDataFromGrid(svtkUniformGrid* grid,
  std::map<svtkIdType, svtkIdType>& gridPntMapping, svtkIdType NumNodes, svtkPointData* PD)
{
  assert("pre: grid is nullptr!" && (grid != nullptr));
  assert("pre: target point data is nullptr!" && (PD != nullptr));

  if ((grid->GetPointData()->GetNumberOfArrays() == 0) || (gridPntMapping.empty()))
  {
    // Nothing to extract short-circuit here
    return;
  }

  svtkPointData* GPD = grid->GetPointData();
  for (int fieldArray = 0; fieldArray < GPD->GetNumberOfArrays(); ++fieldArray)
  {
    svtkDataArray* sourceArray = GPD->GetArray(fieldArray);
    int dataType = sourceArray->GetDataType();
    svtkDataArray* array = svtkDataArray::CreateDataArray(dataType);
    assert("pre: failed to create array!" && (array != nullptr));

    array->SetName(sourceArray->GetName());
    array->SetNumberOfComponents(sourceArray->GetNumberOfComponents());
    array->SetNumberOfTuples(NumNodes);

    // Copy tuples from source array
    std::map<svtkIdType, svtkIdType>::iterator iter = gridPntMapping.begin();
    for (; iter != gridPntMapping.end(); ++iter)
    {
      svtkIdType srcIdx = iter->first;
      svtkIdType targetIdx = iter->second;
      assert("pre: source node index is out-of-bounds" && (srcIdx >= 0) &&
        (srcIdx < grid->GetNumberOfPoints()));
      assert(
        "pre: target node index is out-of-bounds" && (targetIdx >= 0) && (targetIdx < NumNodes));
      array->SetTuple(targetIdx, srcIdx, sourceArray);
    } // END for all extracted nodes

    PD->AddArray(array);
    array->Delete();
  } // END for all arrays
}

//------------------------------------------------------------------------------
void svtkAMRCutPlane::ExtractCellDataFromGrid(
  svtkUniformGrid* grid, std::vector<svtkIdType>& cellIdxList, svtkCellData* CD)
{
  assert("pre: grid is nullptr!" && (grid != nullptr));
  assert("pre: target cell data is nullptr!" && (CD != nullptr));

  if ((grid->GetCellData()->GetNumberOfArrays() == 0) || (cellIdxList.empty()))
  {
    // Nothing to extract short-circuit here
    return;
  }

  int NumCells = static_cast<int>(cellIdxList.size());
  svtkCellData* GCD = grid->GetCellData();
  for (int fieldArray = 0; fieldArray < GCD->GetNumberOfArrays(); ++fieldArray)
  {
    svtkDataArray* sourceArray = GCD->GetArray(fieldArray);
    int dataType = sourceArray->GetDataType();
    svtkDataArray* array = svtkDataArray::CreateDataArray(dataType);
    assert("pre: failed to create array!" && (array != nullptr));

    array->SetName(sourceArray->GetName());
    array->SetNumberOfComponents(sourceArray->GetNumberOfComponents());
    array->SetNumberOfTuples(NumCells);

    // Copy tuples from source array
    for (int i = 0; i < NumCells; ++i)
    {
      svtkIdType cellIdx = cellIdxList[i];
      assert("pre: cell index is out-of-bounds!" && (cellIdx >= 0) &&
        (cellIdx < grid->GetNumberOfCells()));
      array->SetTuple(i, cellIdx, sourceArray);
    } // END for all extracted cells

    CD->AddArray(array);
    array->Delete();
  } // END for all arrays
}

//------------------------------------------------------------------------------
svtkPlane* svtkAMRCutPlane::GetCutPlane(svtkOverlappingAMR* metadata)
{
  assert("pre: metadata is nullptr" && (metadata != nullptr));

  svtkPlane* pl = svtkPlane::New();

  double bounds[6];
  metadata->GetBounds(bounds);

  // Get global bounds
  double minBounds[3] = { bounds[0], bounds[2], bounds[4] };
  double maxBounds[3] = { bounds[1], bounds[3], bounds[5] };

  this->InitializeCenter(minBounds, maxBounds);

  pl->SetNormal(this->Normal);
  pl->SetOrigin(this->Center);
  return (pl);
}

//------------------------------------------------------------------------------
void svtkAMRCutPlane::ComputeAMRBlocksToLoad(svtkPlane* p, svtkOverlappingAMR* m)
{
  assert("pre: Plane object is nullptr" && (p != nullptr));
  assert("pre: metadata is nullptr" && (m != nullptr));

  // Store A,B,C,D from the plane equation
  double plane[4];
  plane[0] = p->GetNormal()[0];
  plane[1] = p->GetNormal()[1];
  plane[2] = p->GetNormal()[2];
  plane[3] = p->GetNormal()[0] * p->GetOrigin()[0] + p->GetNormal()[1] * p->GetOrigin()[1] +
    p->GetNormal()[2] * p->GetOrigin()[2];

  double bounds[6];

  int NumLevels = m->GetNumberOfLevels();
  int maxLevelToLoad = (this->LevelOfResolution < NumLevels) ? this->LevelOfResolution : NumLevels;

  unsigned int level = 0;
  for (; level <= static_cast<unsigned int>(maxLevelToLoad); ++level)
  {
    unsigned int dataIdx = 0;
    for (; dataIdx < m->GetNumberOfDataSets(level); ++dataIdx)
    {
      m->GetBounds(level, dataIdx, bounds);
      if (this->PlaneIntersectsAMRBox(plane, bounds))
      {
        unsigned int amrGridIdx = m->GetCompositeIndex(level, dataIdx);
        this->BlocksToLoad.push_back(amrGridIdx);
      }
    } // END for all data
  }   // END for all levels

  std::sort(this->BlocksToLoad.begin(), this->BlocksToLoad.end());
}

//------------------------------------------------------------------------------
void svtkAMRCutPlane::InitializeCenter(double min[3], double max[3])
{
  if (!this->initialRequest)
  {
    return;
  }

  this->Center[0] = 0.5 * (max[0] - min[0]);
  this->Center[1] = 0.5 * (max[1] - min[1]);
  this->Center[2] = 0.5 * (max[2] - min[2]);
  this->initialRequest = false;
}

//------------------------------------------------------------------------------
bool svtkAMRCutPlane::PlaneIntersectsCell(svtkPlane* pl, svtkCell* cell)
{
  assert("pre: plane is nullptr" && (pl != nullptr));
  assert("pre: cell is nullptr!" && (cell != nullptr));
  return (this->PlaneIntersectsAMRBox(pl, cell->GetBounds()));
}
//------------------------------------------------------------------------------
bool svtkAMRCutPlane::PlaneIntersectsAMRBox(svtkPlane* pl, double bounds[6])
{
  assert("pre: plane is nullptr" && (pl != nullptr));

  // Store A,B,C,D from the plane equation
  double plane[4];
  plane[0] = pl->GetNormal()[0];
  plane[1] = pl->GetNormal()[1];
  plane[2] = pl->GetNormal()[2];
  plane[3] = pl->GetNormal()[0] * pl->GetOrigin()[0] + pl->GetNormal()[1] * pl->GetOrigin()[1] +
    pl->GetNormal()[2] * pl->GetOrigin()[2];

  return (this->PlaneIntersectsAMRBox(plane, bounds));
}

//------------------------------------------------------------------------------
bool svtkAMRCutPlane::PlaneIntersectsAMRBox(double plane[4], double bounds[6])
{
  bool lowPnt = false;
  bool highPnt = false;

  for (int i = 0; i < 8; ++i)
  {
    // Get box coordinates
    double x = (i & 1) ? bounds[1] : bounds[0];
    double y = (i & 2) ? bounds[3] : bounds[2];
    double z = (i & 3) ? bounds[5] : bounds[4];

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

//------------------------------------------------------------------------------
bool svtkAMRCutPlane::IsAMRData2D(svtkOverlappingAMR* input)
{
  assert("pre: Input AMR dataset is nullptr" && (input != nullptr));

  if (input->GetGridDescription() != SVTK_XYZ_GRID)
  {
    return true;
  }

  return false;
}
