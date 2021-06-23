/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFlagpoleLabel.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkFlagpoleLabel.h"

#include "svtkBoundingBox.h"
#include "svtkCamera.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkImageData.h"
#include "svtkLineSource.h"
#include "svtkMath.h"
#include "svtkObjectFactory.h"
#include "svtkPointData.h"
#include "svtkPolyData.h"
#include "svtkPolyDataMapper.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"
#include "svtkTextProperty.h"
#include "svtkTextRenderer.h"
#include "svtkTexture.h"

//------------------------------------------------------------------------------
svtkObjectFactoryNewMacro(svtkFlagpoleLabel);
svtkCxxSetObjectMacro(svtkFlagpoleLabel, TextProperty, svtkTextProperty);

//------------------------------------------------------------------------------
void svtkFlagpoleLabel::PrintSelf(std::ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "Input: " << (this->Input ? this->Input : "(nullptr)") << "\n"
     << indent << "TextProperty: " << this->TextProperty << "\n"
     << indent << "RenderedDPI: " << this->RenderedDPI << "\n"
     << indent << "InputMTime: " << this->InputMTime << "\n"
     << indent << "TextRenderer: " << this->TextRenderer << "\n"
     << indent << "BasePosition: " << this->BasePosition[0] << " " << this->BasePosition[1] << " "
     << this->BasePosition[2] << "\n"
     << indent << "TopPosition: " << this->TopPosition[0] << " " << this->TopPosition[1] << " "
     << this->TopPosition[2] << "\n";

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
void svtkFlagpoleLabel::SetInput(const char* in)
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
void svtkFlagpoleLabel::SetForceOpaque(bool opaque)
{
  this->PoleActor->SetForceOpaque(opaque);
  this->QuadActor->SetForceOpaque(opaque);
}

//------------------------------------------------------------------------------
bool svtkFlagpoleLabel::GetForceOpaque()
{
  return this->QuadActor->GetForceOpaque();
}

//------------------------------------------------------------------------------
void svtkFlagpoleLabel::ForceOpaqueOn()
{
  this->PoleActor->ForceOpaqueOn();
  this->QuadActor->ForceOpaqueOn();
}

//------------------------------------------------------------------------------
void svtkFlagpoleLabel::ForceOpaqueOff()
{
  this->PoleActor->ForceOpaqueOff();
  this->QuadActor->ForceOpaqueOff();
}

//------------------------------------------------------------------------------
void svtkFlagpoleLabel::SetForceTranslucent(bool trans)
{
  this->PoleActor->SetForceTranslucent(trans);
  this->QuadActor->SetForceTranslucent(trans);
}

//------------------------------------------------------------------------------
bool svtkFlagpoleLabel::GetForceTranslucent()
{
  return this->QuadActor->GetForceTranslucent();
}

//------------------------------------------------------------------------------
void svtkFlagpoleLabel::ForceTranslucentOn()
{
  this->PoleActor->ForceTranslucentOn();
  this->QuadActor->ForceTranslucentOn();
}

//------------------------------------------------------------------------------
void svtkFlagpoleLabel::ForceTranslucentOff()
{
  this->PoleActor->ForceTranslucentOff();
  this->QuadActor->ForceTranslucentOff();
}

//------------------------------------------------------------------------------
svtkTypeBool svtkFlagpoleLabel::HasTranslucentPolygonalGeometry()
{
  return this->QuadActor->HasTranslucentPolygonalGeometry();
}

//------------------------------------------------------------------------------
int svtkFlagpoleLabel::RenderOpaqueGeometry(svtkViewport* vp)
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
  this->PoleActor->RenderOpaqueGeometry(vp);
  return this->QuadActor->RenderOpaqueGeometry(vp);
}

//------------------------------------------------------------------------------
int svtkFlagpoleLabel::RenderTranslucentPolygonalGeometry(svtkViewport* vp)
{
  if (!this->InputIsValid() || !this->IsValid())
  {
    return 0;
  }

  this->PreRender();
  this->PoleActor->RenderTranslucentPolygonalGeometry(vp);
  return this->QuadActor->RenderTranslucentPolygonalGeometry(vp);
}

//------------------------------------------------------------------------------
void svtkFlagpoleLabel::ReleaseGraphicsResources(svtkWindow* win)
{
  this->RenderedRenderer = nullptr;
  this->Texture->ReleaseGraphicsResources(win);
  this->QuadMapper->ReleaseGraphicsResources(win);
  this->QuadActor->ReleaseGraphicsResources(win);
  this->PoleMapper->ReleaseGraphicsResources(win);
  this->PoleActor->ReleaseGraphicsResources(win);
}

//------------------------------------------------------------------------------
double* svtkFlagpoleLabel::GetBounds()
{
  if (this->RenderedRenderer)
  {
    this->UpdateInternals(this->RenderedRenderer);
  }

  svtkBoundingBox bb;
  bb.AddPoint(this->TopPosition);
  bb.AddPoint(this->BasePosition);
  if (this->IsValid())
  {
    double bounds[6];
    this->QuadActor->GetBounds(bounds);
    bb.AddBounds(bounds);
  }
  bb.GetBounds(this->Bounds);
  return this->Bounds;
}

//------------------------------------------------------------------------------
svtkFlagpoleLabel::svtkFlagpoleLabel()
  : Input(nullptr)
  , TextProperty(svtkTextProperty::New())
  , RenderedDPI(-1)
{
  this->LineSource->SetOutputPointsPrecision(svtkAlgorithm::DOUBLE_PRECISION);
  this->BasePosition[0] = 0.0;
  this->BasePosition[1] = 0.0;
  this->BasePosition[2] = 0.0;
  this->LineSource->SetPoint1(this->BasePosition);

  this->TopPosition[0] = 0.0;
  this->TopPosition[1] = 1.0;
  this->TopPosition[2] = 0.0;
  this->LineSource->SetPoint2(this->TopPosition);

  this->FlagSize = 1.0;

  // Connect internal rendering pipeline:
  this->Texture = svtkTexture::New();
  this->Texture->InterpolateOn();
  this->Texture->SetInputData(this->Image);
  this->QuadMapper->SetInputData(this->Quad);
  this->QuadActor->SetMapper(this->QuadMapper);
  this->QuadActor->SetTexture(this->Texture);

  // some reasonable defaults
  this->TextProperty->SetFontSize(32);
  this->TextProperty->SetFontFamilyToTimes();
  this->TextProperty->SetFrameWidth(3);
  this->TextProperty->FrameOn();
  this->TextRenderer->SetScaleToPowerOfTwo(false);

  this->PoleMapper->SetInputConnection(this->LineSource->GetOutputPort());
  this->PoleActor->SetMapper(this->PoleMapper);

  svtkNew<svtkPoints> points;
  points->SetDataTypeToDouble();
  svtkDoubleArray* quadPoints = svtkDoubleArray::FastDownCast(points->GetData());
  assert(quadPoints);
  quadPoints->SetNumberOfComponents(3);
  quadPoints->SetNumberOfTuples(4);
  this->Quad->SetPoints(points);

  svtkNew<svtkFloatArray> tc;
  tc->SetNumberOfComponents(2);
  tc->SetNumberOfTuples(4);
  tc->SetTypedComponent(0, 0, 0.0f);
  tc->SetTypedComponent(0, 1, 0.0f);
  tc->SetTypedComponent(1, 0, 1.0f);
  tc->SetTypedComponent(1, 1, 0.0f);
  tc->SetTypedComponent(2, 0, 1.0f);
  tc->SetTypedComponent(2, 1, 1.0f);
  tc->SetTypedComponent(3, 0, 0.0f);
  tc->SetTypedComponent(3, 1, 1.0f);
  tc->Modified();

  this->Quad->GetPointData()->SetTCoords(tc);

  svtkNew<svtkCellArray> cellArray;
  this->Quad->SetPolys(cellArray);
  svtkIdType quadIds[4] = { 0, 1, 2, 3 };
  this->Quad->InsertNextCell(SVTK_QUAD, 4, quadIds);
}

//------------------------------------------------------------------------------
svtkFlagpoleLabel::~svtkFlagpoleLabel()
{
  this->SetInput(nullptr);
  this->SetTextProperty(nullptr);
  this->RenderedRenderer = nullptr;
}

void svtkFlagpoleLabel::SetTopPosition(double x, double y, double z)
{
  if (this->TopPosition[0] == x && this->TopPosition[1] == y && this->TopPosition[2] == z)
  {
    return;
  }

  this->TopPosition[0] = x;
  this->TopPosition[1] = y;
  this->TopPosition[2] = z;

  this->LineSource->SetPoint2(x, y, z);

  this->Modified();
}

void svtkFlagpoleLabel::SetBasePosition(double x, double y, double z)
{
  if (this->BasePosition[0] == x && this->BasePosition[1] == y && this->BasePosition[2] == z)
  {
    return;
  }

  this->BasePosition[0] = x;
  this->BasePosition[1] = y;
  this->BasePosition[2] = z;

  this->LineSource->SetPoint1(x, y, z);

  this->Modified();
}

//------------------------------------------------------------------------------
bool svtkFlagpoleLabel::InputIsValid()
{
  return (this->Input != nullptr && this->Input[0] != '\0' && this->TextProperty != nullptr &&
    this->TextRenderer != nullptr);
}

//------------------------------------------------------------------------------
void svtkFlagpoleLabel::UpdateInternals(svtkRenderer* ren)
{
  this->PoleActor->SetProperty(this->GetProperty());
  this->QuadActor->SetProperty(this->GetProperty());

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
bool svtkFlagpoleLabel::TextureIsStale(svtkRenderer* ren)
{
  return (this->RenderedDPI != ren->GetRenderWindow()->GetDPI() ||
    this->Image->GetMTime() < this->InputMTime ||
    this->Image->GetMTime() < this->TextProperty->GetMTime());
}

//------------------------------------------------------------------------------
void svtkFlagpoleLabel::GenerateTexture(svtkRenderer* ren)
{
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
bool svtkFlagpoleLabel::QuadIsStale(svtkRenderer* ren)
{
  return (this->Quad->GetMTime() < this->GetMTime() ||
    this->Quad->GetMTime() < this->Image->GetMTime() || this->Quad->GetMTime() < ren->GetMTime() ||
    this->Quad->GetMTime() < ren->GetRenderWindow()->GetMTime() ||
    this->Quad->GetMTime() < ren->GetActiveCamera()->GetMTime());
}

//------------------------------------------------------------------------------
void svtkFlagpoleLabel::GenerateQuad(svtkRenderer* ren)
{
  svtkTextRenderer::Metrics metrics;
  if (!this->TextRenderer->GetMetrics(this->TextProperty, this->Input, metrics, this->RenderedDPI))
  {
    svtkErrorMacro("Error retrieving text metrics for string: " << this->Input);
    this->Invalidate();
    return;
  }

  // First figure out the texture coordinates for our quad (the easy part):

  // Actual size of the text in the texture
  int textSize[2] = { metrics.BoundingBox[1] - metrics.BoundingBox[0] + 1,
    metrics.BoundingBox[3] - metrics.BoundingBox[2] + 1 };

  // Now figure out the world coordinates for our quad (the hard part...):
  svtkDoubleArray* quadPoints = svtkDoubleArray::FastDownCast(this->Quad->GetPoints()->GetData());
  assert(quadPoints);

  // determine scaling, the default is 1.0 = 1000 texels across the screen
  double scale = this->FlagSize * 0.001;
  svtkCamera* cam = ren->GetActiveCamera();
  double pos[3];
  cam->GetPosition(pos);
  if (cam->GetParallelProjection())
  {
    double cscale = cam->GetParallelScale();
    scale = scale * cscale;
  }
  else
  {
    double vangle = cam->GetViewAngle();
    double dist = sqrt(svtkMath::Distance2BetweenPoints(pos, this->TopPosition));
    dist *= 2.0 * tan(svtkMath::RadiansFromDegrees(vangle / 2.0));
    scale = scale * dist;
  }

  // the middle bottom of the quad should be at TopPosition
  double height = textSize[1] * scale;
  double width = textSize[0] * scale;

  // compute the right and up basis vectors
  double right[3];
  double up[3];
  up[0] = this->TopPosition[0] - this->BasePosition[0];
  up[1] = this->TopPosition[1] - this->BasePosition[1];
  up[2] = this->TopPosition[2] - this->BasePosition[2];
  svtkMath::Normalize(up);

  // right is the cross of up and vpn
  double vpn[3] = { pos[0] - this->TopPosition[0], pos[1] - this->TopPosition[1],
    pos[2] - this->TopPosition[2] };
  svtkMath::Normalize(vpn);
  svtkMath::Cross(up, vpn, right);
  svtkMath::Normalize(right);

  double loc[3];
  loc[0] = this->TopPosition[0] - 0.5 * width * right[0];
  loc[1] = this->TopPosition[1] - 0.5 * width * right[1];
  loc[2] = this->TopPosition[2] - 0.5 * width * right[2];

  quadPoints->SetTypedComponent(0, 0, loc[0]);
  quadPoints->SetTypedComponent(0, 1, loc[1]);
  quadPoints->SetTypedComponent(0, 2, loc[2]);

  loc[0] += width * right[0];
  loc[1] += width * right[1];
  loc[2] += width * right[2];

  quadPoints->SetTypedComponent(1, 0, loc[0]);
  quadPoints->SetTypedComponent(1, 1, loc[1]);
  quadPoints->SetTypedComponent(1, 2, loc[2]);

  loc[0] += height * up[0];
  loc[1] += height * up[1];
  loc[2] += height * up[2];

  quadPoints->SetTypedComponent(2, 0, loc[0]);
  quadPoints->SetTypedComponent(2, 1, loc[1]);
  quadPoints->SetTypedComponent(2, 2, loc[2]);

  loc[0] -= width * right[0];
  loc[1] -= width * right[1];
  loc[2] -= width * right[2];

  quadPoints->SetTypedComponent(3, 0, loc[0]);
  quadPoints->SetTypedComponent(3, 1, loc[1]);
  quadPoints->SetTypedComponent(3, 2, loc[2]);

  quadPoints->Modified();
}

//------------------------------------------------------------------------------
void svtkFlagpoleLabel::Invalidate()
{
  this->Image->Initialize();
}

//------------------------------------------------------------------------------
bool svtkFlagpoleLabel::IsValid()
{
  return this->Image->GetNumberOfPoints() > 0;
}

//------------------------------------------------------------------------------
void svtkFlagpoleLabel::PreRender()
{
  // The internal actor needs to share property keys. This allows depth peeling
  // etc to work.
  this->PoleActor->SetPropertyKeys(this->GetPropertyKeys());
  this->QuadActor->SetPropertyKeys(this->GetPropertyKeys());
}
