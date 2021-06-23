/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestStencilWithPolyDataSurface.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkAppendPolyData.h"
#include "svtkBoxMuellerRandomSequence.h"
#include "svtkCamera.h"
#include "svtkCutter.h"
#include "svtkImageData.h"
#include "svtkImageProperty.h"
#include "svtkImageSlice.h"
#include "svtkImageSliceMapper.h"
#include "svtkImageStencil.h"
#include "svtkInteractorStyleImage.h"
#include "svtkPlane.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataToImageStencil.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkStripper.h"
#include "svtkTransform.h"
#include "svtkTransformPolyDataFilter.h"
#include "svtkTriangleFilter.h"

#include <cmath>
#include <cstring>

int TestStencilWithPolyDataSurface(int, char*[])
{
  svtkSmartPointer<svtkImageData> image = svtkSmartPointer<svtkImageData>::New();
  double spacing[3] = { 0.9765625, 0.9765625, 3.0 };
  double origin[3] = { -124.51171875, -124.51171875, -105.0 };
  int extent[6] = { 0, 255, 0, 255, 0, 70 };
  image->SetSpacing(spacing);
  image->SetOrigin(origin);
  image->SetExtent(extent);
  image->AllocateScalars(SVTK_UNSIGNED_CHAR, 1);
  unsigned char* vptr = static_cast<unsigned char*>(image->GetScalarPointer());
  memset(vptr, 255, image->GetNumberOfPoints());

  svtkSmartPointer<svtkSphereSource> sphereSource = svtkSmartPointer<svtkSphereSource>::New();
  sphereSource->SetRadius(100);
  sphereSource->SetPhiResolution(21);
  sphereSource->SetThetaResolution(41);
  sphereSource->Update();

  svtkSmartPointer<svtkTriangleFilter> triangleFilter = svtkSmartPointer<svtkTriangleFilter>::New();
  triangleFilter->SetInputConnection(sphereSource->GetOutputPort());
  triangleFilter->Update();

  // add some noise to the point positions
  svtkSmartPointer<svtkBoxMuellerRandomSequence> randomSequence =
    svtkSmartPointer<svtkBoxMuellerRandomSequence>::New();
  svtkSmartPointer<svtkPolyData> polyData = svtkSmartPointer<svtkPolyData>::New();
  polyData->DeepCopy(triangleFilter->GetOutput());
  svtkPoints* points = polyData->GetPoints();
  svtkSmartPointer<svtkPoints> newPoints = svtkSmartPointer<svtkPoints>::New();
  newPoints->SetNumberOfPoints(points->GetNumberOfPoints());
  for (svtkIdType i = 0; i < points->GetNumberOfPoints(); i++)
  {
    double point[3];
    points->GetPoint(i, point);
    double r = exp(randomSequence->GetScaledValue(0.0, 0.1));
    randomSequence->Next();
    point[0] *= r;
    point[1] *= r;
    point[2] *= r;
    newPoints->SetPoint(i, point);
  }
  polyData->SetPoints(newPoints);

  // make sure triangle strips can be used as input
  svtkSmartPointer<svtkStripper> stripper = svtkSmartPointer<svtkStripper>::New();
  stripper->SetInputConnection(triangleFilter->GetOutputPort());

  svtkSmartPointer<svtkTransform> transform = svtkSmartPointer<svtkTransform>::New();
  transform->Scale(0.49, 0.5, 0.6);
  transform->Translate(9.111, -7.56, 1.0);
  transform->RotateWXYZ(30, 1.0, 0.5, 0.0);

  svtkSmartPointer<svtkTransformPolyDataFilter> transformFilter =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  transformFilter->SetTransform(transform);
  transformFilter->SetInputConnection(stripper->GetOutputPort());

  // use append to make sure nested surfaces are handled
  svtkSmartPointer<svtkAppendPolyData> append = svtkSmartPointer<svtkAppendPolyData>::New();
  append->SetInputData(polyData);
  append->AddInputConnection(transformFilter->GetOutputPort());

  svtkSmartPointer<svtkPolyDataToImageStencil> stencilSource =
    svtkSmartPointer<svtkPolyDataToImageStencil>::New();
  stencilSource->SetOutputOrigin(origin);
  stencilSource->SetOutputSpacing(spacing);
  stencilSource->SetOutputWholeExtent(extent);
  stencilSource->SetInputConnection(append->GetOutputPort());

  svtkSmartPointer<svtkImageStencil> stencil = svtkSmartPointer<svtkImageStencil>::New();
  stencil->SetInputData(image);
  stencil->SetStencilConnection(stencilSource->GetOutputPort());
  stencil->Update();

  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetSize(256 * 3, 256 * 2);

  svtkSmartPointer<svtkInteractorStyleImage> style = svtkSmartPointer<svtkInteractorStyleImage>::New();

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  for (int i = 0; i < 6; i++)
  {
    int zIdx = 3 + 11 * i;
    double z = zIdx * spacing[2] + origin[2];

    svtkSmartPointer<svtkPlane> plane = svtkSmartPointer<svtkPlane>::New();
    plane->SetNormal(0.0, 0.0, 1.0);
    plane->SetOrigin(0.0, 0.0, z);

    svtkSmartPointer<svtkCutter> cutter = svtkSmartPointer<svtkCutter>::New();
    cutter->SetInputConnection(append->GetOutputPort());
    cutter->SetCutFunction(plane);
    cutter->GenerateCutScalarsOff();

    svtkSmartPointer<svtkPolyDataMapper> polyMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
    polyMapper->SetInputConnection(cutter->GetOutputPort());
    polyMapper->ScalarVisibilityOff();

    svtkSmartPointer<svtkActor> polyActor = svtkSmartPointer<svtkActor>::New();
    polyActor->SetMapper(polyMapper);
    polyActor->GetProperty()->SetDiffuse(0.0);
    polyActor->GetProperty()->SetAmbient(1.0);
    polyActor->GetProperty()->SetColor(0.1, 0.6, 0.1);
    polyActor->SetPosition(0.0, 0.0, 1.0); // zbuffer offset

    svtkSmartPointer<svtkImageSliceMapper> mapper = svtkSmartPointer<svtkImageSliceMapper>::New();
    mapper->SetOrientation(2);
    mapper->SetSliceNumber(zIdx);
    mapper->SetInputConnection(stencil->GetOutputPort());

    svtkSmartPointer<svtkImageSlice> actor = svtkSmartPointer<svtkImageSlice>::New();
    actor->GetProperty()->SetColorWindow(255.0);
    actor->GetProperty()->SetColorLevel(127.5);
    actor->GetProperty()->SetInterpolationTypeToLinear();
    actor->SetMapper(mapper);

    svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
    int j = i % 3;
    int k = 1 - (i / 3);
    renderer->SetViewport(j / 3.0, k / 2.0, (j + 1) / 3.0, (k + 1) / 2.0);
    renderer->AddViewProp(actor);
    renderer->AddViewProp(polyActor);

    svtkCamera* camera = renderer->GetActiveCamera();
    camera->ParallelProjectionOn();
    camera->SetParallelScale(0.5 * spacing[1] * (extent[3] - extent[2]));
    camera->SetFocalPoint(0.0, 0.0, z);
    camera->SetPosition(0.0, 0.0, z + 10.0);
    camera->SetViewUp(0.0, 1.0, 0.0);
    camera->SetClippingRange(5.0, 15.0);

    renWin->AddRenderer(renderer);
  }

  iren->Initialize();
  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
