/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPropCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPropCollection
 * @brief   an ordered list of Props
 *
 * svtkPropCollection represents and provides methods to manipulate a list of
 * Props (i.e., svtkProp and subclasses). The list is ordered and duplicate
 * entries are not prevented.
 *
 * @sa
 * svtkProp svtkCollection
 */

#ifndef svtkPropCollection_h
#define svtkPropCollection_h

#include "svtkCollection.h"
#include "svtkRenderingCoreModule.h" // For export macro

#include "svtkProp.h" // Needed for inline methods

class SVTKRENDERINGCORE_EXPORT svtkPropCollection : public svtkCollection
{
public:
  static svtkPropCollection* New();
  svtkTypeMacro(svtkPropCollection, svtkCollection);

  /**
   * Add a Prop to the bottom of the list.
   */
  void AddItem(svtkProp* a);

  /**
   * Get the next Prop in the list.
   */
  svtkProp* GetNextProp();

  /**
   * Get the last Prop in the list.
   */
  svtkProp* GetLastProp();

  /**
   * Get the number of paths contained in this list. (Recall that a
   * svtkProp can consist of multiple parts.) Used in picking and other
   * activities to get the parts of composite entities like svtkAssembly
   * or svtkPropAssembly.
   */
  int GetNumberOfPaths();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkProp* GetNextProp(svtkCollectionSimpleIterator& cookie)
  {
    return static_cast<svtkProp*>(this->GetNextItemAsObject(cookie));
  }

protected:
  svtkPropCollection() {}
  ~svtkPropCollection() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }

private:
  svtkPropCollection(const svtkPropCollection&) = delete;
  void operator=(const svtkPropCollection&) = delete;
};

inline void svtkPropCollection::AddItem(svtkProp* a)
{
  this->svtkCollection::AddItem(a);
}

inline svtkProp* svtkPropCollection::GetNextProp()
{
  return static_cast<svtkProp*>(this->GetNextItemAsObject());
}

inline svtkProp* svtkPropCollection::GetLastProp()
{
  if (this->Bottom == nullptr)
  {
    return nullptr;
  }
  else
  {
    return static_cast<svtkProp*>(this->Bottom->Item);
  }
}

#endif

// SVTK-HeaderTest-Exclude: svtkPropCollection.h
