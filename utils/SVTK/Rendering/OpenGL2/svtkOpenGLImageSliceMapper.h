/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLImageSliceMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLImageSliceMapper
 * @brief   OpenGL mapper for image slice display
 *
 * svtkOpenGLImageSliceMapper is a concrete implementation of the abstract
 * class svtkImageSliceMapper that interfaces to the OpenGL library.
 * @par Thanks:
 * Thanks to David Gobbi at the Seaman Family MR Centre and Dept. of Clinical
 * Neurosciences, Foothills Medical Centre, Calgary, for providing this class.
 */

#ifndef svtkOpenGLImageSliceMapper_h
#define svtkOpenGLImageSliceMapper_h

#include "svtkImageSliceMapper.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkRenderWindow;
class svtkOpenGLRenderWindow;
class svtkActor;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLImageSliceMapper : public svtkImageSliceMapper
{
public:
  static svtkOpenGLImageSliceMapper* New();
  svtkTypeMacro(svtkOpenGLImageSliceMapper, svtkImageSliceMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Implement base class method.  Perform the render.
   */
  void Render(svtkRenderer* ren, svtkImageSlice* prop) override;

  /**
   * Release any graphics resources that are being consumed by this
   * mapper, the image texture in particular. Using the same texture
   * in multiple render windows is NOT currently supported.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

protected:
  svtkOpenGLImageSliceMapper();
  ~svtkOpenGLImageSliceMapper() override;

  /**
   * Recursive internal method, will call the non-recursive method
   * as many times as necessary if the texture must be broken up into
   * pieces that are small enough for the GPU to render
   */
  void RecursiveRenderTexturedPolygon(svtkRenderer* ren, svtkImageProperty* property,
    svtkImageData* image, int extent[6], bool recursive);

  /**
   * Non-recursive internal method, generate a single texture
   * and its corresponding geometry.
   */
  void RenderTexturedPolygon(svtkRenderer* ren, svtkImageProperty* property, svtkImageData* image,
    int extent[6], bool recursive);

  /**
   * Basic polygon rendering, if the textured parameter is set the tcoords
   * are included, otherwise they aren't.
   */
  void RenderPolygon(svtkActor* actor, svtkPoints* points, const int extent[6], svtkRenderer* ren);

  /**
   * Render the background, which means rendering everything within the
   * plane of the image except for the polygon that displays the image data.
   */
  void RenderBackground(svtkActor* actor, svtkPoints* points, const int extent[6], svtkRenderer* ren);

  /**
   * Given an extent that describes a slice (it must have unit thickness
   * in one of the three directions), return the dimension indices that
   * correspond to the texture "x" and "y", provide the x, y image size,
   * and provide the texture size (padded to a power of two if the hardware
   * requires).
   */
  void ComputeTextureSize(
    const int extent[6], int& xdim, int& ydim, int imageSize[2], int textureSize[2]) override;

  /**
   * Test whether a given texture size is supported.  This includes a
   * check of whether the texture will fit into texture memory.
   */
  bool TextureSizeOK(const int size[2], svtkRenderer* ren);

  svtkRenderWindow* RenderWindow; // RenderWindow used for previous render
  int TextureSize[2];
  int TextureBytesPerPixel;
  int LastOrientation;
  int LastSliceNumber;

  svtkActor* PolyDataActor;
  svtkActor* BackingPolyDataActor;
  svtkActor* BackgroundPolyDataActor;

  svtkTimeStamp LoadTime;

private:
  svtkOpenGLImageSliceMapper(const svtkOpenGLImageSliceMapper&) = delete;
  void operator=(const svtkOpenGLImageSliceMapper&) = delete;
};

#endif
