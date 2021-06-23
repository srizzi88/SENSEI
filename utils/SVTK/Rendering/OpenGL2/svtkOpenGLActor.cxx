/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLActor.h"

#include "svtkDepthPeelingPass.h"
#include "svtkDualDepthPeelingPass.h"
#include "svtkInformation.h"
#include "svtkInformationIntegerKey.h"
#include "svtkMapper.h"
#include "svtkMatrix3x3.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLRenderer.h"
#include "svtkOpenGLState.h"
#include "svtkProperty.h"
#include "svtkRenderWindow.h"
#include "svtkTransform.h"

#include <cmath>

svtkStandardNewMacro(svtkOpenGLActor);

svtkInformationKeyMacro(svtkOpenGLActor, GLDepthMaskOverride, Integer);

svtkOpenGLActor::svtkOpenGLActor()
{
  this->MCWCMatrix = svtkMatrix4x4::New();
  this->NormalMatrix = svtkMatrix3x3::New();
  this->NormalTransform = svtkTransform::New();
}

svtkOpenGLActor::~svtkOpenGLActor()
{
  this->MCWCMatrix->Delete();
  this->NormalMatrix->Delete();
  this->NormalTransform->Delete();
}

// Actual actor render method.
void svtkOpenGLActor::Render(svtkRenderer* ren, svtkMapper* mapper)
{
  svtkOpenGLClearErrorMacro();

  svtkOpenGLState* ostate = static_cast<svtkOpenGLRenderer*>(ren)->GetState();
  svtkOpenGLState::ScopedglDepthMask dmsaver(ostate);

  // get opacity
  bool opaque = !this->IsRenderingTranslucentPolygonalGeometry();
  if (opaque)
  {
    ostate->svtkglDepthMask(GL_TRUE);
  }
  else
  {
    svtkHardwareSelector* selector = ren->GetSelector();
    bool picking = (selector != nullptr);
    if (picking)
    {
      ostate->svtkglDepthMask(GL_TRUE);
    }
    else
    {
      // check for depth peeling
      svtkInformation* info = this->GetPropertyKeys();
      if (info && info->Has(svtkOpenGLActor::GLDepthMaskOverride()))
      {
        int maskoverride = info->Get(svtkOpenGLActor::GLDepthMaskOverride());
        switch (maskoverride)
        {
          case 0:
            ostate->svtkglDepthMask(GL_FALSE);
            break;
          case 1:
            ostate->svtkglDepthMask(GL_TRUE);
            break;
          default:
            // Do nothing.
            break;
        }
      }
      else
      {
        ostate->svtkglDepthMask(GL_FALSE); // transparency with alpha blending
      }
    }
  }

  // send a render to the mapper; update pipeline
  mapper->Render(ren, this);

  if (!opaque)
  {
    ostate->svtkglDepthMask(GL_TRUE);
  }

  svtkOpenGLCheckErrorMacro("failed after Render");
}

//----------------------------------------------------------------------------
void svtkOpenGLActor::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void svtkOpenGLActor::GetKeyMatrices(svtkMatrix4x4*& mcwc, svtkMatrix3x3*& normMat)
{
  // has the actor changed?
  if (this->GetMTime() > this->KeyMatrixTime)
  {
    this->ComputeMatrix();
    this->MCWCMatrix->DeepCopy(this->Matrix);
    this->MCWCMatrix->Transpose();

    if (this->GetIsIdentity())
    {
      this->NormalMatrix->Identity();
    }
    else
    {
      this->NormalTransform->SetMatrix(this->Matrix);
      svtkMatrix4x4* mat4 = this->NormalTransform->GetMatrix();
      for (int i = 0; i < 3; ++i)
      {
        for (int j = 0; j < 3; ++j)
        {
          this->NormalMatrix->SetElement(i, j, mat4->GetElement(i, j));
        }
      }
    }
    this->NormalMatrix->Invert();
    this->KeyMatrixTime.Modified();
  }

  mcwc = this->MCWCMatrix;
  normMat = this->NormalMatrix;
}
