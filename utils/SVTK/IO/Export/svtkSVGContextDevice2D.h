/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSVGContextDevice2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkSVGContextDevice2D
 * @brief svtkContextDevice2D implementation for use with svtkSVGExporter.
 *
 * Limitations:
 * - The Nearest/Linear texture properties are ignored, since SVG doesn't
 *   provide any reliable control over interpolation.
 * - Embedded fonts are experimental and poorly tested. Viewer support is
 *   lacking at the time of writing, hence the feature is largely useless. By
 *   default, fonts are not embedded since they're basically useless bloat.
 * - TextAsPath is enabled by default, since viewers differ wildly in how they
 *   handle text objects (eg. Inkscape renders at expected size, but webkit is
 *   way too big).
 * - Pattern fills and markers are not shown on some viewers, e.g. KDE's okular
 *   (Webkit seems to work, though).
 * - Clipping seems to be broken in most viewers. Webkit is buggy and forces the
 *   clip coordinates to objectBoundingBox, even when explicitly set to
 *   userSpaceOnUse.
 * - Many viewers anti-alias the output, leaving thin outlines around the
 *   triangles that make up larger polygons. This is a viewer issue and there
 *   not much we can do about it from the SVTK side of things (and most viewers
 *   don't seem to have an antialiasing toggle, either...).
 */

#ifndef svtkSVGContextDevice2D_h
#define svtkSVGContextDevice2D_h

#include "svtkContextDevice2D.h"
#include "svtkIOExportModule.h" // For export macro
#include "svtkNew.h"            // For svtkNew!

#include <array> // For std::array!

class svtkColor3ub;
class svtkColor4ub;
class svtkPath;
class svtkRenderer;
class svtkTransform;
class svtkVector3f;
class svtkXMLDataElement;

class SVTKIOEXPORT_EXPORT svtkSVGContextDevice2D : public svtkContextDevice2D
{
public:
  static svtkSVGContextDevice2D* New();
  svtkTypeMacro(svtkSVGContextDevice2D, svtkContextDevice2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /** The svg container element to draw into, and the global definitions
   *  element. */
  void SetSVGContext(svtkXMLDataElement* context, svtkXMLDataElement* defs);

  /**
   * EXPERIMENTAL: If true, the font glyph information will be embedded in the
   * output. Default is false.
   *
   * @note This feature is experimental and not well tested, as most browsers
   * and SVG viewers do not support rendering embedded fonts. As such, enabling
   * this option typically just increases file size for no real benefit.
   *
   * @{
   */
  svtkSetMacro(EmbedFonts, bool);
  svtkGetMacro(EmbedFonts, bool);
  svtkBooleanMacro(EmbedFonts, bool);
  /**@}*/

  /**
   * If true, draw all text as path objects rather than text objects. Enabling
   * this option will:
   *
   * - Improve portability (text will look exactly the same everywhere).
   * - Increase file size (text objects are much more compact than paths).
   * - Prevent text from being easily edited (text metadata is lost).
   *
   * Note that some text (e.g. MathText) is always rendered as a path.
   *
   * The default is true, as many browsers and SVG viewers render text objects
   * inconsistently.
   *
   * @{
   */
  svtkSetMacro(TextAsPath, bool);
  svtkGetMacro(TextAsPath, bool);
  svtkBooleanMacro(TextAsPath, bool);
  /**@}*/

  /**
   * Set the threshold for subdividing gradient-shaded polygons/line. Default
   * value is 1, and lower values yield higher quality and larger files. Larger
   * values will reduce the number of primitives, but will decrease quality.
   *
   * A triangle / line will not be subdivided further if all of it's vertices
   * satisfy the equation:
   *
   * |v1 - v2|^2 < thresh
   *
   * e.g. the squared norm of the vector between any verts must be greater than
   * the threshold for subdivision to occur.
   *
   * @{
   */
  svtkSetMacro(SubdivisionThreshold, float);
  svtkGetMacro(SubdivisionThreshold, float);
  /**@}*/

  /**
   * Write any definition information (fonts, images, etc) that are accumulated
   * between actors.
   */
  void GenerateDefinitions();

  void Begin(svtkViewport*) override;
  void End() override;

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

  void SetLineType(int type) override;
  void SetMatrix(svtkMatrix3x3* m) override;
  void GetMatrix(svtkMatrix3x3* m) override;
  void MultiplyMatrix(svtkMatrix3x3* m) override;
  void PushMatrix() override;
  void PopMatrix() override;
  void SetClipping(int* x) override;
  void EnableClipping(bool enable) override;

protected:
  svtkSVGContextDevice2D();
  ~svtkSVGContextDevice2D() override;

  void SetViewport(svtkViewport*);

  void PushGraphicsState();
  void PopGraphicsState();

  // Apply clipping and transform information current active node.
  void SetupClippingAndTransform();

  // pen -> stroke state
  void ApplyPenStateToNode(svtkXMLDataElement* node);
  void ApplyPenColorToNode(svtkXMLDataElement* node);
  void ApplyPenOpacityToNode(svtkXMLDataElement* node);
  void ApplyPenWidthToNode(svtkXMLDataElement* node);
  void ApplyPenStippleToNode(svtkXMLDataElement* node);

  // pen -> fill state
  void ApplyPenAsFillColorToNode(svtkXMLDataElement* node);
  void ApplyPenAsFillOpacityToNode(svtkXMLDataElement* node);

  // brush -> fill state
  void ApplyBrushStateToNode(svtkXMLDataElement* node);
  void ApplyBrushColorToNode(svtkXMLDataElement* node);
  void ApplyBrushOpacityToNode(svtkXMLDataElement* node);
  void ApplyBrushTextureToNode(svtkXMLDataElement* node);

  // tprop --> text state
  void ApplyTextPropertyStateToNode(svtkXMLDataElement* node, float x, float y);
  void ApplyTextPropertyStateToNodeForPath(svtkXMLDataElement* node, float x, float y);

  void ApplyTransform();

  // Add marker symbols to defs, return symbol id.
  std::string AddCrossSymbol(bool highlight);
  std::string AddPlusSymbol(bool highlight);
  std::string AddSquareSymbol(bool highlight);
  std::string AddCircleSymbol(bool highlight);
  std::string AddDiamondSymbol(bool highlight);

  void DrawPath(svtkPath* path, std::ostream& out);

  void DrawLineGradient(const svtkVector2f& p1, const svtkColor4ub& c1, const svtkVector2f& p2,
    const svtkColor4ub& c2, bool useAlpha);
  void DrawTriangleGradient(const svtkVector2f& p1, const svtkColor4ub& c1, const svtkVector2f& p2,
    const svtkColor4ub& c2, const svtkVector2f& p3, const svtkColor4ub& c3, bool useAlpha);

  // Used by the Draw*Gradient methods to prevent subdividing triangles / lines
  // that are already really small.
  bool AreaLessThanTolerance(const svtkVector2f& p1, const svtkVector2f& p2, const svtkVector2f& p3);
  bool LengthLessThanTolerance(const svtkVector2f& p1, const svtkVector2f& p2);

  bool ColorsAreClose(const svtkColor4ub& c1, const svtkColor4ub& c2, bool useAlpha);
  bool ColorsAreClose(
    const svtkColor4ub& c1, const svtkColor4ub& c2, const svtkColor4ub& c3, bool useAlpha);

  void WriteFonts();
  void WriteImages();
  void WritePatterns();
  void WriteClipRects();

  void AdjustMatrixForSVG(const double in[9], double out[9]);
  void GetSVGMatrix(double svg[9]);
  static bool Transform2DEqual(const double mat3[9], const double mat4[16]);
  static void Matrix3ToMatrix4(const double mat3[9], double mat4[16]);
  static void Matrix4ToMatrix3(const double mat4[16], double mat3[9]);

  float GetScaledPenWidth();
  void GetScaledPenWidth(float& x, float& y);
  void TransformSize(float& x, float& y);

  svtkImageData* PreparePointSprite(svtkImageData* in);

  struct Details;
  Details* Impl;

  svtkViewport* Viewport;
  svtkXMLDataElement* ContextNode;
  svtkXMLDataElement* ActiveNode;
  svtkXMLDataElement* DefinitionNode;

  // This is a 3D transform, the 2D version doesn't support push/pop.
  svtkNew<svtkTransform> Matrix;
  std::array<double, 9> ActiveNodeTransform;

  std::array<int, 4> ClipRect;           // x, y, w, h
  std::array<int, 4> ActiveNodeClipRect; // x, y, w, h

  float CanvasHeight; // Used in y coordinate conversions.
  float SubdivisionThreshold;
  bool IsClipping;
  bool ActiveNodeIsClipping;
  bool EmbedFonts;
  bool TextAsPath;

private:
  svtkSVGContextDevice2D(const svtkSVGContextDevice2D&) = delete;
  void operator=(const svtkSVGContextDevice2D&) = delete;
};

#endif // svtkSVGContextDevice2D_h
