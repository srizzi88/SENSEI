//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/TextAnnotationBillboard.h>

#include <svtkm/Matrix.h>

namespace svtkm
{
namespace rendering
{

TextAnnotationBillboard::TextAnnotationBillboard(const std::string& text,
                                                 const svtkm::rendering::Color& color,
                                                 svtkm::Float32 scalar,
                                                 const svtkm::Vec3f_32& position,
                                                 svtkm::Float32 angleDegrees)
  : TextAnnotation(text, color, scalar)
  , Position(position)
  , Angle(angleDegrees)
{
}

TextAnnotationBillboard::~TextAnnotationBillboard()
{
}

void TextAnnotationBillboard::SetPosition(const svtkm::Vec3f_32& position)
{
  this->Position = position;
}

void TextAnnotationBillboard::SetPosition(svtkm::Float32 xpos,
                                          svtkm::Float32 ypos,
                                          svtkm::Float32 zpos)
{
  this->SetPosition(svtkm::make_Vec(xpos, ypos, zpos));
}

void TextAnnotationBillboard::Render(const svtkm::rendering::Camera& camera,
                                     const svtkm::rendering::WorldAnnotator& worldAnnotator,
                                     svtkm::rendering::Canvas& canvas) const
{
  using MatrixType = svtkm::Matrix<svtkm::Float32, 4, 4>;
  using VectorType = svtkm::Vec3f_32;

  MatrixType viewMatrix = camera.CreateViewMatrix();
  MatrixType projectionMatrix =
    camera.CreateProjectionMatrix(canvas.GetWidth(), canvas.GetHeight());

  VectorType screenPos = svtkm::Transform3DPointPerspective(
    svtkm::MatrixMultiply(projectionMatrix, viewMatrix), this->Position);

  canvas.SetViewToScreenSpace(camera, true);
  MatrixType translateMatrix =
    svtkm::Transform3DTranslate(screenPos[0], screenPos[1], -screenPos[2]);

  svtkm::Float32 windowAspect = svtkm::Float32(canvas.GetWidth()) / svtkm::Float32(canvas.GetHeight());

  MatrixType scaleMatrix = svtkm::Transform3DScale(1.f / windowAspect, 1.f, 1.f);

  MatrixType viewportMatrix;
  svtkm::MatrixIdentity(viewportMatrix);
  //if view type == 2D?
  {
    svtkm::Float32 vl, vr, vb, vt;
    camera.GetRealViewport(canvas.GetWidth(), canvas.GetHeight(), vl, vr, vb, vt);
    svtkm::Float32 xs = (vr - vl);
    svtkm::Float32 ys = (vt - vb);
    viewportMatrix = svtkm::Transform3DScale(2.f / xs, 2.f / ys, 1.f);
  }

  MatrixType rotateMatrix = svtkm::Transform3DRotateZ(this->Angle * svtkm::Pi_180f());

  svtkm::Matrix<svtkm::Float32, 4, 4> fullTransformMatrix = svtkm::MatrixMultiply(
    translateMatrix,
    svtkm::MatrixMultiply(scaleMatrix, svtkm::MatrixMultiply(viewportMatrix, rotateMatrix)));

  VectorType origin = svtkm::Transform3DPointPerspective(fullTransformMatrix, VectorType(0, 0, 0));
  VectorType right = svtkm::Transform3DVector(fullTransformMatrix, VectorType(1, 0, 0));
  VectorType up = svtkm::Transform3DVector(fullTransformMatrix, VectorType(0, 1, 0));

  // scale depth from (1, -1) to (0, 1);
  svtkm::Float32 depth = screenPos[2] * .5f + .5f;
  worldAnnotator.AddText(
    origin, right, up, this->Scale, this->Anchor, this->TextColor, this->Text, depth);

  canvas.SetViewToWorldSpace(camera, true);
}
}
} // svtkm::rendering
