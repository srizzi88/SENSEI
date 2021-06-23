/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAbstractPicker.h"

#include "svtkObjectFactory.h"
#include "svtkRenderer.h"

// Construct object with initial tolerance of 1/40th of window. There are no
// pick methods and picking is performed from the renderer's actors.
svtkAbstractPicker::svtkAbstractPicker()
{
  this->Renderer = nullptr;

  this->SelectionPoint[0] = 0.0;
  this->SelectionPoint[1] = 0.0;
  this->SelectionPoint[2] = 0.0;

  this->PickPosition[0] = 0.0;
  this->PickPosition[1] = 0.0;
  this->PickPosition[2] = 0.0;

  this->PickFromList = 0;
  this->PickList = svtkPropCollection::New();
}

svtkAbstractPicker::~svtkAbstractPicker()
{
  this->PickList->Delete();
}

// Initialize the picking process.
void svtkAbstractPicker::Initialize()
{
  this->Renderer = nullptr;

  this->SelectionPoint[0] = 0.0;
  this->SelectionPoint[1] = 0.0;
  this->SelectionPoint[2] = 0.0;

  this->PickPosition[0] = 0.0;
  this->PickPosition[1] = 0.0;
  this->PickPosition[2] = 0.0;
}

// Initialize list of actors in pick list.
void svtkAbstractPicker::InitializePickList()
{
  this->Modified();
  this->PickList->RemoveAllItems();
}

// Add an actor to the pick list.
void svtkAbstractPicker::AddPickList(svtkProp* a)
{
  this->Modified();
  this->PickList->AddItem(a);
}

// Delete an actor from the pick list.
void svtkAbstractPicker::DeletePickList(svtkProp* a)
{
  this->Modified();
  this->PickList->RemoveItem(a);
}

void svtkAbstractPicker::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->PickFromList)
  {
    os << indent << "Picking from list\n";
  }
  else
  {
    os << indent << "Picking from renderer's prop list\n";
  }

  os << indent << "Renderer: " << this->Renderer << "\n";

  os << indent << "Selection Point: (" << this->SelectionPoint[0] << "," << this->SelectionPoint[1]
     << "," << this->SelectionPoint[2] << ")\n";

  os << indent << "Pick Position: (" << this->PickPosition[0] << "," << this->PickPosition[1] << ","
     << this->PickPosition[2] << ")\n";
}
