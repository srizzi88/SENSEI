/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLTexture
 * @brief   OpenGL texture map
 *
 * svtkOpenGLTexture is a concrete implementation of the abstract class
 * svtkTexture. svtkOpenGLTexture interfaces to the OpenGL rendering library.
 */

#ifndef svtkOpenGLTexture_h
#define svtkOpenGLTexture_h

#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkTexture.h"
#include "svtkWeakPointer.h" // needed for svtkWeakPointer.

class svtkRenderWindow;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLTexture : public svtkTexture
{
public:
  static svtkOpenGLTexture* New();
  svtkTypeMacro(svtkOpenGLTexture, svtkTexture);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Renders a texture map. It first checks the object's modified time
   * to make sure the texture maps Input is valid, then it invokes the
   * Load() method.
   */
  void Render(svtkRenderer* ren) override;

  /**
   * Implement base class method.
   */
  void Load(svtkRenderer*) override;

  // Descsription:
  // Clean up after the rendering is complete.
  void PostRender(svtkRenderer*) override;

  /**
   * Release any graphics resources that are being consumed by this texture.
   * The parameter window could be used to determine which graphic
   * resources to release. Using the same texture object in multiple
   * render windows is NOT currently supported.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  /**
   * copy the renderers read buffer into this texture
   */
  void CopyTexImage(int x, int y, int width, int height);

  //@{
  /**
   * Provide for specifying a format for the texture
   */
  svtkGetMacro(IsDepthTexture, int);
  svtkSetMacro(IsDepthTexture, int);
  //@}

  //@{
  /**
   * What type of texture map GL_TEXTURE_2D versus GL_TEXTURE_RECTANGLE
   */
  svtkGetMacro(TextureType, int);
  svtkSetMacro(TextureType, int);
  //@}

  svtkGetObjectMacro(TextureObject, svtkTextureObject);
  void SetTextureObject(svtkTextureObject*);

  /**
   * Return the texture unit used for this texture
   */
  int GetTextureUnit() override;

  /**
   * Is this Texture Translucent?
   * returns false (0) if the texture is either fully opaque or has
   * only fully transparent pixels and fully opaque pixels and the
   * Interpolate flag is turn off.
   */
  int IsTranslucent() override;

protected:
  svtkOpenGLTexture();
  ~svtkOpenGLTexture() override;

  svtkTimeStamp LoadTime;
  svtkWeakPointer<svtkRenderWindow> RenderWindow; // RenderWindow used for previous render

  bool ExternalTextureObject;
  svtkTextureObject* TextureObject;

  int IsDepthTexture;
  int TextureType;
  int PrevBlendParams[4];

  // used when the texture exceeds the GL limit
  unsigned char* ResampleToPowerOfTwo(
    int& xsize, int& ysize, unsigned char* dptr, int bpp, int maxTexSize);

private:
  svtkOpenGLTexture(const svtkOpenGLTexture&) = delete;
  void operator=(const svtkOpenGLTexture&) = delete;
};

#endif
