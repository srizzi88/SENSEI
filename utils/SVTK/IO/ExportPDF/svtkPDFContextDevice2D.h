/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPDFContextDevice2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkPDFContextDevice2D
 * @brief svtkContextDevice2D implementation for use with svtkPDFExporter.
 *
 * Quirks:
 * - Libharu does not support RGBA images. If an alpha channel is present in any
 *   drawn images, it will be blended into an opaque background filled with the
 *   active Brush color to produce a flat RGB image.
 */

#ifndef svtkPDFContextDevice2D_h
#define svtkPDFContextDevice2D_h

#include "svtkContextDevice2D.h"
#include "svtkIOExportPDFModule.h" // For export macro
#include "svtkNew.h"               // For svtkNew!

class svtkColor3ub;
class svtkPath;
class svtkRenderer;
class svtkTransform;

class SVTKIOEXPORTPDF_EXPORT svtkPDFContextDevice2D : public svtkContextDevice2D
{
public:
  static svtkPDFContextDevice2D* New();
  svtkTypeMacro(svtkPDFContextDevice2D, svtkContextDevice2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Set the HPDF_Doc and HPDF_Page to use while exporting the scene. The
   * type is void* to keep the libharu opaque types from leaking into headers.
   * This function expects HPDF_Document* and HPDF_Page* as the arguments.
   */
  void SetHaruObjects(void* doc, void* page);

  void SetRenderer(svtkRenderer*);

  void DrawPoly(float* points, int n, unsigned char* colors = nullptr, int nc_comps = 0) override;
  void DrawLines(float* f, int n, unsigned char* colors = nullptr, int nc_comps = 0) override;
  void DrawPoints(float* points, int n, unsigned char* colors = nullptr, int nc_comps = 0) override;
  void DrawPointSprites(svtkImageData* sprite, float* points, int n, unsigned char* colors = nullptr,
    int nc_comps = 0) override;
  void DrawMarkers(int shape, bool highlight, float* points, int n, unsigned char* colors = nullptr,
    int nc_comps = 0) override;
  void DrawQuad(float*, int) override;
  void DrawQuadStrip(float*, int) override;
  void DrawPolygon(float*, int) override;
  void DrawColoredPolygon(
    float* points, int numPoints, unsigned char* colors = nullptr, int nc_comps = 0) override;
  void DrawEllipseWedge(float x, float y, float outRx, float outRy, float inRx, float inRy,
    float startAngle, float stopAngle) override;
  void DrawEllipticArc(
    float x, float y, float rX, float rY, float startAngle, float stopAngle) override;
  void DrawString(float* point, const svtkStdString& string) override;
  void ComputeStringBounds(const svtkStdString& string, float bounds[4]) override;
  void DrawString(float* point, const svtkUnicodeString& string) override;
  void ComputeStringBounds(const svtkUnicodeString& string, float bounds[4]) override;
  void ComputeJustifiedStringBounds(const char* string, float bounds[4]) override;
  void DrawMathTextString(float* point, const svtkStdString& str) override;
  void DrawImage(float p[2], float scale, svtkImageData* image) override;
  void DrawImage(const svtkRectf& pos, svtkImageData* image) override;
  void SetColor4(unsigned char color[4]) override;
  void SetTexture(svtkImageData* image, int properties) override;
  void SetPointSize(float size) override;
  void SetLineWidth(float width) override;
  void DrawPolyData(float p[2], float scale, svtkPolyData* polyData, svtkUnsignedCharArray* colors,
    int scalarMode) override;

  void SetLineType(int type) override;
  void SetMatrix(svtkMatrix3x3* m) override;
  void GetMatrix(svtkMatrix3x3* m) override;
  void MultiplyMatrix(svtkMatrix3x3* m) override;
  void PushMatrix() override;
  void PopMatrix() override;
  void SetClipping(int* x) override;
  void EnableClipping(bool enable) override;

protected:
  svtkPDFContextDevice2D();
  ~svtkPDFContextDevice2D() override;

  void PushGraphicsState();
  void PopGraphicsState();

  void ApplyPenState();
  void ApplyStrokeColor(unsigned char* color, int numComps);
  void ApplyLineWidth(float width);
  void ApplyLineType(int type);
  void Stroke();

  void ApplyPenStateAsFill();
  void ApplyBrushState();
  void ApplyTextPropertyState();
  void ApplyFillColor(unsigned char* color, int numComps);
  void ApplyFillAlpha(unsigned char alpha);
  void Fill(bool stroke = false);
  void FillEvenOdd(bool stroke = false);

  void BeginClipPathForTexture();
  void RegisterTexturePoints(float* data, int numPoints);
  void FillTexture();

  // converts input to RGB if needed. Call Delete() on the returned object when
  // finished with it.
  svtkImageData* PrepareImageData(svtkImageData* in);

  void DrawEllipticArcSegments(
    float x, float y, float rX, float rY, float startAngle, float stopAngle, bool startPath);
  int GetNumberOfArcIterations(float rX, float rY, float startAngle, float stopAngle);

  void DrawCrossMarkers(bool highlight, float* points, int n, unsigned char* colors, int nc_comps);
  void DrawPlusMarkers(bool highlight, float* points, int n, unsigned char* colors, int nc_comps);
  void DrawSquareMarkers(bool highlight, float* points, int n, unsigned char* colors, int nc_comps);
  void DrawCircleMarkers(bool highlight, float* points, int n, unsigned char* colors, int nc_comps);
  void DrawDiamondMarkers(
    bool highlight, float* points, int n, unsigned char* colors, int nc_comps);

  void DrawPath(svtkPath* path, float x, float y);

  void ApplyTransform();

  // This is weird, but the pen width must not be affected by the transform's
  // scaling factors. This function returns the unscaled x/y components of
  // the pen width.
  svtkVector2f GetUnscaledPenWidth();

  // Converts a 2D transform matrix into a 3D transform matrix, or vice versa
  static void Matrix3ToMatrix4(svtkMatrix3x3* mat3, double mat4[16]);
  static void Matrix4ToMatrix3(double mat4[16], svtkMatrix3x3* mat3);
  static void Matrix4ToMatrix3(double mat4[16], double mat3[9]);

  // Convert a 3D transform matrix to an HPDF transformation.
  // trans = {a, b, c, d, x, y}, which define the transform:
  // | a b x |
  // | c d y |
  // | 0 0 1 |
  static void Matrix4ToHPDFTransform(const double mat4[16], float hpdfMat[6]);
  static void Matrix3ToHPDFTransform(const double mat4[9], float hpdfMat[6]);
  static void HPDFTransformToMatrix3(
    float a, float b, float c, float d, float x, float y, double mat3[9]);

  struct Details;
  Details* Impl;

  // This is a 3D transform, the 2D version doesn't support push/pop.
  svtkNew<svtkTransform> Matrix;

  svtkRenderer* Renderer;
  float PointSize;
  float ClipBox[4]; // x, y, w, h

  bool IsInTexturedFill;
  float TextureBounds[4]; // xmin, xmax, ymin, ymax; used for placing textures

private:
  svtkPDFContextDevice2D(const svtkPDFContextDevice2D&) = delete;
  void operator=(const svtkPDFContextDevice2D&) = delete;
};

#endif // svtkPDFContextDevice2D_h
