/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLTextMapper.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLTextMapper.h"

#include "svtkActor2D.h"
#include "svtkCoordinate.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLGL2PSHelper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkViewport.h"

svtkStandardNewMacro(svtkOpenGLTextMapper);

//------------------------------------------------------------------------------
void svtkOpenGLTextMapper::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
void svtkOpenGLTextMapper::RenderOverlay(svtkViewport* vp, svtkActor2D* act)
{
  // Render to GL2PS if capturing:
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps)
  {
    switch (gl2ps->GetActiveState())
    {
      case svtkOpenGLGL2PSHelper::Capture:
        this->RenderGL2PS(vp, act, gl2ps);
        return;
      case svtkOpenGLGL2PSHelper::Background:
        return; // No rendering.
      case svtkOpenGLGL2PSHelper::Inactive:
        break; // continue rendering.
    }
  }

  this->Superclass::RenderOverlay(vp, act);
}

//------------------------------------------------------------------------------
svtkOpenGLTextMapper::svtkOpenGLTextMapper() = default;

//------------------------------------------------------------------------------
svtkOpenGLTextMapper::~svtkOpenGLTextMapper() = default;

//------------------------------------------------------------------------------
void svtkOpenGLTextMapper::RenderGL2PS(svtkViewport* vp, svtkActor2D* act, svtkOpenGLGL2PSHelper* gl2ps)
{
  std::string input = (this->Input && this->Input[0]) ? this->Input : "";
  if (input.empty())
  {
    return;
  }

  svtkRenderer* ren = svtkRenderer::SafeDownCast(vp);
  if (!ren)
  {
    svtkWarningMacro("Viewport is not a renderer.");
    return;
  }

  // Figure out position:
  svtkCoordinate* coord = act->GetActualPositionCoordinate();
  double* textPos2 = coord->GetComputedDoubleDisplayValue(ren);
  double pos[3];
  pos[0] = textPos2[0];
  pos[1] = textPos2[1];
  pos[2] = -1.;

  gl2ps->DrawString(input, this->TextProperty, pos, pos[2] + 1e-6, ren);
}
