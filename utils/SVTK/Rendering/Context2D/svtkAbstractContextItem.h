/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkAbstractContextItem
 * @brief   base class for items that are part of a
 * svtkContextScene.
 *
 *
 * This class is the common base for all context scene items. You should
 * generally derive from svtkContextItem, rather than this class, as it provides
 * most of the commonly used API.
 */

#ifndef svtkAbstractContextItem_h
#define svtkAbstractContextItem_h

#include "svtkObject.h"
#include "svtkRenderingContext2DModule.h" // For export macro

class svtkContext2D;
class svtkContextMouseEvent;
class svtkContextKeyEvent;
class svtkContextScene;
class svtkContextScenePrivate;
class svtkVector2f;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkAbstractContextItem : public svtkObject
{
public:
  svtkTypeMacro(svtkAbstractContextItem, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform any updates to the item that may be necessary before rendering.
   * The scene should take care of calling this on all items before their
   * Paint function is invoked.
   */
  virtual void Update();

  /**
   * Paint event for the item, called whenever the item needs to be drawn.
   */
  virtual bool Paint(svtkContext2D* painter);

  /**
   * Paint the children of the item, should be called whenever the children
   * need to be rendered.
   */
  virtual bool PaintChildren(svtkContext2D* painter);

  /**
   * Release graphics resources hold by the item. The default implementation
   * is empty.
   */
  virtual void ReleaseGraphicsResources();

  /**
   * Add child items to this item. Increments reference count of item.
   * \return the index of the child item.
   */
  svtkIdType AddItem(svtkAbstractContextItem* item);

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
  bool RemoveItem(svtkIdType index);

  /**
   * Get the item at the specified index.
   * \return the item at the specified index (null if index is invalid).
   */
  svtkAbstractContextItem* GetItem(svtkIdType index);

  /**
   * Get the index of the specified item in itemIndex.
   * \return the item index if found or -1 if not.
   */
  svtkIdType GetItemIndex(svtkAbstractContextItem* item);

  /**
   * Get the number of child items.
   */
  svtkIdType GetNumberOfItems();

  /**
   * Remove all child items from this item.
   */
  void ClearItems();

  /**
   * Raises the \a child to the top of the item's stack.
   * \return The new index of the item
   * \sa StackAbove(), Lower(), LowerUnder()
   */
  svtkIdType Raise(svtkIdType index);

  /**
   * Raises the \a child above the \a under sibling. If \a under is invalid,
   * the item is raised to the top of the item's stack.
   * \return The new index of the item
   * \sa Raise(), Lower(), StackUnder()
   */
  virtual svtkIdType StackAbove(svtkIdType index, svtkIdType under);

  /**
   * Lowers the \a child to the bottom of the item's stack.
   * \return The new index of the item
   * \sa StackUnder(), Raise(), StackAbove()
   */
  svtkIdType Lower(svtkIdType index);

  /**
   * Lowers the \a child under the \a above sibling. If \a above is invalid,
   * the item is lowered to the bottom of the item's stack
   * \return The new index of the item
   * \sa Lower(), Raise(), StackAbove()
   */
  virtual svtkIdType StackUnder(svtkIdType child, svtkIdType above);

  /**
   * Return true if the supplied x, y coordinate is inside the item.
   */
  virtual bool Hit(const svtkContextMouseEvent& mouse);

  /**
   * Return the item under the mouse.
   * If no item is under the mouse, the method returns a null pointer.
   */
  virtual svtkAbstractContextItem* GetPickedItem(const svtkContextMouseEvent& mouse);

  /**
   * Mouse enter event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseEnterEvent(const svtkContextMouseEvent& mouse);

  /**
   * Mouse move event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseMoveEvent(const svtkContextMouseEvent& mouse);

  /**
   * Mouse leave event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseLeaveEvent(const svtkContextMouseEvent& mouse);

  /**
   * Mouse button down event
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseButtonPressEvent(const svtkContextMouseEvent& mouse);

  /**
   * Mouse button release event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseButtonReleaseEvent(const svtkContextMouseEvent& mouse);

  /**
   * Mouse button double click event.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseDoubleClickEvent(const svtkContextMouseEvent& mouse);

  /**
   * Mouse wheel event, positive delta indicates forward movement of the wheel.
   * Return true if the item holds the event, false if the event can be
   * propagated to other items.
   */
  virtual bool MouseWheelEvent(const svtkContextMouseEvent& mouse, int delta);

  /**
   * Key press event.
   */
  virtual bool KeyPressEvent(const svtkContextKeyEvent& key);

  /**
   * Key release event.
   */
  virtual bool KeyReleaseEvent(const svtkContextKeyEvent& key);

  /**
   * Set the svtkContextScene for the item, always set for an item in a scene.
   */
  virtual void SetScene(svtkContextScene* scene);

  /**
   * Get the svtkContextScene for the item, always set for an item in a scene.
   */
  svtkContextScene* GetScene() { return this->Scene; }

  /**
   * Set the parent item. The parent will be set for all items except top
   * level items in a scene.
   */
  virtual void SetParent(svtkAbstractContextItem* parent);

  /**
   * Get the parent item. The parent will be set for all items except top
   * level items in a tree.
   */
  svtkAbstractContextItem* GetParent() { return this->Parent; }

  /**
   * Maps the point to the parent coordinate system.
   */
  virtual svtkVector2f MapToParent(const svtkVector2f& point);

  /**
   * Maps the point from the parent coordinate system.
   */
  virtual svtkVector2f MapFromParent(const svtkVector2f& point);

  /**
   * Maps the point to the scene coordinate system.
   */
  virtual svtkVector2f MapToScene(const svtkVector2f& point);

  /**
   * Maps the point from the scene coordinate system.
   */
  virtual svtkVector2f MapFromScene(const svtkVector2f& point);

  //@{
  /**
   * Get the visibility of the item (should it be drawn).
   */
  svtkGetMacro(Visible, bool);
  //@}

  //@{
  /**
   * Set the visibility of the item (should it be drawn).
   * Visible by default.
   */
  svtkSetMacro(Visible, bool);
  //@}

  //@{
  /**
   * Get if the item is interactive (should respond to mouse events).
   */
  svtkGetMacro(Interactive, bool);
  //@}

  //@{
  /**
   * Set if the item is interactive (should respond to mouse events).
   */
  svtkSetMacro(Interactive, bool);
  //@}

protected:
  svtkAbstractContextItem();
  ~svtkAbstractContextItem() override;

  /**
   * Point to the scene the item is on - can be null.
   */
  svtkContextScene* Scene;

  /**
   * Point to the parent item - can be null.
   */
  svtkAbstractContextItem* Parent;

  /**
   * This structure provides a list of children, along with convenience
   * functions to paint the children etc. It is derived from
   * std::vector<svtkAbstractContextItem>, defined in a private header.
   */
  svtkContextScenePrivate* Children;

  /**
   * Store the visibility of the item (default is true).
   */
  bool Visible;

  /**
   * Store whether the item should respond to interactions (default is true).
   */
  bool Interactive;

private:
  svtkAbstractContextItem(const svtkAbstractContextItem&) = delete;
  void operator=(const svtkAbstractContextItem&) = delete;
};

#endif // svtkContextItem_h
