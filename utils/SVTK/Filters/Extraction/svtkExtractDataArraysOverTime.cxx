/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractDataArraysOverTime.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExtractDataArraysOverTime.h"

#include "svtkArrayDispatch.h"
#include "svtkCharArray.h"
#include "svtkCompositeDataIterator.h"
#include "svtkDataArrayRange.h"
#include "svtkDataSet.h"
#include "svtkDataSetAttributes.h"
#include "svtkDescriptiveStatistics.h"
#include "svtkDoubleArray.h"
#include "svtkGenericCell.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOrderStatistics.h"
#include "svtkSmartPointer.h"
#include "svtkSplitColumnComponents.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkTable.h"
#include "svtkWeakPointer.h"

#include <algorithm>
#include <cassert>
#include <map>
#include <sstream>
#include <string>
#include <vector>

namespace
{
struct ClearInvalidElementsWorker
{
private:
  svtkCharArray* MaskArray;

public:
  ClearInvalidElementsWorker(svtkCharArray* maskArray)
    : MaskArray(maskArray)
  {
  }

  template <typename ArrayType>
  void operator()(ArrayType* svtkarray)
  {
    const auto mask = svtk::DataArrayValueRange<1>(this->MaskArray);
    auto data = svtk::DataArrayTupleRange(svtkarray);

    for (svtkIdType t = 0; t < data.size(); ++t)
    {
      if (mask[t] == 0)
      {
        data[t].fill(0);
      }
    }
  }
};
}

class svtkExtractDataArraysOverTime::svtkInternal
{
private:
  class svtkKey
  {
  public:
    unsigned int CompositeID;
    svtkIdType ID;

    svtkKey(svtkIdType id)
    {
      this->CompositeID = 0;
      this->ID = id;
    }
    svtkKey(unsigned int cid, svtkIdType id)
    {
      this->CompositeID = cid;
      this->ID = id;
    }

    bool operator<(const svtkKey& other) const
    {
      if (this->CompositeID == other.CompositeID)
      {
        return (this->ID < other.ID);
      }
      return (this->CompositeID < other.CompositeID);
    }
  };

public:
  class svtkValue
  {
  public:
    svtkSmartPointer<svtkTable> Output;
    svtkSmartPointer<svtkCharArray> ValidMaskArray;
    svtkSmartPointer<svtkDoubleArray> PointCoordinatesArray;
    bool UsingGlobalIDs;
    svtkValue()
      : UsingGlobalIDs(false)
    {
    }
  };

private:
  typedef std::map<svtkKey, svtkValue> MapType;
  MapType OutputGrids;
  int NumberOfTimeSteps;
  svtkWeakPointer<svtkExtractDataArraysOverTime> Self;
  // We use the same time array for all extracted time lines, since that doesn't
  // change.
  svtkSmartPointer<svtkDoubleArray> TimeArray;

  void AddTimeStepInternal(unsigned int cid, int ts_index, double time, svtkDataObject* data);

  /**
   * Runs stats filters to summarize the data and return
   * a new dataobject with the summary.
   */
  svtkSmartPointer<svtkDataObject> Summarize(svtkDataObject* data);

  svtkValue* GetOutput(const svtkKey& key, svtkDataSetAttributes* inDSA, bool using_gid);

  // For all arrays in dsa, for any element that's not valid (i.e. has value 1
  // in validArray), we initialize that element to 0 (rather than having some
  // garbage value).
  void RemoveInvalidPoints(svtkCharArray* validArray, svtkDataSetAttributes* dsa)
  {
    ClearInvalidElementsWorker worker(validArray);
    const auto narrays = dsa->GetNumberOfArrays();
    for (svtkIdType a = 0; a < narrays; a++)
    {
      if (svtkDataArray* da = dsa->GetArray(a))
      {
        if (!svtkArrayDispatch::Dispatch::Execute(da, worker))
        {
          // use svtkDataArray fallback.
          worker(da);
        }
      }
    }
  }

public:
  // Initializes the data structure.
  svtkInternal(int numTimeSteps, svtkExtractDataArraysOverTime* self)
    : NumberOfTimeSteps(numTimeSteps)
    , Self(self)
  {
    this->TimeArray = svtkSmartPointer<svtkDoubleArray>::New();
    this->TimeArray->SetNumberOfTuples(this->NumberOfTimeSteps);
    std::fill_n(
      this->TimeArray->WritePointer(0, this->NumberOfTimeSteps), this->NumberOfTimeSteps, 0.0);
    this->OutputGrids.clear();
  }

  void AddTimeStep(int ts_index, double time, svtkDataObject* data);

  // Collect the gathered timesteps into the output.
  void CollectTimesteps(svtkDataObject* input, svtkMultiBlockDataSet* mboutput)
  {
    assert(mboutput);

    mboutput->Initialize();

    // for now, let's not use blocknames. Seems like they are not consistent
    // across ranks currently. that makes it harder to merge blocks using
    // names in svtkPExtractDataArraysOverTime.
    (void)input;
#if 0
    auto mbinput = svtkCompositeDataSet::SafeDownCast(input);

    // build a datastructure to make block-name lookup fast.
    std::map<unsigned int, std::string> block_names;
    if (mbinput)
    {
      svtkCompositeDataIterator* iter = mbinput->NewIterator();
      for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
      {
        if (iter->HasCurrentMetaData() &&
          iter->GetCurrentMetaData()->Has(svtkCompositeDataSet::NAME()))
        {
          block_names[iter->GetCurrentFlatIndex()] =
            iter->GetCurrentMetaData()->Get(svtkCompositeDataSet::NAME());
        }
      }
      iter->Delete();
    }
#endif

    unsigned int cc = 0;
    for (auto& item : this->OutputGrids)
    {
      const svtkKey& key = item.first;
      const svtkValue& value = item.second;
      if (value.Output == nullptr)
      {
        continue;
      }
      auto outputRD = value.Output->GetRowData();

      svtkSmartPointer<svtkDataArray> originalIdsArray = nullptr;
      if (!this->Self->GetReportStatisticsOnly())
      {
        std::string originalIdsArrayName = "svtkOriginalCellIds";
        if (this->Self->GetFieldAssociation() == svtkDataObject::POINT)
        {
          originalIdsArrayName = "svtkOriginalPointIds";
        }
        originalIdsArray = outputRD->GetArray(originalIdsArrayName.c_str());
        // Remove svtkOriginalCellIds or svtkOriginalPointIds arrays which were added by
        // svtkExtractSelection.
        outputRD->RemoveArray(originalIdsArrayName.c_str());
      }

      outputRD->RemoveArray(value.ValidMaskArray->GetName());
      outputRD->AddArray(value.ValidMaskArray);
      if (value.PointCoordinatesArray)
      {
        outputRD->RemoveArray(value.PointCoordinatesArray->GetName());
        outputRD->AddArray(value.PointCoordinatesArray);
      }
      this->RemoveInvalidPoints(value.ValidMaskArray, outputRD);
      // note: don't add time array before the above step to avoid clearing
      // time values entirely.
      outputRD->RemoveArray(this->TimeArray->GetName());
      outputRD->AddArray(this->TimeArray);

      mboutput->SetBlock(cc, value.Output);

      // build a good name for the block.
      std::ostringstream stream;

      // add element id if not reporting stats.
      if (!this->Self->GetReportStatisticsOnly())
      {
        if (value.UsingGlobalIDs)
        {
          stream << "gid=" << key.ID;
        }
        else if (originalIdsArray)
        {
          stream << "originalId=" << originalIdsArray->GetTuple1(0);
        }
        else
        {
          stream << "id=" << key.ID;
        }
      }
      if (key.CompositeID != 0)
      {
        // for now, let's not use blocknames. Seems like they are not consistent
        // across ranks currently. that makes it harder to merge blocks using
        // names in svtkPExtractDataArraysOverTime.
#if 0
        auto iter = block_names.find(key.CompositeID);
        if (iter != block_names.end())
        {
          stream << " block=" << iter->second;
        }
        else
#endif
        {
          stream << " block=" << key.CompositeID;
        }
      }
      else if (stream.str().empty())
      {
        assert(this->Self->GetReportStatisticsOnly());
        stream << "stats";
      }
      mboutput->GetMetaData(cc)->Set(svtkCompositeDataSet::NAME(), stream.str().c_str());
      cc++;
    }
    this->OutputGrids.clear();
  }
};

//----------------------------------------------------------------------------
void svtkExtractDataArraysOverTime::svtkInternal::AddTimeStep(
  int ts_index, double time, svtkDataObject* data)
{
  this->TimeArray->SetTypedComponent(ts_index, 0, time);
  const int attributeType = this->Self->GetFieldAssociation();

  if (auto cd = svtkCompositeDataSet::SafeDownCast(data))
  {
    svtkCompositeDataIterator* iter = cd->NewIterator();
    for (iter->InitTraversal(); !iter->IsDoneWithTraversal(); iter->GoToNextItem())
    {
      if (auto block = iter->GetCurrentDataObject())
      {
        if (block->GetAttributesAsFieldData(attributeType) != nullptr)
        {
          this->AddTimeStepInternal(iter->GetCurrentFlatIndex(), ts_index, time, block);
        }
      }
    }
    iter->Delete();
  }
  else if (data)
  {
    if (data->GetAttributesAsFieldData(attributeType) != nullptr)
    {
      this->AddTimeStepInternal(0, ts_index, time, data);
    }
  }
}

//----------------------------------------------------------------------------
static void svtkExtractArraysAssignUniqueCoordNames(
  svtkDataSetAttributes* statInDSA, svtkDataArray* px, svtkDataArray* py, svtkDataArray* pz)
{
  std::string actualNames[3];
  actualNames[0] = "X";
  actualNames[1] = "Y";
  actualNames[2] = "Z";
  // We need to find unique but consistent names as close to
  // ("X","Y","Z") as possible, but that aren't in use.
  svtkAbstractArray* arrX;
  svtkAbstractArray* arrY;
  svtkAbstractArray* arrZ;
  int counter = 0;
  while ((arrX = statInDSA->GetArray(actualNames[0].c_str())) != nullptr &&
    (arrY = statInDSA->GetArray(actualNames[1].c_str())) != nullptr &&
    (arrZ = statInDSA->GetArray(actualNames[2].c_str())) != nullptr)
  {
    for (int i = 0; i < 3; ++i)
    {
      std::ostringstream os;
      os << "SelnCoords" << counter << "_" << (i ? (i > 1 ? "Z" : "Y") : "X");
      actualNames[i] = os.str();
    }
    ++counter;
  }
  px->SetName(actualNames[0].c_str());
  py->SetName(actualNames[1].c_str());
  pz->SetName(actualNames[2].c_str());
  statInDSA->AddArray(px);
  statInDSA->AddArray(py);
  statInDSA->AddArray(pz);
}

//------------------------------------------------------------------------------
static void svtkExtractArraysAddColumnValue(
  svtkTable* statSummary, const std::string& colName, int colType, const svtkVariant& val)
{
  std::string actualColumnName(colName);
  // We need to find a unique column name as close to colName that isn't taken.
  svtkAbstractArray* arr;
  int counter = 0;
  while ((arr = statSummary->GetColumnByName(actualColumnName.c_str())) != nullptr)
  {
    std::ostringstream os;
    os << colName << "_" << ++counter;
    actualColumnName = os.str();
  }
  arr = svtkAbstractArray::CreateArray(colType);
  arr->SetName(actualColumnName.c_str());
  arr->SetNumberOfTuples(1);
  arr->SetVariantValue(0, val);
  statSummary->AddColumn(arr);
  arr->FastDelete();
}

//------------------------------------------------------------------------------
svtkSmartPointer<svtkDataObject> svtkExtractDataArraysOverTime::svtkInternal::Summarize(
  svtkDataObject* input)
{
  assert(input != nullptr);

  const int attributeType = this->Self->GetFieldAssociation();
  svtkFieldData* inFD = input->GetAttributesAsFieldData(attributeType);
  assert(inFD != nullptr);

  const svtkIdType numIDs = inFD->GetNumberOfTuples();
  if (numIDs <= 0)
  {
    return nullptr;
  }

  // Make a svtkTable containing all fields plus possibly point coordinates.
  // We'll pass the table, after splitting multi-component arrays, to
  // svtkDescriptiveStatistics to get information about all the selected data at
  // this timestep.
  svtkNew<svtkTable> statInput;   // Input table created from input's attributes
  svtkNew<svtkTable> statSummary; // Reformatted statistics filter output
  svtkNew<svtkSplitColumnComponents> splitColumns;
  auto descrStats = this->Self->NewDescriptiveStatistics();
  auto orderStats = this->Self->NewOrderStatistics();
  descrStats->SetLearnOption(1);
  descrStats->SetDeriveOption(1);
  descrStats->SetAssessOption(0);
  orderStats->SetLearnOption(1);
  orderStats->SetDeriveOption(1);
  orderStats->SetAssessOption(0);

  svtkDataSetAttributes* statInDSA = statInput->GetRowData();
  statInDSA->ShallowCopy(inFD);
  // Add point coordinates to selected data if we are tracking point-data.
  if (attributeType == svtkDataObject::POINT)
  {
    svtkDataSet* ds = svtkDataSet::SafeDownCast(input);
    svtkNew<svtkDoubleArray> pX[3];
    for (int comp = 0; comp < 3; ++comp)
    {
      pX[comp]->SetNumberOfComponents(1);
      pX[comp]->SetNumberOfTuples(numIDs);
    }
    for (svtkIdType cc = 0; cc < numIDs; ++cc)
    {
      double* coords = ds->GetPoint(cc);
      for (int comp = 0; comp < 3; ++comp)
      {
        pX[comp]->SetValue(cc, coords[comp]);
      }
    }
    svtkExtractArraysAssignUniqueCoordNames(statInDSA, pX[0], pX[1], pX[2]);
  }
  splitColumns->SetInputDataObject(0, statInput);
  splitColumns->SetCalculateMagnitudes(1);
  splitColumns->Update();
  svtkTable* splits = splitColumns->GetOutput();
  descrStats->SetInputConnection(splitColumns->GetOutputPort());
  orderStats->SetInputConnection(splitColumns->GetOutputPort());
  // Add a column holding the number of points/cells/rows
  // in the data at this timestep.
  svtkExtractArraysAddColumnValue(statSummary, "N", SVTK_DOUBLE, numIDs);
  // Compute statistics 1 column at a time to save space (esp. for order stats)
  for (int i = 0; i < splits->GetNumberOfColumns(); ++i)
  {
    svtkAbstractArray* col = splits->GetColumn(i);
    int cType = col->GetDataType();
    const char* cname = col->GetName();
    orderStats->ResetRequests();
    orderStats->AddColumn(cname);
    orderStats->Update();
    svtkMultiBlockDataSet* order = svtkMultiBlockDataSet::SafeDownCast(
      orderStats->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));
    if (order && order->GetNumberOfBlocks() >= 3)
    {
      svtkTable* model = svtkTable::SafeDownCast(order->GetBlock(2));
      std::ostringstream minName;
      std::ostringstream medName;
      std::ostringstream maxName;
      std::ostringstream q1Name;
      std::ostringstream q3Name;
      minName << "min(" << cname << ")";
      q1Name << "q1(" << cname << ")";
      medName << "med(" << cname << ")";
      q3Name << "q3(" << cname << ")";
      maxName << "max(" << cname << ")";
      svtkExtractArraysAddColumnValue(statSummary, minName.str(), cType, model->GetValue(0, 1));
      svtkExtractArraysAddColumnValue(statSummary, q1Name.str(), cType, model->GetValue(1, 1));
      svtkExtractArraysAddColumnValue(statSummary, medName.str(), cType, model->GetValue(2, 1));
      svtkExtractArraysAddColumnValue(statSummary, q3Name.str(), cType, model->GetValue(3, 1));
      svtkExtractArraysAddColumnValue(statSummary, maxName.str(), cType, model->GetValue(4, 1));
    }
    if (svtkArrayDownCast<svtkDataArray>(col))
    {
      descrStats->ResetRequests();
      descrStats->AddColumn(cname);
      descrStats->Update();
      svtkMultiBlockDataSet* descr = svtkMultiBlockDataSet::SafeDownCast(
        descrStats->GetOutputDataObject(svtkStatisticsAlgorithm::OUTPUT_MODEL));
      if (descr && descr->GetNumberOfBlocks() >= 2)
      { // block 0: raw model; block 1: derived model
        svtkTable* rawModel = svtkTable::SafeDownCast(descr->GetBlock(0));
        svtkTable* drvModel = svtkTable::SafeDownCast(descr->GetBlock(1));
        std::ostringstream avgName;
        std::ostringstream stdName;
        avgName << "avg(" << cname << ")";
        stdName << "std(" << cname << ")";
        svtkExtractArraysAddColumnValue(
          statSummary, avgName.str(), SVTK_DOUBLE, rawModel->GetValueByName(0, "Mean"));
        svtkExtractArraysAddColumnValue(statSummary, stdName.str(), SVTK_DOUBLE,
          drvModel->GetValueByName(0, "Standard Deviation"));
      }
    }
  }

  svtkDataSetAttributes* statOutDSA = statSummary->GetRowData();
  auto table = svtkSmartPointer<svtkTable>::New();
  table->SetRowData(statOutDSA);
  return table;
}

//----------------------------------------------------------------------------
void svtkExtractDataArraysOverTime::svtkInternal::AddTimeStepInternal(
  unsigned int composite_index, int ts_index, double svtkNotUsed(time), svtkDataObject* input)
{
  int attributeType = this->Self->GetFieldAssociation();
  const bool statsOnly = this->Self->GetReportStatisticsOnly();

  svtkSmartPointer<svtkDataObject> data = input;
  if (statsOnly)
  {
    // instead of saving raw-data, we're going to track the summary.
    data = this->Summarize(input);
    attributeType = svtkDataObject::ROW;
  }

  if (!data)
  {
    return;
  }

  svtkDataSetAttributes* inDSA = data->GetAttributes(attributeType);
  const svtkIdType numIDs = inDSA->GetNumberOfTuples();
  if (numIDs <= 0)
  {
    return;
  }

  svtkIdTypeArray* indexArray = nullptr;
  if (!statsOnly)
  {
    if (this->Self->GetUseGlobalIDs())
    {
      indexArray = svtkIdTypeArray::SafeDownCast(inDSA->GetGlobalIds());
    }
    else
    {
      // when not reporting stats, user can specify which array to use to index
      // elements.
      int association;
      indexArray =
        svtkIdTypeArray::SafeDownCast(this->Self->GetInputArrayToProcess(0, data, association));
      if (indexArray != nullptr && association != attributeType)
      {
        indexArray = nullptr;
      }
    }
  }

  const bool is_gid = (indexArray != nullptr && inDSA->GetGlobalIds() == indexArray);
  if (is_gid)
  {
    // if using global ids, then they are expected to be unique across
    // blocks. By discarding the composite-index, we can easily track
    // elements moving between blocks.
    composite_index = 0;
  }

  svtkDataSet* dsData = svtkDataSet::SafeDownCast(data);
  for (svtkIdType cc = 0; cc < numIDs; ++cc)
  {
    const svtkIdType curid = indexArray ? indexArray->GetTypedComponent(cc, 0) : cc;
    const svtkKey key(composite_index, curid);

    // This will allocate a new svtkTable is none is present
    svtkValue* value = this->GetOutput(key, inDSA, is_gid);
    svtkTable* output = value->Output;
    output->GetRowData()->CopyData(inDSA, cc, ts_index);

    // Mark the entry valid.
    value->ValidMaskArray->SetTypedComponent(ts_index, 0, 1);

    // Record the point coordinate if we are tracking a point.
    if (value->PointCoordinatesArray && dsData)
    {
      double coords[3];
      dsData->GetPoint(cc, coords);
      value->PointCoordinatesArray->SetTypedTuple(ts_index, coords);
    }
  }
}

//----------------------------------------------------------------------------
svtkExtractDataArraysOverTime::svtkInternal::svtkValue*
svtkExtractDataArraysOverTime::svtkInternal::GetOutput(
  const svtkKey& key, svtkDataSetAttributes* inDSA, bool using_gid)
{
  MapType::iterator iter = this->OutputGrids.find(key);
  if (iter == this->OutputGrids.end())
  {
    svtkValue value;
    svtkTable* output = svtkTable::New();
    value.Output.TakeReference(output);

    svtkDataSetAttributes* rowData = output->GetRowData();
    rowData->CopyAllocate(inDSA, this->NumberOfTimeSteps);
    // since CopyAllocate only allocates memory, but doesn't change the number
    // of tuples in each of the arrays, we need to do this explicitly.
    // see (paraview/paraview#18090).
    rowData->SetNumberOfTuples(this->NumberOfTimeSteps);

    // Add an array to hold the time at each step
    svtkDoubleArray* timeArray = this->TimeArray;
    if (inDSA && inDSA->GetArray("Time"))
    {
      timeArray->SetName("TimeData");
    }
    else
    {
      timeArray->SetName("Time");
    }

    if (this->Self->GetFieldAssociation() == svtkDataObject::POINT &&
      this->Self->GetReportStatisticsOnly() == false)
    {
      // These are the point coordinates of the original data
      svtkDoubleArray* coordsArray = svtkDoubleArray::New();
      coordsArray->SetNumberOfComponents(3);
      coordsArray->SetNumberOfTuples(this->NumberOfTimeSteps);
      if (inDSA && inDSA->GetArray("Point Coordinates"))
      {
        coordsArray->SetName("Points");
      }
      else
      {
        coordsArray->SetName("Point Coordinates");
      }
      std::fill_n(coordsArray->WritePointer(0, 3 * this->NumberOfTimeSteps),
        3 * this->NumberOfTimeSteps, 0.0);
      value.PointCoordinatesArray.TakeReference(coordsArray);
    }

    // This array is used to make particular samples as invalid.
    // This happens when we are looking at a location which is not contained
    // by a cell or at a cell or point id that is destroyed.
    // It is used in the parallel subclass as well.
    svtkCharArray* validPts = svtkCharArray::New();
    validPts->SetName("svtkValidPointMask");
    validPts->SetNumberOfComponents(1);
    validPts->SetNumberOfTuples(this->NumberOfTimeSteps);
    std::fill_n(validPts->WritePointer(0, this->NumberOfTimeSteps), this->NumberOfTimeSteps,
      static_cast<char>(0));
    value.ValidMaskArray.TakeReference(validPts);
    value.UsingGlobalIDs = using_gid;
    iter = this->OutputGrids.insert(MapType::value_type(key, value)).first;
  }
  else
  {
    if (iter->second.UsingGlobalIDs != using_gid)
    {
      // global id indication is mismatched over time. Should that ever happen?
      // Not sure.
    }
  }

  return &iter->second;
}

//****************************************************************************
svtkStandardNewMacro(svtkExtractDataArraysOverTime);
//----------------------------------------------------------------------------
svtkExtractDataArraysOverTime::svtkExtractDataArraysOverTime()
  : CurrentTimeIndex(0)
  , NumberOfTimeSteps(0)
  , FieldAssociation(svtkDataObject::POINT)
  , ReportStatisticsOnly(false)
  , UseGlobalIDs(true)
  , Error(svtkExtractDataArraysOverTime::NoError)
  , Internal(nullptr)
{
  this->SetNumberOfInputPorts(1);
  // set to something that we know will never select that array (as
  // we want the user to explicitly set it).
  this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_NONE, "-invalid-array-");
}

//----------------------------------------------------------------------------
svtkExtractDataArraysOverTime::~svtkExtractDataArraysOverTime()
{
  delete this->Internal;
}

//----------------------------------------------------------------------------
void svtkExtractDataArraysOverTime::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "FieldAssociation: " << this->FieldAssociation << endl;
  os << indent << "ReportStatisticsOnly: " << this->ReportStatisticsOnly << endl;
  os << indent << "UseGlobalIDs: " << this->UseGlobalIDs << endl;
  os << indent << "NumberOfTimeSteps: " << this->NumberOfTimeSteps << endl;
}

//----------------------------------------------------------------------------
int svtkExtractDataArraysOverTime::FillInputPortInformation(int, svtkInformation* info)
{
  // We can handle composite datasets.
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataObject");
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractDataArraysOverTime::RequestInformation(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  if (inInfo->Has(svtkStreamingDemandDrivenPipeline::TIME_STEPS()))
  {
    this->NumberOfTimeSteps = inInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  }
  else
  {
    this->NumberOfTimeSteps = 0;
  }

  // The output of this filter does not contain a specific time, rather
  // it contains a collection of time steps. Also, this filter does not
  // respond to time requests. Therefore, we remove all time information
  // from the output.
  svtkInformation* outInfo = outputVector->GetInformationObject(0);
  outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  outInfo->Remove(svtkStreamingDemandDrivenPipeline::TIME_RANGE());
  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractDataArraysOverTime::RequestUpdateExtent(svtkInformation*,
  svtkInformationVector** inputVector, svtkInformationVector* svtkNotUsed(outputVector))
{
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);

  // get the requested update extent
  const double* inTimes = inInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (inTimes && this->CurrentTimeIndex >= 0)
  {
    assert(inInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS()) > this->CurrentTimeIndex);
    double timeReq = inTimes[this->CurrentTimeIndex];
    inInfo->Set(svtkStreamingDemandDrivenPipeline::UPDATE_TIME_STEP(), timeReq);
  }

  return 1;
}

//----------------------------------------------------------------------------
int svtkExtractDataArraysOverTime::RequestData(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  if (this->NumberOfTimeSteps <= 0)
  {
    svtkErrorMacro("No time steps in input data!");
    return 0;
  }

  if (this->FieldAssociation == svtkDataObject::FIELD ||
    this->FieldAssociation == svtkDataObject::POINT_THEN_CELL || this->FieldAssociation < 0 ||
    this->FieldAssociation >= svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES)
  {
    svtkErrorMacro("Unsupported FieldAssociation '" << this->FieldAssociation << "'.");
    return 0;
  }

  // is this the first request?
  if (this->Internal == nullptr)
  {
    this->Internal = new svtkInternal(this->NumberOfTimeSteps, this);
    this->Error = svtkExtractDataArraysOverTime::NoError;
    this->CurrentTimeIndex = 0;

    // Tell the pipeline to start looping.
    request->Set(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING(), 1);
  }

  assert(this->Internal);

  svtkDataObject* input = svtkDataObject::GetData(inputVector[0], 0);
  const double time_step = input->GetInformation()->Get(svtkDataObject::DATA_TIME_STEP());
  this->Internal->AddTimeStep(this->CurrentTimeIndex, time_step, input);
  this->UpdateProgress(static_cast<double>(this->CurrentTimeIndex) / this->NumberOfTimeSteps);

  // increment the time index
  this->CurrentTimeIndex++;
  if (this->CurrentTimeIndex == this->NumberOfTimeSteps)
  {
    this->PostExecute(request, inputVector, outputVector);
    delete this->Internal;
    this->Internal = nullptr;
  }

  return 1;
}

//----------------------------------------------------------------------------
void svtkExtractDataArraysOverTime::PostExecute(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // Tell the pipeline to stop looping.
  request->Remove(svtkStreamingDemandDrivenPipeline::CONTINUE_EXECUTING());
  this->CurrentTimeIndex = 0;
  this->Internal->CollectTimesteps(
    svtkDataObject::GetData(inputVector[0], 0), svtkMultiBlockDataSet::GetData(outputVector, 0));
}

//----------------------------------------------------------------------------
svtkSmartPointer<svtkDescriptiveStatistics> svtkExtractDataArraysOverTime::NewDescriptiveStatistics()
{
  return svtkSmartPointer<svtkDescriptiveStatistics>::New();
}

//----------------------------------------------------------------------------
svtkSmartPointer<svtkOrderStatistics> svtkExtractDataArraysOverTime::NewOrderStatistics()
{
  return svtkSmartPointer<svtkOrderStatistics>::New();
}
