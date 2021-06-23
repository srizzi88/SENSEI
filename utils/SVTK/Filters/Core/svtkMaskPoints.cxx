/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMaskPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkMaskPoints.h"

#include "svtkCellArray.h"
#include "svtkDataSet.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"

#include <cstdlib>

namespace
{

//-----------------------------------------------------------------------------
inline double d_rand()
{
  return rand() / (double)((unsigned long)RAND_MAX + 1);
}

//-----------------------------------------------------------------------------
inline void SwapPoint(
  svtkPoints* points, svtkPointData* data, svtkPointData* temp, svtkIdType a, svtkIdType b)
{
  double ta[3];

  // a -> temp
  points->GetPoint(a, ta);
  temp->CopyData(data, a, 0);

  // b -> a
  points->SetPoint(a, points->GetPoint(b));
  data->CopyData(data, b, a);

  // temp -> b
  points->SetPoint(b, ta);
  data->CopyData(temp, 0, b);
}

//-----------------------------------------------------------------------------
// AKA select, quickselect, nth_element:
// this is average case linear, worse case quadratic implementation
// (i.e., just like quicksort) -- there is the median of 5 or
// median of medians algorithm, but I'm too lazy to implement it
static void QuickSelect(svtkPoints* points, svtkPointData* data, svtkPointData* temp, svtkIdType start,
  svtkIdType end, svtkIdType nth, int axis)
{
  // base case
  if (end - start < 2)
  {
    return;
  }

  // pick a pivot
  svtkIdType pivot = (svtkIdType)(rand() % (end - start)) + start;
  double value = points->GetPoint(pivot)[axis];

  // swap the pivot to end
  end = end - 1;
  SwapPoint(points, data, temp, pivot, end);

  // partition by pivot
  svtkIdType left = start;
  int allequal = 1;
  for (svtkIdType i = start; i < end; i = i + 1)
  {
    allequal = allequal && (points->GetPoint(i)[axis] == value);

    if (points->GetPoint(i)[axis] < value)
    {
      SwapPoint(points, data, temp, i, left);
      left = left + 1;
    }
  }

  // swap pivot to correct position
  SwapPoint(points, data, temp, left, end);
  end = end + 1;

  // recurse if we didn't find it
  if ((left != nth) && !allequal)
  {
    if (left < nth) // it's in the right half
    {
      QuickSelect(points, data, temp, left, end, nth, axis);
    }
    else // it's in the left half
    {
      QuickSelect(points, data, temp, start, left, nth, axis);
    }
  }
}

//-----------------------------------------------------------------------------
// divide the data into sampling strata and randomly sample it
// (one sample per stratum)
static void SortAndSample(svtkPoints* points, svtkPointData* data, svtkPointData* temp,
  svtkIdType start, svtkIdType end, svtkIdType size, int depth)
{
  // if size >= end - start return them all
  if (size >= (end - start))
  {
    return;
  }

  // if size == 1 return it (get one sample from a stratum)
  if (size < 2)
  {
    svtkIdType pick = (svtkIdType)(rand() % (end - start)) + start;
    SwapPoint(points, data, temp, start, pick);
    return;
  }

  // do median split of left and right (AKA select, quickselect, nth_element)
  svtkIdType half = start + (end - start) / 2;
  int bigger = 0;
  // randomly make one side bigger if it doesn't split evenly
  if ((end - start) % 2)
  {
    if (rand() % 2)
    {
      bigger = 1;
      half = half + 1;
    }
    else
    {
      bigger = 2;
    }
  }

  QuickSelect(points, data, temp, start, end, half, depth % 3);

  // sample the left and right halves
  svtkIdType leftsize, rightsize;
  if (size % 2)
  {
    if (bigger)
    {
      if (bigger == 1)
      {
        leftsize = size / 2 + 1;
        rightsize = size / 2;
      }
      else
      {
        leftsize = size / 2;
        rightsize = size / 2 + 1;
      }
    }
    // randomly make a sample size bigger if it doesn't split evenly
    else if (rand() % 2)
    {
      leftsize = size / 2 + 1;
      rightsize = size / 2;
    }
    else
    {
      leftsize = size / 2;
      rightsize = size / 2 + 1;
    }
  }
  else
  {
    leftsize = size / 2;
    rightsize = size / 2;
  }

  // get samples from children
  SortAndSample(points, data, temp, start, half, leftsize, depth + 1);
  SortAndSample(points, data, temp, half, end, rightsize, depth + 1);

  // combine the two halves
  for (svtkIdType i = 0; i < rightsize; i = i + 1)
  {
    SwapPoint(points, data, temp, start + leftsize + i, half + i);
  }
}

}

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkMaskPoints);

//-----------------------------------------------------------------------------
svtkMaskPoints::svtkMaskPoints()
{
  this->OnRatio = 2;
  this->Offset = 0;
  this->RandomMode = 0;
  this->MaximumNumberOfPoints = SVTK_ID_MAX;
  this->GenerateVertices = 0;
  this->SingleVertexPerCell = 0;
  this->RandomModeType = 0;
  this->ProportionalMaximumNumberOfPoints = 0;
  this->OutputPointsPrecision = DEFAULT_PRECISION;
}

//-----------------------------------------------------------------------------
unsigned long svtkMaskPoints::GetLocalSampleSize(svtkIdType numPts, int np)
{
  // send number of points to process 0
  unsigned long send = (unsigned long)numPts;
  unsigned long* recv = new unsigned long[np];
  this->InternalGather(&send, recv, 1, 0);

  // process 0 figures it out
  unsigned long* dist = new unsigned long[np];
  if (this->InternalGetLocalProcessId() == 0)
  {
    // sum them
    unsigned long total = 0;
    for (int i = 0; i < np; i = i + 1)
    {
      total = total + recv[i];
    }
    // find the number of current processing points
    svtkIdType totalInVtkIdType = static_cast<svtkIdType>(total);
    svtkIdType numberOfProcessingPoints = std::min(this->MaximumNumberOfPoints, totalInVtkIdType);
    // if it's greater than 0
    if (total > 0)
    {
      // each process gets a proportional fraction
      svtkIdType left = numberOfProcessingPoints;
      double ratio = numberOfProcessingPoints / (double)total;
      for (int i = 0; i < np; i = i + 1)
      {
        dist[i] = (unsigned long)(recv[i] * ratio);
        left = left - dist[i];
      }

      // if it didn't evenly divide, we need to assign the remaining
      // samples -- doing it completely randomly, though probably a better
      // way is to weight the randomness by the size of the remaining fraction
      if (left > 0)
      {
        unsigned long* rem = new unsigned long[np];

        for (int i = 0; i < np; i = i + 1)
        {
          rem[i] = i < left ? 1 : 0;
        }

        for (int i = 0; i < np; i = i + 1)
        {
          svtkIdType index = (svtkIdType)(rand() % np);
          unsigned long temp = rem[index];
          rem[index] = rem[i];
          rem[i] = temp;
        }

        for (int i = 0; i < np; i = i + 1)
        {
          dist[i] = dist[i] + rem[i];
        }

        delete[] rem;
      }
    }
    // no points
    else
    {
      for (int i = 0; i < np; i = i + 1)
      {
        dist[i] = 0;
      }
    }
  }

  // process 0 sends the fraction to each process
  this->InternalScatter(dist, recv, 1, 0);
  unsigned long retval = (svtkIdType)recv[0];

  delete[] dist;
  delete[] recv;

  return retval;
}

//----------------------------------------------------------------------------
int svtkMaskPoints::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataSet* input = svtkDataSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPoints* newPts;
  svtkPointData* pd = input->GetPointData();
  svtkIdType numNewPts;
  double x[3];
  svtkIdType ptId, id = 0;
  svtkPointData* outputPD = output->GetPointData();
  svtkIdType numPts = input->GetNumberOfPoints();

  int abort = 0;

  // figure out how many sample points per process
  svtkIdType localMaxPts;
  // Make sure this does not exceed the number of points in the input array
  localMaxPts = this->MaximumNumberOfPoints > numPts ? numPts : this->MaximumNumberOfPoints;
  if (this->InternalGetNumberOfProcesses() > 1 && this->ProportionalMaximumNumberOfPoints)
  {
    localMaxPts = this->GetLocalSampleSize(numPts, this->InternalGetNumberOfProcesses());
  }

  // make sure new points aren't too big
  numNewPts = numPts / this->OnRatio;
  numNewPts = numNewPts > numPts ? numPts : numNewPts;
  if (numNewPts > localMaxPts || this->RandomMode)
  {
    numNewPts = localMaxPts;
  }

  bool hasPoints = numPts > 0 && numNewPts > 0;
  // Split the controller between ranks that have some points and the others
  this->InternalSplitController(hasPoints ? 1 : 0, this->InternalGetLocalProcessId());

  if (!hasPoints)
  {
    // Ranks that don't have any points can leave now
    this->InternalResetController();
    return 1;
  }

  svtkDebugMacro(<< "Masking points");

  // Allocate space
  newPts = svtkPoints::New();

  // Set the desired precision for the points in the output.
  if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
  {
    svtkPointSet* inputPointSet = svtkPointSet::SafeDownCast(input);
    if (inputPointSet)
    {
      newPts->SetDataType(inputPointSet->GetPoints()->GetDataType());
    }
    else
    {
      newPts->SetDataType(SVTK_FLOAT);
    }
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
  {
    newPts->SetDataType(SVTK_FLOAT);
  }
  else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
  {
    newPts->SetDataType(SVTK_DOUBLE);
  }

  newPts->Allocate(numNewPts);
  // Mask points preserves all attributes of the points, so copy all of them.
  outputPD->CopyAllOn();
  outputPD->CopyAllocate(pd, numNewPts);

  // Traverse points and copy
  svtkIdType progressInterval = numPts / 20 + 1;
  if (this->RandomMode) // random modes
  {
    if (this->RandomModeType == 0)
    {
      // original random mode
      double cap;

      if ((static_cast<double>(numPts) / this->OnRatio) > localMaxPts)
      {
        cap = 2.0 * numPts / localMaxPts - 1;
      }
      else
      {
        cap = 2.0 * this->OnRatio - 1;
      }

      for (ptId = this->Offset; (ptId < numPts) && (id < localMaxPts) && !abort;
           ptId += (1 + static_cast<int>(static_cast<double>(svtkMath::Random()) * cap)))
      {
        input->GetPoint(ptId, x);
        id = newPts->InsertNextPoint(x);
        outputPD->CopyData(pd, ptId, id);
        if (!(id % progressInterval)) // abort/progress
        {
          this->UpdateProgress(0.5 * id / numPts);
          abort = this->GetAbortExecute();
        }
      }
    }
    else if (this->RandomModeType == 1)
    {
      // Vitter's algorithm D (without A)
      // for generating random samples incrementally: O(samplesize)
      ptId = -1;
      double vprime = log(d_rand());
      svtkIdType size = numPts;
      svtkIdType samplesize = localMaxPts;
      svtkIdType q1 = size - samplesize + 1;

      while (samplesize > 1)
      {
        double q2 = (q1 - 1.0) / (size - 1.0);
        double q3 = log(q2);
        svtkIdType s;

        while (1)
        {
          while (1)
          {
            s = (svtkIdType)(vprime / q3);
            if (s < q1)
            {
              break;
            }
            vprime = log(d_rand());
          }

          double lhs = log(d_rand());
          double rhs = s * (log((double)(q1 - s) / (size - s)) - q3);

          if (lhs <= rhs)
          {
            vprime = lhs - rhs;
            break;
          }

          double y = 1.0;
          svtkIdType bottom;
          svtkIdType limit;
          if (samplesize - 1 > s)
          {
            bottom = size - samplesize;
            limit = size - s;
          }
          else
          {
            bottom = size - s - 1;
            limit = q1;
          }

          for (svtkIdType top = size - 1; top >= limit; top = top - 1)
          {
            y = y * top / bottom;
            bottom = bottom - 1;
          }

          vprime = log(d_rand());
          if (q3 <= -(log(y) + lhs) / s)
          {
            break;
          }
        }

        // add a point
        ptId = ptId + s + 1;
        input->GetPoint(ptId, x);
        id = newPts->InsertNextPoint(x);
        outputPD->CopyData(pd, ptId, id);

        size = size - s - 1;
        samplesize = samplesize - 1;
        q1 = q1 - s;
      }

      // add last point
      ptId = ptId + (svtkIdType)(d_rand() * size) + 1;
      input->GetPoint(ptId, x);
      id = newPts->InsertNextPoint(x);
      outputPD->CopyData(pd, ptId, id);
    }
    else if (this->RandomModeType == 2)
    {
      // need to copy the entire data to sort it, to leave original intact
      svtkPoints* pointCopy = svtkPoints::New();

      // Set the desired precision for the points.
      if (this->OutputPointsPrecision == svtkAlgorithm::DEFAULT_PRECISION)
      {
        svtkPointSet* inputPointSet = svtkPointSet::SafeDownCast(input);
        if (inputPointSet)
        {
          pointCopy->SetDataType(inputPointSet->GetPoints()->GetDataType());
        }
        else
        {
          pointCopy->SetDataType(SVTK_FLOAT);
        }
      }
      else if (this->OutputPointsPrecision == svtkAlgorithm::SINGLE_PRECISION)
      {
        pointCopy->SetDataType(SVTK_FLOAT);
      }
      else if (this->OutputPointsPrecision == svtkAlgorithm::DOUBLE_PRECISION)
      {
        pointCopy->SetDataType(SVTK_DOUBLE);
      }

      svtkPointData* dataCopy = svtkPointData::New();
      svtkPointData* tempData = svtkPointData::New();

      pointCopy->Allocate(numPts);
      dataCopy->CopyAllOn();
      dataCopy->CopyAllocate(pd, numPts);
      for (svtkIdType i = 0; i < numPts; i = i + 1)
      {
        input->GetPoint(i, x);
        id = pointCopy->InsertNextPoint(x);
        dataCopy->CopyData(pd, i, id);
      }
      tempData->CopyAllOn();
      tempData->CopyAllocate(dataCopy, 1);

      // Woodring's spatially stratified random sampling: O(N log N)
      SortAndSample(pointCopy, dataCopy, tempData, 0, numPts, numNewPts, 0);

      // copy the results back
      for (svtkIdType i = 0; i < numNewPts; i = i + 1)
      {
        pointCopy->GetPoint(i, x);
        id = newPts->InsertNextPoint(x);
        outputPD->CopyData(dataCopy, i, id);
      }

      tempData->Delete();
      dataCopy->Delete();
      pointCopy->Delete();
    }
  }
  else // striding mode
  {
    for (ptId = this->Offset; (ptId < numPts) && (id < localMaxPts) && !abort;
         ptId += this->OnRatio)
    {
      input->GetPoint(ptId, x);
      id = newPts->InsertNextPoint(x);
      outputPD->CopyData(pd, ptId, id);
      if (!(id % progressInterval)) // abort/progress
      {
        this->UpdateProgress(0.5 * id / numPts);
        abort = this->GetAbortExecute();
      }
    }
  }

  // Generate vertices if requested
  if (this->GenerateVertices)
  {
    svtkCellArray* verts = svtkCellArray::New();
    if (this->SingleVertexPerCell)
    {
      verts->AllocateEstimate(id, 1);
    }
    else
    {
      verts->AllocateEstimate(1, id + 1);
      verts->InsertNextCell(id + 1);
    }
    for (ptId = 0; ptId < (id + 1) && !abort; ptId++)
    {
      if (!(ptId % progressInterval)) // abort/progress
      {
        this->UpdateProgress(0.5 + 0.5 * ptId / (id + 1));
        abort = this->GetAbortExecute();
      }
      if (this->SingleVertexPerCell)
      {
        verts->InsertNextCell(1, &ptId);
      }
      else
      {
        verts->InsertCellPoint(ptId);
      }
    }
    output->SetVerts(verts);
    verts->Delete();
  }

  // Update ourselves
  output->SetPoints(newPts);
  newPts->Delete();

  output->Squeeze();

  svtkDebugMacro(<< "Masked " << numPts << " original points to " << id + 1 << " points");

  this->InternalResetController();

  return 1;
}

//----------------------------------------------------------------------------
int svtkMaskPoints::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkDataSet");
  return 1;
}

//----------------------------------------------------------------------------
void svtkMaskPoints::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Generate Vertices: " << (this->GetGenerateVertices() ? "On\n" : "Off\n");
  os << indent << "SingleVertexPerCell: " << (this->GetSingleVertexPerCell() ? "On\n" : "Off\n");
  os << indent << "MaximumNumberOfPoints: " << this->GetMaximumNumberOfPoints() << "\n";
  os << indent << "On Ratio: " << this->GetOnRatio() << "\n";
  os << indent << "Offset: " << this->GetOffset() << "\n";
  os << indent << "Random Mode: " << (this->GetRandomMode() ? "On\n" : "Off\n");
  os << indent << "Random Mode Type: " << this->GetRandomModeType() << "\n";
  os << indent
     << "Proportional Maximum Number of Points: " << this->GetProportionalMaximumNumberOfPoints()
     << "\n";

  os << indent << "Output Points Precision: " << this->GetOutputPointsPrecision() << "\n";
}
