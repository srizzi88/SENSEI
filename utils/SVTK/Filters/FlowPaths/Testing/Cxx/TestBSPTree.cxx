/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestBSPTree.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test covers intersection of a ray with many polygons
// using the svtkModifiedBSPTree class.
//
// The command line arguments are:
// -I        => run in interactive mode; unless this is used, the program will
//              not allow interaction and exit

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

#include "svtkActor.h"
#include "svtkBoundingBox.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkDataSetMapper.h"
#include "svtkGlyph3D.h"
#include "svtkLineSource.h"
#include "svtkLookupTable.h"
#include "svtkMath.h"
#include "svtkModifiedBSPTree.h"
#include "svtkPointData.h"
#include "svtkPointSource.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkSphereSource.h"
//
#include "svtkExtractSelection.h"
#include "svtkSelection.h"
#include "svtkSelectionNode.h"
#include "svtkSelectionSource.h"

//#define TESTING_LOOP

int TestBSPTree(int argc, char* argv[])
{
  // Standard rendering classes
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  renWin->AddRenderer(renderer);
  iren->SetRenderWindow(renWin);

  svtkIdType maxI = 0;
  int bestSeed = 0;
  for (int s = 931; s <= 931; s++)
  {
    renderer->RemoveAllViewProps();

    //
    // Create random point cloud
    //
    svtkMath::RandomSeed(s);
    svtkSmartPointer<svtkPointSource> points = svtkSmartPointer<svtkPointSource>::New();
    points->SetRadius(0.05);
    points->SetNumberOfPoints(30);

    //
    // Create small sphere
    //
    svtkSmartPointer<svtkSphereSource> sphere = svtkSmartPointer<svtkSphereSource>::New();
    sphere->SetRadius(0.0125);
    sphere->SetCenter(0.0, 0.0, 0.0);
    sphere->SetThetaResolution(16);
    sphere->SetPhiResolution(16);

    //
    // Glyph many small spheres over point cloud
    //
    svtkSmartPointer<svtkGlyph3D> glyph = svtkSmartPointer<svtkGlyph3D>::New();
    glyph->SetInputConnection(0, points->GetOutputPort(0));
    glyph->SetSourceConnection(sphere->GetOutputPort(0));
    glyph->SetScaling(0);
    glyph->Update();

    double bounds[6];
    glyph->GetOutput()->GetBounds(bounds);
    svtkBoundingBox box(bounds);
    double tol = box.GetDiagonalLength() / 1E6;

    //
    // Intersect Ray with BSP tree full of spheres
    //
    svtkSmartPointer<svtkModifiedBSPTree> bspTree = svtkSmartPointer<svtkModifiedBSPTree>::New();
    bspTree->SetDataSet(glyph->GetOutput(0));
    bspTree->SetMaxLevel(12);
    bspTree->SetNumberOfCellsPerNode(16);
    bspTree->BuildLocator();

    svtkSmartPointer<svtkPoints> verts = svtkSmartPointer<svtkPoints>::New();
    svtkSmartPointer<svtkIdList> cellIds = svtkSmartPointer<svtkIdList>::New();
    double p1[3] = { -0.1, -0.1, -0.1 }, p2[3] = { 0.1, 0.1, 0.1 };
    bspTree->IntersectWithLine(p1, p2, tol, verts, cellIds);

    svtkSmartPointer<svtkPolyData> intersections = svtkSmartPointer<svtkPolyData>::New();
    svtkSmartPointer<svtkCellArray> vertices = svtkSmartPointer<svtkCellArray>::New();
    svtkIdType n = verts->GetNumberOfPoints();
    for (svtkIdType i = 0; i < n; i++)
    {
      vertices->InsertNextCell(1, &i);
    }
    intersections->SetPoints(verts);
    intersections->SetVerts(vertices);

    std::cout << "Seed = " << s << " Number of intersections is " << n << std::endl;

    svtkSmartPointer<svtkSelectionSource> selection = svtkSmartPointer<svtkSelectionSource>::New();
    svtkSmartPointer<svtkExtractSelection> extract = svtkSmartPointer<svtkExtractSelection>::New();
    selection->SetContentType(svtkSelectionNode::INDICES);
    selection->SetFieldType(svtkSelectionNode::CELL);
    for (int i = 0; i < cellIds->GetNumberOfIds(); i++)
    {
      std::cout << cellIds->GetId(i) << ",";
      selection->AddID(-1, cellIds->GetId(i));
    }
    std::cout << std::endl;
    //
    extract->SetInputConnection(glyph->GetOutputPort());
    extract->SetSelectionConnection(selection->GetOutputPort());
    extract->Update();

    if (n > maxI)
    {
      maxI = n;
      bestSeed = s;
    }
    std::cout << "maxI = " << maxI << " At seed " << bestSeed << std::endl << std::endl;

#ifndef TESTING_LOOP
    //
    // Render cloud of target spheres
    //
    svtkSmartPointer<svtkPolyDataMapper> smapper = svtkSmartPointer<svtkPolyDataMapper>::New();
    smapper->SetInputConnection(glyph->GetOutputPort(0));

    svtkSmartPointer<svtkProperty> sproperty = svtkSmartPointer<svtkProperty>::New();
    sproperty->SetColor(1.0, 1.0, 1.0);
    //  sproperty->SetOpacity(0.25);
    sproperty->SetAmbient(0.0);
    sproperty->SetBackfaceCulling(1);
    sproperty->SetFrontfaceCulling(0);
    sproperty->SetRepresentationToPoints();
    sproperty->SetInterpolationToFlat();

    svtkSmartPointer<svtkActor> sactor = svtkSmartPointer<svtkActor>::New();
    sactor->SetMapper(smapper);
    sactor->SetProperty(sproperty);
    renderer->AddActor(sactor);

    //
    // Render Intersection points
    //
    svtkSmartPointer<svtkGlyph3D> iglyph = svtkSmartPointer<svtkGlyph3D>::New();
    iglyph->SetInputData(0, intersections);
    iglyph->SetSourceConnection(sphere->GetOutputPort(0));
    iglyph->SetScaling(1);
    iglyph->SetScaleFactor(0.05);
    iglyph->Update();

    svtkSmartPointer<svtkPolyDataMapper> imapper = svtkSmartPointer<svtkPolyDataMapper>::New();
    imapper->SetInputConnection(iglyph->GetOutputPort(0));

    svtkSmartPointer<svtkProperty> iproperty = svtkSmartPointer<svtkProperty>::New();
    iproperty->SetOpacity(1.0);
    iproperty->SetColor(0.0, 0.0, 1.0);
    iproperty->SetBackfaceCulling(1);
    iproperty->SetFrontfaceCulling(0);

    svtkSmartPointer<svtkActor> iactor = svtkSmartPointer<svtkActor>::New();
    iactor->SetMapper(imapper);
    iactor->SetProperty(iproperty);
    renderer->AddActor(iactor);

    //
    // Render Ray
    //
    svtkSmartPointer<svtkLineSource> ray = svtkSmartPointer<svtkLineSource>::New();
    ray->SetPoint1(p1);
    ray->SetPoint2(p2);

    svtkSmartPointer<svtkPolyDataMapper> rmapper = svtkSmartPointer<svtkPolyDataMapper>::New();
    rmapper->SetInputConnection(ray->GetOutputPort(0));

    svtkSmartPointer<svtkActor> lactor = svtkSmartPointer<svtkActor>::New();
    lactor->SetMapper(rmapper);
    renderer->AddActor(lactor);

    //
    // Render Intersected Cells (extracted using selection)
    //
    svtkSmartPointer<svtkDataSetMapper> cmapper = svtkSmartPointer<svtkDataSetMapper>::New();
    cmapper->SetInputConnection(extract->GetOutputPort(0));

    svtkSmartPointer<svtkProperty> cproperty = svtkSmartPointer<svtkProperty>::New();
    cproperty->SetColor(0.0, 1.0, 1.0);
    cproperty->SetBackfaceCulling(0);
    cproperty->SetFrontfaceCulling(0);
    cproperty->SetAmbient(1.0);
    cproperty->SetLineWidth(3.0);
    cproperty->SetRepresentationToWireframe();
    cproperty->SetInterpolationToFlat();

    svtkSmartPointer<svtkActor> cactor = svtkSmartPointer<svtkActor>::New();
    cactor->SetMapper(cmapper);
    cactor->SetProperty(cproperty);
    renderer->AddActor(cactor);

    //
    // Standard testing code.
    //
    renWin->SetSize(300, 300);
    renWin->SetMultiSamples(0);
    renWin->Render();
    renderer->GetActiveCamera()->SetPosition(0.0, 0.15, 0.0);
    renderer->GetActiveCamera()->SetFocalPoint(0.0, 0.0, 0.0);
    renderer->GetActiveCamera()->SetViewUp(0.0, 0.0, 1.0);
    renderer->SetBackground(0.0, 0.0, 0.0);
    renWin->Render();
    renderer->ResetCameraClippingRange();
    renWin->Render();
#endif
  }

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
