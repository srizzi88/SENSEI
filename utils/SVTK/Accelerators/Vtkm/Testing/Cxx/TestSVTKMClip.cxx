/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSVTKMClip.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkmClip.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkDelaunay3D.h"
#include "svtkDoubleArray.h"
#include "svtkImageData.h"
#include "svtkImageToPoints.h"
#include "svtkNew.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkUnstructuredGrid.h"

namespace
{

template <typename DataSetT>
void GenerateScalars(DataSetT* dataset, bool negate)
{
  svtkIdType numPoints = dataset->GetNumberOfPoints();

  svtkNew<svtkDoubleArray> scalars;
  scalars->SetName("x+y");
  scalars->SetNumberOfComponents(1);
  scalars->SetNumberOfTuples(numPoints);

  double point[3];
  for (svtkIdType i = 0; i < numPoints; ++i)
  {
    dataset->GetPoint(i, point);
    scalars->SetTypedComponent(i, 0, (negate ? -point[0] - point[1] : point[0] + point[1]));
  }
  dataset->GetPointData()->SetScalars(scalars);
}

} // end anon namespace

int TestSVTKMClip(int, char*[])
{
  svtkNew<svtkRenderer> renderer;

  // First input is a polydata with 2D cells. This should produce a polydata
  // output from svtkmClip.
  svtkNew<svtkSphereSource> sphereSource;
  sphereSource->SetThetaResolution(50);
  sphereSource->SetPhiResolution(50);
  sphereSource->Update();
  svtkPolyData* sphere = sphereSource->GetOutput();
  GenerateScalars(sphere, false);

  // Clip at zero:
  svtkNew<svtkmClip> sphereClipper;
  sphereClipper->SetInputData(sphere);
  sphereClipper->SetComputeScalars(true);
  sphereClipper->SetClipValue(0.);

  svtkNew<svtkDataSetSurfaceFilter> sphSurface;
  sphSurface->SetInputConnection(sphereClipper->GetOutputPort());

  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphSurface->GetOutputPort());
  sphereMapper->SetScalarVisibility(1);
  sphereMapper->SetScalarModeToUsePointFieldData();
  sphereMapper->SelectColorArray("x+y");
  sphereMapper->SetScalarRange(0, 1);

  svtkNew<svtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);
  sphereActor->SetPosition(0.5, 0.5, 0.);
  sphereActor->RotateWXYZ(90., 0., 0., 1.);
  renderer->AddActor(sphereActor);

  // Second input is an unstructured grid with 3D cells. This should produce an
  // unstructured grid output from svtkmClip.
  svtkNew<svtkRTAnalyticSource> imageSource;
  imageSource->SetWholeExtent(-5, 5, -5, 5, -5, 5);

  // Convert image to pointset
  svtkNew<svtkImageToPoints> imageToPoints;
  imageToPoints->SetInputConnection(imageSource->GetOutputPort());

  // Convert point set to tets:
  svtkNew<svtkDelaunay3D> tetrahedralizer;
  tetrahedralizer->SetInputConnection(imageToPoints->GetOutputPort());
  tetrahedralizer->Update();
  svtkUnstructuredGrid* tets = tetrahedralizer->GetOutput();
  GenerateScalars(tets, true);

  // Clip at zero:
  svtkNew<svtkmClip> tetClipper;
  tetClipper->SetInputData(tets);
  tetClipper->SetComputeScalars(true);
  tetClipper->SetClipValue(0.);

  svtkNew<svtkDataSetSurfaceFilter> tetSurface;
  tetSurface->SetInputConnection(tetClipper->GetOutputPort());

  svtkNew<svtkPolyDataMapper> tetMapper;
  tetMapper->SetInputConnection(tetSurface->GetOutputPort());
  tetMapper->SetScalarVisibility(1);
  tetMapper->SetScalarModeToUsePointFieldData();
  tetMapper->SelectColorArray("x+y");
  tetMapper->SetScalarRange(0, 10);

  svtkNew<svtkActor> tetActor;
  tetActor->SetMapper(tetMapper);
  tetActor->SetScale(1. / 5.);
  renderer->AddActor(tetActor);

  // Third dataset tests imagedata. This should produce an unstructured grid:
  svtkImageData* image = imageSource->GetOutput();
  GenerateScalars(image, false);

  svtkNew<svtkmClip> imageClipper;
  imageClipper->SetInputData(image);
  imageClipper->SetComputeScalars(true);
  imageClipper->SetClipValue(0.);

  svtkNew<svtkDataSetSurfaceFilter> imageSurface;
  imageSurface->SetInputConnection(imageClipper->GetOutputPort());

  svtkNew<svtkPolyDataMapper> imageMapper;
  imageMapper->SetInputConnection(imageSurface->GetOutputPort());
  imageMapper->SetScalarVisibility(1);
  imageMapper->SetScalarModeToUsePointFieldData();
  imageMapper->SelectColorArray("x+y");
  imageMapper->SetScalarRange(0, 10);

  svtkNew<svtkActor> imageActor;
  imageActor->SetMapper(imageMapper);
  imageActor->SetScale(1. / 5.);
  imageActor->SetPosition(1.0, 1.0, 0.);
  renderer->AddActor(imageActor);

  svtkNew<svtkRenderWindowInteractor> iren;
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  iren->SetRenderWindow(renWin);
  renWin->AddRenderer(renderer);

  renWin->SetSize(500, 500);
  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();

  renWin->Render();
  iren->Start();

  return EXIT_SUCCESS;
}
