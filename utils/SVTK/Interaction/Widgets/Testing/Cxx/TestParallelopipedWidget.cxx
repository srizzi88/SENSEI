/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestParallelopipedWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSmartPointer.h"

#include "svtkAppendPolyData.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkCubeAxesActor2D.h"
#include "svtkCubeSource.h"
#include "svtkGlyph3D.h"
#include "svtkMatrix4x4.h"
#include "svtkMatrixToLinearTransform.h"
#include "svtkParallelopipedRepresentation.h"
#include "svtkParallelopipedWidget.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTransformPolyDataFilter.h"

//----------------------------------------------------------------------------
int TestParallelopipedWidget(int svtkNotUsed(argc), char* svtkNotUsed(argv)[])
{
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(renderer);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  renderer->SetBackground(0.8, 0.8, 1.0);
  renWin->SetSize(800, 600);

  svtkSmartPointer<svtkConeSource> cone = svtkSmartPointer<svtkConeSource>::New();
  cone->SetResolution(6);
  svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);
  svtkSmartPointer<svtkGlyph3D> glyph = svtkSmartPointer<svtkGlyph3D>::New();
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  svtkSmartPointer<svtkAppendPolyData> append = svtkSmartPointer<svtkAppendPolyData>::New();
  append->AddInputConnection(glyph->GetOutputPort());
  append->AddInputConnection(sphere->GetOutputPort());
  append->Update();

  svtkSmartPointer<svtkCubeSource> cube = svtkSmartPointer<svtkCubeSource>::New();
  double bounds[6];
  append->GetOutput()->GetBounds(bounds);
  bounds[0] -= (bounds[1] - bounds[0]) * 0.25;
  bounds[1] += (bounds[1] - bounds[0]) * 0.25;
  bounds[2] -= (bounds[3] - bounds[2]) * 0.25;
  bounds[3] += (bounds[3] - bounds[2]) * 0.25;
  bounds[4] -= (bounds[5] - bounds[4]) * 0.25;
  bounds[5] += (bounds[5] - bounds[4]) * 0.25;
  bounds[0] = -1.0;
  bounds[1] = 1.0;
  bounds[2] = -1.0;
  bounds[3] = 1.0;
  bounds[4] = -1.0;
  bounds[5] = 1.0;
  cube->SetBounds(bounds);

  svtkSmartPointer<svtkMatrix4x4> affineMatrix = svtkSmartPointer<svtkMatrix4x4>::New();
  const double m[] = { 1.0, 0.1, 0.2, 0.0, 0.1, 1.0, 0.1, 0.0, 0.2, 0.1, 1.0, 0.0, 0.0, 0.0, 0.0,
    1.0 };
  affineMatrix->DeepCopy(m);
  svtkSmartPointer<svtkMatrixToLinearTransform> transform =
    svtkSmartPointer<svtkMatrixToLinearTransform>::New();
  transform->SetInput(affineMatrix);
  transform->Update();
  svtkSmartPointer<svtkTransformPolyDataFilter> transformFilter =
    svtkSmartPointer<svtkTransformPolyDataFilter>::New();
  transformFilter->SetTransform(transform);
  transformFilter->SetInputConnection(cube->GetOutputPort());
  transformFilter->Update();

  svtkSmartPointer<svtkPoints> parallelopipedPoints = svtkSmartPointer<svtkPoints>::New();
  parallelopipedPoints->DeepCopy(transformFilter->GetOutput()->GetPoints());

  transformFilter->SetInputConnection(append->GetOutputPort());
  transformFilter->Update();

  svtkSmartPointer<svtkPolyDataMapper> maceMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  maceMapper->SetInputConnection(transformFilter->GetOutputPort());

  svtkSmartPointer<svtkActor> maceActor = svtkSmartPointer<svtkActor>::New();
  maceActor->SetMapper(maceMapper);

  renderer->AddActor(maceActor);

  double parallelopipedPts[8][3];
  parallelopipedPoints->GetPoint(0, parallelopipedPts[0]);
  parallelopipedPoints->GetPoint(1, parallelopipedPts[1]);
  parallelopipedPoints->GetPoint(2, parallelopipedPts[3]);
  parallelopipedPoints->GetPoint(3, parallelopipedPts[2]);
  parallelopipedPoints->GetPoint(4, parallelopipedPts[4]);
  parallelopipedPoints->GetPoint(5, parallelopipedPts[5]);
  parallelopipedPoints->GetPoint(6, parallelopipedPts[7]);
  parallelopipedPoints->GetPoint(7, parallelopipedPts[6]);

  svtkSmartPointer<svtkParallelopipedWidget> widget = svtkSmartPointer<svtkParallelopipedWidget>::New();
  svtkSmartPointer<svtkParallelopipedRepresentation> rep =
    svtkSmartPointer<svtkParallelopipedRepresentation>::New();
  widget->SetRepresentation(rep);
  widget->SetInteractor(iren);
  rep->SetPlaceFactor(0.5);
  rep->PlaceWidget(parallelopipedPts);

  iren->Initialize();
  renWin->Render();

  widget->EnabledOn();

  svtkSmartPointer<svtkCubeAxesActor2D> axes = svtkSmartPointer<svtkCubeAxesActor2D>::New();
  axes->SetInputConnection(transformFilter->GetOutputPort());
  axes->SetCamera(renderer->GetActiveCamera());
  axes->SetLabelFormat("%6.1f");
  axes->SetFlyModeToOuterEdges();
  axes->SetFontFactor(0.8);
  renderer->AddViewProp(axes);

  iren->Start();

  return EXIT_SUCCESS;
}
