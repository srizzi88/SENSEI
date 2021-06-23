/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContext2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkContext2D
 * @brief   Class for drawing 2D primitives to a graphical context.
 *
 *
 * This defines the interface for drawing onto a 2D context. The context must
 * be set up with a svtkContextDevice2D derived class that provides the functions
 * to facilitate the low level calls to the context. Currently only an OpenGL
 * based device is provided, but this could be extended in the future.
 */

#ifndef svtkContext2D_h
#define svtkContext2D_h

#include "svtkObject.h"
#include "svtkRenderingContext2DModule.h" // For export macro

class svtkWindow;

class svtkContext3D;
class svtkStdString;
class svtkUnicodeString;
class svtkTextProperty;

class svtkPoints2D;
class svtkVector2f;
class svtkRectf;
class svtkUnsignedCharArray;
class svtkContextDevice2D;
class svtkPen;
class svtkBrush;
class svtkImageData;
class svtkPolyData;
class svtkTransform2D;
class svtkAbstractContextBufferId;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkContext2D : public svtkObject
{
public:
  svtkTypeMacro(svtkContext2D, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 2D Painter object.
   */
  static svtkContext2D* New();

  /**
   * Begin painting on a svtkContextDevice2D, no painting can occur before this call
   * has been made. Only one painter is allowed at a time on any given paint
   * device. Returns true if successful, otherwise false.
   */
  bool Begin(svtkContextDevice2D* device);

  svtkGetObjectMacro(Device, svtkContextDevice2D);

  /**
   * Ends painting on the device, you would not usually need to call this as it
   * should be called by the destructor. Returns true if the painter is no
   * longer active, otherwise false.
   */
  bool End();

  /**
   * Tell if the context is in BufferId creation mode. Initial value is false.
   */
  bool GetBufferIdMode() const;

  /**
   * Start BufferId creation Mode.
   * \pre not_yet: !GetBufferIdMode()
   * \pre bufferId_exists: bufferId!=0
   * \post started: GetBufferIdMode()
   */
  void BufferIdModeBegin(svtkAbstractContextBufferId* bufferId);

  /**
   * Finalize BufferId creation Mode. It makes sure that the content of the
   * bufferId passed in argument of BufferIdModeBegin() is correctly set.
   * \pre started: GetBufferIdMode()
   * \post done: !GetBufferIdMode()
   */
  void BufferIdModeEnd();

  /**
   * Draw a line between the specified points.
   */
  void DrawLine(float x1, float y1, float x2, float y2);

  /**
   * Draw a line between the specified points.
   */
  void DrawLine(float p[4]);

  /**
   * Draw a line between the specified points.
   * Note: Fastest path - points packed in x and y.
   */
  void DrawLine(svtkPoints2D* points);

  /**
   * Draw a poly line between the specified points.
   */
  void DrawPoly(float* x, float* y, int n);

  /**
   * Draw a poly line between the specified points - fastest code path due to
   * memory layout of the coordinates.
   */
  void DrawPoly(svtkPoints2D* points);

  /**
   * Draw a poly line between the specified points, where the float array is of
   * size 2*n and the points are packed x1, y1, x2, y2 etc.
   * Note: Fastest code path - points packed in x and y.
   */
  void DrawPoly(float* points, int n);

  /**
   * Draw a poly line between the specified points, where the float array is of
   * size 2*n and the points are packed x1, y1, x2, y2 etc. The line will be colored by
   * the colors array, which must have nc_comps components (defining a single color).
   * Note: Fastest code path - points packed in x and y.
   */
  void DrawPoly(float* points, int n, unsigned char* colors, int nc_comps);

  /**
   * Draw multiple lines between the specified pairs of points.
   * \sa DrawPoly()
   */
  void DrawLines(svtkPoints2D* points);

  /**
   * Draw multiple lines between the specified pairs of points
   * \sa DrawPoly()
   */
  void DrawLines(float* points, int n);

  /**
   * Draw a point at the supplied x and y coordinate
   */
  void DrawPoint(float x, float y);

  /**
   * Draw the specified number of points using the x and y arrays supplied
   */
  void DrawPoints(float* x, float* y, int n);

  /**
   * Draw a poly line between the specified points - fastest code path due to
   * memory layout of the coordinates.
   */
  void DrawPoints(svtkPoints2D* points);

  /**
   * Draw a poly line between the specified points, where the float array is of
   * size 2*n and the points are packed x1, y1, x2, y2 etc.
   * Note: Fastest code path - points packed in x and y.
   */
  void DrawPoints(float* points, int n);

  /**
   * Draw a series of point sprites, images centred at the points supplied.
   * The supplied svtkImageData is the sprite to be drawn, only squares will be
   * drawn and the size is set using SetPointSize.
   */
  void DrawPointSprites(svtkImageData* sprite, svtkPoints2D* points);

  //@{
  /**
   * Draw a series of point sprites, images centred at the points supplied.
   * The supplied svtkImageData is the sprite to be drawn, only squares will be
   * drawn and the size is set using SetPointSize. Points will be colored by
   * the colors array, which must be the same length as points.
   */
  void DrawPointSprites(svtkImageData* sprite, svtkPoints2D* points, svtkUnsignedCharArray* colors);
  void DrawPointSprites(
    svtkImageData* sprite, float* points, int n, unsigned char* colors, int nc_comps);
  //@}

  /**
   * Draw a series of point sprites, images centred at the points supplied.
   * The supplied svtkImageData is the sprite to be drawn, only squares will be
   * drawn and the size is set using SetPointSize.
   */
  void DrawPointSprites(svtkImageData* sprite, float* points, int n);

  //@{
  /**
   * Draw a series of markers centered at the points supplied. The \a shape
   * argument controls the marker shape, and can be one of
   * - SVTK_MARKER_CROSS
   * - SVTK_MARKER_PLUS
   * - SVTK_MARKER_SQUARE
   * - SVTK_MARKER_CIRCLE
   * - SVTK_MARKER_DIAMOND
   * Marker size is determined by the current pen width.
   * \param colors is an optional array of colors.
   * \param nc_comps is the number of components for the color.
   */
  virtual void DrawMarkers(
    int shape, bool highlight, float* points, int n, unsigned char* colors, int nc_comps);
  virtual void DrawMarkers(int shape, bool highlight, float* points, int n);
  virtual void DrawMarkers(int shape, bool highlight, svtkPoints2D* points);
  virtual void DrawMarkers(
    int shape, bool highlight, svtkPoints2D* points, svtkUnsignedCharArray* colors);
  //@}

  /**
   * Draw a rectangle with origin at x, y and width w, height h
   */
  void DrawRect(float x, float y, float w, float h);

  //@{
  /**
   * Draw a quadrilateral at the specified points (4 points, 8 floats in x, y).
   */
  void DrawQuad(float x1, float y1, float x2, float y2, float x3, float y3, float x4, float y4);
  void DrawQuad(float* p);
  //@}

  //@{
  /**
   * Draw a strip of quads
   */
  void DrawQuadStrip(svtkPoints2D* points);
  void DrawQuadStrip(float* p, int n);
  //@}

  /**
   * Draw a polygon specified specified by the points using the x and y arrays
   * supplied
   */
  void DrawPolygon(float* x, float* y, int n);

  /**
   * Draw a polygon defined by the specified points - fastest code path due to
   * memory layout of the coordinates.
   */
  void DrawPolygon(svtkPoints2D* points);

  /**
   * Draw a polygon defined by the specified points, where the float array is
   * of size 2*n and the points are packed x1, y1, x2, y2 etc.
   * Note: Fastest code path - points packed in x and y.
   */
  void DrawPolygon(float* points, int n);

  /**
   * Draw a polygon specified specified by the points using the x and y arrays
   * supplied
   */
  void DrawPolygon(float* x, float* y, int n, unsigned char* color, int nc_comps);

  /**
   * Draw a polygon defined by the specified points - fastest code path due to
   * memory layout of the coordinates.
   */
  void DrawPolygon(svtkPoints2D* points, unsigned char* color, int nc_comps);

  /**
   * Draw a polygon defined by the specified points, where the float array is
   * of size 2*n and the points are packed x1, y1, x2, y2 etc.
   * Note: Fastest code path - points packed in x and y.
   */
  void DrawPolygon(float* points, int n, unsigned char* color, int nc_comps);

  /**
   * Draw an ellipse with center at x, y and radii rx, ry.
   * \pre positive_rx: rx>=0
   * \pre positive_ry: ry>=0
   */
  void DrawEllipse(float x, float y, float rx, float ry);

  /**
   * Draw a circular wedge with center at x, y, outer radius outRadius,
   * inner radius inRadius between angles startAngle and stopAngle
   * (expressed in degrees).
   * \pre positive_outRadius: outRadius>=0
   * \pre positive_inRadius: inRadius>=0
   * \pre ordered_radii: inRadius<=outRadius
   */
  void DrawWedge(
    float x, float y, float outRadius, float inRadius, float startAngle, float stopAngle);

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
  void DrawEllipseWedge(float x, float y, float outRx, float outRy, float inRx, float inRy,
    float startAngle, float stopAngle);

  /**
   * Draw a circular arc with center at x,y with radius r between angles
   * startAngle and stopAngle (expressed in degrees).
   * \pre positive_radius: r>=0
   */
  void DrawArc(float x, float y, float r, float startAngle, float stopAngle);

  /**
   * Draw an elliptic arc with center at x,y with radii rX and rY between
   * angles startAngle and stopAngle (expressed in degrees).
   * \pre positive_rX: rX>=0
   * \pre positive_rY: rY>=0
   */
  void DrawEllipticArc(float x, float y, float rX, float rY, float startAngle, float stopAngle);

  /**
   * Draw the supplied image at the given x, y location (bottom corner).
   */
  void DrawImage(float x, float y, svtkImageData* image);

  /**
   * Draw the supplied image at the given x, y location (bottom corner).
   * Scale the supplied image by scale.
   */
  void DrawImage(float x, float y, float scale, svtkImageData* image);

  /**
   * Draw the supplied image at the given position. The origin, width, and
   * height are specified by the supplied svtkRectf variable pos. The image
   * will be drawn scaled to that size.
   */
  void DrawImage(const svtkRectf& pos, svtkImageData* image);

  /**
   * Draw the supplied polyData at the given x, y position (bottom corner).
   * \note Supports only 2D meshes.
   */
  void DrawPolyData(
    float x, float y, svtkPolyData* polyData, svtkUnsignedCharArray* colors, int scalarMode);

  //@{
  /**
   * Draw some text to the screen in a bounding rectangle with the alignment
   * of the text properties respecting the rectangle. The points should be
   * supplied as bottom corner (x, y), width, height.
   */
  void DrawStringRect(svtkPoints2D* rect, const svtkStdString& string);
  void DrawStringRect(svtkPoints2D* rect, const svtkUnicodeString& string);
  void DrawStringRect(svtkPoints2D* rect, const char* string);
  //@}

  //@{
  /**
   * Draw some text to the screen.
   */
  void DrawString(svtkPoints2D* point, const svtkStdString& string);
  void DrawString(float x, float y, const svtkStdString& string);
  void DrawString(svtkPoints2D* point, const svtkUnicodeString& string);
  void DrawString(float x, float y, const svtkUnicodeString& string);
  void DrawString(svtkPoints2D* point, const char* string);
  void DrawString(float x, float y, const char* string);
  //@}

  //@{
  /**
   * Compute the bounds of the supplied string. The bounds will be copied to the
   * supplied bounds variable, the first two elements are the bottom corner of
   * the string, and the second two elements are the width and height of the
   * bounding box.

   * NOTE:the text justification from the current text property is
   * NOT considered when computing these bounds.
   */
  void ComputeStringBounds(const svtkStdString& string, svtkPoints2D* bounds);
  void ComputeStringBounds(const svtkStdString& string, float bounds[4]);
  void ComputeStringBounds(const svtkUnicodeString& string, svtkPoints2D* bounds);
  void ComputeStringBounds(const svtkUnicodeString& string, float bounds[4]);
  void ComputeStringBounds(const char* string, svtkPoints2D* bounds);
  void ComputeStringBounds(const char* string, float bounds[4]);
  //@}

  /**
   * Compute the bounds of the supplied string while taking into account the
   * justification and rotation of the currently applied text property.
   */
  void ComputeJustifiedStringBounds(const char* string, float bounds[4]);

  /**
   * Calculate the largest possible font size where the supplied string will fit
   * within the specified bounds.  In addition to being returned, this font size
   * is also used to update the svtkTextProperty used by this object.
   * NOTE: text rotation is ignored for the purposes of this function.
   */
  int ComputeFontSizeForBoundedString(const svtkStdString& string, float width, float height);

  //@{
  /**
   * Draw a MathText formatted equation to the screen. See
   * http://matplotlib.sourceforge.net/users/mathtext.html for more information.
   * MathText requires matplotlib and python, and the svtkMatplotlib module must
   * be enabled manually during build configuration. This method will do nothing
   * but print a warning if svtkMathTextUtilities::GetInstance() returns NULL.
   */
  void DrawMathTextString(svtkPoints2D* point, const svtkStdString& string);
  void DrawMathTextString(float x, float y, const svtkStdString& string);
  void DrawMathTextString(svtkPoints2D* point, const char* string);
  void DrawMathTextString(float x, float y, const char* string);
  //@}

  //@{
  /**
   * Draw a MathText formatted equation to the screen. See
   * http://matplotlib.sourceforge.net/users/mathtext.html for more information.
   * MathText requires matplotlib and python, and the svtkMatplotlib module must
   * be enabled manually during build configuration.
   * If MathText is not available on the target device the non-MathText string
   * in "fallback" is rendered using DrawString.
   */
  void DrawMathTextString(
    svtkPoints2D* point, const svtkStdString& string, const svtkStdString& fallback);
  void DrawMathTextString(
    float x, float y, const svtkStdString& string, const svtkStdString& fallback);
  void DrawMathTextString(svtkPoints2D* point, const char* string, const char* fallback);
  void DrawMathTextString(float x, float y, const char* string, const char* fallback);
  //@}

  /**
   * Return true if MathText rendering available on the current device.
   */
  bool MathTextIsSupported();

  /**
   * Apply the supplied pen which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the svtkPen
   * object in the svtkContext2D, it does not hold a pointer to the supplied object.
   */
  void ApplyPen(svtkPen* pen);

  /**
   * Get the pen which controls the outlines of shapes, as well as lines,
   * points and related primitives. This object can be modified and the changes
   * will be reflected in subsequent drawing operations.
   */
  svtkPen* GetPen();

  /**
   * Apply the supplied brush which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the svtkBrush
   * object in the svtkContext2D, it does not hold a pointer to the supplied object.
   */
  void ApplyBrush(svtkBrush* brush);

  /**
   * Get the pen which controls the outlines of shapes as well as lines, points
   * and related primitives.
   */
  svtkBrush* GetBrush();

  /**
   * Apply the supplied text property which controls how text is rendered.
   * This makes a deep copy of the svtkTextProperty object in the svtkContext2D,
   * it does not hold a pointer to the supplied object.
   */
  void ApplyTextProp(svtkTextProperty* prop);

  /**
   * Get the text properties object for the svtkContext2D.
   */
  svtkTextProperty* GetTextProp();

  /**
   * Set the transform for the context, the underlying device will use the
   * matrix of the transform. Note, this is set immediately, later changes to
   * the matrix will have no effect until it is set again.
   */
  void SetTransform(svtkTransform2D* transform);

  /**
   * Compute the current transform applied to the context.
   */
  svtkTransform2D* GetTransform();

  /**
   * Append the transform for the context, the underlying device will use the
   * matrix of the transform. Note, this is set immediately, later changes to
   * the matrix will have no effect until it is set again. The matrix of the
   * transform will multiply the current context transform.
   */
  void AppendTransform(svtkTransform2D* transform);

  //@{
  /**
   * Push/pop the transformation matrix for the painter (sets the underlying
   * matrix for the device when available).
   */
  void PushMatrix();
  void PopMatrix();
  //@}

  /**
   * Apply id as a color.
   */
  void ApplyId(svtkIdType id);

  /**
   * Float to int conversion, performs truncation but with a rounding
   * tolerance for float values that are within 1/256 of their closest
   * integer.
   */
  static int FloatToInt(float x);

  //@{
  /**
   * Get the svtkContext3D device, in order to do some 3D rendering. This API
   * is very experimental, and may be moved around.
   */
  svtkGetObjectMacro(Context3D, svtkContext3D);
  virtual void SetContext3D(svtkContext3D* context);
  //@}

protected:
  svtkContext2D();
  ~svtkContext2D() override;

  svtkContextDevice2D* Device; // The underlying device
  svtkTransform2D* Transform;  // Current transform

  svtkAbstractContextBufferId* BufferId;
  svtkContext3D* Context3D; // May be very temporary - get at a 3D version.

private:
  svtkContext2D(const svtkContext2D&) = delete;
  void operator=(const svtkContext2D&) = delete;

  /**
   * Calculate position of text for rendering in a rectangle.
   * The first point in rect is the bottom left corner of
   * the text box, and the second point is the width and
   * height of the rect.
   */
  svtkVector2f CalculateTextPosition(svtkPoints2D* rect);

  /**
   * Calculate position of text for rendering in a rectangle.
   * The first two elements of rect represent the lower left
   * corner of the text box, and the 3rd and 4th elements
   * represent width and height.
   */
  svtkVector2f CalculateTextPosition(float rect[4]);
};

inline int svtkContext2D::FloatToInt(float x)
{
  // Use a tolerance of 1/256 of a pixel when converting.
  // A float has only 24 bits of precision, so we cannot
  // make the tolerance too small.  For example, a tolerance
  // of 2^-8 means that the tolerance will be significant
  // for float values up to 2^16 or 65536.0.  But a
  // tolerance of 2^-16 would only be significant for
  // float values up to 2^8 or 256.0.  A small tolerance
  // disappears into insignificance when added to a large float.
  float tol = 0.00390625; // 1.0/256.0
  tol = (x >= 0 ? tol : -tol);
  return static_cast<int>(x + tol);
}

#endif // svtkContext2D_h
