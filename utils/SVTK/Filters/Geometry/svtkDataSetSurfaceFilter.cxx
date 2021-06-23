/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataSetSurfaceFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDataSetSurfaceFilter.h"

#include "svtkBezierQuadrilateral.h"
#include "svtkBezierTriangle.h"
#include "svtkCell.h"
#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellIterator.h"
#include "svtkCellTypes.h"
#include "svtkDoubleArray.h"
#include "svtkGenericCell.h"
#include "svtkHexahedron.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLagrangeQuadrilateral.h"
#include "svtkLagrangeTriangle.h"
#include "svtkMergePoints.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPyramid.h"
#include "svtkRectilinearGrid.h"
#include "svtkRectilinearGridGeometryFilter.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredData.h"
#include "svtkStructuredGrid.h"
#include "svtkStructuredGridGeometryFilter.h"
#include "svtkStructuredPoints.h"
#include "svtkTetra.h"
#include "svtkUniformGrid.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#include "svtkUnstructuredGridBase.h"
#include "svtkUnstructuredGridGeometryFilter.h"
#include "svtkVector.h"
#include "svtkVoxel.h"
#include "svtkWedge.h"
#include <algorithm>
#include <cassert>
#include <unordered_map>

static inline int sizeofFastQuad(int numPts)
{
  const int qsize = sizeof(svtkFastGeomQuad);
  const int sizeId = sizeof(svtkIdType);
  // If necessary, we create padding after svtkFastGeomQuad such that
  // the beginning of ids aligns evenly with sizeof(svtkIdType).
  if (qsize % sizeId == 0)
  {
    return static_cast<int>(qsize + numPts * sizeId);
  }
  else
  {
    return static_cast<int>((qsize / sizeId + 1 + numPts) * sizeId);
  }
}

class svtkDataSetSurfaceFilter::svtkEdgeInterpolationMap
{
public:
  void AddEdge(svtkIdType endpoint1, svtkIdType endpoint2, svtkIdType midpoint)
  {
    if (endpoint1 > endpoint2)
      std::swap(endpoint1, endpoint2);
    Map.insert(std::make_pair(std::make_pair(endpoint1, endpoint2), midpoint));
  }
  svtkIdType FindEdge(svtkIdType endpoint1, svtkIdType endpoint2)
  {
    if (endpoint1 > endpoint2)
      std::swap(endpoint1, endpoint2);
    MapType::iterator iter = Map.find(std::make_pair(endpoint1, endpoint2));
    if (iter != Map.end())
    {
      return iter->second;
    }
    else
    {
      return -1;
    }
  }

protected:
  struct HashFunction
  {
  public:
    size_t operator()(std::pair<svtkIdType, svtkIdType> edge) const
    {
      return static_cast<size_t>(edge.first + edge.second);
    }
  };
  typedef std::unordered_map<std::pair<svtkIdType, svtkIdType>, svtkIdType, HashFunction> MapType;
  MapType Map;
};

svtkObjectFactoryNewMacro(svtkDataSetSurfaceFilter);

//----------------------------------------------------------------------------
svtkDataSetSurfaceFilter::svtkDataSetSurfaceFilter()
{
  this->QuadHash = nullptr;
  this->PointMap = nullptr;
  this->EdgeMap = nullptr;
  this->QuadHashLength = 0;
  this->UseStrips = 0;
  this->NumberOfNewCells = 0;

  // Quad allocation stuff.
  this->FastGeomQuadArrayLength = 0;
  this->NumberOfFastGeomQuadArrays = 0;
  this->FastGeomQuadArrays = nullptr;
  this->NextArrayIndex = 0;
  this->NextQuadIndex = 0;

  this->PieceInvariant = 0;

  this->PassThroughCellIds = 0;
  this->PassThroughPointIds = 0;
  this->OriginalCellIds = nullptr;
  this->OriginalPointIds = nullptr;
  this->OriginalCellIdsName = nullptr;
  this->OriginalPointIdsName = nullptr;

  this->NonlinearSubdivisionLevel = 1;
}

//----------------------------------------------------------------------------
svtkDataSetSurfaceFilter::~svtkDataSetSurfaceFilter()
{
  this->SetOriginalCellIdsName(nullptr);
  this->SetOriginalPointIdsName(nullptr);
}

//----------------------------------------------------------------------------
int svtkDataSetSurfaceFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType numCells = input->GetNumberOfCells();
  svtkIdType ext[6], wholeExt[6];

  if (input->CheckAttributes())
  {
    return 1;
  }

  if (numCells == 0)
  {
    svtkDebugMacro(<< "Number of cells is zero, no data to process.");
    return 1;
  }

  if (input->GetExtentType() == SVTK_3D_EXTENT)
  {
    const int* wholeExt32;
    wholeExt32 = inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());
    wholeExt[0] = wholeExt32[0];
    wholeExt[1] = wholeExt32[1];
    wholeExt[2] = wholeExt32[2];
    wholeExt[3] = wholeExt32[3];
    wholeExt[4] = wholeExt32[4];
    wholeExt[5] = wholeExt32[5];
  }

  switch (input->GetDataObjectType())
  {
    case SVTK_UNSTRUCTURED_GRID:
    case SVTK_UNSTRUCTURED_GRID_BASE:
    {
      this->UnstructuredGridExecute(input, output);
      output->CheckAttributes();
      return 1;
    }
    case SVTK_RECTILINEAR_GRID:
    {
      svtkRectilinearGrid* grid = svtkRectilinearGrid::SafeDownCast(input);
      int* tmpext = grid->GetExtent();
      ext[0] = tmpext[0];
      ext[1] = tmpext[1];
      ext[2] = tmpext[2];
      ext[3] = tmpext[3];
      ext[4] = tmpext[4];
      ext[5] = tmpext[5];
      return this->StructuredExecute(grid, output, ext, wholeExt);
    }
    case SVTK_STRUCTURED_GRID:
    {
      svtkStructuredGrid* grid = svtkStructuredGrid::SafeDownCast(input);
      if (grid->HasAnyBlankCells())
      {
        return this->StructuredWithBlankingExecute(grid, output);
      }
      else
      {
        int* tmpext = grid->GetExtent();
        ext[0] = tmpext[0];
        ext[1] = tmpext[1];
        ext[2] = tmpext[2];
        ext[3] = tmpext[3];
        ext[4] = tmpext[4];
        ext[5] = tmpext[5];
        return this->StructuredExecute(grid, output, ext, wholeExt);
      }
    }
    case SVTK_UNIFORM_GRID:
    case SVTK_STRUCTURED_POINTS:
    case SVTK_IMAGE_DATA:
    {
      svtkImageData* image = svtkImageData::SafeDownCast(input);
      int* tmpext = image->GetExtent();
      ext[0] = tmpext[0];
      ext[1] = tmpext[1];
      ext[2] = tmpext[2];
      ext[3] = tmpext[3];
      ext[4] = tmpext[4];
      ext[5] = tmpext[5];
      return this->StructuredExecute(image, output, ext, wholeExt);
    }
    case SVTK_POLY_DATA:
    {
      svtkPolyData* inPd = svtkPolyData::SafeDownCast(input);
      output->ShallowCopy(inPd);
      if (this->PassThroughCellIds)
      {
        // make a 1:1 mapping
        this->OriginalCellIds = svtkIdTypeArray::New();
        this->OriginalCellIds->SetName(this->GetOriginalCellIdsName());
        this->OriginalCellIds->SetNumberOfComponents(1);
        svtkCellData* outputCD = output->GetCellData();
        outputCD->AddArray(this->OriginalCellIds);
        svtkIdType numTup = output->GetNumberOfCells();
        this->OriginalCellIds->SetNumberOfValues(numTup);
        for (svtkIdType cId = 0; cId < numTup; cId++)
        {
          this->OriginalCellIds->SetValue(cId, cId);
        }
        this->OriginalCellIds->Delete();
        this->OriginalCellIds = nullptr;
      }
      if (this->PassThroughPointIds)
      {
        // make a 1:1 mapping
        this->OriginalPointIds = svtkIdTypeArray::New();
        this->OriginalPointIds->SetName(this->GetOriginalPointIdsName());
        this->OriginalPointIds->SetNumberOfComponents(1);
        svtkPointData* outputPD = output->GetPointData();
        outputPD->AddArray(this->OriginalPointIds);
        svtkIdType numTup = output->GetNumberOfPoints();
        this->OriginalPointIds->SetNumberOfValues(numTup);
        for (svtkIdType cId = 0; cId < numTup; cId++)
        {
          this->OriginalPointIds->SetValue(cId, cId);
        }
        this->OriginalPointIds->Delete();
        this->OriginalPointIds = nullptr;
      }

      return 1;
    }
    default:
      return this->DataSetExecute(input, output);
  }
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::EstimateStructuredDataArraySizes(
  svtkIdType* ext, svtkIdType* wholeExt, svtkIdType& numPoints, svtkIdType& numCells)
{
  // Sanity Checks
  assert(ext != nullptr);
  assert(wholeExt != nullptr);

  numPoints = numCells = 0;

  // xMin face
  if (ext[0] == wholeExt[0] && ext[2] != ext[3] && ext[4] != ext[5] && ext[0] != ext[1])
  {
    numCells += (ext[3] - ext[2]) * (ext[5] - ext[4]);
    numPoints += (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1);
  }
  // xMax face
  if (ext[1] == wholeExt[1] && ext[2] != ext[3] && ext[4] != ext[5])
  {
    numCells += (ext[3] - ext[2]) * (ext[5] - ext[4]);
    numPoints += (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1);
  }
  // yMin face
  if (ext[2] == wholeExt[2] && ext[0] != ext[1] && ext[4] != ext[5] && ext[2] != ext[3])
  {
    numCells += (ext[1] - ext[0]) * (ext[5] - ext[4]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[5] - ext[4] + 1);
  }
  // yMax face
  if (ext[3] == wholeExt[3] && ext[0] != ext[1] && ext[4] != ext[5])
  {
    numCells += (ext[1] - ext[0]) * (ext[5] - ext[4]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[5] - ext[4] + 1);
  }
  // zMin face
  if (ext[4] == wholeExt[4] && ext[0] != ext[1] && ext[2] != ext[3] && ext[4] != ext[5])
  {
    numCells += (ext[1] - ext[0]) * (ext[3] - ext[2]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1);
  }
  // zMax face
  if (ext[5] == wholeExt[5] && ext[0] != ext[1] && ext[2] != ext[3])
  {
    numCells += (ext[1] - ext[0]) * (ext[3] - ext[2]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1);
  }
}

//----------------------------------------------------------------------------
int svtkDataSetSurfaceFilter::UniformGridExecute(
  svtkDataSet* input, svtkPolyData* output, svtkIdType* ext, svtkIdType* wholeExt, bool extractface[6])
{

  if (this->UseStrips)
  {
    svtkErrorMacro("Strips are not supported for uniform grid!");
    return 0;
  }

  svtkIdType numPoints, numCells;
  svtkPoints* gridPnts = svtkPoints::New();
  svtkCellArray* gridCells = svtkCellArray::New();

  int originalPassThroughCellIds = this->PassThroughCellIds;

  // Lets figure out the max number of cells and points we are going to have
  numPoints = numCells = 0;
  this->EstimateStructuredDataArraySizes(ext, wholeExt, numPoints, numCells);
  gridPnts->Allocate(numPoints);
  gridCells->AllocateEstimate(numCells, 1);
  output->SetPoints(gridPnts);
  gridPnts->Delete();
  output->SetPolys(gridCells);
  gridCells->Delete();

  // Allocate attributes for copying.
  output->GetPointData()->CopyGlobalIdsOn();
  output->GetPointData()->CopyAllocate(input->GetPointData(), numPoints);
  output->GetCellData()->CopyGlobalIdsOn();
  output->GetCellData()->CopyAllocate(input->GetCellData(), numCells);

  if (this->PassThroughCellIds)
  {
    this->OriginalCellIds = svtkIdTypeArray::New();
    this->OriginalCellIds->SetName(this->GetOriginalCellIdsName());
    this->OriginalCellIds->SetNumberOfComponents(1);
    this->OriginalCellIds->Allocate(numCells);
    output->GetCellData()->AddArray(this->OriginalCellIds);
  }
  if (this->PassThroughPointIds)
  {
    this->OriginalPointIds = svtkIdTypeArray::New();
    this->OriginalPointIds->SetName(this->GetOriginalPointIdsName());
    this->OriginalPointIds->SetNumberOfComponents(1);
    this->OriginalPointIds->Allocate(numPoints);
    output->GetPointData()->AddArray(this->OriginalPointIds);
  }

  // xMin face
  if (extractface[0])
    this->ExecuteFaceQuads(input, output, 0, ext, 0, 1, 2, wholeExt, true);

  // xMax face
  if (extractface[1])
    this->ExecuteFaceQuads(input, output, 1, ext, 0, 2, 1, wholeExt, true);

  // yMin face
  if (extractface[2])
    this->ExecuteFaceQuads(input, output, 0, ext, 1, 2, 0, wholeExt, true);

  // yMax face
  if (extractface[3])
    this->ExecuteFaceQuads(input, output, 1, ext, 1, 0, 2, wholeExt, true);

  // zMin face
  if (extractface[4])
    this->ExecuteFaceQuads(input, output, 0, ext, 2, 0, 1, wholeExt, true);

  // zMax face
  if (extractface[5])
    this->ExecuteFaceQuads(input, output, 1, ext, 2, 1, 0, wholeExt, true);

  output->Squeeze();
  this->PassThroughCellIds = originalPassThroughCellIds;

  if (this->OriginalPointIds)
  {
    this->OriginalPointIds->Delete();
    this->OriginalPointIds = nullptr;
  }
  if (this->OriginalCellIds)
  {
    this->OriginalCellIds->Delete();
    this->OriginalPointIds = nullptr;
  }
  return 1;
}

//----------------------------------------------------------------------------
// It is a pain that structured data sets do not share a common super class
// other than data set, and data set does not allow access to extent!
int svtkDataSetSurfaceFilter::StructuredExecute(
  svtkDataSet* input, svtkPolyData* output, svtkIdType* ext, svtkIdType* wholeExt)
{
  svtkRectilinearGrid* rgrid = svtkRectilinearGrid::SafeDownCast(input);
  svtkStructuredGrid* sgrid = svtkStructuredGrid::SafeDownCast(input);
  if (rgrid || sgrid)
  {
    // Fetch the grid dimension
    int iext[6];
    std::copy(ext, ext + 6, iext);
    int dimension = svtkStructuredData::GetDataDimension(iext);

    if (dimension == 1)
    {
      // Use specialized filter in case of 1D grid
      if (rgrid)
      {
        svtkNew<svtkRectilinearGridGeometryFilter> filter;
        filter->SetInputData(input);
        filter->SetExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
        filter->Update();
        output->ShallowCopy(filter->GetOutput());
        return 1;
      }
      else if (sgrid)
      {
        svtkNew<svtkStructuredGridGeometryFilter> filter;
        filter->SetInputData(input);
        filter->SetExtent(ext[0], ext[1], ext[2], ext[3], ext[4], ext[5]);
        filter->Update();
        output->ShallowCopy(filter->GetOutput());
        return 1;
      }
    }
  }

  svtkIdType numPoints, cellArraySize;
  svtkCellArray* outStrips;
  svtkCellArray* outPolys;
  svtkPoints* outPoints;

  // Cell Array Size is a pretty good estimate.
  // Does not consider direction of strip.

  // Lets figure out how many cells and points we are going to have.
  // It may be overkill comptuing the exact amount, but we can do it, so ...
  cellArraySize = numPoints = 0;
  // xMin face
  if (ext[0] == wholeExt[0] && ext[2] != ext[3] && ext[4] != ext[5] && ext[0] != ext[1])
  {
    cellArraySize += (ext[3] - ext[2]) * (ext[5] - ext[4]);
    numPoints += (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1);
  }
  // xMax face
  if (ext[1] == wholeExt[1] && ext[2] != ext[3] && ext[4] != ext[5])
  {
    cellArraySize += (ext[3] - ext[2]) * (ext[5] - ext[4]);
    numPoints += (ext[3] - ext[2] + 1) * (ext[5] - ext[4] + 1);
  }
  // yMin face
  if (ext[2] == wholeExt[2] && ext[0] != ext[1] && ext[4] != ext[5] && ext[2] != ext[3])
  {
    cellArraySize += (ext[1] - ext[0]) * (ext[5] - ext[4]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[5] - ext[4] + 1);
  }
  // yMax face
  if (ext[3] == wholeExt[3] && ext[0] != ext[1] && ext[4] != ext[5])
  {
    cellArraySize += (ext[1] - ext[0]) * (ext[5] - ext[4]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[5] - ext[4] + 1);
  }
  // zMin face
  if (ext[4] == wholeExt[4] && ext[0] != ext[1] && ext[2] != ext[3] && ext[4] != ext[5])
  {
    cellArraySize += (ext[1] - ext[0]) * (ext[3] - ext[2]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1);
  }
  // zMax face
  if (ext[5] == wholeExt[5] && ext[0] != ext[1] && ext[2] != ext[3])
  {
    cellArraySize += (ext[1] - ext[0]) * (ext[3] - ext[2]);
    numPoints += (ext[1] - ext[0] + 1) * (ext[3] - ext[2] + 1);
  }

  int originalPassThroughCellIds = this->PassThroughCellIds;
  if (this->UseStrips)
  {
    outStrips = svtkCellArray::New();
    outStrips->AllocateEstimate(cellArraySize, 1);
    output->SetStrips(outStrips);
    outStrips->Delete();

    // disable cell ids passing since we are using tstrips.
    this->PassThroughCellIds = 0;
  }
  else
  {
    outPolys = svtkCellArray::New();
    outPolys->AllocateEstimate(cellArraySize, 4);
    output->SetPolys(outPolys);
    outPolys->Delete();
  }
  outPoints = svtkPoints::New();
  int dataType;
  switch (input->GetDataObjectType())
  {
    case SVTK_RECTILINEAR_GRID:
    {
      dataType = rgrid->GetXCoordinates()->GetDataType();
      break;
    }
    case SVTK_STRUCTURED_GRID:
    {
      dataType = sgrid->GetPoints()->GetDataType();
      break;
    }
    case SVTK_UNIFORM_GRID:
    case SVTK_STRUCTURED_POINTS:
    case SVTK_IMAGE_DATA:
    {
      dataType = SVTK_DOUBLE;
      break;
    }
    default:
      dataType = SVTK_DOUBLE;
      svtkErrorMacro("Invalid data set type: " << input->GetDataObjectType());
      outPoints->Delete();
      return 1;
  }

  outPoints->SetDataType(dataType);
  outPoints->Allocate(numPoints);
  output->SetPoints(outPoints);
  outPoints->Delete();

  // Allocate attributes for copying.
  output->GetPointData()->CopyGlobalIdsOn();
  output->GetPointData()->CopyAllocate(input->GetPointData(), numPoints);
  output->GetCellData()->CopyGlobalIdsOn();
  output->GetCellData()->CopyAllocate(input->GetCellData(), cellArraySize);

  if (this->PassThroughCellIds)
  {
    this->OriginalCellIds = svtkIdTypeArray::New();
    this->OriginalCellIds->SetName(this->GetOriginalCellIdsName());
    this->OriginalCellIds->SetNumberOfComponents(1);
    this->OriginalCellIds->Allocate(cellArraySize);
    output->GetCellData()->AddArray(this->OriginalCellIds);
  }
  if (this->PassThroughPointIds)
  {
    this->OriginalPointIds = svtkIdTypeArray::New();
    this->OriginalPointIds->SetName(this->GetOriginalPointIdsName());
    this->OriginalPointIds->SetNumberOfComponents(1);
    this->OriginalPointIds->Allocate(numPoints);
    output->GetPointData()->AddArray(this->OriginalPointIds);
  }

  if (this->UseStrips)
  {
    // xMin face
    this->ExecuteFaceStrips(input, output, 0, ext, 0, 1, 2, wholeExt);
    // xMax face
    this->ExecuteFaceStrips(input, output, 1, ext, 0, 2, 1, wholeExt);
    // yMin face
    this->ExecuteFaceStrips(input, output, 0, ext, 1, 2, 0, wholeExt);
    // yMax face
    this->ExecuteFaceStrips(input, output, 1, ext, 1, 0, 2, wholeExt);
    // zMin face
    this->ExecuteFaceStrips(input, output, 0, ext, 2, 0, 1, wholeExt);
    // zMax face
    this->ExecuteFaceStrips(input, output, 1, ext, 2, 1, 0, wholeExt);
  }
  else
  {
    // xMin face
    this->ExecuteFaceQuads(input, output, 0, ext, 0, 1, 2, wholeExt);
    // xMax face
    this->ExecuteFaceQuads(input, output, 1, ext, 0, 2, 1, wholeExt);
    // yMin face
    this->ExecuteFaceQuads(input, output, 0, ext, 1, 2, 0, wholeExt);
    // yMax face
    this->ExecuteFaceQuads(input, output, 1, ext, 1, 0, 2, wholeExt);
    // zMin face
    this->ExecuteFaceQuads(input, output, 0, ext, 2, 0, 1, wholeExt);
    // zMax face
    this->ExecuteFaceQuads(input, output, 1, ext, 2, 1, 0, wholeExt);
  }
  output->Squeeze();
  if (this->OriginalCellIds != nullptr)
  {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = nullptr;
  }
  if (this->OriginalPointIds != nullptr)
  {
    this->OriginalPointIds->Delete();
    this->OriginalPointIds = nullptr;
  }

  this->PassThroughCellIds = originalPassThroughCellIds;

  return 1;
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::ExecuteFaceStrips(svtkDataSet* input, svtkPolyData* output, int maxFlag,
  svtkIdType* ext, int aAxis, int bAxis, int cAxis, svtkIdType* wholeExt)
{
  svtkPoints* outPts;
  svtkCellArray* outStrips;
  svtkPointData *inPD, *outPD;
  svtkIdType pInc[3];
  svtkIdType qInc[3];
  svtkIdType ptCInc[3];
  svtkIdType cOutInc;
  double pt[3];
  svtkIdType inStartPtId;
  svtkIdType outStartPtId;
  svtkIdType outPtId;
  svtkIdType inId, outId;
  svtkIdType ib, ic;
  int aA2, bA2, cA2;
  int rotatedFlag;
  svtkIdType* stripArray;
  svtkIdType stripArrayIdx;

  outPts = output->GetPoints();
  outPD = output->GetPointData();
  inPD = input->GetPointData();

  pInc[0] = 1;
  pInc[1] = (ext[1] - ext[0] + 1);
  pInc[2] = (ext[3] - ext[2] + 1) * pInc[1];
  // quad increments (cell incraments, but cInc could be confused with c axis).
  qInc[0] = 1;
  qInc[1] = ext[1] - ext[0];
  qInc[2] = (ext[3] - ext[2]) * qInc[1];
  ptCInc[0] = 1;
  ptCInc[1] = ext[1] - ext[0];
  if (ptCInc[1] == 0)
  {
    ptCInc[1] = 1;
  }
  ptCInc[2] = (ext[3] - ext[2]);
  if (ptCInc[2] == 0)
  {
    ptCInc[2] = 1;
  }
  ptCInc[2] = ptCInc[2] * ptCInc[1];

  // Tempoprary variables to avoid many multiplications.
  aA2 = aAxis * 2;
  bA2 = bAxis * 2;
  cA2 = cAxis * 2;

  // We might as well put the test for this face here.
  if (ext[bA2] == ext[bA2 + 1] || ext[cA2] == ext[cA2 + 1])
  {
    return;
  }
  if (maxFlag)
  { // max faces have a slightly different condition to avoid coincident faces.
    if (ext[aA2] == ext[aA2 + 1] || ext[aA2 + 1] < wholeExt[aA2 + 1])
    {
      return;
    }
  }
  else
  {
    if (ext[aA2] > wholeExt[aA2])
    {
      return;
    }
  }

  // Lets rotate the image to make b the longest axis.
  // This will make the tri strips longer.
  rotatedFlag = 0;
  if (ext[bA2 + 1] - ext[bA2] < ext[cA2 + 1] - ext[cA2])
  {
    int tmp;
    rotatedFlag = 1;
    tmp = cAxis;
    cAxis = bAxis;
    bAxis = tmp;
    bA2 = bAxis * 2;
    cA2 = cAxis * 2;
  }

  // Assuming no ghost cells ...
  inStartPtId = 0;
  if (maxFlag)
  {
    inStartPtId = pInc[aAxis] * (ext[aA2 + 1] - ext[aA2]);
  }

  svtkIdType outCellId = 0;
  svtkIdType inStartCellId = 0;
  svtkIdType inCellId = 0;
  if (this->PassThroughCellIds)
  {
    outCellId = this->OriginalCellIds->GetNumberOfTuples();
    if (maxFlag && ext[aA2] < ext[1 + aA2])
    {
      inStartCellId = qInc[aAxis] * (ext[aA2 + 1] - ext[aA2] - 1);
    }
  }

  outStartPtId = outPts->GetNumberOfPoints();
  // Make the points for this face.
  for (ic = ext[cA2]; ic <= ext[cA2 + 1]; ++ic)
  {
    for (ib = ext[bA2]; ib <= ext[bA2 + 1]; ++ib)
    {
      inId = inStartPtId + (ib - ext[bA2]) * pInc[bAxis] + (ic - ext[cA2]) * pInc[cAxis];
      input->GetPoint(inId, pt);
      outId = outPts->InsertNextPoint(pt);
      // Copy point data.
      outPD->CopyData(inPD, inId, outId);
      this->RecordOrigPointId(outId, inId);
    }
  }

  // Do the cells.
  cOutInc = ext[bA2 + 1] - ext[bA2] + 1;

  // Tri Strips (no cell data ...).
  // Allocate the temporary array used to create the tri strips.
  stripArray = new svtkIdType[2 * (ext[bA2 + 1] - ext[bA2] + 1)];
  // Make the cells for this face.
  outStrips = output->GetStrips();

  for (ic = ext[cA2]; ic < ext[cA2 + 1]; ++ic)
  {
    // Fill in the array describing the strips.
    stripArrayIdx = 0;
    outPtId = outStartPtId + (ic - ext[cA2]) * cOutInc;

    if (rotatedFlag)
    {
      for (ib = ext[bA2]; ib <= ext[bA2 + 1]; ++ib)
      {
        stripArray[stripArrayIdx++] = outPtId + cOutInc;
        stripArray[stripArrayIdx++] = outPtId;
        ++outPtId;
        if (this->PassThroughCellIds && ib != ext[bA2])
        {
          // Record the two triangular output cells just defined
          // both belong to the same input quad cell
          inCellId =
            inStartCellId + (ib - ext[bA2] - 1) * ptCInc[bAxis] + (ic - ext[cA2]) * ptCInc[cAxis];
          this->RecordOrigCellId(outCellId++, inCellId);
          this->RecordOrigCellId(outCellId++, inCellId);
        }
      }
    }
    else
    { // Faster to justto duplicate the inner most loop.
      for (ib = ext[bA2]; ib <= ext[bA2 + 1]; ++ib)
      {
        stripArray[stripArrayIdx++] = outPtId;
        stripArray[stripArrayIdx++] = outPtId + cOutInc;
        ++outPtId;
        if (this->PassThroughCellIds && ib != ext[bA2])
        {
          // Record the two triangular output cells just defined
          // both belong to the same input quad cell
          inCellId =
            inStartCellId + (ib - ext[bA2] - 1) * ptCInc[bAxis] + (ic - ext[cA2]) * ptCInc[cAxis];
          this->RecordOrigCellId(outCellId++, inCellId);
          this->RecordOrigCellId(outCellId++, inCellId);
        }
      }
    }
    outStrips->InsertNextCell(stripArrayIdx, stripArray);
  }
  delete[] stripArray;
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::ExecuteFaceQuads(svtkDataSet* input, svtkPolyData* output, int maxFlag,
  svtkIdType* ext, int aAxis, int bAxis, int cAxis, svtkIdType* wholeExt, bool checkVisibility)
{
  svtkPoints* outPts;
  svtkCellArray* outPolys;
  svtkPointData *inPD, *outPD;
  svtkCellData *inCD, *outCD;
  svtkIdType pInc[3];
  svtkIdType qInc[3];
  svtkIdType cOutInc;
  double pt[3];
  svtkIdType inStartPtId;
  svtkIdType inStartCellId;
  svtkIdType outStartPtId;
  svtkIdType outPtId;
  svtkIdType inId, outId;
  svtkIdType ib, ic;
  int aA2, bA2, cA2;

  outPts = output->GetPoints();
  outPD = output->GetPointData();
  inPD = input->GetPointData();
  outCD = output->GetCellData();
  inCD = input->GetCellData();

  pInc[0] = 1;
  pInc[1] = (ext[1] - ext[0] + 1);
  pInc[2] = (ext[3] - ext[2] + 1) * pInc[1];
  // quad increments (cell incraments, but cInc could be confused with c axis).
  qInc[0] = 1;
  qInc[1] = ext[1] - ext[0];
  // The conditions are for when we have one or more degenerate axes (2d or 1d cells).
  if (qInc[1] == 0)
  {
    qInc[1] = 1;
  }
  qInc[2] = (ext[3] - ext[2]) * qInc[1];
  if (qInc[2] == 0)
  {
    qInc[2] = qInc[1];
  }

  // Temporary variables to avoid many multiplications.
  aA2 = aAxis * 2;
  bA2 = bAxis * 2;
  cA2 = cAxis * 2;

  // We might as well put the test for this face here.
  if (ext[bA2] == ext[bA2 + 1] || ext[cA2] == ext[cA2 + 1])
  {
    return;
  }
  if (maxFlag)
  {
    if (ext[aA2 + 1] < wholeExt[aA2 + 1])
    {
      return;
    }
  }
  else
  { // min faces have a slightly different condition to avoid coincident faces.
    if (ext[aA2] == ext[aA2 + 1] || ext[aA2] > wholeExt[aA2])
    {
      return;
    }
  }

  // Assuming no ghost cells ...
  inStartPtId = inStartCellId = 0;
  // I put this confusing conditional to fix a regression test.
  // If we are creating a maximum face, then we indeed have to offset
  // the input cell Ids.  However, svtkGeometryFilter created a 2d
  // image as a max face, but the cells are copied as a min face (no
  // offset).  Hence maxFlag = 1 and there should be no offset.
  if (maxFlag && ext[aA2] < ext[1 + aA2])
  {
    inStartPtId = pInc[aAxis] * (ext[aA2 + 1] - ext[aA2]);
    inStartCellId = qInc[aAxis] * (ext[aA2 + 1] - ext[aA2] - 1);
  }

  svtkUniformGrid* grid = static_cast<svtkUniformGrid*>(input);
  assert(grid != nullptr);

  outStartPtId = outPts->GetNumberOfPoints();
  // Make the points for this face.
  for (ic = ext[cA2]; ic <= ext[cA2 + 1]; ++ic)
  {
    for (ib = ext[bA2]; ib <= ext[bA2 + 1]; ++ib)
    {
      inId = inStartPtId + (ib - ext[bA2]) * pInc[bAxis] + (ic - ext[cA2]) * pInc[cAxis];
      input->GetPoint(inId, pt);
      outId = outPts->InsertNextPoint(pt);
      // Copy point data.
      outPD->CopyData(inPD, inId, outId);
      this->RecordOrigPointId(outId, inId);
    }
  }

  // Do the cells.
  cOutInc = ext[bA2 + 1] - ext[bA2] + 1;

  outPolys = output->GetPolys();

  // Old method for creating quads (needed for cell data.).
  for (ic = ext[cA2]; ic < ext[cA2 + 1]; ++ic)
  {
    for (ib = ext[bA2]; ib < ext[bA2 + 1]; ++ib)
    {
      outPtId = outStartPtId + (ib - ext[bA2]) + (ic - ext[cA2]) * cOutInc;
      inId = inStartCellId + (ib - ext[bA2]) * qInc[bAxis] + (ic - ext[cA2]) * qInc[cAxis];

      if (checkVisibility && grid->IsCellVisible(inId))
      {
        outId = outPolys->InsertNextCell(4);
        outPolys->InsertCellPoint(outPtId);
        outPolys->InsertCellPoint(outPtId + cOutInc);
        outPolys->InsertCellPoint(outPtId + cOutInc + 1);
        outPolys->InsertCellPoint(outPtId + 1);
        // Copy cell data.
        outCD->CopyData(inCD, inId, outId);
        this->RecordOrigCellId(outId, inId);
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::ExecuteFaceQuads(svtkDataSet* input, svtkPolyData* output, int maxFlag,
  svtkIdType* ext, int aAxis, int bAxis, int cAxis, svtkIdType* wholeExt)
{
  svtkPoints* outPts;
  svtkCellArray* outPolys;
  svtkPointData *inPD, *outPD;
  svtkCellData *inCD, *outCD;
  svtkIdType pInc[3];
  svtkIdType qInc[3];
  svtkIdType cOutInc;
  double pt[3];
  svtkIdType inStartPtId;
  svtkIdType inStartCellId;
  svtkIdType outStartPtId;
  svtkIdType outPtId;
  svtkIdType inId, outId;
  svtkIdType ib, ic;
  int aA2, bA2, cA2;

  outPts = output->GetPoints();
  outPD = output->GetPointData();
  inPD = input->GetPointData();
  outCD = output->GetCellData();
  inCD = input->GetCellData();

  pInc[0] = 1;
  pInc[1] = (ext[1] - ext[0] + 1);
  pInc[2] = (ext[3] - ext[2] + 1) * pInc[1];
  // quad increments (cell incraments, but cInc could be confused with c axis).
  qInc[0] = 1;
  qInc[1] = ext[1] - ext[0];
  // The conditions are for when we have one or more degenerate axes (2d or 1d cells).
  if (qInc[1] == 0)
  {
    qInc[1] = 1;
  }
  qInc[2] = (ext[3] - ext[2]) * qInc[1];
  if (qInc[2] == 0)
  {
    qInc[2] = qInc[1];
  }

  // Temporary variables to avoid many multiplications.
  aA2 = aAxis * 2;
  bA2 = bAxis * 2;
  cA2 = cAxis * 2;

  // We might as well put the test for this face here.
  if (ext[bA2] == ext[bA2 + 1] || ext[cA2] == ext[cA2 + 1])
  {
    return;
  }
  if (maxFlag)
  {
    if (ext[aA2 + 1] < wholeExt[aA2 + 1])
    {
      return;
    }
  }
  else
  { // min faces have a slightly different condition to avoid coincident faces.
    if (ext[aA2] == ext[aA2 + 1] || ext[aA2] > wholeExt[aA2])
    {
      return;
    }
  }

  // Assuming no ghost cells ...
  inStartPtId = inStartCellId = 0;
  // I put this confusing conditional to fix a regression test.
  // If we are creating a maximum face, then we indeed have to offset
  // the input cell Ids.  However, svtkGeometryFilter created a 2d
  // image as a max face, but the cells are copied as a min face (no
  // offset).  Hence maxFlag = 1 and there should be no offset.
  if (maxFlag && ext[aA2] < ext[1 + aA2])
  {
    inStartPtId = pInc[aAxis] * (ext[aA2 + 1] - ext[aA2]);
    inStartCellId = qInc[aAxis] * (ext[aA2 + 1] - ext[aA2] - 1);
  }

  outStartPtId = outPts->GetNumberOfPoints();
  // Make the points for this face.
  for (ic = ext[cA2]; ic <= ext[cA2 + 1]; ++ic)
  {
    for (ib = ext[bA2]; ib <= ext[bA2 + 1]; ++ib)
    {
      inId = inStartPtId + (ib - ext[bA2]) * pInc[bAxis] + (ic - ext[cA2]) * pInc[cAxis];
      input->GetPoint(inId, pt);
      outId = outPts->InsertNextPoint(pt);
      // Copy point data.
      outPD->CopyData(inPD, inId, outId);
      this->RecordOrigPointId(outId, inId);
    }
  }

  // Do the cells.
  cOutInc = ext[bA2 + 1] - ext[bA2] + 1;

  outPolys = output->GetPolys();

  // Old method for creating quads (needed for cell data.).
  for (ic = ext[cA2]; ic < ext[cA2 + 1]; ++ic)
  {
    for (ib = ext[bA2]; ib < ext[bA2 + 1]; ++ib)
    {
      outPtId = outStartPtId + (ib - ext[bA2]) + (ic - ext[cA2]) * cOutInc;
      inId = inStartCellId + (ib - ext[bA2]) * qInc[bAxis] + (ic - ext[cA2]) * qInc[cAxis];

      outId = outPolys->InsertNextCell(4);
      outPolys->InsertCellPoint(outPtId);
      outPolys->InsertCellPoint(outPtId + cOutInc);
      outPolys->InsertCellPoint(outPtId + cOutInc + 1);
      outPolys->InsertCellPoint(outPtId + 1);
      // Copy cell data.
      outCD->CopyData(inCD, inId, outId);
      this->RecordOrigCellId(outId, inId);
    }
  }
}

//----------------------------------------------------------------------------
int svtkDataSetSurfaceFilter::StructuredWithBlankingExecute(
  svtkStructuredGrid* input, svtkPolyData* output)
{
  svtkIdType newCellId;
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  svtkCell* face;
  double x[3];
  svtkIdList* cellIds;
  svtkIdList* pts;
  svtkPoints* newPts;
  svtkIdType ptId, pt;
  int npts;
  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  if (numCells == 0)
  {
    svtkDebugMacro(<< "Number of cells is zero, no data to process.");
    return 1;
  }

  if (this->PassThroughCellIds)
  {
    this->OriginalCellIds = svtkIdTypeArray::New();
    this->OriginalCellIds->SetName(this->GetOriginalCellIdsName());
    this->OriginalCellIds->SetNumberOfComponents(1);
    this->OriginalCellIds->Allocate(numCells);
    outputCD->AddArray(this->OriginalCellIds);
  }
  if (this->PassThroughPointIds)
  {
    this->OriginalPointIds = svtkIdTypeArray::New();
    this->OriginalPointIds->SetName(this->GetOriginalPointIdsName());
    this->OriginalPointIds->SetNumberOfComponents(1);
    this->OriginalPointIds->Allocate(numPts);
    outputPD->AddArray(this->OriginalPointIds);
  }

  cellIds = svtkIdList::New();
  pts = svtkIdList::New();

  svtkDebugMacro(<< "Executing geometry filter");

  // Allocate
  //
  newPts = svtkPoints::New();
  // we don't know what type of data the input points are so
  // we keep the output points to have the default type (float)
  newPts->Allocate(numPts, numPts / 2);
  output->AllocateEstimate(numCells, 3);
  outputPD->CopyGlobalIdsOn();
  outputPD->CopyAllocate(pd, numPts, numPts / 2);
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(cd, numCells, numCells / 2);

  // Traverse cells to extract geometry
  //
  int abort = 0;
  int dims[3];
  input->GetCellDims(dims);
  svtkIdType d01 = static_cast<svtkIdType>(dims[0]) * dims[1];
  for (int k = 0; k < dims[2] && !abort; ++k)
  {
    svtkDebugMacro(<< "Process cell #" << d01 * k);
    this->UpdateProgress(k / dims[2]);
    abort = this->GetAbortExecute();
    for (int j = 0; j < dims[1]; ++j)
    {
      for (int i = 0; i < dims[0]; ++i)
      {
        svtkIdType cellId = d01 * k + dims[0] * j + i;
        if (!input->IsCellVisible(cellId))
        {
          continue;
        }
        svtkCell* cell = input->GetCell(i, j, k);
        switch (cell->GetCellDimension())
        {
          // create new points and then cell
          case 0:
          case 1:
          case 2:
            npts = cell->GetNumberOfPoints();
            pts->Reset();
            for (int l = 0; l < npts; ++l)
            {
              ptId = cell->GetPointId(l);
              input->GetPoint(ptId, x);
              pt = newPts->InsertNextPoint(x);
              outputPD->CopyData(pd, ptId, pt);
              this->RecordOrigPointId(pt, ptId);
              pts->InsertId(l, pt);
            }
            newCellId = output->InsertNextCell(cell->GetCellType(), pts);
            outputCD->CopyData(cd, cellId, newCellId);
            this->RecordOrigCellId(newCellId, cellId);
            break;
          case 3:
            int even[3] = { i, j, k };
            int odd[3] = { i + 1, j + 1, k + 1 };
            for (int m = 0; m < cell->GetNumberOfFaces(); ++m)
            {
              face = cell->GetFace(m);
              if (m % 2)
              {
                input->GetCellNeighbors(cellId, face->PointIds, cellIds, odd);
              }
              else
              {
                input->GetCellNeighbors(cellId, face->PointIds, cellIds, even);
              }
              // faces with only blank neighbors count as external faces
              bool noNeighbors = cellIds->GetNumberOfIds() <= 0;
              for (svtkIdType ci = 0; ci < cellIds->GetNumberOfIds(); ci++)
              {
                if (input->IsCellVisible(cellIds->GetId(ci)))
                {
                  noNeighbors = false;
                  break;
                }
              }
              if (noNeighbors)
              {
                npts = face->GetNumberOfPoints();
                pts->Reset();
                for (int n = 0; n < npts; ++n)
                {
                  ptId = face->GetPointId(n);
                  input->GetPoint(ptId, x);
                  pt = newPts->InsertNextPoint(x);
                  outputPD->CopyData(pd, ptId, pt);
                  this->RecordOrigPointId(pt, ptId);
                  pts->InsertId(n, pt);
                }
                newCellId = output->InsertNextCell(face->GetCellType(), pts);
                outputCD->CopyData(cd, cellId, newCellId);
                this->RecordOrigCellId(newCellId, cellId);
              }
            }
            break;
        } // switch
      }
    }
  } // for all cells

  svtkDebugMacro(<< "Extracted " << newPts->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();
  if (this->OriginalCellIds)
  {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = nullptr;
  }
  if (this->OriginalPointIds)
  {
    this->OriginalPointIds->Delete();
    this->OriginalPointIds = nullptr;
  }

  // free storage
  output->Squeeze();

  cellIds->Delete();
  pts->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int svtkDataSetSurfaceFilter::DataSetExecute(svtkDataSet* input, svtkPolyData* output)
{
  svtkIdType cellId, newCellId;
  int i, j;
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  svtkCell* face;
  double x[3];
  svtkIdList* cellIds;
  svtkIdList* pts;
  svtkPoints* newPts;
  svtkIdType ptId, pt;
  int npts;
  svtkPointData* pd = input->GetPointData();
  svtkCellData* cd = input->GetCellData();
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  if (numCells == 0)
  {
    svtkDebugMacro(<< "Number of cells is zero, no data to process.");
    return 1;
  }

  if (this->PassThroughCellIds)
  {
    this->OriginalCellIds = svtkIdTypeArray::New();
    this->OriginalCellIds->SetName(this->GetOriginalCellIdsName());
    this->OriginalCellIds->SetNumberOfComponents(1);
    this->OriginalCellIds->Allocate(numCells);
    outputCD->AddArray(this->OriginalCellIds);
  }
  if (this->PassThroughPointIds)
  {
    this->OriginalPointIds = svtkIdTypeArray::New();
    this->OriginalPointIds->SetName(this->GetOriginalPointIdsName());
    this->OriginalPointIds->SetNumberOfComponents(1);
    this->OriginalPointIds->Allocate(numPts);
    outputPD->AddArray(this->OriginalPointIds);
  }

  cellIds = svtkIdList::New();
  pts = svtkIdList::New();

  svtkDebugMacro(<< "Executing geometry filter");

  // Allocate
  //
  newPts = svtkPoints::New();
  // we don't know what type of data the input points are so
  // we keep the output points to have the default type (float)
  newPts->Allocate(numPts, numPts / 2);
  output->AllocateEstimate(numCells, 3);
  outputPD->CopyGlobalIdsOn();
  outputPD->CopyAllocate(pd, numPts, numPts / 2);
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(cd, numCells, numCells / 2);

  // Traverse cells to extract geometry
  //
  int abort = 0;
  svtkIdType progressInterval = numCells / 20 + 1;

  for (cellId = 0; cellId < numCells && !abort; cellId++)
  {
    // Progress and abort method support
    if (!(cellId % progressInterval))
    {
      svtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
    }
    svtkCell* cell = input->GetCell(cellId);
    switch (cell->GetCellDimension())
    {
      // create new points and then cell
      case 0:
      case 1:
      case 2:

        npts = cell->GetNumberOfPoints();
        pts->Reset();
        for (i = 0; i < npts; i++)
        {
          ptId = cell->GetPointId(i);
          input->GetPoint(ptId, x);
          pt = newPts->InsertNextPoint(x);
          outputPD->CopyData(pd, ptId, pt);
          this->RecordOrigPointId(pt, ptId);
          pts->InsertId(i, pt);
        }
        newCellId = output->InsertNextCell(cell->GetCellType(), pts);
        if (newCellId > 0)
        {
          outputCD->CopyData(cd, cellId, newCellId);
          this->RecordOrigCellId(newCellId, cellId);
        }
        break;
      case 3:
        for (j = 0; j < cell->GetNumberOfFaces(); j++)
        {
          face = cell->GetFace(j);
          input->GetCellNeighbors(cellId, face->PointIds, cellIds);
          bool noNeighbors = cellIds->GetNumberOfIds() <= 0;
          if (noNeighbors)
          {
            npts = face->GetNumberOfPoints();
            pts->Reset();
            for (i = 0; i < npts; i++)
            {
              ptId = face->GetPointId(i);
              input->GetPoint(ptId, x);
              pt = newPts->InsertNextPoint(x);
              outputPD->CopyData(pd, ptId, pt);
              this->RecordOrigPointId(pt, ptId);
              pts->InsertId(i, pt);
            }
            newCellId = output->InsertNextCell(face->GetCellType(), pts);
            if (newCellId > 0)
            {
              outputCD->CopyData(cd, cellId, newCellId);
              this->RecordOrigCellId(newCellId, cellId);
            }
          }
        }
        break;
    } // switch
  }   // for all cells

  svtkDebugMacro(<< "Extracted " << newPts->GetNumberOfPoints() << " points,"
                << output->GetNumberOfCells() << " cells.");

  // Update ourselves and release memory
  //
  output->SetPoints(newPts);
  newPts->Delete();
  if (this->OriginalCellIds)
  {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = nullptr;
  }
  if (this->OriginalPointIds)
  {
    this->OriginalPointIds->Delete();
    this->OriginalPointIds = nullptr;
  }

  // free storage
  output->Squeeze();

  cellIds->Delete();
  pts->Delete();

  return 1;
}

//----------------------------------------------------------------------------
int svtkDataSetSurfaceFilter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  int piece, numPieces, ghostLevels;

  piece = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER());
  numPieces = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES());
  ghostLevels = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());

  if (numPieces > 1 && this->PieceInvariant)
  {
    // The special execute for structured data handle boundaries internally.
    // PolyData does not need any ghost levels.
    svtkDataObject* dobj = inInfo->Get(svtkDataObject::DATA_OBJECT());
    if (dobj && !strcmp(dobj->GetClassName(), "svtkUnstructuredGrid"))
    { // Processing does nothing fo ghost levels yet so ...
      // Be careful to set output ghost level value one less than default
      // when they are implemented.  I had trouble with multiple executes.
      ++ghostLevels;
    }
  }

  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER(), piece);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES(), numPieces);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS(), ghostLevels);
  inInfo->Set(svtkStreamingDemandDrivenPipeline::EXACT_EXTENT(), 1);

  return 1;
}

//----------------------------------------------------------------------------
int svtkDataSetSurfaceFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->GetUseStrips())
  {
    os << indent << "UseStripsOn\n";
  }
  else
  {
    os << indent << "UseStripsOff\n";
  }

  os << indent << "PieceInvariant: " << this->GetPieceInvariant() << endl;
  os << indent << "PassThroughCellIds: " << (this->GetPassThroughCellIds() ? "On\n" : "Off\n");
  os << indent << "PassThroughPointIds: " << (this->GetPassThroughPointIds() ? "On\n" : "Off\n");

  os << indent << "OriginalCellIdsName: " << this->GetOriginalCellIdsName() << endl;
  os << indent << "OriginalPointIdsName: " << this->GetOriginalPointIdsName() << endl;

  os << indent << "NonlinearSubdivisionLevel: " << this->GetNonlinearSubdivisionLevel() << endl;
}

//========================================================================
// Tris are now degenerate quads so we only need one hash table.
// We might want to change the method names from QuadHash to just Hash.

//----------------------------------------------------------------------------
int svtkDataSetSurfaceFilter::UnstructuredGridExecute(svtkDataSet* dataSetInput, svtkPolyData* output)
{
  svtkUnstructuredGridBase* input = svtkUnstructuredGridBase::SafeDownCast(dataSetInput);

  svtkSmartPointer<svtkCellIterator> cellIter =
    svtkSmartPointer<svtkCellIterator>::Take(input->NewCellIterator());

  // Before we start doing anything interesting, check if we need handle
  // non-linear cells using sub-division.
  bool handleSubdivision = false;
  if (this->NonlinearSubdivisionLevel >= 1)
  {
    // Check to see if the data actually has nonlinear cells.  Handling
    // nonlinear cells adds unnecessary work if we only have linear cells.
    svtkIdType numCells = input->GetNumberOfCells();
    if (input->IsHomogeneous())
    {
      if (numCells >= 1)
      {
        handleSubdivision = !svtkCellTypes::IsLinear(input->GetCellType(0));
      }
    }
    else
    {
      for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
      {
        if (!svtkCellTypes::IsLinear(cellIter->GetCellType()))
        {
          handleSubdivision = true;
          break;
        }
      }
    }
  }

  svtkSmartPointer<svtkUnstructuredGrid> tempInput;
  if (handleSubdivision)
  {
    // Since this filter only properly subdivides 2D cells past
    // level 1, we convert 3D cells to 2D by using
    // svtkUnstructuredGridGeometryFilter.
    svtkNew<svtkUnstructuredGridGeometryFilter> uggf;
    svtkNew<svtkUnstructuredGrid> clone;
    clone->ShallowCopy(input);
    uggf->SetInputData(clone);
    uggf->SetPassThroughCellIds(this->PassThroughCellIds);
    uggf->SetOriginalCellIdsName(this->GetOriginalCellIdsName());
    uggf->SetPassThroughPointIds(this->PassThroughPointIds);
    uggf->SetOriginalPointIdsName(this->GetOriginalPointIdsName());
    uggf->DuplicateGhostCellClippingOff();
    // Disable point merging as it may prevent the correct visualization
    // of non-continuous attributes.
    uggf->MergingOff();
    uggf->Update();

    tempInput = svtkSmartPointer<svtkUnstructuredGrid>::New();
    tempInput->ShallowCopy(uggf->GetOutputDataObject(0));
    input = tempInput;
    cellIter = svtkSmartPointer<svtkCellIterator>::Take(input->NewCellIterator());
  }

  svtkUnsignedCharArray* ghosts = input->GetPointGhostArray();
  svtkCellArray* newVerts;
  svtkCellArray* newLines;
  svtkCellArray* newPolys;
  svtkPoints* newPts;
  svtkIdType* ids;
  int progressCount;
  int i, j;
  int cellType;
  svtkIdType numPts = input->GetNumberOfPoints();
  svtkIdType numCells = input->GetNumberOfCells();
  svtkGenericCell* cell;
  svtkIdList* pointIdList;
  svtkIdType* pointIdArray;
  svtkIdType* pointIdArrayEnd;
  int numFacePts, numCellPts;
  svtkIdType inPtId, outPtId;
  svtkPointData* inputPD = input->GetPointData();
  svtkCellData* inputCD = input->GetCellData();
  svtkFieldData* inputFD = input->GetFieldData();
  svtkCellData* cd = input->GetCellData();
  svtkPointData* outputPD = output->GetPointData();
  svtkCellData* outputCD = output->GetCellData();
  svtkFieldData* outputFD = output->GetFieldData();
  svtkFastGeomQuad* q;

  // Shallow copy field data not associated with points or cells
  outputFD->ShallowCopy(inputFD);

  // These are for the default case/
  svtkIdList* pts;
  svtkPoints* coords;
  svtkCell* face;
  int flag2D = 0;

  // These are for subdividing quadratic cells
  svtkDoubleArray* parametricCoords;
  svtkDoubleArray* parametricCoords2;
  svtkIdList* outPts;
  svtkIdList* outPts2;

  pts = svtkIdList::New();
  coords = svtkPoints::New();
  parametricCoords = svtkDoubleArray::New();
  parametricCoords2 = svtkDoubleArray::New();
  outPts = svtkIdList::New();
  outPts2 = svtkIdList::New();
  // might not be necessary to set the data type for coords
  // but certainly safer to do so
  coords->SetDataType(input->GetPoints()->GetData()->GetDataType());
  cell = svtkGenericCell::New();

  this->NumberOfNewCells = 0;
  this->InitializeQuadHash(numPts);

  // Allocate
  //
  newPts = svtkPoints::New();
  newPts->SetDataType(input->GetPoints()->GetData()->GetDataType());
  newPts->Allocate(numPts);
  newPolys = svtkCellArray::New();
  newPolys->AllocateEstimate(numCells, 3);
  newVerts = svtkCellArray::New();
  newLines = svtkCellArray::New();

  if (this->NonlinearSubdivisionLevel < 2)
  {
    outputPD->CopyGlobalIdsOn();
    outputPD->CopyAllocate(inputPD, numPts, numPts / 2);
  }
  else
  {
    outputPD->InterpolateAllocate(inputPD, numPts, numPts / 2);
  }
  outputCD->CopyGlobalIdsOn();
  outputCD->CopyAllocate(inputCD, numCells, numCells / 2);

  if (this->PassThroughCellIds)
  {
    this->OriginalCellIds = svtkIdTypeArray::New();
    this->OriginalCellIds->SetName(this->GetOriginalCellIdsName());
    this->OriginalCellIds->SetNumberOfComponents(1);
  }
  if (this->PassThroughPointIds)
  {
    this->OriginalPointIds = svtkIdTypeArray::New();
    this->OriginalPointIds->SetName(this->GetOriginalPointIdsName());
    this->OriginalPointIds->SetNumberOfComponents(1);
  }

  // First insert all points.  Points have to come first in poly data.
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal(); cellIter->GoToNextCell())
  {
    cellType = cellIter->GetCellType();

    // A couple of common cases to see if things go faster.
    if (cellType == SVTK_VERTEX || cellType == SVTK_POLY_VERTEX)
    {
      pointIdList = cellIter->GetPointIds();
      numCellPts = pointIdList->GetNumberOfIds();
      pointIdArray = pointIdList->GetPointer(0);
      pointIdArrayEnd = pointIdArray + numCellPts;
      newVerts->InsertNextCell(numCellPts);
      while (pointIdArray != pointIdArrayEnd)
      {
        outPtId = this->GetOutputPointId(*(pointIdArray++), input, newPts, outputPD);
        newVerts->InsertCellPoint(outPtId);
      }
      svtkIdType cellId = cellIter->GetCellId();
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
    }
  }

  // Traverse cells to extract geometry
  //
  progressCount = 0;
  int abort = 0;
  svtkIdType progressInterval = numCells / 20 + 1;

  // First insert all points lines in output and 3D geometry in hash.
  // Save 2D geometry for second pass.
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal() && !abort;
       cellIter->GoToNextCell())
  {
    svtkIdType cellId = cellIter->GetCellId();
    // Progress and abort method support
    if (progressCount >= progressInterval)
    {
      svtkDebugMacro(<< "Process cell #" << cellId);
      this->UpdateProgress(static_cast<double>(cellId) / numCells);
      abort = this->GetAbortExecute();
      progressCount = 0;
    }
    progressCount++;

    cellType = cellIter->GetCellType();
    switch (cellType)
    {
      case SVTK_VERTEX:
      case SVTK_POLY_VERTEX:
        // Do nothing -- these were handled previously.
        break;

      case SVTK_LINE:
      case SVTK_POLY_LINE:
        pointIdList = cellIter->GetPointIds();
        numCellPts = pointIdList->GetNumberOfIds();
        pointIdArray = pointIdList->GetPointer(0);
        pointIdArrayEnd = pointIdArray + numCellPts;

        newLines->InsertNextCell(numCellPts);
        while (pointIdArray != pointIdArrayEnd)
        {
          outPtId = this->GetOutputPointId(*(pointIdArray++), input, newPts, outputPD);
          newLines->InsertCellPoint(outPtId);
        }

        this->RecordOrigCellId(this->NumberOfNewCells, cellId);
        outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
        break;
      case SVTK_BEZIER_CURVE:
        pointIdList = cellIter->GetPointIds();
        numCellPts = pointIdList->GetNumberOfIds();
        pointIdArray = pointIdList->GetPointer(0);
        pointIdArrayEnd = pointIdArray + numCellPts;

        newLines->InsertNextCell(numCellPts);

        outPtId = this->GetOutputPointId(*(pointIdList->GetPointer(0)), input, newPts, outputPD);
        newLines->InsertCellPoint(outPtId);

        pointIdArray += 2;
        while (pointIdArray != pointIdArrayEnd)
        {
          outPtId = this->GetOutputPointId(*(pointIdArray++), input, newPts, outputPD);
          newLines->InsertCellPoint(outPtId);
        }
        outPtId = this->GetOutputPointId(*(pointIdList->GetPointer(1)), input, newPts, outputPD);
        newLines->InsertCellPoint(outPtId);

        this->RecordOrigCellId(this->NumberOfNewCells, cellId);
        outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
        break;

      case SVTK_HEXAHEDRON:
        pointIdList = cellIter->GetPointIds();
        ids = pointIdList->GetPointer(0);
        this->InsertQuadInHash(ids[0], ids[1], ids[5], ids[4], cellId);
        this->InsertQuadInHash(ids[0], ids[3], ids[2], ids[1], cellId);
        this->InsertQuadInHash(ids[0], ids[4], ids[7], ids[3], cellId);
        this->InsertQuadInHash(ids[1], ids[2], ids[6], ids[5], cellId);
        this->InsertQuadInHash(ids[2], ids[3], ids[7], ids[6], cellId);
        this->InsertQuadInHash(ids[4], ids[5], ids[6], ids[7], cellId);
        break;

      case SVTK_VOXEL:
        pointIdList = cellIter->GetPointIds();
        ids = pointIdList->GetPointer(0);
        this->InsertQuadInHash(ids[0], ids[1], ids[5], ids[4], cellId);
        this->InsertQuadInHash(ids[0], ids[2], ids[3], ids[1], cellId);
        this->InsertQuadInHash(ids[0], ids[4], ids[6], ids[2], cellId);
        this->InsertQuadInHash(ids[1], ids[3], ids[7], ids[5], cellId);
        this->InsertQuadInHash(ids[2], ids[6], ids[7], ids[3], cellId);
        this->InsertQuadInHash(ids[4], ids[5], ids[7], ids[6], cellId);
        break;

      case SVTK_TETRA:
        pointIdList = cellIter->GetPointIds();
        ids = pointIdList->GetPointer(0);
        this->InsertTriInHash(ids[0], ids[1], ids[3], cellId, 2);
        this->InsertTriInHash(ids[0], ids[2], ids[1], cellId, 3);
        this->InsertTriInHash(ids[0], ids[3], ids[2], cellId, 1);
        this->InsertTriInHash(ids[1], ids[2], ids[3], cellId, 0);
        break;

      case SVTK_PENTAGONAL_PRISM:
        pointIdList = cellIter->GetPointIds();
        ids = pointIdList->GetPointer(0);
        this->InsertQuadInHash(ids[0], ids[1], ids[6], ids[5], cellId);
        this->InsertQuadInHash(ids[1], ids[2], ids[7], ids[6], cellId);
        this->InsertQuadInHash(ids[2], ids[3], ids[8], ids[7], cellId);
        this->InsertQuadInHash(ids[3], ids[4], ids[9], ids[8], cellId);
        this->InsertQuadInHash(ids[4], ids[0], ids[5], ids[9], cellId);
        this->InsertPolygonInHash(ids, 5, cellId);
        this->InsertPolygonInHash(&ids[5], 5, cellId);
        break;

      case SVTK_HEXAGONAL_PRISM:
        pointIdList = cellIter->GetPointIds();
        ids = pointIdList->GetPointer(0);
        this->InsertQuadInHash(ids[0], ids[1], ids[7], ids[6], cellId);
        this->InsertQuadInHash(ids[1], ids[2], ids[8], ids[7], cellId);
        this->InsertQuadInHash(ids[2], ids[3], ids[9], ids[8], cellId);
        this->InsertQuadInHash(ids[3], ids[4], ids[10], ids[9], cellId);
        this->InsertQuadInHash(ids[4], ids[5], ids[11], ids[10], cellId);
        this->InsertQuadInHash(ids[5], ids[0], ids[6], ids[11], cellId);
        this->InsertPolygonInHash(ids, 6, cellId);
        this->InsertPolygonInHash(&ids[6], 6, cellId);
        break;

      case SVTK_PYRAMID:
        pointIdList = cellIter->GetPointIds();
        ids = pointIdList->GetPointer(0);
        this->InsertQuadInHash(ids[3], ids[2], ids[1], ids[0], cellId);
        this->InsertTriInHash(ids[0], ids[1], ids[4], cellId);
        this->InsertTriInHash(ids[1], ids[2], ids[4], cellId);
        this->InsertTriInHash(ids[2], ids[3], ids[4], cellId);
        this->InsertTriInHash(ids[3], ids[0], ids[4], cellId);
        break;

      case SVTK_WEDGE:
        pointIdList = cellIter->GetPointIds();
        ids = pointIdList->GetPointer(0);
        this->InsertQuadInHash(ids[0], ids[2], ids[5], ids[3], cellId);
        this->InsertQuadInHash(ids[1], ids[0], ids[3], ids[4], cellId);
        this->InsertQuadInHash(ids[2], ids[1], ids[4], ids[5], cellId);
        this->InsertTriInHash(ids[0], ids[1], ids[2], cellId);
        this->InsertTriInHash(ids[3], ids[5], ids[4], cellId);
        break;

      case SVTK_PIXEL:
      case SVTK_QUAD:
      case SVTK_TRIANGLE:
      case SVTK_POLYGON:
      case SVTK_TRIANGLE_STRIP:
      case SVTK_QUADRATIC_TRIANGLE:
      case SVTK_BIQUADRATIC_TRIANGLE:
      case SVTK_QUADRATIC_QUAD:
      case SVTK_QUADRATIC_LINEAR_QUAD:
      case SVTK_BIQUADRATIC_QUAD:
      case SVTK_QUADRATIC_POLYGON:
      case SVTK_LAGRANGE_TRIANGLE:
      case SVTK_LAGRANGE_QUADRILATERAL:
      case SVTK_BEZIER_TRIANGLE:
      case SVTK_BEZIER_QUADRILATERAL:
        // save 2D cells for third pass
        flag2D = 1;
        break;

      default:
      {
        // Default way of getting faces. Differentiates between linear
        // and higher order cells.
        cellIter->GetCell(cell);
        if (cell->IsLinear())
        {
          if (cell->GetCellDimension() == 3)
          {
            int numFaces = cell->GetNumberOfFaces();
            for (j = 0; j < numFaces; j++)
            {
              face = cell->GetFace(j);
              numFacePts = face->GetNumberOfPoints();
              if (numFacePts == 4)
              {
                this->InsertQuadInHash(face->PointIds->GetId(0), face->PointIds->GetId(1),
                  face->PointIds->GetId(2), face->PointIds->GetId(3), cellId);
              }
              else if (numFacePts == 3)
              {
                this->InsertTriInHash(face->PointIds->GetId(0), face->PointIds->GetId(1),
                  face->PointIds->GetId(2), cellId);
              }
              else
              {
                this->InsertPolygonInHash(
                  face->PointIds->GetPointer(0), face->PointIds->GetNumberOfIds(), cellId);
              }
            } // for all cell faces
          }   // if 3D
          else
          {
            svtkDebugMacro("Missing cell type.");
          }
        }    // a linear cell type
        else // process nonlinear cells via triangulation
        {
          input->SetCellOrderAndRationalWeights(cellId, cell);
          if (cell->GetCellDimension() == 1)
          {
            cell->Triangulate(0, pts, coords);
            for (i = 0; i < pts->GetNumberOfIds(); i += 2)
            {
              newLines->InsertNextCell(2);
              inPtId = pts->GetId(i);
              this->RecordOrigCellId(this->NumberOfNewCells, cellId);
              outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
              outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD);
              newLines->InsertCellPoint(outPtId);
              inPtId = pts->GetId(i + 1);
              outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD);
              newLines->InsertCellPoint(outPtId);
            }
          }
          else if (cell->GetCellDimension() == 2)
          {
            svtkWarningMacro(<< "2-D nonlinear cells must be processed with all other 2-D cells.");
          }
          else // 3D nonlinear cell
          {
            svtkIdList* cellIds = svtkIdList::New();
            int numFaces = cell->GetNumberOfFaces();
            for (j = 0; j < numFaces; j++)
            {
              face = cell->GetFace(j);
              input->GetCellNeighbors(cellId, face->PointIds, cellIds);
              if (cellIds->GetNumberOfIds() <= 0)
              {
                // FIXME: Face could not be consistent. svtkOrderedTriangulator is a better option
                if (this->NonlinearSubdivisionLevel >= 1)
                {
                  // TODO: Handle NonlinearSubdivisionLevel > 1 correctly.
                  face->Triangulate(0, pts, coords);
                  for (i = 0; i < pts->GetNumberOfIds(); i += 3)
                  {
                    this->InsertTriInHash(
                      pts->GetId(i), pts->GetId(i + 1), pts->GetId(i + 2), cellId);
                  }
                }
                else
                {
                  switch (face->GetCellType())
                  {
                    case SVTK_QUADRATIC_TRIANGLE:
                    case SVTK_LAGRANGE_TRIANGLE:
                    case SVTK_BEZIER_TRIANGLE:
                      this->InsertTriInHash(face->PointIds->GetId(0), face->PointIds->GetId(1),
                        face->PointIds->GetId(2), cellId);
                      break;
                    case SVTK_QUADRATIC_QUAD:
                    case SVTK_BIQUADRATIC_QUAD:
                    case SVTK_QUADRATIC_LINEAR_QUAD:
                    case SVTK_LAGRANGE_QUADRILATERAL:
                    case SVTK_BEZIER_QUADRILATERAL:
                      this->InsertQuadInHash(face->PointIds->GetId(0), face->PointIds->GetId(1),
                        face->PointIds->GetId(2), face->PointIds->GetId(3), cellId);
                      break;
                    default:
                      svtkWarningMacro(<< "Encountered unknown nonlinear face.");
                      break;
                  } // switch cell type
                }   // subdivision level
              }     // cell has ids
            }       // for faces
            cellIds->Delete();
          } // 3d cell
        }   // nonlinear cell
      }     // default switch case
    }       // switch(cellType)
  }         // for all cells.

  // It would be possible to add these (except for polygons with 5+ sides)
  // to the hashes.  Alternatively, the higher order 2d cells could be handled
  // in the following loop.

  // Now insert 2DCells.  Because of poly datas (cell data) ordering,
  // the 2D cells have to come after points and lines.
  for (cellIter->InitTraversal(); !cellIter->IsDoneWithTraversal() && !abort && flag2D;
       cellIter->GoToNextCell())
  {
    svtkIdType cellId = cellIter->GetCellId();
    cellType = cellIter->GetCellType();
    numCellPts = cellIter->GetNumberOfPoints();

    // If we have a quadratic face and our subdivision level is zero, just treat
    // it as a linear cell.  This should work so long as the first points of the
    // quadratic cell correspond to all those of the equivalent linear cell
    // (which all the current definitions do).
    if (this->NonlinearSubdivisionLevel < 1)
    {
      switch (cellType)
      {
        case SVTK_QUADRATIC_TRIANGLE:
        case SVTK_LAGRANGE_TRIANGLE:
        case SVTK_BEZIER_TRIANGLE:
          cellType = SVTK_TRIANGLE;
          numCellPts = 3;
          break;
        case SVTK_QUADRATIC_QUAD:
        case SVTK_BIQUADRATIC_QUAD:
        case SVTK_QUADRATIC_LINEAR_QUAD:
        case SVTK_LAGRANGE_QUADRILATERAL:
        case SVTK_BEZIER_QUADRILATERAL:
          cellType = SVTK_POLYGON;
          numCellPts = 4;
          break;
      }
    }

    // A couple of common cases to see if things go faster.
    if (cellType == SVTK_PIXEL)
    { // Do we really want to insert the 2D cells into a hash?
      pointIdList = cellIter->GetPointIds();
      ids = pointIdList->GetPointer(0);
      pts->Reset();
      pts->InsertId(0, this->GetOutputPointId(ids[0], input, newPts, outputPD));
      pts->InsertId(1, this->GetOutputPointId(ids[1], input, newPts, outputPD));
      pts->InsertId(2, this->GetOutputPointId(ids[3], input, newPts, outputPD));
      pts->InsertId(3, this->GetOutputPointId(ids[2], input, newPts, outputPD));
      newPolys->InsertNextCell(pts);
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
    }
    else if (cellType == SVTK_POLYGON || cellType == SVTK_TRIANGLE || cellType == SVTK_QUAD)
    {
      pointIdList = cellIter->GetPointIds();
      ids = pointIdList->GetPointer(0);
      pts->Reset();
      for (i = 0; i < numCellPts; i++)
      {
        inPtId = ids[i];
        outPtId = this->GetOutputPointId(inPtId, input, newPts, outputPD);
        pts->InsertId(i, outPtId);
      }
      newPolys->InsertNextCell(pts);
      this->RecordOrigCellId(this->NumberOfNewCells, cellId);
      outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
    }
    else if (cellType == SVTK_TRIANGLE_STRIP)
    {
      pointIdList = cellIter->GetPointIds();
      ids = pointIdList->GetPointer(0);
      // Change strips to triangles so we do not have to worry about order.
      int toggle = 0;
      svtkIdType ptIds[3];
      // This check is not really necessary.  It was put here because of another (now fixed) bug.
      if (numCellPts > 1)
      {
        ptIds[0] = this->GetOutputPointId(ids[0], input, newPts, outputPD);
        ptIds[1] = this->GetOutputPointId(ids[1], input, newPts, outputPD);
        for (i = 2; i < numCellPts; ++i)
        {
          ptIds[2] = this->GetOutputPointId(ids[i], input, newPts, outputPD);
          newPolys->InsertNextCell(3, ptIds);
          this->RecordOrigCellId(this->NumberOfNewCells, cellId);
          outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
          ptIds[toggle] = ptIds[2];
          toggle = !toggle;
        }
      }
    }
    else if (cellType == SVTK_QUADRATIC_TRIANGLE || cellType == SVTK_BIQUADRATIC_TRIANGLE ||
      cellType == SVTK_QUADRATIC_QUAD || cellType == SVTK_BIQUADRATIC_QUAD ||
      cellType == SVTK_QUADRATIC_LINEAR_QUAD || cellType == SVTK_QUADRATIC_POLYGON ||
      cellType == SVTK_LAGRANGE_TRIANGLE || cellType == SVTK_LAGRANGE_QUADRILATERAL ||
      cellType == SVTK_BEZIER_TRIANGLE || cellType == SVTK_BEZIER_QUADRILATERAL)
    {
      // If one of the points is hidden (meaning invalid), do not
      // extract surface cell.
      // Removed checking for whether all points are ghost, because that's an
      // incorrect assumption.
      bool oneHidden = false;
      pointIdList = cellIter->GetPointIds();
      svtkIdType nIds = pointIdList->GetNumberOfIds();
      if (ghosts)
      {
        for (i = 0; i < nIds; i++)
        {
          unsigned char val = ghosts->GetValue(pointIdList->GetId(i));
          if (val & svtkDataSetAttributes::HIDDENPOINT)
          {
            oneHidden = true;
            break;
          }
        }
      }
      if (oneHidden)
      {
        continue;
      }

      // Note: we should not be here if this->NonlinearSubdivisionLevel is less
      // than 1.  See the check above.
      cellIter->GetCell(cell);

      // If the cell is of Bezier type, the weights might be rational and the degree nonuniform.
      // This need to be initiated.

      input->SetCellOrderAndRationalWeights(cellId, cell);

      cell->Triangulate(0, pts, coords);

      // Copy the level 1 subdivision points (which also exist in the input and
      // can therefore just be copied over.  Note that the output of Triangulate
      // records triangles in pts where each 3 points defines a triangle.  We
      // will keep this invariant and also keep the same invariant in
      // parametericCoords and outPts later.
      // Note that coords is actually not used by default, but only the pts ids are used.
      // This is problem for Bezier cells, because the interior points are non-interpolartory,
      // and so, cannot be used as trangulation point.
      // For this reason, the output from Triangulate is overload in the case of Bezier cell,
      // to get the projection of the non-interpolate points

      outPts->Reset();

      switch (cellType)
      {
        case SVTK_BEZIER_QUADRILATERAL:
        {
          int subId = -1;
          double wcoords[3];
          std::vector<double> weights(cell->GetNumberOfPoints());
          svtkBezierQuadrilateral* cellBezier =
            dynamic_cast<svtkBezierQuadrilateral*>(cell->GetRepresentativeCell());
          for (i = 0; i < pts->GetNumberOfIds(); i++)
          {
            svtkIdType op;
            op = this->GetOutputPointId(pts->GetId(i), input, newPts, outputPD);
            cellBezier->EvaluateLocationProjectedNode(
              subId, pts->GetId(i), wcoords, weights.data());
            newPts->SetPoint(op, wcoords);
            outputPD->InterpolatePoint(
              input->GetPointData(), op, cell->GetPointIds(), weights.data());
            outPts->InsertNextId(op);
          }
          break;
        }
        case SVTK_BEZIER_TRIANGLE:
        {
          int subId = -1;
          double wcoords[3];
          std::vector<double> weights(cell->GetNumberOfPoints());
          svtkBezierTriangle* cellBezier =
            dynamic_cast<svtkBezierTriangle*>(cell->GetRepresentativeCell());
          for (i = 0; i < pts->GetNumberOfIds(); i++)
          {
            svtkIdType op;
            op = this->GetOutputPointId(pts->GetId(i), input, newPts, outputPD);
            cellBezier->EvaluateLocationProjectedNode(
              subId, pts->GetId(i), wcoords, weights.data());
            newPts->SetPoint(op, wcoords);
            outputPD->InterpolatePoint(
              input->GetPointData(), op, cell->GetPointIds(), weights.data());
            outPts->InsertNextId(op);
          }
          break;
        }
        default:
        {
          for (i = 0; i < pts->GetNumberOfIds(); i++)
          {
            svtkIdType op;
            op = this->GetOutputPointId(pts->GetId(i), input, newPts, outputPD);
            outPts->InsertNextId(op);
          }
          break;
        }
      }

      // Do any further subdivision if necessary.
      double* pc = cell->GetParametricCoords();
      if (this->NonlinearSubdivisionLevel > 1 && pc)
      {
        // We are going to need parametric coordinates to further subdivide.
        parametricCoords->Reset();
        parametricCoords->SetNumberOfComponents(3);
        for (i = 0; i < pts->GetNumberOfIds(); i++)
        {
          svtkIdType ptId = pts->GetId(i);
          svtkIdType cellPtId;
          for (cellPtId = 0; cell->GetPointId(cellPtId) != ptId; cellPtId++)
          {
          }
          parametricCoords->InsertNextTypedTuple(pc + 3 * cellPtId);
        }
        // Subdivide these triangles as many more times as necessary.  Remember
        // that we have already done the first subdivision.
        for (j = 1; j < this->NonlinearSubdivisionLevel; j++)
        {
          parametricCoords2->Reset();
          parametricCoords2->SetNumberOfComponents(3);
          outPts2->Reset();
          // Each triangle will be split into 4 triangles.
          for (i = 0; i < outPts->GetNumberOfIds(); i += 3)
          {
            // Hold the input point ids and parametric coordinates.  First 3
            // indices are the original points.  Second three are the midpoints
            // in the edges (0,1), (1,2) and (2,0), respectively (see comment
            // below).
            svtkIdType inPts[6];
            double inParamCoords[6][3];
            int k;
            for (k = 0; k < 3; k++)
            {
              inPts[k] = outPts->GetId(i + k);
              parametricCoords->GetTypedTuple(i + k, inParamCoords[k]);
            }
            for (k = 3; k < 6; k++)
            {
              int pt1 = k - 3;
              int pt2 = (pt1 < 2) ? (pt1 + 1) : 0;
              inParamCoords[k][0] = 0.5 * (inParamCoords[pt1][0] + inParamCoords[pt2][0]);
              inParamCoords[k][1] = 0.5 * (inParamCoords[pt1][1] + inParamCoords[pt2][1]);
              inParamCoords[k][2] = 0.5 * (inParamCoords[pt1][2] + inParamCoords[pt2][2]);
              inPts[k] = GetInterpolatedPointId(
                inPts[pt1], inPts[pt2], input, cell, inParamCoords[k], newPts, outputPD);
            }
            //       * 0
            //      / \        Use the 6 points recorded
            //     /   \       in inPts and inParamCoords
            //  3 *-----* 5    to create the 4 triangles
            //   / \   / \     shown here.
            //  /   \ /   \    .
            // *-----*-----*
            // 1     4     2
            const int subtriangles[12] = { 0, 3, 5, 3, 1, 4, 3, 4, 5, 5, 4, 2 };
            for (k = 0; k < 12; k++)
            {
              int localId = subtriangles[k];
              outPts2->InsertNextId(inPts[localId]);
              parametricCoords2->InsertNextTypedTuple(inParamCoords[localId]);
            }
          } // Iterate over triangles
          // Now that we have recorded the subdivided triangles in outPts2 and
          // parametricCoords2, swap them with outPts and parametricCoords to
          // make them the current ones.
          std::swap(outPts, outPts2);
          std::swap(parametricCoords, parametricCoords2);
        } // Iterate over subdivision levels
      }   // If further subdivision

      // Now that we have done all the subdivisions and created all of the
      // points, record the triangles.
      for (i = 0; i < outPts->GetNumberOfIds(); i += 3)
      {
        newPolys->InsertNextCell(3, outPts->GetPointer(i));
        this->RecordOrigCellId(this->NumberOfNewCells, cellId);
        outputCD->CopyData(cd, cellId, this->NumberOfNewCells++);
      }
    }
  } // for all cells.

  // Now transfer geometry from hash to output (only triangles and quads).
  this->InitQuadHashTraversal();
  while ((q = this->GetNextVisibleQuadFromHash()))
  {
    // If one of the points is hidden (meaning invalid), do not
    // extract surface cell.
    // Removed checking for whether all points are ghost, because that's an
    // incorrect assumption.
    bool oneHidden = false;
    // handle all polys
    for (i = 0; i < q->numPts; i++)
    {
      if (ghosts)
      {
        unsigned char val = ghosts->GetValue(q->ptArray[i]);
        if (val & svtkDataSetAttributes::HIDDENPOINT)
        {
          oneHidden = true;
        }
      }

      q->ptArray[i] = this->GetOutputPointId(q->ptArray[i], input, newPts, outputPD);
    }

    if (oneHidden)
    {
      continue;
    }
    newPolys->InsertNextCell(q->numPts, q->ptArray);
    this->RecordOrigCellId(this->NumberOfNewCells, q);
    outputCD->CopyData(inputCD, q->SourceId, this->NumberOfNewCells++);
  }

  if (this->PassThroughCellIds)
  {
    outputCD->AddArray(this->OriginalCellIds);
  }
  if (this->PassThroughPointIds)
  {
    outputPD->AddArray(this->OriginalPointIds);
  }

  // Update ourselves and release memory
  //
  cell->Delete();
  coords->Delete();
  pts->Delete();
  parametricCoords->Delete();
  parametricCoords2->Delete();
  outPts->Delete();
  outPts2->Delete();

  output->SetPoints(newPts);
  newPts->Delete();
  output->SetPolys(newPolys);
  newPolys->Delete();
  if (newVerts->GetNumberOfCells() > 0)
  {
    output->SetVerts(newVerts);
  }
  newVerts->Delete();
  newVerts = nullptr;
  if (newLines->GetNumberOfCells() > 0)
  {
    output->SetLines(newLines);
  }
  newLines->Delete();

  // free storage
  output->Squeeze();
  if (this->OriginalCellIds != nullptr)
  {
    this->OriginalCellIds->Delete();
    this->OriginalCellIds = nullptr;
  }
  if (this->OriginalPointIds != nullptr)
  {
    this->OriginalPointIds->Delete();
    this->OriginalPointIds = nullptr;
  }

  this->DeleteQuadHash();

  return 1;
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::InitializeQuadHash(svtkIdType numPoints)
{
  svtkIdType i;

  if (this->QuadHash)
  {
    this->DeleteQuadHash();
  }

  // Prepare our special quad allocator (for efficiency).
  this->InitFastGeomQuadAllocation(numPoints);

  this->QuadHash = new svtkFastGeomQuad*[numPoints];
  this->QuadHashLength = numPoints;
  this->PointMap = new svtkIdType[numPoints];
  for (i = 0; i < numPoints; ++i)
  {
    this->QuadHash[i] = nullptr;
    this->PointMap[i] = -1;
  }
  this->EdgeMap = new svtkEdgeInterpolationMap;
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::DeleteQuadHash()
{
  svtkIdType i;

  this->DeleteAllFastGeomQuads();

  for (i = 0; i < this->QuadHashLength; ++i)
  {
    this->QuadHash[i] = nullptr;
  }

  delete[] this->QuadHash;
  this->QuadHash = nullptr;
  this->QuadHashLength = 0;
  delete[] this->PointMap;
  this->PointMap = nullptr;
  delete this->EdgeMap;
  this->EdgeMap = nullptr;
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::InsertQuadInHash(
  svtkIdType a, svtkIdType b, svtkIdType c, svtkIdType d, svtkIdType sourceId)
{
  svtkIdType tmp;
  svtkFastGeomQuad *quad, **end;

  // Reorder to get smallest id in a.
  if (b < a && b < c && b < d)
  {
    tmp = a;
    a = b;
    b = c;
    c = d;
    d = tmp;
  }
  else if (c < a && c < b && c < d)
  {
    tmp = a;
    a = c;
    c = tmp;
    tmp = b;
    b = d;
    d = tmp;
  }
  else if (d < a && d < b && d < c)
  {
    tmp = a;
    a = d;
    d = c;
    c = b;
    b = tmp;
  }

  // Look for existing quad in the hash;
  end = this->QuadHash + a;
  quad = *end;
  while (quad)
  {
    end = &(quad->Next);
    // a has to match in this bin.
    // c should be independent of point order.
    if (quad->numPts == 4 && c == quad->ptArray[2])
    {
      // Check both orders for b and d.
      if ((b == quad->ptArray[1] && d == quad->ptArray[3]) ||
        (b == quad->ptArray[3] && d == quad->ptArray[1]))
      {
        // We have a match.
        quad->SourceId = -1;
        // That is all we need to do.  Hide any quad shared by two or more cells.
        return;
      }
    }
    quad = *end;
  }

  // Create a new quad and add it to the hash.
  quad = this->NewFastGeomQuad(4);
  quad->Next = nullptr;
  quad->SourceId = sourceId;
  quad->ptArray[0] = a;
  quad->ptArray[1] = b;
  quad->ptArray[2] = c;
  quad->ptArray[3] = d;
  *end = quad;
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::InsertTriInHash(
  svtkIdType a, svtkIdType b, svtkIdType c, svtkIdType sourceId, svtkIdType svtkNotUsed(faceId) /*= -1*/)
{
  svtkIdType tmp;
  svtkFastGeomQuad *quad, **end;

  // Reorder to get smallest id in a.
  if (b < a && b < c)
  {
    tmp = a;
    a = b;
    b = c;
    c = tmp;
  }
  else if (c < a && c < b)
  {
    tmp = a;
    a = c;
    c = b;
    b = tmp;
  }
  // We can't put the second smnallest in b because it might change the order
  // of the vertices in the final triangle.

  // Look for existing tri in the hash;
  end = this->QuadHash + a;
  quad = *end;
  while (quad)
  {
    end = &(quad->Next);
    // a has to match in this bin.
    if (quad->numPts == 3)
    {
      if ((b == quad->ptArray[1] && c == quad->ptArray[2]) ||
        (b == quad->ptArray[2] && c == quad->ptArray[1]))
      {
        // We have a match.
        quad->SourceId = -1;
        // That is all we need to do. Hide any tri shared by two or more cells.
        return;
      }
    }
    quad = *end;
  }

  // Create a new quad and add it to the hash.
  quad = this->NewFastGeomQuad(3);
  quad->Next = nullptr;
  quad->SourceId = sourceId;
  quad->ptArray[0] = a;
  quad->ptArray[1] = b;
  quad->ptArray[2] = c;
  *end = quad;
}

// Insert a polygon into the hash.
// Input: an array of vertex ids
//        the start index of the polygon in the array
//        the end index of the polygon in the array
//        the cellId of the polygon
//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::InsertPolygonInHash(
  const svtkIdType* ids, int numPts, svtkIdType sourceId)
{
  // sanity check
  if (numPts == 0)
  {
    return;
  }
  svtkFastGeomQuad *quad, **end;

  // find the index to the smallest id
  svtkIdType offset = 0;
  for (int i = 0; i < numPts; i++)
  {
    if (ids[i] < ids[offset])
    {
      offset = i;
    }
  }

  // copy ids into ordered array with smallest id first
  svtkIdType* tab = new svtkIdType[numPts];
  for (int i = 0; i < numPts; i++)
  {
    tab[i] = ids[(offset + i) % numPts];
  }

  // Look for existing hex in the hash;
  end = this->QuadHash + tab[0];
  quad = *end;
  while (quad)
  {
    end = &(quad->Next);
    // a has to match in this bin.
    // first just check the polygon size.
    bool match = true;
    if (numPts == quad->numPts)
    {
      if (tab[0] == quad->ptArray[0])
      {
        // if the first two points match loop through forwards
        // checking all points
        if (numPts > 1 && tab[1] == quad->ptArray[1])
        {
          for (int i = 2; i < numPts; ++i)
          {
            if (tab[i] != quad->ptArray[i])
            {
              match = false;
              break;
            }
          }
        }
        else
        {
          // check if the points go in the opposite direction
          for (int i = 1; i < numPts; ++i)
          {
            if (tab[numPts - i] != quad->ptArray[i])
            {
              match = false;
              break;
            }
          }
        }
      }
      else
      {
        match = false;
      }
    }
    else
    {
      match = false;
    }

    if (match)
    {
      // We have a match.
      quad->SourceId = -1;
      // That is all we need to do. Hide any tri shared by two or more cells.
      delete[] tab;
      return;
    }
    quad = *end;
  }

  // Create a new quad and add it to the hash.
  quad = this->NewFastGeomQuad(numPts);
  // mark the structure as a polygon
  quad->Next = nullptr;
  quad->SourceId = sourceId;
  for (int i = 0; i < numPts; i++)
  {
    quad->ptArray[i] = tab[i];
  }
  *end = quad;

  delete[] tab;
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::InitFastGeomQuadAllocation(svtkIdType numberOfCells)
{
  int idx;

  this->DeleteAllFastGeomQuads();
  // Allocate 100 pointers to arrays.
  // This should be plenty (unless we have triangle strips) ...
  this->NumberOfFastGeomQuadArrays = 100;
  this->FastGeomQuadArrays = new unsigned char*[this->NumberOfFastGeomQuadArrays];
  // Initialize all to nullptr;
  for (idx = 0; idx < this->NumberOfFastGeomQuadArrays; ++idx)
  {
    this->FastGeomQuadArrays[idx] = nullptr;
  }
  // Set pointer to the beginning.
  this->NextArrayIndex = 0;
  this->NextQuadIndex = 0;

  // size the chunks based on the size of a quadrilateral
  int quadSize = sizeofFastQuad(4);

  // Lets keep the chunk size relatively small.
  if (numberOfCells < 100)
  {
    this->FastGeomQuadArrayLength = 50 * quadSize;
  }
  else
  {
    this->FastGeomQuadArrayLength = (numberOfCells / 2) * quadSize;
  }
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::DeleteAllFastGeomQuads()
{
  for (int idx = 0; idx < this->NumberOfFastGeomQuadArrays; ++idx)
  {
    delete[] this->FastGeomQuadArrays[idx];
    this->FastGeomQuadArrays[idx] = nullptr;
  }
  delete[] this->FastGeomQuadArrays;
  this->FastGeomQuadArrays = nullptr;
  this->FastGeomQuadArrayLength = 0;
  this->NumberOfFastGeomQuadArrays = 0;
  this->NextArrayIndex = 0;
  this->NextQuadIndex = 0;
}

//----------------------------------------------------------------------------
svtkFastGeomQuad* svtkDataSetSurfaceFilter::NewFastGeomQuad(int numPts)
{
  if (this->FastGeomQuadArrayLength == 0)
  {
    svtkErrorMacro("Face hash allocation has not been initialized.");
    return nullptr;
  }

  // see if there's room for this one
  int polySize = sizeofFastQuad(numPts);
  if (this->NextQuadIndex + polySize > this->FastGeomQuadArrayLength)
  {
    ++(this->NextArrayIndex);
    this->NextQuadIndex = 0;
  }

  // Although this should not happen often, check first.
  if (this->NextArrayIndex >= this->NumberOfFastGeomQuadArrays)
  {
    int idx, num;
    unsigned char** newArrays;
    num = this->NumberOfFastGeomQuadArrays * 2;
    newArrays = new unsigned char*[num];
    for (idx = 0; idx < num; ++idx)
    {
      newArrays[idx] = nullptr;
      if (idx < this->NumberOfFastGeomQuadArrays)
      {
        newArrays[idx] = this->FastGeomQuadArrays[idx];
      }
    }
    delete[] this->FastGeomQuadArrays;
    this->FastGeomQuadArrays = newArrays;
    this->NumberOfFastGeomQuadArrays = num;
  }

  // Next: allocate a new array if necessary.
  if (this->FastGeomQuadArrays[this->NextArrayIndex] == nullptr)
  {
    this->FastGeomQuadArrays[this->NextArrayIndex] =
      new unsigned char[this->FastGeomQuadArrayLength];
  }

  svtkFastGeomQuad* q = reinterpret_cast<svtkFastGeomQuad*>(
    this->FastGeomQuadArrays[this->NextArrayIndex] + this->NextQuadIndex);
  q->numPts = numPts;

  const int qsize = sizeof(svtkFastGeomQuad);
  const int sizeId = sizeof(svtkIdType);
  // If necessary, we create padding after svtkFastGeomQuad such that
  // the beginning of ids aligns evenly with sizeof(svtkIdType).
  if (qsize % sizeId == 0)
  {
    q->ptArray = (svtkIdType*)q + qsize / sizeId;
  }
  else
  {
    q->ptArray = (svtkIdType*)q + qsize / sizeId + 1;
  }

  this->NextQuadIndex += polySize;

  return q;
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::InitQuadHashTraversal()
{
  this->QuadHashTraversalIndex = 0;
  this->QuadHashTraversal = this->QuadHash[0];
}

//----------------------------------------------------------------------------
svtkFastGeomQuad* svtkDataSetSurfaceFilter::GetNextVisibleQuadFromHash()
{
  svtkFastGeomQuad* quad;

  quad = this->QuadHashTraversal;

  // Move till traversal until we have a quad to return.
  // Note: the current traversal has not been returned yet.
  while (quad == nullptr || quad->SourceId == -1)
  {
    if (quad)
    { // The quad must be hidden.  Move to the next.
      quad = quad->Next;
    }
    else
    { // must be the end of the linked list.  Move to the next bin.
      this->QuadHashTraversalIndex += 1;
      if (this->QuadHashTraversalIndex >= this->QuadHashLength)
      { // There are no more bins.
        this->QuadHashTraversal = nullptr;
        return nullptr;
      }
      quad = this->QuadHash[this->QuadHashTraversalIndex];
    }
  }

  // Now we have a quad to return.  Set the traversal to the next entry.
  this->QuadHashTraversal = quad->Next;

  return quad;
}

//----------------------------------------------------------------------------
svtkIdType svtkDataSetSurfaceFilter::GetOutputPointId(
  svtkIdType inPtId, svtkDataSet* input, svtkPoints* outPts, svtkPointData* outPD)
{
  svtkIdType outPtId;

  outPtId = this->PointMap[inPtId];
  if (outPtId == -1)
  {
    outPtId = outPts->InsertNextPoint(input->GetPoint(inPtId));
    outPD->CopyData(input->GetPointData(), inPtId, outPtId);
    this->PointMap[inPtId] = outPtId;
    this->RecordOrigPointId(outPtId, inPtId);
  }

  return outPtId;
}

//-----------------------------------------------------------------------------
svtkIdType svtkDataSetSurfaceFilter::GetInterpolatedPointId(svtkIdType edgePtA, svtkIdType edgePtB,
  svtkDataSet* input, svtkCell* cell, double pcoords[3], svtkPoints* outPts, svtkPointData* outPD)
{
  svtkIdType outPtId;

  outPtId = this->EdgeMap->FindEdge(edgePtA, edgePtB);
  if (outPtId == -1)
  {
    int subId = -1;
    double wcoords[3];
    std::vector<double> weights(cell->GetNumberOfPoints());
    cell->EvaluateLocation(subId, pcoords, wcoords, weights.data());
    outPtId = outPts->InsertNextPoint(wcoords);
    outPD->InterpolatePoint(input->GetPointData(), outPtId, cell->GetPointIds(), weights.data());
    this->RecordOrigPointId(outPtId, -1);
    this->EdgeMap->AddEdge(edgePtA, edgePtB, outPtId);
  }

  return outPtId;
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::RecordOrigCellId(svtkIdType destIndex, svtkIdType originalId)
{
  if (this->OriginalCellIds != nullptr)
  {
    this->OriginalCellIds->InsertValue(destIndex, originalId);
  }
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::RecordOrigCellId(svtkIdType destIndex, svtkFastGeomQuad* quad)
{
  if (this->OriginalCellIds != nullptr)
  {
    this->OriginalCellIds->InsertValue(destIndex, quad->SourceId);
  }
}

//----------------------------------------------------------------------------
void svtkDataSetSurfaceFilter::RecordOrigPointId(svtkIdType destIndex, svtkIdType originalId)
{
  if (this->OriginalPointIds != nullptr)
  {
    this->OriginalPointIds->InsertValue(destIndex, originalId);
  }
}
