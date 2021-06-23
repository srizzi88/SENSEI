/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderWindowCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRenderWindowCollection
 * @brief   an ordered list of RenderWindows
 *
 * svtkRenderWindowCollection represents and provides methods to manipulate a
 * list of RenderWindows. The list is ordered and duplicate entries are
 * not prevented.
 *
 * @sa
 * svtkRenderWindow svtkCollection
 */

#ifndef svtkRenderWindowCollection_h
#define svtkRenderWindowCollection_h

#include "svtkCollection.h"
#include "svtkRenderWindow.h"        // Needed for static cast
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkRenderWindowCollection : public svtkCollection
{
public:
  static svtkRenderWindowCollection* New();
  svtkTypeMacro(svtkRenderWindowCollection, svtkCollection);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add a RenderWindow to the bottom of the list.
   */
  void AddItem(svtkRenderWindow* a) { this->svtkCollection::AddItem(a); }

  /**
   * Get the next RenderWindow in the list. Return NULL when at the end of the
   * list.
   */
  svtkRenderWindow* GetNextItem()
  {
    return static_cast<svtkRenderWindow*>(this->GetNextItemAsObject());
  }

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkRenderWindow* GetNextRenderWindow(svtkCollectionSimpleIterator& cookie)
  {
    return static_cast<svtkRenderWindow*>(this->GetNextItemAsObject(cookie));
  }

protected:
  svtkRenderWindowCollection() {}
  ~svtkRenderWindowCollection() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }

private:
  svtkRenderWindowCollection(const svtkRenderWindowCollection&) = delete;
  void operator=(const svtkRenderWindowCollection&) = delete;
};
#endif
