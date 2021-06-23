/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestQuadRotationalExtrusionMultiBlock.cxx

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
#include "svtkCompositeDataGeometryFilter.h"
#include "svtkInformation.h"
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
#include "svtkXMLPolyDataReader.h"

//----------------------------------------------------------------------------
int TestQuadRotationalExtrusionMultiBlock(int argc, char* argv[])
{
  // Read block 0 of 2D polygonal input mesh
  char* fName0 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SemiDisk/SemiDisk-0.vtp");
  svtkNew<svtkXMLPolyDataReader> reader0;
  reader0->SetFileName(fName0);
  reader0->Update();
  delete[] fName0;

  // Read block 1 of 2D polygonal input mesh
  char* fName1 = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SemiDisk/SemiDisk-1.vtp");
  svtkNew<svtkXMLPolyDataReader> reader1;
  reader1->SetFileName(fName1);
  reader1->Update();
  delete[] fName1;

  // Create multi-block data set tree for quad-based sweep
  svtkNew<svtkMultiBlockDataSet> inMesh;
  inMesh->SetNumberOfBlocks(2);
  inMesh->GetMetaData(static_cast<unsigned>(0))->Set(svtkCompositeDataSet::NAME(), "Block 0");
  inMesh->SetBlock(0, reader0->GetOutput());
  svtkNew<svtkMultiBlockDataSet> inMesh2;
  inMesh->SetBlock(1, inMesh2);
  inMesh2->SetNumberOfBlocks(1);
  inMesh2->GetMetaData(static_cast<unsigned>(0))->Set(svtkCompositeDataSet::NAME(), "Block 1");
  inMesh2->SetBlock(0, reader1->GetOutput());

  // Create 3/4 of a cylinder by rotational extrusion
  svtkNew<svtkQuadRotationalExtrusionFilter> sweeper;
  sweeper->SetResolution(18);
  sweeper->SetInputData(inMesh);
  sweeper->SetAxisToX();
  sweeper->SetDefaultAngle(270);
  sweeper->AddPerBlockAngle(1, 90.);
  sweeper->AddPerBlockAngle(3, 45.);

  // Turn composite output into single polydata
  svtkNew<svtkCompositeDataGeometryFilter> outMesh;
  outMesh->SetInputConnection(sweeper->GetOutputPort());

  // Create normals for smooth rendering
  svtkNew<svtkPolyDataNormals> normals;
  normals->SetInputConnection(outMesh->GetOutputPort());

  // Create mapper for surface representation of whole mesh
  svtkNew<svtkPolyDataMapper> outMeshMapper;
  outMeshMapper->SetInputConnection(normals->GetOutputPort());
  outMeshMapper->SetResolveCoincidentTopologyToPolygonOffset();

  // Create actor for surface representation of whole mesh
  svtkNew<svtkActor> outMeshActor;
  outMeshActor->SetMapper(outMeshMapper);
  outMeshActor->GetProperty()->SetRepresentationToSurface();
  outMeshActor->GetProperty()->SetInterpolationToGouraud();
  outMeshActor->GetProperty()->SetColor(.9, .9, .9);

  // Retrieve polydata blocks output by sweeper
  sweeper->Update();
  svtkMultiBlockDataSet* outMeshMB = sweeper->GetOutput();
  svtkPolyData* outMesh0 = svtkPolyData::SafeDownCast(outMeshMB->GetBlock(0));
  svtkMultiBlockDataSet* outMeshMB2 = svtkMultiBlockDataSet::SafeDownCast(outMeshMB->GetBlock(1));
  svtkPolyData* outMesh1 = svtkPolyData::SafeDownCast(outMeshMB2->GetBlock(0));

  // Create mapper for wireframe representation of block 0
  svtkNew<svtkPolyDataMapper> outBlockMapper0;
  outBlockMapper0->SetInputData(outMesh0);
  outBlockMapper0->SetResolveCoincidentTopologyToPolygonOffset();

  // Create actor for wireframe representation of block 0
  svtkNew<svtkActor> outBlockActor0;
  outBlockActor0->SetMapper(outBlockMapper0);
  outBlockActor0->GetProperty()->SetRepresentationToWireframe();
  outBlockActor0->GetProperty()->SetColor(.9, 0., 0.);
  outBlockActor0->GetProperty()->SetAmbient(1.);
  outBlockActor0->GetProperty()->SetDiffuse(0.);
  outBlockActor0->GetProperty()->SetSpecular(0.);

  // Create mapper for wireframe representation of block 1
  svtkNew<svtkPolyDataMapper> outBlockMapper1;
  outBlockMapper1->SetInputData(outMesh1);
  outBlockMapper1->SetResolveCoincidentTopologyToPolygonOffset();

  // Create actor for wireframe representation of block 1
  svtkNew<svtkActor> outBlockActor1;
  outBlockActor1->SetMapper(outBlockMapper1);
  outBlockActor1->GetProperty()->SetRepresentationToWireframe();
  outBlockActor1->GetProperty()->SetColor(0., .9, 0.);
  outBlockActor1->GetProperty()->SetAmbient(1.);
  outBlockActor1->GetProperty()->SetDiffuse(0.);
  outBlockActor1->GetProperty()->SetSpecular(0.);

  // Create a renderer, add actors to it
  svtkNew<svtkRenderer> ren1;
  ren1->AddActor(outMeshActor);
  ren1->AddActor(outBlockActor0);
  ren1->AddActor(outBlockActor1);
  ren1->SetBackground(1., 1., 1.);

  // Create a renderWindow
  svtkNew<svtkRenderWindow> renWin;
  renWin->AddRenderer(ren1);
  renWin->SetSize(400, 400);
  renWin->SetMultiSamples(0);

  // Create a good view angle
  svtkNew<svtkCamera> camera;
  // camera->SetClippingRange( 0.576398, 28.8199 );
  camera->SetFocalPoint(36.640094041788934, 0.3387609170199118, 1.2087523663629445);
  camera->SetPosition(37.77735939083618, 0.42739828159854326, 2.988046512725565);
  camera->SetViewUp(-0.40432906992858864, 0.8891923825021084, 0.21413759621072337);
  camera->SetViewAngle(30.);
  ren1->SetActiveCamera(camera);
  ren1->ResetCameraClippingRange();

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
