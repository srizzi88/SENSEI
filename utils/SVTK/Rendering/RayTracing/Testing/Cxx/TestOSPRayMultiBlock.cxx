/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayMultiBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that treatment of multiblock data is correct
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              svtkOSPRayTestInteractor.h

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCompositeDataSet.h"
#include "svtkCompositePolyDataMapper2.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"
#include "svtkXMLMultiBlockDataReader.h"

#include "svtkOSPRayTestInteractor.h"

int TestOSPRayMultiBlock(int argc, char* argv[])
{
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(renderer);

  svtkSmartPointer<svtkXMLMultiBlockDataReader> reader =
    svtkSmartPointer<svtkXMLMultiBlockDataReader>::New();
  const char* fileName =
    svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/many_blocks/many_blocks.vtm");
  reader->SetFileName(fileName);
  reader->Update();

  svtkSmartPointer<svtkCompositePolyDataMapper2> mapper =
    svtkSmartPointer<svtkCompositePolyDataMapper2>::New();
  mapper->SetInputConnection(reader->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  renderer->AddActor(actor);
  actor->SetMapper(mapper);
  renderer->SetBackground(0.1, 0.1, 1.0);
  renderer->SetEnvironmentalBG(0.1, 0.1, 1.0);
  renWin->SetSize(400, 400);
  renWin->Render();

  svtkCamera* cam = renderer->GetActiveCamera();
  cam->SetPosition(1.5, 1.5, 0.75);

  svtkSmartPointer<svtkOSPRayPass> ospray = svtkSmartPointer<svtkOSPRayPass>::New();

  renderer->SetPass(ospray);

  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  renWin->Render();

  svtkSmartPointer<svtkOSPRayTestInteractor> style = svtkSmartPointer<svtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();

  return 0;
}
