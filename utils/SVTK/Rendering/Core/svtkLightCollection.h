/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLightCollection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLightCollection
 * @brief   an ordered list of lights
 *
 * svtkLightCollection represents and provides methods to manipulate a list of
 * lights (i.e., svtkLight and subclasses). The list is ordered and duplicate
 * entries are not prevented.
 *
 * @sa
 * svtkCollection svtkLight
 */

#ifndef svtkLightCollection_h
#define svtkLightCollection_h

#include "svtkCollection.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkLight;

class SVTKRENDERINGCORE_EXPORT svtkLightCollection : public svtkCollection
{
public:
  static svtkLightCollection* New();
  svtkTypeMacro(svtkLightCollection, svtkCollection);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Add a light to the bottom of the list.
   */
  void AddItem(svtkLight* a);

  /**
   * Get the next light in the list. NULL is returned when the collection is
   * exhausted.
   */
  svtkLight* GetNextItem();

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkLight* GetNextLight(svtkCollectionSimpleIterator& cookie);

protected:
  svtkLightCollection() {}
  ~svtkLightCollection() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }

private:
  svtkLightCollection(const svtkLightCollection&) = delete;
  void operator=(const svtkLightCollection&) = delete;
};

#endif
