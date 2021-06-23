/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderWindow.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRenderWindow.h"

#include "svtkCamera.h"
#include "svtkCollection.h"
#include "svtkCommand.h"
#include "svtkGraphicsFactory.h"
#include "svtkMath.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkPropCollection.h"
#include "svtkRenderTimerLog.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRendererCollection.h"
#include "svtkStereoCompositor.h"
#include "svtkTimerLog.h"
#include "svtkTransform.h"
#include "svtkUnsignedCharArray.h"

#include <cmath>
#include <utility> // for std::swap

//----------------------------------------------------------------------------
svtkObjectFactoryNewMacro(svtkRenderWindow);

// Construct an instance of  svtkRenderWindow with its screen size
// set to 300x300, borders turned on, positioned at (0,0), double
// buffering turned on, stereo capable off.
svtkRenderWindow::svtkRenderWindow()
{
  this->Borders = 1;
  this->FullScreen = 0;
  this->OldScreen[0] = this->OldScreen[1] = 0;
  this->OldScreen[2] = this->OldScreen[3] = 300;
  this->OldScreen[4] = 1;
  this->DoubleBuffer = 1;
  this->PointSmoothing = 0;
  this->LineSmoothing = 0;
  this->PolygonSmoothing = 0;
  this->StereoRender = 0;
  this->StereoType = SVTK_STEREO_RED_BLUE;
  this->StereoCapableWindow = 0;
  this->AlphaBitPlanes = 0;
  this->StencilCapable = 0;
  this->Interactor = nullptr;
  this->DesiredUpdateRate = 0.0001;
  this->StereoBuffer = svtkSmartPointer<svtkUnsignedCharArray>::New();
  this->ResultFrame = svtkSmartPointer<svtkUnsignedCharArray>::New();
  this->SwapBuffers = 1;
  this->AbortRender = 0;
  this->InAbortCheck = 0;
  this->InRender = 0;
  this->NeverRendered = 1;
  this->Renderers = svtkRendererCollection::New();
  this->NumberOfLayers = 1;
  this->CurrentCursor = SVTK_CURSOR_DEFAULT;
  this->AnaglyphColorSaturation = 0.65f;
  this->AnaglyphColorMask[0] = 4; // red
  this->AnaglyphColorMask[1] = 3; // cyan

  this->AbortCheckTime = 0.0;
  this->CapturingGL2PSSpecialProps = 0;
  this->MultiSamples = 0;
  this->UseSRGBColorSpace = false;

#ifdef SVTK_DEFAULT_RENDER_WINDOW_OFFSCREEN
  this->ShowWindow = false;
  this->UseOffScreenBuffers = true;
#endif
  this->DeviceIndex = 0;
  this->SharedRenderWindow = nullptr;
}

//----------------------------------------------------------------------------
svtkRenderWindow::~svtkRenderWindow()
{
  this->SetInteractor(nullptr);
  this->SetSharedRenderWindow(nullptr);

  if (this->Renderers)
  {
    svtkRenderer* ren;
    svtkCollectionSimpleIterator rit;
    this->Renderers->InitTraversal(rit);
    while ((ren = this->Renderers->GetNextRenderer(rit)))
    {
      ren->SetRenderWindow(nullptr);
    }

    this->Renderers->Delete();
  }
}

void svtkRenderWindow::SetMultiSamples(int val)
{
  if (val == 1)
  {
    val = 0;
  }

  if (val == this->MultiSamples)
  {
    return;
  }

  this->MultiSamples = val;
  this->Modified();
}

//----------------------------------------------------------------------------
// Create an interactor that will work with this renderer.
svtkRenderWindowInteractor* svtkRenderWindow::MakeRenderWindowInteractor()
{
  this->Interactor = svtkRenderWindowInteractor::New();
  this->Interactor->SetRenderWindow(this);
  return this->Interactor;
}

void svtkRenderWindow::SetSharedRenderWindow(svtkRenderWindow* val)
{
  if (this->SharedRenderWindow == val)
  {
    return;
  }

  if (this->SharedRenderWindow)
  {
    // this->ReleaseGraphicsResources();
    this->SharedRenderWindow->UnRegister(this);
  }
  this->SharedRenderWindow = val;
  if (val)
  {
    val->Register(this);
  }
}

//----------------------------------------------------------------------------
// Set the interactor that will work with this renderer.
void svtkRenderWindow::SetInteractor(svtkRenderWindowInteractor* rwi)
{
  if (this->Interactor != rwi)
  {
    // to avoid destructor recursion
    svtkRenderWindowInteractor* temp = this->Interactor;
    this->Interactor = rwi;
    if (temp != nullptr)
    {
      temp->UnRegister(this);
    }
    if (this->Interactor != nullptr)
    {
      this->Interactor->Register(this);

      int isize[2];
      this->Interactor->GetSize(isize);
      if (0 == isize[0] && 0 == isize[1])
      {
        this->Interactor->SetSize(this->GetSize());
      }

      if (this->Interactor->GetRenderWindow() != this)
      {
        this->Interactor->SetRenderWindow(this);
      }
    }
  }
}

//----------------------------------------------------------------------------
void svtkRenderWindow::SetDesiredUpdateRate(double rate)
{
  svtkRenderer* aren;

  if (this->DesiredUpdateRate != rate)
  {
    svtkCollectionSimpleIterator rsit;
    for (this->Renderers->InitTraversal(rsit); (aren = this->Renderers->GetNextRenderer(rsit));)
    {
      aren->SetAllocatedRenderTime(1.0 / (rate * this->Renderers->GetNumberOfItems()));
    }
    this->DesiredUpdateRate = rate;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkRenderWindow::SetStereoType(int stereoType)
{
  if (this->StereoType == stereoType)
  {
    return;
  }

  this->StereoType = stereoType;
  this->InvokeEvent(svtkCommand::WindowStereoTypeChangedEvent);

  this->Modified();
}

//----------------------------------------------------------------------------
//
// Set the variable that indicates that we want a stereo capable window
// be created. This method can only be called before a window is realized.
//
void svtkRenderWindow::SetStereoCapableWindow(svtkTypeBool capable)
{
  if (this->StereoCapableWindow != capable)
  {
    this->StereoCapableWindow = capable;
    this->Modified();
  }
}

//----------------------------------------------------------------------------
// Turn on stereo rendering
void svtkRenderWindow::SetStereoRender(svtkTypeBool stereo)
{
  if (stereo == this->StereoRender)
  {
    return;
  }

  if (this->StereoCapableWindow || (this->StereoType != SVTK_STEREO_CRYSTAL_EYES))
  {
    this->StereoRender = stereo;
    this->Modified();
  }
  else
  {
    svtkWarningMacro(<< "Adjusting stereo mode on a window that does not "
                    << "support stereo type " << this->GetStereoTypeAsString()
                    << " is not possible.");
  }
}

//----------------------------------------------------------------------------
// Ask each renderer owned by this RenderWindow to render its image and
// synchronize this process.
void svtkRenderWindow::Render()
{
  // if we are in the middle of an abort check then return now
  if (this->InAbortCheck)
  {
    return;
  }

  // if we are in a render already from somewhere else abort now
  if (this->InRender)
  {
    return;
  }

  // if SetSize has not yet been called (from a script, possible off
  // screen use, other scenarios?) then call it here with reasonable
  // default values
  if (0 == this->Size[0] && 0 == this->Size[1])
  {
    this->SetSize(300, 300);
  }

  // reset the Abort flag
  this->AbortRender = 0;
  this->InRender = 1;

  svtkDebugMacro(<< "Starting Render Method.\n");
  this->InvokeEvent(svtkCommand::StartEvent, nullptr);

  this->NeverRendered = 0;

  if (this->Interactor && !this->Interactor->GetInitialized())
  {
    this->Interactor->Initialize();
  }

  this->Start(); // Ensure context exists
  svtkRenderTimerLog::ScopedEventLogger event;
  if (this->RenderTimer->GetLoggingEnabled())
  {
    this->RenderTimer->MarkFrame();
    event = this->RenderTimer->StartScopedEvent("svtkRenderWindow::Render");
  }

  this->DoStereoRender();

  this->End(); // restores original bindings

  this->CopyResultFrame();

  // reset the buffer size without freeing any memory.
  this->ResultFrame->Reset();

  // Stop the render timer before invoking the EndEvent.
  event.Stop();

  this->InRender = 0;
  this->InvokeEvent(svtkCommand::EndEvent, nullptr);
}

//----------------------------------------------------------------------------
// Handle rendering the two different views for stereo rendering.
void svtkRenderWindow::DoStereoRender()
{
  svtkCollectionSimpleIterator rsit;

  this->StereoUpdate();

  if (!this->StereoRender || (this->StereoType != SVTK_STEREO_RIGHT))
  { // render the left eye
    svtkRenderer* aren;
    for (this->Renderers->InitTraversal(rsit); (aren = this->Renderers->GetNextRenderer(rsit));)
    {
      // Ugly piece of code - we need to know if the camera already
      // exists or not. If it does not yet exist, we must reset the
      // camera here - otherwise it will never be done (missing its
      // oppportunity to be reset in the Render method of the
      // svtkRenderer because it will already exist by that point...)
      if (!aren->IsActiveCameraCreated())
      {
        aren->ResetCamera();
      }
      aren->GetActiveCamera()->SetLeftEye(1);
    }
    this->Renderers->Render();
  }

  if (this->StereoRender)
  {
    this->StereoMidpoint();
    if (this->StereoType != SVTK_STEREO_LEFT)
    { // render the right eye
      svtkRenderer* aren;
      for (this->Renderers->InitTraversal(rsit); (aren = this->Renderers->GetNextRenderer(rsit));)
      {
        // Duplicate the ugly code here too. Of course, most
        // times the left eye will have been rendered before
        // the right eye, but it is possible that the user sets
        // everything up and renders just the right eye - so we
        // need this check here too.
        if (!aren->IsActiveCameraCreated())
        {
          aren->ResetCamera();
        }
        if (this->StereoType != SVTK_STEREO_FAKE)
        {
          aren->GetActiveCamera()->SetLeftEye(0);
        }
      }
      this->Renderers->Render();
    }
    this->StereoRenderComplete();
  }
}

//----------------------------------------------------------------------------
// Add a renderer to the list of renderers.
void svtkRenderWindow::AddRenderer(svtkRenderer* ren)
{
  if (this->HasRenderer(ren))
  {
    return;
  }
  // we are its parent
  this->MakeCurrent();
  ren->SetRenderWindow(this);
  this->Renderers->AddItem(ren);
  svtkRenderer* aren;
  svtkCollectionSimpleIterator rsit;

  for (this->Renderers->InitTraversal(rsit); (aren = this->Renderers->GetNextRenderer(rsit));)
  {
    aren->SetAllocatedRenderTime(
      1.0 / (this->DesiredUpdateRate * this->Renderers->GetNumberOfItems()));
  }
}

//----------------------------------------------------------------------------
// Remove a renderer from the list of renderers.
void svtkRenderWindow::RemoveRenderer(svtkRenderer* ren)
{
  // we are its parent
  if (ren->GetRenderWindow() == this)
  {
    ren->ReleaseGraphicsResources(this);
    ren->SetRenderWindow(nullptr);
  }
  this->Renderers->RemoveItem(ren);
}

int svtkRenderWindow::HasRenderer(svtkRenderer* ren)
{
  return (ren && this->Renderers->IsItemPresent(ren));
}

//----------------------------------------------------------------------------
int svtkRenderWindow::CheckAbortStatus()
{
  if (!this->InAbortCheck)
  {
    // Only check for abort at most 5 times per second.
    if (svtkTimerLog::GetUniversalTime() - this->AbortCheckTime > 0.2)
    {
      this->InAbortCheck = 1;
      this->InvokeEvent(svtkCommand::AbortCheckEvent, nullptr);
      this->InAbortCheck = 0;
      this->AbortCheckTime = svtkTimerLog::GetUniversalTime();
    }
  }
  return this->AbortRender;
}

//----------------------------------------------------------------------------
void svtkRenderWindow::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Borders: " << (this->Borders ? "On\n" : "Off\n");
  os << indent << "Double Buffer: " << (this->DoubleBuffer ? "On\n" : "Off\n");
  os << indent << "Full Screen: " << (this->FullScreen ? "On\n" : "Off\n");
  os << indent << "Renderers:\n";
  this->Renderers->PrintSelf(os, indent.GetNextIndent());
  os << indent
     << "Stereo Capable Window Requested: " << (this->StereoCapableWindow ? "Yes\n" : "No\n");
  os << indent << "Stereo Render: " << (this->StereoRender ? "On\n" : "Off\n");

  os << indent << "Point Smoothing: " << (this->PointSmoothing ? "On\n" : "Off\n");
  os << indent << "Line Smoothing: " << (this->LineSmoothing ? "On\n" : "Off\n");
  os << indent << "Polygon Smoothing: " << (this->PolygonSmoothing ? "On\n" : "Off\n");
  os << indent << "Abort Render: " << this->AbortRender << "\n";
  os << indent << "Current Cursor: " << this->CurrentCursor << "\n";
  os << indent << "Desired Update Rate: " << this->DesiredUpdateRate << "\n";
  os << indent << "In Abort Check: " << this->InAbortCheck << "\n";
  os << indent << "NeverRendered: " << this->NeverRendered << "\n";
  os << indent << "Interactor: " << this->Interactor << "\n";
  os << indent << "Swap Buffers: " << (this->SwapBuffers ? "On\n" : "Off\n");
  os << indent << "Stereo Type: " << this->GetStereoTypeAsString() << "\n";
  os << indent << "Number of Layers: " << this->NumberOfLayers << "\n";
  os << indent << "AlphaBitPlanes: " << (this->AlphaBitPlanes ? "On" : "Off") << endl;
  os << indent << "UseSRGBColorSpace: " << (this->UseSRGBColorSpace ? "On" : "Off") << endl;

  os << indent << "AnaglyphColorSaturation: " << this->AnaglyphColorSaturation << "\n";
  os << indent << "AnaglyphColorMask: " << this->AnaglyphColorMask[0] << " , "
     << this->AnaglyphColorMask[1] << "\n";

  os << indent << "MultiSamples: " << this->MultiSamples << "\n";
  os << indent << "StencilCapable: " << (this->StencilCapable ? "True" : "False") << endl;
}

//----------------------------------------------------------------------------
// Update the system, if needed, due to stereo rendering. For some stereo
// methods, subclasses might need to switch some hardware settings here.
void svtkRenderWindow::StereoUpdate() {}

//----------------------------------------------------------------------------
// Intermediate method performs operations required between the rendering
// of the left and right eye.
void svtkRenderWindow::StereoMidpoint()
{
  svtkRenderer* aren;
  /* For IceT stereo */
  for (Renderers->InitTraversal(); (aren = Renderers->GetNextItem());)
  {
    aren->StereoMidpoint();
  }
  if ((this->StereoType == SVTK_STEREO_RED_BLUE) || (this->StereoType == SVTK_STEREO_INTERLACED) ||
    (this->StereoType == SVTK_STEREO_DRESDEN) || (this->StereoType == SVTK_STEREO_ANAGLYPH) ||
    (this->StereoType == SVTK_STEREO_CHECKERBOARD) ||
    (this->StereoType == SVTK_STEREO_SPLITVIEWPORT_HORIZONTAL))
  {
    int* size;
    // get the size
    size = this->GetSize();
    // get the data
    this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->StereoBuffer);
  }
}

//----------------------------------------------------------------------------
// Handles work required once both views have been rendered when using
// stereo rendering.
void svtkRenderWindow::StereoRenderComplete()
{
  const int* size = this->GetSize();
  switch (this->StereoType)
  {
    case SVTK_STEREO_RED_BLUE:
      this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->ResultFrame);
      this->StereoCompositor->RedBlue(this->StereoBuffer, this->ResultFrame);
      std::swap(this->StereoBuffer, this->ResultFrame);
      break;

    case SVTK_STEREO_ANAGLYPH:
      this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->ResultFrame);
      this->StereoCompositor->Anaglyph(this->StereoBuffer, this->ResultFrame,
        this->AnaglyphColorSaturation, this->AnaglyphColorMask);
      std::swap(this->StereoBuffer, this->ResultFrame);
      break;

    case SVTK_STEREO_INTERLACED:
      this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->ResultFrame);
      this->StereoCompositor->Interlaced(this->StereoBuffer, this->ResultFrame, size);
      std::swap(this->StereoBuffer, this->ResultFrame);
      break;

    case SVTK_STEREO_DRESDEN:
      this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->ResultFrame);
      this->StereoCompositor->Dresden(this->StereoBuffer, this->ResultFrame, size);
      std::swap(this->StereoBuffer, this->ResultFrame);
      break;

    case SVTK_STEREO_CHECKERBOARD:
      this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->ResultFrame);
      this->StereoCompositor->Checkerboard(this->StereoBuffer, this->ResultFrame, size);
      std::swap(this->StereoBuffer, this->ResultFrame);
      break;

    case SVTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
      this->GetPixelData(0, 0, size[0] - 1, size[1] - 1, !this->DoubleBuffer, this->ResultFrame);
      this->StereoCompositor->SplitViewportHorizontal(this->StereoBuffer, this->ResultFrame, size);
      std::swap(this->StereoBuffer, this->ResultFrame);
      break;
  }

  this->StereoBuffer->Reset();
}

//----------------------------------------------------------------------------
void svtkRenderWindow::CopyResultFrame()
{
  if (this->ResultFrame->GetNumberOfTuples() > 0)
  {
    int* size;

    // get the size
    size = this->GetSize();

    assert(this->ResultFrame->GetNumberOfTuples() == size[0] * size[1]);

    this->SetPixelData(0, 0, size[0] - 1, size[1] - 1, this->ResultFrame, !this->DoubleBuffer);
  }

  // Just before we swap buffers (in case of double buffering), we fire the
  // RenderEvent marking that a render call has concluded successfully. We
  // separate this from EndEvent since some applications may want to put some
  // more elements on the "draw-buffer" before calling the rendering complete.
  // This event gives them that opportunity.
  this->InvokeEvent(svtkCommand::RenderEvent);
  this->Frame();
}

//----------------------------------------------------------------------------
// treat renderWindow and interactor as one object.
// it might be easier if the GetReference count method were redefined.
void svtkRenderWindow::UnRegister(svtkObjectBase* o)
{
  if (this->Interactor && this->Interactor->GetRenderWindow() == this && this->Interactor != o)
  {
    if (this->GetReferenceCount() + this->Interactor->GetReferenceCount() == 3)
    {
      this->svtkObject::UnRegister(o);
      svtkRenderWindowInteractor* tmp = this->Interactor;
      tmp->Register(nullptr);
      this->Interactor->SetRenderWindow(nullptr);
      tmp->UnRegister(nullptr);
      return;
    }
  }

  this->svtkObject::UnRegister(o);
}

//----------------------------------------------------------------------------
const char* svtkRenderWindow::GetRenderLibrary()
{
  return svtkGraphicsFactory::GetRenderLibrary();
}

//----------------------------------------------------------------------------
const char* svtkRenderWindow::GetRenderingBackend()
{
  return "Unknown";
}

//----------------------------------------------------------------------------
void svtkRenderWindow::CaptureGL2PSSpecialProps(svtkCollection* result)
{
  if (result == nullptr)
  {
    svtkErrorMacro(<< "CaptureGL2PSSpecialProps was passed a nullptr pointer.");
    return;
  }

  result->RemoveAllItems();

  if (this->CapturingGL2PSSpecialProps)
  {
    svtkDebugMacro(<< "Called recursively.");
    return;
  }
  this->CapturingGL2PSSpecialProps = 1;

  svtkRenderer* ren;
  for (Renderers->InitTraversal(); (ren = Renderers->GetNextItem());)
  {
    svtkNew<svtkPropCollection> props;
    result->AddItem(props);
    ren->SetGL2PSSpecialPropCollection(props);
  }

  this->Render();

  for (Renderers->InitTraversal(); (ren = Renderers->GetNextItem());)
  {
    ren->SetGL2PSSpecialPropCollection(nullptr);
  }
  this->CapturingGL2PSSpecialProps = 0;
}

// Description: Return the stereo type as a character string.
// when this method was inlined, static linking on BlueGene failed
// (symbol referenced which is defined in discarded section)
const char* svtkRenderWindow::GetStereoTypeAsString()
{
  return svtkRenderWindow::GetStereoTypeAsString(this->StereoType);
}

const char* svtkRenderWindow::GetStereoTypeAsString(int type)
{
  switch (type)
  {
    case SVTK_STEREO_CRYSTAL_EYES:
      return "CrystalEyes";
    case SVTK_STEREO_RED_BLUE:
      return "RedBlue";
    case SVTK_STEREO_LEFT:
      return "Left";
    case SVTK_STEREO_RIGHT:
      return "Right";
    case SVTK_STEREO_DRESDEN:
      return "DresdenDisplay";
    case SVTK_STEREO_ANAGLYPH:
      return "Anaglyph";
    case SVTK_STEREO_CHECKERBOARD:
      return "Checkerboard";
    case SVTK_STEREO_SPLITVIEWPORT_HORIZONTAL:
      return "SplitViewportHorizontal";
    case SVTK_STEREO_FAKE:
      return "Fake";
    case SVTK_STEREO_EMULATE:
      return "Emulate";
    default:
      return "";
  }
}

#if !defined(SVTK_LEGACY_REMOVE)
svtkTypeBool svtkRenderWindow::GetIsPicking()
{
  SVTK_LEGACY_BODY(svtkRenderWindow::GetIsPicking, "SVTK 9.0");
  return false;
}
void svtkRenderWindow::SetIsPicking(svtkTypeBool)
{
  SVTK_LEGACY_BODY(svtkRenderWindow::SetIsPicking, "SVTK 9.0");
}
void svtkRenderWindow::IsPickingOn()
{
  SVTK_LEGACY_BODY(svtkRenderWindow::IsPickingOn, "SVTK 9.0");
}
void svtkRenderWindow::IsPickingOff()
{
  SVTK_LEGACY_BODY(svtkRenderWindow::IsPickingOff, "SVTK 9.0");
}
#endif

//----------------------------------------------------------------------------
#ifndef SVTK_LEGACY_REMOVE
bool svtkRenderWindow::IsDrawable()
{
  return true;
}
#endif
