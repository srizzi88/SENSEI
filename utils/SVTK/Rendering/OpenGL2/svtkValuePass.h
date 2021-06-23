/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkValuePass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkValuePass
 *
 * Renders geometry using the values of a field array as fragment colors. The
 * output can be used for deferred color mapping. It supports using arrays of
 * either point or cell data. The target array can be selected by setting an
 * array name/id and a component number. Only opaque geometry is supported.
 *
 * There are two rendering modes available:
 *
 * * INVERTIBLE_LUT  Encodes array values as RGB data and renders the result to
 * the default framebuffer.  It uses a texture as a color LUT to map the values
 * to RGB data. Texture size constraints limit its precision (currently 12-bit).
 * The implementation of this mode is in svtkInternalsInvertible. This option
 * is deprecated now that the SGI patent on floating point textures has
 * expired and Mesa and other OpenGL's always supports it.
 *
 * * FLOATING_POINT  Renders actual array values as floating point data to an
 * internal RGBA32F framebuffer.  This class binds and unbinds the framebuffer
 * on each render pass. Resources are allocated on demand. When rendering point
 * data values are uploaded to the GPU as vertex attributes. When rendering cell
 * data values are uploaded as a texture buffer. Custom vertex and fragment
 * shaders are defined in order to adjust its behavior for either type of data.
 *
 * @sa
 * svtkRenderPass svtkOpenGLRenderPass
 */
#ifndef svtkValuePass_h
#define svtkValuePass_h

#include "svtkOpenGLRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include "svtkSmartPointer.h"           //for ivar

class svtkAbstractArray;
class svtkActor;
class svtkDataArray;
class svtkDataObject;
class svtkFloatArray;
class svtkMapper;
class svtkOpenGLVertexArrayObject;
class svtkProperty;
class svtkRenderer;
class svtkRenderWindow;
class svtkShaderProgram;

class SVTKRENDERINGOPENGL2_EXPORT svtkValuePass : public svtkOpenGLRenderPass
{
public:
  enum Mode
  {
    INVERTIBLE_LUT = 1,
    FLOATING_POINT = 2
  };

  static svtkValuePass* New();
  svtkTypeMacro(svtkValuePass, svtkOpenGLRenderPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // @deprecated As of 9.0, We are moving to only FLOATING_POINT.
  SVTK_LEGACY(svtkSetMacro(RenderingMode, int));
  SVTK_LEGACY(svtkGetMacro(RenderingMode, int));
  void SetInputArrayToProcess(int fieldAssociation, const char* name);
  void SetInputArrayToProcess(int fieldAssociation, int fieldId);
  void SetInputComponentToProcess(int component);
  // @deprecated As of 9.0, Not needed with FLOATING_POINT.
  SVTK_LEGACY(void SetScalarRange(double min, double max));

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const svtkRenderState* s) override;

  /**
   * Interface to get the rendered image in FLOATING_POINT mode.  Returns a
   * single component array containing the rendered values.
   * \warning The returned array is owned by this class.
   */
  svtkFloatArray* GetFloatImageDataArray(svtkRenderer* ren);

  /**
   * Interface to get the rendered image in FLOATING_POINT mode.  Low level API,
   * a format for the internal glReadPixels call can be specified. 'data' is expected
   * to be allocated and cleaned-up by the caller.
   */
  void GetFloatImageData(int const format, int const width, int const height, void* data);

  /**
   * Interface to get the rendered image in FLOATING_POINT mode.  Image extents of
   * the value array.
   */
  int* GetFloatImageExtents();

  /**
   * Check for extension support.
   * @deprecated As of 9.0, All platforms support FLOATING_POINT.
   */
  SVTK_LEGACY(bool IsFloatingPointModeSupported());

  void ReleaseGraphicsResources(svtkWindow* win) override;

  /**
   * Convert an RGB triplet to a floating point value. This method is exposed
   * as a convenience function for testing (TestValuePass2).
   * @deprecated As of 9.0, not necessary with FLOATING_POINT.
   */
  SVTK_LEGACY(void ColorToValue(
    unsigned char const* color, double const min, double const scale, double& value));

protected:
  svtkValuePass();
  ~svtkValuePass() override;

  ///@{
  /**
   * \brief svtkOpenGLRenderPass API.
   */

  /**
   * Use svtkShaderProgram::Substitute to replace //SVTK::XXX:YYY declarations in
   * the shader sources. Gets called after other mapper shader replacements.
   * Return false on error.
   */
  bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, svtkAbstractMapper* mapper, svtkProp* prop) override;
  /**
   * Update the uniforms of the shader program.
   * Return false on error.
   */
  bool SetShaderParameters(svtkShaderProgram* program, svtkAbstractMapper* mapper, svtkProp* prop,
    svtkOpenGLVertexArrayObject* VAO = nullptr) override;
  /**
   * For multi-stage render passes that need to change shader code during a
   * single pass, use this method to notify a mapper that the shader needs to be
   * rebuilt (rather than reuse the last cached shader. This method should
   * return the last time that the shader stage changed, or 0 if the shader
   * is single-stage.
   */
  svtkMTimeType GetShaderStageMTime() override;
  ///@}

  /**
   * Manages graphics resources depending on the rendering mode.  Binds internal
   * FBO when FLOATING_POINT mode is enabled.
   */
  void BeginPass(svtkRenderer* ren);

  /**
   * Unbinds internal FBO when FLOATING_POINT mode is enabled.
   */
  void EndPass();

  /**
   * Opaque pass with key checking.
   * \pre s_exists: s!=0
   */
  void RenderOpaqueGeometry(const svtkRenderState* s);

  /**
   * Unbind textures, etc.
   */
  void RenderPieceFinish();

  /**
   * Upload new data if necessary, bind textures, etc.
   */
  void RenderPieceStart(svtkDataArray* dataArr, svtkMapper* m);

  /**
   * Setup the mapper state, buffer objects or property variables necessary
   * to render the active rendering mode.
   */
  void BeginMapperRender(svtkMapper* mapper, svtkDataArray* dataArray, svtkProperty* property);

  /**
   * Revert any changes made in BeginMapperRender.
   */
  void EndMapperRender(svtkMapper* mapper, svtkProperty* property);

  void InitializeBuffers(svtkRenderer* ren);

  /**
   * Add necessary shader definitions.
   */
  bool UpdateShaders(std::string& VSSource, std::string& FSSource);

  /**
   * Bind shader variables.
   */
  void BindAttributes(svtkShaderProgram* prog, svtkOpenGLVertexArrayObject* VAO);
  void BindUniforms(svtkShaderProgram* prog);

  //@{
  /**
   * Methods managing graphics resources required during FLOATING_POINT mode.
   */
  bool HasWindowSizeChanged(svtkRenderer* ren);
  bool InitializeFBO(svtkRenderer* ren);
  void ReleaseFBO(svtkWindow* win);
  //@}

  class svtkInternalsFloat;
  svtkInternalsFloat* ImplFloat;

  class svtkInternalsInvertible;
  svtkInternalsInvertible* ImplInv;

  struct Parameters;
  Parameters* PassState;

  int RenderingMode;

private:
  svtkDataArray* GetCurrentArray(svtkMapper* mapper, Parameters* arrayPar);

  svtkAbstractArray* GetArrayFromCompositeData(svtkMapper* mapper, Parameters* arrayPar);

  svtkSmartPointer<svtkAbstractArray> MultiBlocksArray;

  void PopulateCellCellMap(const svtkRenderState* s);

  svtkValuePass(const svtkValuePass&) = delete;
  void operator=(const svtkValuePass&) = delete;
};

#endif
