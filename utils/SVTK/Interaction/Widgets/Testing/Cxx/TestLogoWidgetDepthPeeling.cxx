/*=========================================================================

  Program:   Visualization Toolkit
  Module:    TestLogoWidgetDepthPeeling.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example tests the svtkLogoWidget with depth peeling.
// The translucent sphere uses depth peeling. The logo image is translucent
// on the overlay. This test makes sure that depth peeling restore the
// blending state to render translucent geometry on the overlay.

// First include the required header files for the SVTK classes we are using.
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkCommand.h"
#include "svtkConeSource.h"
#include "svtkCylinderSource.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkLogoRepresentation.h"
#include "svtkLogoWidget.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSphereSource.h"
#include "svtkTIFFReader.h"
#include "svtkTestUtilities.h"

int TestLogoWidgetDepthPeeling(int argc, char* argv[])
{
  // Create the RenderWindow, Renderer and both Actors
  //
  svtkSmartPointer<svtkRenderer> ren1 = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(ren1);
  renWin->SetMultiSamples(0);
  renWin->SetAlphaBitPlanes(1);

  ren1->SetUseDepthPeeling(1);
  ren1->SetMaximumNumberOfPeels(200);
  ren1->SetOcclusionRatio(0.1);

  svtkSmartPointer<svtkInteractorStyleTrackballCamera> style =
    svtkSmartPointer<svtkInteractorStyleTrackballCamera>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);
  iren->SetInteractorStyle(style);

  // Create an image for the balloon widget
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/beach.tif");
  svtkSmartPointer<svtkTIFFReader> image1 = svtkSmartPointer<svtkTIFFReader>::New();
  image1->SetFileName(fname);
  /*
  "beach.tif" image contains ORIENTATION tag which is
  ORIENTATION_TOPLEFT (row 0 top, col 0 lhs) type. The TIFF
  reader parses this tag and sets the internal TIFF image
  orientation accordingly.  To overwrite this orientation with a svtk
  convention of ORIENTATION_BOTLEFT (row 0 bottom, col 0 lhs ), invoke
  SetOrientationType method with parameter value of 4.
  */
  image1->SetOrientationType(4);
  delete[] fname;
  image1->Update();

  // Create a test pipeline
  //
  svtkSmartPointer<svtkSphereSource> ss = svtkSmartPointer<svtkSphereSource>::New();
  svtkSmartPointer<svtkPolyDataMapper> mapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapper->SetInputConnection(ss->GetOutputPort());
  svtkSmartPointer<svtkActor> sph = svtkSmartPointer<svtkActor>::New();
  sph->SetMapper(mapper);

  svtkSmartPointer<svtkProperty> property = svtkSmartPointer<svtkProperty>::New();
  property->SetOpacity(0.2);
  property->SetColor(0.0, 1.0, 0.0);
  sph->SetProperty(property);

  svtkSmartPointer<svtkCylinderSource> cs = svtkSmartPointer<svtkCylinderSource>::New();
  svtkSmartPointer<svtkPolyDataMapper> csMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  csMapper->SetInputConnection(cs->GetOutputPort());
  svtkSmartPointer<svtkActor> cyl = svtkSmartPointer<svtkActor>::New();
  cyl->SetMapper(csMapper);
  cyl->AddPosition(5, 0, 0);

  svtkSmartPointer<svtkConeSource> coneSource = svtkSmartPointer<svtkConeSource>::New();
  svtkSmartPointer<svtkPolyDataMapper> coneMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  coneMapper->SetInputConnection(coneSource->GetOutputPort());
  svtkSmartPointer<svtkActor> cone = svtkSmartPointer<svtkActor>::New();
  cone->SetMapper(coneMapper);
  cone->AddPosition(0, 5, 0);

  // Create the widget
  svtkSmartPointer<svtkLogoRepresentation> rep = svtkSmartPointer<svtkLogoRepresentation>::New();
  rep->SetImage(image1->GetOutput());

  svtkSmartPointer<svtkLogoWidget> widget = svtkSmartPointer<svtkLogoWidget>::New();
  widget->SetInteractor(iren);
  widget->SetRepresentation(rep);

  // Add the actors to the renderer, set the background and size
  //
  ren1->AddActor(sph);
  ren1->AddActor(cyl);
  ren1->AddActor(cone);
  ren1->SetBackground(0.1, 0.2, 0.4);
  renWin->SetSize(300, 300);

  // record events
  svtkSmartPointer<svtkInteractorEventRecorder> recorder =
    svtkSmartPointer<svtkInteractorEventRecorder>::New();
  recorder->SetInteractor(iren);
  //  recorder->SetFileName("c:/record.log");
  //  recorder->Record();
  //  recorder->ReadFromInputStringOn();
  //  recorder->SetInputString(eventLog);

  // render the image
  //
  iren->Initialize();
  renWin->Render();
  widget->On();
  //  recorder->Play();

  // Remove the observers so we can go interactive. Without this the "-I"
  // testing option fails.
  recorder->Off();

  iren->Start();

  return EXIT_SUCCESS;
}
