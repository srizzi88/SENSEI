/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkActor2DCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkActor2DCollection
 * @brief    a list of 2D actors
 *
 * svtkActor2DCollection is a subclass of svtkCollection.  svtkActor2DCollection
 * maintains a collection of svtkActor2D objects that is sorted by layer
 * number, with lower layer numbers at the start of the list.  This allows
 * the svtkActor2D objects to be rendered in the correct order.
 *
 * @sa
 * svtkActor2D svtkCollection
 */

#ifndef svtkActor2DCollection_h
#define svtkActor2DCollection_h

#include "svtkPropCollection.h"
#include "svtkRenderingCoreModule.h" // For export macro

#include "svtkActor2D.h" // Needed for inline methods

class svtkViewport;

class SVTKRENDERINGCORE_EXPORT svtkActor2DCollection : public svtkPropCollection
{
public:
  /**
   * Destructor for the svtkActor2DCollection class. This removes all
   * objects from the collection.
   */
  static svtkActor2DCollection* New();

  svtkTypeMacro(svtkActor2DCollection, svtkPropCollection);

  /**
   * Sorts the svtkActor2DCollection by layer number.  Smaller layer
   * numbers are first.  Layer numbers can be any integer value.
   */
  void Sort();

  /**
   * Add an actor to the list.  The new actor is inserted in the list
   * according to it's layer number.
   */
  void AddItem(svtkActor2D* a);

  //@{
  /**
   * Standard Collection methods
   */
  int IsItemPresent(svtkActor2D* a);
  svtkActor2D* GetNextActor2D();
  svtkActor2D* GetLastActor2D();
  //@}

  //@{
  /**
   * Access routines that are provided for compatibility with previous
   * version of SVTK.  Please use the GetNextActor2D(), GetLastActor2D()
   * variants where possible.
   */
  svtkActor2D* GetNextItem();
  svtkActor2D* GetLastItem();
  //@}

  /**
   * Sort and then render the collection of 2D actors.
   */
  void RenderOverlay(svtkViewport* viewport);

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkActor2D* GetNextActor2D(svtkCollectionSimpleIterator& cookie)
  {
    return static_cast<svtkActor2D*>(this->GetNextItemAsObject(cookie));
  }

protected:
  svtkActor2DCollection() {}
  ~svtkActor2DCollection() override;

  void DeleteElement(svtkCollectionElement*) override;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }
  void AddItem(svtkProp* o) { this->svtkPropCollection::AddItem(o); }
  int IsItemPresent(svtkObject* o) { return this->svtkCollection::IsItemPresent(o); }

private:
  svtkActor2DCollection(const svtkActor2DCollection&) = delete;
  void operator=(const svtkActor2DCollection&) = delete;
};

inline int svtkActor2DCollection::IsItemPresent(svtkActor2D* a)
{
  return this->svtkCollection::IsItemPresent(a);
}

inline svtkActor2D* svtkActor2DCollection::GetNextActor2D()
{
  return static_cast<svtkActor2D*>(this->GetNextItemAsObject());
}

inline svtkActor2D* svtkActor2DCollection::GetLastActor2D()
{
  if (this->Bottom == nullptr)
  {
    return nullptr;
  }
  else
  {
    return static_cast<svtkActor2D*>(this->Bottom->Item);
  }
}

inline svtkActor2D* svtkActor2DCollection::GetNextItem()
{
  return this->GetNextActor2D();
}

inline svtkActor2D* svtkActor2DCollection::GetLastItem()
{
  return this->GetLastActor2D();
}

#endif

// SVTK-HeaderTest-Exclude: svtkActor2DCollection.h
