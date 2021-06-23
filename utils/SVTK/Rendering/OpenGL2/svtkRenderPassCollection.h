/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRenderPassCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRenderPassCollection
 * @brief   an ordered list of RenderPasses
 *
 * svtkRenderPassCollection represents a list of RenderPasses
 * (i.e., svtkRenderPass and subclasses) and provides methods to manipulate the
 * list. The list is ordered and duplicate entries are not prevented.
 *
 * @sa
 * svtkRenderPass svtkCollection
 */

#ifndef svtkRenderPassCollection_h
#define svtkRenderPassCollection_h

#include "svtkCollection.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkRenderPass;

class SVTKRENDERINGOPENGL2_EXPORT svtkRenderPassCollection : public svtkCollection
{
public:
  static svtkRenderPassCollection* New();
  svtkTypeMacro(svtkRenderPassCollection, svtkCollection);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add an RenderPass to the bottom of the list.
   */
  void AddItem(svtkRenderPass* pass);

  /**
   * Get the next RenderPass in the list.
   */
  svtkRenderPass* GetNextRenderPass();

  /**
   * Get the last RenderPass in the list.
   */
  svtkRenderPass* GetLastRenderPass();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkRenderPass* GetNextRenderPass(svtkCollectionSimpleIterator& cookie);

protected:
  svtkRenderPassCollection();
  ~svtkRenderPassCollection() override;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o);

private:
  svtkRenderPassCollection(const svtkRenderPassCollection&) = delete;
  void operator=(const svtkRenderPassCollection&) = delete;
};

#endif
