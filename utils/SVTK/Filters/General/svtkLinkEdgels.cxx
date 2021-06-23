/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLinkEdgels.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLinkEdgels.h"

#include "svtkCellArray.h"
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"

svtkStandardNewMacro(svtkLinkEdgels);

// Construct instance of svtkLinkEdgels with GradientThreshold set to
// 0.1, PhiThreshold set to 90 degrees and LinkThreshold set to 90 degrees.
svtkLinkEdgels::svtkLinkEdgels()
{
  this->GradientThreshold = 0.1;
  this->PhiThreshold = 90;
  this->LinkThreshold = 90;
}

int svtkLinkEdgels::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkImageData* input = svtkImageData::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPolyData* output = svtkPolyData::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  svtkPointData* pd;
  svtkPoints* newPts;
  svtkCellArray* newLines;
  svtkDoubleArray* inScalars;
  svtkDoubleArray* outScalars;
  svtkDoubleArray* outVectors;
  int* dimensions;
  double *CurrMap, *inDataPtr;
  svtkDataArray* inVectors;
  int ptId;

  svtkDebugMacro(<< "Extracting structured points geometry");

  pd = input->GetPointData();
  dimensions = input->GetDimensions();
  inScalars = svtkArrayDownCast<svtkDoubleArray>(pd->GetScalars());
  inVectors = pd->GetVectors();
  if ((input->GetNumberOfPoints()) < 2 || inScalars == nullptr)
  {
    svtkErrorMacro(<< "No data to transform (or wrong data type)!");
    return 1;
  }

  // set up the input
  inDataPtr = inScalars->GetPointer(0);

  // Finally do edge following to extract the edge data from the Thin image
  newPts = svtkPoints::New();
  newLines = svtkCellArray::New();
  outScalars = svtkDoubleArray::New();
  outVectors = svtkDoubleArray::New();
  outVectors->SetNumberOfComponents(3);

  svtkDebugMacro("doing edge linking\n");
  //
  // Traverse all points, for each point find Gradient in the Image map.
  //
  for (ptId = 0; ptId < dimensions[2]; ptId++)
  {
    CurrMap = inDataPtr + dimensions[0] * dimensions[1] * ptId;

    this->LinkEdgels(dimensions[0], dimensions[1], CurrMap, inVectors, newLines, newPts, outScalars,
      outVectors, ptId);
  }

  output->SetPoints(newPts);
  output->SetLines(newLines);

  // Update ourselves
  //  outScalars->ComputeRange();
  output->GetPointData()->SetScalars(outScalars);
  output->GetPointData()->SetVectors(outVectors);

  newPts->Delete();
  newLines->Delete();
  outScalars->Delete();
  outVectors->Delete();

  return 1;
}

// This method links the edges for one image.
void svtkLinkEdgels::LinkEdgels(int xdim, int ydim, double* image, svtkDataArray* inVectors,
  svtkCellArray* newLines, svtkPoints* newPts, svtkDoubleArray* outScalars, svtkDoubleArray* outVectors,
  int z)
{
  int** forward;
  int** backward;
  int x, y, ypos, zpos;
  int currX, currY, i;
  int newX, newY;
  double vec[3], vec1[3], vec2[3];
  double linkThresh, phiThresh;
  // these direction vectors are rotated 90 degrees
  // to convert gradient direction into edgel direction
  static const double directions[8][2] = { { 0, 1 }, { -0.707, 0.707 }, { -1, 0 },
    { -0.707, -0.707 }, { 0, -1 }, { 0.707, -0.707 }, { 1, 0 }, { 0.707, 0.707 } };
  static const int xoffset[8] = { 1, 1, 0, -1, -1, -1, 0, 1 };
  static const int yoffset[8] = { 0, 1, 1, 1, 0, -1, -1, -1 };
  int length, start;
  int bestDirection = 0;
  double error, bestError;

  forward = new int*[ydim];
  backward = new int*[ydim];
  for (i = 0; i < ydim; i++)
  {
    forward[i] = new int[xdim];
    backward[i] = new int[xdim];
    memset(forward[i], 0, xdim * sizeof(int));
    memset(backward[i], 0, xdim * sizeof(int));
  }

  zpos = z * xdim * ydim;
  linkThresh = cos(this->LinkThreshold * svtkMath::Pi() / 180.0);
  phiThresh = cos(this->PhiThreshold * svtkMath::Pi() / 180.0);

  // first find all forward & backwards links
  for (y = 0; y < ydim; y++)
  {
    ypos = y * xdim;
    for (x = 0; x < xdim; x++)
    {
      // find forward and backward neighbor for this pixel
      // if its value is less than threshold then ignore it
      if (image[x + ypos] < this->GradientThreshold)
      {
        forward[y][x] = -1;
        backward[y][x] = -1;
      }
      else
      {
        // try all neighbors as forward, first try four connected
        inVectors->GetTuple(x + ypos + zpos, vec1);
        svtkMath::Normalize(vec1);
        // first eliminate based on phi1 - alpha
        bestError = 0;
        for (i = 0; i < 8; i += 2)
        {
          // make sure it passes the linkThresh test
          if ((directions[i][0] * vec1[0] + directions[i][1] * vec1[1]) >= linkThresh)
          {
            // make sure we don't go off the edge and are >= GradientThresh
            // and it hasn't already been set
            if ((x + xoffset[i] >= 0) && (x + xoffset[i] < xdim) && (y + yoffset[i] >= 0) &&
              (y + yoffset[i] < ydim) && (!backward[y + yoffset[i]][x + xoffset[i]]) &&
              (image[x + xoffset[i] + (y + yoffset[i]) * xdim] >= this->GradientThreshold))
            {
              // satisfied the first test, now check second
              inVectors->GetTuple(x + xoffset[i] + (y + yoffset[i]) * xdim + zpos, vec2);
              svtkMath::Normalize(vec2);
              if ((vec1[0] * vec2[0] + vec1[1] * vec2[1]) >= phiThresh)
              {
                // passed phi - phi test does the forward neighbor
                // pass the link test
                if ((directions[i][0] * vec2[0] + directions[i][1] * vec2[1]) >= linkThresh)
                {
                  // check against the current best solution
                  error = (directions[i][0] * vec2[0] + directions[i][1] * vec2[1]) +
                    (directions[i][0] * vec1[0] + directions[i][1] * vec1[1]) +
                    (vec1[0] * vec2[0] + vec1[1] * vec2[1]);
                  if (error > bestError)
                  {
                    bestDirection = i;
                    bestError = error;
                  }
                }
              }
            }
          }
        }
        if (bestError > 0)
        {
          forward[y][x] = (bestDirection + 1);
          backward[y + yoffset[bestDirection]][x + xoffset[bestDirection]] =
            ((bestDirection + 4) % 8) + 1;
        }
        else
        {
          // check the eight connected neighbors now
          for (i = 1; i < 8; i += 2)
          {
            // make sure it passes the linkThresh test
            if ((directions[i][0] * vec1[0] + directions[i][1] * vec1[1]) >= linkThresh)
            {
              // make sure we don't go off the edge and are >= GradientThresh
              // and it hasn't already been set
              if ((x + xoffset[i] >= 0) && (x + xoffset[i] < xdim) && (y + yoffset[i] >= 0) &&
                (y + yoffset[i] < ydim) && (!backward[y + yoffset[i]][x + xoffset[i]]) &&
                (image[x + xoffset[i] + (y + yoffset[i]) * xdim] >= this->GradientThreshold))
              {
                // satisfied the first test, now check second
                inVectors->GetTuple(x + xoffset[i] + (y + yoffset[i]) * xdim + zpos, vec2);
                svtkMath::Normalize(vec2);
                if ((vec1[0] * vec2[0] + vec1[1] * vec2[1]) >= phiThresh)
                {
                  // passed phi - phi test does the forward neighbor
                  // pass the link test
                  if ((directions[i][0] * vec2[0] + directions[i][1] * vec2[1]) >= linkThresh)
                  {
                    // check against the current best solution
                    error = (directions[i][0] * vec2[0] + directions[i][1] * vec2[1]) +
                      (directions[i][0] * vec1[0] + directions[i][1] * vec1[1]) +
                      (vec1[0] * vec2[0] + vec1[1] * vec2[1]);
                    if (error > bestError)
                    {
                      bestDirection = i;
                      bestError = error;
                    }
                  }
                }
              }
            }
          }
          if (bestError > 0)
          {
            forward[y][x] = (bestDirection + 1);
            backward[y + yoffset[bestDirection]][x + xoffset[bestDirection]] =
              ((bestDirection + 4) % 8) + 1;
          }
        }
      }
    }
  }

  // now construct the chains
  vec[2] = z;
  for (y = 0; y < ydim; y++)
  {
    for (x = 0; x < xdim; x++)
    {
      // do we have part of an edgel chain ?
      // isolated edgels do not qualify
      if (backward[y][x] > 0)
      {
        // trace back to the beginning
        currX = x;
        currY = y;
        do
        {
          newX = currX + xoffset[backward[currY][currX] - 1];
          currY += yoffset[backward[currY][currX] - 1];
          currX = newX;
        } while ((currX != x || currY != y) && backward[currY][currX]);

        // now trace to the end and build the digital curve
        length = 0;
        start = outScalars->GetNumberOfTuples();
        newX = currX;
        newY = currY;
        do
        {
          currX = newX;
          currY = newY;
          outScalars->InsertNextTuple(&(image[currX + currY * xdim]));
          inVectors->GetTuple(currX + currY * xdim + zpos, vec2);
          svtkMath::Normalize(vec2);
          outVectors->InsertNextTuple(vec2);
          vec[0] = currX;
          vec[1] = currY;
          newPts->InsertNextPoint(vec);
          length++;

          // if there is a next pixel select it
          if (forward[currY][currX])
          {
            newX = currX + xoffset[forward[currY][currX] - 1];
            newY = currY + yoffset[forward[currY][currX] - 1];
          }
          // clear out this edgel now that were done with it
          backward[newY][newX] = 0;
          forward[currY][currX] = 0;
        } while ((currX != newX || currY != newY));

        // build up the cell
        newLines->InsertNextCell(length);
        for (i = 0; i < length; i++)
        {
          newLines->InsertCellPoint(start);
          start++;
        }
      }
    }
  }

  // free up the memory
  for (i = 0; i < ydim; i++)
  {
    delete[] forward[i];
    delete[] backward[i];
  }
  delete[] forward;
  delete[] backward;
}

int svtkLinkEdgels::FillInputPortInformation(int, svtkInformation* info)
{
  info->Set(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkImageData");
  return 1;
}

void svtkLinkEdgels::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "GradientThreshold:" << this->GradientThreshold << "\n";
  os << indent << "LinkThreshold:" << this->LinkThreshold << "\n";
  os << indent << "PhiThreshold:" << this->PhiThreshold << "\n";
}
