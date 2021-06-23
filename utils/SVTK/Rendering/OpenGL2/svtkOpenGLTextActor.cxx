/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLTextActor.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLTextActor.h"

#include "svtkCoordinate.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLGL2PSHelper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkViewport.h"

svtkStandardNewMacro(svtkOpenGLTextActor);

//------------------------------------------------------------------------------
void svtkOpenGLTextActor::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int svtkOpenGLTextActor::RenderOverlay(svtkViewport* viewport)
{
  // Render to GL2PS if capturing:
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps)
  {
    switch (gl2ps->GetActiveState())
    {
      case svtkOpenGLGL2PSHelper::Capture:
        return this->RenderGL2PS(viewport, gl2ps);
      case svtkOpenGLGL2PSHelper::Background:
        return 0; // No rendering.
      case svtkOpenGLGL2PSHelper::Inactive:
        break; // continue rendering.
    }
  }

  return this->Superclass::RenderOverlay(viewport);
}

//------------------------------------------------------------------------------
svtkOpenGLTextActor::svtkOpenGLTextActor() = default;

//------------------------------------------------------------------------------
svtkOpenGLTextActor::~svtkOpenGLTextActor() = default;

//------------------------------------------------------------------------------
int svtkOpenGLTextActor::RenderGL2PS(svtkViewport* viewport, svtkOpenGLGL2PSHelper* gl2ps)
{
  std::string input = (this->Input && this->Input[0]) ? this->Input : "";
  if (input.empty())
  {
    return 0;
  }

  svtkRenderer* ren = svtkRenderer::SafeDownCast(viewport);
  if (!ren)
  {
    svtkWarningMacro("Viewport is not a renderer.");
    return 0;
  }

  // Figure out position:
  svtkCoordinate* coord = this->GetActualPositionCoordinate();
  double* textPos2 = coord->GetComputedDoubleDisplayValue(ren);
  double pos[3];
  pos[0] = textPos2[0];
  pos[1] = textPos2[1];
  pos[2] = -1.;

  svtkTextProperty* tprop = this->GetScaledTextProperty();
  gl2ps->DrawString(input, tprop, pos, pos[2] + 1e-6, ren);

  return 1;
}
