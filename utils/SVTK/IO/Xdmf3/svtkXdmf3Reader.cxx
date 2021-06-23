/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3Reader.cxx
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkXdmf3Reader.h"

#include "svtkDataObjectTreeIterator.h"
#include "svtkDataObjectTypes.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkMultiProcessController.h"
#include "svtkMutableDirectedGraph.h"
#include "svtkObjectFactory.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkTimerLog.h"
#include "svtkXdmf3ArrayKeeper.h"
#include "svtkXdmf3ArraySelection.h"
#include "svtkXdmf3DataSet.h"
#include "svtkXdmf3HeavyDataHandler.h"
#include "svtkXdmf3LightDataHandler.h"
#include "svtkXdmf3SILBuilder.h"
#include "svtksys/SystemTools.hxx"

#include "svtk_xdmf3.h"
#include SVTKXDMF3_HEADER(XdmfCurvilinearGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfDomain.hpp)
#include SVTKXDMF3_HEADER(XdmfGridCollection.hpp)
#include SVTKXDMF3_HEADER(XdmfGridCollectionType.hpp)
#include SVTKXDMF3_HEADER(XdmfReader.hpp)
#include SVTKXDMF3_HEADER(XdmfRectilinearGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfRegularGrid.hpp)
#include SVTKXDMF3_HEADER(XdmfUnstructuredGrid.hpp)

#include <algorithm>

// TODO: implement fast and approximate CanReadFile
// TODO: read from buffer, allowing for xincludes
// TODO: strided access to structured data
// TODO: when too many grids for SIL, allow selection of top level grids
// TODO: break structured data into pieces
// TODO: make domains entirely optional and selectable

//=============================================================================
class svtkXdmf3Reader::Internals
{
  // Private implementation details for svtkXdmf3Reader
public:
  Internals()
  {
    this->FieldArrays = new svtkXdmf3ArraySelection;
    this->CellArrays = new svtkXdmf3ArraySelection;
    this->PointArrays = new svtkXdmf3ArraySelection;
    this->GridsCache = new svtkXdmf3ArraySelection;
    this->SetsCache = new svtkXdmf3ArraySelection;

    this->SILBuilder = new svtkXdmf3SILBuilder();
    this->SILBuilder->Initialize();

    this->Keeper = new svtkXdmf3ArrayKeeper;
  };

  ~Internals()
  {
    delete this->FieldArrays;
    delete this->CellArrays;
    delete this->PointArrays;
    delete this->GridsCache;
    delete this->SetsCache;
    delete this->SILBuilder;
    this->ReleaseArrays(true);
    this->FileNames.clear();
  };

  //--------------------------------------------------------------------------
  bool PrepareDocument(svtkXdmf3Reader* self, const char* fileName, bool AsTime)
  {
    if (this->Domain)
    {
      return true;
    }

    if (!fileName)
    {
      svtkErrorWithObjectMacro(self, "File name not set");
      return false;
    }
    if (!svtksys::SystemTools::FileExists(fileName))
    {
      svtkErrorWithObjectMacro(self, "Error opening file " << fileName);
      return false;
    }
    if (!this->Domain)
    {
      this->Init(fileName, AsTime);
    }
    return true;
  }

  //--------------------------------------------------------------------------
  svtkGraph* GetSIL() { return this->SILBuilder->SIL; }

  //--------------------------------------------------------------------------
  // find out what kind of svtkdataobject the xdmf file will produce
  int GetSVTKType()
  {
    if (this->SVTKType != -1)
    {
      return this->SVTKType;
    }
    unsigned int nGridCollections = this->Domain->getNumberGridCollections();
    if (nGridCollections > 1)
    {
      this->SVTKType = SVTK_MULTIBLOCK_DATA_SET;
      return this->SVTKType;
    }

    // check for temporal of atomic, in which case we produce the atomic type
    shared_ptr<XdmfDomain> toCheck = this->Domain;
    bool temporal = false;
    if (nGridCollections == 1)
    {
      shared_ptr<XdmfGridCollection> gc = this->Domain->getGridCollection(0);
      if (gc->getType() == XdmfGridCollectionType::Temporal())
      {
        if (gc->getNumberGridCollections() == 0)
        {
          temporal = true;
          toCheck = gc;
        }
      }
    }

    unsigned int nUnstructuredGrids = toCheck->getNumberUnstructuredGrids();
    unsigned int nRectilinearGrids = toCheck->getNumberRectilinearGrids();
    unsigned int nCurvilinearGrids = toCheck->getNumberCurvilinearGrids();
    unsigned int nRegularGrids = toCheck->getNumberRegularGrids();
    unsigned int nGraphs = toCheck->getNumberGraphs();
    int numtypes = 0;
    numtypes = numtypes + (nUnstructuredGrids > 0 ? 1 : 0);
    numtypes = numtypes + (nRectilinearGrids > 0 ? 1 : 0);
    numtypes = numtypes + (nCurvilinearGrids > 0 ? 1 : 0);
    numtypes = numtypes + (nRegularGrids > 0 ? 1 : 0);
    numtypes = numtypes + (nGraphs > 0 ? 1 : 0);
    bool atomic = temporal ||
      (numtypes == 1 &&
        (nUnstructuredGrids == 1 || nRectilinearGrids == 1 || nCurvilinearGrids == 1 ||
          nRegularGrids == 1 || nGraphs == 1));
    if (!atomic)
    {
      this->SVTKType = SVTK_MULTIBLOCK_DATA_SET;
    }
    else
    {
      this->SVTKType = SVTK_UNIFORM_GRID;
      // keep a reference to get extent from
      this->TopGrid = toCheck->getRegularGrid(0);
      if (nRectilinearGrids > 0)
      {
        this->SVTKType = SVTK_RECTILINEAR_GRID;
        // keep a reference to get extent from
        this->TopGrid = toCheck->getRectilinearGrid(0);
      }
      else if (nCurvilinearGrids > 0)
      {
        this->SVTKType = SVTK_STRUCTURED_GRID;
        // keep a reference to get extent from
        this->TopGrid = toCheck->getCurvilinearGrid(0);
      }
      else if (nUnstructuredGrids > 0)
      {
        this->SVTKType = SVTK_UNSTRUCTURED_GRID;
        this->TopGrid = toCheck->getUnstructuredGrid(0);
      }
      else if (nGraphs > 0)
      {
        // SVTK_MUTABLE_DIRECTED_GRAPH more specifically
        this->SVTKType = SVTK_DIRECTED_GRAPH;
      }
    }
    if (this->TopGrid)
    {
      shared_ptr<XdmfGrid> grid = shared_dynamic_cast<XdmfGrid>(this->TopGrid);
      if (grid && grid->getNumberSets() > 0)
      {
        this->SVTKType = SVTK_MULTIBLOCK_DATA_SET;
      }
    }
    return this->SVTKType;
  }

  //--------------------------------------------------------------------------
  void ReadHeavyData(unsigned int updatePiece, unsigned int updateNumPieces, bool doTime,
    double time, svtkMultiBlockDataSet* mbds, bool AsTime)
  {
    // traverse the xdmf hierarchy, and convert and return what was requested
    shared_ptr<svtkXdmf3HeavyDataHandler> visitor = svtkXdmf3HeavyDataHandler::New(this->FieldArrays,
      this->CellArrays, this->PointArrays, this->GridsCache, this->SetsCache, updatePiece,
      updateNumPieces, doTime, time, this->Keeper, AsTime);
    visitor->Populate(this->Domain, mbds);
  }

  //--------------------------------------------------------------------------
  svtkMultiPieceDataSet* Flatten(svtkMultiBlockDataSet* mbds);

  //--------------------------------------------------------------------------
  void ReleaseArrays(bool force = false)
  {
    if (!this->Keeper)
    {
      return;
    }
    this->Keeper->Release(force);
  }

  //--------------------------------------------------------------------------
  void BumpKeeper()
  {
    if (!this->Keeper)
    {
      return;
    }
    this->Keeper->BumpGeneration();
  }

  svtkXdmf3ArraySelection* FieldArrays;
  svtkXdmf3ArraySelection* CellArrays;
  svtkXdmf3ArraySelection* PointArrays;
  svtkXdmf3ArraySelection* GridsCache;
  svtkXdmf3ArraySelection* SetsCache;
  std::vector<double> TimeSteps;
  shared_ptr<XdmfItem> TopGrid;
  svtkXdmf3ArrayKeeper* Keeper;

  std::vector<std::string> FileNames;

private:
  //--------------------------------------------------------------------------
  void Init(const char* filename, bool AsTime)
  {
    svtkTimerLog::MarkStartEvent("X3R::Init");
    unsigned int idx = static_cast<unsigned int>(this->FileNames.size());

    this->Reader = XdmfReader::New();

    unsigned int updatePiece = 0;
    unsigned int updateNumPieces = 1;
    svtkMultiProcessController* ctrl = svtkMultiProcessController::GetGlobalController();
    if (ctrl != nullptr)
    {
      updatePiece = ctrl->GetLocalProcessId();
      updateNumPieces = ctrl->GetNumberOfProcesses();
    }
    else
    {
      updatePiece = 0;
      updateNumPieces = 1;
    }

    if (idx == 1)
    {
      this->Domain = shared_dynamic_cast<XdmfDomain>(this->Reader->read(filename));
    }
    else
    {
      this->Domain = XdmfDomain::New();
      shared_ptr<XdmfGridCollection> topc = XdmfGridCollection::New();
      if (AsTime)
      {
        topc->setType(XdmfGridCollectionType::Temporal());
      }
      this->Domain->insert(topc);
      for (unsigned int i = 0; i < idx; i++)
      {
        if (AsTime || (i % updateNumPieces == updatePiece))
        {
          // cerr << updatePiece << " reading " << this->FileNames[i] << endl;
          shared_ptr<XdmfDomain> fdomain =
            shared_dynamic_cast<XdmfDomain>(this->Reader->read(this->FileNames[i]));

          unsigned int j;
          for (j = 0; j < fdomain->getNumberGridCollections(); j++)
          {
            topc->insert(fdomain->getGridCollection(j));
          }
          for (j = 0; j < fdomain->getNumberUnstructuredGrids(); j++)
          {
            topc->insert(fdomain->getUnstructuredGrid(j));
          }
          for (j = 0; j < fdomain->getNumberRectilinearGrids(); j++)
          {
            topc->insert(fdomain->getRectilinearGrid(j));
          }
          for (j = 0; j < fdomain->getNumberCurvilinearGrids(); j++)
          {
            topc->insert(fdomain->getCurvilinearGrid(j));
          }
          for (j = 0; j < fdomain->getNumberRegularGrids(); j++)
          {
            topc->insert(fdomain->getRegularGrid(j));
          }
          for (j = 0; j < fdomain->getNumberGraphs(); j++)
          {
            topc->insert(fdomain->getGraph(j));
          }
        }
      }
    }

    this->SVTKType = -1;
    svtkTimerLog::MarkStartEvent("X3R::learn");
    this->GatherMetaInformation();
    svtkTimerLog::MarkEndEvent("X3R::learn");

    svtkTimerLog::MarkEndEvent("X3R::Init");
  }

  //--------------------------------------------------------------------------
  void GatherMetaInformation()
  {
    svtkTimerLog::MarkStartEvent("X3R::GatherMetaInfo");

    unsigned int updatePiece = 0;
    unsigned int updateNumPieces = 1;
    svtkMultiProcessController* ctrl = svtkMultiProcessController::GetGlobalController();
    if (ctrl != nullptr)
    {
      updatePiece = ctrl->GetLocalProcessId();
      updateNumPieces = ctrl->GetNumberOfProcesses();
    }
    else
    {
      updatePiece = 0;
      updateNumPieces = 1;
    }
    shared_ptr<svtkXdmf3LightDataHandler> visitor =
      svtkXdmf3LightDataHandler::New(this->SILBuilder, this->FieldArrays, this->CellArrays,
        this->PointArrays, this->GridsCache, this->SetsCache, updatePiece, updateNumPieces);
    visitor->InspectXDMF(this->Domain, -1);
    visitor->ClearGridsIfNeeded(this->Domain);
    if (this->TimeSteps.size())
    {
      this->TimeSteps.erase(this->TimeSteps.begin());
    }
    std::set<double> times = visitor->getTimes();
    std::set<double>::const_iterator it = times.begin();
    while (it != times.end())
    {
      this->TimeSteps.push_back(*it);
      ++it;
    }
    svtkTimerLog::MarkEndEvent("X3R::GatherMetaInfo");
  }

  int SVTKType;
  shared_ptr<XdmfReader> Reader;
  shared_ptr<XdmfDomain> Domain;
  svtkXdmf3SILBuilder* SILBuilder;
};

//==============================================================================

svtkStandardNewMacro(svtkXdmf3Reader);

//----------------------------------------------------------------------------
svtkXdmf3Reader::svtkXdmf3Reader()
{
  this->FileNameInternal = nullptr;

  this->Internal = new svtkXdmf3Reader::Internals();
  this->FileSeriesAsTime = true;

  this->FieldArraysCache = this->Internal->FieldArrays;
  this->CellArraysCache = this->Internal->CellArrays;
  this->PointArraysCache = this->Internal->PointArrays;
  this->SetsCache = this->Internal->SetsCache;
  this->GridsCache = this->Internal->GridsCache;

  this->SetNumberOfInputPorts(0);
}

//----------------------------------------------------------------------------
svtkXdmf3Reader::~svtkXdmf3Reader()
{

  this->SetFileName(nullptr);
  delete this->Internal;
  // XdmfHDF5Controller::closeFiles();
}

//----------------------------------------------------------------------------
void svtkXdmf3Reader::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileNameInternal ? this->FileNameInternal : "(none)")
     << endl;
  os << indent << "FileSeriesAsTime: " << (this->FileSeriesAsTime ? "True" : "False") << endl;
}

//----------------------------------------------------------------------------
void svtkXdmf3Reader::AddFileName(const char* filename)
{
  this->Internal->FileNames.push_back(filename);
  if (this->Internal->FileNames.size() == 1)
  {
    this->SetFileNameInternal(filename);
  }
}

//----------------------------------------------------------------------------
void svtkXdmf3Reader::SetFileName(const char* filename)
{
  this->RemoveAllFileNames();
  if (filename)
  {
    this->Internal->FileNames.push_back(filename);
  }
  this->SetFileNameInternal(filename);
}

//----------------------------------------------------------------------------
void svtkXdmf3Reader::RemoveAllFileNames()
{
  this->Internal->FileNames.clear();
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::CanReadFile(const char* filename)
{
  if (!svtksys::SystemTools::FileExists(filename))
  {
    return 0;
  }

  /*
   TODO: this, but really fast
   shared_ptr<XdmfReader> tester = XdmfReader::New();
   try {
     shared_ptr<XdmfItem> item = tester->read(filename);
   } catch (XdmfError & e) {
     return 0;
   }
  */

  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::FillOutputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
svtkTypeBool svtkXdmf3Reader::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObjectInternal(outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::RequestDataObjectInternal(svtkInformationVector* outputVector)
{
  svtkTimerLog::MarkStartEvent("X3R::RDO");
  // let libXdmf parse XML
  if (!this->Internal->PrepareDocument(this, this->FileNameInternal, this->FileSeriesAsTime))
  {
    svtkTimerLog::MarkEndEvent("X3R::RDO");
    return 0;
  }

  // Determine what svtkDataObject we should produce
  int svtk_type = this->Internal->GetSVTKType();

  // Make an empty svtkDataObject
  svtkDataObject* output = svtkDataObject::GetData(outputVector, 0);
  if (!output || output->GetDataObjectType() != svtk_type)
  {
    if (svtk_type == SVTK_DIRECTED_GRAPH)
    {
      output = svtkMutableDirectedGraph::New();
    }
    else
    {
      output = svtkDataObjectTypes::NewDataObject(svtk_type);
    }
    outputVector->GetInformationObject(0)->Set(svtkDataObject::DATA_OBJECT(), output);
    this->GetOutputPortInformation(0)->Set(
      svtkDataObject::DATA_EXTENT_TYPE(), output->GetExtentType());
    output->Delete();
  }

  svtkTimerLog::MarkEndEvent("X3R::RDO");
  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::RequestInformation(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  svtkTimerLog::MarkStartEvent("X3R::RI");
  if (!this->Internal->PrepareDocument(this, this->FileNameInternal, this->FileSeriesAsTime))
  {
    svtkTimerLog::MarkEndEvent("X3R::RI");
    return 0;
  }

  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Publish the fact that this reader can satisfy any piece request.
  outInfo->Set(CAN_HANDLE_PIECE_REQUEST(), 1);

  // Publish the SIL which provides information about the grid hierarchy.
  outInfo->Set(svtkDataObject::SIL(), this->Internal->GetSIL());

  // Publish the times that we have data for
  if (this->Internal->TimeSteps.size() > 0)
  {
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_STEPS(), &this->Internal->TimeSteps[0],
      static_cast<int>(this->Internal->TimeSteps.size()));
    double timeRange[2];
    timeRange[0] = this->Internal->TimeSteps.front();
    timeRange[1] = this->Internal->TimeSteps.back();
    outInfo->Set(svtkStreamingDemandDrivenPipeline::TIME_RANGE(), timeRange, 2);
  }

  // Structured atomic must announce the whole extent it can provide
  int svtk_type = this->Internal->GetSVTKType();
  if (svtk_type == SVTK_STRUCTURED_GRID || svtk_type == SVTK_RECTILINEAR_GRID ||
    svtk_type == SVTK_IMAGE_DATA || svtk_type == SVTK_UNIFORM_GRID)
  {
    int whole_extent[6];
    double origin[3];
    double spacing[3];
    whole_extent[0] = 0;
    whole_extent[1] = -1;
    whole_extent[2] = 0;
    whole_extent[3] = -1;
    whole_extent[4] = 0;
    whole_extent[5] = -1;
    origin[0] = 0.0;
    origin[1] = 0.0;
    origin[2] = 0.0;
    spacing[0] = 1.0;
    spacing[1] = 1.0;
    spacing[2] = 1.0;

    shared_ptr<XdmfRegularGrid> regGrid =
      shared_dynamic_cast<XdmfRegularGrid>(this->Internal->TopGrid);
    if (regGrid)
    {
      svtkImageData* dataSet = svtkImageData::New();
      svtkXdmf3DataSet::CopyShape(regGrid.get(), dataSet, this->Internal->Keeper);
      dataSet->GetExtent(whole_extent);
      dataSet->GetOrigin(origin);
      dataSet->GetSpacing(spacing);
      dataSet->Delete();
    }
    shared_ptr<XdmfRectilinearGrid> recGrid =
      shared_dynamic_cast<XdmfRectilinearGrid>(this->Internal->TopGrid);
    if (recGrid)
    {
      svtkRectilinearGrid* dataSet = svtkRectilinearGrid::New();
      svtkXdmf3DataSet::CopyShape(recGrid.get(), dataSet, this->Internal->Keeper);
      dataSet->GetExtent(whole_extent);
      dataSet->Delete();
    }
    shared_ptr<XdmfCurvilinearGrid> crvGrid =
      shared_dynamic_cast<XdmfCurvilinearGrid>(this->Internal->TopGrid);
    if (crvGrid)
    {
      svtkStructuredGrid* dataSet = svtkStructuredGrid::New();
      svtkXdmf3DataSet::CopyShape(crvGrid.get(), dataSet, this->Internal->Keeper);
      dataSet->GetExtent(whole_extent);
      dataSet->Delete();
    }

    outInfo->Set(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), whole_extent, 6);
    outInfo->Set(svtkDataObject::ORIGIN(), origin, 3);
    outInfo->Set(svtkDataObject::SPACING(), spacing, 3);
  }

  svtkTimerLog::MarkEndEvent("X3R::RI");
  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::RequestData(
  svtkInformation*, svtkInformationVector**, svtkInformationVector* outputVector)
{
  svtkTimerLog::MarkStartEvent("X3R::RD");

  if (!this->Internal->PrepareDocument(this, this->FileNameInternal, this->FileSeriesAsTime))
  {
    svtkTimerLog::MarkEndEvent("X3R::RD");
    return 0;
  }

  svtkTimerLog::MarkStartEvent("X3R::Release");
  this->Internal->ReleaseArrays();
  this->Internal->BumpKeeper();
  svtkTimerLog::MarkEndEvent("X3R::Release");

  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // Collect information about what spatial extent is requested.
  unsigned int updatePiece = 0;
  unsigned int updateNumPieces = 1;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()) &&
    outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()))
  {
    updatePiece = static_cast<unsigned int>(
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_PIECE_NUMBER()));
    updateNumPieces = static_cast<unsigned int>(
      outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_PIECES()));
  }
  /*
  int ghost_levels = 0;
  if (outInfo->Has(
      svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS()))
    {
    ghost_levels = outInfo->Get(
        svtkStreamingDemandDrivenPipeline::UPDATE_NUMBER_OF_GHOST_LEVELS());
    }
  */
  /*
  int update_extent[6] = {0, -1, 0, -1, 0, -1};
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT()))
    {
    outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_EXTENT(),
        update_extent);
    }
  */

  // Collect information about what temporal extent is requested.
  double time = 0.0;
  bool doTime = false;
  if (outInfo->Has(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP()) &&
    this->Internal->TimeSteps.size())
  {
    doTime = true;
    time = outInfo->Get(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP());
    // find the nearest match (floor), so we have something exact to search for
    std::vector<double>::iterator it =
      std::upper_bound(this->Internal->TimeSteps.begin(), this->Internal->TimeSteps.end(), time);
    if (it != this->Internal->TimeSteps.begin())
    {
      --it;
    }
    time = *it;
  }

  svtkDataObject* output = svtkDataObject::GetData(outInfo);
  if (!output)
  {
    return 0;
  }
  if (doTime)
  {
    output->GetInformation()->Set(svtkDataObject::DATA_TIME_STEP(), time);
  }

  svtkMultiBlockDataSet* mbds = svtkMultiBlockDataSet::New();
  this->Internal->ReadHeavyData(
    updatePiece, updateNumPieces, doTime, time, mbds, this->FileSeriesAsTime);

  if (mbds->GetNumberOfBlocks() == 1)
  {
    svtkMultiBlockDataSet* ibds = svtkMultiBlockDataSet::SafeDownCast(mbds->GetBlock(0));
    svtkMultiBlockDataSet* obds = svtkMultiBlockDataSet::SafeDownCast(output);
    if (!this->FileSeriesAsTime && ibds && obds)
    {
      svtkMultiPieceDataSet* mpds = this->Internal->Flatten(ibds);
      obds->SetBlock(0, mpds);
      mpds->Delete();
    }
    else
    {
      output->ShallowCopy(mbds->GetBlock(0));
    }
  }
  else
  {
    svtkMultiBlockDataSet* obds = svtkMultiBlockDataSet::SafeDownCast(output);
    if (!this->FileSeriesAsTime && obds)
    {
      svtkMultiPieceDataSet* mpds = this->Internal->Flatten(mbds);
      obds->SetBlock(0, mpds);
      mpds->Delete();
    }
    else
    {
      output->ShallowCopy(mbds);
    }
  }
  mbds->Delete();

  svtkTimerLog::MarkEndEvent("X3R::RD");

  return 1;
}

//----------------------------------------------------------------------------
svtkMultiPieceDataSet* svtkXdmf3Reader::Internals::Flatten(svtkMultiBlockDataSet* ibds)
{
  svtkDataObjectTreeIterator* it = ibds->NewTreeIterator();
  unsigned int i = 0;

  // found out how many pieces we have locally
  it->InitTraversal();
  it->VisitOnlyLeavesOn();
  while (!it->IsDoneWithTraversal())
  {
    it->GoToNextItem();
    i++;
  }

  // communicate to find out where mine should go
  int mylen = i;
  int* allLens;
  unsigned int procnum;
  unsigned int numProcs;
  svtkMultiProcessController* ctrl = svtkMultiProcessController::GetGlobalController();
  if (ctrl != nullptr)
  {
    procnum = ctrl->GetLocalProcessId();
    numProcs = ctrl->GetNumberOfProcesses();
    allLens = new int[numProcs];
    ctrl->AllGather(&mylen, allLens, 1);
  }
  else
  {
    procnum = 0;
    numProcs = 1;
    allLens = new int[1];
    allLens[0] = mylen;
  }
  unsigned int myStart = 0;
  unsigned int total = 0;
  for (i = 0; i < numProcs; i++)
  {
    if (i < procnum)
    {
      myStart += allLens[i];
    }
    total += allLens[i];
  }
  delete[] allLens;

  // cerr << "PROC " << procnum << " starts at " << myStart << endl;
  // zero out everyone else's
  svtkMultiPieceDataSet* mpds = svtkMultiPieceDataSet::New();
  for (i = 0; i < total; i++)
  {
    mpds->SetPiece(i++, nullptr);
  }

  // fill in my pieces
  it->GoToFirstItem();
  while (!it->IsDoneWithTraversal())
  {
    mpds->SetPiece(myStart++, it->GetCurrentDataObject());
    it->GoToNextItem();
  }

  it->Delete();

  return mpds; // caller must Delete
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::GetNumberOfFieldArrays()
{
  return this->GetFieldArraySelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void svtkXdmf3Reader::SetFieldArrayStatus(const char* arrayname, int status)
{
  this->GetFieldArraySelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::GetFieldArrayStatus(const char* arrayname)
{
  return this->GetFieldArraySelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* svtkXdmf3Reader::GetFieldArrayName(int index)
{
  return this->GetFieldArraySelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
svtkXdmf3ArraySelection* svtkXdmf3Reader::GetFieldArraySelection()
{
  return this->FieldArraysCache;
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::GetNumberOfCellArrays()
{
  return this->GetCellArraySelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void svtkXdmf3Reader::SetCellArrayStatus(const char* arrayname, int status)
{
  this->GetCellArraySelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::GetCellArrayStatus(const char* arrayname)
{
  return this->GetCellArraySelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* svtkXdmf3Reader::GetCellArrayName(int index)
{
  return this->GetCellArraySelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
svtkXdmf3ArraySelection* svtkXdmf3Reader::GetCellArraySelection()
{
  return this->CellArraysCache;
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::GetNumberOfPointArrays()
{
  return this->GetPointArraySelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void svtkXdmf3Reader::SetPointArrayStatus(const char* arrayname, int status)
{
  this->GetPointArraySelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::GetPointArrayStatus(const char* arrayname)
{
  return this->GetPointArraySelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* svtkXdmf3Reader::GetPointArrayName(int index)
{
  return this->GetPointArraySelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
svtkXdmf3ArraySelection* svtkXdmf3Reader::GetPointArraySelection()
{
  return this->PointArraysCache;
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::GetNumberOfGrids()
{
  return this->GetGridsSelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void svtkXdmf3Reader::SetGridStatus(const char* gridname, int status)
{
  this->GetGridsSelection()->SetArrayStatus(gridname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::GetGridStatus(const char* arrayname)
{
  return this->GetGridsSelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* svtkXdmf3Reader::GetGridName(int index)
{
  return this->GetGridsSelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
svtkXdmf3ArraySelection* svtkXdmf3Reader::GetGridsSelection()
{
  return this->GridsCache;
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::GetNumberOfSets()
{
  return this->GetSetsSelection()->GetNumberOfArrays();
}

//----------------------------------------------------------------------------
void svtkXdmf3Reader::SetSetStatus(const char* arrayname, int status)
{
  this->GetSetsSelection()->SetArrayStatus(arrayname, status != 0);
  this->Modified();
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::GetSetStatus(const char* arrayname)
{
  return this->GetSetsSelection()->GetArraySetting(arrayname);
}

//----------------------------------------------------------------------------
const char* svtkXdmf3Reader::GetSetName(int index)
{
  return this->GetSetsSelection()->GetArrayName(index);
}

//----------------------------------------------------------------------------
svtkXdmf3ArraySelection* svtkXdmf3Reader::GetSetsSelection()
{
  return this->SetsCache;
}

//----------------------------------------------------------------------------
svtkGraph* svtkXdmf3Reader::GetSIL()
{
  svtkGraph* ret = this->Internal->GetSIL();
  return ret;
}

//----------------------------------------------------------------------------
int svtkXdmf3Reader::GetSILUpdateStamp()
{
  return this->Internal->GetSIL()->GetMTime();
}
