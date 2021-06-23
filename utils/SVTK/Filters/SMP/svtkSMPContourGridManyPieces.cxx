/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContourGridManyPieces.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSMPContourGridManyPieces.h"

#include "svtkCellArray.h"
#include "svtkCellData.h"
#include "svtkDataArray.h"
#include "svtkGenericCell.h"
#include "svtkInformation.h"
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
#include "svtkUnstructuredGrid.h"

#include "svtkTimerLog.h"

#include <cmath>

svtkStandardNewMacro(svtkSMPContourGridManyPieces);

// Construct object with initial range (0,1) and single contour value
// of 0.0.
svtkSMPContourGridManyPieces::svtkSMPContourGridManyPieces()
{
  SVTK_LEGACY_BODY(svtkSMPContourGridManyPieces::svtkSMPContourGridManyPieces, "SVTK 8.1");
}

svtkSMPContourGridManyPieces::~svtkSMPContourGridManyPieces() = default;

namespace
{
// This functor creates a new svtkPolyData piece each time it runs.
// This is less efficient that the previous version but can be used
// to generate more piece to exploit coarse-grained parallelism
// downstream.
template <typename T>
class svtkContourGridManyPiecesFunctor
{
  svtkSMPContourGridManyPieces* Filter;

  svtkUnstructuredGrid* Input;
  svtkDataArray* InScalars;

  svtkMultiBlockDataSet* Output;

  int NumValues;
  double* Values;

  svtkSMPThreadLocal<std::vector<svtkPolyData*> > Outputs;

public:
  svtkContourGridManyPiecesFunctor(svtkSMPContourGridManyPieces* filter, svtkUnstructuredGrid* input,
    svtkDataArray* inScalars, int numValues, double* values, svtkMultiBlockDataSet* output)
    : Filter(filter)
    , Input(input)
    , InScalars(inScalars)
    , Output(output)
    , NumValues(numValues)
    , Values(values)
  {
  }

  ~svtkContourGridManyPiecesFunctor() = default;

  void Initialize() {}

  void operator()(svtkIdType begin, svtkIdType end)
  {
    svtkNew<svtkPolyData> output;

    svtkNew<svtkPoints> newPts;

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

    // svtkNew<svtkNonMergingPointLocator> locator;
    // locator->SetPoints(newPts);

    svtkNew<svtkMergePoints> locator;
    locator->InitPointInsertion(newPts, this->Input->GetBounds(), this->Input->GetNumberOfPoints());

    // svtkNew<svtkPointLocator> locator;
    // locator->InitPointInsertion (newPts,
    //                              this->Input->GetBounds(),
    //                              this->Input->GetNumberOfPoints());

    svtkNew<svtkCellArray> newVerts;
    newVerts->Allocate(estimatedSize, estimatedSize);

    svtkNew<svtkCellArray> newLines;
    newLines->Allocate(estimatedSize, estimatedSize);

    svtkNew<svtkCellArray> newPolys;
    newPolys->Allocate(estimatedSize, estimatedSize);

    svtkSmartPointer<svtkDataArray> cellScalars;
    cellScalars.TakeReference(this->InScalars->NewInstance());
    cellScalars->SetNumberOfComponents(this->InScalars->GetNumberOfComponents());
    cellScalars->Allocate(SVTK_CELL_SIZE * this->InScalars->GetNumberOfComponents());

    svtkPointData* outPd = output->GetPointData();
    svtkCellData* outCd = output->GetCellData();
    svtkPointData* inPd = this->Input->GetPointData();
    svtkCellData* inCd = this->Input->GetCellData();
    outPd->InterpolateAllocate(inPd, estimatedSize, estimatedSize);
    outCd->CopyAllocate(inCd, estimatedSize, estimatedSize);

    svtkNew<svtkGenericCell> cell;

    const double* values = this->Values;
    int numValues = this->NumValues;

    svtkNew<svtkIdList> pids;
    T range[2];

    for (svtkIdType cellid = begin; cellid < end; cellid++)
    {
      this->Input->GetCellPoints(cellid, pids);
      cellScalars->SetNumberOfTuples(pids->GetNumberOfIds());
      this->InScalars->GetTuples(pids, cellScalars);
      int numCellScalars = cellScalars->GetNumberOfComponents() * cellScalars->GetNumberOfTuples();
      T* cellScalarPtr = static_cast<T*>(cellScalars->GetVoidPointer(0));

      // find min and max values in scalar data
      range[0] = range[1] = cellScalarPtr[0];

      for (T *it = cellScalarPtr + 1, *itEnd = cellScalarPtr + numCellScalars; it != itEnd; ++it)
      {
        if (*it <= range[0])
        {
          range[0] = *it;
        } // if scalar <= min range value
        if (*it >= range[1])
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
            cell->Contour(values[i], cellScalars, locator, newVerts, newLines, newPolys, inPd,
              outPd, inCd, cellid, outCd);
          }
        }
      }
    }

    if (newVerts->GetNumberOfCells())
    {
      output->SetVerts(newVerts);
    }

    if (newLines->GetNumberOfCells())
    {
      output->SetLines(newLines);
    }

    if (newPolys->GetNumberOfCells())
    {
      output->SetPolys(newPolys);
    }

    output->Squeeze();

    output->Register(nullptr);
    this->Outputs.Local().push_back(output);
  }

  void Reduce()
  {
    svtkNew<svtkMultiPieceDataSet> mp;
    int count = 0;

    svtkSMPThreadLocal<std::vector<svtkPolyData*> >::iterator outIter = this->Outputs.begin();
    while (outIter != this->Outputs.end())
    {
      std::vector<svtkPolyData*>& outs = *outIter;
      std::vector<svtkPolyData*>::iterator iter = outs.begin();
      while (iter != outs.end())
      {
        mp->SetPiece(count++, *iter);
        (*iter)->Delete();
        ++iter;
      }
      ++outIter;
    }

    this->Output->SetBlock(0, mp);
  }
};

}

int svtkSMPContourGridManyPieces::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the input and output
  svtkUnstructuredGrid* input = svtkUnstructuredGrid::GetData(inputVector[0]);
  svtkMultiBlockDataSet* output = svtkMultiBlockDataSet::GetData(outputVector);

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

  // When using svtkContourGridManyPiecesFunctor, it is crucial to set the grain
  // right. When the grain is too small, which tends to be the default,
  // the overhead of allocating data structures, building locators etc.
  // ends up being too big.
  if (inScalars->GetDataType() == SVTK_FLOAT)
  {
    svtkContourGridManyPiecesFunctor<float> functor(
      this, input, inScalars, numContours, values, output);
    svtkIdType grain = numCells > 100000 ? numCells / 100 : numCells;
    svtkSMPTools::For(0, numCells, grain, functor);
  }
  else if (inScalars->GetDataType() == SVTK_DOUBLE)
  {
    svtkContourGridManyPiecesFunctor<double> functor(
      this, input, inScalars, numContours, values, output);
    svtkIdType grain = numCells > 100000 ? numCells / 100 : numCells;
    svtkSMPTools::For(0, numCells, grain, functor);
  }

  return 1;
}

int svtkSMPContourGridManyPieces::FillOutputPortInformation(
  int svtkNotUsed(port), svtkInformation* info)
{
  // now add our info
  info->Set(svtkDataObject::DATA_TYPE_NAME(), "svtkMultiBlockDataSet");
  return 1;
}

void svtkSMPContourGridManyPieces::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
