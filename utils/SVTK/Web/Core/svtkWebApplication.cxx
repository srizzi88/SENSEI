/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWebApplication.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkWebApplication.h"

#include "svtkBase64Utilities.h"
#include "svtkCamera.h"
#include "svtkCommand.h"
#include "svtkDataEncoder.h"
#include "svtkImageData.h"
#include "svtkJPEGWriter.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkObjectIdMap.h"
#include "svtkPNGWriter.h"
#include "svtkPointData.h"
#include "svtkRenderWindow.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkRendererCollection.h"
#include "svtkSmartPointer.h"
#include "svtkTimerLog.h"
#include "svtkUnsignedCharArray.h"
#include "svtkWebGLExporter.h"
#include "svtkWebGLObject.h"
#include "svtkWebInteractionEvent.h"
#include "svtkWindowToImageFilter.h"

#include <cassert>
#include <cmath>
#include <map>
#include <sstream>

class svtkWebApplication::svtkInternals
{
public:
  struct ImageCacheValueType
  {
  public:
    svtkSmartPointer<svtkUnsignedCharArray> Data;
    bool NeedsRender;
    bool HasImagesBeingProcessed;
    svtkObject* ViewPointer;
    unsigned long ObserverId;
    ImageCacheValueType()
      : NeedsRender(true)
      , HasImagesBeingProcessed(false)
      , ViewPointer(nullptr)
      , ObserverId(0)
    {
    }

    void SetListener(svtkObject* view)
    {
      if (this->ViewPointer == view)
      {
        return;
      }

      if (this->ViewPointer && this->ObserverId)
      {
        this->ViewPointer->RemoveObserver(this->ObserverId);
        this->ObserverId = 0;
      }
      this->ViewPointer = view;
      if (this->ViewPointer)
      {
        this->ObserverId = this->ViewPointer->AddObserver(
          svtkCommand::AnyEvent, this, &ImageCacheValueType::ViewEventListener);
      }
    }

    void RemoveListener(svtkObject* view)
    {
      if (this->ViewPointer && this->ViewPointer == view && this->ObserverId)
      {
        this->ViewPointer->RemoveObserver(this->ObserverId);
        this->ObserverId = 0;
        this->ViewPointer = nullptr;
      }
    }

    void ViewEventListener(svtkObject*, unsigned long, void*) { this->NeedsRender = true; }
  };
  typedef std::map<void*, ImageCacheValueType> ImageCacheType;
  ImageCacheType ImageCache;

  typedef std::map<void*, unsigned int> ButtonStatesType;
  ButtonStatesType ButtonStates;

  svtkNew<svtkDataEncoder> Encoder;

  // WebGL related struct
  struct WebGLObjCacheValue
  {
  public:
    int ObjIndex;
    std::map<int, std::string> BinaryParts;
  };
  // map for <svtkWebGLExporter, <webgl-objID, WebGLObjCacheValue> >
  typedef std::map<std::string, WebGLObjCacheValue> WebGLObjId2IndexMap;
  std::map<svtkWebGLExporter*, WebGLObjId2IndexMap> WebGLExporterObjIdMap;
  // map for <svtkRenderWindow, svtkWebGLExporter>
  std::map<svtkRenderWindow*, svtkSmartPointer<svtkWebGLExporter> > ViewWebGLMap;
  std::string LastAllWebGLBinaryObjects;
  svtkNew<svtkObjectIdMap> ObjectIdMap;
};

svtkStandardNewMacro(svtkWebApplication);
//----------------------------------------------------------------------------
svtkWebApplication::svtkWebApplication()
  : ImageEncoding(ENCODING_BASE64)
  , ImageCompression(COMPRESSION_JPEG)
  , Internals(new svtkWebApplication::svtkInternals())
{
}

//----------------------------------------------------------------------------
svtkWebApplication::~svtkWebApplication()
{
  delete this->Internals;
  this->Internals = nullptr;
}

//----------------------------------------------------------------------------
void svtkWebApplication::SetNumberOfEncoderThreads(svtkTypeUInt32 numThreads)
{
  this->Internals->Encoder->SetMaxThreads(numThreads);
  this->Internals->Encoder->Initialize();
}

//----------------------------------------------------------------------------
svtkTypeUInt32 svtkWebApplication::GetNumberOfEncoderThreads()
{
  return this->Internals->Encoder->GetMaxThreads();
}

//----------------------------------------------------------------------------
bool svtkWebApplication::GetHasImagesBeingProcessed(svtkRenderWindow* view)
{
  const svtkInternals::ImageCacheValueType& value = this->Internals->ImageCache[view];
  return value.HasImagesBeingProcessed;
}

//----------------------------------------------------------------------------
svtkUnsignedCharArray* svtkWebApplication::InteractiveRender(svtkRenderWindow* view, int quality)
{
  // for now, just do the same as StillRender().
  return this->StillRender(view, quality);
}

//----------------------------------------------------------------------------
void svtkWebApplication::InvalidateCache(svtkRenderWindow* view)
{
  this->Internals->ImageCache[view].NeedsRender = true;
}

//----------------------------------------------------------------------------
svtkUnsignedCharArray* svtkWebApplication::StillRender(svtkRenderWindow* view, int quality)
{
  if (!view)
  {
    svtkErrorMacro("No view specified.");
    return nullptr;
  }

  svtkInternals::ImageCacheValueType& value = this->Internals->ImageCache[view];
  value.SetListener(view);

  if (value.NeedsRender == false &&
    value.Data != nullptr /* FIXME SEB &&
    view->HasDirtyRepresentation() == false */)
  {
    bool latest = this->Internals->Encoder->GetLatestOutput(
      this->Internals->ObjectIdMap->GetGlobalId(view), value.Data);
    value.HasImagesBeingProcessed = !latest;
    return value.Data;
  }

  // cout <<  "Regenerating " << endl;
  // svtkTimerLog::ResetLog();
  // svtkTimerLog::CleanupLog();
  // svtkTimerLog::MarkStartEvent("StillRenderToString");
  // svtkTimerLog::MarkStartEvent("CaptureWindow");

  view->Render();

  // TODO: We should add logic to check if a new rendering needs to be done and
  // then alone do a new rendering otherwise use the cached image.
  svtkNew<svtkWindowToImageFilter> w2i;
  w2i->SetInput(view);
  w2i->SetScale(1);
  w2i->ReadFrontBufferOff();
  w2i->ShouldRerenderOff();
  w2i->FixBoundaryOn();
  w2i->Update();

  svtkImageData* image = svtkImageData::New();
  image->ShallowCopy(w2i->GetOutput());

  // svtkTimerLog::MarkEndEvent("CaptureWindow");

  // svtkTimerLog::MarkEndEvent("StillRenderToString");
  // svtkTimerLog::DumpLogWithIndents(&cout, 0.0);

  this->Internals->Encoder->PushAndTakeReference(
    this->Internals->ObjectIdMap->GetGlobalId(view), image, quality, this->ImageEncoding);
  assert(image == nullptr);

  if (value.Data == nullptr)
  {
    // we need to wait till output is processed.
    // cout << "Flushing" << endl;
    this->Internals->Encoder->Flush(this->Internals->ObjectIdMap->GetGlobalId(view));
    // cout << "Done Flushing" << endl;
  }

  bool latest = this->Internals->Encoder->GetLatestOutput(
    this->Internals->ObjectIdMap->GetGlobalId(view), value.Data);
  value.HasImagesBeingProcessed = !latest;
  value.NeedsRender = false;
  return value.Data;
}

//----------------------------------------------------------------------------
const char* svtkWebApplication::StillRenderToString(
  svtkRenderWindow* view, svtkMTimeType time, int quality)
{
  svtkUnsignedCharArray* array = this->StillRender(view, quality);
  if (array && array->GetMTime() != time)
  {
    this->LastStillRenderToMTime = array->GetMTime();
    // cout << "Image size: " << array->GetNumberOfTuples() << endl;
    return reinterpret_cast<char*>(array->GetPointer(0));
  }
  return nullptr;
}

//----------------------------------------------------------------------------
svtkUnsignedCharArray* svtkWebApplication::StillRenderToBuffer(
  svtkRenderWindow* view, svtkMTimeType time, int quality)
{
  svtkUnsignedCharArray* array = this->StillRender(view, quality);
  if (array && array->GetMTime() != time)
  {
    this->LastStillRenderToMTime = array->GetMTime();
    return array;
  }
  return nullptr;
}

//----------------------------------------------------------------------------
bool svtkWebApplication::HandleInteractionEvent(svtkRenderWindow* view, svtkWebInteractionEvent* event)
{
  svtkRenderWindowInteractor* iren = nullptr;

  if (view)
  {
    iren = view->GetInteractor();
  }
  else
  {
    svtkErrorMacro("Interaction not supported for view : " << view);
    return false;
  }

  int ctrlKey = (event->GetModifiers() & svtkWebInteractionEvent::CTRL_KEY) != 0 ? 1 : 0;
  int shiftKey = (event->GetModifiers() & svtkWebInteractionEvent::SHIFT_KEY) != 0 ? 1 : 0;

  // Handle scroll action if any
  if (event->GetScroll())
  {
    iren->SetEventInformation(0, 0, ctrlKey, shiftKey, event->GetKeyCode(), 0);
    iren->MouseMoveEvent();
    iren->RightButtonPressEvent();
    iren->SetEventInformation(
      0, event->GetScroll() * 10, ctrlKey, shiftKey, event->GetKeyCode(), 0);
    iren->MouseMoveEvent();
    iren->RightButtonReleaseEvent();
    this->Internals->ImageCache[view].NeedsRender = true;
    return true;
  }

  const int* viewSize = view->GetSize();
  int posX = std::floor(viewSize[0] * event->GetX() + 0.5);
  int posY = std::floor(viewSize[1] * event->GetY() + 0.5);

  iren->SetEventInformation(
    posX, posY, ctrlKey, shiftKey, event->GetKeyCode(), event->GetRepeatCount());

  unsigned int prev_buttons = this->Internals->ButtonStates[view];
  unsigned int changed_buttons = (event->GetButtons() ^ prev_buttons);
  iren->MouseMoveEvent();
  if ((changed_buttons & svtkWebInteractionEvent::LEFT_BUTTON) != 0)
  {
    if ((event->GetButtons() & svtkWebInteractionEvent::LEFT_BUTTON) != 0)
    {
      iren->LeftButtonPressEvent();
      if (event->GetRepeatCount() > 0)
      {
        iren->LeftButtonReleaseEvent();
      }
    }
    else
    {
      iren->LeftButtonReleaseEvent();
    }
  }

  if ((changed_buttons & svtkWebInteractionEvent::RIGHT_BUTTON) != 0)
  {
    if ((event->GetButtons() & svtkWebInteractionEvent::RIGHT_BUTTON) != 0)
    {
      iren->RightButtonPressEvent();
      if (event->GetRepeatCount() > 0)
      {
        iren->RightButtonPressEvent();
      }
    }
    else
    {
      iren->RightButtonReleaseEvent();
    }
  }
  if ((changed_buttons & svtkWebInteractionEvent::MIDDLE_BUTTON) != 0)
  {
    if ((event->GetButtons() & svtkWebInteractionEvent::MIDDLE_BUTTON) != 0)
    {
      iren->MiddleButtonPressEvent();
      if (event->GetRepeatCount() > 0)
      {
        iren->MiddleButtonPressEvent();
      }
    }
    else
    {
      iren->MiddleButtonReleaseEvent();
    }
  }

  this->Internals->ButtonStates[view] = event->GetButtons();

  bool needs_render = (changed_buttons != 0 || event->GetButtons());
  this->Internals->ImageCache[view].NeedsRender = needs_render;
  return needs_render;
}

// ---------------------------------------------------------------------------
const char* svtkWebApplication::GetWebGLSceneMetaData(svtkRenderWindow* view)
{
  if (!view)
  {
    svtkErrorMacro("No view specified.");
    return nullptr;
  }

  // We use the camera focal point to be the center of rotation
  double centerOfRotation[3];
  svtkCamera* cam = view->GetRenderers()->GetFirstRenderer()->GetActiveCamera();
  cam->GetFocalPoint(centerOfRotation);

  if (this->Internals->ViewWebGLMap.find(view) == this->Internals->ViewWebGLMap.end())
  {
    this->Internals->ViewWebGLMap[view] = svtkSmartPointer<svtkWebGLExporter>::New();
  }

  std::stringstream globalIdAsString;
  globalIdAsString << this->Internals->ObjectIdMap->GetGlobalId(view);

  svtkWebGLExporter* webglExporter = this->Internals->ViewWebGLMap[view];
  webglExporter->parseScene(view->GetRenderers(), globalIdAsString.str().c_str(), SVTK_PARSEALL);

  svtkInternals::WebGLObjId2IndexMap webglMap;
  for (int i = 0; i < webglExporter->GetNumberOfObjects(); ++i)
  {
    svtkWebGLObject* wObj = webglExporter->GetWebGLObject(i);
    if (wObj && wObj->isVisible())
    {
      svtkInternals::WebGLObjCacheValue val;
      val.ObjIndex = i;
      for (int j = 0; j < wObj->GetNumberOfParts(); ++j)
      {
        val.BinaryParts[j] = "";
      }
      webglMap[wObj->GetId()] = val;
    }
  }
  this->Internals->WebGLExporterObjIdMap[webglExporter] = webglMap;
  webglExporter->SetCenterOfRotation(static_cast<float>(centerOfRotation[0]),
    static_cast<float>(centerOfRotation[1]), static_cast<float>(centerOfRotation[2]));
  return webglExporter->GenerateMetadata();
}

//----------------------------------------------------------------------------
const char* svtkWebApplication::GetWebGLBinaryData(svtkRenderWindow* view, const char* id, int part)
{
  if (!view)
  {
    svtkErrorMacro("No view specified.");
    return nullptr;
  }
  if (this->Internals->ViewWebGLMap.find(view) == this->Internals->ViewWebGLMap.end())
  {
    if (this->GetWebGLSceneMetaData(view) == nullptr)
    {
      svtkErrorMacro("Failed to generate WebGL MetaData for: " << view);
      return nullptr;
    }
  }

  svtkWebGLExporter* webglExporter = this->Internals->ViewWebGLMap[view];
  if (webglExporter == nullptr)
  {
    svtkErrorMacro("There is no cached WebGL Exporter for: " << view);
    return nullptr;
  }

  if (!this->Internals->WebGLExporterObjIdMap[webglExporter].empty() &&
    this->Internals->WebGLExporterObjIdMap[webglExporter].find(id) !=
      this->Internals->WebGLExporterObjIdMap[webglExporter].end())
  {
    svtkInternals::WebGLObjCacheValue* cachedVal =
      &(this->Internals->WebGLExporterObjIdMap[webglExporter][id]);
    if (cachedVal->BinaryParts.find(part) != cachedVal->BinaryParts.end())
    {
      if (cachedVal->BinaryParts[part].empty())
      {
        svtkWebGLObject* obj = webglExporter->GetWebGLObject(cachedVal->ObjIndex);
        if (obj && obj->isVisible())
        {
          // Manage Base64
          svtkNew<svtkBase64Utilities> base64;
          unsigned char* output = new unsigned char[obj->GetBinarySize(part) * 2];
          int size =
            base64->Encode(obj->GetBinaryData(part), obj->GetBinarySize(part), output, false);
          cachedVal->BinaryParts[part] = std::string((const char*)output, size);
          delete[] output;
        }
      }
      return cachedVal->BinaryParts[part].c_str();
    }
  }

  return nullptr;
}

//----------------------------------------------------------------------------
void svtkWebApplication::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "ImageEncoding: " << this->ImageEncoding << endl;
  os << indent << "ImageCompression: " << this->ImageCompression << endl;
}

//----------------------------------------------------------------------------
svtkObjectIdMap* svtkWebApplication::GetObjectIdMap()
{
  return this->Internals->ObjectIdMap;
}

//----------------------------------------------------------------------------
std::string svtkWebApplication::GetObjectId(svtkObject* obj)
{
  std::ostringstream oss;
  oss << std::hex << static_cast<void*>(obj);
  return oss.str();
}
