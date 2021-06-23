/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBillboardTextActor3D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkBillboardTextActor3D.h"

#include "svtkActor.h"
#include "svtkCamera.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkMath.h"
#include "svtkMatrix4x4.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPoints.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkTextProperty.h"
#include "svtkTextRenderer.h"
#include "svtkTexture.h"

#include <algorithm>
#include <cassert>
#include <cmath>

// Define to print debugging info:
//#define DEBUG_BTA3D

namespace
{

#ifdef DEBUG_BTA3D
std::ostream& PrintCoords(
  const std::string& label, const double w[4], const double d[4], std::ostream& out)
{
  out << label << "\n-WorldCoord: " << w[0] << " " << w[1] << " " << w[2] << " " << w[3]
      << "\n-DispCoord:  " << d[0] << " " << d[1] << " " << d[2] << " " << d[3] << "\n";
  return out;
}
#endif // DEBUG_BTA3D

// Used to convert WorldCoords <--> DisplayCoords.
// Required because svtkCoordinate doesn't support depth values for DC.
// Here, we use homogeneous 3D coordinates. This is so a DC's x/y values may be
// modified and passed back to DisplayToWorld to produce an World-space point
// at the same view depth as another.
class FastDepthAwareCoordinateConverter
{
public:
  explicit FastDepthAwareCoordinateConverter(svtkRenderer* ren);

  void WorldToDisplay(const double wc[4], double dc[4]) const;
  void DisplayToWorld(const double dc[4], double wc[4]) const;

private:
  double MVP[16];    // Model * View * Proj matrix
  double InvMVP[16]; // Inverse Model * View * Proj matrix
  double Viewport[4];
  double NormalizedViewport[4];
  double ViewportSize[2];
  double DisplayOffset[2];
};

FastDepthAwareCoordinateConverter::FastDepthAwareCoordinateConverter(svtkRenderer* ren)
{
  svtkCamera* cam = ren->GetActiveCamera();
  assert(cam); // We check in the text actor for this.

  // figure out the same aspect ratio used by the render engine
  // (see svtkOpenGLCamera::Render())
  int lowerLeft[2];
  int usize, vsize;
  double aspect1[2];
  double aspect2[2];
  ren->GetTiledSizeAndOrigin(&usize, &vsize, lowerLeft, lowerLeft + 1);
  ren->ComputeAspect();
  ren->GetAspect(aspect1);
  ren->svtkViewport::ComputeAspect();
  ren->svtkViewport::GetAspect(aspect2);
  double aspectModification = (aspect1[0] * aspect2[1]) / (aspect1[1] * aspect2[0]);
  double aspect = aspectModification * usize / vsize;

  // Build AMVP/InvAMVP
  svtkMatrix4x4::DeepCopy(this->MVP, cam->GetCompositeProjectionTransformMatrix(aspect, -1, 1));
  svtkMatrix4x4::Invert(this->MVP, this->InvMVP);

  // Various other bits needed for conversion
  const int* size = ren->GetSize();
  this->ViewportSize[0] = size[0];
  this->ViewportSize[1] = size[1];

  ren->GetViewport(this->Viewport);

  double tileViewPort[4];
  ren->GetRenderWindow()->GetTileViewport(tileViewPort);

  this->NormalizedViewport[0] = std::max(this->Viewport[0], tileViewPort[0]);
  this->NormalizedViewport[1] = std::max(this->Viewport[1], tileViewPort[1]);
  this->NormalizedViewport[2] = std::min(this->Viewport[2], tileViewPort[2]);
  this->NormalizedViewport[3] = std::min(this->Viewport[3], tileViewPort[3]);

  size = ren->GetRenderWindow()->GetSize();
  this->DisplayOffset[0] = this->Viewport[0] * size[0] + 0.5;
  this->DisplayOffset[1] = this->Viewport[1] * size[1] + 0.5;
}

void FastDepthAwareCoordinateConverter::WorldToDisplay(const double wc[4], double dc[4]) const
{
  // This is adapted from svtkCoordinate's world to display conversion. It is
  // extended to handle a depth value for the display coordinate.

  // svtkRenderer::WorldToView
  const double* x = this->MVP; // Alias so we can write math easier
  dc[0] = wc[0] * x[0] + wc[1] * x[1] + wc[2] * x[2] + wc[3] * x[3];
  dc[1] = wc[0] * x[4] + wc[1] * x[5] + wc[2] * x[6] + wc[3] * x[7];
  dc[2] = wc[0] * x[8] + wc[1] * x[9] + wc[2] * x[10] + wc[3] * x[11];
  dc[3] = wc[0] * x[12] + wc[1] * x[13] + wc[2] * x[14] + wc[3] * x[15];

  double invW = 1. / dc[3];
  dc[0] *= invW;
  dc[1] *= invW;
  dc[2] *= invW;

  // svtkViewport::ViewToNormalizedViewport
  dc[0] = this->NormalizedViewport[0] +
    ((dc[0] + 1.) / 2.) * (this->NormalizedViewport[2] - this->NormalizedViewport[0]);
  dc[1] = this->NormalizedViewport[1] +
    ((dc[1] + 1.) / 2.) * (this->NormalizedViewport[3] - this->NormalizedViewport[1]);
  dc[0] = (dc[0] - this->Viewport[0]) / (this->Viewport[2] - this->Viewport[0]);
  dc[1] = (dc[1] - this->Viewport[1]) / (this->Viewport[3] - this->Viewport[1]);

  // svtkViewport::NormalizedViewportToViewport
  dc[0] *= this->ViewportSize[0] - 1.;
  dc[1] *= this->ViewportSize[1] - 1.;

  // svtkViewport::ViewportToNormalizedDisplay
  // svtkViewport::NormalizedDisplayToDisplay
  dc[0] += this->DisplayOffset[0];
  dc[1] += this->DisplayOffset[1];
}

void FastDepthAwareCoordinateConverter::DisplayToWorld(const double dc[4], double wc[4]) const
{
  // Just the inverse of WorldToDisplay....

  // Make a copy of the input so we can modify it in place before the matrix mul
  double t[4] = { dc[0], dc[1], dc[2], dc[3] };
  t[0] -= this->DisplayOffset[0];
  t[1] -= this->DisplayOffset[1];

  t[0] /= this->ViewportSize[0] - 1.;
  t[1] /= this->ViewportSize[1] - 1.;

  t[0] = t[0] * (this->Viewport[2] - this->Viewport[0]) + this->Viewport[0];
  t[1] = t[1] * (this->Viewport[3] - this->Viewport[1]) + this->Viewport[1];

  t[0] = 2. * (t[0] - this->NormalizedViewport[0]) /
      (this->NormalizedViewport[2] - this->NormalizedViewport[0]) -
    1.;
  t[1] = 2. * (t[1] - this->NormalizedViewport[1]) /
      (this->NormalizedViewport[3] - this->NormalizedViewport[1]) -
    1.;

  t[0] *= t[3];
  t[1] *= t[3];
  t[2] *= t[3];

  const double* x = this->InvMVP; // Alias so we can write math easier
  wc[0] = t[0] * x[0] + t[1] * x[1] + t[2] * x[2] + t[3] * x[3];
  wc[1] = t[0] * x[4] + t[1] * x[5] + t[2] * x[6] + t[3] * x[7];
  wc[2] = t[0] * x[8] + t[1] * x[9] + t[2] * x[10] + t[3] * x[11];
  wc[3] = t[0] * x[12] + t[1] * x[13] + t[2] * x[14] + t[3] * x[15];
}

} // end anon namespace

//------------------------------------------------------------------------------
svtkObjectFactoryNewMacro(svtkBillboardTextActor3D);
svtkCxxSetObjectMacro(svtkBillboardTextActor3D, TextProperty, svtkTextProperty);

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input: " << (this->Input ? this->Input : "(nullptr)") << "\n"
     << indent << "TextProperty: " << this->TextProperty << "\n"
     << indent << "RenderedDPI: " << this->RenderedDPI << "\n"
     << indent << "InputMTime: " << this->InputMTime << "\n"
     << indent << "TextRenderer: " << this->TextRenderer << "\n"
     << indent << "AnchorDC: " << this->AnchorDC[0] << " " << this->AnchorDC[1] << " "
     << this->AnchorDC[2] << "\n"
     << indent << "DisplayOffset: " << this->DisplayOffset[0] << " " << this->DisplayOffset[1]
     << "\n";

  os << indent << "Image:\n";
  this->Image->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Texture:\n";
  this->Texture->PrintSelf(os, indent.GetNextIndent());

  os << indent << "Quad:\n";
  this->Quad->PrintSelf(os, indent.GetNextIndent());

  os << indent << "QuadMapper:\n";
  this->QuadMapper->PrintSelf(os, indent.GetNextIndent());

  os << indent << "QuadActor:\n";
  this->QuadActor->PrintSelf(os, indent.GetNextIndent());
}

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::SetInput(const char* in)
{
  // Adapted svtkSetStringMacro to also mark InputMTime as modified:
  if ((this->Input == nullptr && in == nullptr) ||
    (this->Input && in && strcmp(this->Input, in) == 0))
  {
    return;
  }

  delete[] this->Input;
  if (in)
  {
    size_t n = strlen(in) + 1;
    this->Input = new char[n];
    std::copy(in, in + n, this->Input);
  }
  else
  {
    this->Input = nullptr;
  }
  this->Modified();
  this->InputMTime.Modified();
}

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::SetForceOpaque(bool opaque)
{
  this->QuadActor->SetForceOpaque(opaque);
}

//------------------------------------------------------------------------------
bool svtkBillboardTextActor3D::GetForceOpaque()
{
  return this->QuadActor->GetForceOpaque();
}

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::ForceOpaqueOn()
{
  this->QuadActor->ForceOpaqueOn();
}

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::ForceOpaqueOff()
{
  this->QuadActor->ForceOpaqueOff();
}

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::SetForceTranslucent(bool trans)
{
  this->QuadActor->SetForceTranslucent(trans);
}

//------------------------------------------------------------------------------
bool svtkBillboardTextActor3D::GetForceTranslucent()
{
  return this->QuadActor->GetForceTranslucent();
}

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::ForceTranslucentOn()
{
  this->QuadActor->ForceTranslucentOn();
}

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::ForceTranslucentOff()
{
  this->QuadActor->ForceTranslucentOff();
}

//------------------------------------------------------------------------------
svtkTypeBool svtkBillboardTextActor3D::HasTranslucentPolygonalGeometry()
{
  return this->QuadActor->HasTranslucentPolygonalGeometry();
}

//------------------------------------------------------------------------------
int svtkBillboardTextActor3D::RenderOpaqueGeometry(svtkViewport* vp)
{
  if (!this->InputIsValid())
  {
    return 0;
  }

  svtkRenderer* ren = svtkRenderer::SafeDownCast(vp);
  if (!ren || ren->GetActiveCamera() == nullptr)
  {
    svtkErrorMacro("Viewport is not a renderer, or missing a camera.");
    this->Invalidate();
    return 0;
  }

  // Cache for updating bounds between renders (#17233):
  this->RenderedRenderer = ren;

  // Alert OpenGL1 GL2PS export that this prop needs special handling:
  if (ren->GetRenderWindow() && ren->GetRenderWindow()->GetCapturingGL2PSSpecialProps())
  {
    ren->CaptureGL2PSSpecialProp(this);
  }

  this->UpdateInternals(ren);

  this->PreRender();
  return this->QuadActor->RenderOpaqueGeometry(vp);
}

//------------------------------------------------------------------------------
int svtkBillboardTextActor3D::RenderTranslucentPolygonalGeometry(svtkViewport* vp)
{
  if (!this->InputIsValid() || !this->IsValid())
  {
    return 0;
  }

#ifdef DEBUG_BTA3D
  std::cerr << "Rendering billboard text: " << this->Input << std::endl;
#endif // DEBUG_BTA3D

  this->PreRender();
  return this->QuadActor->RenderTranslucentPolygonalGeometry(vp);
}

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::ReleaseGraphicsResources(svtkWindow* win)
{
  this->RenderedRenderer = nullptr;
  this->Texture->ReleaseGraphicsResources(win);
  this->QuadMapper->ReleaseGraphicsResources(win);
  this->QuadActor->ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
double* svtkBillboardTextActor3D::GetBounds()
{
  if (this->RenderedRenderer)
  {
    this->UpdateInternals(this->RenderedRenderer);
  }

  if (this->IsValid())
  {
    this->QuadActor->GetBounds(this->Bounds);
  }
  else
  { // If the actor isn't prepped, return the actor position as the bounds.
    // We don't know the true extents until we see a camera.
    this->Bounds[0] = this->Bounds[1] = this->Position[0];
    this->Bounds[2] = this->Bounds[3] = this->Position[1];
    this->Bounds[4] = this->Bounds[5] = this->Position[2];
  }
  return this->Bounds;
}

//------------------------------------------------------------------------------
svtkBillboardTextActor3D::svtkBillboardTextActor3D()
  : Input(nullptr)
  , TextProperty(svtkTextProperty::New())
  , RenderedDPI(-1)
{
  std::fill(this->AnchorDC, this->AnchorDC + 3, 0.);
  std::fill(this->DisplayOffset, this->DisplayOffset + 2, 0);

  // Connect internal rendering pipeline:
  this->Texture->InterpolateOff();
  this->Texture->SetInputData(this->Image);
  this->QuadMapper->SetInputData(this->Quad);
  this->QuadActor->SetMapper(this->QuadMapper);
  this->QuadActor->SetTexture(this->Texture);

  svtkNew<svtkPoints> points;
  points->SetDataTypeToFloat();
  svtkFloatArray* quadPoints = svtkFloatArray::FastDownCast(points->GetData());
  assert(quadPoints);
  quadPoints->SetNumberOfComponents(3);
  quadPoints->SetNumberOfTuples(4);
  this->Quad->SetPoints(points);

  svtkNew<svtkFloatArray> tc;
  tc->SetNumberOfComponents(2);
  tc->SetNumberOfTuples(4);
  this->Quad->GetPointData()->SetTCoords(tc);

  svtkNew<svtkCellArray> cellArray;
  this->Quad->SetPolys(cellArray);
  svtkIdType quadIds[4] = { 0, 1, 2, 3 };
  this->Quad->InsertNextCell(SVTK_QUAD, 4, quadIds);
}

//------------------------------------------------------------------------------
svtkBillboardTextActor3D::~svtkBillboardTextActor3D()
{
  this->SetInput(nullptr);
  this->SetTextProperty(nullptr);
  this->RenderedRenderer = nullptr;
}

//------------------------------------------------------------------------------
bool svtkBillboardTextActor3D::InputIsValid()
{
  return (this->Input != nullptr && this->Input[0] != '\0' && this->TextProperty != nullptr &&
    this->TextRenderer != nullptr);
}

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::UpdateInternals(svtkRenderer* ren)
{
  if (this->TextureIsStale(ren))
  {
    this->GenerateTexture(ren);
  }

  if (this->IsValid() && this->QuadIsStale(ren))
  {
    this->GenerateQuad(ren);
  }
}

//------------------------------------------------------------------------------
bool svtkBillboardTextActor3D::TextureIsStale(svtkRenderer* ren)
{
  return (this->RenderedDPI != ren->GetRenderWindow()->GetDPI() ||
    this->Image->GetMTime() < this->InputMTime ||
    this->Image->GetMTime() < this->TextProperty->GetMTime());
}

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::GenerateTexture(svtkRenderer* ren)
{
#ifdef DEBUG_BTA3D
  std::cerr << "Generating texture for string: " << this->Input << std::endl;
#endif // DEBUG_BTA3D

  int dpi = ren->GetRenderWindow()->GetDPI();

  if (!this->TextRenderer->RenderString(this->TextProperty, this->Input, this->Image, nullptr, dpi))
  {
    svtkErrorMacro("Error rendering text string: " << this->Input);
    this->Invalidate();
    return;
  }

  this->RenderedDPI = dpi;
}

//------------------------------------------------------------------------------
bool svtkBillboardTextActor3D::QuadIsStale(svtkRenderer* ren)
{
  return (this->Quad->GetMTime() < this->GetMTime() ||
    this->Quad->GetMTime() < this->Image->GetMTime() || this->Quad->GetMTime() < ren->GetMTime() ||
    this->Quad->GetMTime() < ren->GetRenderWindow()->GetMTime() ||
    this->Quad->GetMTime() < ren->GetActiveCamera()->GetMTime());
}

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::GenerateQuad(svtkRenderer* ren)
{
#ifdef DEBUG_BTA3D
  std::cerr << "Generating quad for string: " << this->Input << std::endl;
#endif // DEBUG_BTA3D

  svtkTextRenderer::Metrics metrics;
  if (!this->TextRenderer->GetMetrics(this->TextProperty, this->Input, metrics, this->RenderedDPI))
  {
    svtkErrorMacro("Error retrieving text metrics for string: " << this->Input);
    this->Invalidate();
    return;
  }

  // First figure out the texture coordinates for our quad (the easy part):

  // Size of the full texture
  int textureSize[3];
  this->Image->GetDimensions(textureSize);

  // Actual size of the text in the texture (in case we allocated NPOT)
  int textSize[2] = { metrics.BoundingBox[1] - metrics.BoundingBox[0] + 1,
    metrics.BoundingBox[3] - metrics.BoundingBox[2] + 1 };

  // Maximum texture coordinate:
  float tcMax[2] = { textSize[0] / static_cast<float>(textureSize[0]),
    textSize[1] / static_cast<float>(textureSize[1]) };

  svtkFloatArray* tc = svtkFloatArray::FastDownCast(this->Quad->GetPointData()->GetTCoords());
  assert(tc);
  tc->SetNumberOfComponents(2);
  tc->SetNumberOfTuples(4);
  tc->SetTypedComponent(0, 0, 0.f);
  tc->SetTypedComponent(0, 1, 0.f);
  tc->SetTypedComponent(1, 0, 0.f);
  tc->SetTypedComponent(1, 1, tcMax[1]);
  tc->SetTypedComponent(2, 0, tcMax[0]);
  tc->SetTypedComponent(2, 1, tcMax[1]);
  tc->SetTypedComponent(3, 0, tcMax[0]);
  tc->SetTypedComponent(3, 1, 0.f);
  tc->Modified();

  // Now figure out the world coordinates for our quad (the hard part...):
  svtkFloatArray* quadPoints = svtkFloatArray::FastDownCast(this->Quad->GetPoints()->GetData());
  assert(quadPoints);

  // This takes care of projecting/unprojecting the points:
  FastDepthAwareCoordinateConverter conv(ren);

  // Convert our anchor position to DC:
  double anchorWC[4];
  double anchorDC[4];
  this->GetPosition(anchorWC);
  anchorWC[3] = 1.;
  conv.WorldToDisplay(anchorWC, anchorDC);

  // Round down to an exact pixel:
  anchorDC[0] = std::floor(anchorDC[0]);
  anchorDC[1] = std::floor(anchorDC[1]);

  // Apply the requested offset:
  anchorDC[0] += static_cast<double>(this->DisplayOffset[0]);
  anchorDC[1] += static_cast<double>(this->DisplayOffset[1]);

  // Cached for OpenGL2 GL2PS exports:
  this->AnchorDC[0] = anchorDC[0];
  this->AnchorDC[1] = anchorDC[1];
  this->AnchorDC[2] = anchorDC[2];

#ifdef DEBUG_BTA3D
  PrintCoords("Anchor Point", anchorWC, anchorDC, std::cerr);
  double sanityWC[4]; // convert back to make sure they match (note rounding)
  conv.DisplayToWorld(anchorDC, sanityWC);
  PrintCoords("Anchor Sanity Check", sanityWC, anchorDC, std::cerr);
#endif // DEBUG_BTA3D

  // Find the DCs that correspond to the texture coordinates we used and
  // convert them to WCs:
  double tmpWC[4];
  double tmpDC[4];
  std::copy(anchorDC, anchorDC + 4, tmpDC);

  // First point
  tmpDC[0] += metrics.BoundingBox[0];
  tmpDC[1] += metrics.BoundingBox[2];
  conv.DisplayToWorld(tmpDC, tmpWC);

#ifdef DEBUG_BTA3D
  PrintCoords("First Point", tmpWC, tmpDC, std::cerr);
#endif // DEBUG_BTA3D

  quadPoints->SetTypedComponent(0, 0, static_cast<float>(tmpWC[0]));
  quadPoints->SetTypedComponent(0, 1, static_cast<float>(tmpWC[1]));
  quadPoints->SetTypedComponent(0, 2, static_cast<float>(tmpWC[2]));

  // Second point
  tmpDC[1] += textSize[1];
  conv.DisplayToWorld(tmpDC, tmpWC);

#ifdef DEBUG_BTA3D
  PrintCoords("Second Point", tmpWC, tmpDC, std::cerr);
#endif // DEBUG_BTA3D

  quadPoints->SetTypedComponent(1, 0, static_cast<float>(tmpWC[0]));
  quadPoints->SetTypedComponent(1, 1, static_cast<float>(tmpWC[1]));
  quadPoints->SetTypedComponent(1, 2, static_cast<float>(tmpWC[2]));

  // Third point
  tmpDC[0] += textSize[0];
  conv.DisplayToWorld(tmpDC, tmpWC);

#ifdef DEBUG_BTA3D
  PrintCoords("Third Point", tmpWC, tmpDC, std::cerr);
#endif // DEBUG_BTA3D

  quadPoints->SetTypedComponent(2, 0, static_cast<float>(tmpWC[0]));
  quadPoints->SetTypedComponent(2, 1, static_cast<float>(tmpWC[1]));
  quadPoints->SetTypedComponent(2, 2, static_cast<float>(tmpWC[2]));

  // Fourth point
  tmpDC[1] -= textSize[1];
  conv.DisplayToWorld(tmpDC, tmpWC);

#ifdef DEBUG_BTA3D
  PrintCoords("Fourth Point", tmpWC, tmpDC, std::cerr) << std::endl;
#endif // DEBUG_BTA3D

  quadPoints->SetTypedComponent(3, 0, static_cast<float>(tmpWC[0]));
  quadPoints->SetTypedComponent(3, 1, static_cast<float>(tmpWC[1]));
  quadPoints->SetTypedComponent(3, 2, static_cast<float>(tmpWC[2]));

  quadPoints->Modified();
}

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::Invalidate()
{
  this->Image->Initialize();
}

//------------------------------------------------------------------------------
bool svtkBillboardTextActor3D::IsValid()
{
  return this->Image->GetNumberOfPoints() > 0;
}

//------------------------------------------------------------------------------
void svtkBillboardTextActor3D::PreRender()
{
  // The internal actor needs to share property keys. This allows depth peeling
  // etc to work.
  this->QuadActor->SetPropertyKeys(this->GetPropertyKeys());
}
