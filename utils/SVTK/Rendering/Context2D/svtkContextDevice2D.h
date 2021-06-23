/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextDevice2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkContextDevice2D
 * @brief   Abstract class for drawing 2D primitives.
 *
 *
 * This defines the interface for a svtkContextDevice2D. In this sense a
 * ContextDevice is a class used to paint 2D primitives onto a device, such as
 * an OpenGL context or a QGraphicsView.
 */

#ifndef svtkContextDevice2D_h
#define svtkContextDevice2D_h

#include "svtkObject.h"
#include "svtkRect.h"                     // For svtkRecti ivar
#include "svtkRenderingContext2DModule.h" // For export macro
#include "svtkRenderingCoreEnums.h"       // For marker enum
#include "svtkVector.h"                   // For svtkVector2i ivar

class svtkWindow;
class svtkViewport;
class svtkStdString;
class svtkUnicodeString;
class svtkTextProperty;
class svtkPoints2D;
class svtkImageData;
class svtkMatrix3x3;
class svtkAbstractContextBufferId;
class svtkPen;
class svtkBrush;
class svtkRectf;
class svtkPolyData;
class svtkUnsignedCharArray;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkContextDevice2D : public svtkObject
{
public:
  svtkTypeMacro(svtkContextDevice2D, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkContextDevice2D* New();

  /**
   * Draw a poly line using the points - fastest code path due to memory
   * layout of the coordinates. The line will be colored by
   * the colors array, which must be have nc_comps components (defining a single
   * color).
   * \sa DrawLines()
   */
  virtual void DrawPoly(
    float* points, int n, unsigned char* colors = nullptr, int nc_comps = 0) = 0;

  /**
   * Draw lines using the points - memory layout is as follows:
   * l1p1,l1p2,l2p1,l2p2... The lines will be colored by colors array
   * which has nc_comps components (defining a single color).
   * \sa DrawPoly()
   */
  virtual void DrawLines(float* f, int n, unsigned char* colors = nullptr, int nc_comps = 0) = 0;

  /**
   * Draw a series of points - fastest code path due to memory layout of the
   * coordinates. The colors and nc_comps are optional - color array.
   */
  virtual void DrawPoints(
    float* points, int n, unsigned char* colors = nullptr, int nc_comps = 0) = 0;

  /**
   * Draw a series of point sprites, images centred at the points supplied.
   * The supplied svtkImageData is the sprite to be drawn, only squares will be
   * drawn and the size is set using SetPointSize.
   * \param colors is an optional array of colors.
   * \param nc_comps is the number of components for the color.
   */
  virtual void DrawPointSprites(svtkImageData* sprite, float* points, int n,
    unsigned char* colors = nullptr, int nc_comps = 0) = 0;

  /**
   * Draw a series of markers centered at the points supplied. The \a shape
   * argument controls the marker shape, and can be one of
   * - SVTK_MARKER_CROSS
   * - SVTK_MARKER_PLUS
   * - SVTK_MARKER_SQUARE
   * - SVTK_MARKER_CIRCLE
   * - SVTK_MARKER_DIAMOND
   * \param colors is an optional array of colors.
   * \param nc_comps is the number of components for the color.
   */
  virtual void DrawMarkers(int shape, bool highlight, float* points, int n,
    unsigned char* colors = nullptr, int nc_comps = 0);

  /**
   * Draw a quad using the specified number of points.
   */
  virtual void DrawQuad(float*, int) { ; }

  /**
   * Draw a quad using the specified number of points.
   */
  virtual void DrawQuadStrip(float*, int) { ; }

  /**
   * Draw a polygon using the specified number of points.
   * @{
   */
  virtual void DrawPolygon(float* p, int n) { this->DrawColoredPolygon(p, n); }
  virtual void DrawColoredPolygon(
    float* points, int numPoints, unsigned char* colors = nullptr, int nc_comps = 0);
  /**@}*/

  /**
   * Draw an elliptic wedge with center at x, y, outer radii outRx, outRy,
   * inner radii inRx, inRy between angles startAngle and stopAngle
   * (expressed in degrees).
   * \pre positive_outRx: outRx>=0
   * \pre positive_outRy: outRy>=0
   * \pre positive_inRx: inRx>=0
   * \pre positive_inRy: inRy>=0
   * \pre ordered_rx: inRx<=outRx
   * \pre ordered_ry: inRy<=outRy
   */
  virtual void DrawEllipseWedge(float x, float y, float outRx, float outRy, float inRx, float inRy,
    float startAngle, float stopAngle) = 0;

  /**
   * Draw an elliptic arc with center at x,y with radii rX and rY between
   * angles startAngle and stopAngle (expressed in degrees).
   * \pre positive_rX: rX>=0
   * \pre positive_rY: rY>=0
   */
  virtual void DrawEllipticArc(
    float x, float y, float rX, float rY, float startAngle, float stopAngle) = 0;

  /**
   * Draw some text to the screen.
   */
  virtual void DrawString(float* point, const svtkStdString& string) = 0;

  /**
   * Compute the bounds of the supplied string. The bounds will be copied to the
   * supplied bounds variable, the first two elements are the bottom corner of
   * the string, and the second two elements are the width and height of the
   * bounding box.
   * NOTE: This function does not take account of the text rotation or justification.
   */
  virtual void ComputeStringBounds(const svtkStdString& string, float bounds[4]) = 0;

  /**
   * Draw some text to the screen.
   */
  virtual void DrawString(float* point, const svtkUnicodeString& string) = 0;

  /**
   * Compute the bounds of the supplied string. The bounds will be copied to the
   * supplied bounds variable, the first two elements are the bottom corner of
   * the string, and the second two elements are the width and height of the
   * bounding box.
   * NOTE: This function does not take account of the text rotation or justification.
   */
  virtual void ComputeStringBounds(const svtkUnicodeString& string, float bounds[4]) = 0;

  /**
   * Compute the bounds of the supplied string while taking into account the
   * justification of the currently applied text property. Simple rotations
   * (0, 90, 180, 270) are also correctly taken into account.
   */
  virtual void ComputeJustifiedStringBounds(const char* string, float bounds[4]) = 0;

  /**
   * Draw text using MathText markup for mathematical equations. See
   * http://matplotlib.sourceforge.net/users/mathtext.html for more information.
   */
  virtual void DrawMathTextString(float* point, const svtkStdString& string) = 0;

  /**
   * Return true if MathText rendering available on this device.
   */
  virtual bool MathTextIsSupported();

  /**
   * Draw the supplied image at the given x, y (p[0], p[1]) (bottom corner),
   * scaled by scale (1.0 would match the image).
   */
  virtual void DrawImage(float p[2], float scale, svtkImageData* image) = 0;

  /**
   * Draw the supplied image at the given position. The origin, width, and
   * height are specified by the supplied svtkRectf variable pos. The image
   * will be drawn scaled to that size.
   */
  virtual void DrawImage(const svtkRectf& pos, svtkImageData* image) = 0;

  /**
   * Draw the supplied PolyData at the given x, y (p[0], p[1]) (bottom corner),
   * scaled by scale (1.0 would match the actual dataset).
   *
   * Only lines and polys are rendered. Only the x/y coordinates of the
   * polydata are used.
   *
   * @param p Offset to apply to polydata.
   * @param scale Isotropic scale for polydata. Applied after offset.
   * @param polyData Draw lines and polys from this dataset.
   * @param colors RGBA for points or cells, depending on value of scalarMode.
   * Must not be NULL.
   * @param scalarMode Must be either SVTK_SCALAR_MODE_USE_POINT_DATA or
   * SVTK_SCALAR_MODE_USE_CELL_DATA.
   *
   * The base implementation breaks the polydata apart and renders each polygon
   * individually using the device API. Subclasses should override this method
   * with a batch-drawing implementation if performance is a concern.
   */
  virtual void DrawPolyData(
    float p[2], float scale, svtkPolyData* polyData, svtkUnsignedCharArray* colors, int scalarMode);

  /**
   * Apply the supplied pen which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the svtkPen
   * object in the svtkContext2D, it does not hold a pointer to the supplied object.
   */
  virtual void ApplyPen(svtkPen* pen);

  //@{
  /**
   * Get the pen which controls the outlines of shapes, as well as lines,
   * points and related primitives. This object can be modified and the changes
   * will be reflected in subsequent drawing operations.
   */
  svtkGetObjectMacro(Pen, svtkPen);
  //@}

  /**
   * Apply the supplied brush which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the svtkBrush
   * object in the svtkContext2D, it does not hold a pointer to the supplied object.
   */
  virtual void ApplyBrush(svtkBrush* brush);

  //@{
  /**
   * Get the pen which controls the outlines of shapes as well as lines, points
   * and related primitives.
   */
  svtkGetObjectMacro(Brush, svtkBrush);
  //@}

  /**
   * Apply the supplied text property which controls how text is rendered.
   * This makes a deep copy of the svtkTextProperty object in the svtkContext2D,
   * it does not hold a pointer to the supplied object.
   */
  virtual void ApplyTextProp(svtkTextProperty* prop);

  //@{
  /**
   * Get the text properties object for the svtkContext2D.
   */
  svtkGetObjectMacro(TextProp, svtkTextProperty);
  //@}

  /**
   * Set the color for the device using unsigned char of length 4, RGBA.
   */
  virtual void SetColor4(unsigned char color[4]) = 0;

  enum TextureProperty
  {
    Nearest = 0x01,
    Linear = 0x02,
    Stretch = 0x04,
    Repeat = 0x08
  };
  /**
   * Set the texture for the device, it is used to fill the polygons
   */
  virtual void SetTexture(svtkImageData* image, int properties) = 0;

  /**
   * Set the point size for glyphs/sprites.
   */
  virtual void SetPointSize(float size) = 0;

  /**
   * Set the line width.
   */
  virtual void SetLineWidth(float width) = 0;

  /**
   * Set the line type type (using anonymous enum in svtkPen).
   */
  virtual void SetLineType(int type) = 0;

  /**
   * Get the width of the device in pixels.
   */
  virtual int GetWidth() { return this->Geometry[0]; }

  /**
   * Get the width of the device in pixels.
   */
  virtual int GetHeight() { return this->Geometry[1]; }

  /**
   * Set the model view matrix for the display
   */
  virtual void SetMatrix(svtkMatrix3x3* m) = 0;

  /**
   * Set the model view matrix for the display
   */
  virtual void GetMatrix(svtkMatrix3x3* m) = 0;

  /**
   * Multiply the current model view matrix by the supplied one
   */
  virtual void MultiplyMatrix(svtkMatrix3x3* m) = 0;

  /**
   * Push the current matrix onto the stack.
   */
  virtual void PushMatrix() = 0;

  /**
   * Pop the current matrix off of the stack.
   */
  virtual void PopMatrix() = 0;

  /**
   * Supply an int array of length 4 with x1, y1, width, height specifying
   * clipping region for the device in pixels.
   */
  virtual void SetClipping(int* x) = 0;

  /**
   * Disable clipping of the display.
   * Remove in a future release - retained for API compatibility.
   */
  virtual void DisableClipping() { this->EnableClipping(false); }

  /**
   * Enable or disable the clipping of the scene.
   */
  virtual void EnableClipping(bool enable) = 0;

  /**
   * Begin drawing, pass in the viewport to set up the view.
   */
  virtual void Begin(svtkViewport*) {}

  /**
   * End drawing, clean up the view.
   */
  virtual void End() {}

  /**
   * Tell if the device context is in BufferId creation mode.
   * Initial value is false.
   */
  virtual bool GetBufferIdMode() const;

  /**
   * Start BufferId creation Mode.
   * The default implementation is empty.
   * \pre not_yet: !GetBufferIdMode()
   * \pre bufferId_exists: bufferId!=0
   * \post started: GetBufferIdMode()
   */
  virtual void BufferIdModeBegin(svtkAbstractContextBufferId* bufferId);

  /**
   * Finalize BufferId creation Mode. It makes sure that the content of the
   * bufferId passed in argument of BufferIdModeBegin() is correctly set.
   * The default implementation is empty.
   * \pre started: GetBufferIdMode()
   * \post done: !GetBufferIdMode()
   */
  virtual void BufferIdModeEnd();

  virtual void SetViewportSize(const svtkVector2i& size) { this->ViewportSize = size; }
  svtkGetMacro(ViewportSize, svtkVector2i);

  virtual void SetViewportRect(const svtkRecti& rect) { this->ViewportRect = rect; }
  svtkGetMacro(ViewportRect, svtkRecti);

protected:
  svtkContextDevice2D();
  ~svtkContextDevice2D() override;

  /**
   * Store the width and height of the device in pixels.
   */
  int Geometry[2];

  /**
   * Store the size of the total viewport.
   */
  svtkVector2i ViewportSize;

  /**
   * Store our origin and size in the total viewport.
   */
  svtkRecti ViewportRect;

  svtkAbstractContextBufferId* BufferId;

  svtkPen* Pen;               // Outlining
  svtkBrush* Brush;           // Fills
  svtkTextProperty* TextProp; // Text property

private:
  svtkContextDevice2D(const svtkContextDevice2D&) = delete;
  void operator=(const svtkContextDevice2D&) = delete;
};

#endif // svtkContextDevice2D_h
