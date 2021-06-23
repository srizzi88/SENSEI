/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExternalOpenGLCamera.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkExternalOpenGLCamera.h"

#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGL.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLState.h"
#include "svtkOutputWindow.h"
#include "svtkPerspectiveTransform.h"
#include "svtkRenderer.h"
#include "svtkTransform.h"

#include <cmath>

svtkStandardNewMacro(svtkExternalOpenGLCamera);

//----------------------------------------------------------------------------
svtkExternalOpenGLCamera::svtkExternalOpenGLCamera()
{
  this->UserProvidedViewTransform = false;
}

//----------------------------------------------------------------------------
void svtkExternalOpenGLCamera::SetViewTransformMatrix(const double elements[16])
{
  if (!elements)
  {
    return;
  }
  // Transpose the matrix to undo the transpose that SVTK does internally
  svtkMatrix4x4* matrix = svtkMatrix4x4::New();
  matrix->DeepCopy(elements);
  matrix->Transpose();
  this->ViewTransform->SetMatrix(matrix);
  this->ModelViewTransform->SetMatrix(matrix);
  this->UserProvidedViewTransform = true;
  matrix->Delete();
}

//----------------------------------------------------------------------------
void svtkExternalOpenGLCamera::SetProjectionTransformMatrix(const double elements[16])
{
  if (!elements)
  {
    return;
  }
  // Transpose the matrix to undo the transpose that SVTK does internally
  svtkMatrix4x4* matrix = svtkMatrix4x4::New();
  matrix->DeepCopy(elements);
  matrix->Transpose();

  this->SetExplicitProjectionTransformMatrix(matrix);
  this->SetUseExplicitProjectionTransformMatrix(true);
  matrix->Delete();
}

//----------------------------------------------------------------------------
void svtkExternalOpenGLCamera::ComputeViewTransform()
{
  if (this->UserProvidedViewTransform)
  {
    // Do not do anything
    return;
  }
  else
  {
    this->Superclass::ComputeViewTransform();
  }
}

//----------------------------------------------------------------------------
void svtkExternalOpenGLCamera::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
