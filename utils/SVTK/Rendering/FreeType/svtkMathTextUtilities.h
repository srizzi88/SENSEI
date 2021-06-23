/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMathTextUtilities.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMathTextUtilities
 * @brief   Abstract interface to equation rendering.
 *
 * svtkMathTextUtilities defines an interface for equation rendering. Intended
 * for use with the python matplotlib.mathtext module (implemented in the
 * svtkMatplotlib module).
 */

#ifndef svtkMathTextUtilities_h
#define svtkMathTextUtilities_h

#include "svtkObject.h"
#include "svtkRenderingFreeTypeModule.h" // For export macro
#include "svtkTextRenderer.h"            // for metrics

class svtkImageData;
class svtkPath;
class svtkTextProperty;
class svtkTextActor;
class svtkViewport;

//----------------------------------------------------------------------------
// Singleton cleanup

class SVTKRENDERINGFREETYPE_EXPORT svtkMathTextUtilitiesCleanup
{
public:
  svtkMathTextUtilitiesCleanup();
  ~svtkMathTextUtilitiesCleanup();

private:
  svtkMathTextUtilitiesCleanup(const svtkMathTextUtilitiesCleanup& other) = delete;
  svtkMathTextUtilitiesCleanup& operator=(const svtkMathTextUtilitiesCleanup& rhs) = delete;
};

class SVTKRENDERINGFREETYPE_EXPORT svtkMathTextUtilities : public svtkObject
{
public:
  svtkTypeMacro(svtkMathTextUtilities, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Returns true if mathtext rendering is available.
   */
  virtual bool IsAvailable() { return false; } // Override in subclasses.

  /**
   * This is a singleton pattern New. There will be only ONE reference
   * to a svtkMathTextUtilities object per process.  Clients that
   * call this method must use Delete() on the object so that reference
   * counting will work. The single instance will be unreferenced when
   * the program exits. You should just use the static GetInstance() method
   * anyway to get the singleton.
   */
  static svtkMathTextUtilities* New();

  /**
   * Return the singleton instance with no reference counting.
   */
  static svtkMathTextUtilities* GetInstance();

  /**
   * Supply a user defined instance. Call Delete() on the supplied
   * instance after setting it to fix the reference count.
   */
  static void SetInstance(svtkMathTextUtilities* instance);

  /**
   * Determine the dimensions of the image that RenderString will produce for
   * a given str, tprop, and dpi
   */
  virtual bool GetBoundingBox(svtkTextProperty* tprop, const char* str, int dpi, int bbox[4]) = 0;

  /**
   * Return the metrics for the rendered str, tprop, and dpi.
   */
  virtual bool GetMetrics(
    svtkTextProperty* tprop, const char* str, int dpi, svtkTextRenderer::Metrics& metrics) = 0;

  /**
   * Render the given string @a str into the svtkImageData @a data with a
   * resolution of @a dpi. textDims, will be overwritten by the pixel width and
   * height of the rendered string. This is useful when ScaleToPowerOfTwo is
   * set to true, and the image dimensions may not match the dimensions of the
   * rendered text.
   */
  virtual bool RenderString(const char* str, svtkImageData* data, svtkTextProperty* tprop, int dpi,
    int textDims[2] = nullptr) = 0;

  /**
   * Parse the MathText expression in str and fill path with a contour of the
   * glyphs.
   */
  virtual bool StringToPath(const char* str, svtkPath* path, svtkTextProperty* tprop, int dpi) = 0;

  /**
   * This function returns the font size (in points) required to fit the string
   * in the target rectangle. The font size of tprop is updated to the computed
   * value as well. If an error occurs (e.g. an improperly formatted MathText
   * string), -1 is returned.
   */
  virtual int GetConstrainedFontSize(
    const char* str, svtkTextProperty* tprop, int targetWidth, int targetHeight, int dpi);

  //@{
  /**
   * Set to true if the graphics implementation requires texture image dimensions
   * to be a power of two. Default is true, but this member will be set
   * appropriately when GL is inited.
   */
  virtual bool GetScaleToPowerOfTwo() = 0;
  virtual void SetScaleToPowerOfTwo(bool scale) = 0;
  //@}

protected:
  svtkMathTextUtilities();
  ~svtkMathTextUtilities() override;

private:
  svtkMathTextUtilities(const svtkMathTextUtilities&) = delete;
  void operator=(const svtkMathTextUtilities&) = delete;

  //@{
  /**
   * The singleton instance and the singleton cleanup instance
   */
  static svtkMathTextUtilities* Instance;
  static svtkMathTextUtilitiesCleanup Cleanup;
  //@}
};

#endif
