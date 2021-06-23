/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOverlayPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOverlayPass
 * @brief   Render the overlay geometry with property key
 * filtering.
 *
 * svtkOverlayPass renders the overlay geometry of all the props that have the
 * keys contained in svtkRenderState.
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 *
 * @sa
 * svtkRenderPass svtkDefaultPass
 */

#ifndef svtkOverlayPass_h
#define svtkOverlayPass_h

#include "svtkDefaultPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class SVTKRENDERINGOPENGL2_EXPORT svtkOverlayPass : public svtkDefaultPass
{
public:
  static svtkOverlayPass* New();
  svtkTypeMacro(svtkOverlayPass, svtkDefaultPass);
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
  svtkOverlayPass();

  /**
   * Destructor.
   */
  ~svtkOverlayPass() override;

private:
  svtkOverlayPass(const svtkOverlayPass&) = delete;
  void operator=(const svtkOverlayPass&) = delete;
};

#endif
