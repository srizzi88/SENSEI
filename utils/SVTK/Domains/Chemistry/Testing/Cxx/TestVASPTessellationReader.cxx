/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkExecutive.h"
#include "svtkInformation.h"
#include "svtkMolecule.h"
#include "svtkMoleculeMapper.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStreamingDemandDrivenPipeline.h"
#include "svtkVASPTessellationReader.h"

int TestVASPTessellationReader(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "Missing test file argument." << std::endl;
    return EXIT_FAILURE;
  }

  std::string fname(argv[1]);

  svtkNew<svtkVASPTessellationReader> reader;
  reader->SetFileName(fname.c_str());

  reader->UpdateInformation();
  svtkInformation* outInfo = reader->GetExecutive()->GetOutputInformation(0);
  double* times = outInfo->Get(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  int nTimes = outInfo->Length(svtkStreamingDemandDrivenPipeline::TIME_STEPS());
  if (nTimes < 8)
  {
    std::cerr << "Need at least 8 timesteps, only " << nTimes << " found.\n";
    return EXIT_FAILURE;
  }

  svtkNew<svtkDataSetSurfaceFilter> geomFilter;
  geomFilter->SetInputConnection(reader->GetOutputPort(1));

  // Show different time steps in each renderer:
  svtkNew<svtkRenderer> rens[4];
  rens[0]->SetViewport(0.0, 0.5, 0.5, 1.0);
  rens[1]->SetViewport(0.5, 0.5, 1.0, 1.0);
  rens[2]->SetViewport(0.0, 0.0, 0.5, 0.5);
  rens[3]->SetViewport(0.5, 0.0, 1.0, 0.5);

  svtkNew<svtkMoleculeMapper> molMappers[4];
  svtkNew<svtkActor> molActors[4];
  svtkNew<svtkPolyDataMapper> tessMappers[4];
  svtkNew<svtkActor> tessActors[4];
  svtkNew<svtkRenderWindow> win;
  for (size_t i = 0; i < 4; ++i)
  {
    // Render different timestamps for each:
    svtkNew<svtkMolecule> mol;
    reader->UpdateTimeStep(times[2 * i]);
    mol->ShallowCopy(reader->GetOutput(0));

    svtkNew<svtkPolyData> polyData;
    geomFilter->UpdateTimeStep(times[2 * i]);
    polyData->ShallowCopy(geomFilter->GetOutput(0));

    // Rendering setup:
    molMappers[i]->SetInputData(mol);
    molMappers[i]->UseBallAndStickSettings();
    molMappers[i]->RenderLatticeOn();
    molActors[i]->SetMapper(molMappers[i]);
    rens[i]->AddActor(molActors[i]);

    tessMappers[i]->SetInputData(polyData);
    tessMappers[i]->SelectColorArray("Atomic Numbers");
    tessMappers[i]->SetLookupTable(molMappers[i]->GetLookupTable());
    tessActors[i]->SetMapper(tessMappers[i]);
    tessActors[i]->GetProperty()->SetOpacity(0.5);
    rens[i]->AddActor(tessActors[i]);

    rens[i]->SetBackground(0.0, 0.0, 0.0);
    win->AddRenderer(rens[i]);
  }

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(win);

  win->SetSize(450, 450);
  win->Render();

  for (size_t i = 0; i < 4; ++i)
  {
    rens[i]->GetActiveCamera()->Dolly(1.5);
    rens[i]->ResetCameraClippingRange();
  }
  win->Render();

  // Finally render the scene and compare the image to a reference image
  win->SetMultiSamples(0);
  win->GetInteractor()->Initialize();
  win->GetInteractor()->Start();

  return EXIT_SUCCESS;
}
