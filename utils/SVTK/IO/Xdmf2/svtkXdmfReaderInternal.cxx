/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmfReaderInternal.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkXdmfReaderInternal.h"

#include "svtkDataArray.h"
#include "svtkSmartPointer.h"
#include "svtkVariant.h"
#include "svtkXdmfDataArray.h"

#define USE_IMAGE_DATA // otherwise uniformgrid

// As soon as num-grids (sub-grids and all) grows beyond this number, we assume
// that the grids are way too numerous for the user to select individually and
// hence only the top-level grids are made accessible.
#define MAX_COLLECTABLE_NUMBER_OF_GRIDS 1000

template <class T>
T svtkMAX(T a, T b)
{
  return (a > b ? a : b);
}

using namespace xdmf2;

//----------------------------------------------------------------------------
svtkXdmfDocument::svtkXdmfDocument()
{
  this->ActiveDomain = 0;
  this->ActiveDomainIndex = -1;
  this->LastReadContents = 0;
  this->LastReadContentsLength = 0;
}

//----------------------------------------------------------------------------
svtkXdmfDocument::~svtkXdmfDocument()
{
  delete this->ActiveDomain;
  delete[] this->LastReadContents;
}

//----------------------------------------------------------------------------
bool svtkXdmfDocument::Parse(const char* xmffilename)
{
  if (!xmffilename)
  {
    return false;
  }

  if (this->LastReadFilename == xmffilename)
  {
    return true;
  }

  this->ActiveDomainIndex = -1;
  delete this->ActiveDomain;
  this->ActiveDomain = 0;

  delete[] this->LastReadContents;
  this->LastReadContents = 0;
  this->LastReadContentsLength = 0;
  this->LastReadFilename = std::string();

  this->XMLDOM.SetInputFileName(xmffilename);
  if (!this->XMLDOM.Parse())
  {
    return false;
  }

  // Tell the parser what the working directory is.
  std::string directory = svtksys::SystemTools::GetFilenamePath(xmffilename) + "/";
  if (directory == "/")
  {
    directory = svtksys::SystemTools::GetCurrentWorkingDirectory() + "/";
  }
  this->XMLDOM.SetWorkingDirectory(directory.c_str());

  this->LastReadFilename = xmffilename;
  this->UpdateDomains();
  return true;
}

//----------------------------------------------------------------------------
bool svtkXdmfDocument::ParseString(const char* xmfdata, size_t length)
{
  if (!xmfdata || !length)
  {
    return false;
  }

  if (this->LastReadContents && this->LastReadContentsLength == length &&
    STRNCASECMP(xmfdata, this->LastReadContents, length) == 0)
  {
    return true;
  }

  this->ActiveDomainIndex = -1;
  delete this->ActiveDomain;
  this->ActiveDomain = 0;

  delete this->LastReadContents;
  this->LastReadContentsLength = 0;
  this->LastReadFilename = std::string();

  this->LastReadContents = new char[length + 1];
  this->LastReadContentsLength = length;

  memcpy(this->LastReadContents, xmfdata, length);
  this->LastReadContents[length] = 0;

  this->XMLDOM.SetInputFileName(0);
  if (!this->XMLDOM.Parse(this->LastReadContents))
  {
    delete this->LastReadContents;
    this->LastReadContents = 0;
    this->LastReadContentsLength = 0;
    return false;
  }

  this->UpdateDomains();
  return true;
}

//----------------------------------------------------------------------------
void svtkXdmfDocument::UpdateDomains()
{
  this->Domains.clear();
  XdmfXmlNode domain = this->XMLDOM.FindElement("Domain", 0);
  while (domain)
  {
    XdmfConstString domainName = this->XMLDOM.Get(domain, "Name");
    if (domainName)
    {
      this->Domains.push_back(domainName);
    }
    else
    {
      std::ostringstream str;
      str << "Domain" << this->Domains.size() << ends;
      this->Domains.push_back(str.str());
    }
    domain = this->XMLDOM.FindNextElement("Domain", domain);
  }
}

//----------------------------------------------------------------------------
bool svtkXdmfDocument::SetActiveDomain(const char* domainname)
{
  for (int cc = 0; cc < static_cast<int>(this->Domains.size()); cc++)
  {
    if (this->Domains[cc] == domainname)
    {
      return this->SetActiveDomain(cc);
    }
  }
  return false;
}

//----------------------------------------------------------------------------
bool svtkXdmfDocument::SetActiveDomain(int index)
{
  if (this->ActiveDomainIndex == index)
  {
    return true;
  }

  this->ActiveDomainIndex = -1;
  delete this->ActiveDomain;
  this->ActiveDomain = 0;

  svtkXdmfDomain* domain = new svtkXdmfDomain(&this->XMLDOM, index);
  if (!domain->IsValid())
  {
    delete domain;
    return false;
  }
  this->ActiveDomain = domain;
  this->ActiveDomainIndex = index;
  return true;
}

//*****************************************************************************
// svtkXdmfDomain

//----------------------------------------------------------------------------
svtkXdmfDomain::svtkXdmfDomain(XdmfDOM* xmlDom, int domain_index)
{
  this->XMLDOM = 0;
  this->XMFGrids = nullptr;
  this->NumberOfGrids = 0;
  this->SIL = svtkMutableDirectedGraph::New();
  this->SILBuilder = svtkSILBuilder::New();
  this->SILBuilder->SetSIL(this->SIL);

  this->PointArrays = new svtkXdmfArraySelection;
  this->CellArrays = new svtkXdmfArraySelection;
  this->Grids = new svtkXdmfArraySelection;
  this->Sets = new svtkXdmfArraySelection;

  this->XMLDomain = xmlDom->FindElement("Domain", domain_index);
  if (!this->XMLDomain)
  {
    // no such domain exists!!!
    return;
  }
  this->XMLDOM = xmlDom;

  // Allocate XdmfGrid instances for each of the grids in this domain.
  this->NumberOfGrids = this->XMLDOM->FindNumberOfElements("Grid", this->XMLDomain);
  this->XMFGrids = new XdmfGrid[this->NumberOfGrids + 1];

  XdmfXmlNode xmlGrid = this->XMLDOM->FindElement("Grid", 0, this->XMLDomain);
  XdmfInt64 cc = 0;
  while (xmlGrid)
  {
    this->XMFGrids[cc].SetDOM(this->XMLDOM);
    this->XMFGrids[cc].SetElement(xmlGrid);
    // Read the light data for this grid (and all its sub-grids, if
    // applicable).
    this->XMFGrids[cc].UpdateInformation();

    xmlGrid = this->XMLDOM->FindNextElement("Grid", xmlGrid);
    cc++;
  }

  // There are a few meta-information that we need to collect from the domain
  // * number of data-arrays so that the user can choose which to load.
  // * grid-structure so that the user can choose the hierarchy
  // * time information so that reader can report the number of timesteps
  //   available.
  this->CollectMetaData();
}

//----------------------------------------------------------------------------
svtkXdmfDomain::~svtkXdmfDomain()
{
  // free the XdmfGrid allocated.
  delete[] this->XMFGrids;
  this->XMFGrids = nullptr;
  this->SIL->Delete();
  this->SIL = 0;
  this->SILBuilder->Delete();
  this->SILBuilder = 0;
  delete this->PointArrays;
  delete this->CellArrays;
  delete this->Grids;
  delete this->Sets;
}

//----------------------------------------------------------------------------
XdmfGrid* svtkXdmfDomain::GetGrid(XdmfInt64 cc)
{
  if (cc >= 0 && cc < this->NumberOfGrids)
  {
    return &this->XMFGrids[cc];
  }
  return nullptr;
}

//----------------------------------------------------------------------------
int svtkXdmfDomain::GetSVTKDataType()
{
  if (this->NumberOfGrids > 1)
  {
    return SVTK_MULTIBLOCK_DATA_SET;
  }
  if (this->NumberOfGrids == 1)
  {
    return this->GetSVTKDataType(&this->XMFGrids[0]);
  }
  return -1;
}

//----------------------------------------------------------------------------
int svtkXdmfDomain::GetSVTKDataType(XdmfGrid* xmfGrid)
{
  XdmfInt32 gridType = xmfGrid->GetGridType();
  if ((gridType & XDMF_GRID_COLLECTION) &&
    xmfGrid->GetCollectionType() == XDMF_GRID_COLLECTION_TEMPORAL)
  {
    // this is a temporal collection, the type depends on the child with
    // correct time-stamp. But since we assume that all items in a temporal
    // collection must be of the same type, we simply use the first child.
    return this->GetSVTKDataType(xmfGrid->GetChild(0));
  }

  if ((gridType & XDMF_GRID_COLLECTION) || (gridType & XDMF_GRID_TREE))
  {
    return SVTK_MULTIBLOCK_DATA_SET;
  }
  if (xmfGrid->GetTopology()->GetClass() == XDMF_UNSTRUCTURED)
  {
    return SVTK_UNSTRUCTURED_GRID;
  }
  XdmfInt32 topologyType = xmfGrid->GetTopology()->GetTopologyType();
  if (topologyType == XDMF_2DSMESH || topologyType == XDMF_3DSMESH)
  {
    return SVTK_STRUCTURED_GRID;
  }
  else if (topologyType == XDMF_2DCORECTMESH || topologyType == XDMF_3DCORECTMESH)
  {
#ifdef USE_IMAGE_DATA
    return SVTK_IMAGE_DATA;
#else
    return SVTK_UNIFORM_GRID;
#endif
  }
  else if (topologyType == XDMF_2DRECTMESH || topologyType == XDMF_3DRECTMESH)
  {
    return SVTK_RECTILINEAR_GRID;
  }
  return -1;
}

//----------------------------------------------------------------------------
int svtkXdmfDomain::GetIndexForTime(double time)
{
  std::map<XdmfFloat64, int>::const_iterator iter = this->TimeSteps.find(time);
  if (iter != this->TimeSteps.end())
  {
    return iter->second;
  }

  iter = this->TimeSteps.upper_bound(time);
  if (iter == this->TimeSteps.begin())
  {
    // The requested time step is before any available time.  We will use it by
    // doing nothing here.
  }
  else
  {
    // Back up one to the item we really want.
    --iter;
  }

  std::map<XdmfFloat64, int>::iterator iter2 = this->TimeSteps.begin();
  int counter = 0;
  while (iter2 != iter)
  {
    ++iter2;
    counter++;
  }

  return counter;
}

//----------------------------------------------------------------------------
XdmfGrid* svtkXdmfDomain::GetGrid(XdmfGrid* xmfGrid, double time)
{
  XdmfInt32 gridType = xmfGrid->GetGridType();
  if ((gridType & XDMF_GRID_COLLECTION) &&
    xmfGrid->GetCollectionType() == XDMF_GRID_COLLECTION_TEMPORAL)
  {
    for (XdmfInt32 cc = 0; cc < xmfGrid->GetNumberOfChildren(); cc++)
    {
      XdmfGrid* child = xmfGrid->GetChild(cc);
      if (child && child->GetTime()->IsValid(time, time))
      {
        return child;
      }
    }

    // It's possible that user has not specified a <Time /> element at all. In
    // that case, try to locate the first grid with no time value set.
    for (XdmfInt32 cc = 0; cc < xmfGrid->GetNumberOfChildren(); cc++)
    {
      XdmfGrid* child = xmfGrid->GetChild(cc);
      if (child && child->GetTime()->GetTimeType() == XDMF_TIME_UNSET)
      {
        return child;
      }
    }

    // not sure what to do if no sub-grid matches the requested time.
    return nullptr;
  }

  return xmfGrid;
}

//----------------------------------------------------------------------------
bool svtkXdmfDomain::IsStructured(XdmfGrid* xmfGrid)
{
  switch (this->GetSVTKDataType(xmfGrid))
  {
    case SVTK_IMAGE_DATA:
    case SVTK_UNIFORM_GRID:
    case SVTK_RECTILINEAR_GRID:
    case SVTK_STRUCTURED_GRID:
      return true;
  }

  return false;
}

//----------------------------------------------------------------------------
bool svtkXdmfDomain::GetWholeExtent(XdmfGrid* xmfGrid, int extents[6])
{
  extents[0] = extents[2] = extents[4] = 0;
  extents[1] = extents[3] = extents[5] = -1;
  if (!this->IsStructured(xmfGrid))
  {
    return false;
  }

  XdmfInt64 dimensions[XDMF_MAX_DIMENSION];
  XdmfDataDesc* xmfDataDesc = xmfGrid->GetTopology()->GetShapeDesc();
  XdmfInt32 num_of_dims = xmfDataDesc->GetShape(dimensions);
  // clear out un-filled dimensions.
  for (int cc = num_of_dims; cc < 3; cc++) // only need to until the 3rd dimension
                                           // since we don't care about any higher
                                           // dimensions yet.
  {
    dimensions[cc] = 1;
  }

  // svtk Dims are i,j,k XDMF are k,j,i
  extents[5] = svtkMAX(static_cast<XdmfInt64>(0), dimensions[0] - 1);
  extents[3] = svtkMAX(static_cast<XdmfInt64>(0), dimensions[1] - 1);
  extents[1] = svtkMAX(static_cast<XdmfInt64>(0), dimensions[2] - 1);
  return true;
}

//----------------------------------------------------------------------------
bool svtkXdmfDomain::GetOriginAndSpacing(XdmfGrid* xmfGrid, double origin[3], double spacing[3])
{
  if (xmfGrid->GetTopology()->GetTopologyType() != XDMF_2DCORECTMESH &&
    xmfGrid->GetTopology()->GetTopologyType() != XDMF_3DCORECTMESH)
  {
    return false;
  }

  XdmfGeometry* xmfGeometry = xmfGrid->GetGeometry();
  if (xmfGeometry->GetGeometryType() == XDMF_GEOMETRY_ORIGIN_DXDYDZ)
  {
    // Update geometry so that origin and spacing are read
    xmfGeometry->Update(); // read heavy-data for the geometry.
    XdmfFloat64* xmfOrigin = xmfGeometry->GetOrigin();
    XdmfFloat64* xmfSpacing = xmfGeometry->GetDxDyDz();
    origin[0] = xmfOrigin[2];
    origin[1] = xmfOrigin[1];
    origin[2] = xmfOrigin[0];

    spacing[0] = xmfSpacing[2];
    spacing[1] = xmfSpacing[1];
    spacing[2] = xmfSpacing[0];
    return true;
  }
  else if (xmfGeometry->GetGeometryType() == XDMF_GEOMETRY_ORIGIN_DXDY)
  {
    // Update geometry so that origin and spacing are read
    xmfGeometry->Update(); // read heavy-data for the geometry.
    XdmfFloat64* xmfOrigin = xmfGeometry->GetOrigin();
    XdmfFloat64* xmfSpacing = xmfGeometry->GetDxDyDz();
    origin[0] = 0.0;
    origin[1] = xmfOrigin[1];
    origin[2] = xmfOrigin[0];

    spacing[0] = 1.0;
    spacing[1] = xmfSpacing[1];
    spacing[2] = xmfSpacing[0];
    return true;
  }
  origin[0] = origin[1] = origin[2] = 0.0;
  spacing[0] = spacing[1] = spacing[2] = 1.0;
  return false;
}

//----------------------------------------------------------------------------
int svtkXdmfDomain::GetDataDimensionality(XdmfGrid* xmfGrid)
{
  if (!xmfGrid || !xmfGrid->IsUniform())
  {
    return -1;
  }

  switch (xmfGrid->GetTopology()->GetTopologyType())
  {
    case XDMF_NOTOPOLOGY:
    case XDMF_POLYVERTEX:
    case XDMF_POLYLINE:
    case XDMF_POLYGON:
    case XDMF_TRI:
    case XDMF_QUAD:
    case XDMF_TET:
    case XDMF_PYRAMID:
    case XDMF_WEDGE:
    case XDMF_HEX:
    case XDMF_EDGE_3:
    case XDMF_TRI_6:
    case XDMF_QUAD_8:
    case XDMF_QUAD_9:
    case XDMF_TET_10:
    case XDMF_PYRAMID_13:
    case XDMF_WEDGE_15:
    case XDMF_WEDGE_18:
    case XDMF_HEX_20:
    case XDMF_HEX_24:
    case XDMF_HEX_27:
    case XDMF_MIXED:
      return 1; // unstructured data-sets have no inherent dimensionality.

    case XDMF_2DSMESH:
    case XDMF_2DRECTMESH:
    case XDMF_2DCORECTMESH:
      return 2;

    case XDMF_3DSMESH:
    case XDMF_3DRECTMESH:
    case XDMF_3DCORECTMESH:
      return 3;
  }

  return -1;
}

//----------------------------------------------------------------------------
void svtkXdmfDomain::CollectMetaData()
{
  this->SILBuilder->Initialize();
  this->GridsOverflowCounter = 0;

  svtkIdType blocksRoot = this->SILBuilder->AddVertex("Blocks");
  svtkIdType hierarchyRoot = this->SILBuilder->AddVertex("Hierarchy");
  this->SILBuilder->AddChildEdge(this->SILBuilder->GetRootVertex(), blocksRoot);
  this->SILBuilder->AddChildEdge(this->SILBuilder->GetRootVertex(), hierarchyRoot);
  this->SILBlocksRoot = blocksRoot;

  for (XdmfInt64 cc = 0; cc < this->NumberOfGrids; cc++)
  {
    this->CollectMetaData(&this->XMFGrids[cc], hierarchyRoot);
  }

  if (this->GridsOverflowCounter >= MAX_COLLECTABLE_NUMBER_OF_GRIDS)
  {
    this->Grids->clear();

    // We have aborted collecting grids information since it was too numerous to
    // be of any use to the user.
    this->SILBuilder->Initialize();
    blocksRoot = this->SILBuilder->AddVertex("Blocks");
    hierarchyRoot = this->SILBuilder->AddVertex("Hierarchy");
    this->SILBuilder->AddChildEdge(this->SILBuilder->GetRootVertex(), blocksRoot);
    this->SILBuilder->AddChildEdge(this->SILBuilder->GetRootVertex(), hierarchyRoot);
    this->SILBlocksRoot = blocksRoot;

    // add only the top-level grids.
    for (XdmfInt64 cc = 0; cc < this->NumberOfGrids; cc++)
    {
      XdmfGrid* xmfGrid = &this->XMFGrids[cc];

      std::string originalGridName = xmfGrid->GetName();
      std::string gridName = xmfGrid->GetName();
      unsigned int count = 1;
      while (this->Grids->HasArray(gridName.c_str()))
      {
        std::ostringstream str;
        str << xmfGrid->GetName() << "[" << count << "]";
        gridName = str.str();
        count++;
      }
      xmfGrid->SetName(gridName.c_str());
      this->Grids->AddArray(gridName.c_str());

      svtkIdType silVertex = this->SILBuilder->AddVertex(xmfGrid->GetName());
      this->SILBuilder->AddChildEdge(this->SILBlocksRoot, silVertex);

      svtkIdType hierarchyVertex = this->SILBuilder->AddVertex(originalGridName.c_str());
      this->SILBuilder->AddChildEdge(hierarchyRoot, hierarchyVertex);
      this->SILBuilder->AddCrossEdge(hierarchyVertex, silVertex);
    }
  }
}

//----------------------------------------------------------------------------
void svtkXdmfDomain::CollectMetaData(XdmfGrid* xmfGrid, svtkIdType silParent)
{
  if (!xmfGrid)
  {
    return;
  }

  // All grids need to be named. If a grid doesn't have a name, we make one
  // up.
  if (xmfGrid->GetName() == nullptr)
  {
    xmfGrid->SetName(this->XMLDOM->GetUniqueName("Grid"));
  }

  if (xmfGrid->IsUniform())
  {
    this->CollectLeafMetaData(xmfGrid, silParent);
  }
  else
  {
    this->CollectNonLeafMetaData(xmfGrid, silParent);
  }
}

//----------------------------------------------------------------------------
void svtkXdmfDomain::CollectNonLeafMetaData(XdmfGrid* xmfGrid, svtkIdType silParent)
{
  svtkIdType silVertex = -1;
  if (silParent != -1 && this->GridsOverflowCounter < MAX_COLLECTABLE_NUMBER_OF_GRIDS)
  {
    // stop building SIL as soon as we have too many blocks--not worth it.
    this->GridsOverflowCounter++;

    // FIXME: how to reflect temporal collections in the SIL?
    silVertex = this->SILBuilder->AddVertex(xmfGrid->GetName());
    this->SILBuilder->AddChildEdge(silParent, silVertex);
  }

  XdmfInt32 numChildren = xmfGrid->GetNumberOfChildren();
  for (XdmfInt32 cc = 0; cc < numChildren; cc++)
  {
    XdmfGrid* xmfChild = xmfGrid->GetChild(cc);
    this->CollectMetaData(xmfChild, silVertex);
  }

  // Collect time information
  // If a non-leaf node is a temporal collection then it may have a <Time/>
  // element which defines the time values for the grids in the collection.
  // Xdmf handles those elements and explicitly sets the Time value on those
  // children, so we don't need to process that. We need to handle only the
  // case when a non-leaf,non-temporal collection has a time value of it's
  // own.
  if ((xmfGrid->GetGridType() & XDMF_GRID_COLLECTION) == 0 ||
    xmfGrid->GetCollectionType() != XDMF_GRID_COLLECTION_TEMPORAL)
  {
    // assert(grid is not a temporal collection).
    XdmfTime* xmfTime = xmfGrid->GetTime();
    if (xmfTime && xmfTime->GetTimeType() != XDMF_TIME_UNSET)
    {
      int step = static_cast<int>(this->TimeSteps.size());
      if (this->TimeSteps.find(xmfTime->GetValue()) == this->TimeSteps.end())
      {
        this->TimeSteps[xmfTime->GetValue()] = step;
        this->TimeStepsRev[step] = xmfTime->GetValue();
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkXdmfDomain::CollectLeafMetaData(XdmfGrid* xmfGrid, svtkIdType silParent)
{
  svtkIdType silVertex = -1;
  if (silParent != -1 && this->GridsOverflowCounter < MAX_COLLECTABLE_NUMBER_OF_GRIDS)
  {
    std::string originalGridName = xmfGrid->GetName();
    std::string gridName = xmfGrid->GetName();
    unsigned int count = 1;
    while (this->Grids->HasArray(gridName.c_str()))
    {
      std::ostringstream str;
      str << xmfGrid->GetName() << "[" << count << "]";
      gridName = str.str();
      count++;
    }
    xmfGrid->SetName(gridName.c_str());
    this->Grids->AddArray(gridName.c_str());

    silVertex = this->SILBuilder->AddVertex(xmfGrid->GetName());
    this->SILBuilder->AddChildEdge(this->SILBlocksRoot, silVertex);

    svtkIdType hierarchyVertex = this->SILBuilder->AddVertex(originalGridName.c_str());
    this->SILBuilder->AddChildEdge(silParent, hierarchyVertex);
    this->SILBuilder->AddCrossEdge(hierarchyVertex, silVertex);

    this->GridsOverflowCounter++;
  }

  // Collect attribute arrays information.
  XdmfInt32 numAttributes = xmfGrid->GetNumberOfAttributes();
  for (XdmfInt32 kk = 0; kk < numAttributes; kk++)
  {
    XdmfAttribute* xmfAttribute = xmfGrid->GetAttribute(kk);
    const char* name = xmfAttribute->GetName();
    if (!name)
    {
      continue;
    }
    XdmfInt32 attributeCenter = xmfAttribute->GetAttributeCenter();
    if (attributeCenter == XDMF_ATTRIBUTE_CENTER_NODE)
    {
      this->PointArrays->AddArray(name);
    }
    else if (attributeCenter == XDMF_ATTRIBUTE_CENTER_CELL)
    {
      this->CellArrays->AddArray(name);
    }
    else if (attributeCenter == XDMF_ATTRIBUTE_CENTER_GRID && silVertex != -1)
    {
      this->UpdateGridAttributeInSIL(xmfAttribute, silVertex);
    }
  }

  // Collect sets information
  XdmfInt32 numSets = xmfGrid->GetNumberOfSets();
  for (XdmfInt32 kk = 0; kk < numSets; kk++)
  {
    XdmfSet* xmfSet = xmfGrid->GetSets(kk);
    const char* name = xmfSet->GetName();

    // if the set is a ghost-cell/node set, then it's not treated as a set for
    // which a new svtkDataSet is created (nor can the user enable-disable it
    // [ofcourse the pipeline will, by using the UPDATE_NUMBER_OF_GHOST_LEVELS()
    // in the request]).
    if (!name || xmfSet->GetGhost() != 0)
    {
      continue;
    }

    // XdmfInt32 setCenter = xmfSet->GetSetType();
    // Not sure if we want to create separate lists for different types of sets
    // or just treat all the sets as same. For now, we are treating them as
    // the same.
    this->Sets->AddArray(name);
  }

  // A leaf node may have a single value time.
  XdmfTime* xmfTime = xmfGrid->GetTime();
  if (xmfTime && xmfTime->GetTimeType() != XDMF_TIME_UNSET)
  {
    int step = static_cast<int>(this->TimeSteps.size());
    if (this->TimeSteps.find(xmfTime->GetValue()) == this->TimeSteps.end())
    {
      this->TimeSteps[xmfTime->GetValue()] = step;
      this->TimeStepsRev[step] = xmfTime->GetValue();
    }
  }
}

//----------------------------------------------------------------------------
bool svtkXdmfDomain::UpdateGridAttributeInSIL(XdmfAttribute* xmfAttribute, svtkIdType silVertex)
{
  // Check if the grid centered attribute is an single component integeral
  // value, (or a string, in future). If that's the case, then these become
  // part of the SIL.
  XdmfDataItem xmfDataItem;
  xmfDataItem.SetDOM(xmfAttribute->GetDOM());
  xmfDataItem.SetElement(xmfAttribute->GetDOM()->FindDataElement(0, xmfAttribute->GetElement()));
  xmfDataItem.UpdateInformation();
  xmfDataItem.Update();

  svtkXdmfDataArray* xmfConvertor = svtkXdmfDataArray::New();
  svtkSmartPointer<svtkDataArray> dataArray;
  dataArray.TakeReference(
    xmfConvertor->FromXdmfArray(xmfDataItem.GetArray()->GetTagName(), 1, 1, 1, 0));
  xmfConvertor->Delete();

  if (dataArray->GetNumberOfTuples() != 1 || dataArray->GetNumberOfComponents() != 1)
  {
    // only single valued arrays are of concern.
    return false;
  }

  switch (dataArray->GetDataType())
  {
    case SVTK_CHAR:
    case SVTK_UNSIGNED_CHAR:
    case SVTK_SHORT:
    case SVTK_UNSIGNED_SHORT:
    case SVTK_INT:
    case SVTK_UNSIGNED_INT:
    case SVTK_LONG:
    case SVTK_UNSIGNED_LONG:
      break;

    default:
      return false; // skip non-integeral types.
  }

  const char* name = xmfAttribute->GetName();
  svtkIdType arrayRoot;
  if (this->GridCenteredAttrbuteRoots.find(name) == this->GridCenteredAttrbuteRoots.end())
  {
    arrayRoot = this->SILBuilder->AddVertex(name);
    this->SILBuilder->AddChildEdge(this->SILBuilder->GetRootVertex(), arrayRoot);
    this->GridCenteredAttrbuteRoots[name] = arrayRoot;
  }
  else
  {
    arrayRoot = this->GridCenteredAttrbuteRoots[name];
  }

  svtkVariant variantValue = dataArray->GetVariantValue(0);
  XdmfInt64 value = variantValue.ToTypeInt64();
  svtkIdType valueRoot;
  if (this->GridCenteredAttrbuteValues[arrayRoot].find(value) ==
    this->GridCenteredAttrbuteValues[arrayRoot].end())
  {
    valueRoot = this->SILBuilder->AddVertex(variantValue.ToString().c_str());
    this->SILBuilder->AddChildEdge(arrayRoot, valueRoot);
    this->GridCenteredAttrbuteValues[arrayRoot][value] = valueRoot;
  }
  else
  {
    valueRoot = this->GridCenteredAttrbuteValues[arrayRoot][value];
  }
  this->SILBuilder->AddCrossEdge(valueRoot, silVertex);
  return true;
}

//----------------------------------------------------------------------------
//----------------------------------------------------------------------------
