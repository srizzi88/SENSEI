/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDefaultPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkDefaultPass.h"
#include "svtkObjectFactory.h"
#include "svtkProp.h"
#include "svtkRenderState.h"
#include "svtkRenderer.h"
#include <cassert>

svtkStandardNewMacro(svtkDefaultPass);

// ----------------------------------------------------------------------------
svtkDefaultPass::svtkDefaultPass() = default;

// ----------------------------------------------------------------------------
svtkDefaultPass::~svtkDefaultPass() = default;

// ----------------------------------------------------------------------------
void svtkDefaultPass::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

// ----------------------------------------------------------------------------
// Description:
// Perform rendering according to a render state \p s.
// \pre s_exists: s!=0
void svtkDefaultPass::Render(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  this->NumberOfRenderedProps = 0;
  this->RenderOpaqueGeometry(s);
  this->RenderTranslucentPolygonalGeometry(s);
  this->RenderVolumetricGeometry(s);
  this->RenderOverlay(s);
}

// ----------------------------------------------------------------------------
// Description:
// Opaque pass without key checking.
// \pre s_exists: s!=0
void svtkDefaultPass::RenderOpaqueGeometry(const svtkRenderState* s)
{
  assert("pre s_exits" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    int rendered = s->GetPropArray()[i]->RenderOpaqueGeometry(s->GetRenderer());
    this->NumberOfRenderedProps += rendered;
    ++i;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Opaque pass with key checking.
// \pre s_exists: s!=0
void svtkDefaultPass::RenderFilteredOpaqueGeometry(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    svtkProp* p = s->GetPropArray()[i];
    if (p->HasKeys(s->GetRequiredKeys()))
    {
      int rendered = p->RenderFilteredOpaqueGeometry(s->GetRenderer(), s->GetRequiredKeys());
      this->NumberOfRenderedProps += rendered;
    }
    ++i;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Translucent pass without key checking.
// \pre s_exists: s!=0
void svtkDefaultPass::RenderTranslucentPolygonalGeometry(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    svtkProp* p = s->GetPropArray()[i];
    int rendered = p->RenderTranslucentPolygonalGeometry(s->GetRenderer());
    this->NumberOfRenderedProps += rendered;
    ++i;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Translucent pass with key checking.
// \pre s_exists: s!=0
void svtkDefaultPass::RenderFilteredTranslucentPolygonalGeometry(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    svtkProp* p = s->GetPropArray()[i];
    if (p->HasKeys(s->GetRequiredKeys()))
    {
      int rendered =
        p->RenderFilteredTranslucentPolygonalGeometry(s->GetRenderer(), s->GetRequiredKeys());
      this->NumberOfRenderedProps += rendered;
    }
    ++i;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Volume pass without key checking.
// \pre s_exists: s!=0
void svtkDefaultPass::RenderVolumetricGeometry(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    int rendered = s->GetPropArray()[i]->RenderVolumetricGeometry(s->GetRenderer());
    this->NumberOfRenderedProps += rendered;
    ++i;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Translucent pass with key checking.
// \pre s_exists: s!=0
void svtkDefaultPass::RenderFilteredVolumetricGeometry(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    svtkProp* p = s->GetPropArray()[i];
    if (p->HasKeys(s->GetRequiredKeys()))
    {
      int rendered = p->RenderFilteredVolumetricGeometry(s->GetRenderer(), s->GetRequiredKeys());
      this->NumberOfRenderedProps += rendered;
    }
    ++i;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Overlay pass without key checking.
// \pre s_exists: s!=0
void svtkDefaultPass::RenderOverlay(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    int rendered = s->GetPropArray()[i]->RenderOverlay(s->GetRenderer());
    this->NumberOfRenderedProps += rendered;
    ++i;
  }
}

// ----------------------------------------------------------------------------
// Description:
// Overlay pass with key checking.
// \pre s_exists: s!=0
void svtkDefaultPass::RenderFilteredOverlay(const svtkRenderState* s)
{
  assert("pre: s_exists" && s != nullptr);

  int c = s->GetPropArrayCount();
  int i = 0;
  while (i < c)
  {
    svtkProp* p = s->GetPropArray()[i];
    if (p->HasKeys(s->GetRequiredKeys()))
    {
      int rendered = p->RenderFilteredOverlay(s->GetRenderer(), s->GetRequiredKeys());
      this->NumberOfRenderedProps += rendered;
    }
    ++i;
  }
}
