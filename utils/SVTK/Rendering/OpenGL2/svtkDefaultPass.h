/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDefaultPass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDefaultPass
 * @brief   Implement the basic render passes.
 *
 * svtkDefaultPass implements the basic standard render passes of SVTK.
 * Subclasses can easily be implemented by reusing some parts of the basic
 * implementation.
 *
 * It implements classic Render operations as well as versions with property
 * key checking.
 *
 * This pass expects an initialized depth buffer and color buffer.
 * Initialized buffers means they have been cleared with farest z-value and
 * background color/gradient/transparent color.
 *
 * @sa
 * svtkRenderPass
 */

#ifndef svtkDefaultPass_h
#define svtkDefaultPass_h

#include "svtkRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLRenderWindow;
class svtkDefaultPassLayerList; // Pimpl

class SVTKRENDERINGOPENGL2_EXPORT svtkDefaultPass : public svtkRenderPass
{
public:
  static svtkDefaultPass* New();
  svtkTypeMacro(svtkDefaultPass, svtkRenderPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * Call RenderOpaqueGeometry(), RenderTranslucentPolygonalGeometry(),
   * RenderVolumetricGeometry(), RenderOverlay()
   * \pre s_exists: s!=0
   */
  void Render(const svtkRenderState* s) override;

protected:
  /**
   * Default constructor.
   */
  svtkDefaultPass();

  /**
   * Destructor.
   */
  ~svtkDefaultPass() override;

  /**
   * Opaque pass without key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderOpaqueGeometry(const svtkRenderState* s);

  /**
   * Opaque pass with key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderFilteredOpaqueGeometry(const svtkRenderState* s);

  /**
   * Translucent pass without key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderTranslucentPolygonalGeometry(const svtkRenderState* s);

  /**
   * Translucent pass with key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderFilteredTranslucentPolygonalGeometry(const svtkRenderState* s);

  /**
   * Volume pass without key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderVolumetricGeometry(const svtkRenderState* s);

  /**
   * Translucent pass with key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderFilteredVolumetricGeometry(const svtkRenderState* s);

  /**
   * Overlay pass without key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderOverlay(const svtkRenderState* s);

  /**
   * Overlay pass with key checking.
   * \pre s_exists: s!=0
   */
  virtual void RenderFilteredOverlay(const svtkRenderState* s);

private:
  svtkDefaultPass(const svtkDefaultPass&) = delete;
  void operator=(const svtkDefaultPass&) = delete;
};

#endif
