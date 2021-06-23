/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLODActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// The test creates a Kline, replaces the default strategy from svtkMaskPoints
// to svtkQuadricClustering ; so instead of seeing a point cloud during
// interaction, (when run with -I) you will see a low res kline.

#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkDataSetMapper.h"
#include "svtkLODActor.h"
#include "svtkLoopSubdivisionFilter.h"
#include "svtkMaskPoints.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkQuadricClustering.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"

int TestLODActor(int argc, char* argv[])
{
  // Create a Kline.

  svtkPoints* points = svtkPoints::New();
  points->InsertNextPoint(0, -16, 0);
  points->InsertNextPoint(0, 0, -14);
  points->InsertNextPoint(0, 0, 14);
  points->InsertNextPoint(14, 0, 0);
  points->InsertNextPoint(10, 20, -10);
  points->InsertNextPoint(10, 20, 10);
  points->InsertNextPoint(10, -20, -10);
  points->InsertNextPoint(10, -20, 10);
  points->InsertNextPoint(-10, -20, -10);
  points->InsertNextPoint(-10, -20, 10);
  points->InsertNextPoint(-10, 20, -10);
  points->InsertNextPoint(-10, 20, 10);
  points->InsertNextPoint(-2, 27, 0);
  points->InsertNextPoint(0, 27, 2);
  points->InsertNextPoint(0, 27, -2);
  points->InsertNextPoint(2, 27, 0);
  points->InsertNextPoint(-14, 4, -1);
  points->InsertNextPoint(-14, 3, 0);
  points->InsertNextPoint(-14, 5, 0);
  points->InsertNextPoint(-14, 4, 1);
  points->InsertNextPoint(-1, 38, -2);
  points->InsertNextPoint(-1, 38, 2);
  points->InsertNextPoint(2, 35, -2);
  points->InsertNextPoint(2, 35, 2);
  points->InsertNextPoint(17, 42, 0);
  points->InsertNextPoint(15, 40, 2);
  points->InsertNextPoint(15, 39, -2);
  points->InsertNextPoint(13, 37, 0);
  points->InsertNextPoint(19, -2, -2);
  points->InsertNextPoint(19, -2, 2);
  points->InsertNextPoint(15, 2, -2);
  points->InsertNextPoint(15, 2, 2);

  svtkCellArray* faces = svtkCellArray::New();
  faces->InsertNextCell(3);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(5);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(7);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(6);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(3);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(4);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(7);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(9);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(9);
  faces->InsertCellPoint(8);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(0);
  faces->InsertCellPoint(8);
  faces->InsertCellPoint(6);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(6);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(6);
  faces->InsertCellPoint(8);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(8);
  faces->InsertCellPoint(10);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(1);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(4);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(9);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(9);
  faces->InsertCellPoint(7);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(7);
  faces->InsertCellPoint(5);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(2);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(11);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(15);
  faces->InsertCellPoint(5);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(4);
  faces->InsertCellPoint(14);
  faces->InsertCellPoint(15);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(13);
  faces->InsertCellPoint(11);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(5);
  faces->InsertCellPoint(15);
  faces->InsertCellPoint(13);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(12);
  faces->InsertCellPoint(10);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(13);
  faces->InsertCellPoint(12);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(14);
  faces->InsertCellPoint(4);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(12);
  faces->InsertCellPoint(14);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(8);
  faces->InsertCellPoint(17);
  faces->InsertCellPoint(16);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(8);
  faces->InsertCellPoint(9);
  faces->InsertCellPoint(17);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(9);
  faces->InsertCellPoint(19);
  faces->InsertCellPoint(17);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(9);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(19);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(18);
  faces->InsertCellPoint(19);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(11);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(18);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(16);
  faces->InsertCellPoint(18);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(10);
  faces->InsertCellPoint(8);
  faces->InsertCellPoint(16);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(13);
  faces->InsertCellPoint(21);
  faces->InsertCellPoint(12);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(12);
  faces->InsertCellPoint(21);
  faces->InsertCellPoint(20);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(12);
  faces->InsertCellPoint(20);
  faces->InsertCellPoint(14);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(14);
  faces->InsertCellPoint(20);
  faces->InsertCellPoint(22);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(14);
  faces->InsertCellPoint(22);
  faces->InsertCellPoint(15);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(15);
  faces->InsertCellPoint(22);
  faces->InsertCellPoint(23);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(15);
  faces->InsertCellPoint(23);
  faces->InsertCellPoint(13);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(13);
  faces->InsertCellPoint(23);
  faces->InsertCellPoint(21);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(21);
  faces->InsertCellPoint(25);
  faces->InsertCellPoint(24);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(21);
  faces->InsertCellPoint(24);
  faces->InsertCellPoint(20);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(20);
  faces->InsertCellPoint(24);
  faces->InsertCellPoint(26);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(20);
  faces->InsertCellPoint(26);
  faces->InsertCellPoint(22);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(22);
  faces->InsertCellPoint(26);
  faces->InsertCellPoint(27);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(22);
  faces->InsertCellPoint(27);
  faces->InsertCellPoint(23);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(23);
  faces->InsertCellPoint(27);
  faces->InsertCellPoint(25);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(23);
  faces->InsertCellPoint(25);
  faces->InsertCellPoint(21);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(25);
  faces->InsertCellPoint(29);
  faces->InsertCellPoint(24);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(24);
  faces->InsertCellPoint(29);
  faces->InsertCellPoint(28);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(24);
  faces->InsertCellPoint(28);
  faces->InsertCellPoint(26);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(26);
  faces->InsertCellPoint(28);
  faces->InsertCellPoint(30);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(26);
  faces->InsertCellPoint(30);
  faces->InsertCellPoint(27);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(27);
  faces->InsertCellPoint(30);
  faces->InsertCellPoint(31);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(27);
  faces->InsertCellPoint(31);
  faces->InsertCellPoint(25);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(25);
  faces->InsertCellPoint(31);
  faces->InsertCellPoint(29);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(29);
  faces->InsertCellPoint(19);
  faces->InsertCellPoint(17);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(29);
  faces->InsertCellPoint(17);
  faces->InsertCellPoint(28);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(28);
  faces->InsertCellPoint(17);
  faces->InsertCellPoint(16);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(28);
  faces->InsertCellPoint(16);
  faces->InsertCellPoint(30);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(30);
  faces->InsertCellPoint(16);
  faces->InsertCellPoint(18);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(30);
  faces->InsertCellPoint(18);
  faces->InsertCellPoint(31);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(31);
  faces->InsertCellPoint(18);
  faces->InsertCellPoint(19);
  faces->InsertNextCell(3);
  faces->InsertCellPoint(31);
  faces->InsertCellPoint(19);
  faces->InsertCellPoint(29);

  svtkPolyData* model = svtkPolyData::New();
  model->SetPolys(faces);
  model->SetPoints(points);

  // Create the RenderWindow, Renderer and both Actors

  svtkRenderer* ren1 = svtkRenderer::New();
  svtkRenderWindow* renWin = svtkRenderWindow::New();
  renWin->AddRenderer(ren1);
  svtkRenderWindowInteractor* iren = svtkRenderWindowInteractor::New();
  iren->SetRenderWindow(renWin);

  svtkLoopSubdivisionFilter* subdivide = svtkLoopSubdivisionFilter::New();
  subdivide->SetInputData(model);
  subdivide->SetNumberOfSubdivisions(6);

  svtkDataSetMapper* mapper = svtkDataSetMapper::New();
  mapper->SetInputConnection(subdivide->GetOutputPort());

  svtkLODActor* rose = svtkLODActor::New();
  rose->SetMapper(mapper);

  // Now replace the default strategy of the LOD Actor from to show a low
  // resolution kline. We will use svtkQuadricClustering for this purpose.

  svtkQuadricClustering* q = svtkQuadricClustering::New();
  q->SetNumberOfXDivisions(8);
  q->SetNumberOfYDivisions(8);
  q->SetNumberOfZDivisions(8);
  q->UseInputPointsOn();
  rose->SetLowResFilter(q);
  q->Delete();

  q = svtkQuadricClustering::New();
  q->SetNumberOfXDivisions(5);
  q->SetNumberOfYDivisions(5);
  q->SetNumberOfZDivisions(5);
  q->UseInputPointsOn();
  rose->SetMediumResFilter(q);
  q->Delete();

  // Add the actors to the renderer, set the background and size

  ren1->AddActor(rose);

  svtkProperty* backP = svtkProperty::New();
  backP->SetDiffuseColor(1, 1, .3);
  rose->SetBackfaceProperty(backP);

  rose->GetProperty()->SetDiffuseColor(1, .4, .3);
  rose->GetProperty()->SetSpecular(.4);
  rose->GetProperty()->SetDiffuse(.8);
  rose->GetProperty()->SetSpecularPower(40);

  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // render the image

  ren1->ResetCamera();
  svtkCamera* cam1 = ren1->GetActiveCamera();
  cam1->Azimuth(-90);
  ren1->ResetCameraClippingRange();
  iren->Initialize();
  iren->SetDesiredUpdateRate(500);

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  model->Delete();
  backP->Delete();
  mapper->Delete();
  subdivide->Delete();
  ren1->Delete();
  renWin->Delete();
  rose->Delete();
  faces->Delete();
  points->Delete();
  iren->Delete();

  return !retVal;
}
