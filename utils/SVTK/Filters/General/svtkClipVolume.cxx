/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClipVolume.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkClipVolume.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkExecutive.h"
#include "svtkFloatArray.h"
#include "svtkGarbageCollector.h"
#include "svtkGenericCell.h"
#include "svtkIdTypeArray.h"
#include "svtkImageData.h"
#include "svtkImplicitFunction.h"
#include "svtkIncrementalPointLocator.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkObjectFactory.h"
#include "svtkOrderedTriangulator.h"
#include "svtkPointData.h"
#include "svtkTetra.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#include "svtkVoxel.h"

svtkStandardNewMacro(svtkClipVolume);
svtkCxxSetObjectMacro(svtkClipVolume, ClipFunction, svtkImplicitFunction);

// Construct with user-specified implicit function; InsideOut turned off; value
// set to 0.0; and generate clip scalars turned off. The merge tolerance is set
// to 0.01.
svtkClipVolume::svtkClipVolume(svtkImplicitFunction* cf)
{
  this->ClipFunction = cf;
  this->InsideOut = 0;
  this->Locator = nullptr;
  this->Value = 0.0;
  this->GenerateClipScalars = 0;
  this->Mixed3DCellGeneration = 1;

  this->GenerateClippedOutput = 0;
  this->MergeTolerance = 0.01;

  this->Triangulator = svtkOrderedTriangulator::New();
  this->Triangulator->PreSortedOn();

  // optional clipped output
  this->SetNumberOfOutputPorts(2);
  svtkUnstructuredGrid* output2 = svtkUnstructuredGrid::New();
  this->GetExecutive()->SetOutputData(1, output2);
  output2->Delete();

  // by default process active point scalars
  this->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, svtkDataSetAttributes::SCALARS);
}

svtkClipVolume::~svtkClipVolume()
{
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }

  this->Triangulator->Delete();
  this->SetClipFunction(nullptr);
}

svtkUnstructuredGrid* svtkClipVolume::GetClippedOutput()
{
  return svtkUnstructuredGrid::SafeDownCast(this->GetExecutive()->GetOutputData(1));
}

// Overload standard modified time function. If Clip functions is modified,
// then this object is modified as well.
svtkMTimeType svtkClipVolume::GetMTime()
{
  svtkMTimeType mTime, time;

  mTime = this->Superclass::GetMTime();

  if (this->Locator != nullptr)
  {
    time = this->Locator->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  if (this->ClipFunction != nullptr)
  {
    time = this->ClipFunction->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }

  return mTime;
}

//
// Clip through volume generating tetrahedra
//
int svtkClipVolume::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkImageData* input = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkUnstructuredGrid* output =
    svtkUnstructuredGrid::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkUnstructuredGrid* clippedOutput = this->GetClippedOutput();
  svtkCellArray* outputConn;
  svtkUnsignedCharArray* outputTypes;
  svtkIdType cellId, newCellId, i;
  int j, k, flip;
  svtkPoints* cellPts;
  svtkDataArray* clipScalars;
  svtkFloatArray* cellScalars;
  svtkPoints* newPoints;
  svtkIdList* cellIds;
  double value, s, x[3], origin[3], spacing[3];
  svtkIdType estimatedSize, numCells = input->GetNumberOfCells();
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkPointData *inPD = input->GetPointData(), *outPD = output->GetPointData();
  svtkCellData *inCD = input->GetCellData(), *outCD = output->GetCellData();
  svtkCellData* clippedCD = clippedOutput->GetCellData();
  svtkCellData* outputCD;
  int dims[3], dimension, numICells, numJCells, numKCells, sliceSize;
  int extOffset;
  int above, below;
  svtkIdList* tetraIds;
  svtkPoints* tetraPts;
  int ii, jj, id, ntetra;
  svtkIdType pts[4];
  svtkIdType npts;
  const svtkIdType* dpts;
  svtkIdType* outputCount;

  svtkDebugMacro(<< "Clipping volume");

  // Initialize self; create output objects
  //
  input->GetDimensions(dims);
  input->GetOrigin(origin);
  input->GetSpacing(spacing);

  extOffset = input->GetExtent()[0] + input->GetExtent()[2] + input->GetExtent()[4];

  for (dimension = 3, i = 0; i < 3; i++)
  {
    if (dims[i] <= 1)
    {
      dimension--;
    }
  }
  if (dimension < 3)
  {
    svtkErrorMacro("This filter only clips 3D volume data");
    return 1;
  }

  if (!this->ClipFunction && this->GenerateClipScalars)
  {
    svtkErrorMacro(<< "Cannot generate clip scalars without clip function");
    return 1;
  }

  // Create objects to hold output of clip operation
  //
  estimatedSize = numCells;
  estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
  if (estimatedSize < 1024)
  {
    estimatedSize = 1024;
  }

  newPoints = svtkPoints::New();
  newPoints->Allocate(estimatedSize / 2, estimatedSize / 2);
  this->NumberOfCells = 0;
  this->Connectivity = svtkCellArray::New();
  this->Connectivity->AllocateEstimate(estimatedSize * 2, 1); // allocate storage for cells
  this->Types = svtkUnsignedCharArray::New();
  this->Types->Allocate(estimatedSize);

  // locator used to merge potentially duplicate points
  if (this->Locator == nullptr)
  {
    this->CreateDefaultLocator();
  }
  this->Locator->InitPointInsertion(newPoints, input->GetBounds());

  // Determine whether we're clipping with input scalars or a clip function
  // and do necessary setup.
  if (this->ClipFunction)
  {
    svtkFloatArray* tmpScalars = svtkFloatArray::New();
    tmpScalars->Allocate(numPts);
    inPD = svtkPointData::New();
    inPD->ShallowCopy(input->GetPointData());
    if (this->GenerateClipScalars)
    {
      inPD->SetScalars(tmpScalars);
    }
    for (i = 0; i < numPts; i++)
    {
      s = this->ClipFunction->FunctionValue(input->GetPoint(i));
      tmpScalars->InsertTuple(i, &s);
    }
    clipScalars = tmpScalars;
  }
  else // using input scalars
  {
    clipScalars = this->GetInputArrayToProcess(0, inputVector);
    if (!clipScalars)
    {
      svtkErrorMacro(<< "Cannot clip without clip function or input scalars");
      return 1;
    }
  }

  if (!this->GenerateClipScalars && !input->GetPointData()->GetScalars())
  {
    outPD->CopyScalarsOff();
  }
  else
  {
    outPD->CopyScalarsOn();
  }
  outPD->InterpolateAllocate(inPD, estimatedSize, estimatedSize / 2);
  outCD->CopyAllocate(inCD, estimatedSize, estimatedSize / 2);
  clippedCD->CopyAllocate(inCD, estimatedSize, estimatedSize / 2);

  // If generating second output, setup clipped output
  if (this->GenerateClippedOutput)
  {
    this->NumberOfClippedCells = 0;
    this->ClippedConnectivity = svtkCellArray::New();
    this->ClippedConnectivity->AllocateEstimate(estimatedSize, 1); // storage for cells
    this->ClippedTypes = svtkUnsignedCharArray::New();
    this->ClippedTypes->Allocate(estimatedSize);
  }

  // perform clipping on voxels - compute appropriate numbers
  value = this->Value;
  numICells = dims[0] - 1;
  numJCells = dims[1] - 1;
  numKCells = dims[2] - 1;
  sliceSize = numICells * numJCells;

  tetraIds = svtkIdList::New();
  tetraIds->Allocate(20);
  cellScalars = svtkFloatArray::New();
  cellScalars->Allocate(8);
  tetraPts = svtkPoints::New();
  tetraPts->Allocate(20);
  svtkGenericCell* cell = svtkGenericCell::New();
  svtkTetra* clipTetra = svtkTetra::New();

  // Interior voxels (i.e., inside the clip region) are tetrahedralized using
  // 5 tetrahedra. This requires swapping the face diagonals on alternating
  // voxels to insure compatibility. Loop over i-j-k directions so that we
  // can control the direction of face diagonals on voxels (i.e., the flip
  // variable). The flip variable also controls the generation of tetrahedra
  // in boundary voxels in ClipTets() and the ordered Delaunay triangulation
  // used in ClipVoxel().
  int abort = 0;
  for (k = 0; k < numKCells && !abort; k++)
  {
    // Check for progress and abort on every z-slice
    this->UpdateProgress(static_cast<double>(k) / numKCells);
    abort = this->GetAbortExecute();
    for (j = 0; j < numJCells; j++)
    {
      for (i = 0; i < numICells; i++)
      {
        flip = (extOffset + i + j + k) & 0x1;
        cellId = i + j * numICells + k * sliceSize;

        input->GetCell(cellId, cell);
        if (cell->GetCellType() == SVTK_EMPTY_CELL)
        {
          continue;
        }
        cellPts = cell->GetPoints();
        cellIds = cell->GetPointIds();

        // gather scalar values for the cell and keep
        for (above = below = 0, ii = 0; ii < 8; ii++)
        {
          s = clipScalars->GetComponent(cellIds->GetId(ii), 0);
          cellScalars->SetComponent(ii, 0, s);
          if (s >= value)
          {
            above = 1;
          }
          else
          {
            below = 1;
          }
        }

        // take into account inside/out flag
        if (this->InsideOut)
        {
          above = !above;
          below = !below;
        }

        // See whether voxel is fully inside or outside and triangulate
        // according to the flup variable.
        if ((above && !below) || (this->GenerateClippedOutput && (below && !above)))
        {
          cell->Triangulate(flip, tetraIds, tetraPts);
          ntetra = tetraPts->GetNumberOfPoints() / 4;

          if (above && !below)
          {
            outputConn = this->Connectivity;
            outputTypes = this->Types;
            outputCount = &this->NumberOfCells;
            outputCD = outCD;
          }
          else
          {
            outputConn = this->ClippedConnectivity;
            outputTypes = this->ClippedTypes;
            outputCount = &this->NumberOfClippedCells;
            outputCD = clippedCD;
          }

          for (ii = 0; ii < ntetra; ii++)
          {
            id = ii * 4;
            for (jj = 0; jj < 4; jj++)
            {
              tetraPts->GetPoint(id + jj, x);
              if (this->Locator->InsertUniquePoint(x, pts[jj]))
              {
                outPD->CopyData(inPD, tetraIds->GetId(id + jj), pts[jj]);
              }
            }
            newCellId = outputConn->InsertNextCell(4, pts);
            (*outputCount)++;
            outputConn->GetNextCell(npts, dpts); // updates traversal location
            outputTypes->InsertNextValue(SVTK_TETRA);
            outputCD->CopyData(inCD, cellId, newCellId);
          } // for each tetra produced by triangulation
        }

        else if (above == below) // clipped voxel, have to triangulate
        {
          if (this->Mixed3DCellGeneration) // use svtkTetra clipping templates
          {
            cell->Triangulate(flip, tetraIds, tetraPts);
            this->ClipTets(value, clipTetra, clipScalars, cellScalars, tetraIds, tetraPts, inPD,
              outPD, inCD, cellId, outCD, clippedCD, this->InsideOut);
          }
          else // use svtkOrderedTriangulator to produce tetrahedra
          {
            this->ClipVoxel(value, cellScalars, flip, origin, spacing, cellIds, cellPts, inPD,
              outPD, inCD, cellId, outCD, clippedCD);
          }
        } // using ordered triangulator
      }   // for i
    }     // for j
  }       // for k

  // Create the output
  output->SetPoints(newPoints);
  output->SetCells(this->Types, this->Connectivity);
  this->Types->Delete();
  this->Connectivity->Delete();
  output->Squeeze();
  svtkDebugMacro(<< "Created: " << newPoints->GetNumberOfPoints() << " points, "
                << output->GetNumberOfCells() << " tetra");

  if (this->GenerateClippedOutput)
  {
    clippedOutput->SetPoints(newPoints);
    clippedOutput->SetCells(this->ClippedTypes, this->ClippedConnectivity);
    this->ClippedTypes->Delete();
    this->ClippedConnectivity->Delete();
    clippedOutput->GetPointData()->PassData(outPD);
    clippedOutput->Squeeze();
    svtkDebugMacro(<< "Created (clipped output): " << clippedOutput->GetNumberOfCells() << " tetra");
  }

  // Update ourselves.  Because we don't know upfront how many cells
  // we've created, take care to reclaim memory.
  //
  if (this->ClipFunction)
  {
    clipScalars->Delete();
    inPD->Delete();
  }

  // Clean up
  newPoints->Delete();
  cell->Delete();
  tetraIds->Delete();
  tetraPts->Delete();
  cellScalars->Delete();
  clipTetra->Delete();

  this->Locator->Initialize(); // release any extra memory

  return 1;
}

// Method to triangulate and clip voxel using svtkTetra::Clip() method.
// This produces a mixed mesh of tetrahedra and wedges but it is faster
// than using the ordered triangulator. It works by using the usual
// alternating five tetrahedra template per voxel, and then using the
// svtkTetra::Clip() method to produce the output.
//
void svtkClipVolume::ClipTets(double value, svtkTetra* clipTetra, svtkDataArray* clipScalars,
  svtkDataArray* cellScalars, svtkIdList* tetraIds, svtkPoints* tetraPts, svtkPointData* inPD,
  svtkPointData* outPD, svtkCellData* inCD, svtkIdType cellId, svtkCellData* outCD,
  svtkCellData* clippedCD, int insideOut)
{
  // Tessellate this cell as if it were inside
  svtkIdType ntetra = tetraPts->GetNumberOfPoints() / 4;
  int i, id, j, k, numNew;
  svtkIdType npts = 0;
  const svtkIdType* pts;

  // Clip each tetrahedron
  for (i = 0; i < ntetra; i++)
  {
    id = i * 4;
    for (j = 0; j < 4; j++)
    {
      clipTetra->PointIds->SetId(j, tetraIds->GetId(id + j));
      clipTetra->Points->SetPoint(j, tetraPts->GetPoint(id + j));
      cellScalars->SetComponent(j, 0, clipScalars->GetComponent(tetraIds->GetId(id + j), 0));
    }
    clipTetra->Clip(value, cellScalars, this->Locator, this->Connectivity, inPD, outPD, inCD,
      cellId, outCD, insideOut);
    numNew = this->Connectivity->GetNumberOfCells() - this->NumberOfCells;
    this->NumberOfCells = this->Connectivity->GetNumberOfCells();
    for (k = 0; k < numNew; k++)
    {
      this->Connectivity->GetNextCell(npts, pts);
      this->Types->InsertNextValue((npts == 4 ? SVTK_TETRA : SVTK_WEDGE));
    }

    if (this->GenerateClippedOutput)
    {
      clipTetra->Clip(value, cellScalars, this->Locator, this->ClippedConnectivity, inPD, outPD,
        inCD, cellId, clippedCD, !insideOut);
      numNew = this->ClippedConnectivity->GetNumberOfCells() - this->NumberOfClippedCells;
      this->NumberOfClippedCells = this->ClippedConnectivity->GetNumberOfCells();
      for (k = 0; k < numNew; k++)
      {
        this->ClippedConnectivity->GetNextCell(npts, pts);
        this->ClippedTypes->InsertNextValue((npts == 4 ? SVTK_TETRA : SVTK_WEDGE));
      }
    }
  }
}

// Method to triangulate and clip voxel using ordered Delaunay
// triangulation to produce tetrahedra. Voxel is initially triangulated
// using 8 voxel corner points inserted in order (to control direction
// of face diagonals). Then edge intersection points are injected into the
// triangulation. The ordering controls the orientation of any face
// diagonals.
void svtkClipVolume::ClipVoxel(double value, svtkDataArray* cellScalars, int flip,
  double svtkNotUsed(origin)[3], double spacing[3], svtkIdList* cellIds, svtkPoints* cellPts,
  svtkPointData* inPD, svtkPointData* outPD, svtkCellData* inCD, svtkIdType cellId, svtkCellData* outCD,
  svtkCellData* clippedCD)
{
  double x[3], s1, s2, t, voxelOrigin[3];
  double bounds[6], p1[3], p2[3];
  int i, k, edgeNum, numPts, numNew;
  svtkIdType id;
  svtkIdType ptId;
  svtkIdType npts;
  const svtkIdType* pts;
  static int edges[12][2] = { { 0, 1 }, { 2, 3 }, { 4, 5 }, { 6, 7 }, { 0, 2 }, { 1, 3 }, { 4, 6 },
    { 5, 7 }, { 0, 4 }, { 1, 5 }, { 2, 6 }, { 3, 7 } };
  static const int order[2][8] = { { 0, 3, 5, 6, 1, 2, 4, 7 },
    { 1, 2, 4, 7, 0, 3, 5, 6 } }; // injection order based on flip

  // compute bounds for voxel and initialize
  cellPts->GetPoint(0, voxelOrigin);
  for (i = 0; i < 3; i++)
  {
    bounds[2 * i] = voxelOrigin[i];
    bounds[2 * i + 1] = voxelOrigin[i] + spacing[i];
  }

  // Initialize Delaunay insertion process with voxel triangulation.
  // No more than 20 points (8 corners + 12 edges) may be inserted.
  this->Triangulator->InitTriangulation(bounds, 20);

  // Inject ordered voxel corner points into triangulation. Recall
  // that the PreSortedOn() flag was set in the triangulator.
  int type;
  svtkIdType internalId[8]; // used to merge points if nearby edge intersection
  for (numPts = 0; numPts < 8; numPts++)
  {
    ptId = order[flip][numPts];

    // Currently all points are injected because of the possibility
    // of intersection point merging.
    s1 = cellScalars->GetComponent(ptId, 0);
    if ((s1 >= value && !this->InsideOut) || (s1 < value && this->InsideOut))
    {
      type = 0; // inside
    }
    else
    {
      // type 1 is "outside"; type 4 is don't insert
      type = (this->GenerateClippedOutput ? 1 : 4);
    }

    cellPts->GetPoint(ptId, x);
    if (this->Locator->InsertUniquePoint(x, id))
    {
      outPD->CopyData(inPD, cellIds->GetId(ptId), id);
    }
    internalId[ptId] = this->Triangulator->InsertPoint(id, x, x, type);
  } // for eight voxel corner points

  // For each edge intersection point, insert into triangulation. Edge
  // intersections come from clipping value. Have to be careful of
  // intersections near existing points (causes bad Delaunay behavior).
  for (edgeNum = 0; edgeNum < 12; edgeNum++)
  {
    s1 = cellScalars->GetComponent(edges[edgeNum][0], 0);
    s2 = cellScalars->GetComponent(edges[edgeNum][1], 0);

    if ((s1 < value && s2 >= value) || (s1 >= value && s2 < value))
    {

      t = (value - s1) / (s2 - s1);

      // Check to see whether near the intersection is near a voxel corner.
      // If so,have to merge requiring a change of type to type=boundary.
      if (t < this->MergeTolerance)
      {
        this->Triangulator->UpdatePointType(internalId[edges[edgeNum][0]], 2);
        continue;
      }
      else if (t > (1.0 - this->MergeTolerance))
      {
        this->Triangulator->UpdatePointType(internalId[edges[edgeNum][1]], 2);
        continue;
      }

      // generate edge intersection point
      cellPts->GetPoint(edges[edgeNum][0], p1);
      cellPts->GetPoint(edges[edgeNum][1], p2);
      for (i = 0; i < 3; i++)
      {
        x[i] = p1[i] + t * (p2[i] - p1[i]);
      }

      // Incorporate point into output and interpolate edge data as necessary
      if (this->Locator->InsertUniquePoint(x, ptId))
      {
        outPD->InterpolateEdge(
          inPD, ptId, cellIds->GetId(edges[edgeNum][0]), cellIds->GetId(edges[edgeNum][1]), t);
      }

      // Insert into Delaunay triangulation
      this->Triangulator->InsertPoint(ptId, x, x, 2);

    } // if edge intersects value
  }   // for all edges

  // triangulate the points
  this->Triangulator->Triangulate();

  // Add the triangulation to the mesh
  svtkIdType newCellId;
  this->Triangulator->AddTetras(0, this->Connectivity);
  numNew = this->Connectivity->GetNumberOfCells() - this->NumberOfCells;
  this->NumberOfCells = this->Connectivity->GetNumberOfCells();
  for (k = 0; k < numNew; k++)
  {
    newCellId = this->Connectivity->GetTraversalCellId();
    this->Connectivity->GetNextCell(npts, pts); // updates traversal location
    this->Types->InsertNextValue(SVTK_TETRA);
    outCD->CopyData(inCD, cellId, newCellId);
  }

  if (this->GenerateClippedOutput)
  {
    this->Triangulator->AddTetras(1, this->ClippedConnectivity);
    numNew = this->ClippedConnectivity->GetNumberOfCells() - this->NumberOfClippedCells;
    this->NumberOfClippedCells = this->ClippedConnectivity->GetNumberOfCells();
    for (k = 0; k < numNew; k++)
    {
      newCellId = this->ClippedConnectivity->GetTraversalCellId();
      this->ClippedConnectivity->GetNextCell(npts, pts);
      this->ClippedTypes->InsertNextValue(SVTK_TETRA);
      clippedCD->CopyData(inCD, cellId, newCellId);
    }
  }
}

// Specify a spatial locator for merging points. By default,
// an instance of svtkMergePoints is used.
void svtkClipVolume::SetLocator(svtkIncrementalPointLocator* locator)
{
  if (this->Locator == locator)
  {
    return;
  }
  if (this->Locator)
  {
    this->Locator->UnRegister(this);
    this->Locator = nullptr;
  }

  if (locator)
  {
    locator->Register(this);
  }

  this->Locator = locator;
  this->Modified();
}

void svtkClipVolume::CreateDefaultLocator()
{
  if (this->Locator == nullptr)
  {
    this->Locator = svtkMergePoints::New();
  }
}

int svtkClipVolume::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

void svtkClipVolume::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->ClipFunction)
  {
    os << indent << "Clip Function: " << this->ClipFunction << "\n";
  }
  else
  {
    os << indent << "Clip Function: (none)\n";
  }
  os << indent << "InsideOut: " << (this->InsideOut ? "On\n" : "Off\n");
  os << indent << "Value: " << this->Value << "\n";
  os << indent << "Merge Tolerance: " << this->MergeTolerance << "\n";
  if (this->Locator)
  {
    os << indent << "Locator: " << this->Locator << "\n";
  }
  else
  {
    os << indent << "Locator: (none)\n";
  }

  os << indent << "Generate Clip Scalars: " << (this->GenerateClipScalars ? "On\n" : "Off\n");

  os << indent << "Generate Clipped Output: " << (this->GenerateClippedOutput ? "On\n" : "Off\n");

  os << indent << "Mixed 3D Cell Type: " << (this->Mixed3DCellGeneration ? "On\n" : "Off\n");
}

//----------------------------------------------------------------------------
void svtkClipVolume::ReportReferences(svtkGarbageCollector* collector)
{
  this->Superclass::ReportReferences(collector);
  // These filters share our input and are therefore involved in a
  // reference loop.
  svtkGarbageCollectorReport(collector, this->ClipFunction, "ClipFunction");
}
