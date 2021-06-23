/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTilingCxx.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"
#include "svtksys/SystemTools.hxx"

#include "svtkActor.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"

#include "svtkActor2D.h"
#include "svtkCellData.h"
#include "svtkDataObject.h"
#include "svtkFloatArray.h"
#include "svtkImageMapper.h"
#include "svtkMath.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProgrammableAttributeDataFilter.h"
#include "svtkScalarBarActor.h"
#include "svtkScalarsToColors.h"
#include "svtkSphereSource.h"
#include "svtkWindowToImageFilter.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, var) svtkSmartPointer<type> var = svtkSmartPointer<type>::New()

void colorCells(void* arg)
{
  SVTK_CREATE(svtkMath, randomColorGenerator);
  svtkProgrammableAttributeDataFilter* randomColors =
    static_cast<svtkProgrammableAttributeDataFilter*>(arg);
  svtkPolyData* input = svtkPolyData::SafeDownCast(randomColors->GetInput());
  svtkPolyData* output = randomColors->GetPolyDataOutput();
  int numCells = input->GetNumberOfCells();
  SVTK_CREATE(svtkFloatArray, colors);
  colors->SetNumberOfTuples(numCells);

  for (int i = 0; i < numCells; i++)
  {
    colors->SetValue(i, randomColorGenerator->Random(0, 1));
  }

  output->GetCellData()->CopyScalarsOff();
  output->GetCellData()->PassData(input->GetCellData());
  output->GetCellData()->SetScalars(colors);
}

int TestTilingCxx(int argc, char* argv[])
{
  SVTK_CREATE(svtkSphereSource, sphere);
  sphere->SetThetaResolution(20);
  sphere->SetPhiResolution(40);

  // Compute random scalars (colors) for each cell
  SVTK_CREATE(svtkProgrammableAttributeDataFilter, randomColors);
  randomColors->SetInputConnection(sphere->GetOutputPort());
  randomColors->SetExecuteMethod(colorCells, randomColors);

  // mapper and actor
  SVTK_CREATE(svtkPolyDataMapper, mapper);
  mapper->SetInputConnection(randomColors->GetOutputPort());
  mapper->SetScalarRange(randomColors->GetPolyDataOutput()->GetScalarRange());

  SVTK_CREATE(svtkActor, sphereActor);
  sphereActor->SetMapper(mapper);

  // Create a scalar bar
  SVTK_CREATE(svtkScalarBarActor, scalarBar);
  scalarBar->SetLookupTable(mapper->GetLookupTable());
  scalarBar->SetTitle("Temperature");
  scalarBar->GetPositionCoordinate()->SetCoordinateSystemToNormalizedViewport();
  scalarBar->GetPositionCoordinate()->SetValue(0.1, 0.05);
  scalarBar->SetOrientationToVertical();
  scalarBar->SetWidth(0.8);
  scalarBar->SetHeight(0.9);
  scalarBar->SetLabelFormat("%-#6.3f");

  // Test the Get/Set Position
  scalarBar->SetPosition(scalarBar->GetPosition());

  // Create graphics stuff
  // Create the RenderWindow, Renderer and both Actors
  SVTK_CREATE(svtkRenderer, ren1);
  SVTK_CREATE(svtkRenderer, ren2);
  SVTK_CREATE(svtkRenderWindow, renWin);
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren1);
  renWin->AddRenderer(ren2);
  SVTK_CREATE(svtkRenderWindowInteractor, iren);
  iren->SetRenderWindow(renWin);

  ren1->AddActor(sphereActor);
  ren2->AddActor2D(scalarBar);
  renWin->SetSize(160, 160);
  ren1->SetViewport(0, 0, 0.75, 1.0);
  ren2->SetViewport(0.75, 0, 1.0, 1.0);
  ren2->SetBackground(0.3, 0.3, 0.3);

  // render the image
  scalarBar->SetNumberOfLabels(8);
  renWin->Render();
  renWin->Render(); // perform an extra render before capturing window

  svtksys::SystemTools::Delay(1000);

  SVTK_CREATE(svtkWindowToImageFilter, w2i);
  w2i->SetInput(renWin);
  w2i->SetScale(3, 2);
  w2i->Update();

  // copy the output
  svtkImageData* outputData = w2i->GetOutput()->NewInstance();
  outputData->DeepCopy(w2i->GetOutput());

  SVTK_CREATE(svtkImageMapper, ia);
  ia->SetInputData(outputData);
  scalarBar->ReleaseGraphicsResources(renWin);
  sphereActor->ReleaseGraphicsResources(renWin);
  ia->SetColorWindow(255);
  ia->SetColorLevel(127.5);

  SVTK_CREATE(svtkActor2D, ia2);
  ia2->SetMapper(ia);

  renWin->SetSize(480, 320);
  renWin->SetPosition(480, 320);

  ren2->RemoveViewProp(scalarBar);
  ren1->RemoveViewProp(sphereActor);
  ren1->AddActor(ia2);
  renWin->RemoveRenderer(ren2);
  ren1->SetViewport(0, 0, 1, 1);

  renWin->Render();
  renWin->Render();

  svtksys::SystemTools::Delay(1000);

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  outputData->Delete();
  return !retVal;
}
