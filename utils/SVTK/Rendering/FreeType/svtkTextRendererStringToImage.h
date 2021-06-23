/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextRendererStringToImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkTextRendererStringToImage
 * @brief   uses svtkTextRenderer to render the
 * supplied text to an image.
 */

#ifndef svtkTextRendererStringToImage_h
#define svtkTextRendererStringToImage_h

#include "svtkRenderingFreeTypeModule.h" // For export macro
#include "svtkStringToImage.h"

class SVTKRENDERINGFREETYPE_EXPORT svtkTextRendererStringToImage : public svtkStringToImage
{
public:
  svtkTypeMacro(svtkTextRendererStringToImage, svtkStringToImage);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkTextRendererStringToImage* New();

  //@{
  /**
   * Given a text property and a string, get the bounding box [xmin, xmax] x
   * [ymin, ymax]. Note that this is the bounding box of the area
   * where actual pixels will be written, given a text/pen/baseline location
   * of (0,0).
   * For example, if the string starts with a 'space', or depending on the
   * orientation, you can end up with a [-20, -10] x [5, 10] bbox (the math
   * to get the real bbox is straightforward).
   * Return 1 on success, 0 otherwise.
   * You can use IsBoundingBoxValid() to test if the computed bbox
   * is valid (it may not if GetBoundingBox() failed or if the string
   * was empty).
   */
  svtkVector2i GetBounds(
    svtkTextProperty* property, const svtkUnicodeString& string, int dpi) override;
  svtkVector2i GetBounds(svtkTextProperty* property, const svtkStdString& string, int dpi) override;
  //@}

  //@{
  /**
   * Given a text property and a string, this function initializes the
   * svtkImageData *data and renders it in a svtkImageData. textDims, if provided,
   * will be overwritten by the pixel width and height of the rendered string.
   * This is useful when ScaleToPowerOfTwo is true, and the image dimensions may
   * not match the dimensions of the rendered text.
   */
  int RenderString(svtkTextProperty* property, const svtkUnicodeString& string, int dpi,
    svtkImageData* data, int textDims[2] = nullptr) override;
  int RenderString(svtkTextProperty* property, const svtkStdString& string, int dpi,
    svtkImageData* data, int textDims[2] = nullptr) override;
  //@}

  /**
   * Should we produce images at powers of 2, makes rendering on old OpenGL
   * hardware easier. Default is false.
   */
  void SetScaleToPowerOfTwo(bool scale) override;

  /**
   * Make a deep copy of the supplied utility class.
   */
  void DeepCopy(svtkTextRendererStringToImage* utility);

protected:
  svtkTextRendererStringToImage();
  ~svtkTextRendererStringToImage() override;

  class Internals;
  Internals* Implementation;

private:
  svtkTextRendererStringToImage(const svtkTextRendererStringToImage&) = delete;
  void operator=(const svtkTextRendererStringToImage&) = delete;
};

#endif // svtkTextRendererStringToImage_h
