/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProp.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkProp.h"
#include "svtkAssemblyPaths.h"
#include "svtkCommand.h"
#include "svtkInformation.h"
#include "svtkInformationDoubleVectorKey.h"
#include "svtkInformationIntegerKey.h"
#include "svtkInformationIterator.h"
#include "svtkInformationKey.h"
#include "svtkObjectFactory.h"
#include "svtkShaderProperty.h"
#include <cassert>

svtkCxxSetObjectMacro(svtkProp, PropertyKeys, svtkInformation);

svtkInformationKeyMacro(svtkProp, GeneralTextureUnit, Integer);
svtkInformationKeyMacro(svtkProp, GeneralTextureTransform, DoubleVector);

//----------------------------------------------------------------------------
// Creates an Prop with the following defaults: visibility on.
svtkProp::svtkProp()
{
  this->Visibility = 1; // ON

  this->Pickable = 1;
  this->Dragable = 1;

  this->UseBounds = true;

  this->AllocatedRenderTime = 10.0;
  this->EstimatedRenderTime = 0.0;
  this->RenderTimeMultiplier = 1.0;

  this->Paths = nullptr;

  this->NumberOfConsumers = 0;
  this->Consumers = nullptr;

  this->PropertyKeys = nullptr;

  this->ShaderProperty = nullptr;
}

//----------------------------------------------------------------------------
svtkProp::~svtkProp()
{
  if (this->Paths)
  {
    this->Paths->Delete();
  }

  delete[] this->Consumers;

  if (this->PropertyKeys != nullptr)
  {
    this->PropertyKeys->Delete();
  }

  if (this->ShaderProperty)
  {
    this->ShaderProperty->UnRegister(this);
  }
}

//----------------------------------------------------------------------------
// This method is invoked if the prop is picked.
void svtkProp::Pick()
{
  this->InvokeEvent(svtkCommand::PickEvent, nullptr);
}

//----------------------------------------------------------------------------
// Shallow copy of svtkProp.
void svtkProp::ShallowCopy(svtkProp* prop)
{
  this->Visibility = prop->GetVisibility();
  this->Pickable = prop->GetPickable();
  this->Dragable = prop->GetDragable();
  this->SetShaderProperty(prop->GetShaderProperty());
}

//----------------------------------------------------------------------------
void svtkProp::InitPathTraversal()
{
  if (this->Paths == nullptr)
  {
    this->Paths = svtkAssemblyPaths::New();
    svtkAssemblyPath* path = svtkAssemblyPath::New();
    path->AddNode(this, nullptr);
    this->BuildPaths(this->Paths, path);
    path->Delete();
  }
  this->Paths->InitTraversal();
}

//----------------------------------------------------------------------------
svtkAssemblyPath* svtkProp::GetNextPath()
{
  if (!this->Paths)
  {
    return nullptr;
  }
  return this->Paths->GetNextItem();
}

//----------------------------------------------------------------------------
// This method is used in conjunction with the assembly object to build a copy
// of the assembly hierarchy. This hierarchy can then be traversed for
// rendering, picking or other operations.
void svtkProp::BuildPaths(svtkAssemblyPaths* paths, svtkAssemblyPath* path)
{
  // This is a leaf node in the assembly hierarchy so we
  // copy the path in preparation to assingning it to paths.
  svtkAssemblyPath* childPath = svtkAssemblyPath::New();
  childPath->ShallowCopy(path);

  // We can add this path to the list of paths
  paths->AddItem(childPath);
  childPath->Delete(); // okay, reference counting
}

//----------------------------------------------------------------------------
void svtkProp::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Dragable: " << (this->Dragable ? "On\n" : "Off\n");
  os << indent << "Pickable: " << (this->Pickable ? "On\n" : "Off\n");

  os << indent << "AllocatedRenderTime: " << this->AllocatedRenderTime << endl;
  os << indent << "EstimatedRenderTime: " << this->EstimatedRenderTime << endl;
  os << indent << "NumberOfConsumers: " << this->NumberOfConsumers << endl;
  os << indent << "RenderTimeMultiplier: " << this->RenderTimeMultiplier << endl;
  os << indent << "Visibility: " << (this->Visibility ? "On\n" : "Off\n");

  os << indent << "PropertyKeys: ";
  if (this->PropertyKeys != nullptr)
  {
    this->PropertyKeys->PrintSelf(os, indent);
    os << endl;
  }
  else
  {
    os << "none." << endl;
  }

  os << indent << "useBounds: " << this->UseBounds << endl;
}

//----------------------------------------------------------------------------
void svtkProp::AddConsumer(svtkObject* c)
{
  // make sure it isn't already there
  if (this->IsConsumer(c))
  {
    return;
  }
  // add it to the list, reallocate memory
  svtkObject** tmp = this->Consumers;
  this->NumberOfConsumers++;
  this->Consumers = new svtkObject*[this->NumberOfConsumers];
  for (int i = 0; i < (this->NumberOfConsumers - 1); i++)
  {
    this->Consumers[i] = tmp[i];
  }
  this->Consumers[this->NumberOfConsumers - 1] = c;
  // free old memory
  delete[] tmp;
}

//----------------------------------------------------------------------------
void svtkProp::RemoveConsumer(svtkObject* c)
{
  // make sure it is already there
  if (!this->IsConsumer(c))
  {
    return;
  }
  // remove it from the list, reallocate memory
  svtkObject** tmp = this->Consumers;
  this->NumberOfConsumers--;
  this->Consumers = new svtkObject*[this->NumberOfConsumers];
  int cnt = 0;
  int i;
  for (i = 0; i <= this->NumberOfConsumers; i++)
  {
    if (tmp[i] != c)
    {
      this->Consumers[cnt] = tmp[i];
      cnt++;
    }
  }
  // free old memory
  delete[] tmp;
}

//----------------------------------------------------------------------------
int svtkProp::IsConsumer(svtkObject* c)
{
  int i;
  for (i = 0; i < this->NumberOfConsumers; i++)
  {
    if (this->Consumers[i] == c)
    {
      return 1;
    }
  }
  return 0;
}

//----------------------------------------------------------------------------
svtkObject* svtkProp::GetConsumer(int i)
{
  if (i >= this->NumberOfConsumers)
  {
    return nullptr;
  }
  return this->Consumers[i];
}

// ----------------------------------------------------------------------------
// Description:
// Tells if the prop has all the required keys.
// \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
bool svtkProp::HasKeys(svtkInformation* requiredKeys)
{
  bool result = requiredKeys == nullptr;
  if (!result)
  {
    svtkInformationIterator* it = svtkInformationIterator::New();
    it->SetInformation(requiredKeys);
    it->GoToFirstItem();
    result = true;
    while (result && !it->IsDoneWithTraversal())
    {
      svtkInformationKey* k = it->GetCurrentKey();
      result = this->PropertyKeys != nullptr && this->PropertyKeys->Has(k);
      it->GoToNextItem();
    }
    it->Delete();
  }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Render the opaque geometry only if the prop has all the requiredKeys.
// This is recursive for composite props like svtkAssembly.
// An implementation is provided in svtkProp but each composite prop
// must override it.
// It returns if the rendering was performed.
// \pre v_exists: v!=0
// \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
bool svtkProp::RenderFilteredOpaqueGeometry(svtkViewport* v, svtkInformation* requiredKeys)
{
  assert("pre: v_exists" && v != nullptr);
  bool result;
  if (this->HasKeys(requiredKeys))
  {
    result = this->RenderOpaqueGeometry(v) == 1;
  }
  else
  {
    result = false;
  }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Render the translucent polygonal geometry only if the prop has all the
// requiredKeys.
// This is recursive for composite props like svtkAssembly.
// An implementation is provided in svtkProp but each composite prop
// must override it.
// It returns if the rendering was performed.
// \pre v_exists: v!=0
// \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
bool svtkProp::RenderFilteredTranslucentPolygonalGeometry(
  svtkViewport* v, svtkInformation* requiredKeys)
{
  assert("pre: v_exists" && v != nullptr);
  bool result;
  if (this->HasKeys(requiredKeys))
  {
    result = this->RenderTranslucentPolygonalGeometry(v) == 1;
  }
  else
  {
    result = false;
  }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Render the volumetric geometry only if the prop has all the
// requiredKeys.
// This is recursive for composite props like svtkAssembly.
// An implementation is provided in svtkProp but each composite prop
// must override it.
// It returns if the rendering was performed.
// \pre v_exists: v!=0
// \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
bool svtkProp::RenderFilteredVolumetricGeometry(svtkViewport* v, svtkInformation* requiredKeys)
{
  assert("pre: v_exists" && v != nullptr);
  bool result;
  if (this->HasKeys(requiredKeys))
  {
    result = this->RenderVolumetricGeometry(v) == 1;
  }
  else
  {
    result = false;
  }
  return result;
}

// ----------------------------------------------------------------------------
// Description:
// Render in the overlay of the viewport only if the prop has all the
// requiredKeys.
// This is recursive for composite props like svtkAssembly.
// An implementation is provided in svtkProp but each composite prop
// must override it.
// It returns if the rendering was performed.
// \pre v_exists: v!=0
// \pre keys_can_be_null: requiredKeys==0 || requiredKeys!=0
bool svtkProp::RenderFilteredOverlay(svtkViewport* v, svtkInformation* requiredKeys)
{
  assert("pre: v_exists" && v != nullptr);
  bool result;
  if (this->HasKeys(requiredKeys))
  {
    result = this->RenderOverlay(v) == 1;
  }
  else
  {
    result = false;
  }
  return result;
}

void svtkProp::SetShaderProperty(svtkShaderProperty* property)
{
  if (this->ShaderProperty != property)
  {
    if (this->ShaderProperty != nullptr)
    {
      this->ShaderProperty->UnRegister(this);
    }
    this->ShaderProperty = property;
    if (this->ShaderProperty != nullptr)
    {
      this->ShaderProperty->Register(this);
    }
    this->Modified();
  }
}

svtkShaderProperty* svtkProp::GetShaderProperty()
{
  if (this->ShaderProperty == nullptr)
  {
    this->ShaderProperty = svtkShaderProperty::New();
    this->ShaderProperty->Register(this);
    this->ShaderProperty->Delete();
  }
  return this->ShaderProperty;
}
