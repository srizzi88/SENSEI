/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataConnectivityFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkPolyDataConnectivityFilter.h"

#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkFloatArray.h"
#include "svtkIdList.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

#include <algorithm> // for fill_n

svtkStandardNewMacro(svtkPolyDataConnectivityFilter);

// Construct with default extraction mode to extract largest regions.
svtkPolyDataConnectivityFilter::svtkPolyDataConnectivityFilter()
{
  this->RegionSizes = svtkIdTypeArray::New();
  this->ExtractionMode = SVTK_EXTRACT_LARGEST_REGION;
  this->ColorRegions = 0;

  this->ScalarConnectivity = 0;
  this->FullScalarConnectivity = 0;
  this->ScalarRange[0] = 0.0;
  this->ScalarRange[1] = 1.0;

  this->ClosestPoint[0] = this->ClosestPoint[1] = this->ClosestPoint[2] = 0.0;

  this->CellScalars = svtkFloatArray::New();
  this->CellScalars->Allocate(8);

  this->NeighborCellPointIds = svtkIdList::New();
  this->NeighborCellPointIds->Allocate(8);

  this->Seeds = svtkIdList::New();
  this->SpecifiedRegionIds = svtkIdList::New();

  this->MarkVisitedPointIds = 0;
  this->VisitedPointIds = svtkIdList::New();

  this->OutputPointsPrecision = DEFAULT_PRECISION;
}

svtkPolyDataConnectivityFilter::~svtkPolyDataConnectivityFilter()
{
  this->RegionSizes->Delete();
  this->CellScalars->Delete();
  this->NeighborCellPointIds->Delete();
  this->Seeds->Delete();
  this->SpecifiedRegionIds->Delete();
  this->VisitedPointIds->Delete();
}

int svtkPolyDataConnectivityFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType cellId, newCellId, i, pt;
  svtkPoints* inPts;
  svtkPoints* newPts;
  svtkIdType *cells, id, n;
  svtkIdType npts;
  const svtkIdType* pts;
  svtkIdType ncells;
  svtkIdType maxCellsInRegion;
  svtkIdType largestRegionId = 0;
  svtkPointData *pd = input->GetPointData(), *outputPD = output->GetPointData();
  svtkCellData *cd = input->GetCellData(), *outputCD = output->GetCellData();

  svtkDebugMacro(<< "Executing polygon connectivity filter.");

  //  Check input/allocate storage
  //
  inPts = input->GetPoints();

  if (inPts == nullptr)
  {
    svtkErrorMacro("No points!");
    return 1;
  }

  const svtkIdType numPts = inPts->GetNumberOfPoints();
  const svtkIdType numCells = input->GetNumberOfCells();

  if (numPts < 1 || numCells < 1)
  {
    svtkDebugMacro(<< "No data to connect!");
    return 1;
  }

  // See whether to consider scalar connectivity
  //
  this->InScalars = input->GetPointData()->GetScalars();
  if (!this->ScalarConnectivity)
  {
    this->InScalars = nullptr;
  }
  else
  {
    if (this->ScalarRange[1] < this->ScalarRange[0])
    {
      this->ScalarRange[1] = this->ScalarRange[0];
    }
  }

  // Build cell structure
  //
  this->Mesh = svtkPolyData::New();
  this->Mesh->CopyStructure(input);
  this->Mesh->BuildLinks();
  this->UpdateProgress(0.10);

  // Remove all visited point ids
  this->VisitedPointIds->Reset();

  // Initialize.  Keep track of points and cells visited.
  //
  this->RegionSizes->Reset();
  this->Visited = new svtkIdType[numCells];
  std::fill_n(this->Visited, numCells, -1);
  this->PointMap = new svtkIdType[numPts];
  std::fill_n(this->PointMap, numPts, -1);

  this->NewScalars = svtkIdTypeArray::New();
  this->NewScalars->SetName("RegionId");
  this->NewScalars->SetNumberOfTuples(numPts);
  newPts = svtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inPts->GetDataType());
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(SVTK_DOUBLE);
  }

  newPts->Allocate(numPts);

  // Traverse all cells marking those visited.  Each new search
  // starts a new connected region. Connected region grows
  // using a connected wave propagation.
  //
  this->Wave.reserve(numPts);
  this->Wave2.reserve(numPts);

  this->PointNumber = 0;
  this->RegionNumber = 0;
  maxCellsInRegion = 0;

  this->CellIds = svtkIdList::New();
  this->CellIds->Allocate(8, SVTK_CELL_SIZE);
  this->PointIds = svtkIdList::New();
  this->PointIds->Allocate(8, SVTK_CELL_SIZE);

  if (this->ExtractionMode != SVTK_EXTRACT_POINT_SEEDED_REGIONS &&
    this->ExtractionMode != SVTK_EXTRACT_CELL_SEEDED_REGIONS &&
    this->ExtractionMode != SVTK_EXTRACT_CLOSEST_POINT_REGION)
  { // visit all cells marking with region number
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (cellId && !(cellId % 5000))
      {
        this->UpdateProgress(0.1 + 0.8 * cellId / numCells);
      }

      if (this->Visited[cellId] < 0)
      {
        this->NumCellsInRegion = 0;
        this->Wave.push_back(cellId);
        this->TraverseAndMark();

        if (this->NumCellsInRegion > maxCellsInRegion)
        {
          maxCellsInRegion = this->NumCellsInRegion;
          largestRegionId = this->RegionNumber;
        }

        this->RegionSizes->InsertValue(this->RegionNumber++, this->NumCellsInRegion);
        this->Wave.clear();
        this->Wave2.clear();
      }
    }
  }
  else // regions have been seeded, everything considered in same region
  {
    this->NumCellsInRegion = 0;

    if (this->ExtractionMode == SVTK_EXTRACT_POINT_SEEDED_REGIONS)
    {
      for (i = 0; i < this->Seeds->GetNumberOfIds(); i++)
      {
        pt = this->Seeds->GetId(i);
        if (pt >= 0)
        {
          this->Mesh->GetPointCells(pt, ncells, cells);
          for (unsigned short j = 0; j < ncells; ++j)
          {
            this->Wave.push_back(cells[j]);
          }
        }
      }
    }
    else if (this->ExtractionMode == SVTK_EXTRACT_CELL_SEEDED_REGIONS)
    {
      for (i = 0; i < this->Seeds->GetNumberOfIds(); i++)
      {
        cellId = this->Seeds->GetId(i);
        if (cellId >= 0)
        {
          this->Wave.push_back(cellId);
        }
      }
    }
    else if (this->ExtractionMode == SVTK_EXTRACT_CLOSEST_POINT_REGION)
    { // loop over points, find closest one
      double minDist2, dist2, x[3];
      int minId = 0;
      for (minDist2 = SVTK_DOUBLE_MAX, i = 0; i < numPts; i++)
      {
        inPts->GetPoint(i, x);
        dist2 = svtkMath::Distance2BetweenPoints(x, this->ClosestPoint);
        if (dist2 < minDist2)
        {
          minId = i;
          minDist2 = dist2;
        }
      }
      this->Mesh->GetPointCells(minId, ncells, cells);
      for (unsigned short j = 0; j < ncells; ++j)
      {
        this->Wave.push_back(cells[j]);
      }
    }
    this->UpdateProgress(0.5);

    // mark all seeded regions
    this->TraverseAndMark();
    this->RegionSizes->InsertValue(this->RegionNumber, this->NumCellsInRegion);
    this->UpdateProgress(0.9);
  } // else extracted seeded cells

  svtkDebugMacro(<< "Extracted " << this->RegionNumber << " region(s)");

  // Now that points and cells have been marked, traverse these lists pulling
  // everything that has been visited.
  //
  // Pass through point data that has been visited
  outputPD->CopyAllocate(pd);
  outputCD->CopyAllocate(cd);

  for (i = 0; i < numPts; i++)
  {
    if (this->PointMap[i] > -1)
    {
      newPts->InsertPoint(this->PointMap[i], inPts->GetPoint(i));
      outputPD->CopyData(pd, i, this->PointMap[i]);
    }
  }

  // if coloring regions; send down new scalar data
  if (this->ColorRegions)
  {
    int idx = outputPD->AddArray(this->NewScalars);
    outputPD->SetActiveAttribute(idx, svtkDataSetAttributes::SCALARS);
  }
  this->NewScalars->Delete();

  output->SetPoints(newPts);
  newPts->Delete();

  // Create output cells. Have to allocate storage first.
  //
  if ((n = input->GetVerts()->GetNumberOfCells()) > 0)
  {
    svtkCellArray* newVerts = svtkCellArray::New();
    newVerts->AllocateEstimate(n, 1);
    output->SetVerts(newVerts);
    newVerts->Delete();
  }
  if ((n = input->GetLines()->GetNumberOfCells()) > 0)
  {
    svtkCellArray* newLines = svtkCellArray::New();
    newLines->AllocateEstimate(n, 2);
    output->SetLines(newLines);
    newLines->Delete();
  }
  if ((n = input->GetPolys()->GetNumberOfCells()) > 0)
  {
    svtkCellArray* newPolys = svtkCellArray::New();
    newPolys->AllocateEstimate(n, 3);
    output->SetPolys(newPolys);
    newPolys->Delete();
  }
  if ((n = input->GetStrips()->GetNumberOfCells()) > 0)
  {
    svtkCellArray* newStrips = svtkCellArray::New();
    newStrips->AllocateEstimate(n, 5);
    output->SetStrips(newStrips);
    newStrips->Delete();
  }

  if (this->ExtractionMode == SVTK_EXTRACT_POINT_SEEDED_REGIONS ||
    this->ExtractionMode == SVTK_EXTRACT_CELL_SEEDED_REGIONS ||
    this->ExtractionMode == SVTK_EXTRACT_CLOSEST_POINT_REGION ||
    this->ExtractionMode == SVTK_EXTRACT_ALL_REGIONS)
  { // extract any cell that's been visited
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (this->Visited[cellId] >= 0)
      {
        this->Mesh->GetCellPoints(cellId, npts, pts);
        this->PointIds->Reset();
        for (i = 0; i < npts; i++)
        {
          id = this->PointMap[pts[i]];
          this->PointIds->InsertId(i, id);

          // If we asked to mark the visited point ids, mark them.
          if (this->MarkVisitedPointIds)
          {
            this->VisitedPointIds->InsertUniqueId(pts[i]);
          }
        }
        newCellId = output->InsertNextCell(this->Mesh->GetCellType(cellId), this->PointIds);
        outputCD->CopyData(cd, cellId, newCellId);
      }
    }
  }
  else if (this->ExtractionMode == SVTK_EXTRACT_SPECIFIED_REGIONS)
  {
    for (cellId = 0; cellId < numCells; cellId++)
    {
      int inReg, regionId;
      if ((regionId = this->Visited[cellId]) >= 0)
      {
        for (inReg = 0, i = 0; i < this->SpecifiedRegionIds->GetNumberOfIds(); i++)
        {
          if (regionId == this->SpecifiedRegionIds->GetId(i))
          {
            inReg = 1;
            break;
          }
        }
        if (inReg)
        {
          this->Mesh->GetCellPoints(cellId, npts, pts);
          this->PointIds->Reset();
          for (i = 0; i < npts; i++)
          {
            id = this->PointMap[pts[i]];
            this->PointIds->InsertId(i, id);

            // If we asked to mark the visited point ids, mark them.
            if (this->MarkVisitedPointIds)
            {
              this->VisitedPointIds->InsertUniqueId(pts[i]);
            }
          }
          newCellId = output->InsertNextCell(this->Mesh->GetCellType(cellId), this->PointIds);
          outputCD->CopyData(cd, cellId, newCellId);
        }
      }
    }
  }
  else // extract largest region
  {
    for (cellId = 0; cellId < numCells; cellId++)
    {
      if (this->Visited[cellId] == largestRegionId)
      {
        this->Mesh->GetCellPoints(cellId, npts, pts);
        this->PointIds->Reset();
        for (i = 0; i < npts; i++)
        {
          id = this->PointMap[pts[i]];
          this->PointIds->InsertId(i, id);

          // If we asked to mark the visited point ids, mark them.
          if (this->MarkVisitedPointIds)
          {
            this->VisitedPointIds->InsertUniqueId(pts[i]);
          }
        }
        newCellId = output->InsertNextCell(this->Mesh->GetCellType(cellId), this->PointIds);
        outputCD->CopyData(cd, cellId, newCellId);
      }
    }
  }

  delete[] this->Visited;
  delete[] this->PointMap;
  this->Mesh->Delete();
  output->Squeeze();
  this->CellIds->Delete();
  this->PointIds->Delete();

  int num = this->GetNumberOfExtractedRegions();
  svtkIdType count = 0;

  for (int ii = 0; ii < num; ii++)
  {
    count += this->RegionSizes->GetValue(ii);
  }
  svtkDebugMacro(<< "Total # of cells accounted for: " << count);
  svtkDebugMacro(<< "Extracted " << output->GetNumberOfCells() << " cells");

  return 1;
}

// Mark current cell as visited and assign region number.  Note:
// traversal occurs across shared vertices.
//
void svtkPolyDataConnectivityFilter::TraverseAndMark()
{
  svtkIdType cellId, ptId, numIds, i;
  int j, k;
  svtkIdType* cells;
  svtkIdType npts;
  const svtkIdType* pts;
  svtkIdType ncells;
  const svtkIdType numCells = this->Mesh->GetNumberOfCells();

  while ((numIds = static_cast<svtkIdType>(this->Wave.size())) > 0)
  {
    for (i = 0; i < numIds; i++)
    {
      cellId = this->Wave[i];
      if (this->Visited[cellId] < 0)
      {
        this->Visited[cellId] = this->RegionNumber;
        this->NumCellsInRegion++;
        this->Mesh->GetCellPoints(cellId, npts, pts);

        for (j = 0; j < npts; j++)
        {
          if (this->PointMap[ptId = pts[j]] < 0)
          {
            this->PointMap[ptId] = this->PointNumber++;
            svtkArrayDownCast<svtkIdTypeArray>(this->NewScalars)
              ->SetValue(this->PointMap[ptId], this->RegionNumber);

            this->Mesh->GetPointCells(ptId, ncells, cells);

            // check connectivity criterion (geometric + scalar)
            if (this->InScalars)
            {
              for (k = 0; k < ncells; ++k)
              {
                if (this->IsScalarConnected(cells[k]))
                {
                  this->Wave2.push_back(cells[k]);
                }
              }
            }
            else
            {
              for (k = 0; k < ncells; ++k)
              {
                this->Wave2.push_back(cells[k]);
              }
            }
          }
        } // for all points of this cell
      }   // if cell not yet visited
    }     // for all cells in this wave

    this->Wave = this->Wave2;
    this->Wave2.clear();
    this->Wave2.reserve(numCells);
  } // while wave is not empty
}

// --------------------------------------------------------------------------
int svtkPolyDataConnectivityFilter::IsScalarConnected(svtkIdType cellId)
{
  double s;

  this->Mesh->GetCellPoints(cellId, this->NeighborCellPointIds);
  const int numScalars = this->NeighborCellPointIds->GetNumberOfIds();

  this->CellScalars->SetNumberOfTuples(numScalars);
  this->InScalars->GetTuples(this->NeighborCellPointIds, this->CellScalars);

  double range[2] = { SVTK_DOUBLE_MAX, SVTK_DOUBLE_MIN };

  // Loop through the cell points.
  for (int ii = 0; ii < numScalars; ii++)
  {
    s = this->CellScalars->GetComponent(ii, 0);
    if (s < range[0])
    {
      range[0] = s;
    }
    if (s > range[1])
    {
      range[1] = s;
    }
  }

  // Check if the scalars lie within the user supplied scalar range.

  if (this->FullScalarConnectivity)
  {
    // All points in this cell must lie in the user supplied scalar range
    // for this cell to qualify as being connected.
    if (range[0] >= this->ScalarRange[0] && range[1] <= this->ScalarRange[1])
    {
      return 1;
    }
  }
  else
  {
    // Any point from this cell must lie is the user supplied scalar range
    // for this cell to qualify as being connected
    if (range[1] >= this->ScalarRange[0] && range[0] <= this->ScalarRange[1])
    {
      return 1;
    }
  }

  return 0;
}

// --------------------------------------------------------------------------
// Obtain the number of connected regions.
int svtkPolyDataConnectivityFilter::GetNumberOfExtractedRegions()
{
  return this->RegionSizes->GetMaxId() + 1;
}

// Initialize list of point ids/cell ids used to seed regions.
void svtkPolyDataConnectivityFilter::InitializeSeedList()
{
  this->Modified();
  this->Seeds->Reset();
}

// Add a seed id (point or cell id). Note: ids are 0-offset.
void svtkPolyDataConnectivityFilter::AddSeed(int id)
{
  this->Modified();
  this->Seeds->InsertNextId(id);
}

// Delete a seed id (point or cell id). Note: ids are 0-offset.
void svtkPolyDataConnectivityFilter::DeleteSeed(int id)
{
  this->Modified();
  this->Seeds->DeleteId(id);
}

// Initialize list of region ids to extract.
void svtkPolyDataConnectivityFilter::InitializeSpecifiedRegionList()
{
  this->Modified();
  this->SpecifiedRegionIds->Reset();
}

// Add a region id to extract. Note: ids are 0-offset.
void svtkPolyDataConnectivityFilter::AddSpecifiedRegion(int id)
{
  this->Modified();
  this->SpecifiedRegionIds->InsertNextId(id);
}

// Delete a region id to extract. Note: ids are 0-offset.
void svtkPolyDataConnectivityFilter::DeleteSpecifiedRegion(int id)
{
  this->Modified();
  this->SpecifiedRegionIds->DeleteId(id);
}

void svtkPolyDataConnectivityFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Extraction Mode: ";
  os << this->GetExtractionModeAsString() << "\n";

  os << indent << "Closest Point: (" << this->ClosestPoint[0] << ", " << this->ClosestPoint[1]
     << ", " << this->ClosestPoint[2] << ")\n";

  os << indent << "Color Regions: " << (this->ColorRegions ? "On\n" : "Off\n");

  os << indent << "Scalar Connectivity: " << (this->ScalarConnectivity ? "On\n" : "Off\n");

  if (this->ScalarConnectivity)
  {
    os << indent << "Full Connectivity: " << (this->FullScalarConnectivity ? "On\n" : "Off\n");
  }

  os << indent << "Mark visited point ids: " << (this->MarkVisitedPointIds ? "On\n" : "Off\n");
  if (this->MarkVisitedPointIds)
  {
    this->VisitedPointIds->PrintSelf(os, indent.GetNextIndent());
  }

  double* range = this->GetScalarRange();
  os << indent << "Scalar Range: (" << range[0] << ", " << range[1] << ")\n";

  os << indent << "RegionSizes: ";
  if (this->GetNumberOfExtractedRegions() > 10)
  {
    os << "Only first ten of " << this->GetNumberOfExtractedRegions() << " listed";
  }
  os << std::endl;

  for (svtkIdType id = 0;
       id < (this->GetNumberOfExtractedRegions() > 10 ? 10 : this->GetNumberOfExtractedRegions());
       id++)
  {
    os << indent << indent << id << ": " << this->RegionSizes->GetValue(id) << std::endl;
  }

  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
