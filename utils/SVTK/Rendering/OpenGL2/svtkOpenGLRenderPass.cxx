/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLRenderPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLRenderPass.h"

#include "svtkInformation.h"
#include "svtkInformationObjectBaseVectorKey.h"
#include "svtkObjectFactory.h"
#include "svtkProp.h"
#include "svtkRenderState.h"

#include <cassert>

svtkInformationKeyMacro(svtkOpenGLRenderPass, RenderPasses, ObjectBaseVector);

//------------------------------------------------------------------------------
void svtkOpenGLRenderPass::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
bool svtkOpenGLRenderPass::PreReplaceShaderValues(
  std::string&, std::string&, std::string&, svtkAbstractMapper*, svtkProp*)
{
  return true;
}
bool svtkOpenGLRenderPass::PostReplaceShaderValues(
  std::string&, std::string&, std::string&, svtkAbstractMapper*, svtkProp*)
{
  return true;
}

//------------------------------------------------------------------------------
bool svtkOpenGLRenderPass::SetShaderParameters(
  svtkShaderProgram*, svtkAbstractMapper*, svtkProp*, svtkOpenGLVertexArrayObject*)
{
  return true;
}

//------------------------------------------------------------------------------
svtkMTimeType svtkOpenGLRenderPass::GetShaderStageMTime()
{
  return 0;
}

//------------------------------------------------------------------------------
svtkOpenGLRenderPass::svtkOpenGLRenderPass() = default;

//------------------------------------------------------------------------------
svtkOpenGLRenderPass::~svtkOpenGLRenderPass() = default;

//------------------------------------------------------------------------------
void svtkOpenGLRenderPass::PreRender(const svtkRenderState* s)
{
  assert("Render state valid." && s);
  size_t numProps = s->GetPropArrayCount();
  for (size_t i = 0; i < numProps; ++i)
  {
    svtkProp* prop = s->GetPropArray()[i];
    svtkInformation* info = prop->GetPropertyKeys();
    if (!info)
    {
      info = svtkInformation::New();
      prop->SetPropertyKeys(info);
      info->FastDelete();
    }
    info->Append(svtkOpenGLRenderPass::RenderPasses(), this);
  }
}

//------------------------------------------------------------------------------
void svtkOpenGLRenderPass::PostRender(const svtkRenderState* s)
{
  assert("Render state valid." && s);
  size_t numProps = s->GetPropArrayCount();
  for (size_t i = 0; i < numProps; ++i)
  {
    svtkProp* prop = s->GetPropArray()[i];
    svtkInformation* info = prop->GetPropertyKeys();
    if (info)
    {
      info->Remove(svtkOpenGLRenderPass::RenderPasses(), this);
      if (info->Length(svtkOpenGLRenderPass::RenderPasses()) == 0)
      {
        info->Remove(svtkOpenGLRenderPass::RenderPasses());
      }
    }
  }
}
