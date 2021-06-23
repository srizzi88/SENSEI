/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEvent.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkEvent.h"
#include "svtkCommand.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindowInteractor.h"

svtkStandardNewMacro(svtkEvent);

svtkEvent::svtkEvent()
{
  this->Modifier = svtkEvent::AnyModifier;
  this->KeyCode = 0;
  this->RepeatCount = 0;
  this->KeySym = nullptr;
  this->EventId = svtkCommand::NoEvent;
}

svtkEvent::~svtkEvent()
{
  delete[] this->KeySym;
}

// Comparison against event with no modifiers
bool svtkEvent::operator==(unsigned long SVTKEvent)
{
  if (this->EventId == SVTKEvent)
  {
    return true;
  }
  else
  {
    return false;
  }
}

// Comparison against event with modifiers
bool svtkEvent::operator==(svtkEvent* e)
{
  if (this->EventId != e->EventId)
  {
    return false;
  }
  if (this->Modifier != svtkEvent::AnyModifier && e->Modifier != svtkEvent::AnyModifier &&
    this->Modifier != e->Modifier)
  {
    return false;
  }
  if (this->KeyCode != '\0' && e->KeyCode != '\0' && this->KeyCode != e->KeyCode)
  {
    return false;
  }
  if (this->RepeatCount != 0 && e->RepeatCount != 0 && this->RepeatCount != e->RepeatCount)
  {
    return false;
  }
  if (this->KeySym != nullptr && e->KeySym != nullptr && strcmp(this->KeySym, e->KeySym) != 0)
  {
    return false;
  }

  return true;
}

//----------------------------------------------------------------------------
int svtkEvent::GetModifier(svtkRenderWindowInteractor* i)
{
  int modifier = 0;
  modifier |= (i->GetShiftKey() ? svtkEvent::ShiftModifier : 0);
  modifier |= (i->GetControlKey() ? svtkEvent::ControlModifier : 0);
  modifier |= (i->GetAltKey() ? svtkEvent::AltModifier : 0);

  return modifier;
}

//----------------------------------------------------------------------------
void svtkEvent::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  // List all the events and their translations
  os << indent << "Event Id: " << this->EventId << "\n";

  os << indent << "Modifier: ";
  if (this->Modifier == -1)
  {
    os << "Any\n";
  }
  else if (this->Modifier == 0)
  {
    os << "None\n";
  }
  else
  {
    os << this->Modifier << "\n";
  }

  os << indent << "Key Code: ";
  if (this->KeyCode == 0)
  {
    os << "Any\n";
  }
  else
  {
    os << this->KeyCode << "\n";
  }

  os << indent << "Repeat Count: ";
  if (this->RepeatCount == 0)
  {
    os << "Any\n";
  }
  else
  {
    os << this->RepeatCount << "\n";
  }

  os << indent << "Key Sym: ";
  if (this->KeySym == nullptr)
  {
    os << "Any\n";
  }
  else
  {
    os << this->KeySym << "\n";
  }
}
