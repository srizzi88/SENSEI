/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDecimatePolylineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkDecimatePolylineFilter.h"

#include "svtkCellArray.h"
#include "svtkCellArrayIterator.h"
#include "svtkCellData.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkLine.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPriorityQueue.h"

#include <map>

svtkStandardNewMacro(svtkDecimatePolylineFilter);

//------------------------------------------------------------------------------
// Representation of a polyline as a doubly linked list of vertices.
class svtkDecimatePolylineFilter::Polyline
{
public:
  struct Vertex
  {
    unsigned index;
    svtkIdType id;
    Vertex* prev;
    Vertex* next;
    bool removable;
  };

  Polyline(const svtkIdType* vertexOrdering, svtkIdType size)
  {
    this->Size = size;
    Vertices = new Vertex[size];
    for (svtkIdType idx = 0; idx < size; ++idx)
    {
      Vertices[idx].index = idx;
      Vertices[idx].id = vertexOrdering[idx];
      Vertices[idx].prev = (idx > 0 ? &Vertices[idx - 1] : nullptr);
      Vertices[idx].next = (idx < size - 1 ? &Vertices[idx + 1] : nullptr);
      Vertices[idx].removable = true;
    }
    Vertices[0].removable = Vertices[size - 1].removable = false;
    // Some polylines close in on themselves
    this->IsLoop = (Vertices[0].id == Vertices[size - 1].id ? true : false);
  }

  ~Polyline()
  {
    if (Vertices)
    {
      delete[] Vertices;
      Vertices = nullptr;
    }
  }

  void Remove(svtkIdType vertexIdx)
  {
    this->Size--;
    (*(this->Vertices[vertexIdx].prev)).next = this->Vertices[vertexIdx].next;
    (*(this->Vertices[vertexIdx].next)).prev = this->Vertices[vertexIdx].prev;
  }

  svtkIdType Size;
  Vertex* Vertices;
  bool IsLoop;
};

//---------------------------------------------------------------------
// Create object with specified reduction of 90%.
svtkDecimatePolylineFilter::svtkDecimatePolylineFilter()
{
  this->TargetReduction = 0.90;
  this->PriorityQueue = svtkSmartPointer<svtkPriorityQueue>::New();
  this->MaximumError = SVTK_DOUBLE_MAX;
  this->OutputPointsPrecision = svtkAlgorithm::DEFAULT_PRECISION;
}

//---------------------------------------------------------------------
svtkDecimatePolylineFilter::~svtkDecimatePolylineFilter() = default;

//---------------------------------------------------------------------
double svtkDecimatePolylineFilter::ComputeError(
  svtkPolyData* input, Polyline* polyline, svtkIdType idx)
{
  svtkPoints* inputPoints = input->GetPoints();

  double x1[3], x[3], x2[3];
  inputPoints->GetPoint(polyline->Vertices[idx].prev->id, x1);
  inputPoints->GetPoint(polyline->Vertices[idx].id, x);
  inputPoints->GetPoint(polyline->Vertices[idx].next->id, x2);

  if (svtkMath::Distance2BetweenPoints(x1, x2) == 0.0)
  {
    return 0.0;
  }
  else
  {
    return svtkLine::DistanceToLine(x, x1, x2);
  }
}

//---------------------------------------------------------------------
//  Reduce the number of points in a set of polylines
//
int svtkDecimatePolylineFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPolyData* input = svtkPolyData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkCellArray* inputLines = input->GetLines();
  svtkPoints* inputPoints = input->GetPoints();

  svtkDebugMacro("Decimating polylines");

  if (!inputLines || !inputPoints)
  {
    return 1;
  }
  svtkIdType numLines = inputLines->GetNumberOfCells();
  svtkIdType numPts = inputPoints->GetNumberOfPoints();
  if (numLines < 1 || numPts < 1)
  {
    return 1;
  }

  // Allocate memory and prepare for data processing
  svtkPoints* newPts = svtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    newPts->SetDataType(inputPoints->GetDataType());
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(SVTK_DOUBLE);
  }

  svtkCellArray* newLines = svtkCellArray::New();
  newLines->AllocateExact(numLines, numLines * 2);
  svtkPointData* inPD = input->GetPointData();
  svtkPointData* outPD = output->GetPointData();
  svtkCellData* inCD = input->GetCellData();
  svtkCellData* outCD = output->GetCellData();
  outPD->CopyAllocate(inPD);
  outCD->CopyAllocate(inCD);

  auto lineIter = svtkSmartPointer<svtkCellArrayIterator>::Take(inputLines->NewIterator());
  svtkIdType firstVertexIndex = 0;

  svtkIdType polylineSize = 0;
  const svtkIdType* polyLineVerts;

  std::map<svtkIdType, svtkIdType> pointIdMap;
  // Decimate each polyline (represented as a single cell) in series
  for (lineIter->GoToFirstCell(); !lineIter->IsDoneWithTraversal();
       lineIter->GoToNextCell(), firstVertexIndex += polylineSize)
  {
    lineIter->GetCurrentCell(polylineSize, polyLineVerts);

    // construct a polyline as a doubly linked list
    svtkDecimatePolylineFilter::Polyline* polyline =
      new svtkDecimatePolylineFilter::Polyline(polyLineVerts, polylineSize);

    double error;
    for (svtkIdType vertexIdx = 0; vertexIdx < polyline->Size; ++vertexIdx)
    {
      // only vertices that are removable have associated error values
      if (polyline->Vertices[vertexIdx].removable)
      {
        error = this->ComputeError(input, polyline, vertexIdx);
        if (error <= this->MaximumError)
        {
          this->PriorityQueue->Insert(error, vertexIdx);
        }
      }
    }

    // Now process structures,
    // deleting vertices until the decimation target is met.
    svtkIdType currentNumPts = polylineSize;
    while (1.0 - (static_cast<double>(currentNumPts) / static_cast<double>(polylineSize)) <
        this->TargetReduction &&
      ((polyline->IsLoop == false && currentNumPts > 2) ||
        (polyline->IsLoop == true && currentNumPts > 3)))
    {
      svtkIdType poppedIdx = this->PriorityQueue->Pop();
      if (poppedIdx < 0)
      {
        break; // all points are exhausted, get out
      }

      --currentNumPts;
      polyline->Remove(poppedIdx);
      svtkIdType prevIdx = polyline->Vertices[poppedIdx].prev->index;
      svtkIdType nextIdx = polyline->Vertices[poppedIdx].next->index;

      // again, only vertices that are removable have associated error values
      if (polyline->Vertices[poppedIdx].prev->removable)
      {
        error = this->ComputeError(input, polyline, prevIdx);
        this->PriorityQueue->DeleteId(prevIdx);
        if (error <= this->MaximumError)
        {
          this->PriorityQueue->Insert(error, prevIdx);
        }
      }

      if (polyline->Vertices[poppedIdx].next->removable)
      {
        error = this->ComputeError(input, polyline, nextIdx);
        this->PriorityQueue->DeleteId(nextIdx);
        if (error <= this->MaximumError)
        {
          this->PriorityQueue->Insert(error, nextIdx);
        }
      }
    }

    // What's left over is now spit out as a new polyline
    svtkIdType newId = newLines->InsertNextCell(currentNumPts);
    outCD->CopyData(inCD, firstVertexIndex, newId);

    std::map<svtkIdType, svtkIdType>::iterator it;

    Polyline::Vertex* vertex = &(polyline->Vertices[0]);
    while (vertex != nullptr)
    {
      // points that are repeated within polylines are represented by
      // only one point instance
      it = pointIdMap.find(vertex->id);
      if (it == pointIdMap.end())
      {
        newId = newPts->InsertNextPoint(inputPoints->GetPoint(vertex->id));
        newLines->InsertCellPoint(newId);
        outPD->CopyData(inPD, vertex->id, newId);
        pointIdMap[vertex->id] = newId;
      }
      else
      {
        newLines->InsertCellPoint(it->second);
      }

      vertex = vertex->next;
    }

    delete polyline;
    this->PriorityQueue->Reset();
  }

  // Create output and clean up
  output->SetPoints(newPts);
  output->SetLines(newLines);

  newLines->Delete();
  newPts->Delete();

  return 1;
}
//---------------------------------------------------------------------
void svtkDecimatePolylineFilter::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Target Reduction: " << this->TargetReduction << "\n";
  os << indent << "Maximum Error: " << this->MaximumError << "\n";
  os << indent << "Output Points Precision: " << this->OutputPointsPrecision << "\n";
}
