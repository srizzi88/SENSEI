/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLTextActor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkOpenGLTextActor3D.h"

#include "svtkCamera.h"
#include "svtkMatrix4x4.h"
#include "svtkNew.h"
#include "svtkObjectFactory.h"
#include "svtkOpenGLGL2PSHelper.h"
#include "svtkPath.h"
#include "svtkRenderer.h"
#include "svtkTextProperty.h"
#include "svtkTextRenderer.h"

#include <sstream>
#include <string>

svtkStandardNewMacro(svtkOpenGLTextActor3D);

//------------------------------------------------------------------------------
void svtkOpenGLTextActor3D::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}

//------------------------------------------------------------------------------
int svtkOpenGLTextActor3D::RenderTranslucentPolygonalGeometry(svtkViewport* vp)
{
  svtkOpenGLGL2PSHelper* gl2ps = svtkOpenGLGL2PSHelper::GetInstance();
  if (gl2ps)
  {
    switch (gl2ps->GetActiveState())
    {
      case svtkOpenGLGL2PSHelper::Capture:
        return this->RenderGL2PS(vp, gl2ps);
      case svtkOpenGLGL2PSHelper::Background:
        return 0; // No render.
      case svtkOpenGLGL2PSHelper::Inactive:
        break; // normal render.
    }
  }

  return this->Superclass::RenderTranslucentPolygonalGeometry(vp);
}

//------------------------------------------------------------------------------
svtkOpenGLTextActor3D::svtkOpenGLTextActor3D() = default;

//------------------------------------------------------------------------------
svtkOpenGLTextActor3D::~svtkOpenGLTextActor3D() = default;

//------------------------------------------------------------------------------
int svtkOpenGLTextActor3D::RenderGL2PS(svtkViewport* vp, svtkOpenGLGL2PSHelper* gl2ps)
{
  svtkRenderer* ren = svtkRenderer::SafeDownCast(vp);
  if (!ren)
  {
    svtkWarningMacro("Viewport is not a renderer.");
    return 0;
  }

  // Get path
  std::string input = this->Input && this->Input[0] ? this->Input : "";
  svtkNew<svtkPath> textPath;

  svtkTextRenderer* tren = svtkTextRenderer::GetInstance();
  if (!tren)
  {
    svtkWarningMacro(<< "Cannot generate path data from 3D text string '" << input
                    << "': Text renderer unavailable.");
    return 0;
  }

  if (!tren->StringToPath(this->TextProperty, input, textPath, svtkTextActor3D::GetRenderedDPI()))
  {
    svtkWarningMacro(<< "Failed to generate path data from 3D text string '" << input
                    << "': StringToPath failed.");
    return 0;
  }

  // Get actor info
  svtkMatrix4x4* actorMatrix = this->GetMatrix();
  double actorBounds[6];
  this->GetBounds(actorBounds);
  double textPos[3] = { (actorBounds[1] + actorBounds[0]) * 0.5,
    (actorBounds[3] + actorBounds[2]) * 0.5, (actorBounds[5] + actorBounds[4]) * 0.5 };

  double* fgColord = this->TextProperty->GetColor();
  unsigned char fgColor[4] = { static_cast<unsigned char>(fgColord[0] * 255),
    static_cast<unsigned char>(fgColord[1] * 255), static_cast<unsigned char>(fgColord[2] * 255),
    static_cast<unsigned char>(this->TextProperty->GetOpacity() * 255) };

  // Draw the background quad as a path:
  if (this->TextProperty->GetBackgroundOpacity() > 0.f)
  {
    double* bgColord = this->TextProperty->GetBackgroundColor();
    unsigned char bgColor[4] = { static_cast<unsigned char>(bgColord[0] * 255),
      static_cast<unsigned char>(bgColord[1] * 255), static_cast<unsigned char>(bgColord[2] * 255),
      static_cast<unsigned char>(this->TextProperty->GetBackgroundOpacity() * 255) };

    // Get the camera so we can calculate an offset to place the background
    // behind the text.
    svtkCamera* cam = ren->GetActiveCamera();
    svtkMatrix4x4* mat =
      cam->GetCompositeProjectionTransformMatrix(ren->GetTiledAspectRatio(), 0., 1.);
    double forward[3] = { mat->GetElement(2, 0), mat->GetElement(2, 1), mat->GetElement(2, 2) };
    svtkMath::Normalize(forward);
    double bgPos[3] = { textPos[0] - (forward[0] * 0.0001), textPos[1] - (forward[1] * 0.0001),
      textPos[2] - (forward[2] * 0.0001) };

    svtkTextRenderer::Metrics metrics;
    if (tren->GetMetrics(this->TextProperty, input, metrics, svtkTextActor3D::GetRenderedDPI()))
    {
      svtkNew<svtkPath> bgPath;
      bgPath->InsertNextPoint(static_cast<double>(metrics.TopLeft.GetX()),
        static_cast<double>(metrics.TopLeft.GetY()), 0., svtkPath::MOVE_TO);
      bgPath->InsertNextPoint(static_cast<double>(metrics.TopRight.GetX()),
        static_cast<double>(metrics.TopRight.GetY()), 0., svtkPath::LINE_TO);
      bgPath->InsertNextPoint(static_cast<double>(metrics.BottomRight.GetX()),
        static_cast<double>(metrics.BottomRight.GetY()), 0., svtkPath::LINE_TO);
      bgPath->InsertNextPoint(static_cast<double>(metrics.BottomLeft.GetX()),
        static_cast<double>(metrics.BottomLeft.GetY()), 0., svtkPath::LINE_TO);
      bgPath->InsertNextPoint(static_cast<double>(metrics.TopLeft.GetX()),
        static_cast<double>(metrics.TopLeft.GetY()), 0., svtkPath::LINE_TO);

      std::ostringstream bgLabel;
      bgLabel << "svtkOpenGLTextActor3D::RenderGL2PS background for string: '" << input << "'.";
      gl2ps->Draw3DPath(bgPath, actorMatrix, bgPos, bgColor, ren, bgLabel.str().c_str());
    }
  }

  // Draw the text path:
  std::ostringstream label;
  label << "svtkOpenGLTextActor3D::RenderGL2PS path for string: '" << input << "'.";
  gl2ps->Draw3DPath(textPath, actorMatrix, textPos, fgColor, ren, label.str().c_str());

  return 1;
}
