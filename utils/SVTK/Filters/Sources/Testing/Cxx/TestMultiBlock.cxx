/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestMultiBlock.cxx

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
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkCompositeDataPipeline.h"
#include "svtkContourFilter.h"
#include "svtkDebugLeaks.h"
#include "svtkExtractBlock.h"
#include "svtkMultiBlockPLOT3DReader.h"
#include "svtkOutlineCornerFilter.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkShrinkPolyData.h"
#include "svtkTestUtilities.h"

int TestMultiBlock(int argc, char* argv[])
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

  char* xyzname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/mbwavelet_ascii.xyz");
  char* qname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/mbwavelet_ascii.q");

  svtkMultiBlockPLOT3DReader* reader = svtkMultiBlockPLOT3DReader::New();
  reader->SetXYZFileName(xyzname);
  reader->SetQFileName(qname);
  reader->SetMultiGrid(1);
  reader->SetBinaryFile(0);
  delete[] xyzname;
  delete[] qname;

  // geometry filter
  svtkCompositeDataGeometryFilter* geom = svtkCompositeDataGeometryFilter::New();
  geom->SetInputConnection(0, reader->GetOutputPort(0));

  svtkShrinkPolyData* shrink = svtkShrinkPolyData::New();
  shrink->SetShrinkFactor(0.2);
  shrink->SetInputConnection(0, geom->GetOutputPort(0));

  // Rendering objects
  svtkPolyDataMapper* shMapper = svtkPolyDataMapper::New();
  shMapper->SetInputConnection(0, shrink->GetOutputPort(0));
  svtkActor* shActor = svtkActor::New();
  shActor->SetMapper(shMapper);
  shActor->GetProperty()->SetColor(0, 0, 1);
  ren->AddActor(shActor);

  // corner outline
  svtkOutlineCornerFilter* ocf = svtkOutlineCornerFilter::New();
  ocf->SetInputConnection(0, reader->GetOutputPort(0));

  // geometry filter
  svtkCompositeDataGeometryFilter* geom2 = svtkCompositeDataGeometryFilter::New();
  geom2->SetInputConnection(0, ocf->GetOutputPort(0));

  // Rendering objects
  svtkPolyDataMapper* ocMapper = svtkPolyDataMapper::New();
  ocMapper->SetInputConnection(0, geom2->GetOutputPort(0));
  svtkActor* ocActor = svtkActor::New();
  ocActor->SetMapper(ocMapper);
  ocActor->GetProperty()->SetColor(1, 0, 0);
  ren->AddActor(ocActor);

  // extract a block
  svtkExtractBlock* eds = svtkExtractBlock::New();
  eds->SetInputConnection(0, reader->GetOutputPort(0));
  eds->AddIndex(2);

  // contour
  svtkContourFilter* contour = svtkContourFilter::New();
  contour->SetInputConnection(0, eds->GetOutputPort(0));
  contour->SetValue(0, 149);

  // geometry filter
  svtkCompositeDataGeometryFilter* geom3 = svtkCompositeDataGeometryFilter::New();
  geom3->SetInputConnection(0, contour->GetOutputPort(0));

  // Rendering objects
  svtkPolyDataMapper* contMapper = svtkPolyDataMapper::New();
  contMapper->SetInputConnection(0, geom3->GetOutputPort(0));
  svtkActor* contActor = svtkActor::New();
  contActor->SetMapper(contMapper);
  contActor->GetProperty()->SetColor(1, 0, 0);
  ren->AddActor(contActor);

  // Standard testing code.
  eds->Delete();
  ocf->Delete();
  geom2->Delete();
  ocMapper->Delete();
  ocActor->Delete();
  contour->Delete();
  geom3->Delete();
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
  geom->Delete();
  shMapper->Delete();
  shActor->Delete();
  ren->Delete();
  renWin->Delete();
  iren->Delete();
  reader->Delete();
  shrink->Delete();

  svtkAlgorithm::SetDefaultExecutivePrototype(nullptr);
  return !retVal;
}
