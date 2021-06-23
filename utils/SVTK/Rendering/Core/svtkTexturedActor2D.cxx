/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTexturedActor2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkTexturedActor2D.h"

#include "svtkInformation.h"
#include "svtkObjectFactory.h"
#include "svtkRenderer.h"
#include "svtkTexture.h"

svtkStandardNewMacro(svtkTexturedActor2D);

svtkCxxSetObjectMacro(svtkTexturedActor2D, Texture, svtkTexture);

//-----------------------------------------------------------------------------
svtkTexturedActor2D::svtkTexturedActor2D()
{
  this->Texture = nullptr;
}

//-----------------------------------------------------------------------------
svtkTexturedActor2D::~svtkTexturedActor2D()
{
  this->SetTexture(nullptr);
}

//-----------------------------------------------------------------------------
void svtkTexturedActor2D::ReleaseGraphicsResources(svtkWindow* win)
{
  this->Superclass::ReleaseGraphicsResources(win);

  // Pass this information to the texture.
  if (this->Texture)
  {
    this->Texture->ReleaseGraphicsResources(win);
  }
}

//-----------------------------------------------------------------------------
int svtkTexturedActor2D::RenderOverlay(svtkViewport* viewport)
{
  // Render the texture.
  svtkRenderer* ren = svtkRenderer::SafeDownCast(viewport);
  svtkInformation* info = this->GetPropertyKeys();
  if (this->Texture)
  {
    this->Texture->Render(ren);
    if (!info)
    {
      info = svtkInformation::New();
      this->SetPropertyKeys(info);
      info->Delete();
    }
    info->Set(svtkProp::GeneralTextureUnit(), this->Texture->GetTextureUnit());
  }
  else if (info)
  {
    info->Remove(svtkProp::GeneralTextureUnit());
  }
  int result = this->Superclass::RenderOverlay(viewport);
  if (this->Texture)
  {
    this->Texture->PostRender(ren);
  }
  return result;
}

//-----------------------------------------------------------------------------
int svtkTexturedActor2D::RenderOpaqueGeometry(svtkViewport* viewport)
{
  // Render the texture.
  svtkRenderer* ren = svtkRenderer::SafeDownCast(viewport);
  if (this->Texture)
  {
    this->Texture->Render(ren);
  }
  int result = this->Superclass::RenderOpaqueGeometry(viewport);
  if (this->Texture)
  {
    this->Texture->PostRender(ren);
  }
  return result;
}

//-----------------------------------------------------------------------------
int svtkTexturedActor2D::RenderTranslucentPolygonalGeometry(svtkViewport* viewport)
{
  // Render the texture.
  svtkRenderer* ren = svtkRenderer::SafeDownCast(viewport);
  if (this->Texture)
  {
    this->Texture->Render(ren);
  }
  int result = this->Superclass::RenderTranslucentPolygonalGeometry(viewport);
  if (this->Texture)
  {
    this->Texture->PostRender(ren);
  }
  return result;
}

//-----------------------------------------------------------------------------
svtkMTimeType svtkTexturedActor2D::GetMTime()
{
  svtkMTimeType mTime = svtkActor2D::GetMTime();
  svtkMTimeType time;
  if (this->Texture)
  {
    time = this->Texture->GetMTime();
    mTime = (time > mTime ? time : mTime);
  }
  return mTime;
}

//-----------------------------------------------------------------------------
void svtkTexturedActor2D::ShallowCopy(svtkProp* prop)
{
  svtkTexturedActor2D* a = svtkTexturedActor2D::SafeDownCast(prop);
  if (a)
  {
    this->SetTexture(a->GetTexture());
  }

  // Now do superclass.
  this->Superclass::ShallowCopy(prop);
}

//-----------------------------------------------------------------------------
void svtkTexturedActor2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Texture: " << (this->Texture ? "" : "(none)") << endl;
  if (this->Texture)
  {
    this->Texture->PrintSelf(os, indent.GetNextIndent());
  }
}
