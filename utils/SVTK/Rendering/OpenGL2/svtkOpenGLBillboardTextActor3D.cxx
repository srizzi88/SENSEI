/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLBillboardTextActor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLBillboardTextActor3D.h"

#include "svtkObjectFactory.h"
#include "svtkOpenGLGL2PSHelper.h"
#include "svtkRenderer.h"

#include <string>

svtkStandardNewMacro(svtkOpenGLBillboardTextActor3D);

//------------------------------------------------------------------------------
void svtkOpenGLBillboardTextActor3D::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int svtkOpenGLBillboardTextActor3D::RenderTranslucentPolygonalGeometry(svtkViewport* vp)
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps)
  {
    switch (gl2ps->GetActiveState())
    {
      case svtkOpenGLGL2PSHelper::Capture: // Render to GL2PS
        return this->RenderGL2PS(vp, gl2ps);
      case svtkOpenGLGL2PSHelper::Background: // No rendering
        return 0;
      case svtkOpenGLGL2PSHelper::Inactive: // Superclass render
        break;
    }
  }

  return this->Superclass::RenderTranslucentPolygonalGeometry(vp);
}

//------------------------------------------------------------------------------
svtkOpenGLBillboardTextActor3D::svtkOpenGLBillboardTextActor3D() = default;

//------------------------------------------------------------------------------
svtkOpenGLBillboardTextActor3D::~svtkOpenGLBillboardTextActor3D() = default;

//------------------------------------------------------------------------------
int svtkOpenGLBillboardTextActor3D::RenderGL2PS(svtkViewport* viewport, svtkOpenGLGL2PSHelper* gl2ps)
{
  if (!this->InputIsValid() || !this->IsValid())
  {
    return 0;
  }

  svtkRenderer* ren = svtkRenderer::SafeDownCast(viewport);
  if (!ren)
  {
    svtkWarningMacro("Viewport is not a renderer?");
    return 0;
  }

  gl2ps->DrawString(this->Input, this->TextProperty, this->AnchorDC, this->AnchorDC[2] + 1e-6, ren);

  return 1;
}
