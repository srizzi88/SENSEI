/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRendererNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkRendererNode.h"

#include "svtkActor.h"
#include "svtkActorNode.h"
#include "svtkCamera.h"
#include "svtkCameraNode.h"
#include "svtkCollectionIterator.h"
#include "svtkLight.h"
#include "svtkLightCollection.h"
#include "svtkLightNode.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkRendererNode.h"
#include "svtkViewNodeCollection.h"

//============================================================================
svtkStandardNewMacro(svtkRendererNode);

//----------------------------------------------------------------------------
svtkRendererNode::svtkRendererNode()
{
  this->Size[0] = 0;
  this->Size[1] = 0;
  this->Viewport[0] = 0.0;
  this->Viewport[1] = 0.0;
  this->Viewport[2] = 1.0;
  this->Viewport[3] = 1.0;
  this->Scale[0] = 1;
  this->Scale[1] = 1;
}

//----------------------------------------------------------------------------
svtkRendererNode::~svtkRendererNode() {}

//----------------------------------------------------------------------------
void svtkRendererNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkRendererNode::Build(bool prepass)
{
  if (prepass)
  {
    svtkRenderer* mine = svtkRenderer::SafeDownCast(this->GetRenderable());
    if (!mine)
    {
      return;
    }

    this->PrepareNodes();
    this->AddMissingNodes(mine->GetLights());
    this->AddMissingNodes(mine->GetActors());
    this->AddMissingNodes(mine->GetVolumes());
    this->AddMissingNode(mine->GetActiveCamera());
    this->RemoveUnusedNodes();
  }
}
