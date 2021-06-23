/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextScenePrivate.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkContextScenePrivate
 * @brief   Private implementation for scene/items.
 *
 *
 * Provides a list of context items, and convenience functions to paint
 * all of the children of the scene/item. This is a private class to be used
 * in svtkContextScene, svtkAbstractContextItem and friends.
 *
 * \internal
 */

#ifndef svtkContextScenePrivate_h
#define svtkContextScenePrivate_h

#include "svtkAbstractContextItem.h"
#include "svtkContextScene.h"

// STL headers
#include <vector> // Needed for STL vector.

class svtkContext2D;

//-----------------------------------------------------------------------------
class svtkContextScenePrivate : public std::vector<svtkAbstractContextItem*>
{
public:
  /**
   * Default constructor.
   */
  svtkContextScenePrivate(svtkAbstractContextItem* item)
    : std::vector<svtkAbstractContextItem*>()
    , Scene(nullptr)
    , Item(item)
  {
  }

  /**
   * Destructor.
   */
  ~svtkContextScenePrivate() { this->Clear(); }

  //@{
  /**
   * A few standard defines
   */
  typedef std::vector<svtkAbstractContextItem*>::const_iterator const_iterator;
  typedef std::vector<svtkAbstractContextItem*>::iterator iterator;
  typedef std::vector<svtkAbstractContextItem*>::const_reverse_iterator const_reverse_iterator;
  typedef std::vector<svtkAbstractContextItem*>::reverse_iterator reverse_iterator;
  //@}

  /**
   * Paint all items in the list.
   */
  void PaintItems(svtkContext2D* context)
  {
    for (const_iterator it = this->begin(); it != this->end(); ++it)
    {
      if ((*it)->GetVisible())
      {
        (*it)->Paint(context);
      }
    }
  }

  //@{
  /**
   * Add an item to the list - ensure it is not already in the list.
   */
  unsigned int AddItem(svtkAbstractContextItem* item)
  {
    item->Register(this->Scene);
    item->SetScene(this->Scene);
    item->SetParent(this->Item);
    //@}

    this->push_back(item);
    return static_cast<unsigned int>(this->size() - 1);
  }

  //@{
  /**
   * Remove an item from the list.
   */
  bool RemoveItem(svtkAbstractContextItem* item)
  {
    for (iterator it = this->begin(); it != this->end(); ++it)
    {
      if (item == *it)
      {
        item->SetParent(nullptr);
        item->SetScene(nullptr);
        (*it)->Delete();
        this->erase(it);
        return true;
      }
    }
    return false;
  }
  //@}

  //@{
  /**
   * Remove an item from the list.
   */
  bool RemoveItem(unsigned int index)
  {
    if (index < this->size())
    {
      return this->RemoveItem(this->at(index));
    }
    return false;
  }
  //@}

  //@{
  /**
   * Clear all items from the list - unregister.
   */
  void Clear()
  {
    for (const_iterator it = this->begin(); it != this->end(); ++it)
    {
      (*it)->SetParent(nullptr);
      (*it)->SetScene(nullptr);
      (*it)->Delete();
    }
    this->clear();
  }
  //@}

  //@{
  /**
   * Set the scene for the instance (and its items).
   */
  void SetScene(svtkContextScene* scene)
  {
    if (this->Scene == scene)
    {
      return;
    }
    this->Scene = scene;
    for (const_iterator it = this->begin(); it != this->end(); ++it)
    {
      (*it)->SetScene(scene);
    }
  }
  //@}

  /**
   * Store a reference to the scene.
   */
  svtkContextScene* Scene;

  //@{
  /**
   * Store a reference to the item that these children are part of.
   * May be NULL for items in the scene itself.
   */
  svtkAbstractContextItem* Item;
  //@}
};

#endif // svtkContextScenePrivate_h
// SVTK-HeaderTest-Exclude: svtkContextScenePrivate.h
