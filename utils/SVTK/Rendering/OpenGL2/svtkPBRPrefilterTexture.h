/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPBRPrefilterTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPBRPrefilterTexture
 * @brief   precompute prefilter texture used in physically based rendering
 *
 * Prefilter texture is a cubemap result of integration of the input cubemap contribution in
 * BRDF equation. The result depends on roughness coefficient so several levels of mipmap are
 * used to store results of different roughness coefficients.
 * It is used in Image Base Lighting to compute the specular part.
 */

#ifndef svtkPBRPrefilterTexture_h
#define svtkPBRPrefilterTexture_h

#include "svtkOpenGLTexture.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLFramebufferObject;
class svtkOpenGLRenderWindow;
class svtkOpenGLTexture;
class svtkRenderWindow;

class SVTKRENDERINGOPENGL2_EXPORT svtkPBRPrefilterTexture : public svtkOpenGLTexture
{
public:
  static svtkPBRPrefilterTexture* New();
  svtkTypeMacro(svtkPBRPrefilterTexture, svtkOpenGLTexture);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the input texture.
   */
  void SetInputTexture(svtkOpenGLTexture*);
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
   * Default is 128.
   * This value should be increased if glossy materials are present
   * in order to have better reflections.
   */
  svtkGetMacro(PrefilterSize, unsigned int);
  svtkSetMacro(PrefilterSize, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the number of samples used during Monte-Carlo integration.
   * Default is 1024.
   * In some OpenGL drivers (OSMesa, old OSX), the default value might be too high leading to
   * artifacts.
   */
  svtkGetMacro(PrefilterSamples, unsigned int);
  svtkSetMacro(PrefilterSamples, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the number of mip-map levels.
   * Default is 5.
   */
  svtkGetMacro(PrefilterLevels, unsigned int);
  svtkSetMacro(PrefilterLevels, unsigned int);
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
  svtkPBRPrefilterTexture() = default;
  ~svtkPBRPrefilterTexture() override;

  unsigned int PrefilterSize = 128;
  unsigned int PrefilterLevels = 5;
  unsigned int PrefilterSamples = 1024;
  svtkOpenGLTexture* InputTexture = nullptr;
  bool ConvertToLinear = false;

private:
  svtkPBRPrefilterTexture(const svtkPBRPrefilterTexture&) = delete;
  void operator=(const svtkPBRPrefilterTexture&) = delete;
};

#endif
