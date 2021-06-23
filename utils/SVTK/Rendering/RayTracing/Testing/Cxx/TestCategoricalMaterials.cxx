/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCategoricalMaterials.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can assign materials to individual cells.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
//              In interactive mode it responds to the keys listed
//              svtkOSPRayTestInteractor.h

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellData.h"
#include "svtkColorSeries.h"
#include "svtkDoubleArray.h"
#include "svtkLookupTable.h"
#include "svtkOSPRayMaterialLibrary.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOSPRayTestInteractor.h"
#include "svtkPlaneSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

int TestCategoricalMaterials(int argc, char* argv[])
{
  // set up the environment
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetSize(700, 700);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkOSPRayRendererNode::SetBackgroundMode(2, renderer);
  renderer->SetEnvironmentalBG(0.0, 0.0, 0.0);
  renderer->SetEnvironmentalBG2(0.8, 0.8, 1.0);
  renderer->GradientEnvironmentalBGOn();
  renWin->AddRenderer(renderer);
  svtkSmartPointer<svtkOSPRayPass> ospray = svtkSmartPointer<svtkOSPRayPass>::New();
  renderer->SetPass(ospray);
  svtkOSPRayRendererNode::SetRendererType("pathtracer", renderer);
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      break;
    }
  }

  svtkSmartPointer<svtkOSPRayTestInteractor> style = svtkSmartPointer<svtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  // make some predictable data to test with
  svtkSmartPointer<svtkPlaneSource> polysource = svtkSmartPointer<svtkPlaneSource>::New();
  polysource->SetXResolution(4);
  polysource->SetYResolution(3);
  polysource->Update();
  svtkPolyData* pd = polysource->GetOutput();
  svtkSmartPointer<svtkDoubleArray> da = svtkSmartPointer<svtkDoubleArray>::New();
  da->SetNumberOfComponents(1);
  da->SetName("test array");
  for (int i = 0; i < pd->GetNumberOfCells(); i++)
  {
    da->InsertNextValue(i);
  }
  pd->GetCellData()->SetScalars(da); // this is what we'll color by, including materials

  // Choose a color scheme
  svtkSmartPointer<svtkColorSeries> palettes = svtkSmartPointer<svtkColorSeries>::New();
  palettes->SetColorSchemeByName("Brewer Qualitative Set3");
  // Create the LUT and add some annotations.
  svtkSmartPointer<svtkLookupTable> lut = svtkSmartPointer<svtkLookupTable>::New();
  lut->SetAnnotation(0., "Zero");
  lut->SetAnnotation(1., "One");
  lut->SetAnnotation(2., "Two");
  lut->SetAnnotation(3, "Three");
  lut->SetAnnotation(4, "Four");
  lut->SetAnnotation(5, "Five");
  lut->SetAnnotation(6, "Six");
  lut->SetAnnotation(7, "Seven");
  lut->SetAnnotation(8, "Eight");
  lut->SetAnnotation(9, "Nine");
  lut->SetAnnotation(10, "Ten");
  lut->SetAnnotation(11, "Eleven");
  lut->SetAnnotation(12, "Twelve");
  palettes->BuildLookupTable(lut);

  // todo: test should turn on/off or let user do so
  lut->SetIndexedLookup(1);

  // get a hold of the material library
  svtkSmartPointer<svtkOSPRayMaterialLibrary> ml = svtkSmartPointer<svtkOSPRayMaterialLibrary>::New();
  svtkOSPRayRendererNode::SetMaterialLibrary(ml, renderer);
  // add materials to it
  ml->AddMaterial("Four", "Metal");
  ml->AddMaterial("One", "ThinGlass");
  // some of material names use the same low level material implementation
  ml->AddMaterial("Two", "ThinGlass");
  // but each one  can be tuned
  double green[3] = { 0.0, 0.9, 0.0 };
  ml->AddShaderVariable("Two", "attenuationColor", 3, green);
  ml->AddShaderVariable("Two", "eta", { 1. });
  ml->AddMaterial("Three", "ThinGlass");
  double blue[3] = { 0.0, 0.0, 0.9 };
  ml->AddShaderVariable("Three", "attenuationColor", 3, blue);
  ml->AddShaderVariable("Three", "eta", { 1.65 });

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  svtkProperty* prop;
  prop = actor->GetProperty();
  prop->SetMaterialName("Value Indexed"); // using several from the library

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputData(pd);
  mapper->SetLookupTable(lut);
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  // set up progressive rendering
  svtkCommand* looper = style->GetLooper(renWin);
  svtkCamera* cam = renderer->GetActiveCamera();
  iren->AddObserver(svtkCommand::KeyPressEvent, looper);
  cam->AddObserver(svtkCommand::ModifiedEvent, looper);
  iren->CreateRepeatingTimer(10); // every 10 msec we'll rerender if needed
  iren->AddObserver(svtkCommand::TimerEvent, looper);

  // todo: use standard svtk testing conventions
  iren->Start();
  return 0;
}
