/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextScene.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkContextScene
 * @brief   Provides a 2D scene for svtkContextItem objects.
 *
 *
 * Provides a 2D scene that svtkContextItem objects can be added to. Manages the
 * items, ensures that they are rendered at the right times and passes on mouse
 * events.
 */

#ifndef svtkContextScene_h
#define svtkContextScene_h

#include "svtkObject.h"
#include "svtkRenderingContext2DModule.h" // For export macro
#include "svtkVector.h"                   // For svtkVector return type.
#include "svtkWeakPointer.h"              // Needed for weak pointer to the window.

class svtkContext2D;
class svtkAbstractContextItem;
class svtkTransform2D;
class svtkContextMouseEvent;
class svtkContextKeyEvent;
class svtkContextScenePrivate;
class svtkContextInteractorStyle;

class svtkAnnotationLink;

class svtkRenderer;
class svtkAbstractContextBufferId;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkContextScene : public svtkObject
{
public:
  svtkTypeMacro(svtkContextScene, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 2D Painter object.
   */
  static svtkContextScene* New();

  /**
   * Paint event for the chart, called whenever the chart needs to be drawn
   */
  virtual bool Paint(svtkContext2D* painter);

  /**
   * Add child items to this item. Increments reference count of item.
   * \return the index of the child item.
   */
  unsigned int AddItem(svtkAbstractContextItem* item);

  /**
   * Remove child item from this item. Decrements reference count of item.
   * \param item the item to be removed.
   * \return true on success, false otherwise.
   */
  bool RemoveItem(svtkAbstractContextItem* item);

  /**
   * Remove child item from this item. Decrements reference count of item.
   * \param index of the item to be removed.
   * \return true on success, false otherwise.
   */
  bool RemoveItem(unsigned int index);

  /**
   * Get the item at the specified index.
   * \return the item at the specified index (null if index is invalid).
   */
  svtkAbstractContextItem* GetItem(unsigned int index);

  /**
   * Get the number of child items.
   */
  unsigned int GetNumberOfItems();

  /**
   * Remove all child items from this item.
   */
  void ClearItems();

  /**
   * Set the svtkAnnotationLink for the chart.
   */
  virtual void SetAnnotationLink(svtkAnnotationLink* link);

  //@{
  /**
   * Get the svtkAnnotationLink for the chart.
   */
  svtkGetObjectMacro(AnnotationLink, svtkAnnotationLink);
  //@}

  //@{
  /**
   * Set the width and height of the scene in pixels.
   */
  svtkSetVector2Macro(Geometry, int);
  //@}

  //@{
  /**
   * Get the width and height of the scene in pixels.
   */
  svtkGetVector2Macro(Geometry, int);
  //@}

  //@{
  /**
   * Set whether the scene should use the color buffer. Default is true.
   */
  svtkSetMacro(UseBufferId, bool);
  //@}

  //@{
  /**
   * Get whether the scene is using the color buffer. Default is true.
   */
  svtkGetMacro(UseBufferId, bool);
  //@}

  /**
   * Get the width of the view
   */
  virtual int GetViewWidth();

  /**
   * Get the height of the view
   */
  virtual int GetViewHeight();

  /**
   * Get the width of the scene.
   */
  int GetSceneWidth();

  /**
   * Get the height of the scene.
   */
  int GetSceneHeight();

  //@{
  /**
   * Whether to scale the scene transform when tiling, for example when
   * using svtkWindowToImageFilter to take a large screenshot.
   * The default is true.
   */
  svtkSetMacro(ScaleTiles, bool);
  svtkGetMacro(ScaleTiles, bool);
  svtkBooleanMacro(ScaleTiles, bool);
  //@}

  /**
   * The tile scale of the target svtkRenderWindow. Hardcoded pixel offsets, etc
   * should properly account for these <x, y> scale factors. This will simply
   * return svtkVector2i(1, 1) if ScaleTiles is false or if this->Renderer is
   * NULL.
   */
  svtkVector2i GetLogicalTileScale();

  //@{
  /**
   * This should not be necessary as the context view should take care of
   * rendering.
   */
  virtual void SetRenderer(svtkRenderer* renderer);
  virtual svtkRenderer* GetRenderer();
  //@}

  //@{
  /**
   * Inform the scene that something changed that requires a repaint of the
   * scene. This should only be used by the svtkContextItem derived objects in
   * a scene in their event handlers.
   */
  void SetDirty(bool isDirty);
  bool GetDirty() const;
  //@}

  /**
   * Release graphics resources hold by the scene.
   */
  void ReleaseGraphicsResources();

  /**
   * Last painter used.
   * Not part of the end-user API. Can be used by context items to
   * create their own colorbuffer id (when a context item is a container).
   */
  svtkWeakPointer<svtkContext2D> GetLastPainter();

  /**
   * Return buffer id.
   * Not part of the end-user API. Can be used by context items to
   * initialize their own colorbuffer id (when a context item is a container).
   */
  svtkAbstractContextBufferId* GetBufferId();

  /**
   * Set the transform for the scene.
   */
  virtual void SetTransform(svtkTransform2D* transform);

  /**
   * Get the transform for the scene.
   */
  svtkTransform2D* GetTransform();

  /**
   * Check whether the scene has a transform.
   */
  bool HasTransform() { return this->Transform != nullptr; }

  /**
   * Enum of valid selection modes for charts in the scene
   */
  enum
  {
    SELECTION_NONE = 0,
    SELECTION_DEFAULT,
    SELECTION_ADDITION,
    SELECTION_SUBTRACTION,
    SELECTION_TOGGLE
  };

protected:
  svtkContextScene();
  ~svtkContextScene() override;

  /**
   * Process a rubber band selection event.
   */
  virtual bool ProcessSelectionEvent(unsigned int rect[5]);

  /**
   * Process a mouse move event.
   */
  virtual bool MouseMoveEvent(const svtkContextMouseEvent& event);

  /**
   * Process a mouse button press event.
   */
  virtual bool ButtonPressEvent(const svtkContextMouseEvent& event);

  /**
   * Process a mouse button release event.
   */
  virtual bool ButtonReleaseEvent(const svtkContextMouseEvent& event);

  /**
   * Process a mouse button double click event.
   */
  virtual bool DoubleClickEvent(const svtkContextMouseEvent& event);

  /**
   * Process a mouse wheel event where delta is the movement forward or back.
   */
  virtual bool MouseWheelEvent(int delta, const svtkContextMouseEvent& event);

  /**
   * Process a key press event.
   */
  virtual bool KeyPressEvent(const svtkContextKeyEvent& keyEvent);

  /**
   * Process a key release event.
   */
  virtual bool KeyReleaseEvent(const svtkContextKeyEvent& keyEvent);

  /**
   * Paint the scene in a special mode to build a cache for picking.
   * Use internally.
   */
  virtual void PaintIds();

  /**
   * Test if BufferId is supported by the OpenGL context.
   */
  void TestBufferIdSupport();

  /**
   * Return the item id under mouse cursor at position (x,y).
   * Return -1 if there is no item under the mouse cursor.
   * \post valid_result: result>=-1 && result<this->GetNumberOfItems()
   */
  svtkIdType GetPickedItem(int x, int y);

  /**
   * Return the item under the mouse.
   * If no item is under the mouse, the method returns a null pointer.
   */
  svtkAbstractContextItem* GetPickedItem();

  /**
   * Make sure the buffer id used for picking is up-to-date.
   */
  void UpdateBufferId();

  svtkAnnotationLink* AnnotationLink;

  // Store the chart dimensions - width, height of scene in pixels
  int Geometry[2];

  /**
   * The svtkContextInteractorStyle class delegates all of the events to the
   * scene, accessing protected API.
   */
  friend class svtkContextInteractorStyle;

  //@{
  /**
   * Private storage object - where we hide all of our STL objects...
   */
  class Private;
  Private* Storage;
  //@}

  /**
   * This structure provides a list of children, along with convenience
   * functions to paint the children etc. It is derived from
   * std::vector<svtkAbstractContextItem>, defined in a private header.
   */
  svtkContextScenePrivate* Children;

  svtkWeakPointer<svtkContext2D> LastPainter;

  svtkWeakPointer<svtkRenderer> Renderer;

  svtkAbstractContextBufferId* BufferId;
  bool BufferIdDirty;

  bool UseBufferId;

  bool BufferIdSupportTested;
  bool BufferIdSupported;

  bool ScaleTiles;

  /**
   * The scene level transform.
   */
  svtkTransform2D* Transform;

private:
  svtkContextScene(const svtkContextScene&) = delete;
  void operator=(const svtkContextScene&) = delete;

  typedef bool (svtkAbstractContextItem::*MouseEvents)(const svtkContextMouseEvent&);
  bool ProcessItem(
    svtkAbstractContextItem* cur, const svtkContextMouseEvent& event, MouseEvents eventPtr);
  void EventCopy(const svtkContextMouseEvent& event);
};

#endif // svtkContextScene_h
