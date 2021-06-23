/*=========================================================================

  Program:   Visualization Toolkit
  Module:    ImagePlaneWidget.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRegressionTestImage.h"
#include "svtkSmartPointer.h"

#include "svtkActor.h"
#include "svtkBiDimensionalWidget.h"
#include "svtkCamera.h"
#include "svtkCellPicker.h"
#include "svtkCommand.h"
#include "svtkDICOMImageReader.h"
#include "svtkImageActor.h"
#include "svtkImageData.h"
#include "svtkImageMapToColors.h"
#include "svtkImagePlaneWidget.h"
#include "svtkImageReader.h"
#include "svtkImageReslice.h"
#include "svtkInteractorEventRecorder.h"
#include "svtkInteractorStyleImage.h"
#include "svtkLookupTable.h"
#include "svtkOutlineFilter.h"
#include "svtkPlane.h"
#include "svtkPlaneSource.h"
#include "svtkPointData.h"
#include "svtkPolyDataMapper.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkResliceCursor.h"
#include "svtkResliceCursorActor.h"
#include "svtkResliceCursorLineRepresentation.h"
#include "svtkResliceCursorPolyDataAlgorithm.h"
#include "svtkResliceCursorWidget.h"
#include "svtkVolume16Reader.h"

#include "svtkTestUtilities.h"

//----------------------------------------------------------------------------
class svtkResliceCursorCallback : public svtkCommand
{
public:
  static svtkResliceCursorCallback* New() { return new svtkResliceCursorCallback; }

  void Execute(svtkObject* caller, unsigned long /*ev*/, void* callData) override
  {
    svtkImagePlaneWidget* ipw = dynamic_cast<svtkImagePlaneWidget*>(caller);
    if (ipw)
    {
      double* wl = static_cast<double*>(callData);

      if (ipw == this->IPW[0])
      {
        this->IPW[1]->SetWindowLevel(wl[0], wl[1], 1);
        this->IPW[2]->SetWindowLevel(wl[0], wl[1], 1);
      }
      else if (ipw == this->IPW[1])
      {
        this->IPW[0]->SetWindowLevel(wl[0], wl[1], 1);
        this->IPW[2]->SetWindowLevel(wl[0], wl[1], 1);
      }
      else if (ipw == this->IPW[2])
      {
        this->IPW[0]->SetWindowLevel(wl[0], wl[1], 1);
        this->IPW[1]->SetWindowLevel(wl[0], wl[1], 1);
      }
    }

    svtkResliceCursorWidget* rcw = dynamic_cast<svtkResliceCursorWidget*>(caller);
    if (rcw)
    {
      svtkResliceCursorLineRepresentation* rep =
        dynamic_cast<svtkResliceCursorLineRepresentation*>(rcw->GetRepresentation());
      svtkResliceCursor* rc = rep->GetResliceCursorActor()->GetCursorAlgorithm()->GetResliceCursor();
      for (int i = 0; i < 3; i++)
      {
        svtkPlaneSource* ps = static_cast<svtkPlaneSource*>(this->IPW[i]->GetPolyDataAlgorithm());
        ps->SetNormal(rc->GetPlane(i)->GetNormal());
        ps->SetCenter(rc->GetPlane(i)->GetOrigin());

        // If the reslice plane has modified, update it on the 3D widget
        this->IPW[i]->UpdatePlacement();

        // std::cout << "Updating placement of plane: " << i << " " <<
        //  rc->GetPlane(i)->GetNormal()[0] << " " <<
        //  rc->GetPlane(i)->GetNormal()[1] << " " <<
        //  rc->GetPlane(i)->GetNormal()[2] << std::endl;
        // this->IPW[i]->GetReslice()->Print(cout);
        // rep->GetReslice()->Print(cout);
        // std::cout << "---------------------" << std::endl;
      }
    }

    // Render everything
    this->RCW[0]->Render();
  }

  svtkResliceCursorCallback() = default;
  svtkImagePlaneWidget* IPW[3];
  svtkResliceCursorWidget* RCW[3];
};

//----------------------------------------------------------------------------
int TestResliceCursorWidget2(int argc, char* argv[])
{
  char* fname = svtkTestUtilities::ExpandDataFileName(argc, argv, "Data/headsq/quarter");

  svtkSmartPointer<svtkVolume16Reader> reader = svtkSmartPointer<svtkVolume16Reader>::New();
  reader->SetDataDimensions(64, 64);
  reader->SetDataByteOrderToLittleEndian();
  reader->SetImageRange(1, 93);
  reader->SetDataSpacing(3.2, 3.2, 1.5);
  reader->SetFilePrefix(fname);
  reader->ReleaseDataFlagOn();
  reader->SetDataMask(0x7fff);
  reader->Update();
  delete[] fname;

  svtkSmartPointer<svtkOutlineFilter> outline = svtkSmartPointer<svtkOutlineFilter>::New();
  outline->SetInputConnection(reader->GetOutputPort());

  svtkSmartPointer<svtkPolyDataMapper> outlineMapper = svtkSmartPointer<svtkPolyDataMapper>::New();
  outlineMapper->SetInputConnection(outline->GetOutputPort());

  svtkSmartPointer<svtkActor> outlineActor = svtkSmartPointer<svtkActor>::New();
  outlineActor->SetMapper(outlineMapper);

  svtkSmartPointer<svtkRenderer> ren[4];

  svtkSmartPointer<svtkRenderWindow> renWin = svtkSmartPointer<svtkRenderWindow>::New();
  renWin->SetMultiSamples(0);

  for (int i = 0; i < 4; i++)
  {
    ren[i] = svtkSmartPointer<svtkRenderer>::New();
    renWin->AddRenderer(ren[i]);
  }

  svtkSmartPointer<svtkRenderWindowInteractor> iren =
    svtkSmartPointer<svtkRenderWindowInteractor>::New();
  iren->SetRenderWindow(renWin);

  svtkSmartPointer<svtkCellPicker> picker = svtkSmartPointer<svtkCellPicker>::New();
  picker->SetTolerance(0.005);

  svtkSmartPointer<svtkProperty> ipwProp = svtkSmartPointer<svtkProperty>::New();

  // assign default props to the ipw's texture plane actor
  svtkSmartPointer<svtkImagePlaneWidget> planeWidget[3];
  int imageDims[3];
  reader->GetOutput()->GetDimensions(imageDims);

  for (int i = 0; i < 3; i++)
  {
    planeWidget[i] = svtkSmartPointer<svtkImagePlaneWidget>::New();
    planeWidget[i]->SetInteractor(iren);
    planeWidget[i]->SetPicker(picker);
    planeWidget[i]->RestrictPlaneToVolumeOn();
    double color[3] = { 0, 0, 0 };
    color[i] = 1;
    planeWidget[i]->GetPlaneProperty()->SetColor(color);
    planeWidget[i]->SetTexturePlaneProperty(ipwProp);
    planeWidget[i]->TextureInterpolateOff();
    planeWidget[i]->SetResliceInterpolateToLinear();
    planeWidget[i]->SetInputConnection(reader->GetOutputPort());
    planeWidget[i]->SetPlaneOrientation(i);
    planeWidget[i]->SetSliceIndex(imageDims[i] / 2);
    planeWidget[i]->DisplayTextOn();
    planeWidget[i]->SetDefaultRenderer(ren[3]);
    planeWidget[i]->SetWindowLevel(1358, -27);
    planeWidget[i]->On();
    planeWidget[i]->InteractionOn();
  }

  planeWidget[1]->SetLookupTable(planeWidget[0]->GetLookupTable());
  planeWidget[2]->SetLookupTable(planeWidget[0]->GetLookupTable());

  svtkSmartPointer<svtkResliceCursorCallback> cbk = svtkSmartPointer<svtkResliceCursorCallback>::New();

  // Create the reslice cursor, widget and rep

  svtkSmartPointer<svtkResliceCursor> resliceCursor = svtkSmartPointer<svtkResliceCursor>::New();
  resliceCursor->SetCenter(reader->GetOutput()->GetCenter());
  resliceCursor->SetThickMode(0);
  resliceCursor->SetThickness(10, 10, 10);
  resliceCursor->SetImage(reader->GetOutput());

  svtkSmartPointer<svtkResliceCursorWidget> resliceCursorWidget[3];
  svtkSmartPointer<svtkResliceCursorLineRepresentation> resliceCursorRep[3];

  double viewUp[3][3] = { { 0, 0, -1 }, { 0, 0, 1 }, { 0, 1, 0 } };
  for (int i = 0; i < 3; i++)
  {
    resliceCursorWidget[i] = svtkSmartPointer<svtkResliceCursorWidget>::New();
    resliceCursorWidget[i]->SetInteractor(iren);

    resliceCursorRep[i] = svtkSmartPointer<svtkResliceCursorLineRepresentation>::New();
    resliceCursorWidget[i]->SetRepresentation(resliceCursorRep[i]);
    resliceCursorRep[i]->GetResliceCursorActor()->GetCursorAlgorithm()->SetResliceCursor(
      resliceCursor);
    resliceCursorRep[i]->GetResliceCursorActor()->GetCursorAlgorithm()->SetReslicePlaneNormal(i);

    const double minVal = reader->GetOutput()->GetScalarRange()[0];
    if (svtkImageReslice* reslice = svtkImageReslice::SafeDownCast(resliceCursorRep[i]->GetReslice()))
    {
      reslice->SetBackgroundColor(minVal, minVal, minVal, minVal);
    }

    resliceCursorWidget[i]->SetDefaultRenderer(ren[i]);
    resliceCursorWidget[i]->SetEnabled(1);

    ren[i]->GetActiveCamera()->SetFocalPoint(0, 0, 0);
    double camPos[3] = { 0, 0, 0 };
    camPos[i] = 1;
    ren[i]->GetActiveCamera()->SetPosition(camPos);

    ren[i]->GetActiveCamera()->ParallelProjectionOn();
    ren[i]->GetActiveCamera()->SetViewUp(viewUp[i][0], viewUp[i][1], viewUp[i][2]);
    ren[i]->ResetCamera();
    // ren[i]->ResetCameraClippingRange();

    // Tie the Image plane widget and the reslice cursor widget together
    cbk->IPW[i] = planeWidget[i];
    cbk->RCW[i] = resliceCursorWidget[i];
    resliceCursorWidget[i]->AddObserver(svtkResliceCursorWidget::ResliceAxesChangedEvent, cbk);

    // Initialize the window level to a sensible value
    double range[2];
    reader->GetOutput()->GetScalarRange(range);
    resliceCursorRep[i]->SetWindowLevel(range[1] - range[0], (range[0] + range[1]) / 2.0);
    planeWidget[i]->SetWindowLevel(range[1] - range[0], (range[0] + range[1]) / 2.0);

    // Make them all share the same color map.
    resliceCursorRep[i]->SetLookupTable(resliceCursorRep[0]->GetLookupTable());
    planeWidget[i]->GetColorMap()->SetLookupTable(resliceCursorRep[0]->GetLookupTable());
  }

  // Add the actors
  //
  ren[0]->SetBackground(0.3, 0.1, 0.1);
  ren[1]->SetBackground(0.1, 0.3, 0.1);
  ren[2]->SetBackground(0.1, 0.1, 0.3);
  ren[3]->AddActor(outlineActor);
  ren[3]->SetBackground(0.1, 0.1, 0.1);
  renWin->SetSize(600, 600);
  // renWin->SetFullScreen(1);

  ren[0]->SetViewport(0, 0, 0.5, 0.5);
  ren[1]->SetViewport(0.5, 0, 1, 0.5);
  ren[2]->SetViewport(0, 0.5, 0.5, 1);
  ren[3]->SetViewport(0.5, 0.5, 1, 1);

  // Set the actors' positions
  //
  renWin->Render();

  ren[3]->GetActiveCamera()->Elevation(110);
  ren[3]->GetActiveCamera()->SetViewUp(0, 0, -1);
  ren[3]->GetActiveCamera()->Azimuth(45);
  ren[3]->GetActiveCamera()->Dolly(1.15);
  ren[3]->ResetCameraClippingRange();

  svtkSmartPointer<svtkInteractorStyleImage> style = svtkSmartPointer<svtkInteractorStyleImage>::New();
  iren->SetInteractorStyle(style);

  iren->Initialize();

  int retVal = svtkRegressionTestImage(renWin);
  if (retVal == svtkRegressionTester::DO_INTERACTOR)
  {
    iren->Start();
  }

  return !retVal;
}
