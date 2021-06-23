/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestPolygonalHandleRepresentations.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkDEMReader.h"
#include "svtkGlyphSource2D.h"
#include "svtkHandleWidget.h"
#include "svtkImageData.h"
#include "svtkImageDataGeometryFilter.h"
#include "svtkImageResample.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkLODActor.h"
#include "svtkLookupTable.h"
#include "svtkOrientedPolygonalHandleRepresentation3D.h"
#include "svtkPointHandleRepresentation3D.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataCollection.h"
#include "svtkPolyDataMapper.h"
#include "svtkPolyDataNormals.h"
#include "svtkPolygonalHandleRepresentation3D.h"
#include "svtkPolygonalSurfacePointPlacer.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTestUtilities.h"
#include "svtkTriangleFilter.h"
#include "svtkWarpScalar.h"

#include "svtkSmartPointer.h"
#define SVTK_CREATE(type, name) svtkSmartPointer<type> name = svtkSmartPointer<type>::New()

svtkSmartPointer<svtkHandleWidget> CreateWidget(svtkRenderWindowInteractor* iren, int shape, double x,
  double y, double z, bool cameraFacing = false, const char* label = nullptr,
  svtkActor* demActor = nullptr, svtkPolyData* demPolys = nullptr, bool constrainedToSurface = false,
  double heightOffsetAboveSurface = 0.0)
{
  SVTK_CREATE(svtkHandleWidget, widget);

  svtkHandleRepresentation* rep = nullptr;

  if (cameraFacing && shape <= 12)
  {
    rep = svtkOrientedPolygonalHandleRepresentation3D::New();

    SVTK_CREATE(svtkGlyphSource2D, glyphs);
    glyphs->SetGlyphType(shape);
    glyphs->SetScale(600);
    glyphs->Update();
    static_cast<svtkOrientedPolygonalHandleRepresentation3D*>(rep)->SetHandle(glyphs->GetOutput());
  }
  else
  {
    if (shape == 12)
    {
      rep = svtkPolygonalHandleRepresentation3D::New();

      SVTK_CREATE(svtkSphereSource, sphere);
      sphere->SetThetaResolution(10);
      sphere->SetPhiResolution(10);
      sphere->SetRadius(300.0);
      sphere->Update();
      static_cast<svtkPolygonalHandleRepresentation3D*>(rep)->SetHandle(sphere->GetOutput());
    }

    if (shape == 13)
    {
      rep = svtkPointHandleRepresentation3D::New();
    }
  }

  if (constrainedToSurface)
  {
    SVTK_CREATE(svtkPolygonalSurfacePointPlacer, pointPlacer);
    pointPlacer->AddProp(demActor);
    pointPlacer->GetPolys()->AddItem(demPolys);
    pointPlacer->SetDistanceOffset(heightOffsetAboveSurface);
    rep->SetPointPlacer(pointPlacer);

    // Let the surface constrained point-placer be the sole constraint dictating
    // the placement of handles. Lets not over-constrain it allowing axis
    // constrained interactions.
    widget->EnableAxisConstraintOff();
  }

  double xyz[3] = { x, y, z };
  rep->SetWorldPosition(xyz);
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  // Set some defaults on the handle widget
  double color[3] = { ((double)(shape % 4)) / 3.0, ((double)((shape + 3) % 7)) / 6.0,
    (double)(shape % 2) };
  double selectedColor[3] = { 1.0, 0.0, 0.0 };

  if (svtkAbstractPolygonalHandleRepresentation3D* arep =
        svtkAbstractPolygonalHandleRepresentation3D::SafeDownCast(rep))
  {
    arep->GetProperty()->SetColor(color);
    arep->GetProperty()->SetLineWidth(1.0);
    arep->GetSelectedProperty()->SetColor(selectedColor);

    if (label)
    {
      arep->SetLabelVisibility(1);
      arep->SetLabelText(label);
    }
  }

  if (svtkPointHandleRepresentation3D* prep = svtkPointHandleRepresentation3D::SafeDownCast(rep))
  {
    prep->GetProperty()->SetColor(color);
    prep->GetProperty()->SetLineWidth(1.0);
    prep->GetSelectedProperty()->SetColor(selectedColor);
  }

  rep->Delete();

  return widget;
}

int TestPolygonalHandleRepresentations(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "Demonstrates various polyonal handle representations in a scene." << std::endl;
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
  resample->SetAxisMagnificationFactor(0, 1);
  resample->SetAxisMagnificationFactor(1, 1);

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

  normals->SetInputConnection(warp->GetOutputPort());
  normals->SetFeatureAngle(60);
  normals->SplittingOff();

  // svtkPolygonalSurfacePointPlacer needs cell normals
  normals->ComputeCellNormalsOn();
  normals->Update();

  svtkPolyData* pd = normals->GetOutput();

  svtkSmartPointer<svtkPolyDataMapper> demMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  demMapper->SetInputConnection(normals->GetOutputPort());
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
  svtkSmartPointer<svtkHandleWidget> widget[14];
  widget[0] = CreateWidget(iren, SVTK_VERTEX_GLYPH, 561909, 5.11921e+06, 4381.48, true, "Vertex");
  widget[1] = CreateWidget(iren, SVTK_DASH_GLYPH, 559400, 5.11064e+06, 2323.25, true, "Dash");
  widget[2] = CreateWidget(iren, SVTK_CROSS_GLYPH, 563531, 5.11924e+06, 5202.51, true, "cross");
  widget[3] =
    CreateWidget(iren, SVTK_THICKCROSS_GLYPH, 563300, 5.11729e+06, 4865.47, true, "Thick Cross");
  widget[4] =
    CreateWidget(iren, SVTK_TRIANGLE_GLYPH, 564392, 5.11248e+06, 3936.91, true, "triangle");
  widget[5] = CreateWidget(iren, SVTK_SQUARE_GLYPH, 563715, 5.11484e+06, 4345.68, true, "square");
  widget[6] = CreateWidget(iren, SVTK_CIRCLE_GLYPH, 564705, 5.10849e+06, 2335.16, true, "circle");
  widget[7] = CreateWidget(iren, SVTK_DIAMOND_GLYPH, 560823, 5.1202e+06, 3783.94, true, "diamond");
  widget[8] = CreateWidget(iren, SVTK_ARROW_GLYPH, 559637, 5.12068e+06, 2718.66, true, "arrow");
  widget[9] =
    CreateWidget(iren, SVTK_THICKARROW_GLYPH, 560597, 5.10817e+06, 3582.44, true, "thickArrow");
  widget[10] =
    CreateWidget(iren, SVTK_HOOKEDARROW_GLYPH, 558266, 5.12137e+06, 2559.14, true, "hookedArrow");
  widget[11] =
    CreateWidget(iren, SVTK_EDGEARROW_GLYPH, 568869, 5.11028e+06, 2026.57, true, "EdgeArrow");
  widget[12] = CreateWidget(iren, 12, 561753, 5.11577e+06, 3183, false,
    "Sphere contrained to surface", demActor, pd, true, 100.0);
  widget[13] = CreateWidget(iren, 13, 562692, 5.11521e+06, 3355.65, false, "Crosshair");

  renWin->SetSize(600, 600);
  renWin->Render();
  iren->Initialize();

  for (unsigned int i = 0; i < 14; i++)
  {
    widget[i]->EnabledOn();
  }

  renWin->Render();

  iren->Start();

  return EXIT_SUCCESS;
}
