/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitPolyDataDistance.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.
=========================================================================*/
#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkGlyph3D.h"
#include "svtkImplicitPolyDataDistance.h"
#include "svtkNew.h"
#include "svtkPlaneSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkXMLPolyDataReader.h"

#include <vector>

int TestImplicitPolyDataDistance(int argc, char* argv[])
{
  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/CuspySurface.vtp");
  std::cout << fileName << std::endl;

  // Set up reader
  svtkNew<svtkXMLPolyDataReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;
  reader->Update();

  // Set up distance calculator
  svtkNew<svtkImplicitPolyDataDistance> implicitDistance;
  implicitDistance->SetInput(reader->GetOutput());

  // Test SetNoClosestPoint() and GetNoClosestPoint()
  double noClosestPoint[3] = { 1.0, 1.0, 1.0 };
  implicitDistance->SetNoClosestPoint(noClosestPoint);
  implicitDistance->GetNoClosestPoint(noClosestPoint);
  if (noClosestPoint[0] != 1.0 && noClosestPoint[1] != 1.0 && noClosestPoint[2] != 1.0)
  {
    return EXIT_FAILURE;
  }

  // Compute distances to test points, saving those within the cuspy surface for display
  svtkNew<svtkPoints> insidePoints;
  svtkNew<svtkPoints> surfacePoints;
  double xRange[2] = { -47.6, 46.9 };
  double yRange[2] = { -18.2, 82.1 };
  double zRange[2] = { 1.63, 102 };
  const double spacing = 10.0;
  for (double z = zRange[0]; z < zRange[1]; z += spacing)
  {
    for (double y = yRange[0]; y < yRange[1]; y += spacing)
    {
      for (double x = xRange[0]; x < xRange[1]; x += spacing)
      {
        double point[3] = { x, y, z };
        double surfacePoint[3];
        double distance = implicitDistance->EvaluateFunctionAndGetClosestPoint(point, surfacePoint);
        if (distance <= 0.0)
        {
          insidePoints->InsertNextPoint(point);
          surfacePoints->InsertNextPoint(surfacePoint);
        }
      }
    }
  }

  // Set up inside points data structure
  svtkNew<svtkPolyData> insidePointsPolyData;
  insidePointsPolyData->SetPoints(insidePoints);

  // Glyph the points
  svtkNew<svtkSphereSource> insidePointSphere;
  insidePointSphere->SetRadius(3);
  svtkNew<svtkGlyph3D> insidePointsGlypher;
  insidePointsGlypher->SetInputData(insidePointsPolyData);
  insidePointsGlypher->SetSourceConnection(insidePointSphere->GetOutputPort());

  // Display the glyphs
  svtkNew<svtkPolyDataMapper> insidePointMapper;
  insidePointMapper->SetInputConnection(insidePointsGlypher->GetOutputPort());

  svtkNew<svtkActor> insidePointActor;
  insidePointActor->SetMapper(insidePointMapper);
  insidePointActor->GetProperty()->SetColor(1.0, 0.0, 0.0);

  // Set up surface points data structure
  svtkNew<svtkPolyData> surfacePointsPolyData;
  surfacePointsPolyData->SetPoints(surfacePoints);

  // Glyph the points
  svtkNew<svtkSphereSource> surfacePointSphere;
  surfacePointSphere->SetRadius(3);
  svtkNew<svtkGlyph3D> surfacePointsGlypher;
  surfacePointsGlypher->SetInputData(surfacePointsPolyData);
  surfacePointsGlypher->SetSourceConnection(surfacePointSphere->GetOutputPort());

  // Display the glyphs
  svtkNew<svtkPolyDataMapper> surfacePointMapper;
  surfacePointMapper->SetInputConnection(surfacePointsGlypher->GetOutputPort());

  svtkNew<svtkActor> surfacePointActor;
  surfacePointActor->SetMapper(surfacePointMapper);
  surfacePointActor->GetProperty()->SetColor(0.0, 0.0, 1.0);

  // Display the bounding surface
  svtkNew<svtkPolyDataMapper> surfaceMapper;
  surfaceMapper->SetInputConnection(reader->GetOutputPort());

  svtkNew<svtkActor> surfaceActor;
  surfaceActor->SetMapper(surfaceMapper);
  surfaceActor->GetProperty()->FrontfaceCullingOn();

  // Standard rendering classes
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  renderer->AddActor(insidePointActor);
  renderer->AddActor(surfacePointActor);
  renderer->AddActor(surfaceActor);

  // Standard testing code.
  renderer->SetBackground(0.0, 0.0, 0.0);
  renWin->SetSize(300, 300);

  svtkCamera* camera = renderer->GetActiveCamera();
  renderer->ResetCamera();
  camera->Azimuth(30);
  camera->Elevation(-20);

  iren->Initialize();

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
