/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDistanceToCamera.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for morei nformation.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
#include "svtkDistanceToCamera.h"

#include "svtkCamera.h"
#include "svtkDoubleArray.h"
#include "svtkInformation.h"
#include "svtkInformationVector.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPointSet.h"
#include "svtkPoints.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

svtkStandardNewMacro(svtkDistanceToCamera);

svtkDistanceToCamera::svtkDistanceToCamera()
{
  this->Renderer = nullptr;
  this->ScreenSize = 5.0;
  this->Scaling = false;
  this->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "scale");
  this->LastRendererSize[0] = 0;
  this->LastRendererSize[1] = 0;
  this->LastCameraPosition[0] = 0.0;
  this->LastCameraPosition[1] = 0.0;
  this->LastCameraPosition[2] = 0.0;
  this->LastCameraFocalPoint[0] = 0.0;
  this->LastCameraFocalPoint[1] = 0.0;
  this->LastCameraFocalPoint[2] = 0.0;
  this->LastCameraViewUp[0] = 0.0;
  this->LastCameraViewUp[1] = 0.0;
  this->LastCameraViewUp[2] = 0.0;
  this->LastCameraParallelScale = 0.0;
  this->DistanceArrayName = nullptr;
  this->SetDistanceArrayName("DistanceToCamera");
}

svtkDistanceToCamera::~svtkDistanceToCamera()
{
  delete[] this->DistanceArrayName;
}

void svtkDistanceToCamera::SetRenderer(svtkRenderer* ren)
{
  if (ren != this->Renderer)
  {
    this->Renderer = ren;
    this->Modified();
  }
}

svtkMTimeType svtkDistanceToCamera::GetMTime()
{
  // Check for minimal changes
  if (this->Renderer)
  {
    const int* sz = this->Renderer->GetSize();
    if (this->LastRendererSize[0] != sz[0] || this->LastRendererSize[1] != sz[1])
    {
      this->LastRendererSize[0] = sz[0];
      this->LastRendererSize[1] = sz[1];
      this->Modified();
    }
    svtkCamera* cam = this->Renderer->GetActiveCamera();
    if (cam)
    {
      double* pos = cam->GetPosition();
      if (this->LastCameraPosition[0] != pos[0] || this->LastCameraPosition[1] != pos[1] ||
        this->LastCameraPosition[2] != pos[2])
      {
        this->LastCameraPosition[0] = pos[0];
        this->LastCameraPosition[1] = pos[1];
        this->LastCameraPosition[2] = pos[2];
        this->Modified();
      }
      double* fp = cam->GetFocalPoint();
      if (this->LastCameraFocalPoint[0] != fp[0] || this->LastCameraFocalPoint[1] != fp[1] ||
        this->LastCameraFocalPoint[2] != fp[2])
      {
        this->LastCameraFocalPoint[0] = fp[0];
        this->LastCameraFocalPoint[1] = fp[1];
        this->LastCameraFocalPoint[2] = fp[2];
        this->Modified();
      }
      double* up = cam->GetViewUp();
      if (this->LastCameraViewUp[0] != up[0] || this->LastCameraViewUp[1] != up[1] ||
        this->LastCameraViewUp[2] != up[2])
      {
        this->LastCameraViewUp[0] = up[0];
        this->LastCameraViewUp[1] = up[1];
        this->LastCameraViewUp[2] = up[2];
        this->Modified();
      }
      double scale = cam->GetParallelScale();
      if (this->LastCameraParallelScale != scale)
      {
        this->LastCameraParallelScale = scale;
        this->Modified();
      }
    }
  }
  return this->Superclass::GetMTime();
}

int svtkDistanceToCamera::RequestData(svtkInformation* svtkNotUsed(request),
  svtkInformationVector** inputVector, svtkInformationVector* outputVector)
{
  // get the info objects
  svtkInformation* inInfo = inputVector[0]->GetInformationObject(0);
  svtkInformation* outInfo = outputVector->GetInformationObject(0);

  // get the input and output
  svtkPointSet* input = svtkPointSet::SafeDownCast(inInfo->Get(svtkDataObject::DATA_OBJECT()));
  svtkPointSet* output = svtkPointSet::SafeDownCast(outInfo->Get(svtkDataObject::DATA_OBJECT()));

  if (input->GetNumberOfPoints() == 0)
  {
    return 1;
  }

  if (!this->Renderer)
  {
    svtkErrorMacro("Renderer must be non-nullptr");
    return 0;
  }

  if (!this->DistanceArrayName || strlen(this->DistanceArrayName) == 0)
  {
    svtkErrorMacro("The name of the distance array must be specified");
    return 0;
  }

  svtkCamera* camera = this->Renderer->GetActiveCamera();
  double* pos = camera->GetPosition();

  svtkDataArray* scaleArr = nullptr;
  if (this->Scaling)
  {
    scaleArr = this->GetInputArrayToProcess(0, inputVector);
    if (!scaleArr)
    {
      svtkErrorMacro("Scaling array not found.");
      return 0;
    }
    if (scaleArr->GetNumberOfComponents() > 1)
    {
      svtkErrorMacro("Scaling array has more than one component.");
      return 0;
    }
  }

  output->ShallowCopy(input);
  svtkIdType numPoints = input->GetNumberOfPoints();
  svtkSmartPointer<svtkDoubleArray> distArr = svtkSmartPointer<svtkDoubleArray>::New();
  distArr->SetName(this->DistanceArrayName);
  distArr->SetNumberOfTuples(numPoints);
  output->GetPointData()->AddArray(distArr);
  if (camera->GetParallelProjection())
  {
    double size = 1;
    if (this->Renderer->GetSize()[1] > 0)
    {
      size = 2.0 * (camera->GetParallelScale() / this->Renderer->GetSize()[1]) * this->ScreenSize;
    }
    if (scaleArr)
    {
      double tuple[1];
      for (svtkIdType i = 0; i < numPoints; ++i)
      {
        scaleArr->GetTuple(i, tuple);
        distArr->SetValue(i, size * tuple[0]);
      }
    }
    else
    {
      for (svtkIdType i = 0; i < numPoints; ++i)
      {
        distArr->SetValue(i, size);
      }
    }
  }
  else
  {
    double factor = 1;
    if (this->Renderer->GetSize()[1] > 0)
    {
      factor = 2.0 * this->ScreenSize *
        tan(svtkMath::RadiansFromDegrees(camera->GetViewAngle() / 2.0)) /
        this->Renderer->GetSize()[1];
    }
    if (scaleArr)
    {
      double tuple[1];
      for (svtkIdType i = 0; i < numPoints; ++i)
      {
        double dist = sqrt(svtkMath::Distance2BetweenPoints(input->GetPoint(i), pos));
        double size = factor * dist;
        scaleArr->GetTuple(i, tuple);
        distArr->SetValue(i, size * tuple[0]);
      }
    }
    else
    {
      for (svtkIdType i = 0; i < numPoints; ++i)
      {
        double dist = sqrt(svtkMath::Distance2BetweenPoints(input->GetPoint(i), pos));
        double size = factor * dist;
        distArr->SetValue(i, size);
      }
    }
  }

  return 1;
}

void svtkDistanceToCamera::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Renderer: ";
  if (this->Renderer)
  {
    os << "\n";
    this->Renderer->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)";
  }
  os << indent << "ScreenSize: " << this->ScreenSize << endl;
  os << indent << "Scaling: " << (this->Scaling ? "on" : "off") << endl;
}
