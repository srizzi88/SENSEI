/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContourGrid.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSMPContourGrid.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkDemandDrivenPipeline.h"
#include "svtkGenericCell.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMergePoints.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkMultiPieceDataSet.h"
#include "svtkNew.h"
#include "svtkNonMergingPointLocator.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkSMPMergePoints.h"
#include "svtkSMPMergePolyDataHelper.h"
#include "svtkSMPThreadLocal.h"
#include "svtkSMPThreadLocalObject.h"
#include "svtkSMPTools.h"
#include "svtkSmartPointer.h"
#include "svtkSpanSpace.h"
#include "svtkUnstructuredGrid.h"

#include "svtkTimerLog.h"

#include <cmath>

svtkStandardNewMacro(svtkSMPContourGrid);

//-----------------------------------------------------------------------------
// Construct object with initial range (0,1) and single contour value
// of 0.0.
svtkSMPContourGrid::svtkSMPContourGrid()
{
  this->MergePieces = true;
}

//-----------------------------------------------------------------------------
svtkSMPContourGrid::~svtkSMPContourGrid() = default;

//-----------------------------------------------------------------------------
// This is to support parallel processing and potential polydata merging.
namespace
{

struct svtkLocalDataType
{
  svtkPolyData* Output;
  svtkSMPMergePoints* Locator;
  svtkIdList* VertCellOffsets;
  svtkIdList* VertConnOffsets;
  svtkIdList* LineCellOffsets;
  svtkIdList* LineConnOffsets;
  svtkIdList* PolyCellOffsets;
  svtkIdList* PolyConnOffsets;

  svtkLocalDataType()
    : Output(nullptr)
  {
  }
};

//-----------------------------------------------------------------------------
// This functor uses thread local storage to create one svtkPolyData per
// thread. Each execution of the functor adds to the svtkPolyData that is
// local to the thread it is running on.
template <typename T>
class svtkContourGridFunctor
{
public:
  svtkSMPContourGrid* Filter;

  svtkUnstructuredGrid* Input;
  svtkDataArray* InScalars;

  svtkDataObject* Output;

  svtkSMPThreadLocal<svtkDataArray*> CellScalars;

  svtkSMPThreadLocalObject<svtkGenericCell> Cell;
  svtkSMPThreadLocalObject<svtkPoints> NewPts;
  svtkSMPThreadLocalObject<svtkCellArray> NewVerts;
  svtkSMPThreadLocalObject<svtkCellArray> NewLines;
  svtkSMPThreadLocalObject<svtkCellArray> NewPolys;

  svtkSMPThreadLocal<svtkLocalDataType> LocalData;

  int NumValues;
  double* Values;

  svtkContourGridFunctor(svtkSMPContourGrid* filter, svtkUnstructuredGrid* input,
    svtkDataArray* inScalars, int numValues, double* values, svtkDataObject* output)
    : Filter(filter)
    , Input(input)
    , InScalars(inScalars)
    , Output(output)
    , NumValues(numValues)
    , Values(values)
  {
  }

  virtual ~svtkContourGridFunctor()
  {
    // Cleanup all temporaries

    svtkSMPThreadLocal<svtkDataArray*>::iterator cellScalarsIter = this->CellScalars.begin();
    while (cellScalarsIter != this->CellScalars.end())
    {
      (*cellScalarsIter)->Delete();
      ++cellScalarsIter;
    }

    svtkSMPThreadLocal<svtkLocalDataType>::iterator dataIter = this->LocalData.begin();
    while (dataIter != this->LocalData.end())
    {
      (*dataIter).Output->Delete();
      (*dataIter).Locator->Delete();
      (*dataIter).VertCellOffsets->Delete();
      (*dataIter).VertConnOffsets->Delete();
      (*dataIter).LineCellOffsets->Delete();
      (*dataIter).LineConnOffsets->Delete();
      (*dataIter).PolyCellOffsets->Delete();
      (*dataIter).PolyConnOffsets->Delete();
      ++dataIter;
    }
  }

  void Initialize()
  {
    // Initialize thread local object before any processing happens.
    // This gets called once per thread.

    svtkPointLocator* locator;
    svtkPolyData* output;
    svtkIdList* vertCellOffsets;
    svtkIdList* vertConnOffsets;
    svtkIdList* lineCellOffsets;
    svtkIdList* lineConnOffsets;
    svtkIdList* polyCellOffsets;
    svtkIdList* polyConnOffsets;

    svtkLocalDataType& localData = this->LocalData.Local();

    localData.Output = svtkPolyData::New();
    output = localData.Output;

    localData.Locator = svtkSMPMergePoints::New();
    locator = localData.Locator;

    localData.VertCellOffsets = svtkIdList::New();
    vertCellOffsets = localData.VertCellOffsets;
    localData.VertConnOffsets = svtkIdList::New();
    vertConnOffsets = localData.VertConnOffsets;

    localData.LineCellOffsets = svtkIdList::New();
    lineCellOffsets = localData.LineCellOffsets;
    localData.LineConnOffsets = svtkIdList::New();
    lineConnOffsets = localData.LineConnOffsets;

    localData.PolyCellOffsets = svtkIdList::New();
    polyCellOffsets = localData.PolyCellOffsets;
    localData.PolyConnOffsets = svtkIdList::New();
    polyConnOffsets = localData.PolyConnOffsets;

    svtkPoints*& newPts = this->NewPts.Local();

    // set precision for the points in the output
    if (this->Filter->GetOutputPointsPrecision() == svtkAlgorithm::DEFAULT_PRECISION)
    {
      newPts->SetDataType(this->Input->GetPoints()->GetDataType());
    }
    else if (this->Filter->GetOutputPointsPrecision() == svtkAlgorithm::SINGLE_PRECISION)
    {
      newPts->SetDataType(SVTK_FLOAT);
    }
    else if (this->Filter->GetOutputPointsPrecision() == svtkAlgorithm::DOUBLE_PRECISION)
    {
      newPts->SetDataType(SVTK_DOUBLE);
    }

    output->SetPoints(newPts);

    svtkIdType numCells = this->Input->GetNumberOfCells();

    svtkIdType estimatedSize = static_cast<svtkIdType>(pow(static_cast<double>(numCells), .75));
    estimatedSize = estimatedSize / 1024 * 1024; // multiple of 1024
    if (estimatedSize < 1024)
    {
      estimatedSize = 1024;
    }

    newPts->Allocate(estimatedSize, estimatedSize);

    vertCellOffsets->Allocate(estimatedSize);
    vertConnOffsets->Allocate(estimatedSize);
    lineCellOffsets->Allocate(estimatedSize);
    lineConnOffsets->Allocate(estimatedSize);
    polyCellOffsets->Allocate(estimatedSize);
    polyConnOffsets->Allocate(estimatedSize);

    // locator->SetPoints(newPts);
    locator->InitPointInsertion(newPts, this->Input->GetBounds(), this->Input->GetNumberOfPoints());

    svtkCellArray*& newVerts = this->NewVerts.Local();
    newVerts->AllocateExact(estimatedSize, estimatedSize);
    output->SetVerts(newVerts);

    svtkCellArray*& newLines = this->NewLines.Local();
    newLines->AllocateExact(estimatedSize, estimatedSize);
    output->SetLines(newLines);

    svtkCellArray*& newPolys = this->NewPolys.Local();
    newPolys->AllocateExact(estimatedSize, estimatedSize);
    output->SetPolys(newPolys);

    svtkDataArray*& cellScalars = this->CellScalars.Local();
    cellScalars = this->InScalars->NewInstance();
    cellScalars->SetNumberOfComponents(this->InScalars->GetNumberOfComponents());
    cellScalars->Allocate(SVTK_CELL_SIZE * this->InScalars->GetNumberOfComponents());

    svtkPointData* outPd = output->GetPointData();
    svtkCellData* outCd = output->GetCellData();
    svtkPointData* inPd = this->Input->GetPointData();
    svtkCellData* inCd = this->Input->GetCellData();
    outPd->InterpolateAllocate(inPd, estimatedSize, estimatedSize);
    outCd->CopyAllocate(inCd, estimatedSize, estimatedSize);
  }

  void operator()(svtkIdType begin, svtkIdType end)
  {
    // Actual computation.
    // Note the usage of thread local objects. These objects
    // persist for each thread across multiple execution of the
    // functor.

    svtkLocalDataType& localData = this->LocalData.Local();

    svtkGenericCell* cell = this->Cell.Local();
    svtkDataArray* cs = this->CellScalars.Local();
    svtkPointData* inPd = this->Input->GetPointData();
    svtkCellData* inCd = this->Input->GetCellData();

    svtkPolyData* output = localData.Output;
    svtkPointData* outPd = output->GetPointData();
    svtkCellData* outCd = output->GetCellData();

    svtkCellArray* vrts = this->NewVerts.Local();
    svtkCellArray* lines = this->NewLines.Local();
    svtkCellArray* polys = this->NewPolys.Local();

    svtkPointLocator* loc = localData.Locator;

    svtkIdList* vertCellOffsets = localData.VertCellOffsets;
    svtkIdList* vertConnOffsets = localData.VertConnOffsets;
    svtkIdList* lineCellOffsets = localData.LineCellOffsets;
    svtkIdList* lineConnOffsets = localData.LineConnOffsets;
    svtkIdList* polyCellOffsets = localData.PolyCellOffsets;
    svtkIdList* polyConnOffsets = localData.PolyConnOffsets;

    const double* values = this->Values;
    int numValues = this->NumValues;

    svtkNew<svtkIdList> pids;
    T range[2];
    svtkIdType cellid;

    // If UseScalarTree is enabled at this point, we assume that a scalar
    // tree has been computed and thus the way cells are traversed changes.
    if (!this->Filter->GetUseScalarTree())
    {
      // This code assumes no scalar tree, thus it checks scalar range prior
      // to invoking contour.
      for (cellid = begin; cellid < end; cellid++)
      {
        this->Input->GetCellPoints(cellid, pids);
        cs->SetNumberOfTuples(pids->GetNumberOfIds());
        this->InScalars->GetTuples(pids, cs);
        int numCellScalars = cs->GetNumberOfComponents() * cs->GetNumberOfTuples();
        T* cellScalarPtr = static_cast<T*>(cs->GetVoidPointer(0));

        // find min and max values in scalar data
        range[0] = range[1] = cellScalarPtr[0];

        for (T *it = cellScalarPtr + 1, *itEnd = cellScalarPtr + numCellScalars; it != itEnd; ++it)
        {
          if (*it < range[0])
          {
            range[0] = *it;
          } // if scalar <= min range value
          if (*it > range[1])
          {
            range[1] = *it;
          } // if scalar >= max range value
        }   // for all cellScalars

        bool needCell = false;
        for (int i = 0; i < numValues; i++)
        {
          if ((values[i] >= range[0]) && (values[i] <= range[1]))
          {
            needCell = true;
          } // if contour value in range for this cell
        }   // end for numContours

        if (needCell)
        {
          this->Input->GetCell(cellid, cell);

          for (int i = 0; i < numValues; i++)
          {
            if ((values[i] >= range[0]) && (values[i] <= range[1]))
            {
              svtkIdType begVertCellSize = vrts->GetNumberOfCells();
              svtkIdType begVertConnSize = vrts->GetNumberOfConnectivityIds();
              svtkIdType begLineCellSize = lines->GetNumberOfCells();
              svtkIdType begLineConnSize = lines->GetNumberOfConnectivityIds();
              svtkIdType begPolyCellSize = polys->GetNumberOfCells();
              svtkIdType begPolyConnSize = polys->GetNumberOfConnectivityIds();
              cell->Contour(
                values[i], cs, loc, vrts, lines, polys, inPd, outPd, inCd, cellid, outCd);
              // We keep track of the insertion point of verts, lines and polys.
              // This is later used when merging these data structures in parallel.
              // The reason this is needed is that svtkCellArray is not normally
              // random access with makes processing it in parallel very difficult.
              // So we create a semi-random-access structures in parallel. This
              // is only useful for merging since each of these indices can point
              // to multiple cells.
              if (vrts->GetNumberOfCells() > begVertCellSize)
              {
                vertCellOffsets->InsertNextId(begVertCellSize);
              }
              if (vrts->GetNumberOfConnectivityIds() > begVertConnSize)
              {
                vertConnOffsets->InsertNextId(begVertConnSize);
              }
              if (lines->GetNumberOfCells() > begLineCellSize)
              {
                lineCellOffsets->InsertNextId(begLineCellSize);
              }
              if (lines->GetNumberOfConnectivityIds() > begLineConnSize)
              {
                lineConnOffsets->InsertNextId(begLineConnSize);
              }
              if (polys->GetNumberOfCells() > begPolyCellSize)
              {
                polyCellOffsets->InsertNextId(begPolyCellSize);
              }
              if (polys->GetNumberOfConnectivityIds() > begPolyConnSize)
              {
                polyConnOffsets->InsertNextId(begPolyConnSize);
              }
            }
          }
        } // if cell need be contoured
      }   // for all cells
    }     // if no scalar tree requested
    else
    { // scalar tree provided
      // The begin / end parameters to this function represent batches of candidate
      // cells.
      svtkIdType numCellsContoured = 0;
      svtkScalarTree* scalarTree = this->Filter->GetScalarTree();
      const svtkIdType* cellIds;
      svtkIdType numCells;
      for (svtkIdType batchNum = begin; batchNum < end; ++batchNum)
      {
        cellIds = scalarTree->GetCellBatch(batchNum, numCells);
        for (svtkIdType idx = 0; idx < numCells; ++idx)
        {
          cellid = cellIds[idx];
          this->Input->GetCellPoints(cellid, pids);
          cs->SetNumberOfTuples(pids->GetNumberOfIds());
          this->InScalars->GetTuples(pids, cs);

          // Okay let's grab the cell and contour it
          numCellsContoured++;
          this->Input->GetCell(cellid, cell);
          svtkIdType begVertCellSize = vrts->GetNumberOfCells();
          svtkIdType begVertConnSize = vrts->GetNumberOfConnectivityIds();
          svtkIdType begLineCellSize = lines->GetNumberOfCells();
          svtkIdType begLineConnSize = lines->GetNumberOfConnectivityIds();
          svtkIdType begPolyCellSize = polys->GetNumberOfCells();
          svtkIdType begPolyConnSize = polys->GetNumberOfConnectivityIds();
          cell->Contour(scalarTree->GetScalarValue(), cs, loc, vrts, lines, polys, inPd, outPd,
            inCd, cellid, outCd);
          // We keep track of the insertion point of verts, lines and polys.
          // This is later used when merging these data structures in parallel.
          // The reason this is needed is that svtkCellArray is not normally
          // random access with makes processing it in parallel very difficult.
          // So we create a semi-random-access structures in parallel. This
          // is only useful for merging since each of these indices can point
          // to multiple cells.
          if (vrts->GetNumberOfCells() > begVertCellSize)
          {
            vertCellOffsets->InsertNextId(begVertCellSize);
          }
          if (vrts->GetNumberOfConnectivityIds() > begVertConnSize)
          {
            vertConnOffsets->InsertNextId(begVertConnSize);
          }
          if (lines->GetNumberOfCells() > begLineCellSize)
          {
            lineCellOffsets->InsertNextId(begLineCellSize);
          }
          if (lines->GetNumberOfConnectivityIds() > begLineConnSize)
          {
            lineConnOffsets->InsertNextId(begLineConnSize);
          }
          if (polys->GetNumberOfCells() > begPolyCellSize)
          {
            polyCellOffsets->InsertNextId(begPolyCellSize);
          }
          if (polys->GetNumberOfConnectivityIds() > begPolyConnSize)
          {
            polyConnOffsets->InsertNextId(begPolyConnSize);
          }
        } // for all cells in this batch
      }   // for this batch of cells
    }     // using scalar tree

  } // operator()

  void Reduce()
  {
    // Create the final multi-block dataset

    svtkNew<svtkMultiPieceDataSet> mp;
    int count = 0;

    svtkSMPThreadLocal<svtkLocalDataType>::iterator outIter = this->LocalData.begin();
    while (outIter != this->LocalData.end())
    {
      svtkPolyData* output = (*outIter).Output;

      if (output->GetVerts()->GetNumberOfCells() == 0)
      {
        output->SetVerts(nullptr);
      }

      if (output->GetLines()->GetNumberOfCells() == 0)
      {
        output->SetLines(nullptr);
      }

      if (output->GetPolys()->GetNumberOfCells() == 0)
      {
        output->SetPolys(nullptr);
      }

      output->Squeeze();

      mp->SetPiece(count++, output);

      ++outIter;
    }

    svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::SafeDownCast(this->Output);
    // If the output is a svtkPolyData (no merging), we throw away the multi-piece
    // dataset.
    if (output)
    {
      output->SetBlock(0, mp);
    }
  }
};

//-----------------------------------------------------------------------------
template <typename T>
void DoContour(svtkSMPContourGrid* filter, svtkUnstructuredGrid* input, svtkIdType numCells,
  svtkDataArray* inScalars, int numContours, double* values, svtkDataObject* output)
{
  // Contour in parallel; create the processing functor
  svtkContourGridFunctor<T> functor(filter, input, inScalars, numContours, values, output);

  // If a scalar tree is used, then the way in which cells are iterated over changes.
  // With a scalar tree, batches of candidate cells are provided. Without one, then all
  // cells are iterated over one by one.
  if (filter->GetUseScalarTree())
  { // process in threaded using scalar tree
    svtkScalarTree* scalarTree = filter->GetScalarTree();
    svtkIdType numBatches;
    for (int i = 0; i < numContours; ++i)
    {
      numBatches = scalarTree->GetNumberOfCellBatches(values[i]);
      if (numBatches > 0)
      {
        svtkSMPTools::For(0, numBatches, functor);
      }
    }
  }
  else
  { // process all cells in parallel manner
    svtkSMPTools::For(0, numCells, functor);
  }

  // Now process the output from the separate threads. Merging may or may not be
  // required.
  if (output->IsA("svtkPolyData"))
  {
    // Do the merging.
    svtkSMPThreadLocal<svtkLocalDataType>::iterator itr = functor.LocalData.begin();
    svtkSMPThreadLocal<svtkLocalDataType>::iterator end = functor.LocalData.end();

    std::vector<svtkSMPMergePolyDataHelper::InputData> mpData;
    while (itr != end)
    {
      mpData.push_back(svtkSMPMergePolyDataHelper::InputData((*itr).Output, (*itr).Locator,
        (*itr).VertCellOffsets, (*itr).VertConnOffsets, (*itr).LineCellOffsets,
        (*itr).LineConnOffsets, (*itr).PolyCellOffsets, (*itr).PolyConnOffsets));
      ++itr;
    }

    svtkPolyData* moutput = svtkSMPMergePolyDataHelper::MergePolyData(mpData);
    output->ShallowCopy(moutput);
    moutput->Delete();
  }
}

} // end namespace

//-----------------------------------------------------------------------------
int svtkSMPContourGrid::RequestDataObject(
  svtkInformation* svtkNotUsed(request), svtkInformationVector**, svtkInformationVector* outputVector)
{
  svtkInformation* info = outputVector->GetInformationObject(0);
  if (this->MergePieces)
  {
    svtkPolyData* output = svtkPolyData::GetData(info);
    if (!output)
    {
      svtkPolyData* newOutput = svtkPolyData::New();
      info->Set(svtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
    }
  }
  else
  {
    svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::GetData(info);
    if (!output)
    {
      svtkMultiBlockDataSet* newOutput = svtkMultiBlockDataSet::New();
      info->Set(svtkDataObject::DATA_OBJECT(), newOutput);
      newOutput->Delete();
    }
  }
  return 1;
}

//-----------------------------------------------------------------------------
int svtkSMPContourGrid::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the input and output
  svtkUnstructuredGrid* input = svtkUnstructuredGrid::GetData(inputVector[0]);
  svtkDataObject* output = svtkDataObject::GetData(outputVector);

  if (input->GetNumberOfCells() == 0)
  {
    return 1;
  }

  svtkDataArray* inScalars = this->GetInputArrayToProcess(0, inputVector);
  if (!inScalars)
  {
    return 1;
  }

  // Not thread safe so calculate first.
  input->GetBounds();

  svtkIdType numContours = this->GetNumberOfContours();
  if (numContours < 1)
  {
    return 1;
  }

  double* values = this->GetValues();

  svtkIdType numCells = input->GetNumberOfCells();

  // Create scalar tree if necessary and if requested
  int useScalarTree = this->GetUseScalarTree();
  if (useScalarTree)
  {
    if (this->ScalarTree == nullptr)
    {
      this->ScalarTree = svtkSpanSpace::New();
    }
    this->ScalarTree->SetDataSet(input);
    this->ScalarTree->SetScalars(inScalars);
  }

  // Actually execute the contouring operation
  if (inScalars->GetDataType() == SVTK_FLOAT)
  {
    DoContour<float>(this, input, numCells, inScalars, numContours, values, output);
  }
  else if (inScalars->GetDataType() == SVTK_DOUBLE)
  {
    DoContour<double>(this, input, numCells, inScalars, numContours, values, output);
  }

  return 1;
}

//-----------------------------------------------------------------------------
int svtkSMPContourGrid::FillOutputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkDataObject");
  return 1;
}

//-----------------------------------------------------------------------------
svtkTypeBool svtkSMPContourGrid::ProcessRequest(
  svtkInformation* request, svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // generate the data
  if (request->Has(svtkDemandDrivenPipeline::REQUEST_DATA_OBJECT()))
  {
    return this->RequestDataObject(request, inputVector, outputVector);
  }

  return this->Superclass::ProcessRequest(request, inputVector, outputVector);
}

//-----------------------------------------------------------------------------
void svtkSMPContourGrid::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Merge Pieces: " << (this->MergePieces ? "On\n" : "Off\n");
}
