/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStereoCompositor.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkStereoCompositor
 * @brief helper class to generate composited stereo images.
 *
 * svtkStereoCompositor is used by svtkRenderWindow to composite left and right
 * eye rendering results into a single color buffer.
 *
 * Note that all methods on svtkStereoCompositor take in pointers to the left and
 * right rendering results and generate the result in the buffer passed for the
 * left eye.
 */

#ifndef svtkStereoCompositor_h
#define svtkStereoCompositor_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkUnsignedCharArray;

class SVTKRENDERINGCORE_EXPORT svtkStereoCompositor : public svtkObject
{
public:
  static svtkStereoCompositor* New();
  svtkTypeMacro(svtkStereoCompositor, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Methods for compositing left and right eye images based on various
   * supported modes. See svtkRenderWindow::SetStereoType for explanation of each
   * of these modes. Note that all these methods generate the result in the
   * buffer passed for the left eye.
   */
  bool RedBlue(svtkUnsignedCharArray* rgbLeftNResult, svtkUnsignedCharArray* rgbRight);

  bool Anaglyph(svtkUnsignedCharArray* rgbLeftNResult, svtkUnsignedCharArray* rgbRight,
    float colorSaturation, const int colorMask[2]);

  bool Interlaced(
    svtkUnsignedCharArray* rgbLeftNResult, svtkUnsignedCharArray* rgbRight, const int size[2]);

  bool Dresden(
    svtkUnsignedCharArray* rgbLeftNResult, svtkUnsignedCharArray* rgbRight, const int size[2]);

  bool Checkerboard(
    svtkUnsignedCharArray* rgbLeftNResult, svtkUnsignedCharArray* rgbRight, const int size[2]);

  bool SplitViewportHorizontal(
    svtkUnsignedCharArray* rgbLeftNResult, svtkUnsignedCharArray* rgbRight, const int size[2]);
  //@}

protected:
  svtkStereoCompositor();
  ~svtkStereoCompositor() override;

  bool Validate(
    svtkUnsignedCharArray* rgbLeft, svtkUnsignedCharArray* rgbRight, const int* size = nullptr);

private:
  svtkStereoCompositor(const svtkStereoCompositor&) = delete;
  void operator=(const svtkStereoCompositor&) = delete;
};

#endif
