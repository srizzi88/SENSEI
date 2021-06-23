/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWebGLObject.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkWebGLObject.h"

#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkUnsignedCharArray.h"

#include <algorithm>

svtkStandardNewMacro(svtkWebGLObject);
#include <map>
#include <vector>

//-----------------------------------------------------------------------------
svtkWebGLObject::svtkWebGLObject()
{
  this->iswireframeMode = false;
  this->hasChanged = false;
  this->webGlType = wTRIANGLES;
  this->hasTransparency = false;
  this->iswidget = false;
  this->interactAtServer = false;
}

//-----------------------------------------------------------------------------
svtkWebGLObject::~svtkWebGLObject() = default;

//-----------------------------------------------------------------------------
std::string svtkWebGLObject::GetId()
{
  return this->id;
}

//-----------------------------------------------------------------------------
void svtkWebGLObject::SetId(const std::string& i)
{
  this->id = i;
}

//-----------------------------------------------------------------------------
void svtkWebGLObject::SetType(WebGLObjectTypes t)
{
  this->webGlType = t;
}

//-----------------------------------------------------------------------------
void svtkWebGLObject::SetTransformationMatrix(svtkMatrix4x4* m)
{
  for (int i = 0; i < 16; i++)
    this->Matrix[i] = m->GetElement(i / 4, i % 4);
}

//-----------------------------------------------------------------------------
std::string svtkWebGLObject::GetMD5()
{
  return this->MD5;
}

//-----------------------------------------------------------------------------
void svtkWebGLObject::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//-----------------------------------------------------------------------------
bool svtkWebGLObject::HasChanged()
{
  return this->hasChanged;
}

//-----------------------------------------------------------------------------
void svtkWebGLObject::SetWireframeMode(bool wireframe)
{
  this->iswireframeMode = wireframe;
}

//-----------------------------------------------------------------------------
bool svtkWebGLObject::isWireframeMode()
{
  return this->iswireframeMode;
}

//-----------------------------------------------------------------------------
void svtkWebGLObject::SetVisibility(bool vis)
{
  this->isvisible = vis;
}

//-----------------------------------------------------------------------------
bool svtkWebGLObject::isVisible()
{
  return this->isvisible;
}

//-----------------------------------------------------------------------------
void svtkWebGLObject::SetHasTransparency(bool t)
{
  this->hasTransparency = t;
}

//-----------------------------------------------------------------------------
void svtkWebGLObject::SetIsWidget(bool w)
{
  this->iswidget = w;
}

//-----------------------------------------------------------------------------
bool svtkWebGLObject::isWidget()
{
  return this->iswidget;
}

//-----------------------------------------------------------------------------
bool svtkWebGLObject::HasTransparency()
{
  return this->hasTransparency;
}

//-----------------------------------------------------------------------------
void svtkWebGLObject::SetRendererId(size_t i)
{
  this->rendererId = i;
}

//-----------------------------------------------------------------------------
size_t svtkWebGLObject::GetRendererId()
{
  return this->rendererId;
}

//-----------------------------------------------------------------------------
void svtkWebGLObject::SetLayer(int l)
{
  this->layer = l;
}

//-----------------------------------------------------------------------------
int svtkWebGLObject::GetLayer()
{
  return this->layer;
}

//-----------------------------------------------------------------------------
bool svtkWebGLObject::InteractAtServer()
{
  return this->interactAtServer;
}

//-----------------------------------------------------------------------------
void svtkWebGLObject::SetInteractAtServer(bool i)
{
  this->interactAtServer = i;
}

//-----------------------------------------------------------------------------
void svtkWebGLObject::GetBinaryData(int part, svtkUnsignedCharArray* buffer)
{
  if (!buffer)
  {
    svtkErrorMacro("Buffer must not be nullptr.");
    return;
  }

  const int binarySize = this->GetBinarySize(part);
  const unsigned char* binaryData = this->GetBinaryData(part);

  buffer->SetNumberOfComponents(1);
  buffer->SetNumberOfTuples(binarySize);

  if (binarySize)
  {
    std::copy(binaryData, binaryData + binarySize, buffer->GetPointer(0));
  }
}

//-----------------------------------------------------------------------------
void svtkWebGLObject::GenerateBinaryData()
{
  this->hasChanged = false;
}
//-----------------------------------------------------------------------------
unsigned char* svtkWebGLObject::GetBinaryData(int svtkNotUsed(part))
{
  return nullptr;
}
//-----------------------------------------------------------------------------
int svtkWebGLObject::GetBinarySize(int svtkNotUsed(part))
{
  return 0;
}
//-----------------------------------------------------------------------------
int svtkWebGLObject::GetNumberOfParts()
{
  return 0;
}
