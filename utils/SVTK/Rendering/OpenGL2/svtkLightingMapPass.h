/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLightingMapPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLightingMapPass
 * @brief   TO DO
 *
 * Renders lighting information directly instead of final shaded colors.
 * The information keys allow the selection of either normal rendering or
 * luminance. For normals, the (nx, ny, nz) tuple are rendered directly into
 * the (r,g,b) fragment. For luminance, the diffuse and specular intensities are
 * rendered into the red and green channels, respectively. The blue channel is
 * zero. For both luminances and normals, the alpha channel is set to 1.0 if
 * present.
 *
 * @sa
 * svtkRenderPass svtkDefaultPass
 */

#ifndef svtkLightingMapPass_h
#define svtkLightingMapPass_h

#include "svtkDefaultPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkInformationIntegerKey;

class SVTKRENDERINGOPENGL2_EXPORT svtkLightingMapPass : public svtkDefaultPass
{
public:
  static svtkLightingMapPass* New();
  svtkTypeMacro(svtkLightingMapPass, svtkDefaultPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the type of lighting render to perform
   */
  enum RenderMode
  {
    LUMINANCE,
    NORMALS
  };
  svtkSetMacro(RenderType, RenderMode);
  svtkGetMacro(RenderType, RenderMode);
  //@}

  /**
   * If this key exists on the PropertyKeys of a prop, the active scalar array
   * on the prop will be rendered as its color. This key is mutually exclusive
   * with the RENDER_LUMINANCE key.
   */
  static svtkInformationIntegerKey* RENDER_LUMINANCE();

  /**
   * if this key exists on the ProperyKeys of a prop, the active vector array on
   * the prop will be rendered as its color. This key is mutually exclusive with
   * the RENDER_LUMINANCE key.
   */
  static svtkInformationIntegerKey* RENDER_NORMALS();

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const svtkRenderState* s) override;

protected:
  /**
   * Default constructor.
   */
  svtkLightingMapPass();

  /**
   * Destructor.
   */
  ~svtkLightingMapPass() override;

  /**
   * Opaque pass with key checking.
   * \pre s_exists: s!=0
   */
  void RenderOpaqueGeometry(const svtkRenderState* s) override;

private:
  svtkLightingMapPass(const svtkLightingMapPass&) = delete;
  void operator=(const svtkLightingMapPass&) = delete;

  RenderMode RenderType;
};

#endif
