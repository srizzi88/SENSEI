/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageReader2Collection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageReader2Collection
 * @brief   maintain a list of image readers
 *
 * svtkImageReader2Collection is an object that creates and manipulates
 * lists of objects of type svtkImageReader2 and its subclasses.
 * @sa
 * svtkCollection svtkPlaneCollection
 */

#ifndef svtkImageReader2Collection_h
#define svtkImageReader2Collection_h

#include "svtkCollection.h"
#include "svtkIOImageModule.h" // For export macro

class svtkImageReader2;

class SVTKIOIMAGE_EXPORT svtkImageReader2Collection : public svtkCollection
{
public:
  svtkTypeMacro(svtkImageReader2Collection, svtkCollection);
  static svtkImageReader2Collection* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add an image reader to the list.
   */
  void AddItem(svtkImageReader2*);

  /**
   * Get the next image reader in the list.
   */
  svtkImageReader2* GetNextItem();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkImageReader2* GetNextImageReader2(svtkCollectionSimpleIterator& cookie);

protected:
  svtkImageReader2Collection() {}
  ~svtkImageReader2Collection() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }

private:
  svtkImageReader2Collection(const svtkImageReader2Collection&) = delete;
  void operator=(const svtkImageReader2Collection&) = delete;
};

#endif
