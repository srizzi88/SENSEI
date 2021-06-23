/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkVolumeNode.h"

#include "svtkAbstractVolumeMapper.h"
#include "svtkCellArray.h"
#include "svtkMapper.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkProperty.h"
#include "svtkVolume.h"

//============================================================================
svtkStandardNewMacro(svtkVolumeNode);

//----------------------------------------------------------------------------
svtkVolumeNode::svtkVolumeNode() {}

//----------------------------------------------------------------------------
svtkVolumeNode::~svtkVolumeNode() {}

//----------------------------------------------------------------------------
void svtkVolumeNode::Build(bool prepass)
{
  if (prepass)
  {
    svtkVolume* mine = svtkVolume::SafeDownCast(this->GetRenderable());
    if (!mine)
    {
      return;
    }
    if (!mine->GetMapper())
    {
      return;
    }

    this->PrepareNodes();
    this->AddMissingNode(mine->GetMapper());
    this->RemoveUnusedNodes();
  }
}

//----------------------------------------------------------------------------
void svtkVolumeNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
