/*=========================================================================
Program:   Visualization Toolkit
Module:    TestGPURayCastClippingUserTransform.cxx
Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.
This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.
=========================================================================*/

// Description
// This test creates a svtkImageData with two components.
// The data is volume rendered considering the two components as independent.
#include <fstream>
#include <iostream>
using namespace std;

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkColorTransferFunction.h"
#include "svtkCommand.h"
#include "svtkGPUVolumeRayCastMapper.h"
#include "svtkImageData.h"
#include "svtkImageReader2.h"
#include "svtkInteractorStyleImage.h"
#include "svtkInteractorStyleTrackballCamera.h"
#include "svtkMatrix4x4.h"
#include "svtkNew.h"
#include "svtkOutlineFilter.h"
#include "svtkPiecewiseFunction.h"
#include "svtkPlane.h"
#include "svtkPlaneCollection.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkUnsignedShortArray.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"
#include "svtksys/FStream.hxx"

#include "svtkRegressionTestImage.h"
#include "svtkTestUtilities.h"

static double* ComputeNormal(double* reference, bool flipSign)
{
  double* normal = new double[3];
  if (flipSign)
  {
    normal[0] = -reference[0];
    normal[1] = -reference[1];
    normal[2] = -reference[2];
  }
  else
  {
    normal[0] = reference[0];
    normal[1] = reference[1];
    normal[2] = reference[2];
  }

  return normal;
}

static double* ComputeOrigin(double* focalPoint, double* reference, double distance, bool flipSign)
{

  double* origin = new double[3];
  if (flipSign)
  {
    origin[0] = focalPoint[0] - distance * reference[0];
    origin[1] = focalPoint[1] - distance * reference[1];
    origin[2] = focalPoint[2] - distance * reference[2];
  }
  else
  {
    origin[0] = focalPoint[0] + distance * reference[0];
    origin[1] = focalPoint[1] + distance * reference[1];
    origin[2] = focalPoint[2] + distance * reference[2];
  }

  return origin;
}

static void UpdateFrontClippingPlane(
  svtkPlane* frontClippingPlane, double* normal, double* focalPoint, double slabThickness)
{
  // The front plane is the start of ray cast
  // The front normal should be in the same direction as the camera
  // direction (opposite to the plane facing direction)
  double* frontNormal = ComputeNormal(normal, true);

  // Get the origin of the front clipping plane
  double halfSlabThickness = slabThickness / 2;
  double* frontOrigin = ComputeOrigin(focalPoint, normal, halfSlabThickness, false);

  // Set the normal and origin of the front clipping plane
  frontClippingPlane->SetNormal(frontNormal);
  frontClippingPlane->SetOrigin(frontOrigin);

  delete[] frontNormal;
  delete[] frontOrigin;
}

static void UpdateRearClippingPlane(
  svtkPlane* rearClippingPlane, double* normal, double* focalPoint, double slabThickness)
{

  // The rear normal is the end of ray cast
  // The rear normal should be in the opposite direction to the
  // camera direction (same as the plane facing direction)
  double* rearNormal = ComputeNormal(normal, false);

  // Get the origin of the rear clipping plane
  double halfSlabThickness = slabThickness / 2;
  double* rearOrigin = ComputeOrigin(focalPoint, normal, halfSlabThickness, true);

  // Set the normal and origin of the rear clipping plane
  rearClippingPlane->SetNormal(rearNormal);
  rearClippingPlane->SetOrigin(rearOrigin);

  delete[] rearNormal;
  delete[] rearOrigin;
}

class svtkInteractorStyleCallback : public svtkCommand
{
public:
  static svtkInteractorStyleCallback* New() { return new svtkInteractorStyleCallback; }

  void Execute(svtkObject* caller, unsigned long, void*) override
  {
    svtkInteractorStyle* style = reinterpret_cast<svtkInteractorStyle*>(caller);

    svtkCamera* camera = style->GetCurrentRenderer()->GetActiveCamera();
    // svtkCamera *camera = reinterpret_cast<svtkCamera*>(caller);

    // Get the normal and focal point of the camera
    double* normal = camera->GetViewPlaneNormal();
    double* focalPoint = camera->GetFocalPoint();

    // Fixed slab thickness
    slabThickness = 3.0;
    UpdateFrontClippingPlane(frontClippingPlane, normal, focalPoint, slabThickness);
    UpdateRearClippingPlane(rearClippingPlane, normal, focalPoint, slabThickness);
  }

  svtkInteractorStyleCallback() = default;

  void SetFrontClippingPlane(svtkPlane* fcPlane) { this->frontClippingPlane = fcPlane; }

  void SetRearClippingPlane(svtkPlane* rcPlane) { this->rearClippingPlane = rcPlane; }

  double slabThickness;
  svtkPlane* frontClippingPlane;
  svtkPlane* rearClippingPlane;
};

int TestGPURayCastClippingUserTransform(int argc, char* argv[])
{
  int width = 256;
  int height = 256;
  int depth = 148;
  double spacing[3] = { 1.4844, 1.4844, 1.2 };

  // Read the image
  streampos size;
  char* memblock;

  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/MagnitudeImage_256x256x148");

  svtksys::ifstream file(fname, ios::in | ios::binary | ios::ate);
  if (file.is_open())
  {
    size = file.tellg();
    memblock = new char[size];
    file.seekg(0, ios::beg);
    file.read(memblock, size);
    file.close();
  }
  else
  {
    cout << "Unable to open file";
    return 1;
  }

  // Convert to short
  unsigned short* shortData = new unsigned short[size / 2];
  int idx = 0;
  int idx2 = 0;
  for (idx = 0; idx < size / 2; idx++)
  {
    idx2 = idx * 2;
    shortData[idx] = (short)(((memblock[idx2] & 0xFF) << 8) | (memblock[idx2 + 1] & 0xFF));
  }

  //
  int volumeSizeInSlice = width * height * depth;
  svtkNew<svtkUnsignedShortArray> dataArrayMag;
  dataArrayMag->Allocate(volumeSizeInSlice, 0);
  dataArrayMag->SetNumberOfComponents(1);
  dataArrayMag->SetNumberOfTuples(volumeSizeInSlice);
  dataArrayMag->SetArray(shortData, volumeSizeInSlice, 1);

  svtkNew<svtkImageData> imageData;
  imageData->SetDimensions(width, height, depth);
  imageData->SetSpacing(spacing);
  imageData->GetPointData()->SetScalars(dataArrayMag);

  // Create a clipping plane
  svtkNew<svtkPlane> frontClippingPlane;
  svtkNew<svtkPlane> rearClippingPlane;

  // Create a clipping plane collection
  svtkNew<svtkPlaneCollection> clippingPlaneCollection;
  clippingPlaneCollection->AddItem(frontClippingPlane);
  clippingPlaneCollection->AddItem(rearClippingPlane);

  // Create a mapper
  svtkNew<svtkGPUVolumeRayCastMapper> volumeMapper;
  // volumeMapper->SetInputConnection(reader->GetOutputPort());
  volumeMapper->SetInputData(imageData);
  volumeMapper->SetBlendModeToMaximumIntensity();
  volumeMapper->AutoAdjustSampleDistancesOff();
  volumeMapper->SetSampleDistance(1.0);
  volumeMapper->SetImageSampleDistance(1.0);
  volumeMapper->SetClippingPlanes(clippingPlaneCollection);

  // Create volume scale opacity
  svtkNew<svtkPiecewiseFunction> volumeScalarOpacity;
  volumeScalarOpacity->AddPoint(0, 0.0);
  volumeScalarOpacity->AddPoint(32767, 1.0);
  volumeScalarOpacity->ClampingOn();

  // Create a property
  svtkNew<svtkVolumeProperty> volumeProperty;
  volumeProperty->SetInterpolationTypeToLinear();
  volumeProperty->ShadeOff();
  volumeProperty->SetAmbient(1.0);
  volumeProperty->SetDiffuse(0.0);
  volumeProperty->SetSpecular(0.0);
  volumeProperty->IndependentComponentsOn();
  volumeProperty->SetScalarOpacity(volumeScalarOpacity);
  volumeProperty->SetColor(volumeScalarOpacity);

  // Create a volume
  svtkNew<svtkVolume> volume;
  volume->SetMapper(volumeMapper);
  volume->SetProperty(volumeProperty);
  volume->PickableOff();

  // Rotate the blue props
  double rowVector[3] = { 0.0, 0.0, -1.0 };
  double columnVector[3] = { 1.0, 0.0, 0.0 };
  double normalVector[3];
  svtkMath::Cross(rowVector, columnVector, normalVector);
  double position[3] = { 0.0, 0.0, 0.0 };

  svtkSmartPointer<svtkMatrix4x4> matrix = svtkSmartPointer<svtkMatrix4x4>::New();
  matrix->Identity();
  matrix->SetElement(0, 0, rowVector[0]);
  matrix->SetElement(0, 1, rowVector[1]);
  matrix->SetElement(0, 2, rowVector[2]);
  matrix->SetElement(0, 3, position[0]);
  matrix->SetElement(1, 0, columnVector[0]);
  matrix->SetElement(1, 1, columnVector[1]);
  matrix->SetElement(1, 2, columnVector[2]);
  matrix->SetElement(1, 3, position[1]);
  matrix->SetElement(2, 0, normalVector[0]);
  matrix->SetElement(2, 1, normalVector[1]);
  matrix->SetElement(2, 2, normalVector[2]);
  matrix->SetElement(2, 3, position[2]);

  volume->SetUserMatrix(matrix);

  // Create a outline filter
  svtkNew<svtkOutlineFilter> outlineFilter;
  outlineFilter->SetInputData(imageData);

  // Create an outline mapper
  svtkNew<svtkPolyDataMapper> outlineMapper;
  outlineMapper->SetInputConnection(outlineFilter->GetOutputPort());

  svtkNew<svtkActor> outline;
  outline->SetMapper(outlineMapper);
  outline->PickableOff();

  // Create a renderer
  svtkNew<svtkRenderer> ren;
  ren->AddViewProp(volume);
  ren->AddViewProp(outline);

  // Get the center of volume
  double* center = volume->GetCenter();
  double cameraFocal[3];
  cameraFocal[0] = center[0];
  cameraFocal[1] = center[1];
  cameraFocal[2] = center[2];

  double cameraViewUp[3] = { 0.00, -1.00, 0.00 };
  double cameraNormal[3] = { 0.00, 0.00, -1.00 };
  double cameraDistance = 1000.0;

  double cameraPosition[3];
  cameraPosition[0] = cameraFocal[0] + cameraDistance * cameraNormal[0];
  cameraPosition[1] = cameraFocal[1] + cameraDistance * cameraNormal[1];
  cameraPosition[2] = cameraFocal[2] + cameraDistance * cameraNormal[2];

  // Update clipping planes
  UpdateFrontClippingPlane(frontClippingPlane, cameraNormal, cameraFocal, 3.0);
  UpdateRearClippingPlane(rearClippingPlane, cameraNormal, cameraFocal, 3.0);

  // Get the active camera
  svtkCamera* camera = ren->GetActiveCamera();
  camera->ParallelProjectionOn();
  camera->SetParallelScale(250);
  camera->SetPosition(cameraPosition);
  camera->SetFocalPoint(cameraFocal);
  camera->SetViewUp(cameraViewUp);

  // Create a render window
  svtkNew<svtkRenderWindow> renWin;
  renWin->SetSize(500, 500);
  renWin->AddRenderer(ren);

  // Create a style
  svtkNew<svtkInteractorStyleImage> style;
  style->SetInteractionModeToImage3D();

  // Create a interactor style callback
  svtkNew<svtkInteractorStyleCallback> interactorStyleCallback;
  interactorStyleCallback->frontClippingPlane = frontClippingPlane;
  interactorStyleCallback->rearClippingPlane = rearClippingPlane;
  style->AddObserver(svtkCommand::InteractionEvent, interactorStyleCallback);

  // Create an interactor
  svtkNew<svtkRenderWindowInteractor> iren;
  iren->SetInteractorStyle(style);
  iren->SetRenderWindow(renWin);

  // Start
  iren->Initialize();
  renWin->Render();

  int retVal = svtkRegressionTestImageThreshold(renWin, 70);

  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  delete[] memblock;
  delete[] shortData;
  delete[] fname;

  return !retVal;
}
