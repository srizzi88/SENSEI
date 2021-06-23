/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLContextDevice2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOpenGLContextDevice2D
 * @brief   Class for drawing 2D primitives using OpenGL
 * 1.1+.
 *
 *
 * This class takes care of drawing the 2D primitives for the svtkContext2D class.
 * In general this class should not be used directly, but called by svtkContext2D
 * which takes care of many of the higher level details.
 *
 * @sa
 * svtkOpenGL2ContextDevice2D
 */

#ifndef svtkOpenGLContextDevice2D_h
#define svtkOpenGLContextDevice2D_h

#include "svtkContextDevice2D.h"
#include "svtkRenderingContextOpenGL2Module.h" // For export macro

#include <list>   // for std::list
#include <vector> // STL Header

class svtkMatrix4x4;
class svtkOpenGLExtensionManager;
class svtkOpenGLHelper;
class svtkOpenGLRenderWindow;
class svtkPath;
class svtkRenderer;
class svtkShaderProgram;
class svtkStringToImage;
class svtkTransform;
class svtkViewport;
class svtkWindow;

class SVTKRENDERINGCONTEXTOPENGL2_EXPORT svtkOpenGLContextDevice2D : public svtkContextDevice2D
{
public:
  svtkTypeMacro(svtkOpenGLContextDevice2D, svtkContextDevice2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 2D Painter object.
   */
  static svtkOpenGLContextDevice2D* New();

  /**
   * Draw a poly line using the points - fastest code path due to memory
   * layout of the coordinates. The line will be colored by colors array
   * which has nc_comps components
   */
  void DrawPoly(float* f, int n, unsigned char* colors = nullptr, int nc_comps = 0) override;

  /**
   * Draw lines using the points - memory layout is as follows:
   * l1p1,l1p2,l2p1,l2p2... The lines will be colored by colors array
   * which has nc_comps components.
   * \sa DrawPoly()
   */
  void DrawLines(float* f, int n, unsigned char* colors = nullptr, int nc_comps = 0) override;

  /**
   * Draw a series of points - fastest code path due to memory
   * layout of the coordinates. Points are colored by colors array
   * which has nc_comps components
   */
  void DrawPoints(float* points, int n, unsigned char* colors = nullptr, int nc_comps = 0) override;

  /**
   * Draw a series of point sprites, images centred at the points supplied.
   * The supplied svtkImageData is the sprite to be drawn, only squares will be
   * drawn and the size is set using SetPointSize. Points are colored by colors
   * array which has nc_comps components - this part is optional.
   */
  void DrawPointSprites(svtkImageData* sprite, float* points, int n, unsigned char* colors = nullptr,
    int nc_comps = 0) override;

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
  void DrawMarkers(int shape, bool highlight, float* points, int n, unsigned char* colors = nullptr,
    int nc_comps = 0) override;

  //@{
  /**
   * Adjust the size of the MarkerCache. This implementation generates point
   * sprites for each mark size/shape and uses DrawPointSprites to render them.
   * The number of cached markers can be accessed with this function.
   */
  svtkSetMacro(MaximumMarkerCacheSize, int);
  svtkGetMacro(MaximumMarkerCacheSize, int);
  //@}

  /**
   * Draws a rectangle
   */
  void DrawQuad(float* points, int n) override;

  /**
   * Draws a rectangle
   */
  void DrawQuadStrip(float* points, int n) override;

  /**
   * Draw a polygon using the specified number of points.
   */
  void DrawPolygon(float*, int) override;
  void DrawColoredPolygon(
    float* points, int numPoints, unsigned char* colors = nullptr, int nc_comps = 0) override;

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
    float startAngle, float stopAngle) override;

  /**
   * Draw an elliptic arc with center at x,y with radii rX and rY between
   * angles startAngle and stopAngle (expressed in degrees).
   * \pre positive_rX: rX>=0
   * \pre positive_rY: rY>=0
   */
  void DrawEllipticArc(
    float x, float y, float rX, float rY, float startAngle, float stopAngle) override;

  /**
   * Draw some text to the screen!
   */
  void DrawString(float* point, const svtkStdString& string) override;

  /**
   * Compute the bounds of the supplied string. The bounds will be copied to the
   * supplied bounds variable, the first two elements are the bottom corner of
   * the string, and the second two elements are the width and height of the
   * bounding box. An empty bounding box (0, 0, 0, 0) is returned for an
   * empty string or string with only characters that cannot be rendered.
   * NOTE: This function does not take account of the text rotation.
   */
  void ComputeStringBounds(const svtkStdString& string, float bounds[4]) override;

  /**
   * Draw some text to the screen.
   */
  void DrawString(float* point, const svtkUnicodeString& string) override;

  /**
   * Compute the bounds of the supplied string. The bounds will be copied to the
   * supplied bounds variable, the first two elements are the bottom corner of
   * the string, and the second two elements are the width and height of the
   * bounding box. An empty bounding box (0, 0, 0, 0) is returned for an
   * empty string or string with only characters that cannot be rendered.
   * NOTE: This function does not take account of the text rotation.
   */
  void ComputeStringBounds(const svtkUnicodeString& string, float bounds[4]) override;

  /**
   * Compute the bounds of the supplied string while taking into account the
   * justification of the currently applied text property. Simple rotations
   * (0, 90, 180, 270 degrees) are also properly taken into account.
   */
  void ComputeJustifiedStringBounds(const char* string, float bounds[4]) override;

  /**
   * Draw text using MathText markup for mathematical equations. See
   * http://matplotlib.sourceforge.net/users/mathtext.html for more information.
   */
  void DrawMathTextString(float point[2], const svtkStdString& string) override;

  /**
   * Draw the supplied image at the given x, y (p[0], p[1]) (bottom corner),
   * scaled by scale (1.0 would match the image).
   */
  void DrawImage(float p[2], float scale, svtkImageData* image) override;

  /**
   * Draw the supplied image at the given position. The origin, width, and
   * height are specified by the supplied svtkRectf variable pos. The image
   * will be drawn scaled to that size.
   */
  void DrawImage(const svtkRectf& pos, svtkImageData* image) override;

  /**
   * Draw the supplied PolyData at the given x, y (p[0], p[1]) (bottom corner),
   * scaled by scale (1.0 would match the actual dataset).
   */
  void DrawPolyData(float p[2], float scale, svtkPolyData* polyData, svtkUnsignedCharArray* colors,
    int scalarMode) override;

  /**
   * Set the color for the device using unsigned char of length 4, RGBA.
   */
  void SetColor4(unsigned char color[4]) override;

  /**
   * Set the color for the device using unsigned char of length 3, RGB.
   */
  virtual void SetColor(unsigned char color[3]);

  /**
   * Set the texture for the device, it is used to fill the polygons
   */
  void SetTexture(svtkImageData* image, int properties = 0) override;

  /**
   * Set the point size for glyphs/sprites.
   */
  void SetPointSize(float size) override;

  /**
   * Set the line width for glyphs/sprites.
   */
  void SetLineWidth(float width) override;

  /**
   * Set the line type type (using anonymous enum in svtkPen).
   */
  void SetLineType(int type) override;

  /**
   * Multiply the current model view matrix by the supplied one.
   */
  void MultiplyMatrix(svtkMatrix3x3* m) override;

  /**
   * Set the model view matrix for the display
   */
  void SetMatrix(svtkMatrix3x3* m) override;

  /**
   * Set the model view matrix for the display
   */
  void GetMatrix(svtkMatrix3x3* m) override;

  /**
   * Push the current matrix onto the stack.
   */
  void PushMatrix() override;

  /**
   * Pop the current matrix off of the stack.
   */
  void PopMatrix() override;

  /*
   * Supply an int array of length 4 with x1, y1, width, height specifying
   * clipping region for the device in pixels.
   */
  void SetClipping(int* x) override;

  /**
   * Disable clipping of the display.
   */
  void EnableClipping(bool enable) override;

  /**
   * Begin drawing, pass in the viewport to set up the view.
   */
  void Begin(svtkViewport* viewport) override;

  /**
   * End drawing, clean up the view.
   */
  void End() override;

  /**
   * Start BufferId creation Mode.
   * The default implementation is empty.
   * \pre not_yet: !GetBufferIdMode()
   * \pre bufferId_exists: bufferId!=0
   * \post started: GetBufferIdMode()
   */
  void BufferIdModeBegin(svtkAbstractContextBufferId* bufferId) override;

  /**
   * Finalize BufferId creation Mode. It makes sure that the content of the
   * bufferId passed in argument of BufferIdModeBegin() is correctly set.
   * The default implementation is empty.
   * \pre started: GetBufferIdMode()
   * \post done: !GetBufferIdMode()
   */
  void BufferIdModeEnd() override;

  /**
   * Force the use of the freetype based render strategy. If Qt is available
   * then freetype will be used preferentially, otherwise this has no effect.
   * Returns true on success.
   */
  bool SetStringRendererToFreeType();

  /**
   * Force the use of the Qt based string render strategy. If Qt is not available
   * then freetype will be used and this will return false.
   */
  bool SetStringRendererToQt();

  /**
   * Check whether the current context device has support for GLSL.
   */
  bool HasGLSL();

  //@{
  /**
   * Get the active RenderWindow of the device. Will return null if not active.
   */
  svtkGetObjectMacro(RenderWindow, svtkOpenGLRenderWindow);
  //@}

  /**
   * Release any graphics resources that are being consumed by this device.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  virtual void ReleaseGraphicsResources(svtkWindow* window);

  //@{
  /**
   * Get the projection matrix this is needed
   */
  svtkMatrix4x4* GetProjectionMatrix();
  svtkMatrix4x4* GetModelMatrix();
  //@}

protected:
  svtkOpenGLContextDevice2D();
  ~svtkOpenGLContextDevice2D() override;

  /**
   * Factorized code called by DrawEllipseWedge() and DrawEllipticArc()
   * to figure out the number of iterations required to make an arc smooth.
   * \pre positive_rX: rX>=0.0f
   * \pre positive_rY: rY>=0.0f
   * \pre not_both_null: rX>0.0 || rY>0.0
   */
  int GetNumberOfArcIterations(float rX, float rY, float startAngle, float stopAngle);

  /**
   * We need to store a pointer to the renderer for the text rendering
   */
  svtkRenderer* Renderer;

  /**
   * Is the device currently rendering? Prevent multiple End() calls.
   */
  bool InRender;

  //@{
  /**
   * Private data pointer of the class
   */
  class Private;
  Private* Storage;

  class CellArrayHelper;
  CellArrayHelper* PolyDataImpl;
  //@}

  /**
   * The OpenGL render window being used by the device
   */
  svtkOpenGLRenderWindow* RenderWindow;

  svtkOpenGLHelper* LinesCBO; // vertex + color
  void ReadyLinesCBOProgram();
  svtkOpenGLHelper* LinesBO; // vertex
  void ReadyLinesBOProgram();
  svtkOpenGLHelper* VCBO; // vertex + color
  void ReadyVCBOProgram();
  svtkOpenGLHelper* VBO; // vertex
  void ReadyVBOProgram();
  svtkOpenGLHelper* VTBO; // vertex + tcoord
  void ReadyVTBOProgram();
  svtkOpenGLHelper* SCBO; // sprite + color
  void ReadySCBOProgram();
  svtkOpenGLHelper* SBO; // sprite
  void ReadySBOProgram();

  void SetMatrices(svtkShaderProgram* prog);
  void BuildVBO(
    svtkOpenGLHelper* cbo, float* v, int nv, unsigned char* coolors, int nc, float* tcoords);
  void CoreDrawTriangles(
    std::vector<float>& tverts, unsigned char* colors = nullptr, int numComp = 0);
  // used for stipples
  unsigned short LinePattern;

  //@{
  /**
   * Draw the markers as paths/polydata instead of sprites for detailed GL2PS
   * capture.
   */
  void DrawMarkersGL2PS(
    int shape, bool highlight, float* points, int n, unsigned char* colors, int nc_comps);
  void DrawCrossMarkersGL2PS(
    bool highlight, float* points, int n, unsigned char* colors, int nc_comps);
  void DrawPlusMarkersGL2PS(
    bool highlight, float* points, int n, unsigned char* colors, int nc_comps);
  void DrawSquareMarkersGL2PS(
    bool highlight, float* points, int n, unsigned char* colors, int nc_comps);
  void DrawCircleMarkersGL2PS(
    bool highlight, float* points, int n, unsigned char* colors, int nc_comps);
  void DrawDiamondMarkersGL2PS(
    bool highlight, float* points, int n, unsigned char* colors, int nc_comps);
  //@}

  //@{
  /**
   * Embed an RGBA image in the GL2PS output at the supplied point.
   */
  void DrawImageGL2PS(float p[2], svtkImageData* image);
  void DrawImageGL2PS(float p[2], float scale, svtkImageData* image);
  void DrawImageGL2PS(const svtkRectf& rect, svtkImageData* image);
  //@}

  //@{
  /**
   * Inject smooth primitives into the GL2PS stream.
   */
  void DrawCircleGL2PS(float x, float y, float rX, float rY);
  void DrawWedgeGL2PS(float x, float y, float outRx, float outRy, float inRx, float inRy);
  //@}

  /**
   * Add an ellipse to a svtkPath. Used during GL2PS export.
   */
  void AddEllipseToPath(svtkPath* path, float x, float y, float rx, float ry, bool reverse);

  /**
   * Transform the path using the current modelview matrix.
   */
  void TransformPath(svtkPath* path) const;

  /**
   * Transform the 2D point using the current modelview matrix.
   */
  void TransformPoint(float& x, float& y) const;

  /**
   * Transform the width and height from pixels to data units.
   */
  void TransformSize(float& dx, float& dy) const;

private:
  svtkOpenGLContextDevice2D(const svtkOpenGLContextDevice2D&) = delete;
  void operator=(const svtkOpenGLContextDevice2D&) = delete;

  /**
   * Retrieve a point sprite image for a given marker shape and size. The
   * image data will be either generated or retrieved from a cache. This class
   * manages the lifetime of returned image data. Setting \a highlight to
   * true produces an alternate (usually thicker) version of the marker.
   */
  svtkImageData* GetMarker(int shape, int size, bool highlight);

  class svtkMarkerCacheObject
  {
  public:
    svtkTypeUInt64 Key;
    svtkImageData* Value;
    bool operator==(svtkTypeUInt64 key) { return this->Key == key; }
  };

  void ComputeStringBoundsInternal(const svtkUnicodeString& string, float bounds[4]);

  svtkTransform* ProjectionMatrix;
  svtkTransform* ModelMatrix;

  std::list<svtkMarkerCacheObject> MarkerCache;
  int MaximumMarkerCacheSize;

  /**
   * Generate the marker with the specified shape and size. This function should
   * not be used directly -- use GetMarker, which caches results, instead.
   */
  svtkImageData* GenerateMarker(int shape, int size, bool highlight);
};

#endif // svtkOpenGLContextDevice2D_h
