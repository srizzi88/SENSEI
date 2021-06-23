/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInteractorEventRecorder.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkInteractorEventRecorder.h"
#include "svtkCallbackCommand.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"

#include <cassert>
#include <locale>
#include <sstream>
#include <string>
#include <svtksys/FStream.hxx>
#include <svtksys/SystemTools.hxx>

svtkStandardNewMacro(svtkInteractorEventRecorder);

float svtkInteractorEventRecorder::StreamVersion = 1.1f;

//----------------------------------------------------------------------------
svtkInteractorEventRecorder::svtkInteractorEventRecorder()
{
  // take over the processing of keypress events from the superclass
  this->KeyPressCallbackCommand->SetCallback(svtkInteractorEventRecorder::ProcessCharEvent);
  this->KeyPressCallbackCommand->SetPassiveObserver(1); // get events first
  // processes delete events
  this->DeleteEventCallbackCommand = svtkCallbackCommand::New();
  this->DeleteEventCallbackCommand->SetClientData(this);
  this->DeleteEventCallbackCommand->SetCallback(svtkInteractorEventRecorder::ProcessDeleteEvent);

  this->EventCallbackCommand->SetCallback(svtkInteractorEventRecorder::ProcessEvents);
  this->EventCallbackCommand->SetPassiveObserver(1); // get events first

  this->FileName = nullptr;

  this->State = svtkInteractorEventRecorder::Start;
  this->InputStream = nullptr;
  this->OutputStream = nullptr;

  this->ReadFromInputString = 0;
  this->InputString = nullptr;
}

//----------------------------------------------------------------------------
svtkInteractorEventRecorder::~svtkInteractorEventRecorder()
{
  this->SetInteractor(nullptr);

  delete[] this->FileName;

  if (this->InputStream)
  {
    this->InputStream->clear();
    delete this->InputStream;
    this->InputStream = nullptr;
  }

  delete this->OutputStream;
  this->OutputStream = nullptr;

  delete[] this->InputString;
  this->InputString = nullptr;
  this->DeleteEventCallbackCommand->Delete();
}

//----------------------------------------------------------------------------
void svtkInteractorEventRecorder::SetEnabled(int enabling)
{
  if (!this->Interactor)
  {
    svtkErrorMacro(<< "The interactor must be set prior to enabling/disabling widget");
    return;
  }

  if (enabling) //----------------------------------------------------------
  {
    svtkDebugMacro(<< "Enabling widget");

    if (this->Enabled) // already enabled, just return
    {
      return;
    }

    this->Enabled = 1;

    // listen to any event
    svtkRenderWindowInteractor* i = this->Interactor;
    i->AddObserver(svtkCommand::AnyEvent, this->EventCallbackCommand, this->Priority);

    // Make sure that the interactor does not exit in response
    // to a StartEvent. The Interactor has code to allow others to handle
    // the event look of they want to
    i->HandleEventLoop = 1;

    this->InvokeEvent(svtkCommand::EnableEvent, nullptr);
  }

  else // disabling-----------------------------------------------------------
  {
    svtkDebugMacro(<< "Disabling widget");

    if (!this->Enabled) // already disabled, just return
    {
      return;
    }

    this->Enabled = 0;

    // don't listen for events any more
    this->Interactor->RemoveObserver(this->EventCallbackCommand);
    this->Interactor->HandleEventLoop = 0;

    this->InvokeEvent(svtkCommand::DisableEvent, nullptr);
  }
}

//----------------------------------------------------------------------------
void svtkInteractorEventRecorder::Record()
{
  if (this->State == svtkInteractorEventRecorder::Start)
  {
    if (!this->OutputStream) // need to open file
    {
      this->OutputStream = new svtksys::ofstream(this->FileName, ios::out);
      if (this->OutputStream->fail())
      {
        svtkErrorMacro(<< "Unable to open file: " << this->FileName);
        delete this->OutputStream;
        return;
      }

      // Use C locale. We don't want the user-defined locale when we write
      // float values.
      (*this->OutputStream).imbue(std::locale::classic());

      *this->OutputStream << "# StreamVersion " << svtkInteractorEventRecorder::StreamVersion
                          << "\n";
    }

    svtkDebugMacro(<< "Recording");
    this->State = svtkInteractorEventRecorder::Recording;
  }
}

//----------------------------------------------------------------------------
void svtkInteractorEventRecorder::Play()
{
  if (this->State == svtkInteractorEventRecorder::Start)
  {
    if (this->ReadFromInputString)
    {
      svtkDebugMacro(<< "Reading from InputString");
      size_t len = 0;
      if (this->InputString != nullptr)
      {
        len = strlen(this->InputString);
      }
      if (len == 0)
      {
        svtkErrorMacro(<< "No input string specified");
        return;
      }
      std::string inputStr(this->InputString, len);
      delete this->InputStream;
      this->InputStream = new std::istringstream(inputStr);
      if (this->InputStream->fail())
      {
        svtkErrorMacro(<< "Unable to read from string");
        delete this->InputStream;
        return;
      }
    }
    else
    {
      if (!this->InputStream) // need to open file
      {
        this->InputStream = new svtksys::ifstream(this->FileName, ios::in);
        if (this->InputStream->fail())
        {
          svtkErrorMacro(<< "Unable to open file: " << this->FileName);
          delete this->InputStream;
          return;
        }
      }
    }

    svtkDebugMacro(<< "Playing");
    this->State = svtkInteractorEventRecorder::Playing;

    // Read events and invoke them on the object in question
    char event[256] = {}, keySym[256] = {};
    int pos[2], ctrlKey, shiftKey, altKey, keyCode, repeatCount;
    float stream_version = 0.0f, tempf;
    std::string line;

    while (svtksys::SystemTools::GetLineFromStream(*this->InputStream, line))
    {
      std::istringstream iss(line);

      // Use classic locale, we don't want to parse float values with
      // user-defined locale.
      iss.imbue(std::locale::classic());

      iss.width(256);
      iss >> event;

      // Quick skip comment
      if (*event == '#')
      {
        // Parse the StreamVersion (not using >> since comment could be empty)
        // Expecting: # StreamVersion x.y

        if (strlen(line.c_str()) > 16 && !strncmp(line.c_str(), "# StreamVersion ", 16))
        {
          int res = sscanf(line.c_str() + 16, "%f", &tempf);
          if (res && res != EOF)
          {
            stream_version = tempf;
          }
        }
      }
      else
      {
        unsigned long ievent = svtkCommand::GetEventIdFromString(event);
        if (ievent != svtkCommand::NoEvent)
        {
          iss >> pos[0];
          iss >> pos[1];
          if (stream_version >= 1.1)
          {
            int m;
            iss >> m;
            shiftKey = (m & ModifierKey::ShiftKey) ? 1 : 0;
            ctrlKey = (m & ModifierKey::ControlKey) ? 1 : 0;
            altKey = (m & ModifierKey::AltKey) ? 1 : 0;
          }
          else
          {
            iss >> ctrlKey;
            iss >> shiftKey;
            altKey = 0;
          }
          iss >> keyCode;
          iss >> repeatCount;
          iss >> keySym;

          this->Interactor->SetEventPosition(pos);
          this->Interactor->SetControlKey(ctrlKey);
          this->Interactor->SetShiftKey(shiftKey);
          this->Interactor->SetAltKey(altKey);
          this->Interactor->SetKeyCode(static_cast<char>(keyCode));
          this->Interactor->SetRepeatCount(repeatCount);
          this->Interactor->SetKeySym(keySym);

          this->Interactor->InvokeEvent(ievent, nullptr);
        }
      }
      assert(iss.good() || iss.eof());
    }
  }

  this->State = svtkInteractorEventRecorder::Start;
}

//----------------------------------------------------------------------------
void svtkInteractorEventRecorder::Stop()
{
  this->State = svtkInteractorEventRecorder::Start;
  this->Modified();
}

void svtkInteractorEventRecorder::Rewind()
{
  if (!this->InputStream) // need to already have an open file
  {
    svtkGenericWarningMacro(<< "No input file opened to rewind...");
    return;
  }
  this->InputStream->clear();
  this->InputStream->seekg(0);
}

//----------------------------------------------------------------------------
// This adds the keypress event observer and the delete event observer
void svtkInteractorEventRecorder::SetInteractor(svtkRenderWindowInteractor* i)
{
  if (i == this->Interactor)
  {
    return;
  }

  // if we already have an Interactor then stop observing it
  if (this->Interactor)
  {
    this->SetEnabled(0); // disable the old interactor
    this->Interactor->RemoveObserver(this->KeyPressCallbackCommand);
    this->Interactor->RemoveObserver(this->DeleteEventCallbackCommand);
  }

  this->Interactor = i;

  // add observers for each of the events handled in ProcessEvents
  if (i)
  {
    i->AddObserver(svtkCommand::CharEvent, this->KeyPressCallbackCommand, this->Priority);
    i->AddObserver(svtkCommand::DeleteEvent, this->DeleteEventCallbackCommand, this->Priority);
  }

  this->Modified();
}

//----------------------------------------------------------------------------
void svtkInteractorEventRecorder::ProcessDeleteEvent(
  svtkObject* svtkNotUsed(object), unsigned long event, void* clientData, void* svtkNotUsed(callData))
{
  assert(event == svtkCommand::DeleteEvent);
  (void)event;
  svtkInteractorEventRecorder* self = reinterpret_cast<svtkInteractorEventRecorder*>(clientData);
  // if the interactor is being deleted then remove the event handlers
  self->SetInteractor(nullptr);
}

//----------------------------------------------------------------------------
void svtkInteractorEventRecorder::ProcessCharEvent(
  svtkObject* object, unsigned long event, void* clientData, void* svtkNotUsed(callData))
{
  assert(event == svtkCommand::CharEvent);
  (void)event;
  svtkInteractorEventRecorder* self = reinterpret_cast<svtkInteractorEventRecorder*>(clientData);
  svtkRenderWindowInteractor* rwi = static_cast<svtkRenderWindowInteractor*>(object);
  if (self->KeyPressActivation)
  {
    if (rwi->GetKeyCode() == self->KeyPressActivationValue)
    {
      if (!self->Enabled)
      {
        self->On();
      }
      else
      {
        self->Off();
      }
    } // event not aborted
  }   // if activation enabled
}

//----------------------------------------------------------------------------
void svtkInteractorEventRecorder::ProcessEvents(
  svtkObject* object, unsigned long event, void* clientData, void* svtkNotUsed(callData))
{
  svtkInteractorEventRecorder* self = reinterpret_cast<svtkInteractorEventRecorder*>(clientData);
  svtkRenderWindowInteractor* rwi = static_cast<svtkRenderWindowInteractor*>(object);

  // all events are processed
  if (self->State == svtkInteractorEventRecorder::Recording)
  {
    switch (event)
    {
      case svtkCommand::ModifiedEvent: // don't want these
        break;

      default:
        // A 'e' or a 'q' will stop the recording
        if (rwi->GetKeySym() &&
          (rwi->GetKeySym() == std::string("e") || rwi->GetKeySym() == std::string("q")))
        {
          self->Off();
        }
        else
        {
          int m = 0;
          if (rwi->GetShiftKey())
          {
            m |= ModifierKey::ShiftKey;
          }
          if (rwi->GetControlKey())
          {
            m |= ModifierKey::ControlKey;
          }
          if (rwi->GetAltKey())
          {
            m |= ModifierKey::AltKey;
          }
          self->WriteEvent(svtkCommand::GetStringFromEventId(event), rwi->GetEventPosition(), m,
            rwi->GetKeyCode(), rwi->GetRepeatCount(), rwi->GetKeySym());
        }
    }
    self->OutputStream->flush();
  }
}

//----------------------------------------------------------------------------
void svtkInteractorEventRecorder::WriteEvent(
  const char* event, int pos[2], int modifiers, int keyCode, int repeatCount, char* keySym)
{
  *this->OutputStream << event << " " << pos[0] << " " << pos[1] << " " << modifiers << " "
                      << keyCode << " " << repeatCount << " ";
  if (keySym)
  {
    *this->OutputStream << keySym << "\n";
  }
  else
  {
    *this->OutputStream << "0\n";
  }
}

//----------------------------------------------------------------------------
void svtkInteractorEventRecorder::ReadEvent() {}

//----------------------------------------------------------------------------
void svtkInteractorEventRecorder::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->FileName)
  {
    os << indent << "File Name: " << this->FileName << "\n";
  }

  os << indent << "ReadFromInputString: " << (this->ReadFromInputString ? "On\n" : "Off\n");

  if (this->InputString)
  {
    os << indent << "Input String: " << this->InputString << "\n";
  }
  else
  {
    os << indent << "Input String: (None)\n";
  }
}
