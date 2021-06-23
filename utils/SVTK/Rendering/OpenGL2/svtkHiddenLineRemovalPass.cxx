/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHiddenLineRemovalPass.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtk_glew.h"

#include "svtkHiddenLineRemovalPass.h"

#include "svtkActor.h"
#include "svtkMapper.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLRenderUtilities.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLState.h"
#include "svtkProp.h"
#include "svtkProperty.h"
#include "svtkRenderState.h"
#include "svtkRenderer.h"

#include <string>

namespace
{
void annotate(const std::string& str)
{
  svtkOpenGLRenderUtilities::MarkDebugEvent(str);
}
}

svtkStandardNewMacro(svtkHiddenLineRemovalPass);

//------------------------------------------------------------------------------
void svtkHiddenLineRemovalPass::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkHiddenLineRemovalPass::Render(const svtkRenderState* s)
{
  this->NumberOfRenderedProps = 0;

  // Separate the wireframe props from the others:
  std::vector<svtkProp*> wireframeProps;
  std::vector<svtkProp*> otherProps;
  for (int i = 0; i < s->GetPropArrayCount(); ++i)
  {
    bool isWireframe = false;
    svtkProp* prop = s->GetPropArray()[i];
    svtkActor* actor = svtkActor::SafeDownCast(prop);
    if (actor)
    {
      svtkProperty* property = actor->GetProperty();
      if (property->GetRepresentation() == SVTK_WIREFRAME)
      {
        isWireframe = true;
      }
    }
    if (isWireframe)
    {
      wireframeProps.push_back(prop);
    }
    else
    {
      otherProps.push_back(prop);
    }
  }

  svtkViewport* vp = s->GetRenderer();
  svtkOpenGLState* ostate = static_cast<svtkOpenGLRenderer*>(vp)->GetState();

  // Render the non-wireframe geometry as normal:
  annotate("Rendering non-wireframe props.");
  this->NumberOfRenderedProps = this->RenderProps(otherProps, vp);
  svtkOpenGLStaticCheckErrorMacro("Error after non-wireframe geometry.");

  // Store the coincident topology parameters -- we want to force polygon
  // offset to keep the drawn lines sharp:
  int ctMode = svtkMapper::GetResolveCoincidentTopology();
  double ctFactor, ctUnits;
  svtkMapper::GetResolveCoincidentTopologyPolygonOffsetParameters(ctFactor, ctUnits);
  svtkMapper::SetResolveCoincidentTopology(SVTK_RESOLVE_POLYGON_OFFSET);
  svtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters(2.0, 2.0);

  // Draw the wireframe props as surfaces into the depth buffer only:
  annotate("Rendering wireframe prop surfaces.");
  this->SetRepresentation(wireframeProps, SVTK_SURFACE);
  ostate->svtkglColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
  this->RenderProps(wireframeProps, vp);
  svtkOpenGLStaticCheckErrorMacro("Error after wireframe surface rendering.");

  // Now draw the wireframes as normal:
  annotate("Rendering wireframes.");
  this->SetRepresentation(wireframeProps, SVTK_WIREFRAME);
  ostate->svtkglColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
  this->NumberOfRenderedProps = this->RenderProps(wireframeProps, vp);
  svtkOpenGLStaticCheckErrorMacro("Error after wireframe rendering.");

  // Restore the previous coincident topology parameters:
  svtkMapper::SetResolveCoincidentTopology(ctMode);
  svtkMapper::SetResolveCoincidentTopologyPolygonOffsetParameters(ctFactor, ctUnits);
}

//------------------------------------------------------------------------------
bool svtkHiddenLineRemovalPass::WireframePropsExist(svtkProp** propArray, int nProps)
{
  for (int i = 0; i < nProps; ++i)
  {
    svtkActor* actor = svtkActor::SafeDownCast(propArray[i]);
    if (actor)
    {
      svtkProperty* property = actor->GetProperty();
      if (property->GetRepresentation() == SVTK_WIREFRAME)
      {
        return true;
      }
    }
  }

  return false;
}

//------------------------------------------------------------------------------
svtkHiddenLineRemovalPass::svtkHiddenLineRemovalPass() = default;

//------------------------------------------------------------------------------
svtkHiddenLineRemovalPass::~svtkHiddenLineRemovalPass() = default;

//------------------------------------------------------------------------------
void svtkHiddenLineRemovalPass::SetRepresentation(std::vector<svtkProp*>& props, int repr)
{
  for (std::vector<svtkProp*>::iterator it = props.begin(), itEnd = props.end(); it != itEnd; ++it)
  {
    svtkActor* actor = svtkActor::SafeDownCast(*it);
    if (actor)
    {
      actor->GetProperty()->SetRepresentation(repr);
    }
  }
}

//------------------------------------------------------------------------------
int svtkHiddenLineRemovalPass::RenderProps(std::vector<svtkProp*>& props, svtkViewport* vp)
{
  int propsRendered = 0;
  for (std::vector<svtkProp*>::iterator it = props.begin(), itEnd = props.end(); it != itEnd; ++it)
  {
    propsRendered += (*it)->RenderOpaqueGeometry(vp);
  }
  return propsRendered;
}
