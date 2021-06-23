/*=========================================================================

  Program:   Visualization Toolkit
  Module:    Medical3.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

//
// This example reads a volume dataset, extracts two isosurfaces that
// represent the skin and bone, creates three orthogonal planes
// (sagittal, axial, coronal), and displays them.
//
#include <svtkActor.h>
#include <svtkCamera.h>
#include <svtkContourFilter.h>
#include <svtkImageActor.h>
#include <svtkImageData.h>
#include <svtkImageDataGeometryFilter.h>
#include <svtkImageMapToColors.h>
#include <svtkImageMapper3D.h>
#include <svtkLookupTable.h>
#include <svtkOutlineFilter.h>
#include <svtkPolyDataMapper.h>
#include <svtkPolyDataNormals.h>
#include <svtkProperty.h>
#include <svtkRegressionTestImage.h>
#include <svtkRenderWindow.h>
#include <svtkRenderWindowInteractor.h>
#include <svtkRenderer.h>
#include <svtkSmartPointer.h>
#include <svtkStripper.h>
#include <svtkVolume16Reader.h>

int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    cout << "Usage: " << argv[0] << " DATADIR/headsq/quarter" << endl;
    return EXIT_FAILURE;
  }

  // Create the renderer, the render window, and the interactor. The
  // renderer draws into the render window, the interactor enables
  // mouse- and keyboard-based interaction with the data within the
  // render window.
  //
  svtkSmartPointer<svtkRenderer> aRenderer = svtkSmartPointer<svtkRenderer>::New();
  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->AddRenderer(aRenderer);
  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  // Set a background color for the renderer and set the size of the
  // render window (expressed in pixels).
  aRenderer->SetBackground(.2, .3, .4);
  renWin->SetSize(640, 480);

  // The following reader is used to read a series of 2D slices (images)
  // that compose the volume. The slice dimensions are set, and the
  // pixel spacing. The data Endianness must also be specified. The
  // reader uses the FilePrefix in combination with the slice number to
  // construct filenames using the format FilePrefix.%d. (In this case
  // the FilePrefix is the root name of the file: quarter.)
  svtkSmartPointer<svtkVolume16Reader> v16 = svtkSmartPointer<svtkVolume16Reader>::New();
  v16->SetDataDimensions(64, 64);
  v16->SetImageRange(1, 93);
  v16->SetDataByteOrderToLittleEndian();
  v16->SetFilePrefix(argv[1]);
  v16->SetDataSpacing(3.2, 3.2, 1.5);
  v16->Update();

  // An isosurface, or contour value of 500 is known to correspond to
  // the skin of the patient. Once generated, a svtkPolyDataNormals
  // filter is used to create normals for smooth surface shading
  // during rendering.  The triangle stripper is used to create triangle
  // strips from the isosurface; these render much faster on may
  // systems.
  svtkSmartPointer<svtkContourFilter> skinExtractor = svtkSmartPointer<svtkContourFilter>::New();
  skinExtractor->SetInputConnection(v16->GetOutputPort());
  skinExtractor->SetValue(0, 500);
  skinExtractor->Update();

  svtkSmartPointer<svtkPolyDataNormals> skinNormals = svtkSmartPointer<svtkPolyDataNormals>::New();
  skinNormals->SetInputConnection(skinExtractor->GetOutputPort());
  skinNormals->SetFeatureAngle(60.0);
  skinNormals->Update();

  svtkSmartPointer<svtkStripper> skinStripper = svtkSmartPointer<svtkStripper>::New();
  skinStripper->SetInputConnection(skinNormals->GetOutputPort());
  skinStripper->Update();

  svtkSmartPointer<svtkPolyDataMapper> skinMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  skinMapper->SetInputConnection(skinStripper->GetOutputPort());
  skinMapper->ScalarVisibilityOff();

  svtkSmartPointer<svtkActor> skin = svtkSmartPointer<svtkActor>::New();
  skin->SetMapper(skinMapper);
  skin->GetProperty()->SetDiffuseColor(1, .49, .25);
  skin->GetProperty()->SetSpecular(.3);
  skin->GetProperty()->SetSpecularPower(20);

  // An isosurface, or contour value of 1150 is known to correspond to
  // the skin of the patient. Once generated, a svtkPolyDataNormals
  // filter is used to create normals for smooth surface shading
  // during rendering.  The triangle stripper is used to create triangle
  // strips from the isosurface; these render much faster on may
  // systems.
  svtkSmartPointer<svtkContourFilter> boneExtractor = svtkSmartPointer<svtkContourFilter>::New();
  boneExtractor->SetInputConnection(v16->GetOutputPort());
  boneExtractor->SetValue(0, 1150);

  svtkSmartPointer<svtkPolyDataNormals> boneNormals = svtkSmartPointer<svtkPolyDataNormals>::New();
  boneNormals->SetInputConnection(boneExtractor->GetOutputPort());
  boneNormals->SetFeatureAngle(60.0);

  svtkSmartPointer<svtkStripper> boneStripper = svtkSmartPointer<svtkStripper>::New();
  boneStripper->SetInputConnection(boneNormals->GetOutputPort());

  svtkSmartPointer<svtkPolyDataMapper> boneMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  boneMapper->SetInputConnection(boneStripper->GetOutputPort());
  boneMapper->ScalarVisibilityOff();

  svtkSmartPointer<svtkActor> bone = svtkSmartPointer<svtkActor>::New();
  bone->SetMapper(boneMapper);
  bone->GetProperty()->SetDiffuseColor(1, 1, .9412);

  // An outline provides context around the data.
  //
  svtkSmartPointer<svtkOutlineFilter> outlineData = svtkSmartPointer<svtkOutlineFilter>::New();
  outlineData->SetInputConnection(v16->GetOutputPort());
  outlineData->Update();

  svtkSmartPointer<svtkPolyDataMapper> mapOutline = svtkSmartPointer<svtkPolyDataMapper>::New();
  mapOutline->SetInputConnection(outlineData->GetOutputPort());

  svtkSmartPointer<svtkActor> outline = svtkSmartPointer<svtkActor>::New();
  outline->SetMapper(mapOutline);
  outline->GetProperty()->SetColor(0, 0, 0);

  // Now we are creating three orthogonal planes passing through the
  // volume. Each plane uses a different texture map and therefore has
  // different coloration.

  // Start by creating a black/white lookup table.
  svtkSmartPointer<svtkLookupTable> bwLut = svtkSmartPointer<svtkLookupTable>::New();
  bwLut->SetTableRange(0, 2000);
  bwLut->SetSaturationRange(0, 0);
  bwLut->SetHueRange(0, 0);
  bwLut->SetValueRange(0, 1);
  bwLut->Build(); // effective built

  // Now create a lookup table that consists of the full hue circle
  // (from HSV).
  svtkSmartPointer<svtkLookupTable> hueLut = svtkSmartPointer<svtkLookupTable>::New();
  hueLut->SetTableRange(0, 2000);
  hueLut->SetHueRange(0, 1);
  hueLut->SetSaturationRange(1, 1);
  hueLut->SetValueRange(1, 1);
  hueLut->Build(); // effective built

  // Finally, create a lookup table with a single hue but having a range
  // in the saturation of the hue.
  svtkSmartPointer<svtkLookupTable> satLut = svtkSmartPointer<svtkLookupTable>::New();
  satLut->SetTableRange(0, 2000);
  satLut->SetHueRange(.6, .6);
  satLut->SetSaturationRange(0, 1);
  satLut->SetValueRange(1, 1);
  satLut->Build(); // effective built

  // Create the first of the three planes. The filter svtkImageMapToColors
  // maps the data through the corresponding lookup table created above.  The
  // svtkImageActor is a type of svtkProp and conveniently displays an image on
  // a single quadrilateral plane. It does this using texture mapping and as
  // a result is quite fast. (Note: the input image has to be unsigned char
  // values, which the svtkImageMapToColors produces.) Note also that by
  // specifying the DisplayExtent, the pipeline requests data of this extent
  // and the svtkImageMapToColors only processes a slice of data.
  svtkSmartPointer<svtkImageMapToColors> sagittalColors = svtkSmartPointer<svtkImageMapToColors>::New();
  sagittalColors->SetInputConnection(v16->GetOutputPort());
  sagittalColors->SetLookupTable(bwLut);
  sagittalColors->Update();

  svtkSmartPointer<svtkImageActor> sagittal = svtkSmartPointer<svtkImageActor>::New();
  sagittal->GetMapper()->SetInputConnection(sagittalColors->GetOutputPort());
  sagittal->SetDisplayExtent(32, 32, 0, 63, 0, 92);
  sagittal->ForceOpaqueOn();

  // Create the second (axial) plane of the three planes. We use the
  // same approach as before except that the extent differs.
  svtkSmartPointer<svtkImageMapToColors> axialColors = svtkSmartPointer<svtkImageMapToColors>::New();
  axialColors->SetInputConnection(v16->GetOutputPort());
  axialColors->SetLookupTable(hueLut);
  axialColors->Update();

  svtkSmartPointer<svtkImageActor> axial = svtkSmartPointer<svtkImageActor>::New();
  axial->GetMapper()->SetInputConnection(axialColors->GetOutputPort());
  axial->SetDisplayExtent(0, 63, 0, 63, 46, 46);
  axial->ForceOpaqueOn();

  // Create the third (coronal) plane of the three planes. We use
  // the same approach as before except that the extent differs.
  svtkSmartPointer<svtkImageMapToColors> coronalColors = svtkSmartPointer<svtkImageMapToColors>::New();
  coronalColors->SetInputConnection(v16->GetOutputPort());
  coronalColors->SetLookupTable(satLut);
  coronalColors->Update();

  svtkSmartPointer<svtkImageActor> coronal = svtkSmartPointer<svtkImageActor>::New();
  coronal->GetMapper()->SetInputConnection(coronalColors->GetOutputPort());
  coronal->SetDisplayExtent(0, 63, 32, 32, 0, 92);
  coronal->ForceOpaqueOn();

  // It is convenient to create an initial view of the data. The
  // FocalPoint and Position form a vector direction. Later on
  // (ResetCamera() method) this vector is used to position the camera
  // to look at the data in this direction.
  svtkSmartPointer<svtkCamera> aCamera = svtkSmartPointer<svtkCamera>::New();
  aCamera->SetViewUp(0, 0, -1);
  aCamera->SetPosition(0, 1, 0);
  aCamera->SetFocalPoint(0, 0, 0);
  aCamera->ComputeViewPlaneNormal();
  aCamera->Azimuth(30.0);
  aCamera->Elevation(30.0);

  // Actors are added to the renderer.
  aRenderer->AddActor(outline);
  aRenderer->AddActor(sagittal);
  aRenderer->AddActor(axial);
  aRenderer->AddActor(coronal);
  aRenderer->AddActor(skin);
  aRenderer->AddActor(bone);

  // Turn off bone for this example.
  bone->VisibilityOff();

  // Set skin to semi-transparent.
  skin->GetProperty()->SetOpacity(0.5);

  // An initial camera view is created.  The Dolly() method moves
  // the camera towards the FocalPoint, thereby enlarging the image.
  aRenderer->SetActiveCamera(aCamera);

  // Calling Render() directly on a svtkRenderer is strictly forbidden.
  // Only calling Render() on the svtkRenderWindow is a valid call.
  renWin->Render();

  aRenderer->ResetCamera();
  aCamera->Dolly(1.5);

  // Note that when camera movement occurs (as it does in the Dolly()
  // method), the clipping planes often need adjusting. Clipping planes
  // consist of two planes: near and far along the view direction. The
  // near plane clips out objects in front of the plane; the far plane
  // clips out objects behind the plane. This way only what is drawn
  // between the planes is actually rendered.
  aRenderer->ResetCameraClippingRange();

  // For testing, check if "-V" is used to provide a regression test image
  if (argc >= 4 && strcmp(argv[2], "-V") == 0)
  {
    renWin->Render();
    int retVal = svtkRegressionTestImage(renWin);

    if (retVal == svtkTesting::FAILED)
    {
      return EXIT_FAILURE;
    }
    else if (retVal != svtkTesting::DO_INTERACTOR)
    {
      return EXIT_SUCCESS;
    }
  }
  // interact with data
  iren->Initialize();
  iren->Start();

  return EXIT_SUCCESS;
}
