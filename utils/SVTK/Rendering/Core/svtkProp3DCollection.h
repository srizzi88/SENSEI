/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProp3DCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProp3DCollection
 * @brief   an ordered list of 3D props
 *
 * svtkProp3DCollection represents and provides methods to manipulate a list of
 * 3D props (i.e., svtkProp3D and subclasses). The list is ordered and
 * duplicate entries are not prevented.
 *
 * @sa
 * svtkProp3D svtkCollection
 */

#ifndef svtkProp3DCollection_h
#define svtkProp3DCollection_h

#include "svtkProp3D.h" // Needed for inline methods
#include "svtkPropCollection.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkProp3DCollection : public svtkPropCollection
{
public:
  static svtkProp3DCollection* New();
  svtkTypeMacro(svtkProp3DCollection, svtkPropCollection);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add an actor to the bottom of the list.
   */
  void AddItem(svtkProp3D* p);

  /**
   * Get the next actor in the list.
   */
  svtkProp3D* GetNextProp3D();

  /**
   * Get the last actor in the list.
   */
  svtkProp3D* GetLastProp3D();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkProp3D* GetNextProp3D(svtkCollectionSimpleIterator& cookie)
  {
    return static_cast<svtkProp3D*>(this->GetNextItemAsObject(cookie));
  }

protected:
  svtkProp3DCollection() {}
  ~svtkProp3DCollection() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }
  void AddItem(svtkProp* o) { this->svtkPropCollection::AddItem(o); }

private:
  svtkProp3DCollection(const svtkProp3DCollection&) = delete;
  void operator=(const svtkProp3DCollection&) = delete;
};

inline void svtkProp3DCollection::AddItem(svtkProp3D* a)
{
  this->svtkCollection::AddItem(a);
}

inline svtkProp3D* svtkProp3DCollection::GetNextProp3D()
{
  return static_cast<svtkProp3D*>(this->GetNextItemAsObject());
}

inline svtkProp3D* svtkProp3DCollection::GetLastProp3D()
{
  if (this->Bottom == nullptr)
  {
    return nullptr;
  }
  else
  {
    return static_cast<svtkProp3D*>(this->Bottom->Item);
  }
}

#endif
