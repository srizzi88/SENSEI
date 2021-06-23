/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestHierarchicalBoxPipeline.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This example demonstrates how hierarchical box (uniform rectilinear)
// AMR datasets can be processed using the new svtkHierarchicalBoxDataSet class.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit
// -D <path> => path to the data; the data should be in <path>/Data/

#include "svtkCamera.h"
#include "svtkCellDataToPointData.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkContourFilter.h"
#include "svtkDebugLeaks.h"
#include "svtkHierarchicalDataExtractLevel.h"
#include "svtkHierarchicalDataSetGeometryFilter.h"
#include "svtkHierarchicalPolyDataMapper.h"
#include "svtkOutlineCornerFilter.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkShrinkPolyData.h"
#include "svtkTestUtilities.h"
#include "svtkXMLHierarchicalBoxDataReader.h"

int TestHierarchicalBoxPipeline(int argc, char* argv[])
{
  svtkCompositeDataPipeline* prototype = svtkCompositeDataPipeline::New();
  svtkAlgorithm::SetDefaultExecutivePrototype(prototype);
  prototype->Delete();

  // Standard rendering classes
  svtkRenderer* ren = svtkRenderer::New();
  svtkCamera* cam = ren->GetActiveCamera();
  cam->SetPosition(-5.1828, 5.89733, 8.97969);
  cam->SetFocalPoint(14.6491, -2.08677, -8.92362);
  cam->SetViewUp(0.210794, 0.95813, -0.193784);

  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->SetMultiSamples(0);
  renWin->AddRenderer(ren);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  char* cfname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/chombo3d/chombo3d.vtm");

  svtkXMLHierarchicalBoxDataReader* reader = svtkXMLHierarchicalBoxDataReader::New();
  reader->SetFileName(cfname);
  delete[] cfname;

  // geometry filter
  svtkHierarchicalDataSetGeometryFilter* geom = svtkHierarchicalDataSetGeometryFilter::New();
  geom->SetInputConnection(0, reader->GetOutputPort(0));

  svtkShrinkPolyData* shrink = svtkShrinkPolyData::New();
  shrink->SetShrinkFactor(0.5);
  shrink->SetInputConnection(0, geom->GetOutputPort(0));

  // Rendering objects
  svtkHierarchicalPolyDataMapper* shMapper = svtkHierarchicalPolyDataMapper::New();
  shMapper->SetInputConnection(0, shrink->GetOutputPort(0));
  svtkActor* shActor = svtkActor::New();
  shActor->SetMapper(shMapper);
  shActor->GetProperty()->SetColor(0, 0, 1);
  ren->AddActor(shActor);

  // corner outline
  svtkOutlineCornerFilter* ocf = svtkOutlineCornerFilter::New();
  ocf->SetInputConnection(0, reader->GetOutputPort(0));

  // Rendering objects
  // This one is actually just a svtkPolyData so it doesn't need a hierarchical
  // mapper, but we use this one to test hierarchical mapper with polydata input
  svtkHierarchicalPolyDataMapper* ocMapper = svtkHierarchicalPolyDataMapper::New();
  ocMapper->SetInputConnection(0, ocf->GetOutputPort(0));
  svtkActor* ocActor = svtkActor::New();
  ocActor->SetMapper(ocMapper);
  ocActor->GetProperty()->SetColor(1, 0, 0);
  ren->AddActor(ocActor);

  // cell 2 point and contour
  svtkHierarchicalDataExtractLevel* el = svtkHierarchicalDataExtractLevel::New();
  el->SetInputConnection(0, reader->GetOutputPort(0));
  el->AddLevel(2);

  svtkCellDataToPointData* c2p = svtkCellDataToPointData::New();
  c2p->SetInputConnection(0, el->GetOutputPort(0));

  svtkContourFilter* contour = svtkContourFilter::New();
  contour->SetInputConnection(0, c2p->GetOutputPort(0));
  contour->SetValue(0, -0.013);
  contour->SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, "phi");

  // Rendering objects
  svtkHierarchicalPolyDataMapper* contMapper = svtkHierarchicalPolyDataMapper::New();
  contMapper->SetInputConnection(0, contour->GetOutputPort(0));
  svtkActor* contActor = svtkActor::New();
  contActor->SetMapper(contMapper);
  contActor->GetProperty()->SetColor(1, 0, 0);
  ren->AddActor(contActor);

  // Standard testing code.
  ocf->Delete();
  ocMapper->Delete();
  ocActor->Delete();
  c2p->Delete();
  contour->Delete();
  contMapper->Delete();
  contActor->Delete();
  ren->SetBackground(1, 1, 1);
  renWin->SetSize(300, 300);
  renWin->Render();
  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  // Cleanup
  el->Delete();
  geom->Delete();
  shMapper->Delete();
  shActor->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();
  reader->Delete();
  shrink->Delete();

  svtkAlgorithm::SetDefaultExecutivePrototype(0);
  return !retVal;
}
