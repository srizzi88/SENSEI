/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestImplicitProjectOnPlaneDistance

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
#include "svtkImplicitProjectOnPlaneDistance.h"
#include "svtkNew.h"
#include "svtkPlaneSource.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkType.h"
#include "svtkXMLPolyDataReader.h"

#include <vector>

int TestImplicitProjectOnPlaneDistance(int argc, char* argv[])
{
  char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/CuspySurface.vtp");
  std::cout << fileName << std::endl;

  // Set up reader
  svtkNew<svtkXMLPolyDataReader> reader;
  reader->SetFileName(fileName);
  delete[] fileName;
  reader->Update();
  svtkPolyData* pd = svtkPolyData::SafeDownCast(reader->GetOutputAsDataSet());

  svtkNew<svtkPlaneSource> plane;
  plane->SetOrigin(0, 0, -1);
  plane->SetPoint1(-30, -10, -1);
  plane->SetPoint2(30, 50, -1);
  plane->Update();

  // Set up distance calculator
  svtkNew<svtkImplicitProjectOnPlaneDistance> implicitDistance;
  implicitDistance->SetInput(plane->GetOutput());

  // Compute distances to test points, saving those below the cuspy surface for display
  svtkNew<svtkPoints> insidePoints;
  const svtkIdType pdNbPoints = pd->GetNumberOfPoints();
  for (svtkIdType i = 0; i < pdNbPoints; i++)
  {
    double point[3];
    pd->GetPoint(i, point);
    double distance = implicitDistance->EvaluateFunction(point);
    if (distance <= 0.0)
    {
      insidePoints->InsertNextPoint(point);
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

  // Display the glyphs
  svtkNew<svtkPolyDataMapper> planeMapper;
  planeMapper->SetInputConnection(plane->GetOutputPort());

  svtkNew<svtkActor> planeActor;
  planeActor->SetMapper(planeMapper);
  planeActor->GetProperty()->SetColor(0.0, 0.0, 1.0);

  // Display the bounding surface
  svtkNew<svtkPolyDataMapper> surfaceMapper;
  surfaceMapper->SetInputData(pd);

  svtkNew<svtkActor> surfaceActor;
  surfaceActor->SetMapper(surfaceMapper);
  surfaceActor->GetProperty()->FrontfaceCullingOn();

  // Standard rendering classes
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renderer->AddActor(insidePointActor);
  renderer->AddActor(planeActor);
  renderer->AddActor(surfaceActor);

  // Standard testing code.
  renderer->SetBackground(0.0, 0.0, 0.0);
  renWin->SetSize(300, 300);

  svtkCamera* camera = renderer->GetActiveCamera();
  renderer->ResetCamera();
  camera->Azimuth(60);
  camera->Elevation(-10);

  iren->Initialize();

  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
