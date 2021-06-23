/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3Writer.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkXdmf3Writer.h"

#include "svtkDataObject.h"
#include "svtkDirectedGraph.h"
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkObjectFactory.h"
#include "svtkPointSet.h"
#include "svtkRectilinearGrid.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"
#include "svtkXdmf3DataSet.h"

// clang-format off
#include "svtk_xdmf3.h"
#include SVTKXDMF3_HEADER(XdmfDomain.hpp)
#include SVTKXDMF3_HEADER(XdmfGridCollection.hpp)
#include SVTKXDMF3_HEADER(XdmfGridCollectionType.hpp)
#include SVTKXDMF3_HEADER(core/XdmfHeavyDataWriter.hpp)
#include SVTKXDMF3_HEADER(core/XdmfWriter.hpp)
// clang-format on

#include <stack>
#include <string>

svtkObjectFactoryNewMacro(svtkXdmf3Writer);

//=============================================================================
class svtkXdmf3Writer::Internals
{
public:
  Internals() {}
  ~Internals() {}
  void Init()
  {
    this->NumberOfTimeSteps = 1;
    this->CurrentTimeIndex = 0;

    this->Domain = XdmfDomain::New();
    this->Writer = boost::shared_ptr<XdmfWriter>();
    this->AggregateDomain = boost::shared_ptr<XdmfDomain>();
    this->AggregateWriter = boost::shared_ptr<XdmfWriter>();
    this->DestinationGroups.push(this->Domain);
    this->Destination = this->DestinationGroups.top();
  }
  void InitWriterName(const char* filename, unsigned int lightDataLimit)
  {
    this->Writer = XdmfWriter::New(filename);
    this->Writer->setLightDataLimit(lightDataLimit);
    this->Writer->getHeavyDataWriter()->setReleaseData(true);
  }
  void SwitchToTemporal()
  {
    shared_ptr<XdmfGridCollection> dest = XdmfGridCollection::New();
    dest->setType(XdmfGridCollectionType::Temporal());
    this->DestinationGroups.push(dest);
    this->Destination = this->DestinationGroups.top();
    this->Domain->insert(dest);
  }
  void WriteDataObject(svtkDataObject* dataSet, bool hasTime, double time, const char* name = 0)
  {
    if (!dataSet)
    {
      return;
    }
    switch (dataSet->GetDataObjectType())
    {
      case SVTK_MULTIBLOCK_DATA_SET:
      {
        shared_ptr<XdmfGridCollection> group = XdmfGridCollection::New();
        this->Destination->insert(group);
        this->DestinationGroups.push(group);
        this->Destination = this->DestinationGroups.top();
        svtkMultiBlockDataSet* mbds = svtkMultiBlockDataSet::SafeDownCast(dataSet);
        for (unsigned int i = 0; i < mbds->GetNumberOfBlocks(); i++)
        {
          svtkDataObject* next = mbds->GetBlock(i);
          const char* blockName = mbds->GetMetaData(i)->Get(svtkCompositeDataSet::NAME());
          this->WriteDataObject(next, hasTime, time, blockName);
          this->Domain->accept(this->Writer);
        }
        this->DestinationGroups.pop();
        this->Destination = this->DestinationGroups.top();
        break;
      }
      case SVTK_STRUCTURED_POINTS:
      case SVTK_IMAGE_DATA:
      case SVTK_UNIFORM_GRID:
      {
        svtkXdmf3DataSet::SVTKToXdmf(
          svtkImageData::SafeDownCast(dataSet), this->Destination.get(), hasTime, time, name);
        break;
      }
      case SVTK_RECTILINEAR_GRID:
      { // SVTK_RECTILINEAR_GRID is 3
        svtkXdmf3DataSet::SVTKToXdmf(
          svtkRectilinearGrid::SafeDownCast(dataSet), this->Destination.get(), hasTime, time, name);
        break;
      }
      case SVTK_STRUCTURED_GRID:
      {
        svtkXdmf3DataSet::SVTKToXdmf(
          svtkStructuredGrid::SafeDownCast(dataSet), this->Destination.get(), hasTime, time, name);
        break;
      }
      case SVTK_POLY_DATA:
      case SVTK_UNSTRUCTURED_GRID:
      {
        svtkXdmf3DataSet::SVTKToXdmf(
          svtkPointSet::SafeDownCast(dataSet), this->Destination.get(), hasTime, time, name);
        break;
      }
      // case SVTK_GRAPH:
      case SVTK_DIRECTED_GRAPH:
        // case SVTK_UNDIRECTED_GRAPH:
        {
          svtkXdmf3DataSet::SVTKToXdmf(
            svtkDirectedGraph::SafeDownCast(dataSet), this->Destination.get(), hasTime, time, name);
          break;
        }
      default:
      {
      }
    }

    // this->Domain->accept(this->Writer);
  }

  boost::shared_ptr<XdmfDomain> Domain;
  boost::shared_ptr<XdmfDomain> Destination;
  boost::shared_ptr<XdmfWriter> Writer;
  boost::shared_ptr<XdmfDomain> AggregateDomain;
  boost::shared_ptr<XdmfWriter> AggregateWriter;
  std::stack<boost::shared_ptr<XdmfDomain> > DestinationGroups;

  int NumberOfTimeSteps;
  int CurrentTimeIndex;
};

//==============================================================================

//----------------------------------------------------------------------------
svtkXdmf3Writer::svtkXdmf3Writer()
{
  this->FileName = nullptr;
  this->LightDataLimit = 100;
  this->WriteAllTimeSteps = false;
  this->TimeValues = nullptr;
  this->TimeValues = 0;
  this->InitWriters = true;

  this->Internal = new Internals();
  this->SetNumberOfOutputPorts(0);
}

//----------------------------------------------------------------------------
svtkXdmf3Writer::~svtkXdmf3Writer()
{
  this->SetFileName(nullptr);

  if (this->TimeValues)
  {
    this->TimeValues->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkXdmf3Writer::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FileName: " << (this->FileName ? this->FileName : "(none)") << endl;
  os << indent << "LightDataLimit: " << this->LightDataLimit << endl;
  os << indent << "WriteAllTimeSteps: " << (this->WriteAllTimeSteps ? "ON" : "OFF") << endl;
}

//------------------------------------------------------------------------------
void svtkXdmf3Writer::SetInputData(svtkDataObject* input)
{
  this->SetInputDataInternal(0, input);
}

//------------------------------------------------------------------------------
int svtkXdmf3Writer::Write()
{
  // Make sure we have input.
  if (this->GetNumberOfInputConnections(0) < 1)
  {
    svtkErrorMacro("No input provided!");
    return 0;
  }

  // always write even if the data hasn't changed
  this->Modified();

  if (!this->Internal)
  {
    this->Internal = new Internals();
  }
  this->Internal->Init();

  this->Update();

  delete this->Internal;
  this->Internal = nullptr;

  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmf3Writer::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{

  // Does the input have timesteps?
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->Internal->NumberOfTimeSteps =
      inInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->Internal->NumberOfTimeSteps = 1;
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmf3Writer::RequestUpdateExtent(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{

  // get the requested update extent
  if (!this->TimeValues)
  {
    this->TimeValues = svtkDoubleArray::New();
    svtkInformation* info = inputVector[0]->GetInformationObject(0);
    double* data = info->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    int len = info->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
    this->TimeValues->SetNumberOfValues(len);
    if (data)
    {
      for (int i = 0; i < len; i++)
      {
        this->TimeValues->SetValue(i, data[i]);
      }
    }
  }
  if (this->TimeValues && this->WriteAllTimeSteps)
  {
    // TODO:? Add a user ivar to specify a particular time,
    // which is different from current time. Can do it by updating
    // to a particular time then writing without writealltimesteps,
    // but that is annoying.
    if (this->TimeValues->GetPointer(0))
    {
      double timeReq;
      timeReq = this->TimeValues->GetValue(this->Internal->CurrentTimeIndex);
      inputVector[0]->GetInformationObject(0)->Set(
        svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmf3Writer::RequestData(svtkInformation* request, svtkInformationVector** inputVector,
  svtkInformationVector* svtkNotUsed(outputVector))
{
  // Note: call Write() instead of RD() directly.
  // Write() does setup first before it calls RD().
  if (!this->Internal->Domain)
  {
    return 1;
  }

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  this->OriginalInput = svtkDataObject::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  this->WriteDataInternal(request);
  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmf3Writer::GlobalContinueExecuting(int localContinueExecution)
{
  return localContinueExecution;
}

//----------------------------------------------------------------------------
void svtkXdmf3Writer::WriteDataInternal(svtkInformation* request)
{
  bool isTemporal = false;
  bool firstTimeStep = false;
  if (this->WriteAllTimeSteps && this->Internal->NumberOfTimeSteps > 1)
  {
    isTemporal = true;
  }
  if (isTemporal && this->Internal->CurrentTimeIndex == 0)
  {
    firstTimeStep = true;
    // Tell the pipeline to start looping.
    this->Internal->SwitchToTemporal();
    request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }

  svtkInformation* inDataInfo = this->OriginalInput->GetInformation();
  double dataT = 0;
  bool hasTime = false;
  if (inDataInfo->Has(svtkDataObject::DATA_TIME_STEP()))
  {
    dataT = this->OriginalInput->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP());
    hasTime = true;
  }
  this->CheckParameters();
  std::string testString(this->FileName);
  size_t tempLength = testString.length();
  testString = testString.substr(0, (tempLength - 4));
  std::string choppedFileName = testString;
  if (this->InitWriters == true)
  {
    if (this->NumberOfProcesses == 1)
    {
      this->Internal->InitWriterName(this->FileName, this->LightDataLimit);
    }
    else
    {
      if (this->MyRank == 0)
      {
        this->Internal->AggregateDomain = XdmfDomain::New();
        this->Internal->AggregateWriter = XdmfWriter::New(this->FileName);
        this->Internal->AggregateWriter->setLightDataLimit(this->LightDataLimit);
        this->Internal->AggregateWriter->getHeavyDataWriter()->setReleaseData(true);
      } // end if(this->MyRank == 0)
      std::string rankFileName = choppedFileName;
      rankFileName.append(".");
      rankFileName.append(std::to_string(this->NumberOfProcesses));
      rankFileName.append(".");
      rankFileName.append(std::to_string(this->MyRank));
      rankFileName.append(".xmf");
      this->Internal->InitWriterName(rankFileName.c_str(), this->LightDataLimit);
    }
    this->InitWriters = false;
  }
  this->Internal->WriteDataObject(this->OriginalInput, hasTime, dataT);
  this->Internal->Domain->accept(this->Internal->Writer);
  int rankCount;
  if (this->NumberOfProcesses > 1 && this->MyRank == 0 && (!isTemporal || firstTimeStep))
  {
    // write the root's top level meta file that refers to the satellites'
    // todo would be fancier to write out whole tree but xgrid to each
    // satellites contents. That would require gather calls to
    // determine how many leafs on each satellite and a rewrite.

    // XdmfGridCollections (aka XGrid) are xdmf3's way to cross reference
    shared_ptr<XdmfGridCollection> aggregategroup = XdmfGridCollection::New();
    aggregategroup->setType(XdmfGridCollectionType::Spatial());

    // the structure is simple, one cross reference per top in each satellite
    for (rankCount = 0; rankCount < this->NumberOfProcesses; rankCount++)
    {
      std::string rankFileName = choppedFileName;
      rankFileName.append(".");
      rankFileName.append(std::to_string(this->NumberOfProcesses));
      rankFileName.append(".");
      rankFileName.append(std::to_string(rankCount));
      rankFileName.append(".xmf");
      std::string rankGridName = "/Xdmf/Domain/Grid[1]";

      shared_ptr<XdmfGridController> partController =
        XdmfGridController::New(rankFileName.c_str(), rankGridName.c_str());

      // tricky part is we have to state what type we are referencing to.
      // otherwise readback fails.

      if (isTemporal)
      {
        shared_ptr<XdmfGridCollection> grid = XdmfGridCollection::New();
        grid->setType(XdmfGridCollectionType::Temporal());
        grid->setGridController(partController);
        aggregategroup->insert(grid);
        continue;
      }

      switch (this->OriginalInput->GetDataObjectType())
      {
        case SVTK_STRUCTURED_POINTS:
        case SVTK_IMAGE_DATA:
        case SVTK_UNIFORM_GRID:
        {
          // todo: the content below is a don't care placeholder.
          // the specifics don't matter, only the grid type,
          // but libxdmf won't let you not specify with structured
          shared_ptr<XdmfRegularGrid> grid = XdmfRegularGrid::New(1, 1, 1, 0, 0, 0, 0.0, 0.0, 0.0);
          grid->setGridController(partController);
          aggregategroup->insert(grid);
          break;
        }
        case SVTK_RECTILINEAR_GRID:
        {
          // todo: same as above
          shared_ptr<XdmfArray> xXCoords = XdmfArray::New();
          shared_ptr<XdmfArray> xYCoords = XdmfArray::New();
          shared_ptr<XdmfArray> xZCoords = XdmfArray::New();
          shared_ptr<XdmfRectilinearGrid> grid =
            XdmfRectilinearGrid::New(xXCoords, xYCoords, xZCoords);
          grid->setGridController(partController);
          aggregategroup->insert(grid);
          break;
        }
        case SVTK_STRUCTURED_GRID:
        {
          // todo: same as above
          shared_ptr<XdmfArray> xdims = XdmfArray::New();
          shared_ptr<XdmfCurvilinearGrid> grid = XdmfCurvilinearGrid::New(xdims);
          grid->setGridController(partController);
          aggregategroup->insert(grid);
          break;
        }
        case SVTK_POLY_DATA:
        case SVTK_UNSTRUCTURED_GRID:
        {
          shared_ptr<XdmfUnstructuredGrid> grid = XdmfUnstructuredGrid::New();
          grid->setGridController(partController);
          aggregategroup->insert(grid);
          break;
        }
        // case SVTK_GRAPH:
        case SVTK_DIRECTED_GRAPH:
          // case SVTK_UNDIRECTED_GRAPH:
          {
            // todo: graph can't have a grid controller
            //  shared_ptr<XdmfGraph> grid = XdmfGraph::New(0);
            //  //grid->setGridController(partController);
            //  aggregategroup->insert(grid);
            break;
          }
        default:
        {
          shared_ptr<XdmfGridCollection> grid = XdmfGridCollection::New();
          grid->setType(XdmfGridCollectionType::Spatial());
          grid->setGridController(partController);
          aggregategroup->insert(grid);
          break;
        }
      } // switch data object type
    }   // foreach rank
    this->Internal->AggregateDomain->insert(aggregategroup);
    this->Internal->AggregateDomain->accept(this->Internal->AggregateWriter);
  } // if need to write top level file

  this->Internal->CurrentTimeIndex++;
  if (this->Internal->CurrentTimeIndex >= this->Internal->NumberOfTimeSteps &&
    this->WriteAllTimeSteps)
  {
    // Tell the pipeline to stop looping.
    request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 0);
    this->Internal->CurrentTimeIndex = 0;
  }

  int localContinue = request->Get(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
  if (this->GlobalContinueExecuting(localContinue) != localContinue)
  {
    // Some other node decided to stop the execution.
    assert(localContinue == 1);
    request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 0);
  }
}

//----------------------------------------------------------------------------
int svtkXdmf3Writer::CheckParametersInternal(int _NumberOfProcesses, int _MyRank)
{
  if (!this->FileName)
  {
    svtkErrorMacro("No filename specified.");
    return 0;
  }

  this->NumberOfProcesses = _NumberOfProcesses;
  this->MyRank = _MyRank;

  return 1;
}

//----------------------------------------------------------------------------
int svtkXdmf3Writer::CheckParameters()
{
  return this->CheckParametersInternal(1, 0);
}
