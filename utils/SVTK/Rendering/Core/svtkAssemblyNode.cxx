/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssemblyNode.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkAssemblyNode.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkProp.h"

svtkStandardNewMacro(svtkAssemblyNode);

//----------------------------------------------------------------------------
svtkAssemblyNode::svtkAssemblyNode()
{
  this->ViewProp = nullptr;
  this->Matrix = nullptr;
}

//----------------------------------------------------------------------------
svtkAssemblyNode::~svtkAssemblyNode()
{
  if (this->Matrix)
  {
    this->Matrix->Delete();
    this->Matrix = nullptr;
  }
}

//----------------------------------------------------------------------------
// Don't do reference counting
void svtkAssemblyNode::SetViewProp(svtkProp* prop)
{
  this->ViewProp = prop;
}

//----------------------------------------------------------------------------
void svtkAssemblyNode::SetMatrix(svtkMatrix4x4* matrix)
{
  // delete previous
  if (this->Matrix)
  {
    this->Matrix->Delete();
    this->Matrix = nullptr;
  }
  // return if nullptr matrix specified
  if (!matrix)
  {
    return;
  }

  // else create a copy of the matrix
  svtkMatrix4x4* newMatrix = svtkMatrix4x4::New();
  newMatrix->DeepCopy(matrix);
  this->Matrix = newMatrix;
}

//----------------------------------------------------------------------------
svtkMTimeType svtkAssemblyNode::GetMTime()
{
  svtkMTimeType propMTime = 0;
  svtkMTimeType matrixMTime = 0;

  if (this->ViewProp)
  {
    propMTime = this->ViewProp->GetMTime();
  }
  if (this->Matrix)
  {
    matrixMTime = this->Matrix->GetMTime();
  }

  return (propMTime > matrixMTime ? propMTime : matrixMTime);
}

//----------------------------------------------------------------------------
void svtkAssemblyNode::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  if (this->ViewProp)
  {
    os << indent << "ViewProp: " << this->ViewProp << "\n";
  }
  else
  {
    os << indent << "ViewProp: (none)\n";
  }

  if (this->Matrix)
  {
    os << indent << "Matrix: " << this->Matrix << "\n";
  }
  else
  {
    os << indent << "Matrix: (none)\n";
  }
}
