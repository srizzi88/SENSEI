/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestExtractSubsetWithSeed.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#if SVTK_MODULE_ENABLE_SVTK_ParallelMPI
#include "svtkMPIController.h"
#else
#include "svtkDummyController.h"
#endif

#include "svtkActor.h"
#include "svtkCompositePolyDataMapper2.h"
#include "svtkExtractSubsetWithSeed.h"
#include "svtkGeometryFilter.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkStructuredGridOutlineFilter.h"
#include "svtkTestUtilities.h"
#include "svtkXMLStructuredGridReader.h"

namespace
{
svtkSmartPointer<svtkDataObject> GetDataSet(int argc, char* argv[])
{
  char* fname[3];
  fname[0] = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/multicomb_0.vts");
  fname[1] = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/multicomb_1.vts");
  fname[2] = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/multicomb_2.vts");

  svtkNew<svtkMultiBlockDataSet> mb;
  for (int cc = 0; cc < 3; ++cc)
  {
    svtkNew<svtkXMLStructuredGridReader> reader;
    reader->SetFileName(fname[cc]);
    reader->Update();

    mb->SetBlock(cc, reader->GetOutputDataObject(0));

    delete[] fname[cc];
  }
  return mb;
}
}

int TestExtractSubsetWithSeed(int argc, char* argv[])
{
#if SVTK_MODULE_ENABLE_SVTK_ParallelMPI
  svtkMPIController* contr = svtkMPIController::New();
#else
  svtkDummyController* contr = svtkDummyController::New();
#endif
  contr->Initialize(&argc, &argv);
  svtkMultiProcessController::SetGlobalController(contr);

  auto data = GetDataSet(argc, argv);

  svtkNew<svtkExtractSubsetWithSeed> extract1;
  extract1->SetInputDataObject(data);
  extract1->SetSeed(1.74, 0.65, 26.6);
  extract1->SetDirectionToLineI();
  extract1->Update();

  svtkNew<svtkGeometryFilter> geom1;
  geom1->SetInputConnection(extract1->GetOutputPort());

  svtkNew<svtkCompositePolyDataMapper2> mapper1;
  mapper1->SetInputConnection(geom1->GetOutputPort());

  svtkNew<svtkActor> actor1;
  actor1->SetMapper(mapper1);

  svtkNew<svtkExtractSubsetWithSeed> extract2;
  extract2->SetInputDataObject(data);
  extract2->SetSeed(1.74, 0.65, 26.6);
  extract2->SetDirectionToPlaneJK();
  extract2->Update();

  svtkNew<svtkGeometryFilter> geom2;
  geom2->SetInputConnection(extract2->GetOutputPort());

  svtkNew<svtkCompositePolyDataMapper2> mapper2;
  mapper2->SetInputConnection(geom2->GetOutputPort());

  svtkNew<svtkActor> actor2;
  actor2->SetMapper(mapper2);

  svtkNew<svtkRenderWindow> renWin;
  svtkNew<svtkRenderer> renderer;
  renWin->AddRenderer(renderer);

  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  renderer->AddActor(actor1);
  renderer->AddActor(actor2);

  svtkNew<svtkStructuredGridOutlineFilter> outline;
  outline->SetInputDataObject(data);

  svtkNew<svtkCompositePolyDataMapper2> mapperOutline;
  mapperOutline->SetInputConnection(outline->GetOutputPort());

  svtkNew<svtkActor> actorOutline;
  actorOutline->SetMapper(mapperOutline);
  renderer->AddActor(actorOutline);

  renWin->Render();
  renderer->ResetCamera();
  renWin->Render();

  int retVal = svtkRegressionTester::Test(argc, argv, renWin, 10);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  svtkMultiProcessController::SetGlobalController(nullptr);
  contr->Finalize();
  contr->Delete();
  return !retVal;
}
