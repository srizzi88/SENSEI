/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestResetCameraVerticalAspectRatioParallel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// Make sure that on a window with vertical aspect ratio, the camera is
// reset properly with parallel projection.
//
#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCylinderSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"

// int main( int argc, char* argv[] )
int TestResetCameraVerticalAspectRatioParallel(int argc, char* argv[])
{
  svtkCylinderSource* c = svtkCylinderSource::New();
  c->SetHeight(4.0);

  svtkPolyDataMapper* m = svtkPolyDataMapper::New();
  m->SetInputConnection(c->GetOutputPort());

  svtkActor* a = svtkActor::New();
  a->SetMapper(m);
  a->RotateZ(-90.0);

  svtkRenderer* ren1 = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  ren1->AddActor(a);
  ren1->SetBackground(0.1, 0.2, 0.4);

  // Width cannot be smaller than 104 and 108 respectively on Windows XP and
  // Vista because of decorations. And apparently not smaller than 116 on
  // Vista with standard style and 24" wide screen.
  renWin->SetSize(128, 400);

  ren1->GetActiveCamera()->ParallelProjectionOn();
  // ren1->GetActiveCamera()->SetUseHorizontalViewAngle(1);
  ren1->ResetCamera();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  c->Delete();
  m->Delete();
  a->Delete();
  ren1->Delete();
  renWin->Delete();
  iren->Delete();

  return !retVal;
}
