/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayWindowNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOSPRayWindowNode.h"

#include "svtkCollectionIterator.h"
#include "svtkFloatArray.h"
#include "svtkOSPRayPass.h"
#include "svtkOSPRayRendererNode.h"
#include "svtkOSPRayViewNodeFactory.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRendererCollection.h"
#include "svtkUnsignedCharArray.h"
#include "svtkViewNodeCollection.h"

#include "RTWrapper/RTWrapper.h"

//============================================================================
svtkStandardNewMacro(svtkOSPRayWindowNode);

//----------------------------------------------------------------------------
svtkOSPRayWindowNode::svtkOSPRayWindowNode()
{
  svtkOSPRayPass::RTInit();
  svtkOSPRayViewNodeFactory* fac = svtkOSPRayViewNodeFactory::New();
  this->SetMyFactory(fac);
  fac->Delete();
}

//----------------------------------------------------------------------------
svtkOSPRayWindowNode::~svtkOSPRayWindowNode()
{
  svtkOSPRayPass::RTShutdown();
}

//----------------------------------------------------------------------------
void svtkOSPRayWindowNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkOSPRayWindowNode::Render(bool prepass)
{
  if (!prepass)
  {
    // composite all renderers framebuffers together
    this->ColorBuffer->SetNumberOfComponents(4);
    this->ColorBuffer->SetNumberOfTuples(this->Size[0] * this->Size[1]);
    unsigned char* rgba = static_cast<unsigned char*>(this->ColorBuffer->GetVoidPointer(0));

    this->ZBuffer->SetNumberOfComponents(1);
    this->ZBuffer->SetNumberOfTuples(this->Size[0] * this->Size[1]);
    float* z = static_cast<float*>(this->ZBuffer->GetVoidPointer(0));

    svtkViewNodeCollection* renderers = this->GetChildren();
    svtkCollectionIterator* it = renderers->NewIterator();
    it->InitTraversal();

    int layer = 0;
    int count = 0;
    while (count < renderers->GetNumberOfItems())
    {
      it->InitTraversal();
      while (!it->IsDoneWithTraversal())
      {
        svtkOSPRayRendererNode* child = svtkOSPRayRendererNode::SafeDownCast(it->GetCurrentObject());
        svtkRenderer* ren = svtkRenderer::SafeDownCast(child->GetRenderable());
        if (ren->GetLayer() == layer)
        {
          child->WriteLayer(rgba, z, this->Size[0], this->Size[1], layer);
          count++;
        }
        it->GoToNextItem();
      }
      layer++;
    }
    it->Delete();
  }
}
