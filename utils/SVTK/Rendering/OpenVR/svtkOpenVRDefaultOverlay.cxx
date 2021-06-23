/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenVRDefaultOverlay.h"

#include "svtkCallbackCommand.h"
#include "svtkInteractorStyle3D.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpenVRCamera.h"
#include "svtkOpenVRRenderWindow.h"
#include "svtkOpenVRRenderWindowInteractor.h"
#include "svtkOpenVRRenderer.h"
#include "svtkRendererCollection.h"
#include "svtkTextureObject.h"

#include <cmath>
#include <sstream>

#include "svtkOpenVROverlayInternal.h"

svtkStandardNewMacro(svtkOpenVRDefaultOverlay);

void handleMotionFactor(svtkObject* caller, unsigned long eid, void* clientdata, void* calldata)
{
  if (eid == svtkCommand::LeftButtonReleaseEvent)
  {
    svtkOpenVRDefaultOverlay* ovl = static_cast<svtkOpenVRDefaultOverlay*>(caller);
    svtkOpenVRRenderWindow* win = static_cast<svtkOpenVRRenderWindow*>(calldata);
    svtkInteractorStyle3D* is =
      static_cast<svtkInteractorStyle3D*>(win->GetInteractor()->GetInteractorStyle());
    int option = *(reinterpret_cast<int*>(&clientdata));
    double mf = 0.1;
    switch (option)
    {
      case 0:
        mf = 0.0;
        break;
      case 1:
        mf = 0.1;
        break;
      case 2:
        mf = 1.0;
        break;
      case 3:
        mf = 10.0;
        break;
      case 4:
        mf = 100.0;
        break;
    }
    is->SetDollyPhysicalSpeed(mf);

    // turn off all motion spots
    std::vector<svtkOpenVROverlaySpot>& spots = ovl->GetSpots();
    std::vector<svtkOpenVROverlaySpot>::iterator it = spots.begin();
    for (; it != spots.end(); ++it)
    {
      if (it->Group == "motion" && it->Active)
      {
        it->Active = false;
        ovl->UpdateSpot(&(*it));
      }
    }
    // turn on this one
    ovl->GetLastSpot()->Active = true;
    ovl->Render();
  }
}

void handleScaleFactor(svtkObject* caller, unsigned long eid, void* clientdata, void* calldata)
{
  if (eid == svtkCommand::LeftButtonReleaseEvent)
  {
    svtkOpenVRDefaultOverlay* ovl = static_cast<svtkOpenVRDefaultOverlay*>(caller);
    svtkOpenVRRenderWindow* win = static_cast<svtkOpenVRRenderWindow*>(calldata);
    svtkInteractorStyle3D* is =
      static_cast<svtkInteractorStyle3D*>(win->GetInteractor()->GetInteractorStyle());
    int option = *(reinterpret_cast<int*>(&clientdata));
    double mf = 1.0;
    switch (option)
    {
      case 0:
        mf = 0.01;
        break;
      case 1:
        mf = 0.1;
        break;
      case 2:
        mf = 1.0;
        break;
      case 3:
        mf = 10.0;
        break;
      case 4:
        mf = 100.0;
        break;
    }
    svtkRenderer* ren = static_cast<svtkRenderer*>(win->GetRenderers()->GetItemAsObject(0));
    is->SetScale(ren->GetActiveCamera(), 1.0 / mf);
    ren->ResetCameraClippingRange();
    ovl->Render();
  }
}

void handleSaveCamera(svtkObject* caller, unsigned long eid, void* clientdata, void* /*calldata*/)
{
  if (eid == svtkCommand::LeftButtonReleaseEvent)
  {
    svtkOpenVRDefaultOverlay* ovl = static_cast<svtkOpenVRDefaultOverlay*>(caller);
    int option = *(reinterpret_cast<int*>(&clientdata));

    std::ostringstream s;
    s << "Really save the camera pose into slot " << option << " ?";
    if (vr::VROverlay()->ShowMessageOverlay(s.str().c_str(), "Confirmation", "Yes", "No", nullptr,
          nullptr) == vr::VRMessageOverlayResponse_ButtonPress_0)
    {
      ovl->SaveCameraPose(option);
    }
  }
}

void handleLoadCamera(svtkObject* caller, unsigned long eid, void* clientdata, void* /* calldata */)
{
  if (eid == svtkCommand::LeftButtonReleaseEvent)
  {
    svtkOpenVRDefaultOverlay* ovl = static_cast<svtkOpenVRDefaultOverlay*>(caller);
    //    ovl->ReadCameraPoses();
    int option = *(reinterpret_cast<int*>(&clientdata));
    ovl->LoadCameraPose(option);
  }
}

void handleShowFloor(svtkObject* caller, unsigned long eid, void* clientdata, void* calldata)
{
  if (eid == svtkCommand::LeftButtonReleaseEvent)
  {
    svtkOpenVRDefaultOverlay* ovl = static_cast<svtkOpenVRDefaultOverlay*>(caller);
    int option = *(reinterpret_cast<int*>(&clientdata));
    svtkOpenVRRenderWindow* win = static_cast<svtkOpenVRRenderWindow*>(calldata);
    svtkOpenVRRenderer* ren =
      static_cast<svtkOpenVRRenderer*>(win->GetRenderers()->GetItemAsObject(0));
    ren->SetShowFloor(option != 0);

    // turn off all floor spots
    std::vector<svtkOpenVROverlaySpot>& spots = ovl->GetSpots();
    std::vector<svtkOpenVROverlaySpot>::iterator it = spots.begin();
    for (; it != spots.end(); ++it)
    {
      if (it->Group == "floor" && it->Active)
      {
        it->Active = false;
        ovl->UpdateSpot(&(*it));
      }
    }
    // turn on this one
    ovl->GetLastSpot()->Active = true;
    ovl->Render();
  }
}

void handleSetViewUp(svtkObject* /* caller */, unsigned long eid, void* clientdata, void* calldata)
{
  if (eid == svtkCommand::LeftButtonReleaseEvent)
  {
    svtkOpenVRRenderWindow* win = static_cast<svtkOpenVRRenderWindow*>(calldata);
    int option = *(reinterpret_cast<int*>(&clientdata));
    switch (option)
    {
      case 0:
        win->SetPhysicalViewUp(-1, 0, 0);
        win->SetPhysicalViewDirection(0, 1, 0);
        break;
      case 1:
        win->SetPhysicalViewUp(1, 0, 0);
        win->SetPhysicalViewDirection(0, 1, 0);
        break;
      case 2:
        win->SetPhysicalViewUp(0, -1, 0);
        win->SetPhysicalViewDirection(0, 0, 1);
        break;
      case 3:
        win->SetPhysicalViewUp(0, 1, 0);
        win->SetPhysicalViewDirection(0, 0, 1);
        break;
      case 4:
        win->SetPhysicalViewUp(0, 0, -1);
        win->SetPhysicalViewDirection(0, 1, 0);
        break;
      case 5:
        win->SetPhysicalViewUp(0, 0, 1);
        win->SetPhysicalViewDirection(0, 1, 0);
        break;
    }
  }
}

svtkOpenVRDefaultOverlay::svtkOpenVRDefaultOverlay() {}

svtkOpenVRDefaultOverlay::~svtkOpenVRDefaultOverlay() {}

void svtkOpenVRDefaultOverlay::SetupSpots()
{
  // add default spots
  for (int i = 0; i < 6; i++)
  {
    svtkCallbackCommand* cc = svtkCallbackCommand::New();
    cc->SetClientData(reinterpret_cast<char*>(i));
    cc->SetCallback(handleSetViewUp);
    this->Spots.push_back(svtkOpenVROverlaySpot(913 + i * 91.5, 913 + i * 91.5 + 90, 522, 608, cc));
    cc->Delete();
  }
  for (int i = 0; i < 5; i++)
  {
    svtkCallbackCommand* cc = svtkCallbackCommand::New();
    cc->SetClientData(reinterpret_cast<char*>(i));
    cc->SetCallback(handleMotionFactor);
    svtkOpenVROverlaySpot spot(913 + i * 109.8, 913 + i * 109.8 + 108, 48, 134, cc);
    spot.Group = "motion";
    spot.GroupId = i;
    this->Spots.push_back(spot);
    cc->Delete();
  }
  for (int i = 0; i < 5; i++)
  {
    svtkCallbackCommand* cc = svtkCallbackCommand::New();
    cc->SetClientData(reinterpret_cast<char*>(i));
    cc->SetCallback(handleScaleFactor);
    svtkOpenVROverlaySpot spot(913 + i * 109.8, 913 + i * 109.8 + 108, 284, 370, cc);
    spot.Group = "scale";
    spot.GroupId = i;
    this->Spots.push_back(spot);
    cc->Delete();
  }

  for (int i = 0; i < 2; i++)
  {
    svtkCallbackCommand* cc = svtkCallbackCommand::New();
    cc->SetClientData(reinterpret_cast<char*>(i));
    cc->SetCallback(handleShowFloor);
    svtkOpenVROverlaySpot spot(600 + i * 136, 600 + i * 136 + 135, 530, 601, cc);
    spot.GroupId = i;
    spot.Group = "floor";
    this->Spots.push_back(spot);
    cc->Delete();
  }
  for (int i = 0; i < 8; i++)
  {
    svtkCallbackCommand* cc = svtkCallbackCommand::New();
    cc->SetClientData(reinterpret_cast<char*>(i + 1));
    cc->SetCallback(handleLoadCamera);
    this->Spots.push_back(svtkOpenVROverlaySpot(37 + i * 104.5, 37 + i * 104.5 + 103, 284, 370, cc));
    cc->Delete();
  }
  for (int i = 0; i < 8; i++)
  {
    svtkCallbackCommand* cc = svtkCallbackCommand::New();
    cc->SetClientData(reinterpret_cast<char*>(i + 1));
    cc->SetCallback(handleSaveCamera);
    this->Spots.push_back(svtkOpenVROverlaySpot(37 + i * 104.5, 37 + i * 104.5 + 103, 48, 134, cc));
    cc->Delete();
  }
}

void svtkOpenVRDefaultOverlay::Render()
{
  // update settings
  svtkOpenVRRenderer* ren =
    static_cast<svtkOpenVRRenderer*>(this->Window->GetRenderers()->GetItemAsObject(0));
  bool showFloor = ren->GetShowFloor();

  // set correct floor sports
  std::vector<svtkOpenVROverlaySpot>& spots = this->GetSpots();
  std::vector<svtkOpenVROverlaySpot>::iterator it = spots.begin();
  for (; it != spots.end(); ++it)
  {
    if (it->Group == "floor")
    {
      it->Active = ((it->GroupId == 1) == showFloor);
      this->UpdateSpot(&(*it));
    }
  }

  this->Superclass::Render();
}

void svtkOpenVRDefaultOverlay::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
