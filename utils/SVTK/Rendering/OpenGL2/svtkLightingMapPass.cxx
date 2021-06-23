/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLightingMapPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkLightingMapPass.h"

#include "svtkClearRGBPass.h"
#include "svtkInformation.h"
#include "svtkInformationIntegerKey.h"
#include "svtkObjectFactory.h"
#include "svtkProp.h"
#include "svtkRenderState.h"
#include "svtkRenderer.h"
#include "svtkSmartPointer.h"

#include <cassert>

svtkStandardNewMacro(svtkLightingMapPass);

svtkInformationKeyMacro(svtkLightingMapPass, RENDER_LUMINANCE, Integer);
svtkInformationKeyMacro(svtkLightingMapPass, RENDER_NORMALS, Integer);

// ----------------------------------------------------------------------------
svtkLightingMapPass::svtkLightingMapPass()
{
  this->RenderType = LUMINANCE;
}

// ----------------------------------------------------------------------------
svtkLightingMapPass::~svtkLightingMapPass() = default;

// ----------------------------------------------------------------------------
void svtkLightingMapPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkLightingMapPass::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  // Render filtered geometry according to our keys
  this->NumberOfRenderedProps = 0;

  this->ClearLights(s->GetRenderer());
  this->UpdateLightGeometry(s->GetRenderer());
  this->UpdateLights(s->GetRenderer());

  this->RenderOpaqueGeometry(s);
}

// ----------------------------------------------------------------------------
// Description:
// Opaque pass with key checking.
// \pre s_exists: s!=0
void svtkLightingMapPass::RenderOpaqueGeometry(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  // Clear the RGB buffer first
  svtkSmartPointer<svtkClearRGBPass> clear = svtkSmartPointer<svtkClearRGBPass>::New();
  clear->Render(s);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    svtkProp* p = s->GetPropArray()[i];
    svtkSmartPointer<svtkInformation> keys = p->GetPropertyKeys();
    if (!keys)
    {
      keys.TakeReference(svtkInformation::New());
    }
    switch (this->GetRenderType())
    {
      case LUMINANCE:
        keys->Set(svtkLightingMapPass::RENDER_LUMINANCE(), 1);
        break;
      case NORMALS:
        keys->Set(svtkLightingMapPass::RENDER_NORMALS(), 1);
        break;
    }
    p->SetPropertyKeys(keys);
    int rendered = p->RenderOpaqueGeometry(s->GetRenderer());
    this->NumberOfRenderedProps += rendered;
    ++i;
  }

  // Remove keys
  i = 0;
  while (i < c)
  {
    svtkProp* p = s->GetPropArray()[i];
    svtkInformation* keys = p->GetPropertyKeys();
    switch (this->GetRenderType())
    {
      case LUMINANCE:
        keys->Remove(svtkLightingMapPass::RENDER_LUMINANCE());
        break;
      case NORMALS:
        keys->Remove(svtkLightingMapPass::RENDER_NORMALS());
        break;
    }
    p->SetPropertyKeys(keys);
    ++i;
  }
}
