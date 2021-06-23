/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkButtonRepresentation.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkButtonRepresentation.h"
#include "svtkCommand.h"
#include "svtkObjectFactory.h"

//----------------------------------------------------------------------
svtkButtonRepresentation::svtkButtonRepresentation()
{
  this->NumberOfStates = 0;
  this->State = 0;
  this->HighlightState = svtkButtonRepresentation::HighlightNormal;
}

//----------------------------------------------------------------------
svtkButtonRepresentation::~svtkButtonRepresentation() = default;

//----------------------------------------------------------------------
// Implement the modulo behavior in this method
void svtkButtonRepresentation::SetState(int state)
{
  if (this->NumberOfStates < 1)
  {
    return;
  }

  int remain = state % this->NumberOfStates;
  if (remain < 0)
  {
    remain += this->NumberOfStates;
  }
  state = remain;

  // Modify if necessary
  if (state != this->State)
  {
    this->State = state;
    this->Modified();
  }
}

//----------------------------------------------------------------------
void svtkButtonRepresentation::NextState()
{
  this->SetState(this->State + 1);
}

//----------------------------------------------------------------------
void svtkButtonRepresentation::PreviousState()
{
  this->SetState(this->State - 1);
}

//----------------------------------------------------------------------
void svtkButtonRepresentation::Highlight(int state)
{
  int newState;
  if (state == svtkButtonRepresentation::HighlightNormal)
  {
    newState = svtkButtonRepresentation::HighlightNormal;
  }
  else if (state == svtkButtonRepresentation::HighlightHovering)
  {
    newState = svtkButtonRepresentation::HighlightHovering;
  }
  else // if ( state == svtkButtonRepresentation::HighlightSelecting )
  {
    newState = svtkButtonRepresentation::HighlightSelecting;
  }

  if (newState != this->HighlightState)
  {
    this->HighlightState = newState;
    this->InvokeEvent(svtkCommand::HighlightEvent, &(this->HighlightState));
    this->Modified();
  }
}

//----------------------------------------------------------------------
void svtkButtonRepresentation::ShallowCopy(svtkProp* prop)
{
  svtkButtonRepresentation* rep = svtkButtonRepresentation::SafeDownCast(prop);

  if (rep)
  {
    this->NumberOfStates = rep->NumberOfStates;
    this->State = rep->State;
    this->HighlightState = rep->HighlightState;
  }

  this->Superclass::ShallowCopy(prop);
}

//----------------------------------------------------------------------
void svtkButtonRepresentation::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Number Of States: " << this->NumberOfStates << "\n";
  os << indent << "State: " << this->State << "\n";
  os << indent << "Highlight State: " << this->HighlightState << "\n";
}
