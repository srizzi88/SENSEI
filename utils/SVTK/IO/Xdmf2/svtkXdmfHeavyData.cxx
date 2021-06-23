/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmfHeavyData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXdmfHeavyData.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellTypes.h"
#include "svtkDataArrayRange.h"
#include "svtkDataObjectTypes.h"
#include "svtkDoubleArray.h"
#include "svtkExtractSelectedIds.h"
#include "svtkFloatArray.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkMath.h"
#include "svtkMergePoints.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredData.h"
#include "svtkStructuredGrid.h"
#include "svtkUniformGrid.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnstructuredGrid.h"
#include "svtkXdmfDataArray.h"
#include "svtkXdmfReader.h"
#include "svtkXdmfReaderInternal.h"

#include <algorithm>
#include <cassert>
#include <deque>
#include <numeric>
#include <type_traits>
#include <vector>

#include "svtk_libxml2.h"
#include SVTKLIBXML2_HEADER(tree.h)

#ifdef SVTK_USE_64BIT_IDS
typedef XdmfInt64 svtkXdmfIdType;
#else
typedef XdmfInt32 svtkXdmfIdType;
#endif

using namespace xdmf2;

static void svtkScaleExtents(int in_exts[6], int out_exts[6], int stride[3])
{
  out_exts[0] = in_exts[0] / stride[0];
  out_exts[1] = in_exts[1] / stride[0];
  out_exts[2] = in_exts[2] / stride[1];
  out_exts[3] = in_exts[3] / stride[1];
  out_exts[4] = in_exts[4] / stride[2];
  out_exts[5] = in_exts[5] / stride[2];
}

static void svtkGetDims(int exts[6], int dims[3])
{
  dims[0] = exts[1] - exts[0] + 1;
  dims[1] = exts[3] - exts[2] + 1;
  dims[2] = exts[5] - exts[4] + 1;
}

//----------------------------------------------------------------------------
svtkXdmfHeavyData::svtkXdmfHeavyData(svtkXdmfDomain* domain, svtkAlgorithm* reader)
{
  this->Reader = reader;
  this->Piece = 0;
  this->NumberOfPieces = 0;
  this->GhostLevels = 0;
  this->Extents[0] = this->Extents[2] = this->Extents[4] = 0;
  this->Extents[1] = this->Extents[3] = this->Extents[5] = -1;
  this->Domain = domain;
  this->Stride[0] = this->Stride[1] = this->Stride[2] = 1;
}

//----------------------------------------------------------------------------
svtkXdmfHeavyData::~svtkXdmfHeavyData() {}

//----------------------------------------------------------------------------
svtkDataObject* svtkXdmfHeavyData::ReadData()
{
  if (this->Domain->GetNumberOfGrids() == 1)
  {
    // There's just 1 grid. Now in serial, this is all good. In parallel, we
    // need to be care:
    // 1. If the data is structured, we respect the update-extent and read
    //    accordingly.
    // 2. If the data is unstructrued, we read only on the root node. The user
    //    can apply D3 or something to repartition the data.
    return this->ReadData(this->Domain->GetGrid(0));
  }

  // this code is similar to ReadComposite() however we cannot use the same code
  // since the API for getting the children differs on the domain and the grid.

  bool distribute_leaf_nodes = this->NumberOfPieces > 1;
  XdmfInt32 numChildren = this->Domain->GetNumberOfGrids();
  int number_of_leaf_nodes = 0;

  svtkMultiBlockDataSet* mb = svtkMultiBlockDataSet::New();
  mb->SetNumberOfBlocks(numChildren);

  for (XdmfInt32 cc = 0; cc < numChildren; cc++)
  {
    XdmfGrid* xmfChild = this->Domain->GetGrid(cc);
    mb->GetMetaData(cc)->Set(svtkCompositeDataSet::NAME(), xmfChild->GetName());
    bool child_is_leaf = (xmfChild->IsUniform() != 0);
    if (!child_is_leaf || !distribute_leaf_nodes ||
      (number_of_leaf_nodes % this->NumberOfPieces) == this->Piece)
    {
      // it's possible that the data has way too many blocks, in which case the
      // reader didn't present the user with capabilities to select the actual
      // leaf node blocks as is the norm, instead only top-level grids were
      // shown. In that case we need to ensure that we skip grids the user
      // wanted us to skip explicitly.
      if (!this->Domain->GetGridSelection()->ArrayIsEnabled(xmfChild->GetName()))
      {
        continue;
      }
      svtkDataObject* childDO = this->ReadData(xmfChild);
      if (childDO)
      {
        mb->SetBlock(cc, childDO);
        childDO->Delete();
      }
    }
    number_of_leaf_nodes += child_is_leaf ? 1 : 0;
  }

  return mb;
}

//----------------------------------------------------------------------------
svtkDataObject* svtkXdmfHeavyData::ReadData(XdmfGrid* xmfGrid, int blockId)
{
  if (!xmfGrid || xmfGrid->GetGridType() == XDMF_GRID_UNSET)
  {
    // sanity check-ensure that the xmfGrid is valid.
    return 0;
  }

  XdmfInt32 gridType = (xmfGrid->GetGridType() & XDMF_GRID_MASK);
  if (gridType == XDMF_GRID_COLLECTION &&
    xmfGrid->GetCollectionType() == XDMF_GRID_COLLECTION_TEMPORAL)
  {
    // grid is a temporal collection, pick the sub-grid with matching time and
    // process that.
    return this->ReadTemporalCollection(xmfGrid, blockId);
  }
  else if (gridType == XDMF_GRID_COLLECTION || gridType == XDMF_GRID_TREE)
  {
    return this->ReadComposite(xmfGrid);
  }

  // grid is a primitive grid, so read the data.
  return this->ReadUniformData(xmfGrid, blockId);
}

//----------------------------------------------------------------------------
svtkDataObject* svtkXdmfHeavyData::ReadComposite(XdmfGrid* xmfComposite)
{
  assert(((xmfComposite->GetGridType() & XDMF_GRID_COLLECTION &&
            xmfComposite->GetCollectionType() != XDMF_GRID_COLLECTION_TEMPORAL) ||
           (xmfComposite->GetGridType() & XDMF_GRID_TREE)) &&
    "Input must be a spatial collection or a tree");

  svtkMultiBlockDataSet* multiBlock = svtkMultiBlockDataSet::New();
  XdmfInt32 numChildren = xmfComposite->GetNumberOfChildren();
  multiBlock->SetNumberOfBlocks(numChildren);

  bool distribute_leaf_nodes =
    (xmfComposite->GetGridType() & XDMF_GRID_COLLECTION && this->NumberOfPieces > 1);

  int number_of_leaf_nodes = 0;
  for (XdmfInt32 cc = 0; cc < numChildren; cc++)
  {
    XdmfGrid* xmfChild = xmfComposite->GetChild(cc);
    multiBlock->GetMetaData(cc)->Set(svtkCompositeDataSet::NAME(), xmfChild->GetName());
    bool child_is_leaf = (xmfChild->IsUniform() != 0);
    if (!child_is_leaf || !distribute_leaf_nodes ||
      (number_of_leaf_nodes % this->NumberOfPieces) == this->Piece)
    {
      svtkDataObject* childDO = this->ReadData(xmfChild, cc);
      if (childDO)
      {
        multiBlock->SetBlock(cc, childDO);
        childDO->Delete();
      }
    }
    number_of_leaf_nodes += child_is_leaf ? 1 : 0;
  }

  return multiBlock;
}

//----------------------------------------------------------------------------
svtkDataObject* svtkXdmfHeavyData::ReadTemporalCollection(
  XdmfGrid* xmfTemporalCollection, int blockId)
{
  assert(xmfTemporalCollection->GetGridType() & XDMF_GRID_COLLECTION &&
    xmfTemporalCollection->GetCollectionType() == XDMF_GRID_COLLECTION_TEMPORAL &&
    "Input must be a temporal collection");

  // Find the children that are valid for the requested time (this->Time) and
  // read only those.

  // FIXME: I am tempted to remove support for supporting multiple matching
  // sub-grids for a time-step since that changes the composite data hierarchy
  // over time which makes it hard to use filters such as svtkExtractBlock etc.

  std::deque<XdmfGrid*> valid_children;
  for (XdmfInt32 cc = 0; cc < xmfTemporalCollection->GetNumberOfChildren(); cc++)
  {
    XdmfGrid* child = xmfTemporalCollection->GetChild(cc);
    if (child)
    {
      // ensure that we set correct epsilon for comparison
      // BUG #0013766.
      child->GetTime()->SetEpsilon(SVTK_DBL_EPSILON);
      if (child->GetTime()->IsValid(this->Time, this->Time))
      {
        valid_children.push_back(child);
      }
    }
  }
  // if no child matched this timestep, handle the case where the user didn't
  // specify any <Time /> element for the temporal collection.
  for (XdmfInt32 cc = 0;
       valid_children.size() == 0 && cc < xmfTemporalCollection->GetNumberOfChildren(); cc++)
  {
    XdmfGrid* child = xmfTemporalCollection->GetChild(cc);
    if (child && child->GetTime()->GetTimeType() == XDMF_TIME_UNSET)
    {
      valid_children.push_back(child);
    }
  }

  if (valid_children.size() == 0)
  {
    return 0;
  }

  std::deque<svtkSmartPointer<svtkDataObject> > child_data_objects;
  std::deque<XdmfGrid*>::iterator iter;
  for (iter = valid_children.begin(); iter != valid_children.end(); ++iter)
  {
    svtkDataObject* childDO = this->ReadData(*iter, blockId);
    if (childDO)
    {
      child_data_objects.push_back(childDO);
      childDO->Delete();
    }
  }

  if (child_data_objects.size() == 1)
  {
    svtkDataObject* dataObject = child_data_objects[0];
    dataObject->Register(nullptr);
    return dataObject;
  }
  else if (child_data_objects.size() > 1)
  {
    svtkMultiBlockDataSet* mb = svtkMultiBlockDataSet::New();
    mb->SetNumberOfBlocks(static_cast<unsigned int>(child_data_objects.size()));
    for (unsigned int cc = 0; cc < static_cast<unsigned int>(child_data_objects.size()); cc++)
    {
      mb->SetBlock(cc, child_data_objects[cc]);
    }
    return mb;
  }

  return 0;
}

//----------------------------------------------------------------------------
// Read a non-composite grid. Note here uniform has nothing to do with
// svtkUniformGrid but to what Xdmf's GridType="Uniform".
svtkDataObject* svtkXdmfHeavyData::ReadUniformData(XdmfGrid* xmfGrid, int blockId)
{
  assert(xmfGrid->IsUniform() && "Input must be a uniform xdmf grid.");

  int svtk_data_type = this->Domain->GetSVTKDataType(xmfGrid);

  if (!this->Domain->GetGridSelection()->ArrayIsEnabled(xmfGrid->GetName()))
  {
    // simply create an empty data-object of the correct type and return it.
    return svtkDataObjectTypes::NewDataObject(svtk_data_type);
  }

  // Read heavy data for grid geometry/topology. This does not read any
  // data-arrays. They are read explicitly.
  XdmfTopology* topo = xmfGrid->GetTopology();
  XdmfGeometry* geom = xmfGrid->GetGeometry();
  xmlChar* filePtr;

  bool caching = true;
  XdmfDOM* topoDom = topo->GetDOM();
  XdmfXmlNode topoNode = topo->GetElement();
  XdmfXmlNode topoNodeDataItem = topoDom->FindElement("DataItem", 0, topoNode);
  std::string topoFilename = "NULL";
  if (topoNodeDataItem && caching)
  {
    filePtr = topoNodeDataItem->children->content;
    if (filePtr != nullptr)
    {
      topoFilename = reinterpret_cast<char*>(filePtr);
    }
    else
    {
      // svtkErrorWithObjectMacro(this->Reader, << "Cannot find DataItem element in topology xml, no
      // caching possible");
      caching = false;
    }
  }
  else
  {
    caching = false;
  }

  XdmfDOM* geomDom = geom->GetDOM();
  XdmfXmlNode geomNode = geom->GetElement();
  XdmfXmlNode geomNodeDataItem = geomDom->FindElement("DataItem", 0, geomNode);
  std::string geomFilename = "NULL";
  if (geomNodeDataItem && caching)
  {
    filePtr = geomNodeDataItem->children->content;
    if (filePtr != nullptr)
    {
      geomFilename = reinterpret_cast<char*>(filePtr);
    }
    else
    {
      svtkErrorWithObjectMacro(
        this->Reader, << "Cannot find DataItem element in geometry xml, no caching possible");
      caching = false;
    }
  }
  else
  {
    caching = false;
  }

  svtkXdmfReader::XdmfReaderCachedData& cache =
    svtkXdmfReader::SafeDownCast(this->Reader)->GetDataSetCache();
  svtkXdmfReader::XdmfDataSetTopoGeoPath& cachedData = cache[blockId];
  if (caching && (cachedData.topologyPath == topoFilename) &&
    (cachedData.geometryPath == geomFilename))
  {
    svtkDataSet* ds = svtkDataSet::SafeDownCast(
      svtkDataObjectTypes::NewDataObject(cachedData.dataset->GetDataObjectType()));
    ds->ShallowCopy(cachedData.dataset);
    this->ReadAttributes(ds, xmfGrid);
    return ds;
  }

  if (caching)
  {
    cachedData.topologyPath = topoFilename;
    cachedData.geometryPath = geomFilename;
    if (cache[blockId].dataset != nullptr)
    {
      cache[blockId].dataset->Delete();
      cache[blockId].dataset = nullptr;
    }
  }

  XdmfInt32 status = xmfGrid->Update();
  if (status == XDMF_FAIL)
  {
    return 0;
  }

  svtkDataObject* dataObject = 0;

  switch (svtk_data_type)
  {
    case SVTK_UNIFORM_GRID:
      dataObject = this->RequestImageData(xmfGrid, true);
      break;

    case SVTK_IMAGE_DATA:
      dataObject = this->RequestImageData(xmfGrid, false);
      break;

    case SVTK_STRUCTURED_GRID:
      dataObject = this->RequestStructuredGrid(xmfGrid);
      break;

    case SVTK_RECTILINEAR_GRID:
      dataObject = this->RequestRectilinearGrid(xmfGrid);
      break;

    case SVTK_UNSTRUCTURED_GRID:
      dataObject = this->ReadUnstructuredGrid(xmfGrid);
      break;

    default:
      // un-handled case.
      return 0;
  }

  if (caching)
  {
    cache[blockId].dataset = svtkDataSet::SafeDownCast(dataObject);
    dataObject->Register(0);
  }
  return dataObject;
}

//----------------------------------------------------------------------------
int svtkXdmfHeavyData::GetNumberOfPointsPerCell(int svtk_cell_type)
{
  switch (svtk_cell_type)
  {
    case SVTK_POLY_VERTEX:
      return 0;
    case SVTK_POLY_LINE:
      return 0;
    case SVTK_POLYGON:
      return 0;

    case SVTK_TRIANGLE:
      return 3;
    case SVTK_QUAD:
      return 4;
    case SVTK_TETRA:
      return 4;
    case SVTK_PYRAMID:
      return 5;
    case SVTK_WEDGE:
      return 6;
    case SVTK_HEXAHEDRON:
      return 8;
    case SVTK_QUADRATIC_EDGE:
      return 3;
    case SVTK_QUADRATIC_TRIANGLE:
      return 6;
    case SVTK_QUADRATIC_QUAD:
      return 8;
    case SVTK_BIQUADRATIC_QUAD:
      return 9;
    case SVTK_QUADRATIC_TETRA:
      return 10;
    case SVTK_QUADRATIC_PYRAMID:
      return 13;
    case SVTK_QUADRATIC_WEDGE:
      return 15;
    case SVTK_BIQUADRATIC_QUADRATIC_WEDGE:
      return 18;
    case SVTK_QUADRATIC_HEXAHEDRON:
      return 20;
    case SVTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON:
      return 24;
    case SVTK_TRIQUADRATIC_HEXAHEDRON:
      return 24;
  }
  return -1;
}
//----------------------------------------------------------------------------
int svtkXdmfHeavyData::GetSVTKCellType(XdmfInt32 topologyType)
{
  switch (topologyType)
  {
    case XDMF_POLYVERTEX:
      return SVTK_POLY_VERTEX;
    case XDMF_POLYLINE:
      return SVTK_POLY_LINE;
    case XDMF_POLYGON:
      return SVTK_POLYGON; // FIXME: should this not be treated as mixed?
    case XDMF_TRI:
      return SVTK_TRIANGLE;
    case XDMF_QUAD:
      return SVTK_QUAD;
    case XDMF_TET:
      return SVTK_TETRA;
    case XDMF_PYRAMID:
      return SVTK_PYRAMID;
    case XDMF_WEDGE:
      return SVTK_WEDGE;
    case XDMF_HEX:
      return SVTK_HEXAHEDRON;
    case XDMF_EDGE_3:
      return SVTK_QUADRATIC_EDGE;
    case XDMF_TRI_6:
      return SVTK_QUADRATIC_TRIANGLE;
    case XDMF_QUAD_8:
      return SVTK_QUADRATIC_QUAD;
    case XDMF_QUAD_9:
      return SVTK_BIQUADRATIC_QUAD;
    case XDMF_TET_10:
      return SVTK_QUADRATIC_TETRA;
    case XDMF_PYRAMID_13:
      return SVTK_QUADRATIC_PYRAMID;
    case XDMF_WEDGE_15:
      return SVTK_QUADRATIC_WEDGE;
    case XDMF_WEDGE_18:
      return SVTK_BIQUADRATIC_QUADRATIC_WEDGE;
    case XDMF_HEX_20:
      return SVTK_QUADRATIC_HEXAHEDRON;
    case XDMF_HEX_24:
      return SVTK_BIQUADRATIC_QUADRATIC_HEXAHEDRON;
    case XDMF_HEX_27:
      return SVTK_TRIQUADRATIC_HEXAHEDRON;
    case XDMF_MIXED:
      return SVTK_NUMBER_OF_CELL_TYPES;
  }
  // XdmfErrorMessage("Unknown Topology Type = "
  //  << xmfGrid->GetTopology()->GetTopologyType());
  return SVTK_EMPTY_CELL;
}

//----------------------------------------------------------------------------
svtkDataObject* svtkXdmfHeavyData::ReadUnstructuredGrid(XdmfGrid* xmfGrid)
{
  svtkSmartPointer<svtkUnstructuredGrid> ugData = svtkSmartPointer<svtkUnstructuredGrid>::New();

  // BUG #12527. For non-partitioned data, don't read unstructured grid on
  // process id > 0.
  if (this->Piece != 0 && this->Domain->GetNumberOfGrids() == 1 &&
    this->Domain->GetSVTKDataType() == SVTK_UNSTRUCTURED_GRID &&
    this->Domain->GetSetsSelection()->GetNumberOfArrays() == 0)
  {
    ugData->Register(nullptr);
    return ugData;
  }

  XdmfTopology* xmfTopology = xmfGrid->GetTopology();
  XdmfArray* xmfConnectivity = xmfTopology->GetConnectivity();

  int svtk_cell_type = svtkXdmfHeavyData::GetSVTKCellType(xmfTopology->GetTopologyType());

  if (svtk_cell_type == SVTK_EMPTY_CELL)
  {
    // invalid topology.
    return nullptr;
  }

  if (svtk_cell_type != SVTK_NUMBER_OF_CELL_TYPES)
  // i.e. topologyType != XDMF_MIXED
  {
    // all cells are of the same type.
    XdmfInt32 numPointsPerCell = xmfTopology->GetNodesPerElement();

    // FIXME: is this needed, shouldn't xmfTopology->GetNodesPerElement()
    // return the correct value always?
    if (xmfConnectivity->GetRank() == 2)
    {
      numPointsPerCell = xmfConnectivity->GetDimension(1);
    }

    /* Create Cell Type Array */
    XdmfInt64 conn_length = xmfConnectivity->GetNumberOfElements();
    std::vector<XdmfInt64> xmfConnections(conn_length);
    xmfConnectivity->GetValues(0, xmfConnections.data(), conn_length);

    svtkIdType numCells = xmfTopology->GetShapeDesc()->GetNumberOfElements();

    svtkNew<svtkIdTypeArray> conn;
    svtkNew<svtkIdTypeArray> offsets;

    offsets->SetNumberOfTuples(numCells + 1);

    { // Fill offsets: {0, 1 * cellSize, 2 * cellSize, ..., numCells * cellSize}
      svtkIdType offset = -numPointsPerCell;
      auto generator = [&]() -> svtkIdType { return offset += numPointsPerCell; };
      auto range = svtk::DataArrayValueRange<1>(offsets);
      std::generate(range.begin(), range.end(), generator);
    }

    conn->SetNumberOfTuples(numPointsPerCell * numCells);

    // Fill connections (just copy xmfConnections)
    { // Need to convert explicitly to silence warnings:
      auto range = svtk::DataArrayValueRange<1>(conn);
      std::transform(xmfConnections.cbegin(),
        xmfConnections.cbegin() + (numPointsPerCell * numCells), range.begin(),
        [](XdmfInt64 val) -> svtkIdType { return static_cast<svtkIdType>(val); });
    }

    // Construct and set the cell array
    svtkNew<svtkCellArray> cells;
    cells->SetData(offsets, conn);
    ugData->SetCells(svtk_cell_type, cells);
  }
  else
  {
    // We have cells with mixed types.
    XdmfInt64 conn_length = xmfGrid->GetTopology()->GetConnectivity()->GetNumberOfElements();
    std::vector<XdmfInt64> xmfConnections(static_cast<size_t>(conn_length));
    xmfConnectivity->GetValues(0, xmfConnections.data(), conn_length);

    svtkIdType numCells = xmfTopology->GetShapeDesc()->GetNumberOfElements();
    svtkNew<svtkUnsignedCharArray> cell_types;
    cell_types->SetNumberOfTuples(numCells);

    svtkNew<svtkIdTypeArray> offsets;
    offsets->SetNumberOfTuples(numCells + 1);

    svtkNew<svtkIdTypeArray> conn;
    // This may be an overestimate; will correct after filling.
    conn->SetNumberOfTuples(static_cast<svtkIdType>(conn_length));

    svtkIdType offset = 0;
    svtkIdType index = 0;
    svtkIdType connIndex = 0;
    for (svtkIdType cc = 0; cc < numCells; cc++)
    {
      int svtk_cell_typeI = this->GetSVTKCellType(xmfConnections[index++]);
      XdmfInt32 numPointsPerCell = this->GetNumberOfPointsPerCell(svtk_cell_typeI);
      if (numPointsPerCell == -1)
      {
        // encountered an unknown cell.
        return nullptr;
      }

      if (numPointsPerCell == 0)
      {
        // cell type does not have a fixed number of points in which case the
        // next entry in xmfConnections tells us the number of points.
        numPointsPerCell = xmfConnections[index++];
      }

      cell_types->SetValue(cc, static_cast<unsigned char>(svtk_cell_typeI));
      offsets->SetValue(cc, offset);
      offset += numPointsPerCell;

      for (svtkIdType i = 0; i < numPointsPerCell; i++)
      {
        conn->SetValue(connIndex++, xmfConnections[index++]);
      }
    }
    offsets->SetValue(numCells, offset); // final offset value

    // Resize the Array to the Proper Size
    conn->Resize(connIndex);

    // Create and set the cell array:
    svtkNew<svtkCellArray> cells;
    cells->SetData(offsets, conn);
    ugData->SetCells(cell_types, cells);
  }

  // Read the geometry.
  svtkPoints* points = this->ReadPoints(xmfGrid->GetGeometry());
  if (!points)
  {
    // failed to read points.
    return nullptr;
  }
  ugData->SetPoints(points);
  points->Delete();

  this->ReadAttributes(ugData, xmfGrid);

  // Read ghost cell/point information.
  this->ReadGhostSets(ugData, xmfGrid);

  // If this grid has sets defined on it, then we need to read those as well
  svtkMultiBlockDataSet* sets = this->ReadSets(ugData, xmfGrid);
  if (sets)
  {
    return sets;
  }

  ugData->Register(nullptr);
  return ugData;
}

inline bool svtkExtentsAreValid(int exts[6])
{
  return exts[1] >= exts[0] && exts[3] >= exts[2] && exts[5] >= exts[4];
}

inline bool svtkExtentsAreEqual(int* exts1, int* exts2)
{
  if (!exts1 && !exts2)
  {
    return true;
  }
  if (!exts1 || !exts2)
  {
    return false;
  }
  return (exts1[0] == exts2[0] && exts1[1] == exts2[1] && exts1[2] == exts2[2] &&
    exts1[3] == exts2[3] && exts1[4] == exts2[4] && exts1[5] == exts2[5]);
}

//-----------------------------------------------------------------------------
svtkRectilinearGrid* svtkXdmfHeavyData::RequestRectilinearGrid(XdmfGrid* xmfGrid)
{
  svtkSmartPointer<svtkRectilinearGrid> rg = svtkSmartPointer<svtkRectilinearGrid>::New();
  int whole_extents[6];
  int update_extents[6];
  this->Domain->GetWholeExtent(xmfGrid, whole_extents);

  if (!svtkExtentsAreValid(this->Extents))
  {
    // if this->Extents are not valid, then simply read the whole image.
    memcpy(update_extents, whole_extents, sizeof(int) * 6);
  }
  else
  {
    memcpy(update_extents, this->Extents, sizeof(int) * 6);
  }

  // convert to stridden update extents.
  int scaled_extents[6];
  svtkScaleExtents(update_extents, scaled_extents, this->Stride);
  int scaled_dims[3];
  svtkGetDims(scaled_extents, scaled_dims);

  rg->SetExtent(scaled_extents);

  // Now read rectilinear geometry.
  XdmfGeometry* xmfGeometry = xmfGrid->GetGeometry();

  svtkSmartPointer<svtkDoubleArray> xarray = svtkSmartPointer<svtkDoubleArray>::New();
  xarray->SetNumberOfTuples(scaled_dims[0]);

  svtkSmartPointer<svtkDoubleArray> yarray = svtkSmartPointer<svtkDoubleArray>::New();
  yarray->SetNumberOfTuples(scaled_dims[1]);

  svtkSmartPointer<svtkDoubleArray> zarray = svtkSmartPointer<svtkDoubleArray>::New();
  zarray->SetNumberOfTuples(scaled_dims[2]);

  rg->SetXCoordinates(xarray);
  rg->SetYCoordinates(yarray);
  rg->SetZCoordinates(zarray);

  switch (xmfGeometry->GetGeometryType())
  {
    case XDMF_GEOMETRY_ORIGIN_DXDY:
    case XDMF_GEOMETRY_ORIGIN_DXDYDZ:
    {
      XdmfFloat64* origin = xmfGeometry->GetOrigin();
      XdmfFloat64* dxdydz = xmfGeometry->GetDxDyDz();
      for (int cc = scaled_extents[0]; cc <= scaled_extents[1]; cc++)
      {
        xarray->GetPointer(0)[cc - scaled_extents[0]] =
          origin[0] + (dxdydz[0] * cc * this->Stride[0]);
      }
      for (int cc = scaled_extents[2]; cc <= scaled_extents[3]; cc++)
      {
        yarray->GetPointer(0)[cc - scaled_extents[2]] =
          origin[1] + (dxdydz[1] * cc * this->Stride[1]);
      }
      for (int cc = scaled_extents[4]; cc <= scaled_extents[5]; cc++)
      {
        zarray->GetPointer(0)[cc - scaled_extents[4]] =
          origin[2] + (dxdydz[2] * cc * this->Stride[2]);
      }
    }
    break;

    case XDMF_GEOMETRY_VXVY:
    {
      // Note:
      // XDMF and SVTK structured extents are reversed
      // Where I varies fastest, SVTK's convention is IJK, but XDMF's is KJI
      // However, users naturally don't want VXVY to mean VZVY.
      // Let's accept VisIt's interpretation of this 2D case
      // (KJI is ZXY where Z=0).
      xarray->SetNumberOfTuples(scaled_dims[1]);
      yarray->SetNumberOfTuples(scaled_dims[2]);
      zarray->SetNumberOfTuples(scaled_dims[0]);
      rg->SetExtent(scaled_extents[2], scaled_extents[3], scaled_extents[4], scaled_extents[5],
        scaled_extents[0], scaled_extents[1]);
      xmfGeometry->GetVectorX()->GetValues(
        update_extents[2], xarray->GetPointer(0), scaled_dims[1], this->Stride[1]);
      xmfGeometry->GetVectorY()->GetValues(
        update_extents[4], yarray->GetPointer(0), scaled_dims[2], this->Stride[2]);
      zarray->FillComponent(0, 0);
    }
    break;

    case XDMF_GEOMETRY_VXVYVZ:
    {
      xmfGeometry->GetVectorX()->GetValues(
        update_extents[0], xarray->GetPointer(0), scaled_dims[0], this->Stride[0]);
      xmfGeometry->GetVectorY()->GetValues(
        update_extents[2], yarray->GetPointer(0), scaled_dims[1], this->Stride[1]);
      xmfGeometry->GetVectorZ()->GetValues(
        update_extents[4], zarray->GetPointer(0), scaled_dims[2], this->Stride[2]);
    }
    break;

    default:
      svtkErrorWithObjectMacro(this->Reader,
        "Geometry type : " << xmfGeometry->GetGeometryTypeAsString() << " is not supported for "
                           << xmfGrid->GetTopology()->GetTopologyTypeAsString());
      return nullptr;
  }

  this->ReadAttributes(rg, xmfGrid, update_extents);
  rg->Register(nullptr);
  return rg;
}

//-----------------------------------------------------------------------------
svtkStructuredGrid* svtkXdmfHeavyData::RequestStructuredGrid(XdmfGrid* xmfGrid)
{
  svtkStructuredGrid* sg = svtkStructuredGrid::New();

  int whole_extents[6];
  int update_extents[6];
  this->Domain->GetWholeExtent(xmfGrid, whole_extents);

  if (!svtkExtentsAreValid(this->Extents))
  {
    // if this->Extents are not valid, then simply read the whole image.
    memcpy(update_extents, whole_extents, sizeof(int) * 6);
  }
  else
  {
    memcpy(update_extents, this->Extents, sizeof(int) * 6);
  }

  int scaled_extents[6];
  svtkScaleExtents(update_extents, scaled_extents, this->Stride);
  sg->SetExtent(scaled_extents);

  svtkPoints* points = this->ReadPoints(xmfGrid->GetGeometry(), update_extents, whole_extents);
  sg->SetPoints(points);
  points->Delete();

  this->ReadAttributes(sg, xmfGrid, update_extents);
  return sg;
}

//-----------------------------------------------------------------------------
svtkImageData* svtkXdmfHeavyData::RequestImageData(XdmfGrid* xmfGrid, bool use_uniform_grid)
{
  svtkImageData* imageData =
    use_uniform_grid ? static_cast<svtkImageData*>(svtkUniformGrid::New()) : svtkImageData::New();

  int whole_extents[6];
  this->Domain->GetWholeExtent(xmfGrid, whole_extents);

  int update_extents[6];

  if (!svtkExtentsAreValid(this->Extents))
  {
    // if this->Extents are not valid, then simply read the whole image.
    memcpy(update_extents, whole_extents, sizeof(int) * 6);
  }
  else
  {
    memcpy(update_extents, this->Extents, sizeof(int) * 6);
  }

  int scaled_extents[6];
  svtkScaleExtents(update_extents, scaled_extents, this->Stride);
  imageData->SetExtent(scaled_extents);

  double origin[3], spacing[3];
  if (!this->Domain->GetOriginAndSpacing(xmfGrid, origin, spacing))
  {
    svtkErrorWithObjectMacro(this->Reader,
      "Could not determine image-data origin and spacing. "
      "Required geometry type is ORIGIN_DXDY or ORIGIN_DXDYDZ. "
      "The specified geometry type is : "
        << xmfGrid->GetGeometry()->GetGeometryTypeAsString());
    // release image data.
    imageData->Delete();
    return nullptr;
  }
  imageData->SetOrigin(origin);
  imageData->SetSpacing(
    spacing[0] * this->Stride[0], spacing[1] * this->Stride[1], spacing[2] * this->Stride[2]);
  this->ReadAttributes(imageData, xmfGrid, update_extents);
  return imageData;
}

//-----------------------------------------------------------------------------
svtkPoints* svtkXdmfHeavyData::ReadPoints(
  XdmfGeometry* xmfGeometry, int* update_extents /*=nullptr*/, int* whole_extents /*=nullptr*/)
{
  XdmfInt32 geomType = xmfGeometry->GetGeometryType();

  if (geomType != XDMF_GEOMETRY_X_Y_Z && geomType != XDMF_GEOMETRY_XYZ &&
    geomType != XDMF_GEOMETRY_X_Y && geomType != XDMF_GEOMETRY_XY)
  {
    return nullptr;
  }

  XdmfArray* xmfPoints = xmfGeometry->GetPoints();
  if (!xmfPoints)
  {
    XdmfErrorMessage("No Points to Set");
    return nullptr;
  }

  svtkSmartPointer<svtkPoints> points = svtkSmartPointer<svtkPoints>::New();

  if (xmfPoints->GetNumberType() == XDMF_FLOAT32_TYPE)
  {
    svtkFloatArray* da = svtkFloatArray::New();
    da->SetNumberOfComponents(3);
    points->SetData(da);
    da->Delete();
  }
  else // means == XDMF_FLOAT64_TYPE
  {
    svtkDoubleArray* da = svtkDoubleArray::New();
    da->SetNumberOfComponents(3);
    points->SetData(da);
    da->Delete();
  }

  XdmfInt64 numGeometryPoints = xmfGeometry->GetNumberOfPoints();
  svtkIdType numPoints = numGeometryPoints;
  bool structured_data = false;
  if (update_extents && whole_extents)
  {
    // we are reading a sub-extent.
    structured_data = true;
    int scaled_extents[6];
    int scaled_dims[3];
    svtkScaleExtents(update_extents, scaled_extents, this->Stride);
    svtkGetDims(scaled_extents, scaled_dims);
    numPoints = (scaled_dims[0] * scaled_dims[1] * scaled_dims[2]);
  }
  points->SetNumberOfPoints(numPoints);

  if (!structured_data)
  {
    // read all the points.
    switch (points->GetData()->GetDataType())
    {
      case SVTK_DOUBLE:
        xmfPoints->GetValues(
          0, reinterpret_cast<double*>(points->GetVoidPointer(0)), numPoints * 3);
        break;

      case SVTK_FLOAT:
        xmfPoints->GetValues(0, reinterpret_cast<float*>(points->GetVoidPointer(0)), numPoints * 3);
        break;

      default:
        return nullptr;
    }
  }
  else
  {
    // treating the points as structured points
    std::vector<XdmfFloat64> tempPoints(numGeometryPoints * 3);
    xmfPoints->GetValues(0, tempPoints.data(), numGeometryPoints * 3);
    svtkIdType pointId = 0;
    int xdmf_dims[3];
    svtkGetDims(whole_extents, xdmf_dims);

    for (int z = update_extents[4]; z <= update_extents[5]; z++)
    {
      if ((z - update_extents[4]) % this->Stride[2])
      {
        continue;
      }

      for (int y = update_extents[2]; y <= update_extents[3]; y++)
      {
        if ((y - update_extents[2]) % this->Stride[1])
        {
          continue;
        }

        for (int x = update_extents[0]; x <= update_extents[1]; x++)
        {
          if ((x - update_extents[0]) % this->Stride[0])
          {
            continue;
          }

          int xdmf_index[3] = { x, y, z };
          XdmfInt64 offset = svtkStructuredData::ComputePointId(xdmf_dims, xdmf_index);
          points->SetPoint(pointId, tempPoints[3 * offset], tempPoints[3 * offset + 1],
            tempPoints[3 * offset + 2]);
          pointId++;
        }
      }
    }
  }

  points->Register(0);
  return points;
}

//-----------------------------------------------------------------------------
bool svtkXdmfHeavyData::ReadAttributes(svtkDataSet* dataSet, XdmfGrid* xmfGrid, int* update_extents)
{
  int data_dimensionality = this->Domain->GetDataDimensionality(xmfGrid);

  int numAttributes = xmfGrid->GetNumberOfAttributes();
  for (int cc = 0; cc < numAttributes; cc++)
  {
    XdmfAttribute* xmfAttribute = xmfGrid->GetAttribute(cc);
    const char* attrName = xmfAttribute->GetName();
    int attrCenter = xmfAttribute->GetAttributeCenter();
    if (!attrName)
    {
      svtkWarningWithObjectMacro(this->Reader, "Skipping unnamed attributes.");
      continue;
    }

    svtkFieldData* fieldData = 0;
    // skip disabled arrays.
    switch (attrCenter)
    {
      case XDMF_ATTRIBUTE_CENTER_GRID:
        fieldData = dataSet->GetFieldData();
        break;

      case XDMF_ATTRIBUTE_CENTER_CELL:
        if (!this->Domain->GetCellArraySelection()->ArrayIsEnabled(attrName))
        {
          continue;
        }
        fieldData = dataSet->GetCellData();
        break;

      case XDMF_ATTRIBUTE_CENTER_NODE:
        if (!this->Domain->GetPointArraySelection()->ArrayIsEnabled(attrName))
        {
          continue;
        }
        fieldData = dataSet->GetPointData();
        break;

      case XDMF_ATTRIBUTE_CENTER_FACE:
      case XDMF_ATTRIBUTE_CENTER_EDGE:
      default:
        svtkWarningWithObjectMacro(this->Reader,
          "Skipping attribute " << attrName << " at "
                                << xmfAttribute->GetAttributeCenterAsString());
        continue; // unhandled.
    }

    svtkDataArray* array = this->ReadAttribute(xmfAttribute, data_dimensionality, update_extents);
    if (array)
    {
      array->SetName(attrName);
      fieldData->AddArray(array);
      bool is_active = xmfAttribute->GetActive() != 0;
      svtkDataSetAttributes* attributes = svtkDataSetAttributes::SafeDownCast(fieldData);
      if (attributes)
      {
        // make attribute active.
        switch (xmfAttribute->GetAttributeType())
        {
          case XDMF_ATTRIBUTE_TYPE_SCALAR:
            if (is_active || attributes->GetScalars() == nullptr)
            {
              attributes->SetActiveScalars(attrName);
            }
            break;

          case XDMF_ATTRIBUTE_TYPE_VECTOR:
            if (is_active || attributes->GetVectors() == nullptr)
            {
              attributes->SetActiveVectors(attrName);
            }
            break;

          case XDMF_ATTRIBUTE_TYPE_TENSOR:
          case XDMF_ATTRIBUTE_TYPE_TENSOR6:
            if (is_active || attributes->GetTensors() == nullptr)
            {
              attributes->SetActiveTensors(attrName);
            }
            break;

          case XDMF_ATTRIBUTE_TYPE_GLOBALID:
            if (is_active || attributes->GetGlobalIds() == nullptr)
            {
              attributes->SetActiveGlobalIds(attrName);
            }
        }
      }
      array->Delete();
    }
  }
  return true;
}

// used to convert a symmetric tensor to a regular tensor.
template <class T>
void svtkConvertTensor6(T* source, T* dest, svtkIdType numTensors)
{
  for (svtkIdType cc = 0; cc < numTensors; cc++)
  {
    dest[cc * 9 + 0] = source[cc * 6 + 0];
    dest[cc * 9 + 1] = source[cc * 6 + 1];
    dest[cc * 9 + 2] = source[cc * 6 + 2];

    dest[cc * 9 + 3] = source[cc * 6 + 1];
    dest[cc * 9 + 4] = source[cc * 6 + 3];
    dest[cc * 9 + 5] = source[cc * 6 + 4];

    dest[cc * 9 + 6] = source[cc * 6 + 2];
    dest[cc * 9 + 7] = source[cc * 6 + 4];
    dest[cc * 9 + 8] = source[cc * 6 + 5];
  }
}

//-----------------------------------------------------------------------------
svtkDataArray* svtkXdmfHeavyData::ReadAttribute(
  XdmfAttribute* xmfAttribute, int data_dimensionality, int* update_extents /*=0*/)
{
  if (!xmfAttribute)
  {
    return nullptr;
  }

  int attrType = xmfAttribute->GetAttributeType();
  int attrCenter = xmfAttribute->GetAttributeCenter();
  int numComponents = 1;

  XdmfDataItem xmfDataItem;
  xmfDataItem.SetDOM(xmfAttribute->GetDOM());
  xmfDataItem.SetElement(xmfAttribute->GetDOM()->FindDataElement(0, xmfAttribute->GetElement()));
  xmfDataItem.UpdateInformation();

  XdmfInt64 data_dims[XDMF_MAX_DIMENSION];
  int data_rank = xmfDataItem.GetDataDesc()->GetShape(data_dims);

  switch (attrType)
  {
    case XDMF_ATTRIBUTE_TYPE_TENSOR:
      numComponents = 9;
      break;
    case XDMF_ATTRIBUTE_TYPE_TENSOR6:
      numComponents = 6;
      break;
    case XDMF_ATTRIBUTE_TYPE_VECTOR:
      numComponents = 3;
      break;
    default:
      numComponents = 1;
      break;
  }

  // Handle 2D vectors
  if (attrType == XDMF_ATTRIBUTE_TYPE_VECTOR && data_dims[data_rank - 1] == 2)
  {
    numComponents = 2;
  }

  if (update_extents && attrCenter != XDMF_ATTRIBUTE_CENTER_GRID)
  {
    // for hyperslab selection to work, the data shape must match the topology
    // shape.
    if (data_rank < 0)
    {
      svtkErrorWithObjectMacro(this->Reader, "Unsupported attribute rank: " << data_rank);
      return nullptr;
    }
    if (data_rank > (data_dimensionality + 1))
    {
      svtkErrorWithObjectMacro(
        this->Reader, "The data_dimensionality and topology dimensionality mismatch");
      return nullptr;
    }
    XdmfInt64 start[4] = { update_extents[4], update_extents[2], update_extents[0], 0 };
    XdmfInt64 stride[4] = { this->Stride[2], this->Stride[1], this->Stride[0], 1 };
    XdmfInt64 count[4] = { 0, 0, 0, 0 };
    int scaled_dims[3];
    int scaled_extents[6];
    svtkScaleExtents(update_extents, scaled_extents, this->Stride);
    svtkGetDims(scaled_extents, scaled_dims);
    count[0] = (scaled_dims[2] - 1);
    count[1] = (scaled_dims[1] - 1);
    count[2] = (scaled_dims[0] - 1);
    if (data_rank == (data_dimensionality + 1))
    {
      // this refers the number of components in the attribute.
      count[data_dimensionality] = data_dims[data_dimensionality];
    }

    if (attrCenter == XDMF_ATTRIBUTE_CENTER_NODE)
    {
      // Point count is 1 + cell extent if not a single layer
      count[0] += 1; //((update_extents[5] - update_extents[4]) > 0)? 1 : 0;
      count[1] += 1; //((update_extents[3] - update_extents[2]) > 0)? 1 : 0;
      count[2] += 1; //((update_extents[1] - update_extents[0]) > 0)? 1 : 0;
    }
    xmfDataItem.GetDataDesc()->SelectHyperSlab(start, stride, count);
  }

  if (xmfDataItem.Update() == XDMF_FAIL)
  {
    svtkErrorWithObjectMacro(this->Reader, "Failed to read attribute data");
    return 0;
  }

  svtkXdmfDataArray* xmfConvertor = svtkXdmfDataArray::New();
  svtkDataArray* dataArray = xmfConvertor->FromXdmfArray(
    xmfDataItem.GetArray()->GetTagName(), 1, data_rank, numComponents, 0);
  xmfConvertor->Delete();

  if (attrType == XDMF_ATTRIBUTE_TYPE_TENSOR6)
  {
    // convert Tensor6 to Tensor.
    svtkDataArray* tensor = dataArray->NewInstance();
    svtkIdType numTensors = dataArray->GetNumberOfTuples();
    tensor->SetNumberOfComponents(9);
    tensor->SetNumberOfTuples(numTensors);

    // Copy Symmetrical Tensor Values to Correct Positions in 3x3 matrix
    void* source = dataArray->GetVoidPointer(0);
    void* dest = tensor->GetVoidPointer(0);
    switch (tensor->GetDataType())
    {
      svtkTemplateMacro(svtkConvertTensor6(
        reinterpret_cast<SVTK_TT*>(source), reinterpret_cast<SVTK_TT*>(dest), numTensors));
    }
    dataArray->Delete();
    return tensor;
  }

  if (attrType == XDMF_ATTRIBUTE_TYPE_VECTOR && numComponents == 2)
  {
    // convert 2D vectors to 3-tuple vectors with 0.0 in the z component
    svtkDataArray* vector3D = dataArray->NewInstance();
    svtkIdType numVectors = dataArray->GetNumberOfTuples();
    vector3D->SetNumberOfComponents(3);
    vector3D->SetNumberOfTuples(numVectors);

    // Add 0.0 to third component of vector
    auto inputRange = svtk::DataArrayTupleRange<2>(dataArray);
    auto outputRange = svtk::DataArrayTupleRange<3>(vector3D);
    for (auto i = 0; i < inputRange.size(); ++i)
    {
      outputRange[i][0] = inputRange[i][0];
      outputRange[i][1] = inputRange[i][1];
      outputRange[i][2] = 0.0;
    }
    dataArray->Delete();
    return vector3D;
  }

  return dataArray;
}

//-----------------------------------------------------------------------------
// Read ghost cell/point information. This is simply loaded info a
// svtkGhostType attribute array.
bool svtkXdmfHeavyData::ReadGhostSets(
  svtkDataSet* dataSet, XdmfGrid* xmfGrid, int* svtkNotUsed(update_extents) /*=0*/)
{
  // int data_dimensionality = this->Domain->GetDataDimensionality(xmfGrid);
  for (int cc = 0; cc < xmfGrid->GetNumberOfSets(); cc++)
  {
    XdmfSet* xmfSet = xmfGrid->GetSets(cc);
    int ghost_value = xmfSet->GetGhost();
    if (ghost_value <= 0)
    {
      // not a ghost-set, simply continue.
      continue;
    }
    XdmfInt32 setCenter = xmfSet->GetSetType();
    svtkIdType numElems = 0;
    svtkDataSetAttributes* dsa = 0;
    unsigned char ghostFlag = 0;
    switch (setCenter)
    {
      case XDMF_SET_TYPE_NODE:
        dsa = dataSet->GetPointData();
        numElems = dataSet->GetNumberOfPoints();
        ghostFlag = svtkDataSetAttributes::DUPLICATEPOINT;
        break;

      case XDMF_SET_TYPE_CELL:
        dsa = dataSet->GetCellData();
        numElems = dataSet->GetNumberOfCells();
        ghostFlag = svtkDataSetAttributes::DUPLICATECELL;
        break;

      default:
        svtkWarningWithObjectMacro(
          this->Reader, "Only ghost-cells and ghost-nodes are currently supported.");
        continue;
    }

    svtkUnsignedCharArray* ghosts =
      svtkArrayDownCast<svtkUnsignedCharArray>(dsa->GetArray(svtkDataSetAttributes::GhostArrayName()));
    if (!ghosts)
    {
      ghosts = svtkUnsignedCharArray::New();
      ghosts->SetName(svtkDataSetAttributes::GhostArrayName());
      ghosts->SetNumberOfComponents(1);
      ghosts->SetNumberOfTuples(numElems);
      ghosts->FillComponent(0, 0);
      dsa->AddArray(ghosts);
      ghosts->Delete();
    }

    unsigned char* ptrGhosts = ghosts->GetPointer(0);

    // Read heavy data. We cannot do anything smart if update_extents or stride
    // is specified here. We have to read the entire set and then prune it.
    xmfSet->Update();

    XdmfArray* xmfIds = xmfSet->GetIds();
    XdmfInt64 numIds = xmfIds->GetNumberOfElements();
    std::vector<XdmfInt64> ids(numIds + 1);
    xmfIds->GetValues(0, ids.data(), numIds);

    // release the heavy data that was read.
    xmfSet->Release();

    for (svtkIdType kk = 0; kk < numIds; kk++)
    {
      if (ids[kk] < 0 || ids[kk] > numElems)
      {
        svtkWarningWithObjectMacro(this->Reader, "No such cell or point exists: " << ids[kk]);
        continue;
      }
      ptrGhosts[ids[kk]] = ghostFlag;
    }
  }
  return true;
}

//-----------------------------------------------------------------------------
svtkMultiBlockDataSet* svtkXdmfHeavyData::ReadSets(
  svtkDataSet* dataSet, XdmfGrid* xmfGrid, int* svtkNotUsed(update_extents) /*=0*/)
{
  unsigned int number_of_sets = 0;
  for (int cc = 0; cc < xmfGrid->GetNumberOfSets(); cc++)
  {
    XdmfSet* xmfSet = xmfGrid->GetSets(cc);
    int ghost_value = xmfSet->GetGhost();
    if (ghost_value != 0)
    {
      // skip ghost-sets.
      continue;
    }
    number_of_sets++;
  }
  if (number_of_sets == 0)
  {
    return nullptr;
  }

  svtkMultiBlockDataSet* mb = svtkMultiBlockDataSet::New();
  mb->SetNumberOfBlocks(1 + number_of_sets);
  mb->SetBlock(0, dataSet);
  mb->GetMetaData(static_cast<unsigned int>(0))->Set(svtkCompositeDataSet::NAME(), "Data");

  unsigned int current_set_index = 1;
  for (int cc = 0; cc < xmfGrid->GetNumberOfSets(); cc++)
  {
    XdmfSet* xmfSet = xmfGrid->GetSets(cc);
    int ghost_value = xmfSet->GetGhost();
    if (ghost_value != 0)
    {
      // skip ghost-sets.
      continue;
    }

    const char* setName = xmfSet->GetName();
    mb->GetMetaData(current_set_index)->Set(svtkCompositeDataSet::NAME(), setName);
    if (!this->Domain->GetSetsSelection()->ArrayIsEnabled(setName))
    {
      continue;
    }

    // Okay now we have an enabled set. Create a new dataset for it
    svtkDataSet* set = 0;

    XdmfInt32 setType = xmfSet->GetSetType();
    switch (setType)
    {
      case XDMF_SET_TYPE_NODE:
        set = this->ExtractPoints(xmfSet, dataSet);
        break;

      case XDMF_SET_TYPE_CELL:
        set = this->ExtractCells(xmfSet, dataSet);
        break;

      case XDMF_SET_TYPE_FACE:
        set = this->ExtractFaces(xmfSet, dataSet);
        break;

      case XDMF_SET_TYPE_EDGE:
        set = this->ExtractEdges(xmfSet, dataSet);
        break;
    }

    if (set)
    {
      mb->SetBlock(current_set_index, set);
      set->Delete();
    }
    current_set_index++;
  }
  return mb;
}

//-----------------------------------------------------------------------------
svtkDataSet* svtkXdmfHeavyData::ExtractPoints(XdmfSet* xmfSet, svtkDataSet* dataSet)
{
  // TODO: How to handle structured datasets with update_extents/strides etc.
  // Do they too always produce svtkUniformGrid or do we want to produce
  // structured dataset

  // Read heavy data. We cannot do anything smart if update_extents or stride
  // is specified here. We have to read the entire set and then prune it.
  xmfSet->Update();

  XdmfArray* xmfIds = xmfSet->GetIds();
  XdmfInt64 numIds = xmfIds->GetNumberOfElements();
  std::vector<XdmfInt64> ids(numIds + 1);
  xmfIds->GetValues(0, ids.data(), numIds);

  // release heavy data.
  xmfSet->Release();

  svtkUnstructuredGrid* output = svtkUnstructuredGrid::New();
  svtkPoints* outputPoints = svtkPoints::New();
  outputPoints->SetNumberOfPoints(numIds);
  output->SetPoints(outputPoints);
  outputPoints->Delete();

  svtkIdType numInPoints = dataSet->GetNumberOfPoints();
  for (svtkIdType kk = 0; kk < numIds; kk++)
  {
    if (ids[kk] < 0 || ids[kk] > numInPoints)
    {
      svtkWarningWithObjectMacro(this->Reader, "No such cell or point exists: " << ids[kk]);
      continue;
    }
    double point_location[3];
    dataSet->GetPoint(ids[kk], point_location);
    outputPoints->SetPoint(kk, point_location);
  }
  ids.clear(); // done with ids

  // Read node-centered attributes that may be defined on this set.
  int numAttributes = xmfSet->GetNumberOfAttributes();
  for (int cc = 0; cc < numAttributes; cc++)
  {
    XdmfAttribute* xmfAttribute = xmfSet->GetAttribute(cc);
    const char* attrName = xmfAttribute->GetName();
    int attrCenter = xmfAttribute->GetAttributeCenter();
    if (attrCenter != XDMF_ATTRIBUTE_CENTER_NODE)
    {
      continue;
    }
    svtkDataArray* array = this->ReadAttribute(xmfAttribute, 1, nullptr);
    if (array)
    {
      array->SetName(attrName);
      output->GetPointData()->AddArray(array);
      array->Delete();
    }
  }

  std::vector<svtkIdType> svtk_cell_ids(numIds);
  std::iota(svtk_cell_ids.begin(), svtk_cell_ids.end(), 0);
  output->InsertNextCell(SVTK_POLY_VERTEX, numIds, svtk_cell_ids.data());

  return output;
}

//-----------------------------------------------------------------------------
svtkDataSet* svtkXdmfHeavyData::ExtractCells(XdmfSet* xmfSet, svtkDataSet* dataSet)
{
  // TODO: How to handle structured datasets with update_extents/strides etc.
  // Do they too always produce svtkUniformGrid or do we want to produce
  // structured dataset

  // Read heavy data.
  xmfSet->Update();

  XdmfArray* xmfIds = xmfSet->GetIds();
  XdmfInt64 numIds = xmfIds->GetNumberOfElements();

  svtkIdTypeArray* ids = svtkIdTypeArray::New();
  ids->SetNumberOfComponents(1);
  ids->SetNumberOfTuples(numIds);
  xmfIds->GetValues(0, (svtkXdmfIdType*)ids->GetPointer(0), numIds);

  // release heavy data.
  xmfSet->Release();

  // We directly use svtkExtractSelectedIds for extract cells since the logic to
  // extract cells it no trivial (like extracting points).
  svtkSelectionNode* selNode = svtkSelectionNode::New();
  selNode->SetContentType(svtkSelectionNode::INDICES);
  selNode->SetFieldType(svtkSelectionNode::CELL);
  selNode->SetSelectionList(ids);

  svtkSelection* sel = svtkSelection::New();
  sel->AddNode(selNode);
  selNode->Delete();

  svtkExtractSelectedIds* extractCells = svtkExtractSelectedIds::New();
  extractCells->SetInputData(0, dataSet);
  extractCells->SetInputData(1, sel);
  extractCells->Update();

  svtkDataSet* output = svtkDataSet::SafeDownCast(extractCells->GetOutput()->NewInstance());
  output->CopyStructure(svtkDataSet::SafeDownCast(extractCells->GetOutput()));

  sel->Delete();
  extractCells->Delete();
  ids->Delete();

  // Read cell-centered attributes that may be defined on this set.
  int numAttributes = xmfSet->GetNumberOfAttributes();
  for (int cc = 0; cc < numAttributes; cc++)
  {
    XdmfAttribute* xmfAttribute = xmfSet->GetAttribute(cc);
    const char* attrName = xmfAttribute->GetName();
    int attrCenter = xmfAttribute->GetAttributeCenter();
    if (attrCenter != XDMF_ATTRIBUTE_CENTER_CELL)
    {
      continue;
    }
    svtkDataArray* array = this->ReadAttribute(xmfAttribute, 1, nullptr);
    if (array)
    {
      array->SetName(attrName);
      output->GetCellData()->AddArray(array);
      array->Delete();
    }
  }

  return output;
}

//-----------------------------------------------------------------------------
svtkDataSet* svtkXdmfHeavyData::ExtractFaces(XdmfSet* xmfSet, svtkDataSet* dataSet)
{
  xmfSet->Update();

  XdmfArray* xmfIds = xmfSet->GetIds();
  XdmfArray* xmfCellIds = xmfSet->GetCellIds();

  XdmfInt64 numFaces = xmfIds->GetNumberOfElements();

  // ids is a 2 component array were each tuple is (cell-id, face-id).
  svtkIdTypeArray* ids = svtkIdTypeArray::New();
  ids->SetNumberOfComponents(2);
  ids->SetNumberOfTuples(numFaces);
  xmfCellIds->GetValues(0, (svtkXdmfIdType*)ids->GetPointer(0), numFaces, 1, 2);
  xmfIds->GetValues(0, (svtkXdmfIdType*)ids->GetPointer(1), numFaces, 1, 2);

  svtkPolyData* output = svtkPolyData::New();
  svtkCellArray* polys = svtkCellArray::New();
  output->SetPolys(polys);
  polys->Delete();

  svtkPoints* outPoints = svtkPoints::New();
  output->SetPoints(outPoints);
  outPoints->Delete();

  svtkMergePoints* mergePoints = svtkMergePoints::New();
  mergePoints->InitPointInsertion(outPoints, dataSet->GetBounds());

  for (svtkIdType cc = 0; cc < numFaces; cc++)
  {
    svtkIdType cellId = ids->GetValue(cc * 2);
    svtkIdType faceId = ids->GetValue(cc * 2 + 1);
    svtkCell* cell = dataSet->GetCell(cellId);
    if (!cell)
    {
      svtkWarningWithObjectMacro(this->Reader, "Invalid cellId: " << cellId);
      continue;
    }
    svtkCell* face = cell->GetFace(faceId);
    if (!face)
    {
      svtkWarningWithObjectMacro(this->Reader, "Invalid faceId " << faceId << " on cell " << cellId);
      continue;
    }

    // Now insert this face a new cell in the output dataset.
    svtkIdType numPoints = face->GetNumberOfPoints();
    svtkPoints* facePoints = face->GetPoints();
    std::vector<svtkIdType> outputPts(numPoints + 1);
    for (svtkIdType kk = 0; kk < numPoints; kk++)
    {
      mergePoints->InsertUniquePoint(facePoints->GetPoint(kk), outputPts[kk]);
    }
    polys->InsertNextCell(numPoints, outputPts.data());
  }

  ids->Delete();
  xmfSet->Release();
  mergePoints->Delete();

  // Read face-centered attributes that may be defined on this set.
  int numAttributes = xmfSet->GetNumberOfAttributes();
  for (int cc = 0; cc < numAttributes; cc++)
  {
    XdmfAttribute* xmfAttribute = xmfSet->GetAttribute(cc);
    const char* attrName = xmfAttribute->GetName();
    int attrCenter = xmfAttribute->GetAttributeCenter();
    if (attrCenter != XDMF_ATTRIBUTE_CENTER_FACE)
    {
      continue;
    }
    svtkDataArray* array = this->ReadAttribute(xmfAttribute, 1, nullptr);
    if (array)
    {
      array->SetName(attrName);
      output->GetCellData()->AddArray(array);
      array->Delete();
    }
  }

  return output;
}

//-----------------------------------------------------------------------------
svtkDataSet* svtkXdmfHeavyData::ExtractEdges(XdmfSet* xmfSet, svtkDataSet* dataSet)
{
  xmfSet->Update();

  XdmfArray* xmfIds = xmfSet->GetIds();
  XdmfArray* xmfCellIds = xmfSet->GetCellIds();
  XdmfArray* xmfFaceIds = xmfSet->GetFaceIds();

  XdmfInt64 numEdges = xmfIds->GetNumberOfElements();

  // ids is a 3 component array were each tuple is (cell-id, face-id, edge-id).
  svtkIdTypeArray* ids = svtkIdTypeArray::New();
  ids->SetNumberOfComponents(3);
  ids->SetNumberOfTuples(numEdges);
  xmfCellIds->GetValues(0, (svtkXdmfIdType*)ids->GetPointer(0), numEdges, 1, 3);
  xmfFaceIds->GetValues(0, (svtkXdmfIdType*)ids->GetPointer(1), numEdges, 1, 3);
  xmfIds->GetValues(0, (svtkXdmfIdType*)ids->GetPointer(2), numEdges, 1, 3);

  svtkPolyData* output = svtkPolyData::New();
  svtkCellArray* lines = svtkCellArray::New();
  output->SetLines(lines);
  lines->Delete();

  svtkPoints* outPoints = svtkPoints::New();
  output->SetPoints(outPoints);
  outPoints->Delete();

  svtkMergePoints* mergePoints = svtkMergePoints::New();
  mergePoints->InitPointInsertion(outPoints, dataSet->GetBounds());

  for (svtkIdType cc = 0; cc < numEdges; cc++)
  {
    svtkIdType cellId = ids->GetValue(cc * 3);
    svtkIdType faceId = ids->GetValue(cc * 3 + 1);
    svtkIdType edgeId = ids->GetValue(cc * 3 + 2);
    svtkCell* cell = dataSet->GetCell(cellId);
    if (!cell)
    {
      svtkWarningWithObjectMacro(this->Reader, "Invalid cellId: " << cellId);
      continue;
    }
    svtkCell* face = cell->GetFace(faceId);
    if (!face)
    {
      svtkWarningWithObjectMacro(this->Reader, "Invalid faceId " << faceId << " on cell " << cellId);
      continue;
    }
    svtkCell* edge = cell->GetEdge(edgeId);
    if (!edge)
    {
      svtkWarningWithObjectMacro(this->Reader,
        "Invalid edgeId " << edgeId << " on face " << faceId << " on cell " << cellId);
      continue;
    }

    // Now insert this edge as a new cell in the output dataset.
    svtkIdType numPoints = edge->GetNumberOfPoints();
    svtkPoints* edgePoints = edge->GetPoints();
    std::vector<svtkIdType> outputPts(numPoints + 1);
    for (svtkIdType kk = 0; kk < numPoints; kk++)
    {
      mergePoints->InsertUniquePoint(edgePoints->GetPoint(kk), outputPts[kk]);
    }
    lines->InsertNextCell(numPoints, outputPts.data());
  }

  ids->Delete();
  xmfSet->Release();
  mergePoints->Delete();

  // Read edge-centered attributes that may be defined on this set.
  int numAttributes = xmfSet->GetNumberOfAttributes();
  for (int cc = 0; cc < numAttributes; cc++)
  {
    XdmfAttribute* xmfAttribute = xmfSet->GetAttribute(cc);
    const char* attrName = xmfAttribute->GetName();
    int attrCenter = xmfAttribute->GetAttributeCenter();
    if (attrCenter != XDMF_ATTRIBUTE_CENTER_EDGE)
    {
      continue;
    }
    svtkDataArray* array = this->ReadAttribute(xmfAttribute, 1, nullptr);
    if (array)
    {
      array->SetName(attrName);
      output->GetCellData()->AddArray(array);
      array->Delete();
    }
  }

  return output;
}
