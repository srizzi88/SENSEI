/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpaquePass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpaquePass
 * @brief   Render the opaque geometry with property key
 * filtering.
 *
 * svtkOpaquePass renders the opaque geometry of all the props that have the
 * keys contained in svtkRenderState.
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 *
 * @sa
 * svtkRenderPass svtkDefaultPass
 */

#ifndef svtkOpaquePass_h
#define svtkOpaquePass_h

#include "svtkDefaultPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class SVTKRENDERINGOPENGL2_EXPORT svtkOpaquePass : public svtkDefaultPass
{
public:
  static svtkOpaquePass* New();
  svtkTypeMacro(svtkOpaquePass, svtkDefaultPass);
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
  svtkOpaquePass();

  /**
   * Destructor.
   */
  ~svtkOpaquePass() override;

private:
  svtkOpaquePass(const svtkOpaquePass&) = delete;
  void operator=(const svtkOpaquePass&) = delete;
};

#endif
