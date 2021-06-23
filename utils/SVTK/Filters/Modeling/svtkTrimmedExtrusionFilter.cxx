/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTrimmedExtrusionFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTrimmedExtrusionFilter.h"

#include "svtkAbstractCellLocator.h"
#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkExecutive.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkStaticCellLocator.h"

#include <cmath>

svtkStandardNewMacro(svtkTrimmedExtrusionFilter);
svtkCxxSetObjectMacro(svtkTrimmedExtrusionFilter, Locator, svtkAbstractCellLocator);

namespace
{

//----------------------------------------------------------------------------
// The threaded core of the algorithm.
template <typename T>
struct ExtrudePoints
{
  svtkIdType NPts;
  T* InPoints;
  T* Points;
  unsigned char* Hits;
  svtkAbstractCellLocator* Locator;
  double ExtrusionDirection[3];
  double BoundsCenter[3];
  double BoundsLength;
  double Tol;

  // Don't want to allocate working arrays on every thread invocation. Thread local
  // storage eliminates lots of new/delete.
  svtkSMPThreadLocalObject<svtkGenericCell> Cell;

  ExtrudePoints(svtkIdType npts, T* inPts, T* points, unsigned char* hits,
    svtkAbstractCellLocator* loc, double ed[3], double bds[6])
    : NPts(npts)
    , InPoints(inPts)
    , Points(points)
    , Hits(hits)
    , Locator(loc)
  {
    this->ExtrusionDirection[0] = ed[0];
    this->ExtrusionDirection[1] = ed[1];
    this->ExtrusionDirection[2] = ed[2];
    svtkMath::Normalize(this->ExtrusionDirection);

    this->BoundsCenter[0] = (bds[0] + bds[1]) / 2.0;
    this->BoundsCenter[1] = (bds[2] + bds[3]) / 2.0;
    this->BoundsCenter[2] = (bds[4] + bds[5]) / 2.0;

    this->BoundsLength = sqrt((bds[1] - bds[0]) * (bds[1] - bds[0]) +
      (bds[3] - bds[2]) * (bds[3] - bds[2]) + (bds[5] - bds[4]) * (bds[5] - bds[4]));

    this->Tol = 0.000001 * this->BoundsLength;
  }

  void Initialize() {}

  void operator()(svtkIdType ptId, svtkIdType endPtId)
  {
    const T* xi = this->InPoints + 3 * ptId;
    T* x = this->Points + 3 * ptId;
    T* xo = this->Points + 3 * (this->NPts + ptId);
    double len, p0[3], p1[3];
    const double* ed = this->ExtrusionDirection;
    double t, pc[3], xint[3];
    svtkIdType cellId;
    int subId;
    unsigned char* hits = this->Hits + ptId;
    svtkGenericCell*& cell = this->Cell.Local();

    for (; ptId < endPtId; ++ptId, xi += 3, x += 3, xo += 3, ++hits)
    {
      // Copy input points to output
      x[0] = xi[0];
      x[1] = xi[1];
      x[2] = xi[2];

      // Find a extrusion ray of appropriate length
      len = sqrt((x[0] - this->BoundsCenter[0]) * (x[0] - this->BoundsCenter[0]) +
              (x[1] - this->BoundsCenter[1]) * (x[1] - this->BoundsCenter[1]) +
              (x[2] - this->BoundsCenter[2]) * (x[2] - this->BoundsCenter[2])) +
        this->BoundsLength;

      p0[0] = x[0] - len * ed[0];
      p0[1] = x[1] - len * ed[1];
      p0[2] = x[2] - len * ed[2];
      p1[0] = x[0] + len * ed[0];
      p1[1] = x[1] + len * ed[1];
      p1[2] = x[2] + len * ed[2];

      // Intersect the surface and update whether a successful intersection hit or not
      *hits = this->Locator->IntersectWithLine(p0, p1, this->Tol, t, xint, pc, subId, cellId, cell);
      if (*hits > 0)
      {
        xo[0] = static_cast<T>(xint[0]);
        xo[1] = static_cast<T>(xint[1]);
        xo[2] = static_cast<T>(xint[2]);
      }
      else
      {
        xo[0] = xi[0];
        xo[1] = xi[1];
        xo[2] = xi[2];
      }
    }
  }

  void Reduce() {}

  static void Execute(svtkIdType numPts, T* inPts, T* points, unsigned char* hits,
    svtkAbstractCellLocator* loc, double ed[3], double bds[6])
  {
    ExtrudePoints extrude(numPts, inPts, points, hits, loc, ed, bds);
    svtkSMPTools::For(0, numPts, extrude);
  }
}; // ExtrudePoints

} // anonymous namespace

//-----------------------------------------------------------------------------
// Create object with normal extrusion type, capping on, scale factor=1.0,
// vector (0,0,1), and extrusion point (0,0,0).
svtkTrimmedExtrusionFilter::svtkTrimmedExtrusionFilter()
{
  this->SetNumberOfInputPorts(2);

  this->Capping = 1;

  this->ExtrusionDirection[0] = 0.0;
  this->ExtrusionDirection[1] = 0.0;
  this->ExtrusionDirection[2] = 1.0;

  this->ExtrusionStrategy = svtkTrimmedExtrusionFilter::BOUNDARY_EDGES;
  this->CappingStrategy = svtkTrimmedExtrusionFilter::MAXIMUM_DISTANCE;

  this->Locator = nullptr;
}

//-----------------------------------------------------------------------------
// Destructor
svtkTrimmedExtrusionFilter::~svtkTrimmedExtrusionFilter()
{
  this->SetLocator(nullptr);
}

//-----------------------------------------------------------------------------
int svtkTrimmedExtrusionFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* in2Info = inputVector[1]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  svtkDebugMacro(<< "Executing trimmed extrusion");

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* surface = svtkPolyData::SafeDownCast(in2Info->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (!input || !output)
  {
    svtkErrorMacro(<< "Missing input and/or output!");
    return 1;
  }

  if (!surface)
  {
    svtkErrorMacro(<< "Missing trim surface!");
    return 1;
  }
  if (surface->GetNumberOfPoints() < 1 || surface->GetNumberOfCells() < 1)
  {
    svtkErrorMacro(<< "Empty trim surface!");
    return 1;
  }

  // Initialize / check input
  //
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();

  if (numPts < 1 || numCells < 1)
  {
    svtkErrorMacro(<< "No data to extrude!");
    return 1;
  }

  if (svtkMath::Norm(this->ExtrusionDirection) <= 0.0)
  {
    svtkErrorMacro(<< "Must have nonzero extrusion direction");
    return 1;
  }

  // Generate the new points. Basically replicate points, except the new
  // point lies at the intersection of ray (in the extrusion direction)
  // against the trim surface. Also keep track if there are misses and use
  // this information later for capping (if necessary).
  svtkPointData* pd = input->GetPointData();
  svtkPointData* outputPD = output->GetPointData();
  outputPD->CopyNormalsOff();
  outputPD->CopyAllocate(pd, 2 * numPts);
  for (svtkIdType i = 0; i < numPts; ++i)
  {
    outputPD->CopyData(pd, i, i);
    outputPD->CopyData(pd, i, numPts + i);
  }

  svtkPoints* newPts = svtkPoints::New();
  newPts->SetDataType(input->GetPoints()->GetDataType());
  newPts->SetNumberOfPoints(2 * numPts);
  output->SetPoints(newPts);

  // Extrude the points by intersecting with the trim surface. Use a cell
  // locator to accelerate intersection operations.
  //
  if (this->Locator == nullptr)
  {
    this->Locator = svtkStaticCellLocator::New();
  }
  this->Locator->SetDataSet(surface);
  this->Locator->BuildLocator();
  double surfaceBds[6];
  surface->GetBounds(surfaceBds);

  // This performs the intersection of the extrusion ray. If a hit, the xyz
  // of the intersection point is used and hit[i] set to 1. If not, the xyz
  // is set to the xyz of the generating point and hit[i] remains 0. Later we
  // can use hit value to control the extrusion.
  unsigned char* hits = new unsigned char[numPts];
  void* inPtr = input->GetPoints()->GetVoidPointer(0);
  void* outPtr = newPts->GetVoidPointer(0);
  switch (newPts->GetDataType())
  {
    svtkTemplateMacro(ExtrudePoints<SVTK_TT>::Execute(numPts, (SVTK_TT*)inPtr, (SVTK_TT*)outPtr, hits,
      this->Locator, this->ExtrusionDirection, surfaceBds));
  }

  // Prepare to generate the topology. Different topolgy is built depending
  // on extrusion strategy
  if (this->ExtrusionStrategy == svtkTrimmedExtrusionFilter::BOUNDARY_EDGES)
  {
    input->BuildLinks();
  }
  else // every edge is swept
  {
    input->BuildCells();
  }

  // Depending on the capping strategy, update the point coordinates. This has to be
  // done on a cell-by-cell basis. The adjustment is done in place.
  if (this->CappingStrategy != svtkTrimmedExtrusionFilter::INTERSECTION)
  {
    this->AdjustPoints(input, numPts, numCells, hits, newPts);
  }

  // Now generate the topology.
  this->ExtrudeEdges(input, output, numPts, numCells);

  // Cleanup, add the points to the output and clean up.
  newPts->Delete();
  output->Squeeze();

  return 1;
}

//----------------------------------------------------------------------------
// Based on the capping strategy, adjust the point coordinates along the
// extrusion ray. This requires looping over all cells, grabbing the cap
// points, and then adjusting them as appropriate. Note this could be
// templated / sped up if necessary.
void svtkTrimmedExtrusionFilter::AdjustPoints(
  svtkPolyData* input, svtkIdType numPts, svtkIdType numCells, unsigned char* hits, svtkPoints* newPts)
{
  svtkIdType cellId;
  svtkIdType npts;
  const svtkIdType* ptIds;
  svtkIdType pId;
  svtkIdType i;
  double len, sum, min, max, p0[3], p1[3], ed[3];
  double p10[3], dir, minDir = 1.0, maxDir = 1.0, mDir = 1.0;
  svtkIdType numHits;

  ed[0] = this->ExtrusionDirection[0];
  ed[1] = this->ExtrusionDirection[1];
  ed[2] = this->ExtrusionDirection[2];
  svtkMath::Normalize(ed);

  for (cellId = 0; cellId < numCells; ++cellId)
  {
    input->GetCellPoints(cellId, npts, ptIds);

    // Gather information about cell
    min = SVTK_FLOAT_MAX;
    max = SVTK_FLOAT_MIN;
    sum = 0.0;
    numHits = 0;
    for (i = 0; i < npts; ++i)
    {
      pId = ptIds[i];
      if (hits[pId] > 0)
      {
        numHits++;
        newPts->GetPoint(pId, p0);
        newPts->GetPoint(numPts + pId, p1);

        svtkMath::Subtract(p1, p0, p10);
        dir = svtkMath::Dot(p10, ed);
        dir = (dir > 0.0 ? 1.0 : -1.0);

        len = sqrt(svtkMath::Distance2BetweenPoints(p0, p1));

        if (len < min)
        {
          min = len;
          minDir = dir;
        }
        if (len > max)
        {
          max = len;
          maxDir = dir;
        }
        sum += dir * len;
      }
    } // over primitive points

    // Adjust points if there was an intersection. Note that the extrusion
    // intersection is along the estrusion ray in either the negative or
    // positive direction.
    if (numHits > 0)
    {
      len = fabs(sum / static_cast<double>(numHits));
      if (this->CappingStrategy == svtkTrimmedExtrusionFilter::AVERAGE_DISTANCE)
      {
        mDir = 1.0;
      }
      else if (this->CappingStrategy == svtkTrimmedExtrusionFilter::MINIMUM_DISTANCE)
      {
        mDir = minDir;
      }
      else // if ( this->CappingStrategy == svtkTrimmedExtrusionFilter::MAXIMUM_DISTANCE )
      {
        mDir = maxDir;
      }

      for (i = 0; i < npts; ++i)
      {
        pId = ptIds[i];
        newPts->GetPoint(pId, p0);
        p1[0] = p0[0] + mDir * len * ed[0];
        p1[1] = p0[1] + mDir * len * ed[1];
        p1[2] = p0[2] + mDir * len * ed[2];
        newPts->SetPoint(numPts + pId, p1);
      }
    } // if valid polygon

  } // for all cells
}

//----------------------------------------------------------------------------
svtkIdType svtkTrimmedExtrusionFilter::GetNeighborCount(
  svtkPolyData* input, svtkIdType inCellId, svtkIdType p1, svtkIdType p2, svtkIdList* cellIds)
{
  if (this->ExtrusionStrategy == svtkTrimmedExtrusionFilter::BOUNDARY_EDGES)
  {
    input->GetCellEdgeNeighbors(inCellId, p1, p2, cellIds);
    return cellIds->GetNumberOfIds();
  }
  else // every edge is swept
  {
    return 0;
  }
}

//----------------------------------------------------------------------------
// Somewhat modified from svtkLinearExtrusionFilter
void svtkTrimmedExtrusionFilter::ExtrudeEdges(
  svtkPolyData* input, svtkPolyData* output, svtkIdType numPts, svtkIdType numCells)
{
  svtkIdType inCellId, outCellId;
  int numEdges, dim;
  const svtkIdType* pts = nullptr;
  svtkIdType npts = 0;
  svtkIdType ptId, ncells, p1, p2;
  svtkIdType i, j;
  svtkCellArray *newLines = nullptr, *newPolys = nullptr, *newStrips = nullptr;
  svtkCell* edge;
  svtkIdList *cellIds, *cellPts;
  cellIds = svtkIdList::New();

  // Here is a big pain about ordering of cells. (Copy CellData)
  svtkIdList* lineIds;
  svtkIdList* polyIds;
  svtkIdList* stripIds;

  // Build cell data structure. Create a local copy
  svtkCellArray* inVerts = input->GetVerts();
  svtkCellArray* inLines = input->GetLines();
  svtkCellArray* inPolys = input->GetPolys();
  svtkCellArray* inStrips = input->GetStrips();

  // Allocate memory for output. We don't copy normals because surface geometry
  // is modified. Copy all points - this is the usual requirement and it makes
  // creation of skirt much easier.
  output->GetCellData()->CopyNormalsOff();
  output->GetCellData()->CopyAllocate(input->GetCellData(), 3 * input->GetNumberOfCells());

  if ((ncells = inVerts->GetNumberOfCells()) > 0)
  {
    newLines = svtkCellArray::New();
    newLines->AllocateEstimate(ncells, 2);
  }

  // arbitrary initial allocation size
  ncells = inLines->GetNumberOfCells() + inPolys->GetNumberOfCells() / 10 +
    inStrips->GetNumberOfCells() / 10;
  ncells = (ncells < 100 ? 100 : ncells);

  newPolys = svtkCellArray::New();
  newPolys->AllocateCopy(inPolys);

  svtkIdType progressInterval = numPts / 10 + 1;
  int abort = 0;

  // We need the cellid to copy cell data. Skip points and lines.
  inCellId = outCellId = 0;
  if (input->GetVerts())
  {
    inCellId += input->GetVerts()->GetNumberOfCells();
  }
  if (input->GetLines())
  {
    inCellId += input->GetLines()->GetNumberOfCells();
  }
  // We need to keep track of input cell ids used to generate
  // output cells so that we can copy cell data at the end.
  // We do not know how many lines, polys and strips we will get
  // before hand.
  lineIds = svtkIdList::New();
  polyIds = svtkIdList::New();
  stripIds = svtkIdList::New();

  // If capping is on, copy 2D cells to output (plus create cap)
  //
  if (this->Capping)
  {
    if (inPolys->GetNumberOfCells() > 0)
    {
      for (inPolys->InitTraversal(); inPolys->GetNextCell(npts, pts);)
      {
        newPolys->InsertNextCell(npts, pts);
        polyIds->InsertNextId(inCellId);
        newPolys->InsertNextCell(npts);
        for (i = 0; i < npts; i++)
        {
          newPolys->InsertCellPoint(pts[i] + numPts);
        }
        polyIds->InsertNextId(inCellId);
        ++inCellId;
      }
    }

    if (inStrips->GetNumberOfCells() > 0)
    {
      newStrips = svtkCellArray::New();
      newStrips->AllocateEstimate(ncells, 4);
      for (inStrips->InitTraversal(); inStrips->GetNextCell(npts, pts);)
      {
        newStrips->InsertNextCell(npts, pts);
        stripIds->InsertNextId(inCellId);
        newStrips->InsertNextCell(npts);
        for (i = 0; i < npts; i++)
        {
          newStrips->InsertCellPoint(pts[i] + numPts);
        }
        stripIds->InsertNextId(inCellId);
        ++inCellId;
      }
    }
  }
  this->UpdateProgress(0.4);

  // Loop over all polygons and triangle strips searching for boundary edges.
  // If boundary edge found, extrude quad polygons. (Since the extrusion is
  // linear and guaranteed planar, triangle are not needed.)
  //
  progressInterval = numCells / 10 + 1;
  svtkGenericCell* cell = svtkGenericCell::New();
  for (inCellId = 0; inCellId < numCells && !abort; inCellId++)
  {
    if (!(inCellId % progressInterval)) // manage progress / early abort
    {
      this->UpdateProgress(0.4 + 0.6 * inCellId / numCells);
      abort = this->GetAbortExecute();
    }

    input->GetCell(inCellId, cell);
    cellPts = cell->GetPointIds();

    if ((dim = cell->GetCellDimension()) == 0) // create lines from points
    {
      for (i = 0; i < cellPts->GetNumberOfIds(); i++)
      {
        newLines->InsertNextCell(2);
        ptId = cellPts->GetId(i);
        newLines->InsertCellPoint(ptId);
        newLines->InsertCellPoint(ptId + numPts);
        lineIds->InsertNextId(inCellId);
      }
    }

    else if (dim == 1) // create strips from lines
    {
      for (i = 0; i < (cellPts->GetNumberOfIds() - 1); i++)
      {
        p1 = cellPts->GetId(i);
        p2 = cellPts->GetId(i + 1);
        newPolys->InsertNextCell(4);
        newPolys->InsertCellPoint(p1);
        newPolys->InsertCellPoint(p2);
        newPolys->InsertCellPoint(p2 + numPts);
        newPolys->InsertCellPoint(p1 + numPts);
        polyIds->InsertNextId(inCellId);
      }
    }

    else if (dim == 2) // create strips from boundary edges
    {
      numEdges = cell->GetNumberOfEdges();
      for (i = 0; i < numEdges; i++)
      {
        edge = cell->GetEdge(i);
        for (j = 0; j < (edge->GetNumberOfPoints() - 1); j++)
        {
          p1 = edge->PointIds->GetId(j);
          p2 = edge->PointIds->GetId(j + 1);

          // Check if this is a boundary edge
          if (this->GetNeighborCount(input, inCellId, p1, p2, cellIds) < 1)
          {
            newPolys->InsertNextCell(4);
            newPolys->InsertCellPoint(p1);
            newPolys->InsertCellPoint(p2);
            newPolys->InsertCellPoint(p2 + numPts);
            newPolys->InsertCellPoint(p1 + numPts);
            polyIds->InsertNextId(inCellId);
          }
        } // for each sub-edge
      }   // for each edge
    }     // for each polygon or triangle strip
  }       // for each cell
  cell->Delete();

  // Now Copy cell data.
  outCellId = 0;
  j = lineIds->GetNumberOfIds();
  for (i = 0; i < j; ++i)
  {
    output->GetCellData()->CopyData(input->GetCellData(), lineIds->GetId(i), outCellId);
    ++outCellId;
  }
  j = polyIds->GetNumberOfIds();
  for (i = 0; i < j; ++i)
  {
    output->GetCellData()->CopyData(input->GetCellData(), polyIds->GetId(i), outCellId);
    ++outCellId;
  }
  j = stripIds->GetNumberOfIds();
  for (i = 0; i < j; ++i)
  {
    output->GetCellData()->CopyData(input->GetCellData(), stripIds->GetId(i), outCellId);
    ++outCellId;
  }
  lineIds->Delete();
  stripIds->Delete();
  polyIds->Delete();
  polyIds = nullptr;

  // Send data to output and release memory
  cellIds->Delete();

  if (newLines)
  {
    output->SetLines(newLines);
    newLines->Delete();
  }

  output->SetPolys(newPolys);
  newPolys->Delete();

  if (newStrips)
  {
    output->SetStrips(newStrips);
    newStrips->Delete();
  }
}

//----------------------------------------------------------------------------
// Specify the trim surface
void svtkTrimmedExtrusionFilter::SetTrimSurfaceConnection(svtkAlgorithmOutput* algOutput)
{
  this->SetInputConnection(1, algOutput);
}

//----------------------------------------------------------------------------
// Specify a source object at a specified table location.
void svtkTrimmedExtrusionFilter::SetTrimSurfaceData(svtkPolyData* pd)
{
  this->SetInputData(1, pd);
}

//----------------------------------------------------------------------------
// Get a pointer to a source object at a specified table location.
svtkPolyData* svtkTrimmedExtrusionFilter::GetTrimSurface()
{
  return svtkPolyData::SafeDownCast(this->GetExecutive()->GetInputData(1, 0));
}

//----------------------------------------------------------------------------
svtkPolyData* svtkTrimmedExtrusionFilter::GetTrimSurface(svtkInformationVector* sourceInfo)
{
  svtkInformation* info = sourceInfo->GetInformationObject(1);
  if (!info)
  {
    return nullptr;
  }
  return svtkPolyData::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));
}

//----------------------------------------------------------------------------
int svtkTrimmedExtrusionFilter::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_IS_REPEATABLE(), 0);
  info->Set(svtkAlgorithm::INPUT_IS_OPTIONAL(), 0);
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPolyData");
  return 1;
}

//-----------------------------------------------------------------------------
void svtkTrimmedExtrusionFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Extrusion Direction: (" << this->ExtrusionDirection[0] << ", "
     << this->ExtrusionDirection[1] << ", " << this->ExtrusionDirection[2] << ")\n";

  os << indent << "Capping: " << (this->Capping ? "On\n" : "Off\n");

  os << indent << "Extrusion Strategy: " << this->ExtrusionStrategy << "\n";
  os << indent << "Capping Strategy: " << this->CappingStrategy << "\n";

  os << indent << "Locator: " << this->Locator << "\n";
}
