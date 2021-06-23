/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWindowNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkWindowNode.h"

#include "svtkCollectionIterator.h"
#include "svtkFloatArray.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkRendererCollection.h"
#include "svtkRendererNode.h"
#include "svtkUnsignedCharArray.h"
#include "svtkViewNodeCollection.h"

//============================================================================
svtkStandardNewMacro(svtkWindowNode);

//----------------------------------------------------------------------------
svtkWindowNode::svtkWindowNode()
{
  this->Size[0] = 0;
  this->Size[1] = 0;
  this->ColorBuffer = svtkUnsignedCharArray::New();
  this->ZBuffer = svtkFloatArray::New();
}

//----------------------------------------------------------------------------
svtkWindowNode::~svtkWindowNode()
{
  this->ColorBuffer->Delete();
  this->ColorBuffer = 0;
  this->ZBuffer->Delete();
  this->ZBuffer = 0;
}

//----------------------------------------------------------------------------
void svtkWindowNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//----------------------------------------------------------------------------
void svtkWindowNode::Build(bool prepass)
{
  if (prepass)
  {
    svtkRenderWindow* mine = svtkRenderWindow::SafeDownCast(this->GetRenderable());
    if (!mine)
    {
      return;
    }

    this->PrepareNodes();
    this->AddMissingNodes(mine->GetRenderers());
    this->RemoveUnusedNodes();
  }
}

//----------------------------------------------------------------------------
void svtkWindowNode::Synchronize(bool prepass)
{
  if (prepass)
  {
    svtkRenderWindow* mine = svtkRenderWindow::SafeDownCast(this->GetRenderable());
    if (!mine)
    {
      return;
    }
    /*
      GetAAFrames()   svtkRenderWindow virtual
      GetActualSize() svtkWindow
      GetAlphaBitPlanes()     svtkRenderWindow virtual
      GetDoubleBuffer()       svtkWindow       virtual
      GetDPI()        svtkWindow       virtual
      GetFDFrames()   svtkRenderWindow virtual
      GetFullScreen() svtkRenderWindow virtual
      GetLineSmoothing()      svtkRenderWindow virtual
      GetMapped()     svtkWindow       virtual
      GetMTime()      svtkObject       virtual
      GetMultiSamples()       svtkRenderWindow virtual
      GetNeverRendered()      svtkRenderWindow virtual
      GetNumberOfLayers()     svtkRenderWindow virtual
      GetOffScreenRendering() svtkWindow       virtual
      GetPointSmoothing()     svtkRenderWindow virtual
      GetPolygonSmoothing()   svtkRenderWindow virtual
      GetPosition()   svtkWindow       virtual
      GetScreenSize()=0       svtkWindow       pure virtual
    */
    const int* sz = mine->GetSize();
    this->Size[0] = sz[0];
    this->Size[1] = sz[1];
    /*
      GetStereoType() svtkRenderWindow virtual
      GetSubFrames()  svtkRenderWindow virtual
      GetSwapBuffers()        svtkRenderWindow virtual
      GetTileScale()  svtkWindow       virtual
      GetTileViewport()       svtkWindow       virtual
      GetUseConstantFDOffsets()       svtkRenderWindow virtual
    */

    svtkViewNodeCollection* renderers = this->GetChildren();
    svtkCollectionIterator* it = renderers->NewIterator();
    it->InitTraversal();
    while (!it->IsDoneWithTraversal())
    {
      svtkRendererNode* child = svtkRendererNode::SafeDownCast(it->GetCurrentObject());
      child->SetSize(this->Size);
      it->GoToNextItem();
    }
    it->Delete();
  }
}
