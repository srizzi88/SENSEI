/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSurfaceConstrainedHandleWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkDEMReader.h"
#include "svtkImageData.h"
#include "svtkImageDataGeometryFilter.h"
#include "svtkImageResample.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkLODActor.h"
#include "svtkLookupTable.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataCollection.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkTriangleFilter.h"
#include "svtkWarpScalar.h"

#include "svtkHandleWidget.h"
#include "svtkPointHandleRepresentation3D.h"
#include "svtkPolygonalSurfacePointPlacer.h"
#include "svtkTestUtilities.h"

int TestSurfaceConstrainedHandleWidget(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "Demonstrates interaction of a handle, so that it is constrained \n"
              << "to lie on a polygonal surface.\n\n"
              << "Usage args: [-DistanceOffset height_offset]." << std::endl;
    return EXIT_FAILURE;
  }

  // Read height field.
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SainteHelens.dem");

  // Read height field.
  //
  svtkSmartPointer<svtkDEMReader> demReader = svtkSmartPointer<svtkDEMReader>::New();
  demReader->SetFileName(fname);
  delete[] fname;

  svtkSmartPointer<svtkImageResample> resample = svtkSmartPointer<svtkImageResample>::New();
  resample->SetInputConnection(demReader->GetOutputPort());
  resample->SetDimensionality(2);
  resample->SetAxisMagnificationFactor(0, 1.0);
  resample->SetAxisMagnificationFactor(1, 1.0);

  // Extract geometry
  svtkSmartPointer<svtkImageDataGeometryFilter> surface =
    svtkSmartPointer<svtkImageDataGeometryFilter>::New();
  surface->SetInputConnection(resample->GetOutputPort());

  // The Dijkistra interpolator will not accept cells that aren't triangles
  svtkSmartPointer<svtkTriangleFilter> triangleFilter = svtkSmartPointer<svtkTriangleFilter>::New();
  triangleFilter->SetInputConnection(surface->GetOutputPort());
  triangleFilter->Update();

  svtkSmartPointer<svtkWarpScalar> warp = svtkSmartPointer<svtkWarpScalar>::New();
  warp->SetInputConnection(triangleFilter->GetOutputPort());
  warp->SetScaleFactor(1);
  warp->UseNormalOn();
  warp->SetNormal(0, 0, 1);
  warp->Update();

  // Define a LUT mapping for the height field

  double lo = demReader->GetOutput()->GetScalarRange()[0];
  double hi = demReader->GetOutput()->GetScalarRange()[1];

  svtkSmartPointer<svtkLookupTable> lut = svtkSmartPointer<svtkLookupTable>::New();
  lut->SetHueRange(0.6, 0);
  lut->SetSaturationRange(1.0, 0);
  lut->SetValueRange(0.5, 1.0);

  svtkSmartPointer<svtkPolyDataNormals> normals = svtkSmartPointer<svtkPolyDataNormals>::New();

  bool distanceOffsetSpecified = false;
  double distanceOffset = 0.0;
  for (int i = 0; i < argc - 1; i++)
  {
    if (strcmp("-DistanceOffset", argv[i]) == 0)
    {
      distanceOffset = atof(argv[i + 1]);
      distanceOffsetSpecified = true;
    }
  }

  if (distanceOffsetSpecified)
  {
    normals->SetInputConnection(warp->GetOutputPort());
    normals->SetFeatureAngle(60);
    normals->SplittingOff();

    // svtkPolygonalSurfacePointPlacer needs cell normals
    normals->ComputeCellNormalsOn();
    normals->Update();
  }

  svtkPolyData* pd = (distanceOffsetSpecified) ? normals->GetOutput() : warp->GetPolyDataOutput();

  svtkSmartPointer<svtkPolyDataMapper> demMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  demMapper->SetInputData(pd);
  demMapper->SetScalarRange(lo, hi);
  demMapper->SetLookupTable(lut);

  svtkSmartPointer<svtkActor> demActor = svtkSmartPointer<svtkActor>::New();
  demActor->SetMapper(demMapper);

  // Create the RenderWindow, Renderer and the DEM + path actors.

  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Add the actors to the renderer, set the background and size

  ren1->AddActor(demActor);

  ren1->GetActiveCamera()->SetViewUp(0, 0, 1);
  ren1->GetActiveCamera()->SetPosition(-99900, -21354, 131801);
  ren1->GetActiveCamera()->SetFocalPoint(41461, 41461, 2815);
  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();

  // Here comes the surface constrained handle widget stuff.....

  svtkSmartPointer<svtkHandleWidget> widget = svtkSmartPointer<svtkHandleWidget>::New();
  widget->SetInteractor(iren);
  svtkPointHandleRepresentation3D* rep =
    svtkPointHandleRepresentation3D::SafeDownCast(widget->GetRepresentation());

  svtkSmartPointer<svtkPolygonalSurfacePointPlacer> pointPlacer =
    svtkSmartPointer<svtkPolygonalSurfacePointPlacer>::New();
  pointPlacer->AddProp(demActor);
  pointPlacer->GetPolys()->AddItem(pd);
  rep->SetPointPlacer(pointPlacer);

  // Let the surface constrained point-placer be the sole constraint dictating
  // the placement of handles. Lets not over-constrain it allowing axis
  // constrained interactions.
  widget->EnableAxisConstraintOff();

  // Set some defaults on the handle widget
  double d[3] = { 562532, 5.11396e+06, 2618.62 };
  rep->SetWorldPosition(d);
  rep->GetProperty()->SetColor(1.0, 0.0, 0.0);
  rep->GetProperty()->SetLineWidth(1.0);
  rep->GetSelectedProperty()->SetColor(0.2, 0.0, 1.0);

  if (distanceOffsetSpecified)
  {
    pointPlacer->SetDistanceOffset(distanceOffset);
  }

  renWin->Render();
  iren->Initialize();
  widget->EnabledOn();

  iren->Start();

  return EXIT_SUCCESS;
}
