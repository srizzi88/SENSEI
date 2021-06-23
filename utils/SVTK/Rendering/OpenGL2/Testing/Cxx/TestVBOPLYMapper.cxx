/*=========================================================================

  Program:   Visualization Toolkit

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
#include "svtkLightKit.h"
#include "svtkNew.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkPLYReader.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRenderer.h"
#include "svtkTimerLog.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkRenderWindowInteractor.h"

#include "svtkOpenGLRenderWindow.h"

//----------------------------------------------------------------------------
int TestVBOPLYMapper(int argc, char* argv[])
{
  bool timeit = false;
  if (argc > 1 && argv[1] && !strcmp(argv[1], "-timeit"))
  {
    timeit = true;
  }

  svtkNew<svtkActor> actor;
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkPolyDataMapper> mapper;
  renderer->SetBackground(0.0, 0.0, 0.0);
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->SetSize(timeit ? 800 : 300, timeit ? 800 : 300);
  renderWindow->AddRenderer(renderer);
  renderer->AddActor(actor);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  svtkNew<svtkLightKit> lightKit;
  lightKit->AddLightsToRenderer(renderer);

  if (!renderWindow->SupportsOpenGL())
  {
    cerr << "The platform does not support OpenGL as required\n";
    cerr << svtkOpenGLRenderWindow::SafeDownCast(renderWindow)->GetOpenGLSupportMessage();
    cerr << renderWindow->ReportCapabilities();
    return 1;
  }

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  // svtkNew<svtkPolyDataNormals> norms;
  // norms->SetInputConnection(reader->GetOutputPort());
  // norms->Update();

  mapper->SetInputConnection(reader->GetOutputPort());
  // mapper->SetInputConnection(norms->GetOutputPort());
  actor->SetMapper(mapper);
  actor->GetProperty()->SetAmbientColor(0.2, 0.2, 1.0);
  actor->GetProperty()->SetDiffuseColor(1.0, 0.65, 0.7);
  actor->GetProperty()->SetSpecularColor(1.0, 1.0, 1.0);
  actor->GetProperty()->SetSpecular(0.5);
  actor->GetProperty()->SetDiffuse(0.7);
  actor->GetProperty()->SetAmbient(0.5);
  actor->GetProperty()->SetSpecularPower(20.0);
  actor->GetProperty()->SetOpacity(1.0);
  // actor->GetProperty()->SetRepresentationToWireframe();

  renderWindow->SetMultiSamples(0);

  svtkNew<svtkTimerLog> timer;
  timer->StartTimer();
  renderWindow->Render();
  timer->StopTimer();
  double firstRender = timer->GetElapsedTime();
  cerr << "first render time: " << firstRender << endl;
  int major, minor;
  svtkOpenGLRenderWindow::SafeDownCast(renderWindow)->GetOpenGLVersion(major, minor);
  cerr << "opengl version " << major << "." << minor << "\n";

  timer->StartTimer();
  int numRenders = timeit ? 600 : 8;
  for (int i = 0; i < numRenders; ++i)
  {
    renderer->GetActiveCamera()->Azimuth(80.0 / numRenders);
    renderer->GetActiveCamera()->Elevation(80.0 / numRenders);
    renderWindow->Render();
  }
  timer->StopTimer();
  double elapsed = timer->GetElapsedTime();
  cerr << "interactive render time: " << elapsed / numRenders << endl;
  unsigned int numTris = reader->GetOutput()->GetPolys()->GetNumberOfCells();
  cerr << "number of triangles: " << numTris << endl;
  cerr << "triangles per second: " << numTris * (numRenders / elapsed) << endl;

  renderer->GetActiveCamera()->SetPosition(0, 0, 1);
  renderer->GetActiveCamera()->SetFocalPoint(0, 0, 0);
  renderer->GetActiveCamera()->SetViewUp(0, 1, 0);
  renderer->ResetCamera();
  renderWindow->Render();
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
