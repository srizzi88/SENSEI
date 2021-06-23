/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeMapperNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkVolumeMapperNode.h"

#include "svtkActor.h"
#include "svtkCellArray.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolygon.h"
#include "svtkProperty.h"

//============================================================================
svtkStandardNewMacro(svtkVolumeMapperNode);

//----------------------------------------------------------------------------
svtkVolumeMapperNode::svtkVolumeMapperNode() {}

//----------------------------------------------------------------------------
svtkVolumeMapperNode::~svtkVolumeMapperNode() {}

//----------------------------------------------------------------------------
void svtkVolumeMapperNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
