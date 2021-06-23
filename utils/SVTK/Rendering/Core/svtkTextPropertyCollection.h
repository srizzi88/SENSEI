/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextPropertyCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTextPropertyCollection
 * @brief   an ordered list of svtkTextProperty objects.
 *
 * svtkTextPropertyCollection represents and provides methods to manipulate a
 * list of TextProperty objects. The list is ordered and
 * duplicate entries are not prevented.
 * @sa
 * svtkTextProperty svtkCollection
 */

#ifndef svtkTextPropertyCollection_h
#define svtkTextPropertyCollection_h

#include "svtkCollection.h"
#include "svtkRenderingCoreModule.h" // For export macro
#include "svtkTextProperty.h"        // for inline functions

class SVTKRENDERINGCORE_EXPORT svtkTextPropertyCollection : public svtkCollection
{
public:
  static svtkTextPropertyCollection* New();
  svtkTypeMacro(svtkTextPropertyCollection, svtkCollection);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add a svtkTextProperty to the bottom of the list.
   */
  void AddItem(svtkTextProperty* a);

  /**
   * Get the next svtkTextProperty in the list.
   */
  svtkTextProperty* GetNextItem();

  /**
   * Get the svtkTextProperty at the specified index.
   */
  svtkTextProperty* GetItem(int idx);

  /**
   * Get the last TextProperty in the list.
   */
  svtkTextProperty* GetLastItem();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkTextProperty* GetNextTextProperty(svtkCollectionSimpleIterator& cookie);

protected:
  svtkTextPropertyCollection();
  ~svtkTextPropertyCollection() override;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o);

private:
  svtkTextPropertyCollection(const svtkTextPropertyCollection&) = delete;
  void operator=(const svtkTextPropertyCollection&) = delete;
};

inline void svtkTextPropertyCollection::AddItem(svtkTextProperty* a)
{
  this->svtkCollection::AddItem(a);
}

inline svtkTextProperty* svtkTextPropertyCollection::GetNextItem()
{
  return static_cast<svtkTextProperty*>(this->GetNextItemAsObject());
}

inline svtkTextProperty* svtkTextPropertyCollection::GetItem(int idx)
{
  return static_cast<svtkTextProperty*>(this->GetItemAsObject(idx));
}

inline svtkTextProperty* svtkTextPropertyCollection::GetLastItem()
{
  if (this->Bottom == nullptr)
  {
    return nullptr;
  }
  else
  {
    return static_cast<svtkTextProperty*>(this->Bottom->Item);
  }
}

inline svtkTextProperty* svtkTextPropertyCollection::GetNextTextProperty(
  svtkCollectionSimpleIterator& it)
{
  return static_cast<svtkTextProperty*>(this->GetNextItemAsObject(it));
}

inline void svtkTextPropertyCollection::AddItem(svtkObject* o)
{
  this->svtkCollection::AddItem(o);
}

#endif
