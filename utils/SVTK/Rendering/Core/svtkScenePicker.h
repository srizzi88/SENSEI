/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkScenePicker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkScenePicker
 * @brief   Picks an entire viewport at one shot.
 *
 * The Scene picker, unlike conventional pickers picks an entire viewport at
 * one shot and caches the result, which can be retrieved later.
 *    The utility of the class arises during <b>Actor Selection</b>. Let's
 * say you have a couple of polygonal objects in your scene and you wish to
 * have a status bar that indicates the object your mouse is over. Picking
 * repeatedly every time your mouse moves would be very slow. The
 * scene picker automatically picks your viewport every time the camera is
 * changed and caches the information. Additionally, it observes the
 * svtkRenderWindowInteractor to avoid picking during interaction, so that
 * you still maintain your interactivity. In effect, the picker does an
 * additional pick-render of your scene every time you stop interacting with
 * your scene. As an example, see Rendering/TestScenePicker.
 *
 * @warning
 * - Unlike a svtkHoverWidget, this class is not timer based. The hover widget
 *   picks a scene when the mouse is over an actor for a specified duration.
 * - This class uses a svtkHardwareSelector under the hood. Hence, it will
 *   work only for actors that have opaque geomerty and are rendered by a
 *   svtkPolyDataMapper.
 *
 * @sa
 * svtkHoverWidget svtkHardwareSelector
 */

#ifndef svtkScenePicker_h
#define svtkScenePicker_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;
class svtkProp;
class svtkHardwareSelector;
class svtkRenderWindowInteractor;
class svtkScenePickerSelectionRenderCommand;

class SVTKRENDERINGCORE_EXPORT svtkScenePicker : public svtkObject
{

  friend class svtkRenderer;
  friend class svtkScenePickerSelectionRenderCommand;

public:
  static svtkScenePicker* New();
  svtkTypeMacro(svtkScenePicker, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the renderer. Scene picks are restricted to the viewport.
   */
  virtual void SetRenderer(svtkRenderer*);
  svtkGetObjectMacro(Renderer, svtkRenderer);
  //@}

  /**
   * Get cell id at the pick position.
   * Returns -1 if no cell was picked.
   * Makes sense only after Pick has been called.
   */
  svtkIdType GetCellId(int displayPos[2]);

  /**
   * Get cell id at the pick position.
   * Returns -1 if no cell was picked.
   * Makes sense only after Pick has been called.
   */
  svtkIdType GetVertexId(int displayPos[2]);

  /**
   * Get actor at the pick position.
   * Returns NULL if none.
   * Makes sense only after Pick has been called.
   */
  svtkProp* GetViewProp(int displayPos[2]);

  //@{
  /**
   * Vertex picking (using the method GetVertexId()), required
   * additional resources and can slow down still render time by
   * 5-10%. Enabled by default.
   */
  svtkSetMacro(EnableVertexPicking, svtkTypeBool);
  svtkGetMacro(EnableVertexPicking, svtkTypeBool);
  svtkBooleanMacro(EnableVertexPicking, svtkTypeBool);
  //@}

protected:
  svtkScenePicker();
  ~svtkScenePicker() override;

  // Pick render entire viewport
  // Automatically invoked from svtkRenderer at the end of a still render.
  void PickRender();

  // Pick render a region of the renderwindow
  void PickRender(int x0, int y0, int x1, int y1);

  // Internal update method retrieves info from the Selector
  void Update(int displayPos[2]);

  // The RenderWindowInteractor must be set, so that avoid scene picks (which
  // involve extra renders) during interaction. This is done by observing the
  // RenderWindowInteractor for start and end interaction events.
  void SetInteractor(svtkRenderWindowInteractor*);

  svtkTypeBool EnableVertexPicking;
  svtkHardwareSelector* Selector;
  svtkRenderer* Renderer;
  svtkRenderWindowInteractor* Interactor;
  svtkIdType VertId;
  svtkIdType CellId;
  svtkProp* Prop;
  bool NeedToUpdate;
  int LastQueriedDisplayPos[2];
  svtkScenePickerSelectionRenderCommand* SelectionRenderCommand;

  svtkTimeStamp PickRenderTime;

private:
  svtkScenePicker(const svtkScenePicker&) = delete;
  void operator=(const svtkScenePicker&) = delete;
};

#endif
