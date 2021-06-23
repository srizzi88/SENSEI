/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPBRIrradianceTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPBRIrradianceTexture
 * @brief   precompute irradiance texture used in physically based rendering
 *
 * Irradiance texture is a cubemap which average light of a hemisphere of the input texture.
 * The input texture can be a cubemap or an equirectangular projection.
 * It is used in Image Base Lighting to compute the diffuse part.
 */

#ifndef svtkPBRIrradianceTexture_h
#define svtkPBRIrradianceTexture_h

#include "svtkOpenGLTexture.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLFramebufferObject;
class svtkOpenGLRenderWindow;
class svtkOpenGLTexture;
class svtkRenderWindow;

class SVTKRENDERINGOPENGL2_EXPORT svtkPBRIrradianceTexture : public svtkOpenGLTexture
{
public:
  static svtkPBRIrradianceTexture* New();
  svtkTypeMacro(svtkPBRIrradianceTexture, svtkOpenGLTexture);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the input texture.
   */
  void SetInputTexture(svtkOpenGLTexture* texture);
  svtkGetObjectMacro(InputTexture, svtkOpenGLTexture);
  //@}

  /**
   * Implement base class method.
   */
  void Load(svtkRenderer*) override;

  /**
   * Implement base class method.
   */
  void Render(svtkRenderer* ren) override { this->Load(ren); }

  //@{
  /**
   * Set/Get size of texture.
   * Default is 256.
   */
  svtkGetMacro(IrradianceSize, unsigned int);
  svtkSetMacro(IrradianceSize, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the size of steps in radians used to sample hemisphere.
   * Default is (pi/64).
   * In some OpenGL drivers (OSMesa, old OSX), the default value might be too low leading to
   * artifacts.
   */
  svtkGetMacro(IrradianceStep, float);
  svtkSetMacro(IrradianceStep, float);
  //@}

  //@{
  /**
   * Set/Get the conversion to linear color space.
   * If the input texture is in sRGB color space and the conversion is not done by OpenGL
   * directly with the texture format, the conversion can be done in the shader with this flag.
   */
  svtkGetMacro(ConvertToLinear, bool);
  svtkSetMacro(ConvertToLinear, bool);
  svtkBooleanMacro(ConvertToLinear, bool);
  //@}

  /**
   * Release any graphics resources that are being consumed by this texture.
   * The parameter window could be used to determine which graphic
   * resources to release. Using the same texture object in multiple
   * render windows is NOT currently supported.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

protected:
  svtkPBRIrradianceTexture() = default;
  ~svtkPBRIrradianceTexture() override;

  float IrradianceStep = 0.04908738521; // pi / 64
  unsigned int IrradianceSize = 256;
  svtkOpenGLTexture* InputTexture = nullptr;
  bool ConvertToLinear = false;

private:
  svtkPBRIrradianceTexture(const svtkPBRIrradianceTexture&) = delete;
  void operator=(const svtkPBRIrradianceTexture&) = delete;
};

#endif
