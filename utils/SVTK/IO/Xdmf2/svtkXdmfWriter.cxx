/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmfWriter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkXdmfWriter.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkCellType.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkCompositeDataSet.h"
#include "svtkDataObject.h"
#include "svtkDataObjectTreeIterator.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkFieldData.h"
#include "svtkGenericCell.h"
#include "svtkIdList.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkRectilinearGrid.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkTypeTraits.h"
#include "svtkUnstructuredGrid.h"
#include "svtksys/SystemTools.hxx"

#include "svtk_xdmf2.h"
#include SVTKXDMF2_HEADER(XdmfArray.h)
#include SVTKXDMF2_HEADER(XdmfAttribute.h)
#include SVTKXDMF2_HEADER(XdmfDataDesc.h)
#include SVTKXDMF2_HEADER(XdmfDOM.h)
#include SVTKXDMF2_HEADER(XdmfDomain.h)
#include SVTKXDMF2_HEADER(XdmfGeometry.h)
#include SVTKXDMF2_HEADER(XdmfGrid.h)
#include SVTKXDMF2_HEADER(XdmfRoot.h)
#include SVTKXDMF2_HEADER(XdmfTime.h)
#include SVTKXDMF2_HEADER(XdmfTopology.h)

#include <algorithm>
#include <cstdio>
#include <map>
#include <sstream>
#include <vector>

#include "svtk_libxml2.h"
#include SVTKLIBXML2_HEADER(tree.h) // always after std::blah stuff

#ifdef SVTK_USE_64BIT_IDS
typedef XdmfInt64 svtkXdmfIdType;
#else
typedef XdmfInt32 svtkXdmfIdType;
#endif

using namespace xdmf2;

struct _xmlNode;
typedef _xmlNode* XdmfXmlNode;
struct svtkXW2NodeHelp
{
  xdmf2::XdmfDOM* DOM;
  XdmfXmlNode node;
  bool staticFlag;
  svtkXW2NodeHelp(xdmf2::XdmfDOM* d, XdmfXmlNode n, bool f)
    : DOM(d)
    , node(n)
    , staticFlag(f)
  {
  }
};

class svtkXdmfWriterDomainMemoryHandler
{
public:
  svtkXdmfWriterDomainMemoryHandler() { domain = new XdmfDomain(); }
  ~svtkXdmfWriterDomainMemoryHandler()
  {
    for (std::vector<xdmf2::XdmfGrid*>::iterator iter = domainGrids.begin();
         iter != domainGrids.end(); ++iter)
    {
      delete *iter;
    }
    delete domain;
  }
  void InsertGrid(xdmf2::XdmfGrid* grid)
  {
    domain->Insert(grid);
    domainGrids.push_back(grid);
  }
  void InsertIntoRoot(XdmfRoot& root) { root.Insert(domain); }

private:
  XdmfDomain* domain;
  std::vector<xdmf2::XdmfGrid*> domainGrids;
};

//==============================================================================

struct svtkXdmfWriterInternal
{
  class CellType
  {
  public:
    CellType()
      : SVTKType(0)
      , NumPoints(0)
    {
    }
    CellType(const CellType& ct)
      : SVTKType(ct.SVTKType)
      , NumPoints(ct.NumPoints)
    {
    }
    svtkIdType SVTKType;
    svtkIdType NumPoints;
    bool operator<(const CellType& ct) const
    {
      return this->SVTKType < ct.SVTKType ||
        (this->SVTKType == ct.SVTKType && this->NumPoints < ct.NumPoints);
    }
    bool operator==(const CellType& ct) const
    {
      return this->SVTKType == ct.SVTKType && this->NumPoints == ct.NumPoints;
    }
    CellType& operator=(const CellType& ct)
    {
      this->SVTKType = ct.SVTKType;
      this->NumPoints = ct.NumPoints;
      return *this;
    }
  };
  typedef std::map<CellType, svtkSmartPointer<svtkIdList> > MapOfCellTypes;
  static void DetermineCellTypes(svtkPointSet* t, MapOfCellTypes& vec);
};

//----------------------------------------------------------------------------
void svtkXdmfWriterInternal::DetermineCellTypes(
  svtkPointSet* t, svtkXdmfWriterInternal::MapOfCellTypes& vec)
{
  if (!t)
  {
    return;
  }
  svtkIdType cc;
  svtkGenericCell* cell = svtkGenericCell::New();
  for (cc = 0; cc < t->GetNumberOfCells(); cc++)
  {
    svtkXdmfWriterInternal::CellType ct;
    t->GetCell(cc, cell);
    ct.SVTKType = cell->GetCellType();
    ct.NumPoints = cell->GetNumberOfPoints();
    svtkXdmfWriterInternal::MapOfCellTypes::iterator it = vec.find(ct);
    if (it == vec.end())
    {
      svtkIdList* l = svtkIdList::New();
      it = vec
             .insert(
               svtkXdmfWriterInternal::MapOfCellTypes::value_type(ct, svtkSmartPointer<svtkIdList>(l)))
             .first;
      l->Delete();
    }
    // it->second->InsertUniqueId(cc);
    it->second->InsertNextId(cc);
  }
  cell->Delete();
}

//==============================================================================

svtkStandardNewMacro(svtkXdmfWriter);

//----------------------------------------------------------------------------
svtkXdmfWriter::svtkXdmfWriter()
{
  this->FileName = nullptr;
  this->HeavyDataFileName = nullptr;
  this->HeavyDataGroupName = nullptr;
  this->DOM = nullptr;
  this->Piece = 0; // for parallel
  this->NumberOfPieces = 1;
  this->LightDataLimit = 100;
  this->WriteAllTimeSteps = 0;
  this->NumberOfTimeSteps = 1;
  this->CurrentTimeIndex = 0;
  this->TopTemporalGrid = nullptr;
  this->DomainMemoryHandler = nullptr;
  this->SetNumberOfOutputPorts(0);
  this->MeshStaticOverTime = false;
}

//----------------------------------------------------------------------------
svtkXdmfWriter::~svtkXdmfWriter()
{
  this->SetFileName(nullptr);
  this->SetHeavyDataFileName(nullptr);
  this->SetHeavyDataGroupName(nullptr);
  delete this->DOM;
  this->DOM = nullptr;
  delete this->DomainMemoryHandler;
  this->DomainMemoryHandler = nullptr;
  delete this->TopTemporalGrid;
  this->TopTemporalGrid = nullptr;

  // TODO: Verify memory isn't leaking
}

//-----------------------------------------------------------------------------
svtkExecutive* svtkXdmfWriter::CreateDefaultExecutive()
{
  return svtkCompositeDataPipeline::New();
}

//----------------------------------------------------------------------------
void svtkXdmfWriter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "LightDataLimit: " << this->LightDataLimit << endl;
  os << indent << "WriteAllTimeSteps: " << (this->WriteAllTimeSteps ? "ON" : "OFF") << endl;
}

//------------------------------------------------------------------------------
void svtkXdmfWriter::SetInputData(svtkDataObject* input)
{
  this->SetInputDataInternal(0, input);
}

//------------------------------------------------------------------------------
int svtkXdmfWriter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//------------------------------------------------------------------------------
int svtkXdmfWriter::Write()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    svtkErrorMacro("No input provided!");
    return 0;
  }

  // always write even if the data hasn't changed
  this->Modified();

  this->TopologyAtT0.clear();
  this->GeometryAtT0.clear();
  this->UnlabelledDataArrayId = 0;

  // TODO: Specify name of heavy data companion file?
  if (!this->DOM)
  {
    this->DOM = new xdmf2::XdmfDOM();
  }
  this->DOM->SetOutputFileName(this->FileName);

  XdmfRoot root;
  root.SetDOM(this->DOM);
  root.SetVersion(2.2);
  root.Build();

  delete this->DomainMemoryHandler;
  this->DomainMemoryHandler = new svtkXdmfWriterDomainMemoryHandler();
  this->DomainMemoryHandler->InsertIntoRoot(root);

  this->Update();

  root.Build();
  this->DOM->Write();

  delete this->DomainMemoryHandler;
  this->DomainMemoryHandler = nullptr;

  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmfWriter::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  // Does the input have timesteps?
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->NumberOfTimeSteps = 1;
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmfWriter::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  double* inTimes =
    inputVector[0]->GetInformationObject(0)->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes && this->WriteAllTimeSteps)
  {
    // TODO:? Add a user ivar to specify a particular time,
    // which is different from current time. Can do it by updating
    // to a particular time then writing without writealltimesteps,
    // but that is annoying.
    double timeReq = inTimes[this->CurrentTimeIndex];
    inputVector[0]->GetInformationObject(0)->Set(
      svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmfWriter::RequestData(svtkInformation* request, svtkInformationVector** inputVector,
  svtkInformationVector* svtkNotUsed(outputVector))
{
  if (!this->DomainMemoryHandler)
  {
    // call Write instead of this directly. That does setup first, then calls this.
    return 1;
  }

  this->WorkingDirectory = svtksys::SystemTools::GetFilenamePath(this->FileName);
  this->BaseFileName = svtksys::SystemTools::GetFilenameWithoutLastExtension(this->FileName);

  // If mesh is static we force heavy data to be exported in HDF
  int lightDataLimit = this->LightDataLimit;
  this->LightDataLimit = this->MeshStaticOverTime ? 1 : this->LightDataLimit;

  this->CurrentBlockIndex = 0;

  if (this->CurrentTimeIndex == 0 && this->WriteAllTimeSteps && this->NumberOfTimeSteps > 1)
  {
    // Tell the pipeline to start looping.
    request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);

    // make a top level temporal grid just under domain
    delete this->TopTemporalGrid;
    this->TopTemporalGrid = nullptr;

    xdmf2::XdmfGrid* tgrid = new xdmf2::XdmfGrid();
    tgrid->SetDeleteOnGridDelete(true);
    tgrid->SetGridType(XDMF_GRID_COLLECTION);
    tgrid->SetCollectionType(XDMF_GRID_COLLECTION_TEMPORAL);
    tgrid->SetName(this->BaseFileName.c_str());
    XdmfTopology* t = tgrid->GetTopology();
    t->SetTopologyType(XDMF_NOTOPOLOGY);
    XdmfGeometry* geo = tgrid->GetGeometry();
    geo->SetGeometryType(XDMF_GEOMETRY_NONE);

    this->DomainMemoryHandler->InsertGrid(tgrid);
    this->TopTemporalGrid = tgrid;
  }

  xdmf2::XdmfGrid* grid = new xdmf2::XdmfGrid();
  grid->SetDeleteOnGridDelete(true);
  if (this->TopTemporalGrid)
  {
    this->TopTemporalGrid->Insert(grid);
  }
  else
  {
    this->DomainMemoryHandler->InsertGrid(grid);
  }

  this->CurrentTime = 0;

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkInformation* inDataInfo = input->GetInformation();
  if (inDataInfo->Has(svtkDataObject::DATA_TIME_STEP()))
  {
    // I am assuming we are not given a temporal data object and getting just one time.
    this->CurrentTime = input->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP());
    // cerr << "Writing timestep" << this->CurrentTimeIndex << " (" << this->CurrentTime << ")" <<
    // endl;

    XdmfTime* xT = grid->GetTime();
    xT->SetDeleteOnGridDelete(true);
    xT->SetTimeType(XDMF_TIME_SINGLE);
    xT->SetValue(this->CurrentTime);
    grid->Insert(xT);
  }

  this->WriteDataSet(input, grid);
  // delete grid; //domain takes care of it?

  this->CurrentTimeIndex++;
  if (this->CurrentTimeIndex >= this->NumberOfTimeSteps && this->WriteAllTimeSteps)
  {
    // Tell the pipeline to stop looping.
    request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
    this->CurrentTimeIndex = 0;
    // delete this->TopTemporalGrid; //domain takes care of it?
    this->TopTemporalGrid = nullptr;
  }

  this->LightDataLimit = lightDataLimit;
  return 1;
}

//------------------------------------------------------------------------------
int svtkXdmfWriter::WriteDataSet(svtkDataObject* dobj, xdmf2::XdmfGrid* grid)
{
  // TODO:
  // respect parallelism
  if (!dobj)
  {
    // svtkWarningMacro(<< "Null DS, someone else will take care of it");
    return 0;
  }
  if (!grid)
  {
    svtkWarningMacro(<< "Something is wrong, grid should have already been created for " << dobj);
    return 0;
  }

  svtkCompositeDataSet* cdobj = svtkCompositeDataSet::SafeDownCast(dobj);
  if (cdobj) //! dobj->IsTypeOf("svtkCompositeDataSet")) //TODO: Why doesn't IsTypeOf work?
  {
    this->WriteCompositeDataSet(cdobj, grid);
    return 1;
  }

  return this->WriteAtomicDataSet(dobj, grid);
}

//------------------------------------------------------------------------------
int svtkXdmfWriter::WriteCompositeDataSet(svtkCompositeDataSet* dobj, xdmf2::XdmfGrid* grid)
{
  // cerr << "internal node " << dobj << " is a " << dobj->GetClassName() << endl;
  if (dobj->IsA("svtkMultiPieceDataSet"))
  {
    grid->SetGridType(XDMF_GRID_COLLECTION);
    grid->SetCollectionType(XDMF_GRID_COLLECTION_SPATIAL);
  }
  else
  {
    // fine for svtkMultiBlockDataSet
    // svtkHierarchicalBoxDataSet would be better served by a different xdmf tree type
    // svtkTemporalDataSet is internal to the SVTK pipeline so I am ignoring it
    grid->SetGridType(XDMF_GRID_TREE);
  }

  XdmfTopology* t = grid->GetTopology();
  t->SetTopologyType(XDMF_NOTOPOLOGY);
  XdmfGeometry* geo = grid->GetGeometry();
  geo->SetGeometryType(XDMF_GEOMETRY_NONE);

  svtkCompositeDataIterator* iter = dobj->NewIterator();
  svtkDataObjectTreeIterator* treeIter = svtkDataObjectTreeIterator::SafeDownCast(iter);
  if (treeIter)
  {
    treeIter->VisitOnlyLeavesOff();
    treeIter->TraverseSubTreeOff();
  }
  svtkMultiBlockDataSet* mbds = svtkMultiBlockDataSet::SafeDownCast(dobj);
  iter->GoToFirstItem();
  while (!iter->IsDoneWithTraversal())
  {
    xdmf2::XdmfGrid* childsGrid = new xdmf2::XdmfGrid();
    childsGrid->SetDeleteOnGridDelete(true);
    grid->Insert(childsGrid);
    svtkDataObject* ds = iter->GetCurrentDataObject();

    if (mbds)
    {
      svtkInformation* info = mbds->GetMetaData(iter->GetCurrentFlatIndex() - 1);
      if (info)
      {
        childsGrid->SetName(info->Get(svtkCompositeDataSet::NAME()));
      }
    }

    this->WriteDataSet(ds, childsGrid);
    // delete childsGrid; //parent deletes children in Xdmf
    iter->GoToNextItem();
  }
  iter->Delete();

  return 1;
}

//----------------------------------------------------------------------------
void svtkXdmfWriter::SetupDataArrayXML(XdmfElement* e, XdmfArray* a) const
{
  std::stringstream ss;
  ss << "<DataItem Dimensions = \"" << a->GetShapeAsString() << "\" NumberType = \""
     << XdmfTypeToClassString(a->GetNumberType()) << "\" Precision = \"" << a->GetElementSize()
     << "\" Format = \"HDF\">" << a->GetHeavyDataSetName() << "</DataItem>";
  e->SetDataXml(ss.str().c_str());
}

//------------------------------------------------------------------------------
int svtkXdmfWriter::CreateTopology(svtkDataSet* ds, xdmf2::XdmfGrid* grid, svtkIdType PDims[3],
  svtkIdType CDims[3], svtkIdType& PRank, svtkIdType& CRank, void* staticdata)
{
  // cerr << "Writing " << dobj << " a " << dobj->GetClassName() << endl;

  grid->SetGridType(XDMF_GRID_UNIFORM);

  const char* heavyName = nullptr;
  std::string heavyDataSetName;
  if (this->HeavyDataFileName)
  {
    heavyDataSetName = std::string(this->HeavyDataFileName) + ":";
    if (this->MeshStaticOverTime)
    {
      std::stringstream hdf5group;
      hdf5group << "/Topology_";
      if (this->CurrentBlockIndex >= 0)
      {
        if (grid->GetName())
        {
          hdf5group << grid->GetName();
        }
        else
        {
          hdf5group << "Block_" << this->CurrentBlockIndex;
        }
        heavyDataSetName = heavyDataSetName + hdf5group.str();
      }
    }
    else
    {
      if (this->HeavyDataGroupName)
      {
        heavyDataSetName = heavyDataSetName + HeavyDataGroupName + "/Topology";
      }
    }
    heavyName = heavyDataSetName.c_str();
  }

  XdmfTopology* t = grid->GetTopology();
  t->SetLightDataLimit(this->LightDataLimit);

  //
  // If the topology is unchanged from last grid written, we can reuse the XML
  // and avoid writing any heavy data. We must still compute dimensions etc
  // otherwise the attribute arrays don't get initialized properly
  //
  bool reusing_topology = false;
  svtkXW2NodeHelp* staticnode = (svtkXW2NodeHelp*)staticdata;
  if (staticnode)
  {
    if (staticnode->staticFlag)
    {
      grid->Set("TopologyConstant", "True");
    }
    if (staticnode->DOM && staticnode->node)
    {
      XdmfXmlNode staticTopo = staticnode->DOM->FindElement("Topology", 0, staticnode->node);
      XdmfConstString xmltext = staticnode->DOM->Serialize(staticTopo->children);
      XdmfConstString dimensions = staticnode->DOM->Get(staticTopo, "Dimensions");
      XdmfConstString topologyType = staticnode->DOM->Get(staticTopo, "TopologyType");
      //
      t->SetTopologyTypeFromString(topologyType);
      t->SetNumberOfElements(atoi(dimensions));
      t->SetDataXml(xmltext);
      reusing_topology = true;
      // @TODO : t->SetNodesPerElement(ppCell);
    }
  }

  if (this->MeshStaticOverTime)
  {
    if (this->CurrentTimeIndex == 0)
    {
      // Save current topology node at t0 for next time steps
      this->TopologyAtT0.push_back(t);
    }
    else if (static_cast<int>(this->TopologyAtT0.size()) > this->CurrentBlockIndex)
    {
      // Get topology node at t0
      XdmfTopology* topo = this->TopologyAtT0[this->CurrentBlockIndex];
      // Setup current topology node with t0 properties
      t->SetTopologyTypeFromString(topo->GetTopologyTypeAsString());
      t->SetNumberOfElements(topo->GetNumberOfElements());

      // Setup connectivity data XML according t0 one
      this->SetupDataArrayXML(t, topo->GetConnectivity());
      reusing_topology = true;
      // process continue as need to setup PDims parameters
    }
  }

  // Topology
  switch (ds->GetDataObjectType())
  {
    case SVTK_STRUCTURED_POINTS:
    case SVTK_IMAGE_DATA:
    case SVTK_UNIFORM_GRID:
    {
      t->SetTopologyType(XDMF_3DCORECTMESH);
      t->SetLightDataLimit(this->LightDataLimit);
      svtkImageData* id = svtkImageData::SafeDownCast(ds);
      int wExtent[6];
      id->GetExtent(wExtent);
      XdmfInt64 Dims[3];
      Dims[2] = wExtent[1] - wExtent[0] + 1;
      Dims[1] = wExtent[3] - wExtent[2] + 1;
      Dims[0] = wExtent[5] - wExtent[4] + 1;
      XdmfDataDesc* dd = t->GetShapeDesc();
      dd->SetShape(3, Dims);
      // TODO: verify row/column major ordering

      PDims[0] = Dims[0];
      PDims[1] = Dims[1];
      PDims[2] = Dims[2];
      CDims[0] = Dims[0] - 1;
      CDims[1] = Dims[1] - 1;
      CDims[2] = Dims[2] - 1;
    }
    break;
    case SVTK_RECTILINEAR_GRID:
    {
      t->SetTopologyType(XDMF_3DRECTMESH);
      svtkRectilinearGrid* rgrid = svtkRectilinearGrid::SafeDownCast(ds);
      int wExtent[6];
      rgrid->GetExtent(wExtent);
      XdmfInt64 Dims[3];
      Dims[2] = wExtent[1] - wExtent[0] + 1;
      Dims[1] = wExtent[3] - wExtent[2] + 1;
      Dims[0] = wExtent[5] - wExtent[4] + 1;
      XdmfDataDesc* dd = t->GetShapeDesc();
      dd->SetShape(3, Dims);
      // TODO: verify row/column major ordering

      PDims[0] = Dims[0];
      PDims[1] = Dims[1];
      PDims[2] = Dims[2];
      CDims[0] = Dims[0] - 1;
      CDims[1] = Dims[1] - 1;
      CDims[2] = Dims[2] - 1;
    }
    break;
    case SVTK_STRUCTURED_GRID:
    {
      svtkStructuredGrid* sgrid = svtkStructuredGrid::SafeDownCast(ds);
      int rank = CRank = PRank = sgrid->GetDataDimension();
      if (rank == 3)
      {
        t->SetTopologyType(XDMF_3DSMESH);
      }
      else if (rank == 2)
      {
        t->SetTopologyType(XDMF_2DSMESH);
      }
      else
      {
        XdmfErrorMessage("Structured Grid Dimensions can be 2 or 3: " << rank << " found");
      }

      int wExtent[6];
      sgrid->GetExtent(wExtent);
      XdmfInt64 Dims[3];
      Dims[2] = wExtent[1] - wExtent[0] + 1;
      Dims[1] = wExtent[3] - wExtent[2] + 1;
      Dims[0] = wExtent[5] - wExtent[4] + 1;
      XdmfDataDesc* dd = t->GetShapeDesc();
      dd->SetShape(rank, Dims);
      // TODO: verify row/column major ordering

      PDims[0] = Dims[0];
      PDims[1] = Dims[1];
      PDims[2] = Dims[2];
      CDims[0] = Dims[0] - 1;
      CDims[1] = Dims[1] - 1;
      CDims[2] = Dims[2] - 1;
    }
    break;
    case SVTK_POLY_DATA:
    case SVTK_UNSTRUCTURED_GRID:
    {
      PRank = 1;
      PDims[0] = ds->GetNumberOfPoints();
      CRank = 1;
      CDims[0] = ds->GetNumberOfCells();
      if (reusing_topology)
      {
        // don't need to do all this again
        // @TODO : t->SetNodesPerElement(ppCell);
        break;
      }
      svtkXdmfWriterInternal::MapOfCellTypes cellTypes;
      svtkXdmfWriterInternal::DetermineCellTypes(svtkPointSet::SafeDownCast(ds), cellTypes);

      // TODO: When is it beneficial to take advantage of a homogeneous topology?
      // If no compelling reason not to used MIXED, then this should go away.
      // This special case code requires an in memory copy just to get rid of
      // each cell's preceding number of points int.
      // If don't have to do that, could use pointer sharing,
      // and the extra code path is bound to cause problems eventually.
      if (cellTypes.size() == 1)
      {
        // cerr << "Homogeneous topology" << endl;
        t->SetNumberOfElements(ds->GetNumberOfCells());
        const svtkXdmfWriterInternal::CellType* ct = &cellTypes.begin()->first;
        svtkIdType ppCell = ct->NumPoints;
        switch (ct->SVTKType)
        {
          case SVTK_VERTEX:
          case SVTK_POLY_VERTEX:
            t->SetTopologyType(XDMF_POLYVERTEX);
            break;
          case SVTK_LINE:
          case SVTK_POLY_LINE:
            t->SetTopologyType(XDMF_POLYLINE);
            t->SetNodesPerElement(ppCell);
            break;
          case SVTK_TRIANGLE:
          case SVTK_TRIANGLE_STRIP:
            t->SetTopologyType(XDMF_TRI);
            break;
          case SVTK_POLYGON:
            t->SetTopologyType(XDMF_POLYGON);
            t->SetNodesPerElement(ppCell);
            break;
          case SVTK_PIXEL:
          case SVTK_QUAD:
            t->SetTopologyType(XDMF_QUAD);
            break;
          case SVTK_TETRA:
            t->SetTopologyType(XDMF_TET);
            break;
          case SVTK_VOXEL:
          case SVTK_HEXAHEDRON:
            t->SetTopologyType(XDMF_HEX);
            break;
          case SVTK_WEDGE:
            t->SetTopologyType(XDMF_WEDGE);
            break;
          case SVTK_PYRAMID:
            t->SetTopologyType(XDMF_PYRAMID);
            break;
          case SVTK_EMPTY_CELL:
          default:
            t->SetTopologyType(XDMF_NOTOPOLOGY);
            break;
        }
        XdmfArray* di = t->GetConnectivity();
        di->SetHeavyDataSetName(heavyName);
        if (SVTK_SIZEOF_ID_TYPE == sizeof(XDMF_64_INT))
        {
          di->SetNumberType(XDMF_INT64_TYPE);
        }
        else
        {
          di->SetNumberType(XDMF_INT32_TYPE);
        }

        XdmfInt64 hDim[2];
        hDim[0] = ds->GetNumberOfCells();
        hDim[1] = ppCell;
        di->SetShape(2, hDim);
        svtkIdList* il = cellTypes[*ct];
        svtkIdList* cellPoints = svtkIdList::New();
        svtkIdType cvnt = 0;
        for (svtkIdType i = 0; i < ds->GetNumberOfCells(); i++)
        {
          ds->GetCellPoints(il->GetId(i), cellPoints);
          if (ct->SVTKType == SVTK_VOXEL)
          {
            // Hack for SVTK_VOXEL
            di->SetValue(cvnt++, (svtkXdmfIdType)cellPoints->GetId(0));
            di->SetValue(cvnt++, (svtkXdmfIdType)cellPoints->GetId(1));
            di->SetValue(cvnt++, (svtkXdmfIdType)cellPoints->GetId(3));
            di->SetValue(cvnt++, (svtkXdmfIdType)cellPoints->GetId(2));
            di->SetValue(cvnt++, (svtkXdmfIdType)cellPoints->GetId(4));
            di->SetValue(cvnt++, (svtkXdmfIdType)cellPoints->GetId(5));
            di->SetValue(cvnt++, (svtkXdmfIdType)cellPoints->GetId(7));
            di->SetValue(cvnt++, (svtkXdmfIdType)cellPoints->GetId(6));
          }
          else if (ct->SVTKType == SVTK_PIXEL)
          {
            // Hack for SVTK_PIXEL
            di->SetValue(cvnt++, (svtkXdmfIdType)cellPoints->GetId(0));
            di->SetValue(cvnt++, (svtkXdmfIdType)cellPoints->GetId(1));
            di->SetValue(cvnt++, (svtkXdmfIdType)cellPoints->GetId(3));
            di->SetValue(cvnt++, (svtkXdmfIdType)cellPoints->GetId(2));
          }
          else
          {
            for (svtkIdType j = 0; j < ppCell; j++)
            {
              di->SetValue(cvnt++, (svtkXdmfIdType)cellPoints->GetId(j));
            }
          } // pd has 4 arrays, so it is rarely homogeoneous
        }
        cellPoints->Delete();
      } // homogeneous
      else
      {
        // cerr << "Nonhomogeneous topology" << endl;
        // Non Homogeneous, used mixed topology type to dump them all
        t->SetTopologyType(XDMF_MIXED);
        svtkIdType numCells = ds->GetNumberOfCells();
        t->SetNumberOfElements(numCells);
        XdmfArray* di = t->GetConnectivity();
        di->SetHeavyDataSetName(heavyName);
        if (SVTK_SIZEOF_ID_TYPE == sizeof(XDMF_64_INT))
        {
          di->SetNumberType(XDMF_INT64_TYPE);
        }
        else
        {
          di->SetNumberType(XDMF_INT32_TYPE);
        }
        svtkIdTypeArray* da = svtkIdTypeArray::New();
        da->SetNumberOfComponents(1);
        svtkUnstructuredGrid* ugrid = svtkUnstructuredGrid::SafeDownCast(ds);
        const int ESTIMATE = 4; /*celltype+numids+id0+id1 or celtype+id0+id1+id2*/
        auto countConnSize = [](svtkCellArray* ca) {
          return ca->GetNumberOfCells() + ca->GetNumberOfConnectivityIds();
        };
        if (ugrid)
        {
          da->Allocate(countConnSize(ugrid->GetCells()) * ESTIMATE);
        }
        else
        {
          svtkPolyData* pd = svtkPolyData::SafeDownCast(ds);
          const svtkIdType sizev = countConnSize(pd->GetVerts());
          const svtkIdType sizel = countConnSize(pd->GetLines());
          const svtkIdType sizep = countConnSize(pd->GetPolys());
          const svtkIdType sizes = countConnSize(pd->GetStrips());
          const svtkIdType rtotal = sizev + sizel + sizep + sizes;
          da->Allocate(rtotal * ESTIMATE);
        }

        svtkIdType cntr = 0;
        for (svtkIdType cid = 0; cid < numCells; cid++)
        {
          svtkCell* cell = ds->GetCell(cid);
          svtkIdType cellType = ds->GetCellType(cid);
          svtkIdType numPts = cell->GetNumberOfPoints();
          switch (cellType)
          {
            case SVTK_VERTEX:
            case SVTK_POLY_VERTEX:
              da->InsertValue(cntr++, XDMF_POLYVERTEX);
              da->InsertValue(cntr++, numPts);
              break;
            case SVTK_LINE:
            case SVTK_POLY_LINE:
              da->InsertValue(cntr++, XDMF_POLYLINE);
              da->InsertValue(cntr++, cell->GetNumberOfPoints());
              break;
            // case SVTK_TRIANGLE_STRIP :
            // TODO: Split tri strips into triangles
            // t->SetTopologyType(XDMF_TRI);
            // break;
            case SVTK_TRIANGLE:
              da->InsertValue(cntr++, XDMF_TRI);
              break;
            case SVTK_POLYGON:
              da->InsertValue(cntr++, XDMF_POLYGON);
              da->InsertValue(cntr++, cell->GetNumberOfPoints());
              break;
            case SVTK_PIXEL:
            case SVTK_QUAD:
              da->InsertValue(cntr++, XDMF_POLYGON);
              break;
            case SVTK_TETRA:
              da->InsertValue(cntr++, XDMF_TET);
              break;
            case SVTK_VOXEL:
              da->InsertValue(cntr++, XDMF_HEX);
              break;
            case SVTK_HEXAHEDRON:
              da->InsertValue(cntr++, XDMF_HEX);
              break;
            case SVTK_WEDGE:
              da->InsertValue(cntr++, XDMF_WEDGE);
              break;
            case SVTK_PYRAMID:
              da->InsertValue(cntr++, XDMF_PYRAMID);
              break;
            default:
              da->InsertValue(cntr++, XDMF_NOTOPOLOGY);
              break;
          }
          if (cellType == SVTK_VOXEL)
          {
            // Hack for SVTK_VOXEL
            da->InsertValue(cntr++, cell->GetPointId(0));
            da->InsertValue(cntr++, cell->GetPointId(1));
            da->InsertValue(cntr++, cell->GetPointId(3));
            da->InsertValue(cntr++, cell->GetPointId(2));
            da->InsertValue(cntr++, cell->GetPointId(4));
            da->InsertValue(cntr++, cell->GetPointId(5));
            da->InsertValue(cntr++, cell->GetPointId(7));
            da->InsertValue(cntr++, cell->GetPointId(6));
          }
          else if (cellType == SVTK_PIXEL)
          {
            // Hack for SVTK_PIXEL
            da->InsertValue(cntr++, cell->GetPointId(0));
            da->InsertValue(cntr++, cell->GetPointId(1));
            da->InsertValue(cntr++, cell->GetPointId(3));
            da->InsertValue(cntr++, cell->GetPointId(2));
          }
          for (svtkIdType pid = 0; pid < numPts; pid++)
          {
            da->InsertValue(cntr++, cell->GetPointId(pid));
          }
        }
        this->ConvertVToXArray(da, di, 1, &cntr, 2, heavyName);
        da->Delete();
      }
    }
    break;
    default:
      t->SetTopologyType(XDMF_NOTOPOLOGY);
      svtkWarningMacro(<< "Unrecognized dataset type");
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmfWriter::CreateGeometry(svtkDataSet* ds, xdmf2::XdmfGrid* grid, void* staticdata)
{
  // Geometry
  XdmfGeometry* geo = grid->GetGeometry();
  geo->SetLightDataLimit(this->LightDataLimit);

  const char* heavyName = nullptr;
  std::string heavyDataSetName;
  if (this->HeavyDataFileName)
  {
    heavyDataSetName = std::string(this->HeavyDataFileName) + ":";
    if (this->MeshStaticOverTime)
    {
      std::stringstream hdf5group;
      hdf5group << "/Geometry_";
      if (this->CurrentBlockIndex >= 0)
      {
        if (grid->GetName())
        {
          hdf5group << grid->GetName();
        }
        else
        {
          hdf5group << "Block_" << this->CurrentBlockIndex;
        }
        heavyDataSetName = heavyDataSetName + hdf5group.str();
      }
    }
    else
    {
      if (this->HeavyDataGroupName)
      {
        heavyDataSetName = heavyDataSetName + HeavyDataGroupName + "/Geometry";
      }
    }
    heavyName = heavyDataSetName.c_str();
  }

  svtkXW2NodeHelp* staticnode = (svtkXW2NodeHelp*)staticdata;
  if (staticnode)
  {
    if (staticnode->staticFlag)
    {
      grid->Set("GeometryConstant", "True");
    }
    if (staticnode->DOM && staticnode->node)
    {
      XdmfXmlNode staticGeom = staticnode->DOM->FindElement("Geometry", 0, staticnode->node);
      XdmfConstString text = staticnode->DOM->Serialize(staticGeom->children);
      geo->SetDataXml(text);
      return 1;
    }
  }

  if (this->MeshStaticOverTime)
  {
    if (this->CurrentTimeIndex == 0)
    {
      // Save current geometry node at t0 for next time steps
      this->GeometryAtT0.push_back(geo);
    }
    else if (static_cast<int>(this->TopologyAtT0.size()) > this->CurrentBlockIndex)
    {
      // Get geometry node at t0
      XdmfGeometry* geo0 = this->GeometryAtT0[this->CurrentBlockIndex];
      // Setup current geometry node with t0 properties
      geo->SetGeometryTypeFromString(geo0->GetGeometryTypeAsString());
      // Setup points data XML according t0 one
      this->SetupDataArrayXML(geo, geo0->GetPoints());
      return 1;
    }
  }

  switch (ds->GetDataObjectType())
  {
    case SVTK_STRUCTURED_POINTS:
    case SVTK_IMAGE_DATA:
    case SVTK_UNIFORM_GRID:
    {
      geo->SetGeometryType(XDMF_GEOMETRY_ORIGIN_DXDYDZ);
      svtkImageData* id = svtkImageData::SafeDownCast(ds);
      double orig[3], spacing[3];
      id->GetOrigin(orig);
      double tmp = orig[2];
      orig[2] = orig[0];
      orig[0] = tmp;
      id->GetSpacing(spacing);
      tmp = spacing[2];
      spacing[2] = spacing[0];
      spacing[0] = tmp;
      geo->SetOrigin(orig);
      geo->SetDxDyDz(spacing);
    }
    break;
    case SVTK_RECTILINEAR_GRID:
    {
      svtkIdType len;
      geo->SetGeometryType(XDMF_GEOMETRY_VXVYVZ);
      svtkRectilinearGrid* rgrid = svtkRectilinearGrid::SafeDownCast(ds);
      svtkDataArray* da;
      da = rgrid->GetXCoordinates();
      len = da->GetNumberOfTuples();
      XdmfArray* xdax = new XdmfArray;
      this->ConvertVToXArray(da, xdax, 1, &len, 0, heavyName);
      geo->SetVectorX(xdax, 1);
      da = rgrid->GetYCoordinates();
      len = da->GetNumberOfTuples();
      XdmfArray* xday = new XdmfArray;
      this->ConvertVToXArray(da, xday, 1, &len, 0, heavyName);
      geo->SetVectorY(xday, 1);
      da = rgrid->GetZCoordinates();
      len = da->GetNumberOfTuples();
      XdmfArray* xdaz = new XdmfArray;
      this->ConvertVToXArray(da, xdaz, 1, &len, 0, heavyName);
      geo->SetVectorZ(xdaz, 1);
    }
    break;
    case SVTK_STRUCTURED_GRID:
    case SVTK_POLY_DATA:
    case SVTK_UNSTRUCTURED_GRID:
    {
      geo->SetGeometryType(XDMF_GEOMETRY_XYZ);
      svtkPointSet* pset = svtkPointSet::SafeDownCast(ds);
      svtkPoints* pts = pset->GetPoints();
      if (!pts)
      {
        return 0;
      }
      svtkDataArray* da = pts->GetData();
      XdmfArray* xda = geo->GetPoints();
      svtkIdType shape[2];
      shape[0] = da->GetNumberOfTuples();
      this->ConvertVToXArray(da, xda, 1, shape, 0, heavyName);
      geo->SetPoints(xda);
    }
    break;
    default:
      geo->SetGeometryType(XDMF_GEOMETRY_NONE);
      // TODO: Support non-canonical svtkDataSets (via a callout for extensibility)
      svtkWarningMacro(<< "Unrecognized dataset type");
  }

  return 1;
}

//------------------------------------------------------------------------------
int svtkXdmfWriter::WriteAtomicDataSet(svtkDataObject* dobj, xdmf2::XdmfGrid* grid)
{
  // cerr << "Writing " << dobj << " a " << dobj->GetClassName() << endl;
  svtkDataSet* ds = svtkDataSet::SafeDownCast(dobj);
  if (!ds)
  {
    // TODO: Fill in non Vis data types
    svtkWarningMacro(<< "Can not convert " << dobj->GetClassName() << " to XDMF yet.");
    return 0;
  }

  this->DOM->SetWorkingDirectory(this->WorkingDirectory.c_str());

  // Attributes
  svtkIdType FRank = 1;
  svtkIdType FDims[1];
  svtkIdType CRank = 3;
  svtkIdType CDims[3];
  svtkIdType PRank = 3;
  svtkIdType PDims[3];

  // We need to force a data and group name for supporting still mesh over time
  // otherwise names are generated when the data is dumped in HDF5: too late
  // because we need the name to reuse it when building the tree.
  std::string hdf5name = this->BaseFileName + ".h5";
  this->SetHeavyDataFileName(hdf5name.c_str());

  std::stringstream hdf5group;
  hdf5group << "/";
  if (this->CurrentBlockIndex >= 0)
  {
    if (grid->GetName())
    {
      hdf5group << grid->GetName();
    }
    else
    {
      hdf5group << "Block_" << this->CurrentBlockIndex;
    }
  }
  hdf5group << "_t" << setw(6) << setfill('0') << this->CurrentTime;
  hdf5group << ends;
  this->SetHeavyDataGroupName(hdf5group.str().c_str());

  this->CreateTopology(ds, grid, PDims, CDims, PRank, CRank, nullptr);
  if (!this->CreateGeometry(ds, grid, nullptr))
  {
    return 0;
  }

  FDims[0] = ds->GetFieldData()->GetNumberOfTuples();
  this->WriteArrays(ds->GetFieldData(), grid, XDMF_ATTRIBUTE_CENTER_GRID, FRank, FDims, "Field");
  this->WriteArrays(ds->GetCellData(), grid, XDMF_ATTRIBUTE_CENTER_CELL, CRank, CDims, "Cell");
  this->WriteArrays(ds->GetPointData(), grid, XDMF_ATTRIBUTE_CENTER_NODE, PRank, PDims, "Node");

  this->CurrentBlockIndex++;

  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmfWriter::WriteArrays(svtkFieldData* fd, xdmf2::XdmfGrid* grid, int association,
  svtkIdType rank, svtkIdType* dims, const char* name)
{
  if (!fd)
  {
    return 0;
  }
  svtkDataSetAttributes* dsa = svtkDataSetAttributes::SafeDownCast(fd);

  const char* heavyName = nullptr;
  std::string heavyDataSetName;
  if (this->HeavyDataFileName)
  {
    heavyDataSetName = std::string(this->HeavyDataFileName) + ":";
    if (this->HeavyDataGroupName)
    {
      heavyDataSetName = heavyDataSetName + std::string(HeavyDataGroupName) + "/" + name;
    }
    heavyName = heavyDataSetName.c_str();
  }

  //
  // Sort alphabetically to avoid potential bad ordering problems
  //
  int nbOfArrays = fd->GetNumberOfArrays();
  std::vector<std::pair<int, std::string> > attributeNames;
  attributeNames.reserve(nbOfArrays);
  for (int i = 0; i < nbOfArrays; i++)
  {
    svtkAbstractArray* scalars = fd->GetAbstractArray(i);
    attributeNames.push_back(std::pair<int, std::string>(i, scalars->GetName()));
  }
  std::sort(attributeNames.begin(), attributeNames.end());

  for (int i = 0; i < nbOfArrays; i++)
  {
    svtkDataArray* da = fd->GetArray(attributeNames[i].second.c_str());
    if (!da)
    {
      // TODO: Dump non numeric arrays too
      svtkWarningMacro(<< "xdmfwriter can not convert non-numeric arrays yet.");
      continue;
    }

    XdmfAttribute* attr = new XdmfAttribute;
    attr->SetLightDataLimit(this->LightDataLimit);
    attr->SetDeleteOnGridDelete(true);
    if (da->GetName())
    {
      attr->SetName(da->GetName());
    }
    else
    {
      attr->SetName("ANONYMOUS");
    }
    attr->SetAttributeCenter(association);

    int attributeType = 0;
    if (dsa)
    {
      attributeType = dsa->IsArrayAnAttribute(attributeNames[i].first);
      switch (attributeType)
      {
        case svtkDataSetAttributes::SCALARS:
          attributeType =
            XDMF_ATTRIBUTE_TYPE_SCALAR; // TODO: Is XDMF ok with 3 component(RGB) active scalars?
          break;
        case svtkDataSetAttributes::VECTORS:
          attributeType = XDMF_ATTRIBUTE_TYPE_VECTOR;
          break;
        case svtkDataSetAttributes::GLOBALIDS:
          attributeType = XDMF_ATTRIBUTE_TYPE_GLOBALID;
          break;
        case svtkDataSetAttributes::TENSORS: // TODO: svtk tensors are 9 component, xdmf tensors are
                                            // 6?
        case svtkDataSetAttributes::NORMALS: // TODO: mark as vectors?
        case svtkDataSetAttributes::TCOORDS: // TODO: mark as vectors?
        case svtkDataSetAttributes::PEDIGREEIDS: // TODO: ? type is variable
        default:
          break;
      }
    }

    if (attributeType != 0)
    {
      attr->SetActive(1);
      attr->SetAttributeType(attributeType);
    }
    else
    {
      // svtk doesn't mark it as a special array, use width to tell xdmf what to call it
      if (da->GetNumberOfComponents() == 1)
      {
        attr->SetAttributeType(XDMF_ATTRIBUTE_TYPE_SCALAR);
      }
      else if (da->GetNumberOfComponents() == 3)
      {
        attr->SetAttributeType(XDMF_ATTRIBUTE_TYPE_VECTOR);
      }
      else if (da->GetNumberOfComponents() == 6)
      {
        // TODO: convert SVTK 9 components symmetric tensors to 6 components
        attr->SetAttributeType(XDMF_ATTRIBUTE_TYPE_TENSOR);
      }
    }

    XdmfArray* xda = attr->GetValues();
    this->ConvertVToXArray(da, xda, rank, dims, 0, heavyName);
    attr->SetValues(xda);
    grid->Insert(attr);
  }

  return 1;
}

//------------------------------------------------------------------------------
void svtkXdmfWriter::ConvertVToXArray(svtkDataArray* vda, XdmfArray* xda, svtkIdType rank,
  svtkIdType* dims, int allocStrategy, const char* heavyprefix)
{
  XdmfInt32 lRank = rank;
  std::vector<XdmfInt64> lDims(rank + 1);
  for (svtkIdType i = 0; i < rank; i++)
  {
    lDims[i] = dims[i];
  }
  svtkIdType nc = vda->GetNumberOfComponents();
  // add additional dimension to the xdmf array to match the svtk arrays width,
  // ex coordinate arrays have xyz, so add [3]
  if (nc != 1)
  {
    lDims[rank] = nc;
    lRank += 1;
  }

  switch (vda->GetDataType())
  {
    case SVTK_DOUBLE:
      xda->SetNumberType(XDMF_FLOAT64_TYPE);
      break;
    case SVTK_FLOAT:
      xda->SetNumberType(XDMF_FLOAT32_TYPE);
      break;
    case SVTK_ID_TYPE:
      xda->SetNumberType(
        (SVTK_SIZEOF_ID_TYPE == sizeof(XDMF_64_INT) ? XDMF_INT64_TYPE : XDMF_INT32_TYPE));
      break;
    case SVTK_LONG:
      xda->SetNumberType(XDMF_INT64_TYPE);
      break;
    case SVTK_INT:
      xda->SetNumberType(XDMF_INT32_TYPE);
      break;
    case SVTK_UNSIGNED_INT:
      xda->SetNumberType(XDMF_UINT32_TYPE);
      break;
    case SVTK_SHORT:
      xda->SetNumberType(XDMF_INT16_TYPE);
      break;
    case SVTK_UNSIGNED_SHORT:
      xda->SetNumberType(XDMF_INT16_TYPE);
      break;
    case SVTK_CHAR:
    case SVTK_SIGNED_CHAR:
      xda->SetNumberType(XDMF_INT8_TYPE); // TODO: Do we ever want unicode?
      break;
    case SVTK_UNSIGNED_CHAR:
      xda->SetNumberType(XDMF_UINT8_TYPE);
      break;
    case SVTK_LONG_LONG:
    case SVTK_UNSIGNED_LONG_LONG:
#if !defined(SVTK_LEGACY_REMOVE)
    case SVTK___INT64:
    case SVTK_UNSIGNED___INT64:
#endif
    case SVTK_UNSIGNED_LONG:
    case SVTK_STRING:
    {
      xda->SetNumberType(XDMF_UNKNOWN_TYPE);
      break;
    }
  }

  if (heavyprefix)
  {
    std::string name;
    if (vda->GetName())
    {
      name = vda->GetName();
    }
    else
    {
      std::stringstream ss;
      ss << "DataArray" << this->UnlabelledDataArrayId++;
      name = ss.str();
    }
    std::string dsname = std::string(heavyprefix) + "/" + name;
    xda->SetHeavyDataSetName(dsname.c_str());
  }

  // TODO: if we can make xdmf write out immediately, then wouldn't have to keep around
  // arrays when working with temporal data
  if ((allocStrategy == 0 && !this->TopTemporalGrid) || allocStrategy == 1)
  {
    // Do not let xdmf allocate its own buffer. xdmf just borrows svtk's and doesn't double mem size.
    xda->SetAllowAllocate(0);
    xda->SetShape(lRank, lDims.data());
    xda->SetDataPointer(vda->GetVoidPointer(0));
  }
  else //(allocStrategy==0 && this->TopTemporalGrid) || allocStrategy==2)
  {
    // Unfortunately data doesn't stick around with temporal updates, which is exactly when you want
    // it most.
    xda->SetAllowAllocate(1);
    xda->SetShape(lRank, lDims.data());
    memcpy(xda->GetDataPointer(), vda->GetVoidPointer(0),
      vda->GetNumberOfTuples() * vda->GetNumberOfComponents() * vda->GetElementComponentSize());
  }
}
