/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSelectEnclosedPoints.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates the use of svtkGenerateSurfaceIntersectionLines
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

// If WRITE_RESULT is defined, the result of the surface filter is saved.
//#define WRITE_RESULT

#include "svtkActor.h"
#include "svtkDataArray.h"
#include "svtkGlyph3D.h"
#include "svtkMath.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRandomPool.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSelectEnclosedPoints.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkThresholdPoints.h"
#include "svtkTimerLog.h"

int TestSelectEnclosedPoints(int argc, char* argv[])
{
  // Standard rendering classes
  svtkRenderer* renderer = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(renderer);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Create a containing surface
  svtkSphereSource* ss = svtkSphereSource::New();
  ss->SetPhiResolution(25);
  ss->SetThetaResolution(38);
  ss->SetCenter(4.5, 5.5, 5.0);
  ss->SetRadius(2.5);
  svtkPolyDataMapper* mapper = svtkPolyDataMapper::New();
  mapper->SetInputConnection(ss->GetOutputPort());
  svtkActor* actor = svtkActor::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToWireframe();

  // Generate some random points
  svtkPoints* points = svtkPoints::New();
  points->SetNumberOfPoints(500);

  svtkDataArray* da = points->GetData();
  svtkRandomPool* pool = svtkRandomPool::New();
  pool->PopulateDataArray(da, 0, 2.25, 7);
  pool->PopulateDataArray(da, 1, 1, 10);
  pool->PopulateDataArray(da, 2, 0.5, 10.5);

  svtkPolyData* profile = svtkPolyData::New();
  profile->SetPoints(points);

  svtkSelectEnclosedPoints* select = svtkSelectEnclosedPoints::New();
  select->SetInputData(profile);
  select->SetSurfaceConnection(ss->GetOutputPort());
  //  select->InsideOutOn();

  // Time execution
  svtkTimerLog* timer = svtkTimerLog::New();
  timer->StartTimer();
  select->Update();
  timer->StopTimer();
  double time = timer->GetElapsedTime();
  cout << "Time to extract points: " << time << "\n";

  // Now extract points
  svtkThresholdPoints* thresh = svtkThresholdPoints::New();
  thresh->SetInputConnection(select->GetOutputPort());
  thresh->SetInputArrayToProcess(
    0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "SelectedPoints");
  thresh->ThresholdByUpper(0.9);

  svtkSphereSource* glyph = svtkSphereSource::New();
  svtkGlyph3D* glypher = svtkGlyph3D::New();
  glypher->SetInputConnection(thresh->GetOutputPort());
  glypher->SetSourceConnection(glyph->GetOutputPort());
  glypher->SetScaleModeToDataScalingOff();
  glypher->SetScaleFactor(0.25);

  svtkPolyDataMapper* pointsMapper = svtkPolyDataMapper::New();
  pointsMapper->SetInputConnection(glypher->GetOutputPort());
  pointsMapper->ScalarVisibilityOff();

  svtkActor* pointsActor = svtkActor::New();
  pointsActor->SetMapper(pointsMapper);
  pointsActor->GetProperty()->SetColor(0, 0, 1);

  // Add actors
  //  renderer->AddActor(actor);
  renderer->AddActor(pointsActor);

  // Standard testing code.
  renWin->SetSize(300, 300);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Cleanup
  renderer->Delete();
  renWin->Delete();
  iren->Delete();

  ss->Delete();
  mapper->Delete();
  actor->Delete();

  pool->Delete();
  timer->Delete();

  points->Delete();
  profile->Delete();
  thresh->Delete();
  select->Delete();
  glyph->Delete();
  glypher->Delete();
  pointsMapper->Delete();
  pointsActor->Delete();

  return !retVal;
}
