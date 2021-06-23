/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDepthPeelingPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDepthPeelingPass
 * @brief   Implement Depth Peeling for use within a framebuffer pass
 *
 * Note that this implementation is used as a fallback for drivers that
 * don't support floating point textures. Most renderings will use the subclass
 * svtkDualDepthPeelingPass instead.
 *
 * Render the translucent polygonal geometry of a scene without sorting
 * polygons in the view direction.
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 * An opaque pass may have been performed right after the initialization.
 *
 * The depth peeling algorithm works by rendering the translucent polygonal
 * geometry multiple times (once for each peel). The actually rendering of
 * the translucent polygonal geometry is performed by its delegate
 * TranslucentPass. This delegate is therefore used multiple times.
 *
 * Its delegate is usually set to a svtkTranslucentPass.
 *
 * This implementation makes use of textures and is suitable for ES3
 * For ES3 it must be embedded within a pass that makes use of framebuffers
 * so that the required OpaqueZTexture and OpaqueRGBATexture can be
 * passed from the outer framebuffer pass. For OpenGL ES3 be aware the
 * occlusion ratio test is not supported. The maximum number of peels
 * is used instead so set it to a reasonable value. For many scenes
 * a value of 4 or 5 will work well.
 *
 * @sa
 * svtkRenderPass, svtkTranslucentPass, svtkFramebufferPass
 */

#ifndef svtkDepthPeelingPass_h
#define svtkDepthPeelingPass_h

#include "svtkOpenGLRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro
#include <vector>                      // STL Header

class svtkOpenGLFramebufferObject;
class svtkTextureObject;
class svtkOpenGLRenderWindow;
class svtkOpenGLState;
class svtkOpenGLQuadHelper;

class SVTKRENDERINGOPENGL2_EXPORT svtkDepthPeelingPass : public svtkOpenGLRenderPass
{
public:
  static svtkDepthPeelingPass* New();
  svtkTypeMacro(svtkDepthPeelingPass, svtkOpenGLRenderPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const svtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;

  //@{
  /**
   * Delegate for rendering the translucent polygonal geometry.
   * If it is NULL, nothing will be rendered and a warning will be emitted.
   * It is usually set to a svtkTranslucentPass.
   * Initial value is a NULL pointer.
   */
  svtkGetObjectMacro(TranslucentPass, svtkRenderPass);
  virtual void SetTranslucentPass(svtkRenderPass* translucentPass);
  //@}

  //@{
  /**
   * In case of use of depth peeling technique for rendering translucent
   * material, define the threshold under which the algorithm stops to
   * iterate over peel layers. This is the ratio of the number of pixels
   * that have been touched by the last layer over the total number of pixels
   * of the viewport area.
   * Initial value is 0.0, meaning rendering have to be exact. Greater values
   * may speed-up the rendering with small impact on the quality.
   */
  svtkSetClampMacro(OcclusionRatio, double, 0.0, 0.5);
  svtkGetMacro(OcclusionRatio, double);
  //@}

  //@{
  /**
   * In case of depth peeling, define the maximum number of peeling layers.
   * Initial value is 4. A special value of 0 means no maximum limit.
   * It has to be a positive value.
   */
  svtkSetMacro(MaximumNumberOfPeels, int);
  svtkGetMacro(MaximumNumberOfPeels, int);
  //@}

  // svtkOpenGLRenderPass virtuals:
  bool PostReplaceShaderValues(std::string& vertexShader, std::string& geometryShader,
    std::string& fragmentShader, svtkAbstractMapper* mapper, svtkProp* prop) override;
  bool SetShaderParameters(svtkShaderProgram* program, svtkAbstractMapper* mapper, svtkProp* prop,
    svtkOpenGLVertexArrayObject* VAO = nullptr) override;

  // Set Opaque Z texture, this must be set from the outer FO
  void SetOpaqueZTexture(svtkTextureObject*);

  // Set Opaque RGBA texture, this must be set from the outer FO
  void SetOpaqueRGBATexture(svtkTextureObject*);

  /**
   *  Set the format to use for the depth texture
   * e.g. svtkTextureObject::Float32
   */
  svtkSetMacro(DepthFormat, int);

protected:
  /**
   * Default constructor. TranslucentPass is set to NULL.
   */
  svtkDepthPeelingPass();

  /**
   * Destructor.
   */
  ~svtkDepthPeelingPass() override;

  svtkRenderPass* TranslucentPass;
  svtkTimeStamp CheckTime;

  //@{
  /**
   * Cache viewport values for depth peeling.
   */
  int ViewportX;
  int ViewportY;
  int ViewportWidth;
  int ViewportHeight;
  //@}

  /**
   * In case of use of depth peeling technique for rendering translucent
   * material, define the threshold under which the algorithm stops to
   * iterate over peel layers. This is the ratio of the number of pixels
   * that have been touched by the last layer over the total number of pixels
   * of the viewport area.
   * Initial value is 0.0, meaning rendering have to be exact. Greater values
   * may speed-up the rendering with small impact on the quality.
   */
  double OcclusionRatio;

  /**
   * In case of depth peeling, define the maximum number of peeling layers.
   * Initial value is 4. A special value of 0 means no maximum limit.
   * It has to be a positive value.
   */
  int MaximumNumberOfPeels;

  svtkOpenGLFramebufferObject* Framebuffer;

  svtkOpenGLQuadHelper* FinalBlend;
  svtkOpenGLQuadHelper* IntermediateBlend;

  // obtained from the outer FO, we read from them
  svtkTextureObject* OpaqueZTexture;
  svtkTextureObject* OpaqueRGBATexture;
  bool OwnOpaqueZTexture;
  bool OwnOpaqueRGBATexture;

  // each peel merges two color buffers into one result
  svtkTextureObject* TranslucentRGBATexture[3];
  int ColorDrawCount;
  int PeelCount;

  // each peel compares a prior Z and writes to next
  svtkTextureObject* TranslucentZTexture[2];
  int DepthFormat;

  void BlendIntermediatePeels(svtkOpenGLRenderWindow* renWin, bool);
  void BlendFinalPeel(svtkOpenGLRenderWindow* renWin);

  // useful to store
  svtkOpenGLState* State;

private:
  svtkDepthPeelingPass(const svtkDepthPeelingPass&) = delete;
  void operator=(const svtkDepthPeelingPass&) = delete;
};

#endif
