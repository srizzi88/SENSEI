/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClearZPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkClearZPass
 * @brief   Clear the depth buffer with a given value.
 *
 * Clear the depth buffer with a given value.
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkClearZPass_h
#define svtkClearZPass_h

#include "svtkRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLRenderWindow;

class SVTKRENDERINGOPENGL2_EXPORT svtkClearZPass : public svtkRenderPass
{
public:
  static svtkClearZPass* New();
  svtkTypeMacro(svtkClearZPass, svtkRenderPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const svtkRenderState* s) override;

  //@{
  /**
   * Set/Get the depth value. Initial value is 1.0 (farest).
   */
  svtkSetClampMacro(Depth, double, 0.0, 1.0);
  svtkGetMacro(Depth, double);
  //@}

protected:
  /**
   * Default constructor.
   */
  svtkClearZPass();

  /**
   * Destructor.
   */
  ~svtkClearZPass() override;

  double Depth;

private:
  svtkClearZPass(const svtkClearZPass&) = delete;
  void operator=(const svtkClearZPass&) = delete;
};

#endif
