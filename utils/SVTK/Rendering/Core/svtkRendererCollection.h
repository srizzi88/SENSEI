/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRendererCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRendererCollection
 * @brief   an ordered list of renderers
 *
 * svtkRendererCollection represents and provides methods to manipulate a list
 * of renderers (i.e., svtkRenderer and subclasses). The list is ordered and
 * duplicate entries are not prevented.
 *
 * @sa
 * svtkRenderer svtkCollection
 */

#ifndef svtkRendererCollection_h
#define svtkRendererCollection_h

#include "svtkCollection.h"
#include "svtkRenderer.h"            // Needed for static cast
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkRendererCollection : public svtkCollection
{
public:
  static svtkRendererCollection* New();
  svtkTypeMacro(svtkRendererCollection, svtkCollection);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add a Renderer to the bottom of the list.
   */
  void AddItem(svtkRenderer* a) { this->svtkCollection::AddItem(a); }

  /**
   * Get the next Renderer in the list.
   * Return NULL when at the end of the list.
   */
  svtkRenderer* GetNextItem() { return static_cast<svtkRenderer*>(this->GetNextItemAsObject()); }

  /**
   * Forward the Render() method to each renderer in the list.
   */
  void Render();

  /**
   * Get the first Renderer in the list.
   * Return NULL when at the end of the list.
   */
  svtkRenderer* GetFirstRenderer();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkRenderer* GetNextRenderer(svtkCollectionSimpleIterator& cookie)
  {
    return static_cast<svtkRenderer*>(this->GetNextItemAsObject(cookie));
  }

protected:
  svtkRendererCollection() {}
  ~svtkRendererCollection() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }

  svtkRendererCollection(const svtkRendererCollection&) = delete;
  void operator=(const svtkRendererCollection&) = delete;
};

#endif
