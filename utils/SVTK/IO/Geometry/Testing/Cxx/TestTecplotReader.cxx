/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestTecPlotReader.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .NAME Test of svtkTecplotReader
// .SECTION Description
//

#include "svtkDebugLeaks.h"
#include "svtkTecplotReader.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkTestUtilities.h"

int TestTecplotReader(int argc, char* argv[])
{
  // we have to use a composite pipeline
  svtkCompositeDataPipeline* exec = svtkCompositeDataPipeline::New();
  svtkCompositeDataPipeline* exec2 = svtkCompositeDataPipeline::New();
  svtkCompositeDataPipeline* exec3 = svtkCompositeDataPipeline::New();

  // Basic visualization.
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  svtkRenderer* ren = svtkRenderer::New();
  renWin->AddRenderer(ren);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  // Create the reader.
  char* fname1 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/flow.tec");
  svtkSmartPointer<svtkTecplotReader> reader = svtkSmartPointer<svtkTecplotReader>::New();
  reader->SetFileName(fname1);
  reader->SetDataArrayStatus("V", 1); // both files have a property named V
  reader->Update();
  delete[] fname1;

  svtkSmartPointer<svtkCompositeDataGeometryFilter> geom =
    svtkSmartPointer<svtkCompositeDataGeometryFilter>::New();
  geom->SetExecutive(exec);
  geom->SetInputConnection(0, reader->GetOutputPort(0));
  geom->Update();

  svtkPolyData* data = geom->GetOutput();
  data->GetPointData()->SetScalars(data->GetPointData()->GetArray("V"));

  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputData(data);
  mapper->ScalarVisibilityOn();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarRange(-0.3, 0.3);

  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);

  // create the reader for the cell centered data set.
  char* fname2 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/cellcentered.tec");
  svtkSmartPointer<svtkTecplotReader> reader2 = svtkSmartPointer<svtkTecplotReader>::New();
  reader2->SetFileName(fname2);
  reader2->SetDataArrayStatus("V", 1); // both files have a property named V
  reader2->Update();
  delete[] fname2;

  svtkSmartPointer<svtkCompositeDataGeometryFilter> geom2 =
    svtkSmartPointer<svtkCompositeDataGeometryFilter>::New();
  geom2->SetExecutive(exec2);
  geom2->SetInputConnection(0, reader2->GetOutputPort(0));
  geom2->Update();

  svtkPolyData* data2 = geom2->GetOutput();
  data2->GetPointData()->SetScalars(data2->GetPointData()->GetArray("V"));

  svtkSmartPointer<svtkPolyDataMapper> mapper2 = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper2->SetInputData(data2);
  mapper2->ScalarVisibilityOn();
  mapper2->SetColorModeToMapScalars();
  mapper2->SetScalarRange(-0.3, 0.3);

  svtkSmartPointer<svtkActor> actor2 = svtkSmartPointer<svtkActor>::New();
  actor2->SetMapper(mapper2);

  // create the reader for the gzipped dataset
  char* fname3 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/flow.tec.gz");
  svtkSmartPointer<svtkTecplotReader> reader3 = svtkSmartPointer<svtkTecplotReader>::New();
  reader3->SetFileName(fname3);
  reader3->SetDataArrayStatus("V", 1); // both files have a property named V
  reader3->Update();
  delete[] fname3;

  svtkSmartPointer<svtkCompositeDataGeometryFilter> geom3 =
    svtkSmartPointer<svtkCompositeDataGeometryFilter>::New();
  geom3->SetExecutive(exec3);
  geom3->SetInputConnection(0, reader3->GetOutputPort(0));
  geom3->Update();

  svtkPolyData* data3 = geom3->GetOutput();
  data3->GetPointData()->SetScalars(data3->GetPointData()->GetArray("V"));

  svtkSmartPointer<svtkPolyDataMapper> mapper3 = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper3->SetInputData(data3);
  mapper3->ScalarVisibilityOn();
  mapper3->SetColorModeToMapScalars();
  mapper3->SetScalarRange(-0.3, 0.3);

  svtkSmartPointer<svtkActor> actor3 = svtkSmartPointer<svtkActor>::New();
  actor3->SetMapper(mapper3);
  actor3->SetPosition(1, 0, 0);

  ren->SetBackground(0.0, 0.0, 0.0);
  ren->AddActor(actor);
  ren->AddActor(actor2);
  ren->AddActor(actor3);
  renWin->SetSize(300, 300);
  svtkCamera* cam = ren->GetActiveCamera();
  ren->ResetCamera();
  cam->Azimuth(180);

  // interact with data
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  renWin->Delete();
  ren->Delete();
  iren->Delete();
  exec->Delete();
  exec2->Delete();
  exec3->Delete();
  return !retVal;
}
