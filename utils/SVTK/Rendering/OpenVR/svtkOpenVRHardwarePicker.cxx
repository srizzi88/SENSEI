/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkOpenVRHardwarePicker.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenVRHardwarePicker.h"

#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkDataObject.h"
#include "svtkHardwareSelector.h"
#include "svtkObjectFactory.h"
#include "svtkOpenVRRenderWindow.h"
#include "svtkOpenVRRenderWindowInteractor.h"
#include "svtkRenderer.h"
#include "svtkSelection.h"
#include "svtkTransform.h"

svtkStandardNewMacro(svtkOpenVRHardwarePicker);

svtkOpenVRHardwarePicker::svtkOpenVRHardwarePicker()
{
  this->Selection = nullptr;
}

svtkOpenVRHardwarePicker::~svtkOpenVRHardwarePicker()
{
  if (this->Selection)
  {
    this->Selection->Delete();
  }
}

// set up for a pick
void svtkOpenVRHardwarePicker::Initialize()
{
  this->svtkAbstractPropPicker::Initialize();
}

// Pick from the given collection
int svtkOpenVRHardwarePicker::PickProp(
  double p0[3], double wxyz[4], svtkRenderer* renderer, svtkPropCollection*, bool actorPassOnly)
{
  //  Initialize picking process
  this->Initialize();
  this->Renderer = renderer;

  // Invoke start pick method if defined
  this->InvokeEvent(svtkCommand::StartPickEvent, nullptr);

  svtkOpenVRRenderWindow* renWin = svtkOpenVRRenderWindow::SafeDownCast(renderer->GetRenderWindow());
  if (!renWin)
  {
    return 0;
  }

  svtkNew<svtkHardwareSelector> sel;
  sel->SetFieldAssociation(svtkDataObject::FIELD_ASSOCIATION_CELLS);
  sel->SetRenderer(renderer);
  sel->SetActorPassOnly(actorPassOnly);
  svtkCamera* oldcam = renderer->GetActiveCamera();
  renWin->SetTrackHMD(false);

  svtkNew<svtkTransform> tran;
  tran->RotateWXYZ(wxyz[0], wxyz[1], wxyz[2], wxyz[3]);
  double pin[4] = { 0.0, 0.0, -1.0, 1.0 };
  double dop[4];
  tran->MultiplyPoint(pin, dop);
  double distance = oldcam->GetDistance();
  oldcam->SetPosition(p0);
  oldcam->SetFocalPoint(
    p0[0] + dop[0] * distance, p0[1] + dop[1] * distance, p0[2] + dop[2] * distance);
  oldcam->OrthogonalizeViewUp();

  const int* size = renderer->GetSize();

  sel->SetArea(size[0] / 2 - 5, size[1] / 2 - 5, size[0] / 2 + 5, size[1] / 2 + 5);

  if (this->Selection)
  {
    this->Selection->Delete();
  }

  this->Selection = nullptr;
  if (sel->CaptureBuffers())
  {
    unsigned int outPos[2];
    unsigned int inPos[2] = { static_cast<unsigned int>(size[0] / 2),
      static_cast<unsigned int>(size[1] / 2) };
    // find the data closest to the center
    svtkHardwareSelector::PixelInformation pinfo = sel->GetPixelInformation(inPos, 5, outPos);
    if (pinfo.Valid)
    {
      this->Selection = sel->GenerateSelection(outPos[0], outPos[1], outPos[0], outPos[1]);
    }
  }

  // this->Selection = sel->Select();
  // sel->SetArea(0, 0, size[0]-1, size[1]-1);

  renWin->SetTrackHMD(true);

  this->InvokeEvent(svtkCommand::EndPickEvent, this->Selection);

  if (this->Selection && this->Selection->GetNode(0))
  {
    return 1;
  }
  else
  {
    return 0;
  }
}

void svtkOpenVRHardwarePicker::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
