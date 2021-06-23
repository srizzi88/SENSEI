/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestDelaunay2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkActor.h"
#include "svtkNew.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkUnsignedCharArray.h"

// Regression testing for following crash:
// - polydata with point and cell normals is rendered as phong
// - surface interpolation is then switched to flat
// - next rendering call would provoke a nullptr access because
//   polydata mapper was previously not handling this change correctly
int TestSurfaceInterpolationSwitch(int argc, char* argv[])
{
  auto sphereSource = svtkSmartPointer<svtkSphereSource>::New();
  auto normalsFilter = svtkSmartPointer<svtkPolyDataNormals>::New();
  normalsFilter->SetInputConnection(sphereSource->GetOutputPort());
  normalsFilter->SetComputePointNormals(true);
  normalsFilter->SetComputeCellNormals(true);
  normalsFilter->Update();

  auto polydata = normalsFilter->GetOutput();

  svtkNew<svtkPolyDataMapper> mapper;
  mapper->SetInputData(polydata);

  svtkNew<svtkActor> actor;
  actor->SetMapper(mapper);

  svtkProperty* property = actor->GetProperty();
  property->SetRepresentationToSurface();
  property->SetInterpolationToPhong();

  // Render image
  svtkNew<svtkRenderer> renderer;
  renderer->AddActor(actor);
  renderer->SetBackground(0.0, 0.0, 0.0);

  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(600, 300);
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  if (!renWin->SupportsOpenGL())
  {
    cerr << "The platform does not support OpenGL as required\n";
    cerr << svtkOpenGLRenderWindow::SafeDownCast(renWin)->GetOpenGLSupportMessage();
    cerr << renWin->ReportCapabilities();
    return 1;
  }

  renWin->Render(); // this render call was always ok

  property->SetInterpolationToFlat();
  mapper->Update();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
