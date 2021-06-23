/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkActorCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkActorCollection
 * @brief   an ordered list of actors
 *
 * svtkActorCollection represents and provides methods to manipulate a list of
 * actors (i.e., svtkActor and subclasses). The list is ordered and duplicate
 * entries are not prevented.
 *
 * @sa
 * svtkActor svtkCollection
 */

#ifndef svtkActorCollection_h
#define svtkActorCollection_h

#include "svtkActor.h" // For inline methods
#include "svtkPropCollection.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkProperty;

class SVTKRENDERINGCORE_EXPORT svtkActorCollection : public svtkPropCollection
{
public:
  static svtkActorCollection* New();
  svtkTypeMacro(svtkActorCollection, svtkPropCollection);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add an actor to the bottom of the list.
   */
  void AddItem(svtkActor* a);

  /**
   * Get the next actor in the list.
   */
  svtkActor* GetNextActor();

  /**
   * Get the last actor in the list.
   */
  svtkActor* GetLastActor();

  //@{
  /**
   * Access routines that are provided for compatibility with previous
   * version of SVTK.  Please use the GetNextActor(), GetLastActor() variants
   * where possible.
   */
  svtkActor* GetNextItem();
  svtkActor* GetLastItem();
  //@}

  /**
   * Apply properties to all actors in this collection.
   */
  void ApplyProperties(svtkProperty* p);

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkActor* GetNextActor(svtkCollectionSimpleIterator& cookie)
  {
    return static_cast<svtkActor*>(this->GetNextItemAsObject(cookie));
  }

protected:
  svtkActorCollection() {}
  ~svtkActorCollection() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }
  void AddItem(svtkProp* o) { this->svtkPropCollection::AddItem(o); }

private:
  svtkActorCollection(const svtkActorCollection&) = delete;
  void operator=(const svtkActorCollection&) = delete;
};

inline void svtkActorCollection::AddItem(svtkActor* a)
{
  this->svtkCollection::AddItem(a);
}

inline svtkActor* svtkActorCollection::GetNextActor()
{
  return static_cast<svtkActor*>(this->GetNextItemAsObject());
}

inline svtkActor* svtkActorCollection::GetLastActor()
{
  if (this->Bottom == nullptr)
  {
    return nullptr;
  }
  else
  {
    return static_cast<svtkActor*>(this->Bottom->Item);
  }
}

inline svtkActor* svtkActorCollection::GetNextItem()
{
  return this->GetNextActor();
}

inline svtkActor* svtkActorCollection::GetLastItem()
{
  return this->GetLastActor();
}

#endif
