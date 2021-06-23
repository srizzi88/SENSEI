/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWidgetEvent.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkWidgetEvent.h"
#include "svtkObjectFactory.h"

// this list should only contain the initial, contiguous
// set of events and should not include UserEvent
static const char* svtkWidgetEventStrings[] = {
  "NoEvent",
  "Select",
  "EndSelect",
  "Delete",
  "Translate",
  "EndTranslate",
  "Scale",
  "EndScale",
  "Resize",
  "EndResize",
  "Rotate",
  "EndRotate",
  "Move",
  "SizeHandles",
  "AddPoint",
  "AddFinalPoint",
  "Completed",
  "TimedOut",
  "ModifyEvent",
  "Reset",
  nullptr,
};

svtkStandardNewMacro(svtkWidgetEvent);

//----------------------------------------------------------------------
const char* svtkWidgetEvent::GetStringFromEventId(unsigned long event)
{
  static unsigned long numevents = 0;

  // find length of table
  if (!numevents)
  {
    while (svtkWidgetEventStrings[numevents] != nullptr)
    {
      numevents++;
    }
  }

  if (event < numevents)
  {
    return svtkWidgetEventStrings[event];
  }
  else
  {
    return "NoEvent";
  }
}

//----------------------------------------------------------------------
unsigned long svtkWidgetEvent::GetEventIdFromString(const char* event)
{
  unsigned long i;

  for (i = 0; svtkWidgetEventStrings[i] != nullptr; i++)
  {
    if (!strcmp(svtkWidgetEventStrings[i], event))
    {
      return i;
    }
  }
  return svtkWidgetEvent::NoEvent;
}

//----------------------------------------------------------------------
void svtkWidgetEvent::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
