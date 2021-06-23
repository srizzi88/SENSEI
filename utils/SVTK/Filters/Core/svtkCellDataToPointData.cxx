/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCellDataToPointData.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
#include "svtkCellDataToPointData.h"

#include "svtkArrayDispatch.h"
#include "svtkCell.h"
#include "svtkCellData.h"
#include "svtkDataArrayRange.h"
#include "svtkDataSet.h"
#include "svtkIdList.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkSmartPointer.h"
#include "svtkStructuredGrid.h"
#include "svtkUniformGrid.h"
#include "svtkUnsignedIntArray.h"

#include <algorithm>
#include <functional>
#include <set>

#define SVTK_MAX_CELLS_PER_POINT 4096

svtkStandardNewMacro(svtkCellDataToPointData);

namespace
{

//----------------------------------------------------------------------------
// Helper template function that implement the major part of the algorighm
// which will be expanded by the svtkTemplateMacro. The template function is
// provided so that coverage test can cover this function.
struct Spread
{
  template <typename SrcArrayT, typename DstArrayT>
  void operator()(SrcArrayT* const srcarray, DstArrayT* const dstarray,
                  svtkDataSet* const src, svtkUnsignedIntArray* const num,
                  svtkIdType ncells, svtkIdType npoints, svtkIdType ncomps,
                  int highestCellDimension, int contributingCellOption) const
  {
    // Both arrays will have the same value type:
    using T = svtk::GetAPIType<SrcArrayT>;

    // zero initialization
    std::fill_n(svtk::DataArrayValueRange(dstarray).begin(),
                npoints*ncomps, T(0));

    const auto srcTuples = svtk::DataArrayTupleRange(srcarray);
    auto dstTuples = svtk::DataArrayTupleRange(dstarray);

    // accumulate
    if (contributingCellOption != svtkCellDataToPointData::Patch)
    {
      for (svtkIdType cid = 0; cid < ncells; ++cid)
      {
        svtkCell* cell = src->GetCell(cid);
        if (cell->GetCellDimension() >= highestCellDimension)
        {
          const auto srcTuple = srcTuples[cid];
          svtkIdList* pids = cell->GetPointIds();
          for (svtkIdType i = 0, I = pids->GetNumberOfIds(); i < I; ++i)
          {
            const svtkIdType ptId = pids->GetId(i);
            auto dstTuple = dstTuples[ptId];
            // accumulate cell data to point data <==> point_data += cell_data
            std::transform(srcTuple.cbegin(),
                           srcTuple.cend(),
                           dstTuple.cbegin(),
                           dstTuple.begin(),
                           std::plus<T>());
          }
        }
      }
      // average
      for (svtkIdType pid = 0; pid < npoints; ++pid)
      {
        // guard against divide by zero
        if (unsigned int const denom = num->GetValue(pid))
        {
          // divide point data by the number of cells using it <==>
          // point_data /= denum
          auto dstTuple = dstTuples[pid];
          std::transform(dstTuple.cbegin(),
                         dstTuple.cend(),
                         dstTuple.begin(),
                         std::bind(std::divides<T>(), std::placeholders::_1, denom));
        }
      }
    }
    else
    { // compute over cell patches
      svtkNew<svtkIdList> cellsOnPoint;
      std::vector<T> data(4*ncomps);
      for (svtkIdType pid = 0; pid < npoints; ++pid)
      {
        std::fill(data.begin(), data.end(), 0);
        T numPointCells[4] = {0, 0, 0, 0};
        // Get all cells touching this point.
        src->GetPointCells(pid, cellsOnPoint);
        svtkIdType numPatchCells = cellsOnPoint->GetNumberOfIds();
        for (svtkIdType pc = 0; pc  < numPatchCells; pc++)
        {
          svtkIdType cellId = cellsOnPoint->GetId(pc);
          int cellDimension = src->GetCell(cellId)->GetCellDimension();
          numPointCells[cellDimension] += 1;
          const auto srcTuple = srcTuples[cellId];
          for (int comp=0;comp<ncomps;comp++)
          {
            data[comp+ncomps*cellDimension] += srcTuple[comp];
          }
        }
        auto dstTuple = dstTuples[pid];
        for (int dimension=3;dimension>=0;dimension--)
        {
          if (numPointCells[dimension])
          {
            for (int comp=0;comp<ncomps;comp++)
            {
              dstTuple[comp] = data[comp+dimension*ncomps] / numPointCells[dimension];
            }
            break;
          }
        }
      }
    }
  }
};

} // end anonymous namespace

class svtkCellDataToPointData::Internals
{
public:
  std::set<std::string> CellDataArrays;

  // Special traversal algorithm for svtkUniformGrid and svtkRectilinearGrid to support blanking
  // points will not have more than 8 cells for either of these data sets
  template <typename T>
  int InterpolatePointDataWithMask(svtkCellDataToPointData* filter, T* input, svtkDataSet* output)
  {
    svtkNew<svtkIdList> allCellIds;
    allCellIds->Allocate(8);
    svtkNew<svtkIdList> cellIds;
    cellIds->Allocate(8);

    svtkIdType numPts = input->GetNumberOfPoints();

    svtkCellData* inputInCD = input->GetCellData();
    svtkCellData* inCD;
    svtkPointData* outPD = output->GetPointData();

    if (!filter->GetProcessAllArrays())
    {
      inCD = svtkCellData::New();

      for (const auto& name : this->CellDataArrays)
      {
        svtkAbstractArray* arr = inputInCD->GetAbstractArray(name.c_str());
        if (arr == nullptr)
        {
          svtkWarningWithObjectMacro(filter, "cell data array name not found.");
          continue;
        }
        inCD->AddArray(arr);
      }
    }
    else
    {
      inCD = inputInCD;
    }

    outPD->InterpolateAllocate(inCD, numPts);

    double weights[8];

    int abort = 0;
    svtkIdType progressInterval = numPts / 20 + 1;
    for (svtkIdType ptId = 0; ptId < numPts && !abort; ptId++)
    {
      if (!(ptId % progressInterval))
      {
        filter->UpdateProgress(static_cast<double>(ptId) / numPts);
        abort = filter->GetAbortExecute();
      }
      input->GetPointCells(ptId, allCellIds);
      cellIds->Reset();
      // Only consider cells that are not masked:
      for (svtkIdType cId = 0; cId < allCellIds->GetNumberOfIds(); ++cId)
      {
        svtkIdType curCell = allCellIds->GetId(cId);
        if (input->IsCellVisible(curCell))
        {
          cellIds->InsertNextId(curCell);
        }
      }

      svtkIdType numCells = cellIds->GetNumberOfIds();

      if (numCells > 0)
      {
        double weight = 1.0 / numCells;
        for (svtkIdType cellId = 0; cellId < numCells; cellId++)
        {
          weights[cellId] = weight;
        }
        outPD->InterpolatePoint(inCD, ptId, cellIds, weights);
      }
      else
      {
        outPD->NullPoint(ptId);
      }
    }

    if (!filter->GetProcessAllArrays())
    {
      inCD->Delete();
    }

    return 1;
  }
};

//----------------------------------------------------------------------------
// Instantiate object so that cell data is not passed to output.
svtkCellDataToPointData::svtkCellDataToPointData()
{
  this->PassCellData = 0;
  this->ContributingCellOption = svtkCellDataToPointData::All;
  this->ProcessAllArrays = true;
  this->Implementation = new Internals();
}

//----------------------------------------------------------------------------
svtkCellDataToPointData::~svtkCellDataToPointData()
{
  delete this->Implementation;
}

//----------------------------------------------------------------------------
void svtkCellDataToPointData::AddCellDataArray(const char* name)
{
  if (!name)
  {
    svtkErrorMacro("name cannot be null.");
    return;
  }

  this->Implementation->CellDataArrays.insert(std::string(name));
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkCellDataToPointData::RemoveCellDataArray(const char* name)
{
  if (!name)
  {
    svtkErrorMacro("name cannot be null.");
    return;
  }

  this->Implementation->CellDataArrays.erase(name);
  this->Modified();
}

//----------------------------------------------------------------------------
void svtkCellDataToPointData::ClearCellDataArrays()
{
  if (!this->Implementation->CellDataArrays.empty())
  {
    this->Modified();
  }
  this->Implementation->CellDataArrays.clear();
}

//----------------------------------------------------------------------------
int svtkCellDataToPointData::RequestData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkInformation* info = outputVector->GetInformationObject(0);
  svtkDataSet* output = svtkDataSet::SafeDownCast(info->Get(svtkDataObject::DATA_OBJECT()));

  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkDebugMacro(<< "Mapping cell data to point data");

  // Special traversal algorithm for unstructured grid
  if (input->IsA("svtkUnstructuredGrid") || input->IsA("svtkPolyData"))
  {
    return this->RequestDataForUnstructuredData(nullptr, inputVector, outputVector);
  }

  svtkDebugMacro(<< "Mapping cell data to point data");

  // First, copy the input to the output as a starting point
  output->CopyStructure(input);

  // Pass the point data first. The fields and attributes
  // which also exist in the cell data of the input will
  // be over-written during CopyAllocate
  output->GetPointData()->CopyGlobalIdsOff();
  output->GetPointData()->PassData(input->GetPointData());
  output->GetPointData()->CopyFieldOff(svtkDataSetAttributes::GhostArrayName());

  if (input->GetNumberOfPoints() < 1)
  {
    svtkDebugMacro(<< "No input point data!");
    return 1;
  }

  // Do the interpolation, taking care of masked cells if needed.
  svtkStructuredGrid* sGrid = svtkStructuredGrid::SafeDownCast(input);
  svtkUniformGrid* uniformGrid = svtkUniformGrid::SafeDownCast(input);
  int result;
  if (sGrid && sGrid->HasAnyBlankCells())
  {
    result = this->Implementation->InterpolatePointDataWithMask(this, sGrid, output);
  }
  else if (uniformGrid && uniformGrid->HasAnyBlankCells())
  {
    result = this->Implementation->InterpolatePointDataWithMask(this, uniformGrid, output);
  }
  else
  {
    result = this->InterpolatePointData(input, output);
  }

  if (result == 0)
  {
    return 0;
  }

  if (!this->PassCellData)
  {
    output->GetCellData()->CopyAllOff();
    output->GetCellData()->CopyFieldOn(svtkDataSetAttributes::GhostArrayName());
  }
  output->GetCellData()->PassData(input->GetCellData());
  output->GetFieldData()->PassData(input->GetFieldData());

  return 1;
}

//----------------------------------------------------------------------------
void svtkCellDataToPointData::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "PassCellData: " << (this->PassCellData ? "On\n" : "Off\n");
  os << indent << "ContributingCellOption: " << this->ContributingCellOption << endl;
}

//----------------------------------------------------------------------------
int svtkCellDataToPointData::RequestDataForUnstructuredData(
  svtkInformation*, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  svtkDataSet* const src = svtkDataSet::SafeDownCast(
    inputVector[0]->GetInformationObject(0)->Get(svtkDataObject::DATA_OBJECT()));
  svtkDataSet* const dst = svtkDataSet::SafeDownCast(
    outputVector->GetInformationObject(0)->Get(svtkDataObject::DATA_OBJECT()));

  svtkIdType const ncells = src->GetNumberOfCells();
  svtkIdType const npoints = src->GetNumberOfPoints();
  if (ncells < 1 || npoints < 1)
  {
    svtkDebugMacro(<< "No input data!");
    return 1;
  }

  // count the number of cells associated with each point. if we are doing patches
  // though we will do that later on.
  svtkSmartPointer<svtkUnsignedIntArray> num;
  int highestCellDimension = 0;
  if (this->ContributingCellOption != svtkCellDataToPointData::Patch)
  {
    num = svtkSmartPointer<svtkUnsignedIntArray>::New();
    num->SetNumberOfComponents(1);
    num->SetNumberOfTuples(npoints);
    std::fill_n(num->GetPointer(0), npoints, 0u);
    if (this->ContributingCellOption == svtkCellDataToPointData::DataSetMax)
    {
      int maxDimension = src->IsA("svtkPolyData") == 1 ? 2 : 3;
      for (svtkIdType i = 0; i < src->GetNumberOfCells(); i++)
      {
        int dim = src->GetCell(i)->GetCellDimension();
        if (dim > highestCellDimension)
        {
          highestCellDimension = dim;
          if (highestCellDimension == maxDimension)
          {
            break;
          }
        }
      }
    }
    svtkNew<svtkIdList> pids;
    for (svtkIdType cid = 0; cid < ncells; ++cid)
    {
      if (src->GetCell(cid)->GetCellDimension() >= highestCellDimension)
      {
        src->GetCellPoints(cid, pids);
        for (svtkIdType i = 0, I = pids->GetNumberOfIds(); i < I; ++i)
        {
          svtkIdType const pid = pids->GetId(i);
          num->SetValue(pid, num->GetValue(pid) + 1);
        }
      }
    }
  }

  // First, copy the input to the output as a starting point
  dst->CopyStructure(src);
  svtkPointData* const opd = dst->GetPointData();

  // Pass the point data first. The fields and attributes
  // which also exist in the cell data of the input will
  // be over-written during CopyAllocate
  opd->CopyGlobalIdsOff();
  opd->PassData(src->GetPointData());
  opd->CopyFieldOff(svtkDataSetAttributes::GhostArrayName());

  // Copy all existing cell fields into a temporary cell data array,
  // unless the SelectCellDataArrays option is active
  svtkSmartPointer<svtkCellData> processedCellData;
  if (!this->ProcessAllArrays)
  {
    processedCellData = svtkSmartPointer<svtkCellData>::New();

    svtkCellData* processedCellDataTemp = src->GetCellData();
    for (const auto& name : this->Implementation->CellDataArrays)
    {
      svtkAbstractArray* arr = processedCellDataTemp->GetAbstractArray(name.c_str());
      if (arr == nullptr)
      {
        svtkWarningMacro("cell data array name not found.");
        continue;
      }
      processedCellData->AddArray(arr);
    }
  }
  else
  {
    processedCellData = svtkSmartPointer<svtkCellData>(src->GetCellData());
  }

  // Remove all fields that are not a data array.
  for (svtkIdType fid = processedCellData->GetNumberOfArrays(); fid--;)
  {
    if (!svtkDataArray::FastDownCast(processedCellData->GetAbstractArray(fid)))
    {
      processedCellData->RemoveArray(fid);
    }
  }

  // Cell field list constructed from the filtered cell data array
  svtkDataSetAttributes::FieldList cfl(1);
  cfl.InitializeFieldList(processedCellData);
  opd->InterpolateAllocate(cfl, npoints, npoints);

  const auto nfields = processedCellData->GetNumberOfArrays();
  int fid = 0;
  auto f = [this, &fid, nfields, npoints, src, num, ncells, highestCellDimension](
             svtkAbstractArray* aa_srcarray, svtkAbstractArray* aa_dstarray) {
    // update progress and check for an abort request.
    this->UpdateProgress((fid + 1.0) / nfields);
    ++fid;

    if (this->GetAbortExecute())
    {
      return;
    }

    svtkDataArray* const srcarray = svtkDataArray::FastDownCast(aa_srcarray);
    svtkDataArray* const dstarray = svtkDataArray::FastDownCast(aa_dstarray);
    if (srcarray && dstarray)
    {
      dstarray->SetNumberOfTuples(npoints);
      svtkIdType const ncomps = srcarray->GetNumberOfComponents();

      Spread worker;
      using Dispatcher = svtkArrayDispatch::Dispatch2SameValueType;
      if (!Dispatcher::Execute(srcarray, dstarray, worker, src, num,
                               ncells, npoints, ncomps, highestCellDimension,
                               this->ContributingCellOption))
      { // fallback for unknown arrays:
        worker(srcarray, dstarray, src, num, ncells, npoints, ncomps,
               highestCellDimension, this->ContributingCellOption);
      }
    }
  };

  if (processedCellData != nullptr && dst->GetPointData() != nullptr)
  {
    cfl.TransformData(0, processedCellData, dst->GetPointData(), f);
  }

  if (!this->PassCellData)
  {
    dst->GetCellData()->CopyAllOff();
    dst->GetCellData()->CopyFieldOn(svtkDataSetAttributes::GhostArrayName());
  }
  dst->GetCellData()->PassData(src->GetCellData());

  return 1;
}

int svtkCellDataToPointData::InterpolatePointData(svtkDataSet* input, svtkDataSet* output)
{
  svtkNew<svtkIdList> cellIds;
  cellIds->Allocate(SVTK_MAX_CELLS_PER_POINT);

  svtkIdType numPts = input->GetNumberOfPoints();

  svtkCellData* inputInCD = input->GetCellData();
  svtkCellData* inCD;
  svtkPointData* outPD = output->GetPointData();

  if (!this->ProcessAllArrays)
  {
    inCD = svtkCellData::New();

    for (const auto& name : this->Implementation->CellDataArrays)
    {
      svtkAbstractArray* arr = inputInCD->GetAbstractArray(name.c_str());
      if (arr == nullptr)
      {
        svtkWarningMacro("cell data array name not found.");
        continue;
      }
      inCD->AddArray(arr);
    }
  }
  else
  {
    inCD = inputInCD;
  }

  outPD->InterpolateAllocate(inCD, numPts);

  double weights[SVTK_MAX_CELLS_PER_POINT];

  int abort = 0;
  svtkIdType progressInterval = numPts / 20 + 1;
  for (svtkIdType ptId = 0; ptId < numPts && !abort; ptId++)
  {
    if (!(ptId % progressInterval))
    {
      this->UpdateProgress(static_cast<double>(ptId) / numPts);
      abort = GetAbortExecute();
    }

    input->GetPointCells(ptId, cellIds);
    svtkIdType numCells = cellIds->GetNumberOfIds();

    if (numCells > 0 && numCells < SVTK_MAX_CELLS_PER_POINT)
    {
      double weight = 1.0 / numCells;
      for (svtkIdType cellId = 0; cellId < numCells; cellId++)
      {
        weights[cellId] = weight;
      }
      outPD->InterpolatePoint(inCD, ptId, cellIds, weights);
    }
    else
    {
      outPD->NullPoint(ptId);
    }
  }

  if (!this->ProcessAllArrays)
  {
    inCD->Delete();
  }

  return 1;
}
