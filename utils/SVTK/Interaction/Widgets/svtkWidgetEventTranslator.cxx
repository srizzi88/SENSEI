/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWidgetEventTranslator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkWidgetEventTranslator.h"
#include "svtkAbstractWidget.h"
#include "svtkCallbackCommand.h"
#include "svtkCommand.h"
#include "svtkEvent.h"
#include "svtkEventData.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"
#include "svtkSmartPointer.h"
#include "svtkWidgetEvent.h"
#include <list>
#include <map>

svtkStandardNewMacro(svtkWidgetEventTranslator);

// This is what is place in the list
struct EventItem
{
  svtkSmartPointer<svtkEvent> SVTKEvent;
  unsigned long WidgetEvent;
  svtkEventData* EventData = nullptr;
  bool HasData = false;

  EventItem(svtkEvent* e, unsigned long we)
  {
    this->SVTKEvent = e;
    this->WidgetEvent = we;
    this->HasData = false;
  }
  EventItem(svtkEventData* edata, unsigned long we)
  {
    this->EventData = edata;
    this->EventData->Register(nullptr);
    this->WidgetEvent = we;
    this->HasData = true;
  }
  ~EventItem()
  {
    if (this->HasData && this->EventData)
    {
      this->EventData->UnRegister(nullptr);
      this->EventData = nullptr;
    }
  }

  EventItem(const EventItem& v)
  {
    this->SVTKEvent = v.SVTKEvent;
    this->WidgetEvent = v.WidgetEvent;
    this->HasData = v.HasData;
    this->EventData = v.EventData;
    if (this->HasData && this->EventData)
    {
      this->EventData->Register(nullptr);
    }
  }

private:
  EventItem() = delete;
};

// A list of events
struct EventList : public std::list<EventItem>
{
  unsigned long find(unsigned long SVTKEvent)
  {
    std::list<EventItem>::iterator liter = this->begin();
    for (; liter != this->end(); ++liter)
    {
      if (SVTKEvent == liter->SVTKEvent->GetEventId())
      {
        return liter->WidgetEvent;
      }
    }
    return svtkWidgetEvent::NoEvent;
  }

  unsigned long find(svtkEvent* SVTKEvent)
  {
    std::list<EventItem>::iterator liter = this->begin();
    for (; liter != this->end(); ++liter)
    {
      if (*SVTKEvent == liter->SVTKEvent)
      {
        return liter->WidgetEvent;
      }
    }
    return svtkWidgetEvent::NoEvent;
  }

  unsigned long find(svtkEventData* edata)
  {
    std::list<EventItem>::iterator liter = this->begin();
    for (; liter != this->end(); ++liter)
    {
      if (liter->HasData && *edata == *liter->EventData)
      {
        return liter->WidgetEvent;
      }
    }
    return svtkWidgetEvent::NoEvent;
  }

  // Remove a mapping
  int Remove(svtkEvent* SVTKEvent)
  {
    std::list<EventItem>::iterator liter = this->begin();
    for (; liter != this->end(); ++liter)
    {
      if (*SVTKEvent == liter->SVTKEvent)
      {
        this->erase(liter);
        return 1;
      }
    }
    return 0;
  }
  int Remove(svtkEventData* edata)
  {
    std::list<EventItem>::iterator liter = this->begin();
    for (; liter != this->end(); ++liter)
    {
      if (liter->HasData && *edata == *liter->EventData)
      {
        this->erase(liter);
        return 1;
      }
    }
    return 0;
  }
};

// A STL map used to translate SVTK events into lists of events. The reason
// that we have this list is because of the modifiers on the event. The
// SVTK event id maps to the list, and then comparisons are done to
// determine which event matches.
class svtkEventMap : public std::map<unsigned long, EventList>
{
};
typedef std::map<unsigned long, EventList>::iterator EventMapIterator;

//----------------------------------------------------------------------------
svtkWidgetEventTranslator::svtkWidgetEventTranslator()
{
  this->EventMap = new svtkEventMap;
  this->Event = svtkEvent::New();
}

//----------------------------------------------------------------------------
svtkWidgetEventTranslator::~svtkWidgetEventTranslator()
{
  delete this->EventMap;
  this->Event->Delete();
}

//----------------------------------------------------------------------------
void svtkWidgetEventTranslator::SetTranslation(unsigned long SVTKEvent, unsigned long widgetEvent)
{
  svtkSmartPointer<svtkEvent> e = svtkSmartPointer<svtkEvent>::New();
  e->SetEventId(SVTKEvent); // default modifiers
  if (widgetEvent != svtkWidgetEvent::NoEvent)
  {
    (*this->EventMap)[SVTKEvent].push_back(EventItem(e, widgetEvent));
  }
  else
  {
    this->RemoveTranslation(e);
  }
}

//----------------------------------------------------------------------------
void svtkWidgetEventTranslator::SetTranslation(const char* SVTKEvent, const char* widgetEvent)
{
  this->SetTranslation(
    svtkCommand::GetEventIdFromString(SVTKEvent), svtkWidgetEvent::GetEventIdFromString(widgetEvent));
}

//----------------------------------------------------------------------------
void svtkWidgetEventTranslator::SetTranslation(unsigned long SVTKEvent, int modifier, char keyCode,
  int repeatCount, const char* keySym, unsigned long widgetEvent)
{
  svtkSmartPointer<svtkEvent> e = svtkSmartPointer<svtkEvent>::New();
  e->SetEventId(SVTKEvent);
  e->SetModifier(modifier);
  e->SetKeyCode(keyCode);
  e->SetRepeatCount(repeatCount);
  e->SetKeySym(keySym);
  if (widgetEvent != svtkWidgetEvent::NoEvent)
  {
    (*this->EventMap)[SVTKEvent].push_back(EventItem(e, widgetEvent));
  }
  else
  {
    this->RemoveTranslation(e);
  }
}

void svtkWidgetEventTranslator::SetTranslation(
  unsigned long SVTKEvent, svtkEventData* edata, unsigned long widgetEvent)
{
  if (widgetEvent != svtkWidgetEvent::NoEvent)
  {
    (*this->EventMap)[SVTKEvent].push_back(EventItem(edata, widgetEvent));
  }
  else
  {
    this->RemoveTranslation(edata);
  }
}

//----------------------------------------------------------------------------
void svtkWidgetEventTranslator::SetTranslation(svtkEvent* SVTKEvent, unsigned long widgetEvent)
{
  if (widgetEvent != svtkWidgetEvent::NoEvent)
  {
    (*this->EventMap)[SVTKEvent->GetEventId()].push_back(EventItem(SVTKEvent, widgetEvent));
  }
  else
  {
    this->RemoveTranslation(SVTKEvent);
  }
}

//----------------------------------------------------------------------------
unsigned long svtkWidgetEventTranslator::GetTranslation(unsigned long SVTKEvent)
{
  EventMapIterator iter = this->EventMap->find(SVTKEvent);
  if (iter != this->EventMap->end())
  {
    EventList& elist = (*iter).second;
    return elist.find(SVTKEvent);
  }
  else
  {
    return svtkWidgetEvent::NoEvent;
  }
}

//----------------------------------------------------------------------------
const char* svtkWidgetEventTranslator::GetTranslation(const char* SVTKEvent)
{
  return svtkWidgetEvent::GetStringFromEventId(
    this->GetTranslation(svtkCommand::GetEventIdFromString(SVTKEvent)));
}

//----------------------------------------------------------------------------
unsigned long svtkWidgetEventTranslator::GetTranslation(
  unsigned long SVTKEvent, int modifier, char keyCode, int repeatCount, const char* keySym)
{
  EventMapIterator iter = this->EventMap->find(SVTKEvent);
  if (iter != this->EventMap->end())
  {
    this->Event->SetEventId(SVTKEvent);
    this->Event->SetModifier(modifier);
    this->Event->SetKeyCode(keyCode);
    this->Event->SetRepeatCount(repeatCount);
    this->Event->SetKeySym(keySym);
    EventList& elist = (*iter).second;
    return elist.find(this->Event);
  }
  return svtkWidgetEvent::NoEvent;
}

//----------------------------------------------------------------------------
unsigned long svtkWidgetEventTranslator::GetTranslation(unsigned long, svtkEventData* edata)
{
  EventMapIterator iter = this->EventMap->find(edata->GetType());
  if (iter != this->EventMap->end())
  {
    EventList& elist = (*iter).second;
    return elist.find(edata);
  }
  return svtkWidgetEvent::NoEvent;
}

//----------------------------------------------------------------------------
unsigned long svtkWidgetEventTranslator::GetTranslation(svtkEvent* SVTKEvent)
{
  EventMapIterator iter = this->EventMap->find(SVTKEvent->GetEventId());
  if (iter != this->EventMap->end())
  {
    EventList& elist = (*iter).second;
    return elist.find(SVTKEvent);
  }
  else
  {
    return svtkWidgetEvent::NoEvent;
  }
}

//----------------------------------------------------------------------------
int svtkWidgetEventTranslator::RemoveTranslation(
  unsigned long SVTKEvent, int modifier, char keyCode, int repeatCount, const char* keySym)
{
  svtkSmartPointer<svtkEvent> e = svtkSmartPointer<svtkEvent>::New();
  e->SetEventId(SVTKEvent);
  e->SetModifier(modifier);
  e->SetKeyCode(keyCode);
  e->SetRepeatCount(repeatCount);
  e->SetKeySym(keySym);
  return this->RemoveTranslation(e);
}

//----------------------------------------------------------------------------
int svtkWidgetEventTranslator::RemoveTranslation(svtkEvent* e)
{
  EventMapIterator iter = this->EventMap->find(e->GetEventId());
  int numTranslationsRemoved = 0;
  if (iter != this->EventMap->end())
  {
    while (iter->second.Remove(e))
    {
      ++numTranslationsRemoved;
      iter = this->EventMap->find(e->GetEventId());
      if (iter == this->EventMap->end())
      {
        break;
      }
    }
  }

  return numTranslationsRemoved;
}

//----------------------------------------------------------------------------
int svtkWidgetEventTranslator::RemoveTranslation(svtkEventData* edata)
{
  EventMapIterator iter = this->EventMap->find(edata->GetType());
  int numTranslationsRemoved = 0;
  if (iter != this->EventMap->end())
  {
    while (iter->second.Remove(edata))
    {
      ++numTranslationsRemoved;
      iter = this->EventMap->find(edata->GetType());
      if (iter == this->EventMap->end())
      {
        break;
      }
    }
  }

  return numTranslationsRemoved;
}

//----------------------------------------------------------------------------
int svtkWidgetEventTranslator::RemoveTranslation(unsigned long SVTKEvent)
{
  svtkSmartPointer<svtkEvent> e = svtkSmartPointer<svtkEvent>::New();
  e->SetEventId(SVTKEvent);
  return this->RemoveTranslation(e);
}

//----------------------------------------------------------------------------
int svtkWidgetEventTranslator::RemoveTranslation(const char* SVTKEvent)
{
  svtkSmartPointer<svtkEvent> e = svtkSmartPointer<svtkEvent>::New();
  e->SetEventId(svtkCommand::GetEventIdFromString(SVTKEvent));
  return this->RemoveTranslation(e);
}

//----------------------------------------------------------------------------
void svtkWidgetEventTranslator::ClearEvents()
{
  EventMapIterator iter = this->EventMap->begin();
  for (; iter != this->EventMap->end(); ++iter)
  {
    EventList& elist = (*iter).second;
    elist.clear();
  }
  this->EventMap->clear();
}

//----------------------------------------------------------------------------
void svtkWidgetEventTranslator::AddEventsToInteractor(
  svtkRenderWindowInteractor* i, svtkCallbackCommand* command, float priority)
{
  EventMapIterator iter = this->EventMap->begin();
  for (; iter != this->EventMap->end(); ++iter)
  {
    i->AddObserver((*iter).first, command, priority);
  }
}

//----------------------------------------------------------------------------
void svtkWidgetEventTranslator::AddEventsToParent(
  svtkAbstractWidget* w, svtkCallbackCommand* command, float priority)
{
  EventMapIterator iter = this->EventMap->begin();
  for (; iter != this->EventMap->end(); ++iter)
  {
    w->AddObserver((*iter).first, command, priority);
  }
}

//----------------------------------------------------------------------------
void svtkWidgetEventTranslator::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  // List all the events and their translations
  os << indent << "Event Table:\n";
  EventMapIterator iter = this->EventMap->begin();
  for (; iter != this->EventMap->end(); ++iter)
  {
    EventList& elist = (*iter).second;
    std::list<EventItem>::iterator liter = elist.begin();
    for (; liter != elist.end(); ++liter)
    {
      os << "SVTKEvent(" << svtkCommand::GetStringFromEventId(liter->SVTKEvent->GetEventId()) << ","
         << liter->SVTKEvent->GetModifier() << "," << liter->SVTKEvent->GetKeyCode() << ","
         << liter->SVTKEvent->GetRepeatCount() << ",";
      os << (liter->SVTKEvent->GetKeySym() ? liter->SVTKEvent->GetKeySym() : "(any)");
      os << ") maps to " << svtkWidgetEvent::GetStringFromEventId(liter->WidgetEvent) << "\n";
    }
  }
}
