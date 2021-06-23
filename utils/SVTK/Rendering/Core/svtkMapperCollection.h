/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMapperCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMapperCollection
 * @brief   an ordered list of mappers
 *
 * svtkMapperCollection represents and provides methods to manipulate a list of
 * mappers (i.e., svtkMapper and subclasses). The list is ordered and duplicate
 * entries are not prevented.
 *
 * @sa
 * svtkMapper svtkCollection
 */

#ifndef svtkMapperCollection_h
#define svtkMapperCollection_h

#include "svtkCollection.h"
#include "svtkMapper.h"              // Needed for direct access to mapper methods in
                                    // inline functions
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkMapperCollection : public svtkCollection
{
public:
  static svtkMapperCollection* New();
  svtkTypeMacro(svtkMapperCollection, svtkCollection);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add an mapper to the bottom of the list.
   */
  void AddItem(svtkMapper* a) { this->svtkCollection::AddItem(a); }

  /**
   * Get the next mapper in the list.
   */
  svtkMapper* GetNextItem() { return static_cast<svtkMapper*>(this->GetNextItemAsObject()); }

  /**
   * Get the last mapper in the list.
   */
  svtkMapper* GetLastItem()
  {
    return this->Bottom ? static_cast<svtkMapper*>(this->Bottom->Item) : nullptr;
  }

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkMapper* GetNextMapper(svtkCollectionSimpleIterator& cookie)
  {
    return static_cast<svtkMapper*>(this->GetNextItemAsObject(cookie));
  }

protected:
  svtkMapperCollection() {}
  ~svtkMapperCollection() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }

private:
  svtkMapperCollection(const svtkMapperCollection&) = delete;
  void operator=(const svtkMapperCollection&) = delete;
};

#endif
