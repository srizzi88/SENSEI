/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMathTextFreeTypeTextRenderer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkMathTextFreeTypeTextRenderer
 * @brief   Default implementation of
 * svtkTextRenderer.
 *
 *
 * Default implementation of svtkTextRenderer using svtkFreeTypeTools and
 * svtkMathTextUtilities.
 *
 * @warning
 * The MathText backend does not currently support UTF16 strings, thus
 * UTF16 strings passed to the MathText renderer will be converted to
 * UTF8.
 */

#ifndef svtkMathTextFreeTypeTextRenderer_h
#define svtkMathTextFreeTypeTextRenderer_h

#include "svtkRenderingFreeTypeModule.h" // For export macro
#include "svtkTextRenderer.h"

class svtkFreeTypeTools;
class svtkMathTextUtilities;

class SVTKRENDERINGFREETYPE_EXPORT svtkMathTextFreeTypeTextRenderer : public svtkTextRenderer
{
public:
  svtkTypeMacro(svtkMathTextFreeTypeTextRenderer, svtkTextRenderer);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkMathTextFreeTypeTextRenderer* New();

  //@{
  /**
   * Test for availability of various backends
   */
  bool FreeTypeIsSupported() override;
  bool MathTextIsSupported() override;
  //@}

protected:
  svtkMathTextFreeTypeTextRenderer();
  ~svtkMathTextFreeTypeTextRenderer() override;

  //@{
  /**
   * Reimplemented from svtkTextRenderer.
   */
  bool GetBoundingBoxInternal(
    svtkTextProperty* tprop, const svtkStdString& str, int bbox[4], int dpi, int backend) override;
  bool GetBoundingBoxInternal(svtkTextProperty* tprop, const svtkUnicodeString& str, int bbox[4],
    int dpi, int backend) override;
  bool GetMetricsInternal(svtkTextProperty* tprop, const svtkStdString& str, Metrics& metrics,
    int dpi, int backend) override;
  bool GetMetricsInternal(svtkTextProperty* tprop, const svtkUnicodeString& str, Metrics& metrics,
    int dpi, int backend) override;
  bool RenderStringInternal(svtkTextProperty* tprop, const svtkStdString& str, svtkImageData* data,
    int textDims[2], int dpi, int backend) override;
  bool RenderStringInternal(svtkTextProperty* tprop, const svtkUnicodeString& str, svtkImageData* data,
    int textDims[2], int dpi, int backend) override;
  int GetConstrainedFontSizeInternal(const svtkStdString& str, svtkTextProperty* tprop,
    int targetWidth, int targetHeight, int dpi, int backend) override;
  int GetConstrainedFontSizeInternal(const svtkUnicodeString& str, svtkTextProperty* tprop,
    int targetWidth, int targetHeight, int dpi, int backend) override;
  bool StringToPathInternal(
    svtkTextProperty* tprop, const svtkStdString& str, svtkPath* path, int dpi, int backend) override;
  bool StringToPathInternal(svtkTextProperty* tprop, const svtkUnicodeString& str, svtkPath* path,
    int dpi, int backend) override;
  void SetScaleToPowerOfTwoInternal(bool scale) override;
  //@}

private:
  svtkMathTextFreeTypeTextRenderer(const svtkMathTextFreeTypeTextRenderer&) = delete;
  void operator=(const svtkMathTextFreeTypeTextRenderer&) = delete;

  svtkFreeTypeTools* FreeTypeTools;
  svtkMathTextUtilities* MathTextUtilities;
};

#endif // svtkMathTextFreeTypeTextRenderer_h
