/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenVROverlay.h"

#include "svtkCallbackCommand.h"
#include "svtkDataArray.h"
#include "svtkImageData.h"
#include "svtkInteractorStyle3D.h"
#include "svtkJPEGReader.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpenVRCamera.h"
#include "svtkOpenVRRenderWindow.h"
#include "svtkOpenVRRenderWindowInteractor.h"
#include "svtkPointData.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"
#include "svtkTextureObject.h"
#include "svtkXMLDataElement.h"
#include "svtkXMLUtilities.h"
#include "svtksys/FStream.hxx"
#include "svtksys/SystemTools.hxx"

#include "svtkOpenVROverlayInternal.h"

#include "OpenVRDashboard.h"

#include <cmath>

svtkStandardNewMacro(svtkOpenVROverlay);

svtkOpenVROverlay::svtkOpenVROverlay()
{
  this->OverlayHandle = 0;
  this->OverlayThumbnailHandle = 0;
  this->OriginalTextureData = nullptr;
  this->CurrentTextureData = nullptr;
  this->LastSpot = nullptr;
  this->SessionName = "";
  this->VRSystem = nullptr;
  this->DashboardImageFileName = "OpenVRDashboard.jpg";
  this->LastCameraPoseIndex = -1;
  this->LastSpotIntensity = 0.3;
  this->ActiveSpotIntensity = 0.3;
}

svtkOpenVROverlay::~svtkOpenVROverlay()
{
  if (this->OriginalTextureData)
  {
    delete[] this->OriginalTextureData;
    this->OriginalTextureData = 0;
  }
  if (this->CurrentTextureData)
  {
    delete[] this->CurrentTextureData;
    this->CurrentTextureData = 0;
  }
}

svtkOpenVRCameraPose* svtkOpenVROverlay::GetSavedCameraPose(int i)
{
  auto p = this->SavedCameraPoses.find(i);
  if (p != this->SavedCameraPoses.end())
  {
    return &(p->second);
  }
  return nullptr;
}

void svtkOpenVROverlay::WriteCameraPoses(ostream& os)
{
  svtkNew<svtkXMLDataElement> topel;
  topel->SetName("CameraPoses");
  for (auto p : this->SavedCameraPoses)
  {
    svtkOpenVRCameraPose& pose = p.second;
    if (pose.Loaded)
    {
      svtkNew<svtkXMLDataElement> el;
      el->SetName("CameraPose");
      el->SetIntAttribute("PoseNumber", p.first);
      el->SetVectorAttribute("Position", 3, pose.Position);
      el->SetDoubleAttribute("Distance", pose.Distance);
      el->SetDoubleAttribute("MotionFactor", pose.MotionFactor);
      el->SetVectorAttribute("Translation", 3, pose.Translation);
      el->SetVectorAttribute("InitialViewUp", 3, pose.PhysicalViewUp);
      el->SetVectorAttribute("InitialViewDirection", 3, pose.PhysicalViewDirection);
      el->SetVectorAttribute("ViewDirection", 3, pose.ViewDirection);
      topel->AddNestedElement(el);
    }
  }

  svtkXMLUtilities::FlattenElement(topel, os);
}

void svtkOpenVROverlay::WriteCameraPoses()
{
  std::string fname = this->GetSessionName();
  fname += "SVTKOpenVRCameraPoses.vovrcp";
  svtksys::ofstream os(fname.c_str(), ios::out);
  this->WriteCameraPoses(os);

  os.flush();
  if (os.fail())
  {
    os.close();
    unlink(fname.c_str());
  }
}

void svtkOpenVROverlay::ReadCameraPoses()
{
  std::string fname = this->GetSessionName();
  fname += "SVTKOpenVRCameraPoses.vovrcp";

  if (!svtksys::SystemTools::FileExists(fname.c_str()))
  {
    return;
  }

  svtksys::ifstream is(fname.c_str());
  this->ReadCameraPoses(is);
}

void svtkOpenVROverlay::ReadCameraPoses(istream& is)
{
  svtkXMLDataElement* topel = svtkXMLUtilities::ReadElementFromStream(is);

  this->ReadCameraPoses(topel);
  topel->Delete();
}

void svtkOpenVROverlay::ReadCameraPoses(svtkXMLDataElement* topel)
{
  this->SavedCameraPoses.clear();
  if (topel)
  {
    int numPoses = topel->GetNumberOfNestedElements();
    for (size_t i = 0; i < numPoses; i++)
    {
      svtkXMLDataElement* el = topel->GetNestedElement(static_cast<int>(i));
      int poseNum = 0;
      el->GetScalarAttribute("PoseNumber", poseNum);
      el->GetVectorAttribute("Position", 3, this->SavedCameraPoses[poseNum].Position);
      el->GetVectorAttribute("InitialViewUp", 3, this->SavedCameraPoses[poseNum].PhysicalViewUp);
      el->GetVectorAttribute(
        "InitialViewDirection", 3, this->SavedCameraPoses[poseNum].PhysicalViewDirection);
      el->GetVectorAttribute("ViewDirection", 3, this->SavedCameraPoses[poseNum].ViewDirection);
      el->GetVectorAttribute("Translation", 3, this->SavedCameraPoses[poseNum].Translation);
      el->GetScalarAttribute("Distance", this->SavedCameraPoses[poseNum].Distance);
      el->GetScalarAttribute("MotionFactor", this->SavedCameraPoses[poseNum].MotionFactor);
      this->SavedCameraPoses[poseNum].Loaded = true;
    }
  }
}

void svtkOpenVROverlay::SetSavedCameraPose(int i, svtkOpenVRCameraPose* pose)
{
  if (pose)
  {
    this->SavedCameraPoses[i] = *pose;
  }
}

void svtkOpenVROverlay::SaveCameraPose(int slot)
{
  svtkOpenVRCameraPose* pose = &this->SavedCameraPoses[slot];
  svtkRenderer* ren = static_cast<svtkRenderer*>(this->Window->GetRenderers()->GetItemAsObject(0));
  pose->Set(static_cast<svtkOpenVRCamera*>(ren->GetActiveCamera()), this->Window);
  this->InvokeEvent(svtkCommand::SaveStateEvent, reinterpret_cast<void*>(slot));
}

void svtkOpenVROverlay::LoadCameraPose(int slot)
{
  svtkOpenVRCameraPose* pose = this->GetSavedCameraPose(slot);
  if (pose && pose->Loaded)
  {
    this->LastCameraPoseIndex = slot;
    svtkRenderer* ren = static_cast<svtkRenderer*>(this->Window->GetRenderers()->GetItemAsObject(0));
    pose->Apply(static_cast<svtkOpenVRCamera*>(ren->GetActiveCamera()), this->Window);
    ren->ResetCameraClippingRange();
    this->InvokeEvent(svtkCommand::LoadStateEvent, reinterpret_cast<void*>(slot));
  }
}

void svtkOpenVROverlay::LoadNextCameraPose()
{
  if (this->SavedCameraPoses.size() == 0)
  {
    return;
  }

  int nextValue = -1;
  int firstValue = this->LastCameraPoseIndex;
  // find the next pose index in the map
  for (auto p : this->SavedCameraPoses)
  {
    if (p.first < firstValue)
    {
      firstValue = p.first;
    }
    if (p.first > this->LastCameraPoseIndex)
    {
      nextValue = p.first;
      // is there anything lower than nextValue but still larger than current
      for (auto p2 : this->SavedCameraPoses)
      {
        if (p2.first > this->LastCameraPoseIndex && p2.first < nextValue)
        {
          nextValue = p2.first;
        }
      }
      break;
    }
  }

  if (nextValue == -1)
  {
    nextValue = firstValue;
  }

  this->LoadCameraPose(nextValue);
}

void svtkOpenVROverlay::Show()
{
  vr::VROverlay()->ShowOverlay(this->OverlayHandle);
  this->Render();
}

void svtkOpenVROverlay::Hide()
{
  vr::VROverlay()->HideOverlay(this->OverlayHandle);
}

void svtkOpenVROverlay::SetDashboardImageData(svtkJPEGReader* imgReader)
{
  imgReader->SetMemoryBuffer(OpenVRDashboard);
  imgReader->SetMemoryBufferLength(sizeof(OpenVRDashboard));
  imgReader->Update();
}

void svtkOpenVROverlay::Create(svtkOpenVRRenderWindow* win)
{
  if (!vr::VROverlay())
  {
    svtkErrorMacro("Error creating overlay");
    return;
  }

  if (this->OverlayHandle)
  {
    return;
  }

  this->Window = win;

  this->ReadCameraPoses();

  std::string sKey = std::string("SVTK OpenVR Settings");
  vr::VROverlayError overlayError = vr::VROverlay()->CreateDashboardOverlay(
    sKey.c_str(), "SVTK", &this->OverlayHandle, &this->OverlayThumbnailHandle);
  if (overlayError != vr::VROverlayError_None)
  {
    svtkErrorMacro("Error creating overlay");
    return;
  }

  vr::VROverlay()->SetOverlayFlag(
    this->OverlayHandle, vr::VROverlayFlags_SortWithNonSceneOverlays, true);
  vr::VROverlay()->SetOverlayFlag(this->OverlayHandle, vr::VROverlayFlags_VisibleInDashboard, true);
  vr::VROverlay()->SetOverlayWidthInMeters(this->OverlayHandle, 2.5f);
  vr::VROverlay()->SetOverlayInputMethod(this->OverlayHandle, vr::VROverlayInputMethod_Mouse);

  win->MakeCurrent();

  this->OverlayTexture->SetContext(win);

  // delete any old texture data
  if (this->OriginalTextureData)
  {
    delete[] this->OriginalTextureData;
    this->OriginalTextureData = 0;
  }

  // if dashboard image exists use it
  svtkNew<svtkJPEGReader> imgReader;
  if (!this->DashboardImageFileName.empty() &&
    imgReader->CanReadFile(this->DashboardImageFileName.c_str()))
  {
    imgReader->SetFileName(this->DashboardImageFileName.c_str());
    imgReader->Update();
  }
  else // use compiled in dashboard
  {
    this->SetDashboardImageData(imgReader);
  }

  svtkImageData* id = imgReader->GetOutput();
  int dims[3];
  id->GetDimensions(dims);
  int numC = id->GetPointData()->GetScalars()->GetNumberOfComponents();

  this->OriginalTextureData = new unsigned char[dims[0] * dims[1] * 4];
  this->CurrentTextureData = new unsigned char[dims[0] * dims[1] * 4];
  unsigned char* dataPtr = this->OriginalTextureData;
  unsigned char* inPtr =
    static_cast<unsigned char*>(id->GetPointData()->GetScalars()->GetVoidPointer(0));
  for (int j = 0; j < dims[1]; j++)
  {
    for (int i = 0; i < dims[0]; i++)
    {
      *(dataPtr++) = *(inPtr++);
      *(dataPtr++) = *(inPtr++);
      *(dataPtr++) = *(inPtr++);
      *(dataPtr++) = (numC == 4 ? *(inPtr++) : 255.0);
    }
  }
  memcpy(this->CurrentTextureData, this->OriginalTextureData, dims[0] * dims[1] * 4);
  this->OverlayTexture->Create2DFromRaw(dims[0], dims[1], 4, SVTK_UNSIGNED_CHAR,
    const_cast<void*>(static_cast<const void* const>(this->OriginalTextureData)));

  this->SetupSpots();

  int width = this->OverlayTexture->GetWidth();
  int height = this->OverlayTexture->GetHeight();
  vr::HmdVector2_t vecWindowSize = { static_cast<float>(width), static_cast<float>(height) };
  vr::VROverlay()->SetOverlayMouseScale(this->OverlayHandle, &vecWindowSize);
}

void svtkOpenVROverlay::Render()
{
  // skip rendering if the overlay isn't visible
  if (!vr::VROverlay() ||
    (!vr::VROverlay()->IsOverlayVisible(this->OverlayHandle) &&
      !vr::VROverlay()->IsOverlayVisible(this->OverlayThumbnailHandle)))
  {
    return;
  }

  this->Window->MakeCurrent();
  int dims[2];
  dims[0] = this->OverlayTexture->GetWidth();
  dims[1] = this->OverlayTexture->GetHeight();
  this->OverlayTexture->Create2DFromRaw(dims[0], dims[1], 4, SVTK_UNSIGNED_CHAR,
    const_cast<void*>(static_cast<const void* const>(this->CurrentTextureData)));
  this->OverlayTexture->Bind();
  GLuint unTexture = this->OverlayTexture->GetHandle();
  if (unTexture != 0)
  {
    vr::Texture_t texture = { (void*)(uintptr_t)unTexture, vr::TextureType_OpenGL,
      vr::ColorSpace_Auto };
    vr::VROverlay()->SetOverlayTexture(this->OverlayHandle, &texture);
  }
}

void svtkOpenVROverlay::MouseMoved(int x, int y)
{
  // did we leave the last active spot
  bool leftSpot = false;
  if (this->LastSpot &&
    (x < this->LastSpot->xmin || x > this->LastSpot->xmax || y < this->LastSpot->ymin ||
      y > this->LastSpot->ymax))
  {
    leftSpot = true;
    svtkOpenVROverlaySpot* spot = this->LastSpot;
    this->LastSpot = nullptr;
    this->UpdateSpot(spot);
  }

  // if we are in a spot already and did not leave
  // just return
  if (this->LastSpot)
  {
    return;
  }

  // did we enter a new spot?
  bool enteredSpot = false;

  std::vector<svtkOpenVROverlaySpot>::iterator it = this->Spots.begin();
  for (; !enteredSpot && it != this->Spots.end(); ++it)
  {
    if (x >= it->xmin && x <= it->xmax && y >= it->ymin && y <= it->ymax)
    {
      // if we are not already in this spot
      if (this->LastSpot != &(*it))
      {
        this->LastSpot = &(*it);
        enteredSpot = true;
        this->UpdateSpot(this->LastSpot);
      }
    }
  }

  if (!leftSpot && !enteredSpot)
  {
    return;
  }

  this->Render();
}

void svtkOpenVROverlay::UpdateSpot(svtkOpenVROverlaySpot* spot)
{
  int dims[2];
  dims[0] = this->OverlayTexture->GetWidth();
  dims[1] = this->OverlayTexture->GetHeight();
  unsigned char* currPtr = this->CurrentTextureData;
  unsigned char* origPtr = this->OriginalTextureData;

  float shift = 0.0;
  float scale = 1.0;
  if (spot->Active)
  {
    shift = this->ActiveSpotIntensity * 255.0;
    scale = 1.0 - this->ActiveSpotIntensity;
  }
  if (spot == this->LastSpot)
  {
    shift = this->LastSpotIntensity * 255.0;
    scale = 1.0 - this->LastSpotIntensity;
  }

  for (int j = spot->ymin; j <= spot->ymax; j++)
  {
    unsigned char* dataPtr = currPtr + (j * dims[0] + spot->xmin) * 4;
    unsigned char* inPtr = origPtr + (j * dims[0] + spot->xmin) * 4;
    for (int i = spot->xmin; i <= spot->xmax; i++)
    {
      *(dataPtr++) = static_cast<unsigned char>(scale * *(inPtr++) + shift);
      *(dataPtr++) = static_cast<unsigned char>(scale * *(inPtr++) + shift);
      *(dataPtr++) = static_cast<unsigned char>(scale * *(inPtr++) + shift);
      dataPtr++;
      inPtr++;
    }
  }
}

void svtkOpenVROverlay::MouseButtonPress(int x, int y)
{
  this->MouseMoved(x, y);
  if (this->LastSpot && this->LastSpot->Callback)
  {
    this->LastSpot->Callback->Execute(this, svtkCommand::LeftButtonPressEvent, this->Window);
  }
}

void svtkOpenVROverlay::MouseButtonRelease(int, int)
{
  if (this->LastSpot && this->LastSpot->Callback)
  {
    this->LastSpot->Callback->Execute(this, svtkCommand::LeftButtonReleaseEvent, this->Window);
  }
}

void svtkOpenVROverlay::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
