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
 * @class   svtkOpenGLGlyph3DHelper
 * @brief   PolyDataMapper using OpenGL to render.
 *
 * PolyDataMapper that uses a OpenGL to do the actual rendering.
 */

#ifndef svtkOpenGLGlyph3DHelper_h
#define svtkOpenGLGlyph3DHelper_h

#include "svtkNew.h"                   // For svtkNew
#include "svtkOpenGLBufferObject.h"    // For svtkOpenGLBufferObject
#include "svtkOpenGLHelper.h"          // For svtkOpenGLHelper
#include "svtkOpenGLInstanceCulling.h" // For svtkOpenGLInstanceCulling
#include "svtkOpenGLPolyDataMapper.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkBitArray;

class SVTKRENDERINGOPENGL2_EXPORT svtkOpenGLGlyph3DHelper : public svtkOpenGLPolyDataMapper
{
public:
  static svtkOpenGLGlyph3DHelper* New();
  svtkTypeMacro(svtkOpenGLGlyph3DHelper, svtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Fast path for rendering glyphs comprised of only one type of primitive
   * Must set this->CurrentInput explicitly before calling.
   */
  void GlyphRender(svtkRenderer* ren, svtkActor* actor, svtkIdType numPts,
    std::vector<unsigned char>& colors, std::vector<float>& matrices,
    std::vector<float>& normalMatrices, std::vector<svtkIdType>& pickIds, svtkMTimeType pointMTime,
    bool culling);

  void SetLODs(std::vector<std::pair<float, float> >& lods);

  void SetLODColoring(bool val);

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow* window) override;

protected:
  svtkOpenGLGlyph3DHelper();
  ~svtkOpenGLGlyph3DHelper() override = default;

  // special opengl 32 version that uses instances
  void GlyphRenderInstances(svtkRenderer* ren, svtkActor* actor, svtkIdType numPts,
    std::vector<unsigned char>& colors, std::vector<float>& matrices,
    std::vector<float>& normalMatrices, svtkMTimeType pointMTime, bool culling);

  /**
   * Create the basic shaders before replacement
   */
  void GetShaderTemplate(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act) override;

  //@{
  /**
   * Perform string replacements on the shader templates
   */
  void ReplaceShaderPicking(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act) override;
  void ReplaceShaderColor(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act) override;
  void ReplaceShaderNormal(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act) override;
  void ReplaceShaderClip(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act) override;
  void ReplaceShaderPositionVC(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act) override;
  //@}

  /**
   * Set the shader parameteres related to the actor/mapper
   */
  void SetMapperShaderParameters(svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act) override;

  void BuildCullingShaders(svtkRenderer* ren, svtkActor* actor, svtkIdType numPts, bool withNormals);

  bool UsingInstancing;

  svtkNew<svtkOpenGLBufferObject> NormalMatrixBuffer;
  svtkNew<svtkOpenGLBufferObject> MatrixBuffer;
  svtkNew<svtkOpenGLBufferObject> ColorBuffer;
  svtkTimeStamp InstanceBuffersBuildTime;
  svtkTimeStamp InstanceBuffersLoadTime;

  std::vector<std::pair<float, float> > LODs;
  svtkNew<svtkOpenGLInstanceCulling> InstanceCulling;

private:
  svtkOpenGLGlyph3DHelper(const svtkOpenGLGlyph3DHelper&) = delete;
  void operator=(const svtkOpenGLGlyph3DHelper&) = delete;
};

#endif
