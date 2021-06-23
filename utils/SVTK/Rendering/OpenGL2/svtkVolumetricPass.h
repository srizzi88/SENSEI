/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumetricPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVolumetricPass
 * @brief   Render the volumetric geometry with property key
 * filtering.
 *
 * svtkVolumetricPass renders the volumetric geometry of all the props that
 * have the keys contained in svtkRenderState.
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 *
 * @sa
 * svtkRenderPass svtkDefaultPass
 */

#ifndef svtkVolumetricPass_h
#define svtkVolumetricPass_h

#include "svtkDefaultPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class SVTKRENDERINGOPENGL2_EXPORT svtkVolumetricPass : public svtkDefaultPass
{
public:
  static svtkVolumetricPass* New();
  svtkTypeMacro(svtkVolumetricPass, svtkDefaultPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const svtkRenderState* s) override;

protected:
  /**
   * Default constructor.
   */
  svtkVolumetricPass();

  /**
   * Destructor.
   */
  ~svtkVolumetricPass() override;

private:
  svtkVolumetricPass(const svtkVolumetricPass&) = delete;
  void operator=(const svtkVolumetricPass&) = delete;
};

#endif
