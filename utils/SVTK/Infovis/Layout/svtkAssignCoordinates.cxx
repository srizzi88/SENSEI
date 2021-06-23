/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssignCoordinates.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkAssignCoordinates.h"

#include "svtkGraph.h"
#include "svtkIdTypeArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"

svtkStandardNewMacro(svtkAssignCoordinates);

svtkAssignCoordinates::svtkAssignCoordinates()
{
  this->XCoordArrayName = nullptr;
  this->YCoordArrayName = nullptr;
  this->ZCoordArrayName = nullptr;

  this->Jitter = false;
}

svtkAssignCoordinates::~svtkAssignCoordinates()
{
  delete[] this->XCoordArrayName;
  delete[] this->YCoordArrayName;
  delete[] this->ZCoordArrayName;
}

int svtkAssignCoordinates::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{

  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkDataObject* input = inInfo->Get(svtkDataObject::DATA_OBJECT());
  svtkDataObject* output = outInfo->Get(svtkDataObject::DATA_OBJECT());

  // Do a shallow copy of the input to the output
  output->ShallowCopy(input);

  // Create new points on the output
  svtkDataSetAttributes* data = nullptr;
  svtkPoints* pts = svtkPoints::New();
  if (svtkPointSet::SafeDownCast(input))
  {
    svtkPointSet* psInput = svtkPointSet::SafeDownCast(input);
    svtkPointSet* psOutput = svtkPointSet::SafeDownCast(output);
    pts->DeepCopy(psInput->GetPoints());
    psOutput->SetPoints(pts);
    pts->Delete();
    data = psOutput->GetPointData();
  }
  else if (svtkGraph::SafeDownCast(input))
  {
    svtkGraph* graphInput = svtkGraph::SafeDownCast(input);
    svtkGraph* graphOutput = svtkGraph::SafeDownCast(output);
    pts->DeepCopy(graphInput->GetPoints());
    graphOutput->SetPoints(pts);
    pts->Delete();
    data = graphOutput->GetVertexData();
  }
  else
  {
    svtkErrorMacro(<< "Input must be graph or point set.");
    return 0;
  }

  // I need at least one coordinate array
  if (!this->XCoordArrayName || strlen(XCoordArrayName) == 0)
  {
    return 0;
  }

  // Okay now check for coordinate arrays
  svtkDataArray* XArray = data->GetArray(this->XCoordArrayName);

  // Does the array exist at all?
  if (XArray == nullptr)
  {
    svtkErrorMacro("Could not find array named " << this->XCoordArrayName);
    return 0;
  }

  // Y coordinate array
  svtkDataArray* YArray = nullptr;
  if (this->YCoordArrayName && strlen(YCoordArrayName) > 0)
  {
    YArray = data->GetArray(this->YCoordArrayName);

    // Does the array exist at all?
    if (YArray == nullptr)
    {
      svtkErrorMacro("Could not find array named " << this->YCoordArrayName);
      return 0;
    }
  }

  // Z coordinate array
  svtkDataArray* ZArray = nullptr;
  if (this->ZCoordArrayName && strlen(ZCoordArrayName) > 0)
  {
    ZArray = data->GetArray(this->ZCoordArrayName);

    // Does the array exist at all?
    if (ZArray == nullptr)
    {
      svtkErrorMacro("Could not find array named " << this->ZCoordArrayName);
      return 0;
    }
  }

  // Generate the points, either x,0,0 or x,y,0 or x,y,z
  int numPts = pts->GetNumberOfPoints();
  for (int i = 0; i < numPts; i++)
  {
    double rx, ry, rz;
    if (Jitter)
    {
      rx = svtkMath::Random() - .5;
      ry = svtkMath::Random() - .5;
      rz = svtkMath::Random() - .5;
      rx *= .02;
      ry *= .02;
      rz *= .02;
    }
    else
    {
      rx = ry = rz = 0;
    }
    if (YArray)
    {
      if (ZArray)
      {
        pts->SetPoint(
          i, XArray->GetTuple1(i) + rx, YArray->GetTuple1(i) + ry, ZArray->GetTuple1(i) + rz);
      }
      else
      {
        pts->SetPoint(i, XArray->GetTuple1(i) + rx, YArray->GetTuple1(i) + ry, 0);
      }
    }
    else
    {
      pts->SetPoint(i, XArray->GetTuple1(i) + rx, 0, 0);
    }
  }

  return 1;
}

int svtkAssignCoordinates::FillInputPortInformation(int svtkNotUsed(port), svtkInformation* info)
{
  // This algorithm may accept a svtkPointSet or svtkGraph.
  info->Remove(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE());
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkPointSet");
  info->Append(svtkAlgorithm::INPUT_REQUIRED_DATA_TYPE(), "svtkGraph");
  return 1;
}

void svtkAssignCoordinates::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "XCoordArrayName: " << (this->XCoordArrayName ? this->XCoordArrayName : "(none)")
     << endl;

  os << indent << "YCoordArrayName: " << (this->YCoordArrayName ? this->YCoordArrayName : "(none)")
     << endl;

  os << indent << "ZCoordArrayName: " << (this->ZCoordArrayName ? this->ZCoordArrayName : "(none)")
     << endl;

  os << indent << "Jitter: " << (this->Jitter ? "True" : "False") << endl;
}
