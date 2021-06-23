/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPanoramicProjectionPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPanoramicProjectionPass
 * @brief   Render pass that render the scene in a cubemap and project
 * these six renderings to a single quad.
 * There are currently two different projections implemented (Equirectangular and Azimuthal).
 * This pass can be used to produce images that can be visualize with specific devices that re-maps
 * the distorted image to a panoramic view (for instance VR headsets, domes, panoramic screens)
 *
 * Note that it is often necessary to disable frustum cullers in order to render
 * properly objects that are behind the camera.
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkPanoramicProjectionPass_h
#define svtkPanoramicProjectionPass_h

#include "svtkImageProcessingPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLFramebufferObject;
class svtkOpenGLQuadHelper;
class svtkTextureObject;

class SVTKRENDERINGOPENGL2_EXPORT svtkPanoramicProjectionPass : public svtkImageProcessingPass
{
public:
  static svtkPanoramicProjectionPass* New();
  svtkTypeMacro(svtkPanoramicProjectionPass, svtkImageProcessingPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state.
   */
  void Render(const svtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own resources.
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;

  //@{
  /**
   * Get/Set the cubemap textures resolution used to render (offscreen) all directions.
   * Default is 300.
   */
  svtkGetMacro(CubeResolution, unsigned int);
  svtkSetMacro(CubeResolution, unsigned int);
  //@}

  /**
   * Enumeration of projection types.
   */
  enum : int
  {
    Equirectangular = 1, /**< Equirectangular projection */
    Azimuthal = 2        /**< Azimuthal equidistant projection */
  };

  //@{
  /**
   * Get/Set the type of projection.
   * Equirectangular projection maps meridians to vertical straight lines and circles of latitude to
   * horizontal straight lines.
   * Azimuthal equidistant projection maps all points of the scene based on their distance to the
   * view direction. This projection produces a fisheye effect.
   * Default is Equirectangular.
   */
  svtkGetMacro(ProjectionType, int);
  svtkSetClampMacro(ProjectionType, int, Equirectangular, Azimuthal);
  void SetProjectionTypeToEquirectangular() { this->SetProjectionType(Equirectangular); }
  void SetProjectionTypeToAzimuthal() { this->SetProjectionType(Azimuthal); }
  //@}

  //@{
  /**
   * Get/Set the vertical angle of projection.
   * 180 degrees is a half sphere, 360 degrees is a full sphere,
   * but any values in the range (90;360) can be set.
   * Default is 180 degrees.
   */
  svtkGetMacro(Angle, double);
  svtkSetClampMacro(Angle, double, 90.0, 360.0);
  //@}

  //@{
  /**
   * Get/Set the interpolation mode.
   * If true, the projection of the cubemap use hardware interpolation.
   * Default is off.
   */
  svtkGetMacro(Interpolate, bool);
  svtkSetMacro(Interpolate, bool);
  svtkBooleanMacro(Interpolate, bool);
  //@}

protected:
  svtkPanoramicProjectionPass() = default;
  ~svtkPanoramicProjectionPass() override = default;

  void RenderOnFace(const svtkRenderState* s, int index);

  void Project(svtkOpenGLRenderWindow* renWin);

  void InitOpenGLResources(svtkOpenGLRenderWindow* renWin);

  /**
   * Graphics resources.
   */
  svtkOpenGLFramebufferObject* FrameBufferObject = nullptr;
  svtkTextureObject* CubeMapTexture = nullptr;
  svtkOpenGLQuadHelper* QuadHelper = nullptr;

  unsigned int CubeResolution = 300;
  int ProjectionType = Equirectangular;
  double Angle = 180.0;
  bool Interpolate = false;

private:
  svtkPanoramicProjectionPass(const svtkPanoramicProjectionPass&) = delete;
  void operator=(const svtkPanoramicProjectionPass&) = delete;
};

#endif
