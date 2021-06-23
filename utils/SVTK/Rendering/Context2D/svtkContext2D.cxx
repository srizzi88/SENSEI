/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContext2D.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkContext2D.h"

#include "svtkBrush.h"
#include "svtkContextDevice2D.h"
#include "svtkFloatArray.h"
#include "svtkPen.h"
#include "svtkPoints2D.h"
#include "svtkTextProperty.h"
#include "svtkTransform2D.h"
#include "svtkUnsignedCharArray.h"
#include "svtkVector.h"

#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkRenderer.h"

#include <cassert>

//-----------------------------------------------------------------------------
svtkStandardNewMacro(svtkContext2D);

//-----------------------------------------------------------------------------
bool svtkContext2D::Begin(svtkContextDevice2D* device)
{
  if (this->Device == device)
  {
    // Handle the case where the same device is set multiple times
    return true;
  }
  else if (this->Device)
  {
    this->Device->Delete();
  }
  this->Device = device;
  this->Device->Register(this);
  this->Modified();
  return true;
}

//-----------------------------------------------------------------------------
bool svtkContext2D::End()
{
  if (this->Device)
  {
    this->Device->End();
    this->Device->Delete();
    this->Device = nullptr;
    this->Modified();
    return true;
  }
  return true;
}

// ----------------------------------------------------------------------------
bool svtkContext2D::GetBufferIdMode() const
{
  return this->BufferId != nullptr;
}

// ----------------------------------------------------------------------------
void svtkContext2D::BufferIdModeBegin(svtkAbstractContextBufferId* bufferId)
{
  assert("pre: not_yet" && !this->GetBufferIdMode());
  assert("pre: bufferId_exists" && bufferId != nullptr);

  this->BufferId = bufferId;
  this->Device->BufferIdModeBegin(bufferId);

  assert("post: started" && this->GetBufferIdMode());
}

// ----------------------------------------------------------------------------
void svtkContext2D::BufferIdModeEnd()
{
  assert("pre: started" && this->GetBufferIdMode());

  this->Device->BufferIdModeEnd();
  this->BufferId = nullptr;

  assert("post: done" && !this->GetBufferIdMode());
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawLine(float x1, float y1, float x2, float y2)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  float x[] = { x1, y1, x2, y2 };
  this->Device->DrawPoly(&x[0], 2);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawLine(float p[4])
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  this->Device->DrawPoly(&p[0], 2);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawLine(svtkPoints2D* points)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  if (points->GetNumberOfPoints() < 2)
  {
    svtkErrorMacro(<< "Attempted to paint a line with <2 points.");
    return;
  }
  float* f = svtkArrayDownCast<svtkFloatArray>(points->GetData())->GetPointer(0);
  this->Device->DrawPoly(f, 2);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPoly(float* x, float* y, int n)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  float* p = new float[2 * n];
  for (int i = 0; i < n; ++i)
  {
    p[2 * i] = x[i];
    p[2 * i + 1] = y[i];
  }
  this->Device->DrawPoly(p, n);
  delete[] p;
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPoly(svtkPoints2D* points)
{
  // Construct an array with the correct coordinate packing for OpenGL.
  int n = static_cast<int>(points->GetNumberOfPoints());
  // If the points are of type float then call OpenGL directly
  float* f = svtkArrayDownCast<svtkFloatArray>(points->GetData())->GetPointer(0);
  this->DrawPoly(f, n);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPoly(float* points, int n)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  if (n < 2)
  {
    svtkErrorMacro(<< "Attempted to paint a line with <2 points.");
    return;
  }
  this->Device->DrawPoly(points, n);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPoly(float* points, int n, unsigned char* colors, int nc_comps)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  if (n < 2)
  {
    svtkErrorMacro(<< "Attempted to paint a line with <2 points.");
    return;
  }
  this->Device->DrawPoly(points, n, colors, nc_comps);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawLines(svtkPoints2D* points)
{
  // Construct an array with the correct coordinate packing for OpenGL.
  int n = static_cast<int>(points->GetNumberOfPoints());
  // If the points are of type float then call OpenGL directly
  float* f = svtkArrayDownCast<svtkFloatArray>(points->GetData())->GetPointer(0);
  this->DrawLines(f, n);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawLines(float* points, int n)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  if (n < 2)
  {
    svtkErrorMacro(<< "Attempted to paint a line with <2 points.");
    return;
  }
  this->Device->DrawLines(points, n);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPoint(float x, float y)
{
  float p[] = { x, y };
  this->DrawPoints(&p[0], 1);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPoints(float* x, float* y, int n)
{
  // Copy the points into an array and draw it.
  float* p = new float[2 * n];
  for (int i = 0; i < n; ++i)
  {
    p[2 * i] = x[i];
    p[2 * i + 1] = y[i];
  }
  this->DrawPoints(&p[0], n);
  delete[] p;
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPoints(svtkPoints2D* points)
{
  // Construct an array with the correct coordinate packing for OpenGL.
  int n = static_cast<int>(points->GetNumberOfPoints());
  // If the points are of type float then call OpenGL directly
  float* f = svtkArrayDownCast<svtkFloatArray>(points->GetData())->GetPointer(0);
  this->DrawPoints(f, n);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPoints(float* points, int n)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  this->Device->DrawPoints(points, n);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPointSprites(svtkImageData* sprite, svtkPoints2D* points)
{
  // Construct an array with the correct coordinate packing for OpenGL.
  int n = static_cast<int>(points->GetNumberOfPoints());
  // If the points are of type float then call OpenGL directly
  float* f = svtkArrayDownCast<svtkFloatArray>(points->GetData())->GetPointer(0);
  this->DrawPointSprites(sprite, f, n);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPointSprites(
  svtkImageData* sprite, svtkPoints2D* points, svtkUnsignedCharArray* colors)
{
  // Construct an array with the correct coordinate packing for OpenGL.
  int n = static_cast<int>(points->GetNumberOfPoints());
  int nc = static_cast<int>(colors->GetNumberOfTuples());
  if (n != nc)
  {
    svtkErrorMacro(<< "Attempted to color points with array of wrong length");
    return;
  }
  int nc_comps = static_cast<int>(colors->GetNumberOfComponents());
  // If the points are of type float then call OpenGL directly
  float* f = svtkArrayDownCast<svtkFloatArray>(points->GetData())->GetPointer(0);
  unsigned char* c = colors->GetPointer(0);
  this->DrawPointSprites(sprite, f, n, c, nc_comps);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPointSprites(
  svtkImageData* sprite, float* points, int n, unsigned char* colors, int nc_comps)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  this->Device->DrawPointSprites(sprite, points, n, colors, nc_comps);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPointSprites(svtkImageData* sprite, float* points, int n)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  this->Device->DrawPointSprites(sprite, points, n);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawMarkers(
  int shape, bool highlight, float* points, int n, unsigned char* colors, int nc_comps)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  this->Device->DrawMarkers(shape, highlight, points, n, colors, nc_comps);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawMarkers(int shape, bool highlight, float* points, int n)
{
  this->DrawMarkers(shape, highlight, points, n, nullptr, 0);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawMarkers(int shape, bool highlight, svtkPoints2D* points)
{
  // Construct an array with the correct coordinate packing for OpenGL.
  int n = static_cast<int>(points->GetNumberOfPoints());
  float* f = svtkArrayDownCast<svtkFloatArray>(points->GetData())->GetPointer(0);
  this->DrawMarkers(shape, highlight, f, n);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawMarkers(
  int shape, bool highlight, svtkPoints2D* points, svtkUnsignedCharArray* colors)
{
  // Construct an array with the correct coordinate packing for OpenGL.
  int n = static_cast<int>(points->GetNumberOfPoints());
  int nc = static_cast<int>(colors->GetNumberOfTuples());
  if (n != nc)
  {
    svtkErrorMacro(<< "Attempted to color points with array of wrong length");
    return;
  }
  int nc_comps = static_cast<int>(colors->GetNumberOfComponents());
  float* f = svtkArrayDownCast<svtkFloatArray>(points->GetData())->GetPointer(0);
  unsigned char* c = colors->GetPointer(0);
  this->DrawMarkers(shape, highlight, f, n, c, nc_comps);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawRect(float x, float y, float width, float height)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  float p[] = { x, y, x + width, y, x + width, y + height, x, y + height, x, y };

  // Draw the filled area of the rectangle.
  this->Device->DrawQuad(&p[0], 4);

  // Draw the outline now.
  this->Device->DrawPoly(&p[0], 5);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawQuad(
  float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4)
{
  float p[] = { x1, y1, x2, y2, x3, y3, x4, y4 };
  this->DrawQuad(&p[0]);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawQuad(float* p)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }

  // Draw the filled area of the quad.
  this->Device->DrawQuad(p, 4);

  // Draw the outline now.
  this->Device->DrawPoly(p, 4);
  float closeLine[] = { p[0], p[1], p[6], p[7] };
  this->Device->DrawPoly(&closeLine[0], 2);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawQuadStrip(svtkPoints2D* points)
{
  // Construct an array with the correct coordinate packing for OpenGL.
  int n = static_cast<int>(points->GetNumberOfPoints());
  // If the points are of type float then call OpenGL directly
  float* f = svtkArrayDownCast<svtkFloatArray>(points->GetData())->GetPointer(0);
  this->DrawQuadStrip(f, n);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawQuadStrip(float* points, int n)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  // Draw the filled area of the polygon.
  this->Device->DrawQuadStrip(points, n);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPolygon(float* x, float* y, int n)
{
  // Copy the points into an array and draw it.
  float* p = new float[2 * n];
  for (int i = 0; i < n; ++i)
  {
    p[2 * i] = x[i];
    p[2 * i + 1] = y[i];
  }
  this->DrawPolygon(&p[0], n);
  delete[] p;
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPolygon(svtkPoints2D* points)
{
  // Construct an array with the correct coordinate packing for OpenGL.
  int n = static_cast<int>(points->GetNumberOfPoints());
  // If the points are of type float then call OpenGL directly
  float* f = svtkArrayDownCast<svtkFloatArray>(points->GetData())->GetPointer(0);
  this->DrawPolygon(f, n);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPolygon(float* points, int n)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  // Draw the filled area of the polygon.
  this->Device->DrawPolygon(points, n);

  // Draw the outline now.
  this->Device->DrawPoly(points, n);
  float closeLine[] = { points[0], points[1], points[2 * n - 2], points[2 * n - 1] };
  this->Device->DrawPoly(&closeLine[0], 2);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPolygon(float* x, float* y, int n, unsigned char* color, int nc_comps)
{
  // Copy the points into an array and draw it.
  float* p = new float[2 * n];
  for (int i = 0; i < n; ++i)
  {
    p[2 * i] = x[i];
    p[2 * i + 1] = y[i];
  }
  this->DrawPolygon(&p[0], n, color, nc_comps);
  delete[] p;
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPolygon(svtkPoints2D* points, unsigned char* color, int nc_comps)
{

  // Construct an array with the correct coordinate packing for OpenGL.
  int n = static_cast<int>(points->GetNumberOfPoints());
  // If the points are of type float then call OpenGL directly
  float* f = svtkArrayDownCast<svtkFloatArray>(points->GetData())->GetPointer(0);
  this->DrawPolygon(f, n, color, nc_comps);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPolygon(float* points, int n, unsigned char* color, int nc_comps)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }

  // Draw the filled area of the polygon.
  this->Device->DrawColoredPolygon(points, n, color, nc_comps);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawEllipse(float x, float y, float rx, float ry)
{
  assert("pre: positive_rx" && rx >= 0);
  assert("pre: positive_ry" && ry >= 0);
  this->DrawEllipticArc(x, y, rx, ry, 0.0, 360.0);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawWedge(
  float x, float y, float outRadius, float inRadius, float startAngle, float stopAngle)

{
  assert("pre: positive_outRadius" && outRadius >= 0.0f);
  assert("pre: positive_inRadius" && inRadius >= 0.0f);
  assert("pre: ordered_radii" && inRadius <= outRadius);

  this->DrawEllipseWedge(x, y, outRadius, outRadius, inRadius, inRadius, startAngle, stopAngle);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawEllipseWedge(float x, float y, float outRx, float outRy, float inRx,
  float inRy, float startAngle, float stopAngle)

{
  assert("pre: positive_outRx" && outRx >= 0.0f);
  assert("pre: positive_outRy" && outRy >= 0.0f);
  assert("pre: positive_inRx" && inRx >= 0.0f);
  assert("pre: positive_inRy" && inRy >= 0.0f);
  assert("pre: ordered_rx" && inRx <= outRx);
  assert("pre: ordered_ry" && inRy <= outRy);

  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  // don't tessellate here. The device context knows what to do with an
  // arc. An OpenGL device context will tessellate but and SVG context with
  // just generate an arc.
  this->Device->DrawEllipseWedge(x, y, outRx, outRy, inRx, inRy, startAngle, stopAngle);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawArc(float x, float y, float r, float startAngle, float stopAngle)
{
  assert("pre: positive_radius" && r >= 0);
  this->DrawEllipticArc(x, y, r, r, startAngle, stopAngle);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawEllipticArc(
  float x, float y, float rX, float rY, float startAngle, float stopAngle)
{
  assert("pre: positive_rX" && rX >= 0);
  assert("pre: positive_rY" && rY >= 0);

  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  // don't tessellate here. The device context knows what to do with an
  // arc. An OpenGL device context will tessellate but and SVG context with
  // just generate an arc.
  this->Device->DrawEllipticArc(x, y, rX, rY, startAngle, stopAngle);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawStringRect(svtkPoints2D* rect, const svtkStdString& string)
{
  svtkVector2f p = this->CalculateTextPosition(rect);
  this->DrawString(p.GetX(), p.GetY(), string);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawStringRect(svtkPoints2D* rect, const svtkUnicodeString& string)
{
  svtkVector2f p = this->CalculateTextPosition(rect);
  this->DrawString(p.GetX(), p.GetY(), string);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawStringRect(svtkPoints2D* rect, const char* string)
{
  this->DrawStringRect(rect, svtkStdString(string));
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawString(svtkPoints2D* point, const svtkStdString& string)
{
  float* f = svtkArrayDownCast<svtkFloatArray>(point->GetData())->GetPointer(0);
  this->DrawString(f[0], f[1], string);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawString(float x, float y, const svtkStdString& string)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  if (string.empty())
  {
    return;
  }
  float f[] = { x, y };
  this->Device->DrawString(f, string);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawString(svtkPoints2D* point, const svtkUnicodeString& string)
{
  float* f = svtkArrayDownCast<svtkFloatArray>(point->GetData())->GetPointer(0);
  this->DrawString(f[0], f[1], string);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawString(float x, float y, const svtkUnicodeString& string)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  if (string.empty())
  {
    return;
  }
  float f[] = { x, y };
  this->Device->DrawString(&f[0], string);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawString(svtkPoints2D* point, const char* string)
{
  float* f = svtkArrayDownCast<svtkFloatArray>(point->GetData())->GetPointer(0);
  this->DrawString(f[0], f[1], svtkStdString(string));
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawString(float x, float y, const char* string)
{
  this->DrawString(x, y, svtkStdString(string));
}

//-----------------------------------------------------------------------------
void svtkContext2D::ComputeStringBounds(const svtkStdString& string, svtkPoints2D* bounds)
{
  bounds->SetNumberOfPoints(2);
  float* f = svtkArrayDownCast<svtkFloatArray>(bounds->GetData())->GetPointer(0);
  this->ComputeStringBounds(string, f);
}

//-----------------------------------------------------------------------------
void svtkContext2D::ComputeStringBounds(const svtkStdString& string, float bounds[4])
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  this->Device->ComputeStringBounds(string, bounds);
}

//-----------------------------------------------------------------------------
void svtkContext2D::ComputeStringBounds(const svtkUnicodeString& string, svtkPoints2D* bounds)
{
  bounds->SetNumberOfPoints(2);
  float* f = svtkArrayDownCast<svtkFloatArray>(bounds->GetData())->GetPointer(0);
  this->ComputeStringBounds(string, f);
}

//-----------------------------------------------------------------------------
void svtkContext2D::ComputeStringBounds(const svtkUnicodeString& string, float bounds[4])
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  this->Device->ComputeStringBounds(string, bounds);
}

//-----------------------------------------------------------------------------
void svtkContext2D::ComputeStringBounds(const char* string, svtkPoints2D* bounds)
{
  this->ComputeStringBounds(svtkStdString(string), bounds);
}

//-----------------------------------------------------------------------------
void svtkContext2D::ComputeStringBounds(const char* string, float bounds[4])
{
  this->ComputeStringBounds(svtkStdString(string), bounds);
}

//-----------------------------------------------------------------------------
void svtkContext2D::ComputeJustifiedStringBounds(const char* string, float bounds[4])
{
  this->Device->ComputeJustifiedStringBounds(string, bounds);
}

//-----------------------------------------------------------------------------
int svtkContext2D::ComputeFontSizeForBoundedString(
  const svtkStdString& string, float width, float height)
{
  double orientation = this->GetTextProp()->GetOrientation();
  this->GetTextProp()->SetOrientation(0.0);

  float stringBounds[4];
  int currentFontSize = this->GetTextProp()->GetFontSize();
  this->ComputeStringBounds(string, stringBounds);

  // font size is too big
  if (stringBounds[2] > width || stringBounds[3] > height)
  {
    while (stringBounds[2] > width || stringBounds[3] > height)
    {
      --currentFontSize;
      this->GetTextProp()->SetFontSize(currentFontSize);
      this->ComputeStringBounds(string, stringBounds);
      if (currentFontSize < 0)
      {
        this->GetTextProp()->SetFontSize(0);
        return 0;
      }
    }
  }

  // font size is too small
  else
  {
    while (stringBounds[2] < width && stringBounds[3] < height)
    {
      ++currentFontSize;
      this->GetTextProp()->SetFontSize(currentFontSize);
      this->ComputeStringBounds(string, stringBounds);
    }
    --currentFontSize;
    this->GetTextProp()->SetFontSize(currentFontSize);
  }

  this->GetTextProp()->SetOrientation(orientation);
  return currentFontSize;
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawMathTextString(svtkPoints2D* point, const svtkStdString& string)
{
  float* f = svtkArrayDownCast<svtkFloatArray>(point->GetData())->GetPointer(0);
  this->DrawMathTextString(f[0], f[1], string);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawMathTextString(float x, float y, const svtkStdString& string)
{
  if (!this->Device)
  {
    svtkErrorMacro(<< "Attempted to paint with no active svtkContextDevice2D.");
    return;
  }
  if (string.empty())
  {
    return;
  }
  float f[] = { x, y };
  this->Device->DrawMathTextString(f, string);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawMathTextString(svtkPoints2D* point, const char* string)
{
  float* f = svtkArrayDownCast<svtkFloatArray>(point->GetData())->GetPointer(0);
  this->DrawMathTextString(f[0], f[1], svtkStdString(string));
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawMathTextString(float x, float y, const char* string)
{
  this->DrawMathTextString(x, y, svtkStdString(string));
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawMathTextString(
  svtkPoints2D* point, const svtkStdString& string, const svtkStdString& fallback)
{
  if (this->Device->MathTextIsSupported())
  {
    this->DrawMathTextString(point, string);
  }
  else
  {
    this->DrawString(point, fallback);
  }
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawMathTextString(
  float x, float y, const svtkStdString& string, const svtkStdString& fallback)
{
  if (this->Device->MathTextIsSupported())
  {
    this->DrawMathTextString(x, y, string);
  }
  else
  {
    this->DrawString(x, y, fallback);
  }
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawMathTextString(svtkPoints2D* point, const char* string, const char* fallback)
{
  if (this->Device->MathTextIsSupported())
  {
    this->DrawMathTextString(point, string);
  }
  else
  {
    this->DrawString(point, fallback);
  }
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawMathTextString(float x, float y, const char* string, const char* fallback)
{
  if (this->Device->MathTextIsSupported())
  {
    this->DrawMathTextString(x, y, string);
  }
  else
  {
    this->DrawString(x, y, fallback);
  }
}

//-----------------------------------------------------------------------------
bool svtkContext2D::MathTextIsSupported()
{
  return this->Device->MathTextIsSupported();
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawImage(float x, float y, svtkImageData* image)
{
  float p[] = { x, y };
  this->Device->DrawImage(&p[0], 1.0, image);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawImage(float x, float y, float scale, svtkImageData* image)
{
  float p[] = { x, y };
  this->Device->DrawImage(&p[0], scale, image);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawImage(const svtkRectf& pos, svtkImageData* image)
{
  this->Device->DrawImage(pos, image);
}

//-----------------------------------------------------------------------------
void svtkContext2D::DrawPolyData(
  float x, float y, svtkPolyData* polyData, svtkUnsignedCharArray* colors, int scalarMode)
{
  float p[] = { x, y };
  this->Device->DrawPolyData(&p[0], 1.0, polyData, colors, scalarMode);
}

//-----------------------------------------------------------------------------
void svtkContext2D::ApplyPen(svtkPen* pen)
{
  this->Device->ApplyPen(pen);
}

//-----------------------------------------------------------------------------
svtkPen* svtkContext2D::GetPen()
{
  if (this->Device)
  {
    return this->Device->GetPen();
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void svtkContext2D::ApplyBrush(svtkBrush* brush)
{
  this->Device->ApplyBrush(brush);
}

//-----------------------------------------------------------------------------
svtkBrush* svtkContext2D::GetBrush()
{
  if (this->Device)
  {
    return this->Device->GetBrush();
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void svtkContext2D::ApplyTextProp(svtkTextProperty* prop)
{
  this->Device->ApplyTextProp(prop);
}

//-----------------------------------------------------------------------------
svtkTextProperty* svtkContext2D::GetTextProp()
{
  if (this->Device)
  {
    return this->Device->GetTextProp();
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void svtkContext2D::SetTransform(svtkTransform2D* transform)
{
  if (transform)
  {
    this->Device->SetMatrix(transform->GetMatrix());
  }
}

//-----------------------------------------------------------------------------
svtkTransform2D* svtkContext2D::GetTransform()
{
  if (this->Device && this->Transform)
  {
    this->Device->GetMatrix(this->Transform->GetMatrix());
    return this->Transform;
  }
  return nullptr;
}

//-----------------------------------------------------------------------------
void svtkContext2D::AppendTransform(svtkTransform2D* transform)
{
  if (!transform)
  {
    return;
  }

  this->Device->MultiplyMatrix(transform->GetMatrix());
}

//-----------------------------------------------------------------------------
void svtkContext2D::PushMatrix()
{
  this->Device->PushMatrix();
}

//-----------------------------------------------------------------------------
void svtkContext2D::PopMatrix()
{
  this->Device->PopMatrix();
}

// ----------------------------------------------------------------------------
void svtkContext2D::ApplyId(svtkIdType id)
{
  assert("pre: zero_reserved_for_background" && id > 0);
  assert("pre: 24bit_limited" && id < 16777216);
  unsigned char rgba[4];

  // r most significant bits (16-23).
  // g (8-15)
  // b less significant bits (0-7).

  rgba[0] = static_cast<unsigned char>((id & 0xff0000) >> 16);
  rgba[1] = static_cast<unsigned char>((id & 0xff00) >> 8);
  rgba[2] = static_cast<unsigned char>(id & 0xff);
  rgba[3] = 1; // not used (because the colorbuffer in the default framebuffer
  // may not have an alpha channel)

  assert("check: valid_conversion" &&
    static_cast<svtkIdType>((static_cast<int>(rgba[0]) << 16) | (static_cast<int>(rgba[1]) << 8) |
      static_cast<int>(rgba[2])) == id);

  this->Device->SetColor4(rgba);
}

void svtkContext2D::SetContext3D(svtkContext3D* context)
{
  this->Context3D = context;
}

//-----------------------------------------------------------------------------
svtkVector2f svtkContext2D::CalculateTextPosition(svtkPoints2D* rect)
{
  if (rect->GetNumberOfPoints() < 2)
  {
    return svtkVector2f(0, 0);
  }

  float* f = svtkArrayDownCast<svtkFloatArray>(rect->GetData())->GetPointer(0);
  return this->CalculateTextPosition(f);
}

//-----------------------------------------------------------------------------
svtkVector2f svtkContext2D::CalculateTextPosition(float rect[4])
{
  // Draw the text at the appropriate point inside the rect for the alignment
  // specified. This is a convenience when an area of the screen should have
  // text drawn that is aligned to the entire area.
  svtkVector2f p(0, 0);

  if (this->Device->GetTextProp()->GetJustification() == SVTK_TEXT_LEFT)
  {
    p.SetX(rect[0]);
  }
  else if (this->Device->GetTextProp()->GetJustification() == SVTK_TEXT_CENTERED)
  {
    p.SetX(rect[0] + 0.5f * rect[2]);
  }
  else
  {
    p.SetX(rect[0] + rect[2]);
  }

  if (this->Device->GetTextProp()->GetVerticalJustification() == SVTK_TEXT_BOTTOM)
  {
    p.SetY(rect[1]);
  }
  else if (this->Device->GetTextProp()->GetVerticalJustification() == SVTK_TEXT_CENTERED)
  {
    p.SetY(rect[1] + 0.5f * rect[3]);
  }
  else
  {
    p.SetY(rect[1] + rect[3]);
  }
  return p;
}

//-----------------------------------------------------------------------------
svtkContext2D::svtkContext2D()
  : Context3D(nullptr)
{
  this->Device = nullptr;
  this->Transform = svtkTransform2D::New();
  this->BufferId = nullptr;
}

//-----------------------------------------------------------------------------
svtkContext2D::~svtkContext2D()
{
  if (this->Device)
  {
    this->Device->Delete();
  }
  if (this->Transform)
  {
    this->Transform->Delete();
  }
}

//-----------------------------------------------------------------------------
void svtkContext2D::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
  os << indent << "Context Device: ";
  if (this->Device)
  {
    os << endl;
    this->Device->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }
}
