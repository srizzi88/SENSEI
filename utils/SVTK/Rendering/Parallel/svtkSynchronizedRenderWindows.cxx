/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSynchronizedRenderWindows.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSynchronizedRenderWindows.h"

#include "svtkCommand.h"
#include "svtkMultiProcessController.h"
#include "svtkMultiProcessStream.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkWeakPointer.h"

#include <map>

//----------------------------------------------------------------------------
class svtkSynchronizedRenderWindows::svtkObserver : public svtkCommand
{
public:
  static svtkObserver* New()
  {
    svtkObserver* obs = new svtkObserver();
    obs->Target = nullptr;
    return obs;
  }

  void Execute(svtkObject*, unsigned long eventId, void*) override
  {
    if (this->Target)
    {
      switch (eventId)
      {
        case svtkCommand::StartEvent:
          this->Target->HandleStartRender();
          break;

        case svtkCommand::EndEvent:
          this->Target->HandleEndRender();
          break;

        case svtkCommand::AbortCheckEvent:
          this->Target->HandleAbortRender();
          break;
      }
    }
  }

  svtkSynchronizedRenderWindows* Target;
};

//----------------------------------------------------------------------------
namespace
{
typedef std::map<unsigned int, svtkWeakPointer<svtkSynchronizedRenderWindows> >
  GlobalSynRenderWindowsMapType;
GlobalSynRenderWindowsMapType GlobalSynRenderWindowsMap;

void RenderRMI(
  void* svtkNotUsed(localArg), void* remoteArg, int remoteArgLength, int svtkNotUsed(remoteProcessId))
{
  svtkMultiProcessStream stream;
  stream.SetRawData(reinterpret_cast<unsigned char*>(remoteArg), remoteArgLength);
  unsigned int id = 0;
  stream >> id;
  GlobalSynRenderWindowsMapType::iterator iter = GlobalSynRenderWindowsMap.find(id);
  if (iter != GlobalSynRenderWindowsMap.end() && iter->second != nullptr &&
    iter->second->GetRenderWindow() != nullptr)
  {
    iter->second->GetRenderWindow()->Render();
  }
}
};

//----------------------------------------------------------------------------

svtkStandardNewMacro(svtkSynchronizedRenderWindows);
//----------------------------------------------------------------------------
svtkSynchronizedRenderWindows::svtkSynchronizedRenderWindows()
{
  this->Observer = svtkSynchronizedRenderWindows::svtkObserver::New();
  this->Observer->Target = this;

  this->RenderWindow = nullptr;
  this->ParallelController = nullptr;
  this->Identifier = 0;
  this->ParallelRendering = true;
  this->RenderEventPropagation = true;
  this->RootProcessId = 0;
}

//----------------------------------------------------------------------------
svtkSynchronizedRenderWindows::~svtkSynchronizedRenderWindows()
{
  this->SetIdentifier(0);

  this->Observer->Target = nullptr;

  this->SetRenderWindow(nullptr);
  this->SetParallelController(nullptr);
  this->Observer->Delete();
  this->Observer = nullptr;
}

//----------------------------------------------------------------------------
void svtkSynchronizedRenderWindows::SetIdentifier(unsigned int id)
{
  if (this->Identifier == id)
  {
    return;
  }

  if (this->Identifier != 0)
  {
    GlobalSynRenderWindowsMap.erase(this->Identifier);
    this->Identifier = 0;
  }

  GlobalSynRenderWindowsMapType::iterator iter = GlobalSynRenderWindowsMap.find(id);
  if (iter != GlobalSynRenderWindowsMap.end())
  {
    svtkErrorMacro("Identifier already in use: " << id);
    return;
  }

  this->Identifier = id;
  if (id > 0)
  {
    GlobalSynRenderWindowsMap[id] = this;
  }
}

//----------------------------------------------------------------------------
void svtkSynchronizedRenderWindows::SetRenderWindow(svtkRenderWindow* renWin)
{
  if (this->RenderWindow != renWin)
  {
    if (this->RenderWindow)
    {
      this->RenderWindow->RemoveObserver(this->Observer);
    }
    svtkSetObjectBodyMacro(RenderWindow, svtkRenderWindow, renWin);
    if (this->RenderWindow)
    {
      this->RenderWindow->AddObserver(svtkCommand::StartEvent, this->Observer);
      this->RenderWindow->AddObserver(svtkCommand::EndEvent, this->Observer);
      // this->RenderWindow->AddObserver(svtkCommand::AbortCheckEvent, this->Observer);
    }
  }
}

//----------------------------------------------------------------------------
void svtkSynchronizedRenderWindows::SetParallelController(svtkMultiProcessController* controller)
{
  if (this->ParallelController == controller)
  {
    return;
  }

  svtkSetObjectBodyMacro(ParallelController, svtkMultiProcessController, controller);

  if (controller)
  {
    // no harm in adding this multiple times.
    controller->AddRMI(::RenderRMI, nullptr, SYNC_RENDER_TAG);
  }
}

//----------------------------------------------------------------------------
void svtkSynchronizedRenderWindows::AbortRender()
{
  if (this->ParallelRendering && this->ParallelController &&
    this->ParallelController->GetLocalProcessId() == this->RootProcessId)
  {
    // TODO: trigger abort render message.
  }
}

//----------------------------------------------------------------------------
void svtkSynchronizedRenderWindows::HandleStartRender()
{
  if (!this->RenderWindow || !this->ParallelRendering || !this->ParallelController ||
    (!this->Identifier && this->RenderEventPropagation))
  {
    return;
  }

  if (this->ParallelController->GetLocalProcessId() == this->RootProcessId)
  {
    this->MasterStartRender();
  }
  else
  {
    this->SlaveStartRender();
  }
}

//----------------------------------------------------------------------------
void svtkSynchronizedRenderWindows::MasterStartRender()
{
  if (this->RenderEventPropagation)
  {
    svtkMultiProcessStream stream;
    stream << this->Identifier;

    std::vector<unsigned char> data;
    stream.GetRawData(data);

    this->ParallelController->TriggerRMIOnAllChildren(
      &data[0], static_cast<int>(data.size()), SYNC_RENDER_TAG);
  }

  RenderWindowInfo windowInfo;
  windowInfo.CopyFrom(this->RenderWindow);

  svtkMultiProcessStream stream;
  windowInfo.Save(stream);
  this->ParallelController->Broadcast(stream, this->RootProcessId);
}

//----------------------------------------------------------------------------
void svtkSynchronizedRenderWindows::SlaveStartRender()
{
  svtkMultiProcessStream stream;
  this->ParallelController->Broadcast(stream, this->RootProcessId);

  RenderWindowInfo windowInfo;
  windowInfo.Restore(stream);
  windowInfo.CopyTo(this->RenderWindow);
}

//----------------------------------------------------------------------------
void svtkSynchronizedRenderWindows::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Identifier: " << this->Identifier << endl;
  os << indent << "ParallelRendering: " << this->ParallelRendering << endl;
  os << indent << "RootProcessId: " << this->RootProcessId << endl;
  os << indent << "RenderEventPropagation: " << this->RenderEventPropagation << endl;

  os << indent << "RenderWindow: ";
  if (this->RenderWindow == nullptr)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->RenderWindow << endl;
  }
  if (this->ParallelController == nullptr)
  {
    os << "(none)" << endl;
  }
  else
  {
    os << this->ParallelController << endl;
  }
}

//----------------------------------------------------------------------------
// ********* INFO OBJECT METHODS ***************************
//----------------------------------------------------------------------------
void svtkSynchronizedRenderWindows::RenderWindowInfo::Save(svtkMultiProcessStream& stream)
{
  stream << 1208 << this->WindowSize[0] << this->WindowSize[1] << this->TileScale[0]
         << this->TileScale[1] << this->TileViewport[0] << this->TileViewport[1]
         << this->TileViewport[2] << this->TileViewport[3] << this->DesiredUpdateRate;
}

//----------------------------------------------------------------------------
bool svtkSynchronizedRenderWindows::RenderWindowInfo::Restore(svtkMultiProcessStream& stream)
{
  int tag;
  stream >> tag;
  if (tag != 1208)
  {
    return false;
  }

  stream >> this->WindowSize[0] >> this->WindowSize[1] >> this->TileScale[0] >>
    this->TileScale[1] >> this->TileViewport[0] >> this->TileViewport[1] >> this->TileViewport[2] >>
    this->TileViewport[3] >> this->DesiredUpdateRate;
  return true;
}

//----------------------------------------------------------------------------
void svtkSynchronizedRenderWindows::RenderWindowInfo::CopyFrom(svtkRenderWindow* win)
{
  this->WindowSize[0] = win->GetActualSize()[0];
  this->WindowSize[1] = win->GetActualSize()[1];
  this->DesiredUpdateRate = win->GetDesiredUpdateRate();
  win->GetTileScale(this->TileScale);
  win->GetTileViewport(this->TileViewport);
}

//----------------------------------------------------------------------------
void svtkSynchronizedRenderWindows::RenderWindowInfo::CopyTo(svtkRenderWindow* win)
{
  win->SetSize(this->WindowSize[0], this->WindowSize[1]);
  win->SetTileScale(this->TileScale);
  win->SetTileViewport(this->TileViewport);
  win->SetDesiredUpdateRate(this->DesiredUpdateRate);
}
