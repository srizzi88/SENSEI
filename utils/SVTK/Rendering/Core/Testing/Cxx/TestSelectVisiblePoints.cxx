/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkGlyph3DMapper.h"
#include "svtkInteractorStyleSwitch.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSelectVisiblePoints.h"
#include "svtkSphereSource.h"

int GetNumberOfVisiblePoints(svtkSelectVisiblePoints* selectVisiblePoints, svtkPoints* points)
{
  // Points on the back side of the sphere should not be visible
  int numberOfVisiblePoints = 0;
  for (svtkIdType pointIndex = 0; pointIndex < points->GetNumberOfPoints(); pointIndex++)
  {
    bool occluded = selectVisiblePoints->IsPointOccluded(points->GetPoint(pointIndex), nullptr);
    if (occluded)
    {
      numberOfVisiblePoints++;
    }
  }
  return numberOfVisiblePoints;
}

int TestSelectVisiblePoints(int argc, char* argv[])
{
  // Create a point set that we will test visibility of,
  // using a sphere source.
  svtkNew<svtkSphereSource> sphere;
  sphere->SetRadius(20);
  sphere->Update();
  svtkPoints* spherePoints = sphere->GetOutput()->GetPoints();
  std::cout << "Total number of points: " << spherePoints->GetNumberOfPoints() << std::endl;

  // Set up renderer
  svtkNew<svtkRenderer> ren;
  svtkNew<svtkRenderWindow> win;
  win->AddRenderer(ren);
  svtkNew<svtkRenderWindowInteractor> iren;
  svtkInteractorStyleSwitch::SafeDownCast(iren->GetInteractorStyle())
    ->SetCurrentStyleToTrackballCamera();
  iren->SetRenderWindow(win);
  ren->SetBackground(0.5, 0.5, 0.5);
  win->SetSize(450, 450);

  // Create a sphere actor (to test that labels are only visible on one side).
  svtkNew<svtkActor> sphereActor;
  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  sphereActor->SetMapper(sphereMapper);
  ren->AddActor(sphereActor);

  // Initialize camera
  win->Render();
  ren->GetActiveCamera()->Azimuth(30);
  ren->ResetCamera();
  ren->GetActiveCamera()->Zoom(1.5);
  ren->ResetCameraClippingRange();
  win->Render();

  // Initialize visible point selector
  svtkNew<svtkSelectVisiblePoints> selectVisiblePoints;
  selectVisiblePoints->SetRenderer(ren);
  selectVisiblePoints->Initialize(false);

  int result = EXIT_SUCCESS;

  // Points on the back side of the sphere should not be visible
  int numberOfVisiblePoints = GetNumberOfVisiblePoints(selectVisiblePoints, spherePoints);
  std::cout << "Visible points when sphere is shown: " << numberOfVisiblePoints << std::endl;
  if (numberOfVisiblePoints == 0 || numberOfVisiblePoints == spherePoints->GetNumberOfPoints())
  {
    std::cerr << "Number of points is incorrect" << std::endl;
    result = EXIT_FAILURE;
  }

  // If we hide the sphere then all points should be visible
  if (result == EXIT_SUCCESS)
  {
    sphereActor->SetVisibility(false);
    win->Render();
    numberOfVisiblePoints = GetNumberOfVisiblePoints(selectVisiblePoints, spherePoints);
    std::cout << "Visible points when sphere is not shown: " << numberOfVisiblePoints << std::endl;
    if (numberOfVisiblePoints != spherePoints->GetNumberOfPoints())
    {
      std::cerr << "Number of points is incorrect" << std::endl;
      result = EXIT_FAILURE;
    }
  }

  // We now use glyph filter to visualize the points.
  // At least some points are occluded by the glyphs
  // (some may be considered as visible, due to svtkSelectVisiblePoints Tolerance).
  svtkNew<svtkGlyph3DMapper> glypher;
  if (result == EXIT_SUCCESS)
  {
    // Add glyph at each point
    glypher->SetInputConnection(sphere->GetOutputPort());
    svtkNew<svtkSphereSource> glyphSource;
    glypher->SetSourceConnection(glyphSource->GetOutputPort());
    glypher->SetScaleFactor(3.0);
    svtkNew<svtkActor> glyphActor;
    glyphActor->SetMapper(glypher);
    ren->AddActor(glyphActor);
    win->Render();
    numberOfVisiblePoints = GetNumberOfVisiblePoints(selectVisiblePoints, spherePoints);
    std::cout << "Visible points when glyph is shown at each point: " << numberOfVisiblePoints
              << std::endl;
    if (numberOfVisiblePoints == spherePoints->GetNumberOfPoints())
    {
      std::cerr << "Number of points is incorrect" << std::endl;
      result = EXIT_FAILURE;
    }
  }

  // All points should be visible if we set tolerance to be the glyph size
  // (except those 4 points that are covered by another point's glyph).
  const int numberOfOccludedPoints = 4;
  if (result == EXIT_SUCCESS)
  {
    selectVisiblePoints->SetToleranceWorld(glypher->GetScaleFactor() * 0.5);
    win->Render();
    numberOfVisiblePoints = GetNumberOfVisiblePoints(selectVisiblePoints, spherePoints);
    std::cout << "Visible points when sphere is shown, with world tolerance set: "
              << numberOfVisiblePoints << std::endl;
    if (numberOfVisiblePoints != spherePoints->GetNumberOfPoints() - numberOfOccludedPoints)
    {
      std::cerr << "Number of points is incorrect" << std::endl;
      result = EXIT_FAILURE;
    }
  }

  // All points should be visible if we rotate the view to avoid occluding
  // a point with another point's glyph.
  if (result == EXIT_SUCCESS)
  {
    ren->GetActiveCamera()->Pitch(40);
    ren->ResetCamera();
    ren->GetActiveCamera()->Zoom(1.5);
    ren->ResetCameraClippingRange();
    win->Render();
    numberOfVisiblePoints = GetNumberOfVisiblePoints(selectVisiblePoints, spherePoints);
    std::cout << "Visible points when sphere is shown, with world tolerance set, view aligned: "
              << numberOfVisiblePoints << std::endl;
    if (numberOfVisiblePoints != spherePoints->GetNumberOfPoints())
    {
      std::cerr << "Number of points is incorrect" << std::endl;
      result = EXIT_FAILURE;
    }
  }

  // Show the sphere again. Points on the back side of the sphere should not be visible.
  if (result == EXIT_SUCCESS)
  {
    sphereActor->SetVisibility(true);
    win->Render();
    numberOfVisiblePoints = GetNumberOfVisiblePoints(selectVisiblePoints, spherePoints);
    std::cout << "Visible points when sphere and glyphs are shown: " << numberOfVisiblePoints
              << std::endl;
    if (numberOfVisiblePoints == 0 || numberOfVisiblePoints == spherePoints->GetNumberOfPoints())
    {
      std::cerr << "Number of points is incorrect" << std::endl;
      result = EXIT_FAILURE;
    }
  }

  int retVal = svtkRegressionTestImage(win);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return result;
}
