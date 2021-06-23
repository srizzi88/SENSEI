/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageSliceCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageSliceCollection
 * @brief   a sorted list of image slice objects
 *
 * svtkImageSliceCollection is a svtkPropCollection that maintains
 * a list of svtkImageSlice objects that are sorted by LayerNumber.
 * This allows the images to be rendered in the correct order.
 * @sa
 * svtkImageSlice svtkImageAssembly
 */

#ifndef svtkImageSliceCollection_h
#define svtkImageSliceCollection_h

#include "svtkImageSlice.h" // to allow inline static-cast
#include "svtkPropCollection.h"
#include "svtkRenderingImageModule.h" // For export macro

class SVTKRENDERINGIMAGE_EXPORT svtkImageSliceCollection : public svtkPropCollection
{
public:
  static svtkImageSliceCollection* New();
  svtkTypeMacro(svtkImageSliceCollection, svtkPropCollection);

  /**
   * Sorts the svtkImageSliceCollection by layer number.  Smaller layer
   * numbers are first. Layer numbers can be any integer value. Items
   * with the same layer number will be kept in the same relative order
   * as before the sort.
   */
  void Sort();

  /**
   * Add an image to the list.  The new image is inserted in the list
   * according to its layer number.
   */
  void AddItem(svtkImageSlice* a);

  /**
   * Standard Collection methods.  You must call InitTraversal
   * before calling GetNextImage.  If possible, you should use the
   * GetNextImage method that takes a collection iterator instead.
   */
  svtkImageSlice* GetNextImage();

  /**
   * Reentrant safe way to get an object in a collection.
   */
  svtkImageSlice* GetNextImage(svtkCollectionSimpleIterator& cookie);

  /**
   * Access routine provided for compatibility with previous
   * versions of SVTK.  Please use the GetNextImage() variant
   * where possible.
   */
  svtkImageSlice* GetNextItem() { return this->GetNextImage(); }

protected:
  svtkImageSliceCollection() {}
  ~svtkImageSliceCollection() override;

  void DeleteElement(svtkCollectionElement*) override;

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }
  void AddItem(svtkProp* o) { this->svtkPropCollection::AddItem(o); }

private:
  svtkImageSliceCollection(const svtkImageSliceCollection&) = delete;
  void operator=(const svtkImageSliceCollection&) = delete;
};

inline svtkImageSlice* svtkImageSliceCollection::GetNextImage()
{
  return static_cast<svtkImageSlice*>(this->GetNextItemAsObject());
}

inline svtkImageSlice* svtkImageSliceCollection::GetNextImage(svtkCollectionSimpleIterator& cookie)
{
  return static_cast<svtkImageSlice*>(this->GetNextItemAsObject(cookie));
}

#endif
