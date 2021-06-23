/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWidgetCallbackMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkWidgetCallbackMapper.h"
#include "svtkAbstractWidget.h"
#include "svtkCommand.h"
#include "svtkObjectFactory.h"
#include "svtkWidgetEventTranslator.h"
#include <map>

svtkStandardNewMacro(svtkWidgetCallbackMapper);

// Callbacks are stored as a pair of (Object,Method) in the map.
struct svtkCallbackPair
{
  svtkCallbackPair()
    : Widget(nullptr)
    , Callback(nullptr)
  {
  } // map requires empty constructor
  svtkCallbackPair(svtkAbstractWidget* w, svtkWidgetCallbackMapper::CallbackType f)
    : Widget(w)
    , Callback(f)
  {
  }

  svtkAbstractWidget* Widget;
  svtkWidgetCallbackMapper::CallbackType Callback;
};

// The map tracks the correspondence between widget events and callbacks
class svtkCallbackMap : public std::map<unsigned long, svtkCallbackPair>
{
public:
  typedef svtkCallbackMap CallbackMapType;
  typedef std::map<unsigned long, svtkCallbackPair>::iterator CallbackMapIterator;
};

//----------------------------------------------------------------------------
svtkWidgetCallbackMapper::svtkWidgetCallbackMapper()
{
  this->CallbackMap = new svtkCallbackMap;
  this->EventTranslator = nullptr;
}

//----------------------------------------------------------------------------
svtkWidgetCallbackMapper::~svtkWidgetCallbackMapper()
{
  delete this->CallbackMap;
  if (this->EventTranslator)
  {
    this->EventTranslator->Delete();
  }
}

//----------------------------------------------------------------------------
void svtkWidgetCallbackMapper::SetEventTranslator(svtkWidgetEventTranslator* t)
{
  if (this->EventTranslator != t)
  {
    if (this->EventTranslator)
    {
      this->EventTranslator->Delete();
    }
    this->EventTranslator = t;
    if (this->EventTranslator)
    {
      this->EventTranslator->Register(this);
    }

    this->Modified();
  }
}

//----------------------------------------------------------------------------
void svtkWidgetCallbackMapper::SetCallbackMethod(
  unsigned long SVTKEvent, unsigned long widgetEvent, svtkAbstractWidget* w, CallbackType f)
{
  this->EventTranslator->SetTranslation(SVTKEvent, widgetEvent);
  this->SetCallbackMethod(widgetEvent, w, f);
}

//----------------------------------------------------------------------------
void svtkWidgetCallbackMapper::SetCallbackMethod(unsigned long SVTKEvent, int modifier, char keyCode,
  int repeatCount, const char* keySym, unsigned long widgetEvent, svtkAbstractWidget* w,
  CallbackType f)
{
  this->EventTranslator->SetTranslation(
    SVTKEvent, modifier, keyCode, repeatCount, keySym, widgetEvent);
  this->SetCallbackMethod(widgetEvent, w, f);
}

//----------------------------------------------------------------------------
void svtkWidgetCallbackMapper::SetCallbackMethod(unsigned long SVTKEvent, svtkEventData* edata,
  unsigned long widgetEvent, svtkAbstractWidget* w, CallbackType f)
{
  this->EventTranslator->SetTranslation(SVTKEvent, edata, widgetEvent);
  this->SetCallbackMethod(widgetEvent, w, f);
}

//----------------------------------------------------------------------------
void svtkWidgetCallbackMapper::SetCallbackMethod(
  unsigned long widgetEvent, svtkAbstractWidget* w, CallbackType f)
{
  (*this->CallbackMap)[widgetEvent] = svtkCallbackPair(w, f);
}

//----------------------------------------------------------------------------
void svtkWidgetCallbackMapper::InvokeCallback(unsigned long widgetEvent)
{
  svtkCallbackMap::CallbackMapIterator iter = this->CallbackMap->find(widgetEvent);
  if (iter != this->CallbackMap->end())
  {
    svtkAbstractWidget* w = (*iter).second.Widget;
    CallbackType f = (*iter).second.Callback;
    (*f)(w);
  }
}

//----------------------------------------------------------------------------
void svtkWidgetCallbackMapper::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Event Translator: ";
  if (this->EventTranslator)
  {
    os << this->EventTranslator << "\n";
  }
  else
  {
    os << "(none)\n";
  }
}
