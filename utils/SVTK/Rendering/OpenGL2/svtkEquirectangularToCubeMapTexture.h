/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEquirectangularToCubeMapTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkEquirectangularToCubeMapTexture
 * @brief   compute a cubemap texture based on a standard equirectangular projection
 *
 * This special texture converts a 2D projected texture in equirectangular format to a 3D cubemap
 * using the GPU.
 * The generated texture can be used as input for a skybox or an environment map for PBR shading.
 *
 * @sa svtkSkybox svtkRenderer::SetEnvironmentCubeMap
 */

#ifndef svtkEquirectangularToCubeMapTexture_h
#define svtkEquirectangularToCubeMapTexture_h

#include "svtkOpenGLTexture.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLFramebufferObject;
class svtkOpenGLRenderWindow;
class svtkOpenGLTexture;
class svtkRenderWindow;

class SVTKRENDERINGOPENGL2_EXPORT svtkEquirectangularToCubeMapTexture : public svtkOpenGLTexture
{
public:
  static svtkEquirectangularToCubeMapTexture* New();
  svtkTypeMacro(svtkEquirectangularToCubeMapTexture, svtkOpenGLTexture);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the input equirectangular 2D texture.
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
   * Set/Get size of each face of the output cubemap texture.
   * Default is 512.
   */
  svtkGetMacro(CubeMapSize, unsigned int);
  svtkSetMacro(CubeMapSize, unsigned int);
  //@}

  /**
   * Release any graphics resources that are being consumed by this texture.
   * The parameter window could be used to determine which graphic
   * resources to release. Using the same texture object in multiple
   * render windows is NOT currently supported.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

protected:
  svtkEquirectangularToCubeMapTexture();
  ~svtkEquirectangularToCubeMapTexture() override;

  unsigned int CubeMapSize = 512;
  svtkOpenGLTexture* InputTexture = nullptr;

private:
  svtkEquirectangularToCubeMapTexture(const svtkEquirectangularToCubeMapTexture&) = delete;
  void operator=(const svtkEquirectangularToCubeMapTexture&) = delete;
};

#endif
