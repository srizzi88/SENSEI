/*=========================================================================

  Program:   Visualization Toolkit
  Module:    X3DTest.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor.h"
#include "svtkConeSource.h"
#include "svtkDebugLeaks.h"
#include "svtkGlyph3D.h"
#include "svtkInformation.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkX3DExporter.h"

int X3DTest(int argc, char* argv[])
{
  svtkNew<svtkRenderer> renderer;
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(renderer);
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  svtkNew<svtkSphereSource> sphere;
  sphere->SetThetaResolution(8);
  sphere->SetPhiResolution(8);

  svtkNew<svtkPolyDataMapper> sphereMapper;
  sphereMapper->SetInputConnection(sphere->GetOutputPort());
  svtkNew<svtkActor> sphereActor;
  sphereActor->SetMapper(sphereMapper);

  svtkNew<svtkConeSource> cone;
  cone->SetResolution(6);

  svtkNew<svtkGlyph3D> glyph;
  glyph->SetInputConnection(sphere->GetOutputPort());
  glyph->SetSourceConnection(cone->GetOutputPort());
  glyph->SetVectorModeToUseNormal();
  glyph->SetScaleModeToScaleByVector();
  glyph->SetScaleFactor(0.25);

  svtkNew<svtkPolyDataMapper> spikeMapper;
  spikeMapper->SetInputConnection(glyph->GetOutputPort());

  svtkNew<svtkActor> spikeActor;
  spikeActor->SetMapper(spikeMapper);

  renderer->AddActor(sphereActor);
  renderer->AddActor(spikeActor);
  renderer->SetBackground(1, 1, 1);
  renWin->SetSize(300, 300);

  renWin->Render();

  svtkNew<svtkX3DExporter> exporter;
  exporter->SetInput(renWin);
  exporter->SetFileName("testX3DExporter.x3d");
  exporter->Update();
  exporter->Write();
  exporter->Print(std::cout);

  renderer->RemoveActor(sphereActor);
  renderer->RemoveActor(spikeActor);

  // now try the same with a composite dataset.
  svtkNew<svtkMultiBlockDataSet> mb;
  mb->SetBlock(0, glyph->GetOutputDataObject(0));
  mb->GetMetaData(0u)->Set(svtkMultiBlockDataSet::NAME(), "Spikes");
  mb->SetBlock(1, sphere->GetOutputDataObject(0));
  mb->GetMetaData(1u)->Set(svtkMultiBlockDataSet::NAME(), "Sphere");

  svtkNew<svtkPolyDataMapper> mbMapper;
  mbMapper->SetInputDataObject(mb);

  svtkNew<svtkActor> mbActor;
  mbActor->SetMapper(mbMapper);
  renderer->AddActor(mbActor);

  renWin->Render();
  exporter->SetFileName("testX3DExporter-composite.x3d");
  exporter->Update();
  exporter->Write();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }
  return !retVal;
}
