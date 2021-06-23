/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClearRGBPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkClearRGBPass
 * @brief   Paint in the color buffer.
 *
 * Clear the color buffer to the specified color.
 *
 * @sa
 * svtkValuePasses
 */

#ifndef svtkClearRGBPass_h
#define svtkClearRGBPass_h

#include "svtkRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLRenderWindow;

class SVTKRENDERINGOPENGL2_EXPORT svtkClearRGBPass : public svtkRenderPass
{
public:
  static svtkClearRGBPass* New();
  svtkTypeMacro(svtkClearRGBPass, svtkRenderPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state s.
   */
  void Render(const svtkRenderState* s) override;

  //@{
  /**
   * Set/Get the background color of the rendering screen using an rgb color
   * specification.
   */
  svtkSetVector3Macro(Background, double);
  svtkGetVector3Macro(Background, double);
  //@}

protected:
  /**
   * Default constructor.
   */
  svtkClearRGBPass();

  /**
   * Destructor.
   */
  ~svtkClearRGBPass() override;

  double Background[3];

private:
  svtkClearRGBPass(const svtkClearRGBPass&) = delete;
  void operator=(const svtkClearRGBPass&) = delete;
};

#endif
