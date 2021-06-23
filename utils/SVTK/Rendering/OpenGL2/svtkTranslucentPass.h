/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTranslucentPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTranslucentPass
 * @brief   Render the translucent polygonal geometry
 * with property key filtering.
 *
 * svtkTranslucentPass renders the translucent polygonal geometry of all the
 * props that have the keys contained in svtkRenderState.
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 *
 * @sa
 * svtkRenderPass svtkDefaultPass
 */

#ifndef svtkTranslucentPass_h
#define svtkTranslucentPass_h

#include "svtkDefaultPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class SVTKRENDERINGOPENGL2_EXPORT svtkTranslucentPass : public svtkDefaultPass
{
public:
  static svtkTranslucentPass* New();
  svtkTypeMacro(svtkTranslucentPass, svtkDefaultPass);
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
  svtkTranslucentPass();

  /**
   * Destructor.
   */
  ~svtkTranslucentPass() override;

private:
  svtkTranslucentPass(const svtkTranslucentPass&) = delete;
  void operator=(const svtkTranslucentPass&) = delete;
};

#endif
