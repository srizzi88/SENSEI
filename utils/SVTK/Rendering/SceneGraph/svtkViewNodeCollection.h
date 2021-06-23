/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkViewNodeCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkViewNodeCollection
 * @brief   collection of view nodes
 *
 * svtk centric collection of svtkViewNodes
 */

#ifndef svtkViewNodeCollection_h
#define svtkViewNodeCollection_h

#include "svtkCollection.h"
#include "svtkRenderingSceneGraphModule.h" // For export macro

class svtkViewNode;

class SVTKRENDERINGSCENEGRAPH_EXPORT svtkViewNodeCollection : public svtkCollection
{
public:
  static svtkViewNodeCollection* New();
  svtkTypeMacro(svtkViewNodeCollection, svtkCollection);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add a ViewNode to the list.
   */
  void AddItem(svtkViewNode* a);

  /**
   * Get the next ViewNode in the list. NULL is returned when the collection is
   * exhausted.
   */
  svtkViewNode* GetNextItem();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkViewNode* GetNextViewNode(svtkCollectionSimpleIterator& cookie);

  /**
   * Return true only if we have viewnode for obj.
   */
  bool IsRenderablePresent(svtkObject* obj);

protected:
  svtkViewNodeCollection() {}
  ~svtkViewNodeCollection() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }

  svtkViewNodeCollection(const svtkViewNodeCollection&) = delete;
  void operator=(const svtkViewNodeCollection&) = delete;
};

#endif
