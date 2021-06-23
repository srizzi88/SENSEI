/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSurfaceLICMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSurfaceLICMapper
 * @brief   mapper that performs LIC on the surface of
 *  arbitrary geometry.
 *
 *
 *  svtkSurfaceLICMapper performs LIC on the surface of arbitrary
 *  geometry. Point vectors are used as the vector field for generating the LIC.
 *  The implementation was originallu  based on "Image Space Based Visualization
 *  on Unsteady Flow on Surfaces" by Laramee, Jobard and Hauser appeared in
 *  proceedings of IEEE Visualization '03, pages 131-138.
 *
 *  Internal pipeline:
 * <pre>
 * noise
 *     |
 *     [ PROJ (GAT) (COMP) LIC2D (SCAT) SHADE (CCE) DEP]
 *     |                                               |
 * vectors                                         surface LIC
 * </pre>
 * PROj  - prject vectors onto surface
 * GAT   - gather data for compositing and guard pixel generation  (parallel only)
 * COMP  - composite gathered data
 * LIC2D - line intengral convolution, see svtkLineIntegralConvolution2D.
 * SCAT  - scatter result (parallel only, not all compositors use it)
 * SHADE - combine LIC and scalar colors
 * CCE   - color contrast enhancement (optional)
 * DEP   - depth test and copy to back buffer
 *
 * The result of each stage is cached in a texture so that during interaction
 * a stage may be skipped if the user has not modified its parameters or input
 * data.
 *
 * The parallel parts of algorithm are implemented in svtkPSurfaceLICMapper.
 * Note that for MPI enabled builds this class will be automatically created
 * by the object factory.
 *
 * @sa
 * svtkLineIntegralConvolution2D
 */

#ifndef svtkSurfaceLICMapper_h
#define svtkSurfaceLICMapper_h

#include "svtkOpenGLPolyDataMapper.h"
#include "svtkRenderingLICOpenGL2Module.h" // For export macro

class svtkSurfaceLICInterface;
class svtkPainterCommunicator;

class SVTKRENDERINGLICOPENGL2_EXPORT svtkSurfaceLICMapper : public svtkOpenGLPolyDataMapper
{
public:
  static svtkSurfaceLICMapper* New();
  svtkTypeMacro(svtkSurfaceLICMapper, svtkOpenGLPolyDataMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release. In this case, releases the display lists.
   */
  void ReleaseGraphicsResources(svtkWindow* win) override;

  /**
   * Implemented by sub classes. Actual rendering is done here.
   */
  void RenderPiece(svtkRenderer* ren, svtkActor* act) override;

  /**
   * Shallow copy of an actor.
   */
  void ShallowCopy(svtkAbstractMapper*) override;

  //@{
  /**
   * Get the svtkSurfaceLICInterface used by this mapper
   */
  svtkGetObjectMacro(LICInterface, svtkSurfaceLICInterface);
  //@}

protected:
  svtkSurfaceLICMapper();
  ~svtkSurfaceLICMapper() override;

  /**
   * Methods used for parallel benchmarks. Use cmake to define
   * svtkSurfaceLICMapperTIME to enable benchmarks. During each
   * update timing information is stored, it can be written to
   * disk by calling WriteLog.
   */
  virtual void StartTimerEvent(const char*) {}
  virtual void EndTimerEvent(const char*) {}

  /**
   * Build the VBO/IBO, called by UpdateBufferObjects
   */
  void BuildBufferObjects(svtkRenderer* ren, svtkActor* act) override;

protected:
  /**
   * Set the shader parameteres related to the mapper/input data, called by UpdateShader
   */
  void SetMapperShaderParameters(svtkOpenGLHelper& cellBO, svtkRenderer* ren, svtkActor* act) override;

  /**
   * Perform string replacements on the shader templates
   */
  void ReplaceShaderValues(
    std::map<svtkShader::Type, svtkShader*> shaders, svtkRenderer* ren, svtkActor* act) override;

  svtkSurfaceLICInterface* LICInterface;

private:
  svtkSurfaceLICMapper(const svtkSurfaceLICMapper&) = delete;
  void operator=(const svtkSurfaceLICMapper&) = delete;
};

#endif
