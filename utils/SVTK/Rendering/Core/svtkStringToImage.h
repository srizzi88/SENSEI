/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStringToImage.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkStringToImage
 * @brief   base class for classes that render supplied text
 * to an image.
 *
 *
 *
 */

#ifndef svtkStringToImage_h
#define svtkStringToImage_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkStdString;
class svtkUnicodeString;
class svtkTextProperty;
class svtkImageData;
class svtkVector2i;

class SVTKRENDERINGCORE_EXPORT svtkStringToImage : public svtkObject
{
public:
  svtkTypeMacro(svtkStringToImage, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

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
  virtual svtkVector2i GetBounds(
    svtkTextProperty* property, const svtkUnicodeString& string, int dpi) = 0;
  virtual svtkVector2i GetBounds(svtkTextProperty* property, const svtkStdString& string, int dpi) = 0;
  //@}

  //@{
  /**
   * Given a text property and a string, this function initializes the
   * svtkImageData *data and renders it in a svtkImageData. textDims, if provided,
   * will be overwritten by the pixel width and height of the rendered string.
   * This is useful when ScaleToPowerOfTwo is true, and the image dimensions may
   * not match the dimensions of the rendered text.
   */
  virtual int RenderString(svtkTextProperty* property, const svtkUnicodeString& string, int dpi,
    svtkImageData* data, int textDims[2] = nullptr) = 0;
  virtual int RenderString(svtkTextProperty* property, const svtkStdString& string, int dpi,
    svtkImageData* data, int text_dims[2] = nullptr) = 0;
  //@}

  /**
   * Should we produce images at powers of 2, makes rendering on old OpenGL
   * hardware easier. Default is false.
   */
  virtual void SetScaleToPowerOfTwo(bool scale);
  svtkGetMacro(ScaleToPowerOfTwo, bool);

protected:
  svtkStringToImage();
  ~svtkStringToImage() override;

  bool Antialias;
  bool ScaleToPowerOfTwo;

private:
  svtkStringToImage(const svtkStringToImage&) = delete;
  void operator=(const svtkStringToImage&) = delete;
};

#endif // svtkStringToImage_h
