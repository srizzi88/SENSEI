/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextRenderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkTextRenderer
 * @brief   Interface for generating images and path data from
 * string data, using multiple backends.
 *
 *
 * svtkTextRenderer produces images, bounding boxes, and svtkPath
 * objects that represent text. The advantage of using this class is to easily
 * integrate mathematical expressions into renderings by automatically switching
 * between FreeType and MathText backends. If the input string contains at least
 * two "$" symbols separated by text, the MathText backend will be used. If
 * the string does not meet this criteria, or if no MathText implementation is
 * available, the faster FreeType rendering facilities are used. Literal $
 * symbols can be used by escaping them with backslashes, "\$" (or "\\$" if the
 * string is set programmatically).
 *
 * For example, "Acceleration ($\\frac{m}{s^2}$)" will use MathText, but
 * "\\$500, \\$100" will use FreeType.
 *
 * By default, the backend is set to Detect, which determines the backend based
 * on the contents of the string. This can be changed by setting the
 * DefaultBackend ivar.
 *
 * Note that this class is abstract -- link to the svtkRenderingFreetype module
 * to get the default implementation.
 */

#ifndef svtkTextRenderer_h
#define svtkTextRenderer_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkTuple.h"               // For metrics struct
#include "svtkVector.h"              // For metrics struct

class svtkImageData;
class svtkPath;
class svtkStdString;
class svtkUnicodeString;
class svtkTextProperty;

namespace svtksys
{
class RegularExpression;
}

class SVTKRENDERINGCORE_EXPORT svtkTextRendererCleanup
{
public:
  svtkTextRendererCleanup();
  ~svtkTextRendererCleanup();

private:
  svtkTextRendererCleanup(const svtkTextRendererCleanup& other) = delete;
  svtkTextRendererCleanup& operator=(const svtkTextRendererCleanup& rhs) = delete;
};

class SVTKRENDERINGCORE_EXPORT svtkTextRenderer : public svtkObject
{
public:
  struct Metrics
  {
    /**
     * Construct a Metrics object with all members initialized to 0.
     */
    Metrics()
      : BoundingBox(0)
      , TopLeft(0)
      , TopRight(0)
      , BottomLeft(0)
      , BottomRight(0)
      , Ascent(0)
      , Descent(0)
    {
    }

    /**
     * The axis-aligned bounding box of the rendered text and background, in
     * pixels. The origin of the bounding box is the anchor point of the data
     * when considering justification. Layout is { xMin, xMax, yMin, yMax }.
     */
    svtkTuple<int, 4> BoundingBox;

    //@{
    /**
     * The corners of the rendered text (or background, if applicable), in pixels.
     * Uses the same origin as BoundingBox.
     */
    svtkVector2i TopLeft;
    svtkVector2i TopRight;
    svtkVector2i BottomLeft;
    svtkVector2i BottomRight;
    //@}

    /**
     * Vectors representing the rotated ascent and descent of the text. This is
     * the distance above or below the baseline. Not all backends support this,
     * and may leave these vectors set to 0.
     * @{
     */
    svtkVector2i Ascent;
    svtkVector2i Descent;
    /**@}*/
  };

  svtkTypeMacro(svtkTextRenderer, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This is a singleton pattern New. There will be only ONE reference
   * to a svtkTextRenderer subclass object per process. Clients that
   * call this method must use Delete() on the object so that reference
   * counting will work. The single instance will be unreferenced when
   * the program exits. You should just use the static GetInstance() method
   * anyway to get the singleton. This method may return NULL if the object
   * factory cannot find an override.
   */
  static svtkTextRenderer* New();

  /**
   * Return the singleton instance with no reference counting. May return NULL
   * if the object factory cannot find an override.
   */
  static svtkTextRenderer* GetInstance();

  /**
   * Available backends. FreeType and MathText are provided in the default
   * implementation of this interface. Enum values less than 16 are reserved.
   * Custom overrides should define other backends starting at 16.
   */
  enum Backend
  {
    Default = -1,
    Detect = 0,
    FreeType,
    MathText,

    UserBackend = 16
  };

  //@{
  /**
   * The backend to use when none is specified. Default: Detect
   */
  svtkSetMacro(DefaultBackend, int);
  svtkGetMacro(DefaultBackend, int);
  //@}

  //@{
  /**
   * Determine the appropriate back end needed to render the given string.
   */
  virtual int DetectBackend(const svtkStdString& str);
  virtual int DetectBackend(const svtkUnicodeString& str);
  //@}

  /**
   * Test for availability of various backends
   */
  virtual bool FreeTypeIsSupported() { return false; }
  virtual bool MathTextIsSupported() { return false; }

  //@{
  /**
   * Given a text property and a string, get the bounding box {xmin, xmax,
   * ymin, ymax} of the rendered string in pixels. The origin of the bounding
   * box is the anchor point described by the horizontal and vertical
   * justification text property variables.
   * Return true on success, false otherwise.
   */
  bool GetBoundingBox(
    svtkTextProperty* tprop, const svtkStdString& str, int bbox[4], int dpi, int backend = Default)
  {
    return this->GetBoundingBoxInternal(tprop, str, bbox, dpi, backend);
  }
  bool GetBoundingBox(svtkTextProperty* tprop, const svtkUnicodeString& str, int bbox[4], int dpi,
    int backend = Default)
  {
    return this->GetBoundingBoxInternal(tprop, str, bbox, dpi, backend);
  }
  //@}

  //@{
  /**
   * Given a text property and a string, get some metrics for the rendered
   * string.
   * Return true on success, false otherwise.
   */
  bool GetMetrics(svtkTextProperty* tprop, const svtkStdString& str, Metrics& metrics, int dpi,
    int backend = Default)
  {
    return this->GetMetricsInternal(tprop, str, metrics, dpi, backend);
  }
  bool GetMetrics(svtkTextProperty* tprop, const svtkUnicodeString& str, Metrics& metrics, int dpi,
    int backend = Default)
  {
    return this->GetMetricsInternal(tprop, str, metrics, dpi, backend);
  }
  //@}

  //@{
  /**
   * Given a text property and a string, this function initializes the
   * svtkImageData *data and renders it in a svtkImageData.
   * Return true on success, false otherwise. If using the overload that
   * specifies "textDims", the array will be overwritten with the pixel width
   * and height defining a tight bounding box around the text in the image,
   * starting from the upper-right corner. This is used when rendering for a
   * texture on graphics hardware that requires texture image dimensions to be
   * a power of two; textDims can be used to determine the texture coordinates
   * needed to cleanly fit the text on the target.
   * The origin of the image's extents is aligned with the anchor point
   * described by the text property's vertical and horizontal justification
   * options.
   */
  bool RenderString(svtkTextProperty* tprop, const svtkStdString& str, svtkImageData* data,
    int textDims[2], int dpi, int backend = Default)
  {
    return this->RenderStringInternal(tprop, str, data, textDims, dpi, backend);
  }
  bool RenderString(svtkTextProperty* tprop, const svtkUnicodeString& str, svtkImageData* data,
    int textDims[2], int dpi, int backend = Default)
  {
    return this->RenderStringInternal(tprop, str, data, textDims, dpi, backend);
  }
  //@}

  //@{
  /**
   * This function returns the font size (in points) and sets the size in @a
   * tprop that is required to fit the string in the target rectangle. The
   * computed font size will be set in @a tprop as well. If an error occurs,
   * this function will return -1.
   */
  int GetConstrainedFontSize(const svtkStdString& str, svtkTextProperty* tprop, int targetWidth,
    int targetHeight, int dpi, int backend = Default)
  {
    return this->GetConstrainedFontSizeInternal(
      str, tprop, targetWidth, targetHeight, dpi, backend);
  }
  int GetConstrainedFontSize(const svtkUnicodeString& str, svtkTextProperty* tprop, int targetWidth,
    int targetHeight, int dpi, int backend = Default)
  {
    return this->GetConstrainedFontSizeInternal(
      str, tprop, targetWidth, targetHeight, dpi, backend);
  }
  //@}

  //@{
  /**
   * Given a text property and a string, this function populates the svtkPath
   * path with the outline of the rendered string. The origin of the path
   * coordinates is aligned with the anchor point described by the text
   * property's horizontal and vertical justification options.
   * Return true on success, false otherwise.
   */
  bool StringToPath(
    svtkTextProperty* tprop, const svtkStdString& str, svtkPath* path, int dpi, int backend = Default)
  {
    return this->StringToPathInternal(tprop, str, path, dpi, backend);
  }
  bool StringToPath(svtkTextProperty* tprop, const svtkUnicodeString& str, svtkPath* path, int dpi,
    int backend = Default)
  {
    return this->StringToPathInternal(tprop, str, path, dpi, backend);
  }
  //@}

  /**
   * Set to true if the graphics implementation requires texture image dimensions
   * to be a power of two. Default is true, but this member will be set
   * appropriately by svtkOpenGLRenderWindow::OpenGLInitContext when GL is
   * inited.
   */
  void SetScaleToPowerOfTwo(bool scale) { this->SetScaleToPowerOfTwoInternal(scale); }

  friend class svtkTextRendererCleanup;

protected:
  svtkTextRenderer();
  ~svtkTextRenderer() override;

  //@{
  /**
   * Virtual methods for concrete implementations of the public methods.
   */
  virtual bool GetBoundingBoxInternal(
    svtkTextProperty* tprop, const svtkStdString& str, int bbox[4], int dpi, int backend) = 0;
  virtual bool GetBoundingBoxInternal(
    svtkTextProperty* tprop, const svtkUnicodeString& str, int bbox[4], int dpi, int backend) = 0;
  virtual bool GetMetricsInternal(
    svtkTextProperty* tprop, const svtkStdString& str, Metrics& metrics, int dpi, int backend) = 0;
  virtual bool GetMetricsInternal(svtkTextProperty* tprop, const svtkUnicodeString& str,
    Metrics& metrics, int dpi, int backend) = 0;
  virtual bool RenderStringInternal(svtkTextProperty* tprop, const svtkStdString& str,
    svtkImageData* data, int textDims[2], int dpi, int backend) = 0;
  virtual bool RenderStringInternal(svtkTextProperty* tprop, const svtkUnicodeString& str,
    svtkImageData* data, int textDims[2], int dpi, int backend) = 0;
  virtual int GetConstrainedFontSizeInternal(const svtkStdString& str, svtkTextProperty* tprop,
    int targetWidth, int targetHeight, int dpi, int backend) = 0;
  virtual int GetConstrainedFontSizeInternal(const svtkUnicodeString& str, svtkTextProperty* tprop,
    int targetWidth, int targetHeight, int dpi, int backend) = 0;
  virtual bool StringToPathInternal(
    svtkTextProperty* tprop, const svtkStdString& str, svtkPath* path, int dpi, int backend) = 0;
  virtual bool StringToPathInternal(
    svtkTextProperty* tprop, const svtkUnicodeString& str, svtkPath* path, int dpi, int backend) = 0;
  virtual void SetScaleToPowerOfTwoInternal(bool scale) = 0;
  //@}

  /**
   * Set the singleton instance. Call Delete() on the supplied
   * instance after setting it to fix the reference count.
   */
  static void SetInstance(svtkTextRenderer* instance);

  //@{
  /**
   * The singleton instance and the singleton cleanup instance.
   */
  static svtkTextRenderer* Instance;
  static svtkTextRendererCleanup Cleanup;
  //@}

  svtksys::RegularExpression* MathTextRegExp;
  svtksys::RegularExpression* MathTextRegExp2;

  //@{
  /**
   * Replace all instances of "\$" with "$".
   */
  virtual void CleanUpFreeTypeEscapes(svtkStdString& str);
  virtual void CleanUpFreeTypeEscapes(svtkUnicodeString& str);
  //@}

  /**
   * The backend to use when none is specified. Default: Detect
   */
  int DefaultBackend;

private:
  svtkTextRenderer(const svtkTextRenderer&) = delete;
  void operator=(const svtkTextRenderer&) = delete;
};

#endif // svtkTextRenderer_h
