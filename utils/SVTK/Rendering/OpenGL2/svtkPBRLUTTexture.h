/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPBRLUTTexture.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPBRLUTTexture
 * @brief   precompute BRDF look-up table texture used in physically based rendering
 *
 * This texture is a 2D texture which precompute Fresnel response scale (red) and bias (green)
 * based on roughness (x) and angle between light and normal (y).
 */

#ifndef svtkPBRLUTTexture_h
#define svtkPBRLUTTexture_h

#include "svtkOpenGLTexture.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkSmartPointer.h"           // For svtkSmartPointer

class svtkOpenGLFramebufferObject;
class svtkOpenGLRenderWindow;
class svtkOpenGLTexture;
class svtkRenderWindow;

class SVTKRENDERINGOPENGL2_EXPORT svtkPBRLUTTexture : public svtkOpenGLTexture
{
public:
  static svtkPBRLUTTexture* New();
  svtkTypeMacro(svtkPBRLUTTexture, svtkOpenGLTexture);
  void PrintSelf(ostream& os, svtkIndent indent) override;

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
   * Default is 1024.
   */
  svtkGetMacro(LUTSize, unsigned int);
  svtkSetMacro(LUTSize, unsigned int);
  //@}

  //@{
  /**
   * Set/Get the number of samples used during Monte-Carlo integration.
   * Default is 512.
   */
  svtkGetMacro(LUTSamples, unsigned int);
  svtkSetMacro(LUTSamples, unsigned int);
  //@}

protected:
  svtkPBRLUTTexture() = default;
  ~svtkPBRLUTTexture() override = default;

  unsigned int LUTSize = 512;
  unsigned int LUTSamples = 1024;

private:
  svtkPBRLUTTexture(const svtkPBRLUTTexture&) = delete;
  void operator=(const svtkPBRLUTTexture&) = delete;
};

#endif
