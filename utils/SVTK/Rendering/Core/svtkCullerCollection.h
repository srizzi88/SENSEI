/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCullerCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCullerCollection
 * @brief   an ordered list of Cullers
 *
 * svtkCullerCollection represents and provides methods to manipulate a list
 * of Cullers (i.e., svtkCuller and subclasses). The list is ordered and
 * duplicate entries are not prevented.
 *
 * @sa
 * svtkCuller svtkCollection
 */

#ifndef svtkCullerCollection_h
#define svtkCullerCollection_h

#include "svtkCollection.h"
#include "svtkCuller.h"              // for inline functions
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkCullerCollection : public svtkCollection
{
public:
  static svtkCullerCollection* New();
  svtkTypeMacro(svtkCullerCollection, svtkCollection);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add an Culler to the bottom of the list.
   */
  void AddItem(svtkCuller* a) { this->svtkCollection::AddItem(a); }

  /**
   * Get the next Culler in the list.
   */
  svtkCuller* GetNextItem() { return static_cast<svtkCuller*>(this->GetNextItemAsObject()); }

  /**
   * Get the last Culler in the list.
   */
  svtkCuller* GetLastItem();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkCuller* GetNextCuller(svtkCollectionSimpleIterator& cookie)
  {
    return static_cast<svtkCuller*>(this->GetNextItemAsObject(cookie));
  }

protected:
  svtkCullerCollection() {}
  ~svtkCullerCollection() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }

private:
  svtkCullerCollection(const svtkCullerCollection&) = delete;
  void operator=(const svtkCullerCollection&) = delete;
};

inline svtkCuller* svtkCullerCollection::GetLastItem()
{
  if (this->Bottom == nullptr)
  {
    return nullptr;
  }
  else
  {
    return static_cast<svtkCuller*>(this->Bottom->Item);
  }
}

#endif
