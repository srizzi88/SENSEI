/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestRendererType.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can switch between a variety of raytraced
// rendering modes.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              svtkOSPRayTestInteractor.h

#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPLYReader.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

#include "svtkOSPRayTestInteractor.h"

int TestRendererType(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(renderer);

  const char* fileName = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/bunny.ply");
  svtkSmartPointer<svtkPLYReader> polysource = svtkSmartPointer<svtkPLYReader>::New();
  polysource->SetFileName(fileName);

  // TODO: ospray acts strangely without these such that Diff and Spec are not 0..255 instead of
  // 0..1
  svtkSmartPointer<svtkPolyDataNormals> normals = svtkSmartPointer<svtkPolyDataNormals>::New();
  normals->SetInputConnection(polysource->GetOutputPort());
  // normals->ComputePointNormalsOn();
  // normals->ComputeCellNormalsOff();

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(normals->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  renderer->SetBackground(0.1, 0.1, 1.0);
  renWin->SetSize(400, 400);
  renWin->Render();

  svtkSmartPointer<svtkOSPRayPass> ospray = svtkSmartPointer<svtkOSPRayPass>::New();
  renderer->SetPass(ospray);

  for (int i = 1; i < 9; i++)
  {
    switch (i % 3)
    {
      case 0:
        cerr << "Render via scivis" << endl;
        svtkOSPRayRendererNode::SetRendererType("scivis", renderer);
        break;
      case 1:
        cerr << "Render via ospray pathtracer" << endl;
        svtkOSPRayRendererNode::SetRendererType("pathtracer", renderer);
        break;
      case 2:
        cerr << "Render via optix pathracer" << endl;
        svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
        break;
    }
    for (int j = 0; j < 10; j++)
    {
      renWin->Render();
    }
  }

  svtkSmartPointer<svtkOSPRayTestInteractor> style = svtkSmartPointer<svtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();

  return 0;
}
