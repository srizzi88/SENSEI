/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWidgetSet.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkWidgetSet.h"

#include "svtkObjectFactory.h"
#include "svtkParallelopipedWidget.h" // REMOVE

svtkStandardNewMacro(svtkWidgetSet);

//----------------------------------------------------------------------
svtkWidgetSet::svtkWidgetSet() = default;

//----------------------------------------------------------------------
svtkWidgetSet::~svtkWidgetSet()
{
  for (WidgetIteratorType it = this->Widget.begin(); it != this->Widget.end(); ++it)
  {
    (*it)->UnRegister(this);
  }
}

//----------------------------------------------------------------------
void svtkWidgetSet::SetEnabled(svtkTypeBool enabling)
{
  for (WidgetIteratorType it = this->Widget.begin(); it != this->Widget.end(); ++it)
  {
    (*it)->SetEnabled(enabling);
  }
}

//----------------------------------------------------------------------
void svtkWidgetSet::AddWidget(svtkAbstractWidget* w)
{
  for (unsigned int i = 0; i < this->Widget.size(); i++)
  {
    if (this->Widget[i] == w)
    {
      return;
    }
  }

  this->Widget.push_back(w);
  w->Register(this);

  // TODO : Won't be necessary if we move this to the AbstractWidget.. superclass
  static_cast<svtkParallelopipedWidget*>(w)->WidgetSet = this;
}

//----------------------------------------------------------------------
void svtkWidgetSet::RemoveWidget(svtkAbstractWidget* w)
{
  for (WidgetIteratorType it = this->Widget.begin(); it != this->Widget.end(); ++it)
  {
    if (*it == w)
    {
      this->Widget.erase(it);
      static_cast<svtkParallelopipedWidget*>(w)->WidgetSet = nullptr;
      w->UnRegister(this);
      break;
    }
  }
}

//----------------------------------------------------------------------
svtkAbstractWidget* svtkWidgetSet::GetNthWidget(unsigned int i)
{
  return this->Widget[i];
}

//----------------------------------------------------------------------
unsigned int svtkWidgetSet::GetNumberOfWidgets()
{
  return static_cast<unsigned int>(this->Widget.size());
}

//----------------------------------------------------------------------
void svtkWidgetSet::PrintSelf(ostream& os, svtkIndent indent)
{
  // Superclass typedef defined in svtkTypeMacro() found in svtkSetGet.h
  this->Superclass::PrintSelf(os, indent);
}
