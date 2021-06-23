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
 * @class   svtkOpenGLSphereMapper
 * @brief   draw spheres using imposters
 *
 * An OpenGL mapper that uses imposters to draw spheres. Supports
 * transparency and picking as well.
 */

#ifndef svtkOpenGLSphereMapper_h
#define svtkOpenGLSphereMapper_h

#include "svtkOpenGLPolyDataMapper.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLSphereMapper : public svtkOpenGLPolyDataMapper
{
public:
  static svtkOpenGLSphereMapper* New();
  svtkTypeMacro(svtkOpenGLSphereMapper, svtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Convenience method to set the array to scale with.
   */
  svtkSetStringMacro(ScaleArray);
  //@}

  //@{
  /**
   * This value will be used for the radius is the scale
   * array is not provided.
   */
  svtkSetMacro(Radius, float);
  svtkGetMacro(Radius, float);

  /**
   * This calls RenderPiece (twice when transparent)
   */
  void Render(svtkRenderer* ren, svtkActor* act) override;

  /**
   * allows a mapper to update a selections color buffers
   * Called from a prop which in turn is called from the selector
   */
  // void ProcessSelectorPixelBuffers(svtkHardwareSelector *sel,
  //   int propid, svtkProp *prop) override;

protected:
  svtkOpenGLSphereMapper();
  ~svtkOpenGLSphereMapper() override;

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

  /**
   * Update the VBO to contain point based values
   */
  void BuildBufferObjects(svtkRenderer* ren, svtkActor* act) override;

  void RenderPieceDraw(svtkRenderer* ren, svtkActor* act) override;

  virtual void CreateVBO(svtkPolyData* poly, svtkIdType numPts, unsigned char* colors,
    int colorComponents, svtkIdType nc, float* sizes, svtkIdType ns, svtkRenderer* ren);

  // used for transparency
  bool Invert;
  float Radius;

private:
  svtkOpenGLSphereMapper(const svtkOpenGLSphereMapper&) = delete;
  void operator=(const svtkOpenGLSphereMapper&) = delete;
};

#endif
