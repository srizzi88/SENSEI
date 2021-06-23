/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestCategoricalMultiBlock.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that we can give each block its own material and
// also override them easily.
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
#include "svtkCompositeDataDisplayAttributes.h"
#include "svtkCompositePolyDataMapper2.h"
#include "svtkDoubleArray.h"
#include "svtkLookupTable.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkOSPRayMaterialLibrary.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOSPRayTestInteractor.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"

int TestCategoricalMultiBlock(int argc, char* argv[])
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
  renderer->SetEnvironmentUp(1.0, 0.0, 0.0);
  renderer->SetEnvironmentRight(0.0, 1.0, 0.0);
  renWin->AddRenderer(renderer);
  svtkSmartPointer<svtkOSPRayPass> ospray = svtkSmartPointer<svtkOSPRayPass>::New();
  renderer->SetPass(ospray);
  svtkOSPRayRendererNode::SetRendererType("pathtracer", renderer);

  bool reduceNumMaterials = false;
  for (int i = 0; i < argc; ++i)
  {
    if (!strcmp(argv[i], "--OptiX"))
    {
      svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
      reduceNumMaterials =
        true; // Reduce number of MDL material instantiations to make test run faster
      break;
    }
  }

  svtkSmartPointer<svtkOSPRayTestInteractor> style = svtkSmartPointer<svtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  // make some predictable data to test with
  svtkSmartPointer<svtkMultiBlockDataSet> mbds = svtkSmartPointer<svtkMultiBlockDataSet>::New();
  mbds->SetNumberOfBlocks(12);
  for (int i = 0; i < 12; i++)
  {
    svtkSmartPointer<svtkSphereSource> polysource = svtkSmartPointer<svtkSphereSource>::New();
    polysource->SetPhiResolution(reduceNumMaterials ? 1 : 10);
    polysource->SetThetaResolution(reduceNumMaterials ? 1 : 10);
    polysource->SetCenter(i % 4, i / 4, 0);
    polysource->Update();

    svtkPolyData* pd = polysource->GetOutput();
    svtkSmartPointer<svtkDoubleArray> da = svtkSmartPointer<svtkDoubleArray>::New();
    da->SetNumberOfComponents(1);
    da->SetName("test array");
    for (int c = 0; c < pd->GetNumberOfCells(); c++)
    {
      da->InsertNextValue(i);
    }
    pd->GetCellData()->SetScalars(da);

    mbds->SetBlock(i, polysource->GetOutput());
  }

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
  ml->AddMaterial("Five", "Metal");
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
  prop->SetMaterialName("Value Indexed"); // making submaterials

  svtkSmartPointer<svtkCompositePolyDataMapper2> mapper =
    svtkSmartPointer<svtkCompositePolyDataMapper2>::New();
  mapper->SetInputDataObject(mbds);
  mapper->SetLookupTable(lut);
  actor->SetMapper(mapper);
  renderer->AddActor(actor);

  // override one of the block's with a different material
  svtkSmartPointer<svtkCompositeDataDisplayAttributes> cda =
    svtkSmartPointer<svtkCompositeDataDisplayAttributes>::New();
  mapper->SetCompositeDataDisplayAttributes(cda);

  unsigned int top_index = 0;
  auto dobj = cda->DataObjectFromIndex(12, mbds, top_index);
  cda->SetBlockMaterial(dobj, "Five");

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
