/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQuadRotationalExtrusion.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// .SECTION Thanks
// This test was written by Philippe Pebay, Kitware SAS 2011

#include "svtkCamera.h"
#include "svtkInformation.h"
#include "svtkLineSource.h"
#include "svtkMultiBlockDataSet.h"
#include "svtkNew.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkQuadRotationalExtrusionFilter.h"
#include "svtkRegressionTestImage.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTestUtilities.h"

//----------------------------------------------------------------------------
int TestQuadRotationalExtrusion(int argc, char* argv[])
{
  // Create a line source
  svtkNew<svtkLineSource> line;
  line->SetPoint1(0., 1., 0.);
  line->SetPoint2(0., 1., 2.);
  line->SetResolution(10);
  line->Update();

  // Create mapper for line segment
  svtkNew<svtkPolyDataMapper> lineMapper;
  lineMapper->SetInputConnection(line->GetOutputPort());

  // Create actor for line segment
  svtkNew<svtkActor> lineActor;
  lineActor->SetMapper(lineMapper);
  lineActor->GetProperty()->SetLineWidth(5);
  lineActor->GetProperty()->SetColor(0., .749, 1.); // deep sky blue

  // Create multi-block data set for quad-based sweep
  svtkNew<svtkMultiBlockDataSet> lineMB;
  lineMB->SetNumberOfBlocks(1);
  lineMB->GetMetaData(static_cast<unsigned>(0))->Set(svtkCompositeDataSet::NAME(), "Line");
  lineMB->SetBlock(0, line->GetOutput());

  // Create 3/4 of a cylinder by rotational extrusion
  svtkNew<svtkQuadRotationalExtrusionFilter> lineSweeper;
  lineSweeper->SetResolution(20);
  lineSweeper->SetInputData(lineMB);
  lineSweeper->SetDefaultAngle(270);
  lineSweeper->Update();

  // Retrieve polydata output
  svtkMultiBlockDataSet* cylDS =
    svtkMultiBlockDataSet::SafeDownCast(lineSweeper->GetOutputDataObject(0));
  svtkPolyData* cyl = svtkPolyData::SafeDownCast(cylDS->GetBlock(0));

  // Create normals for smooth rendering
  svtkNew<svtkPolyDataNormals> normals;
  normals->SetInputData(cyl);

  // Create mapper for surface representation
  svtkNew<svtkPolyDataMapper> cylMapper;
  cylMapper->SetInputConnection(normals->GetOutputPort());
  cylMapper->SetResolveCoincidentTopologyToPolygonOffset();

  // Create mapper for wireframe representation
  svtkNew<svtkPolyDataMapper> cylMapperW;
  cylMapperW->SetInputData(cyl);
  cylMapperW->SetResolveCoincidentTopologyToPolygonOffset();

  // Create actor for surface representation
  svtkNew<svtkActor> cylActor;
  cylActor->SetMapper(cylMapper);
  cylActor->GetProperty()->SetRepresentationToSurface();
  cylActor->GetProperty()->SetInterpolationToGouraud();
  cylActor->GetProperty()->SetColor(1., 0.3882, .2784); // tomato

  // Create actor for wireframe representation
  svtkNew<svtkActor> cylActorW;
  cylActorW->SetMapper(cylMapperW);
  cylActorW->GetProperty()->SetRepresentationToWireframe();
  cylActorW->GetProperty()->SetColor(0., 0., 0.);
  cylActorW->GetProperty()->SetAmbient(1.);
  cylActorW->GetProperty()->SetDiffuse(0.);
  cylActorW->GetProperty()->SetSpecular(0.);

  // Create a renderer, add actors to it
  svtkNew<svtkRenderer> ren1;
  ren1->AddActor(lineActor);
  ren1->AddActor(cylActor);
  ren1->AddActor(cylActorW);
  ren1->SetBackground(1., 1., 1.);

  // Create a renderWindow
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);
  renWin->SetSize(300, 300);
  renWin->SetMultiSamples(0);

  // Create a good view angle
  svtkNew<svtkCamera> camera;
  camera->SetClippingRange(0.576398, 28.8199);
  camera->SetFocalPoint(0.0463079, -0.0356571, 1.01993);
  camera->SetPosition(-2.47044, 2.39516, -3.56066);
  camera->SetViewUp(0.607296, -0.513537, -0.606195);
  ren1->SetActiveCamera(camera);

  // Create interactor
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetRenderWindow(renWin);

  // Render and test
  renWin->Render();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
