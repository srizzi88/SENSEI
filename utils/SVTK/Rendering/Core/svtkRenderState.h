/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderState.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRenderState
 * @brief   Context in which a svtkRenderPass will render.
 *
 * svtkRenderState is a ligthweight effective class which gather information
 * used by a svtkRenderPass to perform its execution.
 * @attention
 * Get methods are const to enforce that a renderpass cannot modify the
 * RenderPass object. It works in conjunction with svtkRenderPass::Render where
 * the argument svtkRenderState is const.
 * @sa
 * svtkRenderPass svtkRenderer svtkFrameBufferObject svtkProp
 */

#ifndef svtkRenderState_h
#define svtkRenderState_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;
class svtkProp;
class svtkFrameBufferObjectBase;
class svtkInformation;

class SVTKRENDERINGCORE_EXPORT svtkRenderState
{
public:
  /**
   * Constructor. All values are initialized to 0 or NULL.
   * \pre renderer_exists: renderer!=0
   * \post renderer_is_set: GetRenderer()==renderer.
   * \post valid_state: IsValid()
   */
  svtkRenderState(svtkRenderer* renderer);

  /**
   * Destructor. As a svtkRenderState does not own any of its variables,
   * the destructor does nothing.
   */
  ~svtkRenderState();

  /**
   * Tells if the RenderState is a valid one (Renderer is not null).
   */
  bool IsValid() const;

  /**
   * Return the Renderer. This is the renderer in which the render pass is
   * performed. It gives access to the RenderWindow, to the props.
   * \post result_exists: result!=0
   */
  svtkRenderer* GetRenderer() const;

  /**
   * Return the FrameBuffer. This is the framebuffer in use. NULL means it is
   * the FrameBuffer provided by the RenderWindow (it can actually be an FBO
   * in case the RenderWindow is in off screen mode).
   */
  svtkFrameBufferObjectBase* GetFrameBuffer() const;

  /**
   * Set the FrameBuffer. See GetFrameBuffer().
   * \post is_set: GetFrameBuffer()==fbo
   */
  void SetFrameBuffer(svtkFrameBufferObjectBase* fbo);

  /**
   * Get the window size of the state.
   */
  void GetWindowSize(int size[2]) const;

  /**
   * Return the array of filtered props. See SetPropArrayAndCount().
   */
  svtkProp** GetPropArray() const;

  /**
   * Return the size of the array of filtered props.
   * See SetPropArrayAndCount().
   * \post positive_result: result>=0
   */
  int GetPropArrayCount() const;

  /**
   * Set the array of filtered props and its size.
   * It is a subset of props to render. A renderpass might ignore this
   * filtered list and access to all the props of the svtkRenderer object
   * directly. For example, a render pass may filter props that are visible and
   * not culled by the frustum, but a sub render pass building a shadow map may
   * need all the visible props.
   * \pre positive_size: propArrayCount>=0
   * \pre valid_null_array: propArray!=0 || propArrayCount==0
   * \post is_set: GetPropArray()==propArray && GetPropArrayCount()==propArrayCount
   */
  void SetPropArrayAndCount(svtkProp** propArray, int propArrayCount);

  /**
   * Return the required property keys for the props. It tells that the
   * current render pass it supposed to render only props that have all the
   * RequiredKeys in their property keys.
   */
  svtkInformation* GetRequiredKeys() const;

  /**
   * Set the required property keys for the props. See GetRequiredKeys().
   * \post is_set: GetRequiredKeys()==keys
   */
  void SetRequiredKeys(svtkInformation* keys);

protected:
  /**
   * The renderer in which the render pass is performed.
   * It gives access to the RenderWindow, to the props.
   */
  svtkRenderer* Renderer;

  /**
   * The framebuffer in use. NULL means the FrameBuffer provided by
   * the RenderWindow (it can actually be an FBO in case the RenderWindow
   * is in off screen mode).
   */
  svtkFrameBufferObjectBase* FrameBuffer;

  //@{
  /**
   * Subset of props to render. A renderpass might ignore this filtered list
   * and access to all the props of the svtkRenderer object directly.
   * For example, a render pass may filter props that are visible and
   * not culled by the frustum, but a sub render pass building a shadow map may
   * need all the visible props.
   */
  svtkProp** PropArray;
  int PropArrayCount;
  //@}

  /**
   * It tells that the current render pass it supposed to render only props
   * that have all the RequiredKeys in their property keys.
   */
  svtkInformation* RequiredKeys;

private:
  svtkRenderState(); // no default constructor.
  svtkRenderState(const svtkRenderState&) = delete;
  void operator=(const svtkRenderState&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkRenderState.h
