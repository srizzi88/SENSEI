/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestOSPRayImplicits.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// This test verifies that sizing of implicits spheres and cylinders for
// points and lines works as expected.
//
// The command line arguments are:
// -I         => run in interactive mode; unless this is used, the program will
//               not allow interaction and exit.
//               In interactive mode it responds to the keys listed
//               svtkOSPRayTestInteractor.h
// -GL        => users OpenGL instead of OSPRay to render

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkDataArray.h"
#include "svtkDataSetSurfaceFilter.h"
#include "svtkDoubleArray.h"
#include "svtkExtractEdges.h"
#include "svtkGlyph3D.h"
#include "svtkGlyphSource2D.h"
#include "svtkImageData.h"
#include "svtkInformation.h"
#include "svtkOSPRayActorNode.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOpenGLRenderer.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRTAnalyticSource.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkScalarsToColors.h"
#include "svtkShrinkFilter.h"
#include "svtkSmartPointer.h"

#include <string>
#include <vector>

#include "svtkOSPRayTestInteractor.h"

int TestOSPRayImplicits(int argc, char* argv[])
{
  bool useGL = false;
  for (int i = 0; i < argc; i++)
  {
    if (!strcmp(argv[i], "-GL"))
    {
      useGL = true;
    }
  }

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  iren->SetRenderWindow(renWin);
  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renWin->AddRenderer(renderer);
  renderer->AutomaticLightCreationOn();
  renderer->SetBackground(0.75, 0.75, 0.75);
  renderer->SetEnvironmentalBG(0.75, 0.75, 0.75);
  renWin->SetSize(600, 550);

  svtkSmartPointer<svtkOSPRayPass> ospray = svtkSmartPointer<svtkOSPRayPass>::New();
  if (!useGL)
  {
    for (int i = 0; i < argc; ++i)
    {
      if (!strcmp(argv[i], "--OptiX"))
      {
        svtkOSPRayRendererNode::SetRendererType("optix pathtracer", renderer);
        break;
      }
    }

    renderer->SetPass(ospray);
  }

  svtkSmartPointer<svtkRTAnalyticSource> wavelet = svtkSmartPointer<svtkRTAnalyticSource>::New();
  wavelet->SetWholeExtent(-10, 10, -10, 10, -10, 10);
  wavelet->SetSubsampleRate(5);
  wavelet->Update();
  // use a more predictable array
  svtkSmartPointer<svtkDoubleArray> da = svtkSmartPointer<svtkDoubleArray>::New();
  da->SetName("testarray1");
  da->SetNumberOfComponents(1);
  svtkDataSet* ds = wavelet->GetOutput();
  ds->GetPointData()->AddArray(da);
  int np = ds->GetNumberOfPoints();
  for (int i = 0; i < np; i++)
  {
    da->InsertNextValue((double)i / (double)np);
  }

  svtkSmartPointer<svtkDataSetSurfaceFilter> surfacer =
    svtkSmartPointer<svtkDataSetSurfaceFilter>::New();
  surfacer->SetInputData(ds);
  svtkSmartPointer<svtkShrinkFilter> shrinker = svtkSmartPointer<svtkShrinkFilter>::New();
  shrinker->SetShrinkFactor(0.5);
  shrinker->SetInputConnection(surfacer->GetOutputPort());

  // measure it for placements
  shrinker->Update();
  const double* bds = svtkDataSet::SafeDownCast(shrinker->GetOutputDataObject(0))->GetBounds();
  double x0 = bds[0];
  double y0 = bds[2];
  double z0 = bds[4];
  double dx = (bds[1] - bds[0]) * 1.2;
  double dy = (bds[3] - bds[2]) * 1.2;
  double dz = (bds[5] - bds[4]) * 1.2;

  // make points, point rep works too but only gets outer shell
  svtkSmartPointer<svtkGlyphSource2D> glyph = svtkSmartPointer<svtkGlyphSource2D>::New();
  glyph->SetGlyphTypeToVertex();
  svtkSmartPointer<svtkGlyph3D> glyphFilter = svtkSmartPointer<svtkGlyph3D>::New();
  glyphFilter->SetInputConnection(shrinker->GetOutputPort());
  glyphFilter->SetSourceConnection(glyph->GetOutputPort());

  svtkSmartPointer<svtkExtractEdges> edgeFilter = svtkSmartPointer<svtkExtractEdges>::New();
  edgeFilter->SetInputConnection(shrinker->GetOutputPort());

  // spheres ///////////////////////
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(glyphFilter->GetOutputPort());
  svtkSmartPointer<svtkActor> actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(actor);
  actor->SetPosition(x0 + dx * 0, y0 + dy * 0, z0 + dz * 0);
  svtkOSPRayTestInteractor::AddName("Points default");

  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(glyphFilter->GetOutputPort());
  actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(actor);
  actor->SetPosition(x0 + dx * 1, y0 + dy * 0, z0 + dz * 0);
  actor->GetProperty()->SetPointSize(5);
  svtkOSPRayTestInteractor::AddName("Points SetPointSize()");

  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(glyphFilter->GetOutputPort());
  actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(actor);
  actor->SetPosition(x0 + dx * 2, y0 + dy * 0, z0 + dz * 0);
  svtkInformation* mapInfo = mapper->GetInformation();
  mapInfo->Set(svtkOSPRayActorNode::ENABLE_SCALING(), 1);
  mapInfo->Set(svtkOSPRayActorNode::SCALE_ARRAY_NAME(), "testarray1");
  svtkOSPRayTestInteractor::AddName("Points SCALE_ARRAY");

  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(glyphFilter->GetOutputPort());
  actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToPoints();
  renderer->AddActor(actor);
  actor->SetPosition(x0 + dx * 3, y0 + dy * 0, z0 + dz * 0);
  mapInfo = mapper->GetInformation();
  mapInfo->Set(svtkOSPRayActorNode::ENABLE_SCALING(), 1);
  mapInfo->Set(svtkOSPRayActorNode::SCALE_ARRAY_NAME(), "testarray1");
  svtkSmartPointer<svtkPiecewiseFunction> scaleFunction =
    svtkSmartPointer<svtkPiecewiseFunction>::New();
  scaleFunction->AddPoint(0.00, 0.0);
  scaleFunction->AddPoint(0.50, 0.0);
  scaleFunction->AddPoint(0.51, 0.1);
  scaleFunction->AddPoint(1.00, 1.2);
  mapInfo->Set(svtkOSPRayActorNode::SCALE_FUNCTION(), scaleFunction);
  svtkOSPRayTestInteractor::AddName("Points SCALE_FUNCTION on SCALE_ARRAY");

  // cylinders ////////////////
  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(edgeFilter->GetOutputPort());
  actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor);
  actor->SetPosition(x0 + dx * 0, y0 + dy * 2, z0 + dz * 0);
  svtkOSPRayTestInteractor::AddName("Wireframe default");

  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(edgeFilter->GetOutputPort());
  actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor);
  actor->SetPosition(x0 + dx * 1, y0 + dy * 2, z0 + dz * 0);
  actor->GetProperty()->SetLineWidth(5);
  svtkOSPRayTestInteractor::AddName("Wireframe LineWidth");

  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(edgeFilter->GetOutputPort());
  actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor);
  actor->SetPosition(x0 + dx * 2, y0 + dy * 2, z0 + dz * 0);
  svtkOSPRayActorNode::SetEnableScaling(1, actor);
  svtkOSPRayActorNode::SetScaleArrayName("testarray1", actor);
  svtkOSPRayTestInteractor::AddName("Wireframe SCALE_ARRAY");

  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(edgeFilter->GetOutputPort());
  actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToWireframe();
  renderer->AddActor(actor);
  actor->SetPosition(x0 + dx * 3, y0 + dy * 2, z0 + dz * 0);
  mapInfo = mapper->GetInformation();
  mapInfo->Set(svtkOSPRayActorNode::ENABLE_SCALING(), 1);
  mapInfo->Set(svtkOSPRayActorNode::SCALE_ARRAY_NAME(), "testarray1");
  scaleFunction = svtkSmartPointer<svtkPiecewiseFunction>::New();
  scaleFunction->AddPoint(0.00, 0.0);
  scaleFunction->AddPoint(0.50, 0.0);
  scaleFunction->AddPoint(0.51, 0.1);
  scaleFunction->AddPoint(1.00, 1.2);
  mapInfo->Set(svtkOSPRayActorNode::SCALE_FUNCTION(), scaleFunction);
  svtkOSPRayTestInteractor::AddName("Wireframe SCALE_FUNCTION on SCALE_ARRAY");

  // reference values shown as colors /////////////////
  mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(surfacer->GetOutputPort());
  surfacer->Update();
  mapper->ScalarVisibilityOn();
  mapper->CreateDefaultLookupTable();
  mapper->SetColorModeToMapScalars();
  mapper->SetScalarModeToUsePointFieldData();
  mapper->SelectColorArray("testarray1");
  double* range = surfacer->GetOutput()->GetPointData()->GetArray("testarray1")->GetRange();
  mapper->SetScalarRange(range);
  actor = svtkSmartPointer<svtkActor>::New();
  actor->SetMapper(mapper);
  actor->GetProperty()->SetRepresentationToSurface();

  renderer->AddActor(actor);
  actor->SetPosition(x0 + dx * 2, y0 + dy * 1, z0 + dz * 0);
  svtkOSPRayTestInteractor::AddName("Reference values as colors");

  // just show it //////////////////
  renWin->Render();
  renderer->ResetCamera();

  svtkSmartPointer<svtkOSPRayTestInteractor> style = svtkSmartPointer<svtkOSPRayTestInteractor>::New();
  style->SetPipelineControlPoints(renderer, ospray, nullptr);
  iren->SetInteractorStyle(style);
  style->SetCurrentRenderer(renderer);

  iren->Start();

  return 0;
}
