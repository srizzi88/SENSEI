/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStructuredGridOutlineFilter.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkStructuredGridOutlineFilter.h"

#include "svtkCellArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkObjectFactory.h"
#include "svtkPolyData.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkStructuredGrid.h"

svtkStandardNewMacro(svtkStructuredGridOutlineFilter);

//----------------------------------------------------------------------------
// ComputeDivisionExtents has done most of the work for us.
// Now just connect the points.
int svtkStructuredGridOutlineFilter::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkStructuredGrid* input =
    svtkStructuredGrid::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  int *ext, *wExt, cExt[6];
  int xInc, yInc, zInc;
  svtkPoints* inPts;
  svtkPoints* newPts;
  svtkCellArray* newLines;
  int idx;
  svtkIdType ids[2], numPts, offset;
  // for marching through the points along an edge.
  svtkIdType start = 0, id;
  int num = 0, inc = 0;
  int edgeFlag;
  int i;

  inPts = input->GetPoints();
  if (!inPts)
  {
    return 1;
  }

  newLines = svtkCellArray::New();
  newPts = svtkPoints::New();

  ext = input->GetExtent();
  wExt = inInfo->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT());

  // Since it is possible that the extent is larger than the whole extent,
  // and we want the outline to be the whole extent,
  // compute the clipped extent.
  memcpy(cExt, ext, 6 * sizeof(int));
  for (i = 0; i < 3; ++i)
  {
    if (cExt[2 * i] < wExt[2 * i])
    {
      cExt[2 * i] = wExt[2 * i];
    }
    if (cExt[2 * i + 1] > wExt[2 * i + 1])
    {
      cExt[2 * i + 1] = wExt[2 * i + 1];
    }
  }

  for (i = 0; i < 12; i++)
  {
    // Find the start of this edge, the length of this edge, and increment.
    xInc = 1;
    yInc = ext[1] - ext[0] + 1;
    zInc = yInc * (ext[3] - ext[2] + 1);
    edgeFlag = 0;
    switch (i)
    {
      case 0:
        // start (0, 0, 0) increment z axis.
        if (cExt[0] <= wExt[0] && cExt[2] <= wExt[2])
        {
          edgeFlag = 1;
        }
        num = cExt[5] - cExt[4] + 1;
        start = (cExt[0] - ext[0]) * xInc + (cExt[2] - ext[2]) * yInc + (cExt[4] - ext[4]) * zInc;
        inc = zInc;
        break;
      case 1:
        // start (xMax, 0, 0) increment z axis.
        if (cExt[1] >= wExt[1] && cExt[2] <= wExt[2])
        {
          edgeFlag = 1;
        }
        num = cExt[5] - cExt[4] + 1;
        start = (cExt[1] - ext[0]) * xInc + (cExt[2] - ext[2]) * yInc + (cExt[4] - ext[4]) * zInc;
        inc = zInc;
        break;
      case 2:
        // start (0, yMax, 0) increment z axis.
        if (cExt[0] <= wExt[0] && cExt[3] >= wExt[3])
        {
          edgeFlag = 1;
        }
        num = cExt[5] - cExt[4] + 1;
        start = (cExt[0] - ext[0]) * xInc + (cExt[3] - ext[2]) * yInc + (cExt[4] - ext[4]) * zInc;
        inc = zInc;
        break;
      case 3:
        // start (xMax, yMax, 0) increment z axis.
        if (cExt[1] >= wExt[1] && cExt[3] >= wExt[3])
        {
          edgeFlag = 1;
        }
        num = cExt[5] - cExt[4] + 1;
        start = (cExt[1] - ext[0]) * xInc + (cExt[3] - ext[2]) * yInc + (cExt[4] - ext[4]) * zInc;
        inc = zInc;
        break;
      case 4:
        // start (0, 0, 0) increment y axis.
        if (cExt[0] <= wExt[0] && cExt[4] <= wExt[4])
        {
          edgeFlag = 1;
        }
        num = cExt[3] - cExt[2] + 1;
        start = (cExt[0] - ext[0]) * xInc + (cExt[2] - ext[2]) * yInc + (cExt[4] - ext[4]) * zInc;
        inc = yInc;
        break;
      case 5:
        // start (xMax, 0, 0) increment y axis.
        if (cExt[1] >= wExt[1] && cExt[4] <= wExt[4])
        {
          edgeFlag = 1;
        }
        num = cExt[3] - cExt[2] + 1;
        start = (cExt[1] - ext[0]) * xInc + (cExt[2] - ext[2]) * yInc + (cExt[4] - ext[4]) * zInc;
        inc = yInc;
        break;
      case 6:
        // start (0, 0, zMax) increment y axis.
        if (cExt[0] <= wExt[0] && cExt[5] >= wExt[5])
        {
          edgeFlag = 1;
        }
        num = cExt[3] - cExt[2] + 1;
        start = (cExt[0] - ext[0]) * xInc + (cExt[2] - ext[2]) * yInc + (cExt[5] - ext[4]) * zInc;
        inc = yInc;
        break;
      case 7:
        // start (xMax, 0, zMax) increment y axis.
        if (cExt[1] >= wExt[1] && cExt[5] >= wExt[5])
        {
          edgeFlag = 1;
        }
        num = cExt[3] - cExt[2] + 1;
        start = (cExt[1] - ext[0]) * xInc + (cExt[2] - ext[2]) * yInc + (cExt[5] - ext[4]) * zInc;
        inc = yInc;
        break;
      case 8:
        // start (0, 0, 0) increment x axis.
        if (cExt[2] <= wExt[2] && cExt[4] <= wExt[4])
        {
          edgeFlag = 1;
        }
        num = cExt[1] - cExt[0] + 1;
        start = (cExt[0] - ext[0]) * xInc + (cExt[2] - ext[2]) * yInc + (cExt[4] - ext[4]) * zInc;
        inc = xInc;
        break;
      case 9:
        // start (0, yMax, 0) increment x axis.
        if (cExt[3] >= wExt[3] && cExt[4] <= wExt[4])
        {
          edgeFlag = 1;
        }
        num = cExt[1] - cExt[0] + 1;
        start = (cExt[0] - ext[0]) * xInc + (cExt[3] - ext[2]) * yInc + (cExt[4] - ext[4]) * zInc;
        inc = xInc;
        break;
      case 10:
        // start (0, 0, zMax) increment x axis.
        if (cExt[2] <= wExt[2] && cExt[5] >= wExt[5])
        {
          edgeFlag = 1;
        }
        num = cExt[1] - cExt[0] + 1;
        start = (cExt[0] - ext[0]) * xInc + (cExt[2] - ext[2]) * yInc + (cExt[5] - ext[4]) * zInc;
        inc = xInc;
        break;
      case 11:
        // start (0, yMax, zMax) increment x axis.
        if (cExt[3] >= wExt[3] && cExt[5] >= wExt[5])
        {
          edgeFlag = 1;
        }
        num = cExt[1] - cExt[0] + 1;
        start = (cExt[0] - ext[0]) * xInc + (cExt[3] - ext[2]) * yInc + (cExt[5] - ext[4]) * zInc;
        inc = xInc;
        break;
    }

    if (edgeFlag && num > 1)
    {
      offset = newPts->GetNumberOfPoints();
      numPts = inPts->GetNumberOfPoints();

      // add points
      for (idx = 0; idx < num; ++idx)
      {
        id = start + idx * inc;
        // sanity check
        if (id < 0 || id >= numPts)
        {
          svtkErrorMacro("Error stepping through points.");
          return 0;
        }
        newPts->InsertNextPoint(inPts->GetPoint(id));
      }

      // add lines
      for (idx = 1; idx < num; ++idx)
      {
        ids[0] = idx + offset - 1;
        ids[1] = idx + offset;
        newLines->InsertNextCell(2, ids);
      }
    }
  }

  output->SetPoints(newPts);
  newPts->Delete();
  output->SetLines(newLines);
  newLines->Delete();

  return 1;
}

int svtkStructuredGridOutlineFilter::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkStructuredGrid");
  return 1;
}
