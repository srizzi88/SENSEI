/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkOpenGLCamera.h"

#include "svtkMatrix3x3.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLError.h"
#include "svtkOpenGLRenderWindow.h"
#include "svtkOpenGLState.h"
#include "svtkOutputWindow.h"
#include "svtkRenderer.h"

#include <cmath>

svtkStandardNewMacro(svtkOpenGLCamera);

svtkOpenGLCamera::svtkOpenGLCamera()
{
  this->WCDCMatrix = svtkMatrix4x4::New();
  this->WCVCMatrix = svtkMatrix4x4::New();
  this->NormalMatrix = svtkMatrix3x3::New();
  this->VCDCMatrix = svtkMatrix4x4::New();
  this->LastRenderer = nullptr;
}

svtkOpenGLCamera::~svtkOpenGLCamera()
{
  this->WCDCMatrix->Delete();
  this->WCVCMatrix->Delete();
  this->NormalMatrix->Delete();
  this->VCDCMatrix->Delete();
}

// Implement base class method.
void svtkOpenGLCamera::Render(svtkRenderer* ren)
{
  svtkOpenGLClearErrorMacro();

  int lowerLeft[2];
  int usize, vsize;

  svtkOpenGLRenderWindow* win = svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  svtkOpenGLState* ostate = win->GetState();

  // find out if we should stereo render
  this->Stereo = (ren->GetRenderWindow())->GetStereoRender();
  ren->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft + 1);

  ostate->svtkglViewport(lowerLeft[0], lowerLeft[1], usize, vsize);
  ostate->svtkglEnable(GL_SCISSOR_TEST);
  if (this->UseScissor)
  {
    ostate->svtkglScissor(this->ScissorRect.GetX(), this->ScissorRect.GetY(),
      this->ScissorRect.GetWidth(), this->ScissorRect.GetHeight());
    this->UseScissor = false;
  }
  else
  {
    ostate->svtkglScissor(lowerLeft[0], lowerLeft[1], usize, vsize);
  }

  if ((ren->GetRenderWindow())->GetErase() && ren->GetErase())
  {
    ren->Clear();
  }

  svtkOpenGLCheckErrorMacro("failed after Render");
}

//----------------------------------------------------------------------------
void svtkOpenGLCamera::UpdateViewport(svtkRenderer* ren)
{
  svtkOpenGLClearErrorMacro();
  svtkOpenGLRenderWindow* win = svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow());
  svtkOpenGLState* ostate = win->GetState();

  int lowerLeft[2];
  int usize, vsize;
  ren->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft + 1);

  ostate->svtkglViewport(lowerLeft[0], lowerLeft[1], usize, vsize);
  ostate->svtkglEnable(GL_SCISSOR_TEST);
  if (this->UseScissor)
  {
    ostate->svtkglScissor(this->ScissorRect.GetX(), this->ScissorRect.GetY(),
      this->ScissorRect.GetWidth(), this->ScissorRect.GetHeight());
    this->UseScissor = false;
  }
  else
  {
    ostate->svtkglScissor(lowerLeft[0], lowerLeft[1], usize, vsize);
  }

  svtkOpenGLCheckErrorMacro("failed after UpdateViewport");
}

//----------------------------------------------------------------------------
void svtkOpenGLCamera::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

void svtkOpenGLCamera::GetKeyMatrices(svtkRenderer* ren, svtkMatrix4x4*& wcvc, svtkMatrix3x3*& normMat,
  svtkMatrix4x4*& vcdc, svtkMatrix4x4*& wcdc)
{
  // has the camera changed?
  if (ren != this->LastRenderer || this->MTime > this->KeyMatrixTime ||
    ren->GetMTime() > this->KeyMatrixTime)
  {
    this->WCVCMatrix->DeepCopy(this->GetModelViewTransformMatrix());

    for (int i = 0; i < 3; ++i)
    {
      for (int j = 0; j < 3; ++j)
      {
        this->NormalMatrix->SetElement(i, j, this->WCVCMatrix->GetElement(i, j));
      }
    }
    this->NormalMatrix->Invert();

    this->WCVCMatrix->Transpose();

    this->VCDCMatrix->DeepCopy(
      this->GetProjectionTransformMatrix(ren->GetTiledAspectRatio(), -1, 1));
    this->VCDCMatrix->Transpose();

    svtkMatrix4x4::Multiply4x4(this->WCVCMatrix, this->VCDCMatrix, this->WCDCMatrix);

    this->KeyMatrixTime.Modified();
    this->LastRenderer = ren;
  }

  wcvc = this->WCVCMatrix;
  normMat = this->NormalMatrix;
  vcdc = this->VCDCMatrix;
  wcdc = this->WCDCMatrix;
}
