/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestSphereHandleWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
// Tests the svtkHandleWidget when it uses a svtkSphereHandleRepresentation.
// See also TestPolygonalRepresentationHandleWidget.cxx to plug in any
// generic polydata as a handle.
//
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkCellArray.h"
#include "svtkDEMReader.h"
#include "svtkHandleWidget.h"
#include "svtkImageData.h"
#include "svtkImageDataGeometryFilter.h"
#include "svtkImageResample.h"
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
#include "svtkSphereHandleRepresentation.h"
#include "svtkTestUtilities.h"
#include "svtkTesting.h"
#include "svtkTriangleFilter.h"
#include "svtkWarpScalar.h"

#include "svtkDataSetWriter.h"
#include "svtkMultiThreader.h"

const char TestSphereHandleWidgetEventLog[] = "# StreamVersion 1\n"
                                              "MouseMoveEvent 181 152 0 0 0 0 0\n"
                                              "MouseMoveEvent 180 151 0 0 0 0 0\n"
                                              "MouseMoveEvent 179 150 0 0 0 0 0\n"
                                              "MouseMoveEvent 178 149 0 0 0 0 0\n"
                                              "MouseMoveEvent 177 147 0 0 0 0 0\n"
                                              "MouseMoveEvent 176 146 0 0 0 0 0\n"
                                              "MouseMoveEvent 176 145 0 0 0 0 0\n"
                                              "MouseMoveEvent 176 144 0 0 0 0 0\n"
                                              "MouseMoveEvent 175 144 0 0 0 0 0\n"
                                              "MouseMoveEvent 175 143 0 0 0 0 0\n"
                                              "MouseMoveEvent 175 142 0 0 0 0 0\n"
                                              "MouseMoveEvent 175 141 0 0 0 0 0\n"
                                              "MouseMoveEvent 174 141 0 0 0 0 0\n"
                                              "MouseMoveEvent 173 142 0 0 0 0 0\n"
                                              "MouseMoveEvent 173 143 0 0 0 0 0\n"
                                              "MouseMoveEvent 172 143 0 0 0 0 0\n"
                                              "MouseMoveEvent 172 144 0 0 0 0 0\n"
                                              "MouseMoveEvent 172 145 0 0 0 0 0\n"
                                              "MouseMoveEvent 172 146 0 0 0 0 0\n"
                                              "MouseMoveEvent 171 146 0 0 0 0 0\n"
                                              "MouseMoveEvent 170 146 0 0 0 0 0\n"
                                              "MouseMoveEvent 169 146 0 0 0 0 0\n"
                                              "MouseMoveEvent 168 146 0 0 0 0 0\n"
                                              "MouseMoveEvent 167 146 0 0 0 0 0\n"
                                              "MouseMoveEvent 166 146 0 0 0 0 0\n"
                                              "LeftButtonPressEvent 166 146 0 0 0 0 0\n"
                                              "RenderEvent 166 146 0 0 0 0 0\n"
                                              "RenderEvent 166 146 0 0 0 0 0\n"
                                              "MouseMoveEvent 164 146 0 0 0 0 0\n"
                                              "RenderEvent 164 146 0 0 0 0 0\n"
                                              "MouseMoveEvent 162 146 0 0 0 0 0\n"
                                              "RenderEvent 162 146 0 0 0 0 0\n"
                                              "MouseMoveEvent 160 146 0 0 0 0 0\n"
                                              "RenderEvent 160 146 0 0 0 0 0\n"
                                              "MouseMoveEvent 150 143 0 0 0 0 0\n"
                                              "RenderEvent 150 143 0 0 0 0 0\n"
                                              "MouseMoveEvent 142 140 0 0 0 0 0\n"
                                              "RenderEvent 142 140 0 0 0 0 0\n"
                                              "MouseMoveEvent 133 138 0 0 0 0 0\n"
                                              "RenderEvent 133 138 0 0 0 0 0\n"
                                              "MouseMoveEvent 111 132 0 0 0 0 0\n"
                                              "RenderEvent 111 132 0 0 0 0 0\n"
                                              "MouseMoveEvent 99 126 0 0 0 0 0\n"
                                              "RenderEvent 99 126 0 0 0 0 0\n"
                                              "MouseMoveEvent 95 125 0 0 0 0 0\n"
                                              "RenderEvent 95 125 0 0 0 0 0\n"
                                              "MouseMoveEvent 90 122 0 0 0 0 0\n"
                                              "RenderEvent 90 122 0 0 0 0 0\n"
                                              "MouseMoveEvent 87 121 0 0 0 0 0\n"
                                              "RenderEvent 87 121 0 0 0 0 0\n"
                                              "MouseMoveEvent 85 121 0 0 0 0 0\n"
                                              "RenderEvent 85 121 0 0 0 0 0\n"
                                              "MouseMoveEvent 79 120 0 0 0 0 0\n"
                                              "RenderEvent 79 120 0 0 0 0 0\n"
                                              "MouseMoveEvent 75 119 0 0 0 0 0\n"
                                              "RenderEvent 75 119 0 0 0 0 0\n"
                                              "MouseMoveEvent 73 118 0 0 0 0 0\n"
                                              "RenderEvent 73 118 0 0 0 0 0\n"
                                              "MouseMoveEvent 70 118 0 0 0 0 0\n"
                                              "RenderEvent 70 118 0 0 0 0 0\n"
                                              "MouseMoveEvent 67 118 0 0 0 0 0\n"
                                              "RenderEvent 67 118 0 0 0 0 0\n"
                                              "MouseMoveEvent 66 118 0 0 0 0 0\n"
                                              "RenderEvent 66 118 0 0 0 0 0\n"
                                              "MouseMoveEvent 63 118 0 0 0 0 0\n"
                                              "RenderEvent 63 118 0 0 0 0 0\n"
                                              "MouseMoveEvent 61 118 0 0 0 0 0\n"
                                              "RenderEvent 61 118 0 0 0 0 0\n"
                                              "MouseMoveEvent 58 118 0 0 0 0 0\n"
                                              "RenderEvent 58 118 0 0 0 0 0\n"
                                              "MouseMoveEvent 52 119 0 0 0 0 0\n"
                                              "RenderEvent 52 119 0 0 0 0 0\n"
                                              "MouseMoveEvent 50 120 0 0 0 0 0\n"
                                              "RenderEvent 50 120 0 0 0 0 0\n"
                                              "MouseMoveEvent 48 120 0 0 0 0 0\n"
                                              "RenderEvent 48 120 0 0 0 0 0\n"
                                              "MouseMoveEvent 47 120 0 0 0 0 0\n"
                                              "RenderEvent 47 120 0 0 0 0 0\n"
                                              "MouseMoveEvent 46 121 0 0 0 0 0\n"
                                              "RenderEvent 46 121 0 0 0 0 0\n"
                                              "MouseMoveEvent 45 121 0 0 0 0 0\n"
                                              "RenderEvent 45 121 0 0 0 0 0\n"
                                              "MouseMoveEvent 44 123 0 0 0 0 0\n"
                                              "RenderEvent 44 123 0 0 0 0 0\n"
                                              "MouseMoveEvent 43 124 0 0 0 0 0\n"
                                              "RenderEvent 43 124 0 0 0 0 0\n"
                                              "MouseMoveEvent 43 128 0 0 0 0 0\n"
                                              "RenderEvent 43 128 0 0 0 0 0\n"
                                              "MouseMoveEvent 43 132 0 0 0 0 0\n"
                                              "RenderEvent 43 132 0 0 0 0 0\n"
                                              "MouseMoveEvent 44 136 0 0 0 0 0\n"
                                              "RenderEvent 44 136 0 0 0 0 0\n"
                                              "MouseMoveEvent 45 140 0 0 0 0 0\n"
                                              "RenderEvent 45 140 0 0 0 0 0\n"
                                              "MouseMoveEvent 50 144 0 0 0 0 0\n"
                                              "RenderEvent 50 144 0 0 0 0 0\n"
                                              "MouseMoveEvent 54 148 0 0 0 0 0\n"
                                              "RenderEvent 54 148 0 0 0 0 0\n"
                                              "MouseMoveEvent 56 150 0 0 0 0 0\n"
                                              "RenderEvent 56 150 0 0 0 0 0\n"
                                              "MouseMoveEvent 62 156 0 0 0 0 0\n"
                                              "RenderEvent 62 156 0 0 0 0 0\n"
                                              "MouseMoveEvent 65 159 0 0 0 0 0\n"
                                              "RenderEvent 65 159 0 0 0 0 0\n"
                                              "MouseMoveEvent 68 160 0 0 0 0 0\n"
                                              "RenderEvent 68 160 0 0 0 0 0\n"
                                              "MouseMoveEvent 71 162 0 0 0 0 0\n"
                                              "RenderEvent 71 162 0 0 0 0 0\n"
                                              "MouseMoveEvent 76 164 0 0 0 0 0\n"
                                              "RenderEvent 76 164 0 0 0 0 0\n"
                                              "MouseMoveEvent 81 169 0 0 0 0 0\n"
                                              "RenderEvent 81 169 0 0 0 0 0\n"
                                              "MouseMoveEvent 84 170 0 0 0 0 0\n"
                                              "RenderEvent 84 170 0 0 0 0 0\n"
                                              "MouseMoveEvent 87 171 0 0 0 0 0\n"
                                              "RenderEvent 87 171 0 0 0 0 0\n"
                                              "MouseMoveEvent 92 174 0 0 0 0 0\n"
                                              "RenderEvent 92 174 0 0 0 0 0\n"
                                              "MouseMoveEvent 93 174 0 0 0 0 0\n"
                                              "RenderEvent 93 174 0 0 0 0 0\n"
                                              "MouseMoveEvent 96 174 0 0 0 0 0\n"
                                              "RenderEvent 96 174 0 0 0 0 0\n"
                                              "MouseMoveEvent 100 175 0 0 0 0 0\n"
                                              "RenderEvent 100 175 0 0 0 0 0\n"
                                              "MouseMoveEvent 104 175 0 0 0 0 0\n"
                                              "RenderEvent 104 175 0 0 0 0 0\n"
                                              "MouseMoveEvent 108 175 0 0 0 0 0\n"
                                              "RenderEvent 108 175 0 0 0 0 0\n"
                                              "MouseMoveEvent 114 175 0 0 0 0 0\n"
                                              "RenderEvent 114 175 0 0 0 0 0\n"
                                              "MouseMoveEvent 118 175 0 0 0 0 0\n"
                                              "RenderEvent 118 175 0 0 0 0 0\n"
                                              "MouseMoveEvent 123 174 0 0 0 0 0\n"
                                              "RenderEvent 123 174 0 0 0 0 0\n"
                                              "MouseMoveEvent 127 173 0 0 0 0 0\n"
                                              "RenderEvent 127 173 0 0 0 0 0\n"
                                              "MouseMoveEvent 133 172 0 0 0 0 0\n"
                                              "RenderEvent 133 172 0 0 0 0 0\n"
                                              "MouseMoveEvent 135 172 0 0 0 0 0\n"
                                              "RenderEvent 135 172 0 0 0 0 0\n"
                                              "MouseMoveEvent 140 172 0 0 0 0 0\n"
                                              "RenderEvent 140 172 0 0 0 0 0\n"
                                              "MouseMoveEvent 144 172 0 0 0 0 0\n"
                                              "RenderEvent 144 172 0 0 0 0 0\n"
                                              "MouseMoveEvent 148 172 0 0 0 0 0\n"
                                              "RenderEvent 148 172 0 0 0 0 0\n"
                                              "MouseMoveEvent 152 171 0 0 0 0 0\n"
                                              "RenderEvent 152 171 0 0 0 0 0\n"
                                              "MouseMoveEvent 156 171 0 0 0 0 0\n"
                                              "RenderEvent 156 171 0 0 0 0 0\n"
                                              "MouseMoveEvent 162 171 0 0 0 0 0\n"
                                              "RenderEvent 162 171 0 0 0 0 0\n"
                                              "MouseMoveEvent 164 171 0 0 0 0 0\n"
                                              "RenderEvent 164 171 0 0 0 0 0\n"
                                              "MouseMoveEvent 168 171 0 0 0 0 0\n"
                                              "RenderEvent 168 171 0 0 0 0 0\n"
                                              "MouseMoveEvent 175 171 0 0 0 0 0\n"
                                              "RenderEvent 175 171 0 0 0 0 0\n"
                                              "MouseMoveEvent 180 170 0 0 0 0 0\n"
                                              "RenderEvent 180 170 0 0 0 0 0\n"
                                              "MouseMoveEvent 184 170 0 0 0 0 0\n"
                                              "RenderEvent 184 170 0 0 0 0 0\n"
                                              "MouseMoveEvent 194 168 0 0 0 0 0\n"
                                              "RenderEvent 194 168 0 0 0 0 0\n"
                                              "MouseMoveEvent 198 168 0 0 0 0 0\n"
                                              "RenderEvent 198 168 0 0 0 0 0\n"
                                              "MouseMoveEvent 201 167 0 0 0 0 0\n"
                                              "RenderEvent 201 167 0 0 0 0 0\n"
                                              "MouseMoveEvent 205 166 0 0 0 0 0\n"
                                              "RenderEvent 205 166 0 0 0 0 0\n"
                                              "MouseMoveEvent 233 158 0 0 0 0 0\n"
                                              "RenderEvent 233 158 0 0 0 0 0\n"
                                              "LeftButtonReleaseEvent 233 158 0 0 0 0 0\n"
                                              "RenderEvent 233 158 0 0 0 0 0\n"
                                              "MouseMoveEvent 234 159 0 0 0 0 0\n"
                                              "MouseMoveEvent 238 163 0 0 0 0 0\n"
                                              "MouseMoveEvent 240 164 0 0 0 0 0\n"
                                              "MouseMoveEvent 248 168 0 0 0 0 0\n"
                                              "MouseMoveEvent 258 168 0 0 0 0 0\n"
                                              "MouseMoveEvent 266 168 0 0 0 0 0\n"
                                              "MouseMoveEvent 272 170 0 0 0 0 0\n"
                                              "MouseMoveEvent 271 170 0 0 0 0 0\n";

//#define RECORD

int TestSphereHandleWidget(int argc, char* argv[])
{
  if (argc < 2)
  {
    std::cerr << "Sphere widget with a sphere handle representation." << std::endl;
    return EXIT_FAILURE;
  }

  // Read height field.
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/SainteHelens.dem");

  svtkMultiThreader::SetGlobalMaximumNumberOfThreads(1);

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

  // cout << warp->GetOutput()->GetNumberOfCells() << endl;

  // svtkSmartPointer<svtkDataSetWriter> writer =
  //   svtkSmartPointer<svtkDataSetWriter>::New();
  // writer->SetInputConnection(resample->GetOutputPort());
  // writer->SetFileName("foo.svtk");
  // writer->Write();

  // Define a LUT mapping for the height field

  double lo = demReader->GetOutput()->GetScalarRange()[0];
  double hi = demReader->GetOutput()->GetScalarRange()[1];

  svtkSmartPointer<svtkLookupTable> lut = svtkSmartPointer<svtkLookupTable>::New();
  lut->SetHueRange(0.6, 0);
  lut->SetSaturationRange(1.0, 0);
  lut->SetValueRange(0.5, 1.0);

  svtkSmartPointer<svtkPolyDataNormals> normals = svtkSmartPointer<svtkPolyDataNormals>::New();

  svtkSmartPointer<svtkPolyDataMapper> demMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  demMapper->SetInputConnection(warp->GetOutputPort());
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
  svtkSmartPointer<svtkSphereHandleRepresentation> rep =
    svtkSmartPointer<svtkSphereHandleRepresentation>::New();
  widget->SetRepresentation(rep);

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

  renWin->Render();

  iren->Initialize();
  widget->EnabledOn();
  renWin->Render();
  ren1->ResetCamera();
  ren1->ResetCameraClippingRange();

  return svtkTesting::InteractorEventLoop(argc, argv, iren, TestSphereHandleWidgetEventLog);
}
