//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/WorldAnnotatorGL.h>

#include <svtkm/Matrix.h>
#include <svtkm/rendering/BitmapFontFactory.h>
#include <svtkm/rendering/Color.h>
#include <svtkm/rendering/DecodePNG.h>
#include <svtkm/rendering/MatrixHelpers.h>
#include <svtkm/rendering/Scene.h>

#include <svtkm/rendering/internal/OpenGLHeaders.h>

namespace svtkm
{
namespace rendering
{

WorldAnnotatorGL::WorldAnnotatorGL(const svtkm::rendering::Canvas* canvas)
  : WorldAnnotator(canvas)
{
}

WorldAnnotatorGL::~WorldAnnotatorGL()
{
}

void WorldAnnotatorGL::AddLine(const svtkm::Vec3f_64& point0,
                               const svtkm::Vec3f_64& point1,
                               svtkm::Float32 lineWidth,
                               const svtkm::rendering::Color& color,
                               bool inFront) const
{
  if (inFront)
  {
    glDepthRange(-.0001, .9999);
  }

  glDisable(GL_LIGHTING);
  glEnable(GL_DEPTH_TEST);

  glColor3f(color.Components[0], color.Components[1], color.Components[2]);

  glLineWidth(lineWidth);

  glBegin(GL_LINES);
  glVertex3d(point0[0], point0[1], point0[2]);
  glVertex3d(point1[0], point1[1], point1[2]);
  glEnd();

  if (inFront)
  {
    glDepthRange(0, 1);
  }
}

void WorldAnnotatorGL::AddText(const svtkm::Vec3f_32& origin,
                               const svtkm::Vec3f_32& right,
                               const svtkm::Vec3f_32& up,
                               svtkm::Float32 scale,
                               const svtkm::Vec2f_32& anchor,
                               const svtkm::rendering::Color& color,
                               const std::string& text,
                               const svtkm::Float32 svtkmNotUsed(depth)) const
{

  svtkm::Vec3f_32 n = svtkm::Cross(right, up);
  svtkm::Normalize(n);

  svtkm::Matrix<svtkm::Float32, 4, 4> m;
  m = MatrixHelpers::WorldMatrix(origin, right, up, n);

  svtkm::Float32 ogl[16];
  MatrixHelpers::CreateOGLMatrix(m, ogl);
  glPushMatrix();
  glMultMatrixf(ogl);
  glColor3f(color.Components[0], color.Components[1], color.Components[2]);
  this->RenderText(scale, anchor[0], anchor[1], text);
  glPopMatrix();
}

void WorldAnnotatorGL::RenderText(svtkm::Float32 scale,
                                  svtkm::Float32 anchorx,
                                  svtkm::Float32 anchory,
                                  std::string text) const
{
  if (!this->FontTexture.Valid())
  {
    // When we load a font, we save a reference to it for the next time we
    // use it. Although technically we are changing the state, the logical
    // state does not change, so we go ahead and do it in this const
    // function.
    svtkm::rendering::WorldAnnotatorGL* self = const_cast<svtkm::rendering::WorldAnnotatorGL*>(this);
    self->Font = BitmapFontFactory::CreateLiberation2Sans();
    const std::vector<unsigned char>& rawpngdata = this->Font.GetRawImageData();

    std::vector<unsigned char> rgba;
    unsigned long width, height;
    int error = svtkm::rendering::DecodePNG(rgba, width, height, &rawpngdata[0], rawpngdata.size());
    if (error != 0)
    {
      return;
    }

    self->FontTexture.CreateAlphaFromRGBA(int(width), int(height), rgba);
  }

  this->FontTexture.Enable();

  glDepthMask(GL_FALSE);
  glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
  glEnable(GL_BLEND);
  glTexEnvf(GL_TEXTURE_ENV, GL_TEXTURE_ENV_MODE, GL_MODULATE);
  glDisable(GL_LIGHTING);
  //glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, -.5);

  glBegin(GL_QUADS);

  svtkm::Float32 textwidth = this->Font.GetTextWidth(text);

  svtkm::Float32 fx = -(.5f + .5f * anchorx) * textwidth;
  svtkm::Float32 fy = -(.5f + .5f * anchory);
  svtkm::Float32 fz = 0;
  for (unsigned int i = 0; i < text.length(); ++i)
  {
    char c = text[i];
    char nextchar = (i < text.length() - 1) ? text[i + 1] : 0;

    svtkm::Float32 vl, vr, vt, vb;
    svtkm::Float32 tl, tr, tt, tb;
    this->Font.GetCharPolygon(c, fx, fy, vl, vr, vt, vb, tl, tr, tt, tb, nextchar);

    glTexCoord2f(tl, 1.f - tt);
    glVertex3f(scale * vl, scale * vt, fz);

    glTexCoord2f(tl, 1.f - tb);
    glVertex3f(scale * vl, scale * vb, fz);

    glTexCoord2f(tr, 1.f - tb);
    glVertex3f(scale * vr, scale * vb, fz);

    glTexCoord2f(tr, 1.f - tt);
    glVertex3f(scale * vr, scale * vt, fz);
  }

  glEnd();

  this->FontTexture.Disable();

  //glTexEnvf(GL_TEXTURE_FILTER_CONTROL, GL_TEXTURE_LOD_BIAS, 0);
  glDepthMask(GL_TRUE);
  glDisable(GL_ALPHA_TEST);
}
}
} // namespace svtkm::rendering
