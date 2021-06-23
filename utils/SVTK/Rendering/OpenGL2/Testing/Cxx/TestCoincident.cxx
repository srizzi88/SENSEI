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
#include "svtkPLYReader.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkTimerLog.h"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkRenderWindowInteractor.h"

//----------------------------------------------------------------------------
int TestCoincident(int argc, char* argv[])
{
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renderWindow;
  renderWindow->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renderWindow);
  renderWindow->SetMultiSamples(0);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/dragon.ply");
  svtkNew<svtkPLYReader> reader;
  reader->SetFileName(fileName);
  reader->Update();

  delete[] fileName;

  svtkMapper::SetResolveCoincidentTopologyToPolygonOffset();

  // render points then lines then surface
  // the opposite order of what we want in terms
  // of visibility
  {
    svtkNew<svtkPolyDataMapper> mapper;
    mapper->SetInputConnection(reader->GetOutputPort());
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetDiffuseColor(1.0, 0.3, 1.0);
    actor->GetProperty()->SetPointSize(4.0);
    actor->GetProperty()->SetRepresentationToPoints();
    renderer->AddActor(actor);
  }

  {
    svtkNew<svtkPolyDataMapper> mapper;
    mapper->SetInputConnection(reader->GetOutputPort());
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetDiffuseColor(0.3, 0.3, 1.0);
    actor->GetProperty()->SetRepresentationToWireframe();
    renderer->AddActor(actor);
  }

  {
    svtkNew<svtkPolyDataMapper> mapper;
    mapper->SetInputConnection(reader->GetOutputPort());
    svtkNew<svtkActor> actor;
    actor->SetMapper(mapper);
    actor->GetProperty()->SetDiffuseColor(1.0, 1.0, 0.3);
    renderer->AddActor(actor);
  }

  renderWindow->Render();
  renderer->GetActiveCamera()->Zoom(30.0);
  renderer->ResetCameraClippingRange();
  renderWindow->Render();

  int retVal = svtkRegressionTestImage(renderWindow);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
