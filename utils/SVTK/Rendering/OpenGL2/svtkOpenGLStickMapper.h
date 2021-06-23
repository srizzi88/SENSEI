/*=========================================================================

  Program:   Visualization Toolkit

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOpenGLStickMapper
 * @brief   use imposters to draw cylinders
 *
 * PolyDataMapper that uses imposters to draw cylinders/sticks
 * for ball/stick style molecular rendering. Supports picking.
 */

#ifndef svtkOpenGLStickMapper_h
#define svtkOpenGLStickMapper_h

#include "svtkOpenGLPolyDataMapper.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLStickMapper : public svtkOpenGLPolyDataMapper
{
public:
  static svtkOpenGLStickMapper* New();
  svtkTypeMacro(svtkOpenGLStickMapper, svtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Convenience method to set the array to scale with.
   */
  svtkSetStringMacro(ScaleArray);
  //@}

  //@{
  /**
   * Convenience method to set the array to orient with
   */
  svtkSetStringMacro(OrientationArray);
  //@}

  //@{
  /**
   * Convenience method to set the array to select with
   */
  svtkSetStringMacro(SelectionIdArray);
  //@}

protected:
  svtkOpenGLStickMapper();
  ~svtkOpenGLStickMapper() override;

  /**
   * Create the basic shaders before replacement
   */
  void GetShaderTemplate(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act) override;

  /**
   * Perform string replacements on the shader templates
   */
  void ReplaceShaderValues(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act) override;

  /**
   * Set the shader parameters related to the Camera
   */
  void SetCameraShaderParameters(svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act) override;

  /**
   * Set the shader parameters related to the actor/mapper
   */
  void SetMapperShaderParameters(svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act) override;

  const char* ScaleArray;
  const char* OrientationArray;
  const char* SelectionIdArray;

  /**
   * Does the VBO/IBO need to be rebuilt
   */
  bool GetNeedToRebuildBufferObjects(svtkRenderer* ren, svtkActor* act) override;

  /**
   * Update the VBO to contain point based values
   */
  void BuildBufferObjects(svtkRenderer* ren, svtkActor* act) override;

  void RenderPieceDraw(svtkRenderer* ren, svtkActor* act) override;

private:
  svtkOpenGLStickMapper(const svtkOpenGLStickMapper&) = delete;
  void operator=(const svtkOpenGLStickMapper&) = delete;
};

#endif
