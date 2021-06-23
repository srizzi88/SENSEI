/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDeformPointSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkDeformPointSet.h"
#include "svtkElevationFilter.h"
#include "svtkNew.h"
#include "svtkPlane.h"
#include "svtkPlaneSource.h"
#include "svtkPoints.h"
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

int TestDeformPointSet(int argc, char* argv[])
{
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Create a sphere to warp
  svtkNew<svtkSphereSource> sphere;
  sphere->SetThetaResolution(51);
  sphere->SetPhiResolution(17);

  // Generate some scalars on the sphere
  svtkNew<svtkElevationFilter> ele;
  ele->SetInputConnection(sphere->GetOutputPort());
  ele->SetLowPoint(0, 0, -0.5);
  ele->SetHighPoint(0, 0, 0.5);

  // Now create a control mesh, in this case a octagon
  svtkNew<svtkPoints> pts;
  pts->SetNumberOfPoints(6);
  pts->SetPoint(0, -1, 0, 0);
  pts->SetPoint(1, 1, 0, 0);
  pts->SetPoint(2, 0, -1, 0);
  pts->SetPoint(3, 0, 1, 0);
  pts->SetPoint(4, 0, 0, -1);
  pts->SetPoint(5, 0, 0, 1);

  svtkNew<svtkCellArray> tris;
  tris->InsertNextCell(3);
  tris->InsertCellPoint(2);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(4);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(1);
  tris->InsertCellPoint(2);
  tris->InsertCellPoint(4);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(3);
  tris->InsertCellPoint(1);
  tris->InsertCellPoint(4);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(3);
  tris->InsertCellPoint(4);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(2);
  tris->InsertCellPoint(5);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(2);
  tris->InsertCellPoint(1);
  tris->InsertCellPoint(5);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(1);
  tris->InsertCellPoint(3);
  tris->InsertCellPoint(5);
  tris->InsertNextCell(3);
  tris->InsertCellPoint(3);
  tris->InsertCellPoint(0);
  tris->InsertCellPoint(5);

  svtkNew<svtkPolyData> pd;
  pd->SetPoints(pts);
  pd->SetPolys(tris);

  // Display the control mesh
  svtkNew<svtkPolyDataMapper> meshMapper;
  meshMapper->SetInputData(pd);
  svtkNew<svtkActor> meshActor;
  meshActor->SetMapper(meshMapper);
  meshActor->GetProperty()->SetRepresentationToWireframe();
  meshActor->GetProperty()->SetColor(0, 0, 0);

  // Okay now let's do the initial weight generation
  svtkNew<svtkDeformPointSet> deform;
  deform->SetInputConnection(ele->GetOutputPort());
  deform->SetControlMeshData(pd);
  deform->Update(); // this creates the initial weights

  // Now move one point and deform
  pts->SetPoint(5, 0, 0, 3);
  pts->Modified();
  deform->Update();

  // Display the warped sphere
  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(deform->GetOutputPort());
  svtkNew<svtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  renderer->AddActor(sphereActor);
  renderer->AddActor(meshActor);
  renderer->GetActiveCamera()->SetPosition(1, 1, 1);
  renderer->ResetCamera();

  renderer->SetBackground(1, 1, 1);
  renWin->SetSize(300, 300);
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
