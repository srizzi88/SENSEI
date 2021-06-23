/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVolumeCollection
 * @brief   an ordered list of volumes
 *
 * svtkVolumeCollection represents and provides methods to manipulate a
 * list of volumes (i.e., svtkVolume and subclasses). The list is ordered
 * and duplicate entries are not prevented.
 *
 * @sa
 * svtkCollection svtkVolume
 */

#ifndef svtkVolumeCollection_h
#define svtkVolumeCollection_h

#include "svtkPropCollection.h"
#include "svtkRenderingCoreModule.h" // For export macro

#include "svtkVolume.h" // Needed for static cast

class SVTKRENDERINGCORE_EXPORT svtkVolumeCollection : public svtkPropCollection
{
public:
  static svtkVolumeCollection* New();
  svtkTypeMacro(svtkVolumeCollection, svtkPropCollection);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add a Volume to the bottom of the list.
   */
  void AddItem(svtkVolume* a) { this->svtkCollection::AddItem(a); }

  /**
   * Get the next Volume in the list. Return NULL when at the end of the
   * list.
   */
  svtkVolume* GetNextVolume() { return static_cast<svtkVolume*>(this->GetNextItemAsObject()); }

  /**
   * Access routine provided for compatibility with previous
   * versions of SVTK.  Please use the GetNextVolume() variant
   * where possible.
   */
  svtkVolume* GetNextItem() { return this->GetNextVolume(); }

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkVolume* GetNextVolume(svtkCollectionSimpleIterator& cookie)
  {
    return static_cast<svtkVolume*>(this->GetNextItemAsObject(cookie));
  }

protected:
  svtkVolumeCollection() {}
  ~svtkVolumeCollection() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }
  void AddItem(svtkProp* o) { this->svtkPropCollection::AddItem(o); }

private:
  svtkVolumeCollection(const svtkVolumeCollection&) = delete;
  void operator=(const svtkVolumeCollection&) = delete;
};

#endif
