/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractPropPicker.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAbstractPropPicker.h"

#include "svtkActor.h"
#include "svtkActor2D.h"
#include "svtkAssembly.h"
#include "svtkAssemblyNode.h"
#include "svtkAssemblyPath.h"
#include "svtkObjectFactory.h"
#include "svtkPropAssembly.h"
#include "svtkVolume.h"

svtkCxxSetObjectMacro(svtkAbstractPropPicker, Path, svtkAssemblyPath);

svtkAbstractPropPicker::svtkAbstractPropPicker()
{
  this->Path = nullptr;
}

svtkAbstractPropPicker::~svtkAbstractPropPicker()
{
  if (this->Path)
  {
    this->Path->Delete();
  }
}

// set up for a pick
void svtkAbstractPropPicker::Initialize()
{
  this->svtkAbstractPicker::Initialize();
  if (this->Path)
  {
    this->Path->Delete();
    this->Path = nullptr;
  }
}

//----------------------------------------------------------------------------
svtkProp* svtkAbstractPropPicker::GetViewProp()
{
  if (this->Path != nullptr)
  {
    return this->Path->GetFirstNode()->GetViewProp();
  }
  else
  {
    return nullptr;
  }
}

svtkProp3D* svtkAbstractPropPicker::GetProp3D()
{
  if (this->Path != nullptr)
  {
    svtkProp* prop = this->Path->GetFirstNode()->GetViewProp();
    return svtkProp3D::SafeDownCast(prop);
  }
  else
  {
    return nullptr;
  }
}

svtkActor* svtkAbstractPropPicker::GetActor()
{
  if (this->Path != nullptr)
  {
    svtkProp* prop = this->Path->GetFirstNode()->GetViewProp();
    return svtkActor::SafeDownCast(prop);
  }
  else
  {
    return nullptr;
  }
}

svtkActor2D* svtkAbstractPropPicker::GetActor2D()
{
  if (this->Path != nullptr)
  {
    svtkProp* prop = this->Path->GetFirstNode()->GetViewProp();
    return svtkActor2D::SafeDownCast(prop);
  }
  else
  {
    return nullptr;
  }
}

svtkVolume* svtkAbstractPropPicker::GetVolume()
{
  if (this->Path != nullptr)
  {
    svtkProp* prop = this->Path->GetFirstNode()->GetViewProp();
    return svtkVolume::SafeDownCast(prop);
  }
  else
  {
    return nullptr;
  }
}

svtkAssembly* svtkAbstractPropPicker::GetAssembly()
{
  if (this->Path != nullptr)
  {
    svtkProp* prop = this->Path->GetFirstNode()->GetViewProp();
    return svtkAssembly::SafeDownCast(prop);
  }
  else
  {
    return nullptr;
  }
}

svtkPropAssembly* svtkAbstractPropPicker::GetPropAssembly()
{
  if (this->Path != nullptr)
  {
    svtkProp* prop = this->Path->GetFirstNode()->GetViewProp();
    return svtkPropAssembly::SafeDownCast(prop);
  }
  else
  {
    return nullptr;
  }
}

void svtkAbstractPropPicker::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->Path)
  {
    os << indent << "Path: " << this->Path << endl;
  }
  else
  {
    os << indent << "Path: (none)" << endl;
  }
}
