/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImageSlicing.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
// This example shows how to load a 3D image into SVTK and then reformat
// that image into a different orientation for viewing.  It uses
// svtkImageReslice for reformatting the image, and uses svtkImageActor
// and svtkInteractorStyleImage to display the image.  This InteractorStyle
// forces the camera to stay perpendicular to the XY plane.
//
// Thanks to David Gobbi of Atamai Inc. for contributing this example.
//

#include "svtkCommand.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMapToColors.h"
#include "svtkImageMapper3D.h"
#include "svtkImageReader2.h"
#include "svtkImageReslice.h"
#include "svtkInformation.h"
#include "svtkInteractorStyleImage.h"
#include "svtkLookupTable.h"
#include "svtkMatrix4x4.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"
#include "svtkStreamingDemandDrivenPipeline.h"

// The mouse motion callback, to turn "Slicing" on and off
class svtkImageInteractionCallback : public svtkCommand
{
public:
  static svtkImageInteractionCallback* New() { return new svtkImageInteractionCallback; }

  svtkImageInteractionCallback()
  {
    this->Slicing = 0;
    this->ImageReslice = nullptr;
    this->Interactor = nullptr;
  }

  void SetImageReslice(svtkImageReslice* reslice) { this->ImageReslice = reslice; }

  svtkImageReslice* GetImageReslice() { return this->ImageReslice; }

  void SetInteractor(svtkRenderWindowInteractor* interactor) { this->Interactor = interactor; }

  svtkRenderWindowInteractor* GetInteractor() { return this->Interactor; }

  void Execute(svtkObject*, unsigned long event, void*) override
  {
    svtkRenderWindowInteractor* interactor = this->GetInteractor();

    int lastPos[2];
    interactor->GetLastEventPosition(lastPos);
    int currPos[2];
    interactor->GetEventPosition(currPos);

    if (event == svtkCommand::LeftButtonPressEvent)
    {
      this->Slicing = 1;
    }
    else if (event == svtkCommand::LeftButtonReleaseEvent)
    {
      this->Slicing = 0;
    }
    else if (event == svtkCommand::MouseMoveEvent)
    {
      if (this->Slicing)
      {
        svtkImageReslice* reslice = this->ImageReslice;

        // Increment slice position by deltaY of mouse
        int deltaY = lastPos[1] - currPos[1];

        reslice->Update();
        double sliceSpacing = reslice->GetOutput()->GetSpacing()[2];
        svtkMatrix4x4* matrix = reslice->GetResliceAxes();
        // move the center point that we are slicing through
        double point[4];
        double center[4];
        point[0] = 0.0;
        point[1] = 0.0;
        point[2] = sliceSpacing * deltaY;
        point[3] = 1.0;
        matrix->MultiplyPoint(point, center);
        matrix->SetElement(0, 3, center[0]);
        matrix->SetElement(1, 3, center[1]);
        matrix->SetElement(2, 3, center[2]);
        interactor->Render();
      }
      else
      {
        svtkInteractorStyle* style =
          svtkInteractorStyle::SafeDownCast(interactor->GetInteractorStyle());
        if (style)
        {
          style->OnMouseMove();
        }
      }
    }
  }

private:
  // Actions (slicing only, for now)
  int Slicing;

  // Pointer to svtkImageReslice
  svtkImageReslice* ImageReslice;

  // Pointer to the interactor
  svtkRenderWindowInteractor* Interactor;
};

// The program entry point
int main(int argc, char* argv[])
{
  if (argc < 2)
  {
    cout << "Usage: " << argv[0] << " DATADIR/headsq/quarter" << endl;
    return 1;
  }

  // Start by loading some data.
  svtkSmartPointer<svtkImageReader2> reader = svtkSmartPointer<svtkImageReader2>::New();
  reader->SetFilePrefix(argv[1]);
  reader->SetDataExtent(0, 63, 0, 63, 1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetDataOrigin(0.0, 0.0, 0.0);
  reader->SetDataScalarTypeToUnsignedShort();
  reader->SetDataByteOrderToLittleEndian();
  reader->UpdateWholeExtent();

  // Calculate the center of the volume
  reader->Update();
  int extent[6];
  double spacing[3];
  double origin[3];

  reader->GetOutputInformation(0)->Get(svtkStreamingDemandDrivenPipeline::WHOLE_EXTENT(), extent);
  reader->GetOutput()->GetSpacing(spacing);
  reader->GetOutput()->GetOrigin(origin);

  double center[3];
  center[0] = origin[0] + spacing[0] * 0.5 * (extent[0] + extent[1]);
  center[1] = origin[1] + spacing[1] * 0.5 * (extent[2] + extent[3]);
  center[2] = origin[2] + spacing[2] * 0.5 * (extent[4] + extent[5]);

  // Matrices for axial, coronal, sagittal, oblique view orientations
  // static double axialElements[16] = {
  //         1, 0, 0, 0,
  //         0, 1, 0, 0,
  //         0, 0, 1, 0,
  //         0, 0, 0, 1 };

  // static double coronalElements[16] = {
  //         1, 0, 0, 0,
  //         0, 0, 1, 0,
  //         0,-1, 0, 0,
  //         0, 0, 0, 1 };

  static double sagittalElements[16] = {
    0, 0, -1, 0, //
    1, 0, 0, 0,  //
    0, -1, 0, 0, //
    0, 0, 0, 1   //
  };

  // static double obliqueElements[16] = {
  //         1, 0, 0, 0,
  //         0, 0.866025, -0.5, 0,
  //         0, 0.5, 0.866025, 0,
  //         0, 0, 0, 1 };

  // Set the slice orientation
  svtkSmartPointer<svtkMatrix4x4> resliceAxes = svtkSmartPointer<svtkMatrix4x4>::New();
  resliceAxes->DeepCopy(sagittalElements);
  // Set the point through which to slice
  resliceAxes->SetElement(0, 3, center[0]);
  resliceAxes->SetElement(1, 3, center[1]);
  resliceAxes->SetElement(2, 3, center[2]);

  // Extract a slice in the desired orientation
  svtkSmartPointer<svtkImageReslice> reslice = svtkSmartPointer<svtkImageReslice>::New();
  reslice->SetInputConnection(reader->GetOutputPort());
  reslice->SetOutputDimensionality(2);
  reslice->SetResliceAxes(resliceAxes);
  reslice->SetInterpolationModeToLinear();

  // Create a greyscale lookup table
  svtkSmartPointer<svtkLookupTable> table = svtkSmartPointer<svtkLookupTable>::New();
  table->SetRange(0, 2000);            // image intensity range
  table->SetValueRange(0.0, 1.0);      // from black to white
  table->SetSaturationRange(0.0, 0.0); // no color saturation
  table->SetRampToLinear();
  table->Build();

  // Map the image through the lookup table
  svtkSmartPointer<svtkImageMapToColors> color = svtkSmartPointer<svtkImageMapToColors>::New();
  color->SetLookupTable(table);
  color->SetInputConnection(reslice->GetOutputPort());

  // Display the image
  svtkSmartPointer<svtkImageActor> actor = svtkSmartPointer<svtkImageActor>::New();
  actor->GetMapper()->SetInputConnection(color->GetOutputPort());

  svtkSmartPointer<svtkRenderer> renderer = svtkSmartPointer<svtkRenderer>::New();
  renderer->AddActor(actor);

  svtkSmartPointer<svtkRenderWindow> window = svtkSmartPointer<svtkRenderWindow>::New();
  window->AddRenderer(renderer);

  // Set up the interaction
  svtkSmartPointer<svtkInteractorStyleImage> imageStyle =
    svtkSmartPointer<svtkInteractorStyleImage>::New();
  svtkSmartPointer<svtkRenderWindowInteractor> interactor =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  interactor->SetInteractorStyle(imageStyle);
  window->SetInteractor(interactor);
  window->Render();

  svtkSmartPointer<svtkImageInteractionCallback> callback =
    svtkSmartPointer<svtkImageInteractionCallback>::New();
  callback->SetImageReslice(reslice);
  callback->SetInteractor(interactor);

  imageStyle->AddObserver(svtkCommand::MouseMoveEvent, callback);
  imageStyle->AddObserver(svtkCommand::LeftButtonPressEvent, callback);
  imageStyle->AddObserver(svtkCommand::LeftButtonReleaseEvent, callback);

  // Start interaction
  // The Start() method doesn't return until the window is closed by the user
  interactor->Start();

  return EXIT_SUCCESS;
}
