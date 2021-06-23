/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAssemblyPaths.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAssemblyPaths
 * @brief   a list of lists of props representing an assembly hierarchy
 *
 * svtkAssemblyPaths represents an assembly hierarchy as a list of
 * svtkAssemblyPath. Each path represents the complete path from the
 * top level assembly (if any) down to the leaf prop.
 *
 * @sa
 * svtkAssemblyPath svtkAssemblyNode svtkPicker svtkAssembly svtkProp
 */

#ifndef svtkAssemblyPaths_h
#define svtkAssemblyPaths_h

#include "svtkCollection.h"
#include "svtkRenderingCoreModule.h" // For export macro

#include "svtkAssemblyPath.h" // Needed for inline methods

class svtkProp;

class SVTKRENDERINGCORE_EXPORT svtkAssemblyPaths : public svtkCollection
{
public:
  static svtkAssemblyPaths* New();
  svtkTypeMacro(svtkAssemblyPaths, svtkCollection);

  /**
   * Add a path to the list.
   */
  void AddItem(svtkAssemblyPath* p);

  /**
   * Remove a path from the list.
   */
  void RemoveItem(svtkAssemblyPath* p);

  /**
   * Determine whether a particular path is present. Returns its position
   * in the list.
   */
  int IsItemPresent(svtkAssemblyPath* p);

  /**
   * Get the next path in the list.
   */
  svtkAssemblyPath* GetNextItem();

  /**
   * Override the standard GetMTime() to check for the modified times
   * of the paths.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Reentrant safe way to get an object in a collection. Just pass the
   * same cookie back and forth.
   */
  svtkAssemblyPath* GetNextPath(svtkCollectionSimpleIterator& cookie)
  {
    return static_cast<svtkAssemblyPath*>(this->GetNextItemAsObject(cookie));
  }

protected:
  svtkAssemblyPaths() {}
  ~svtkAssemblyPaths() override {}

private:
  // hide the standard AddItem from the user and the compiler.
  void AddItem(svtkObject* o) { this->svtkCollection::AddItem(o); }
  void RemoveItem(svtkObject* o) { this->svtkCollection::RemoveItem(o); }
  void RemoveItem(int i) { this->svtkCollection::RemoveItem(i); }
  int IsItemPresent(svtkObject* o) { return this->svtkCollection::IsItemPresent(o); }

private:
  svtkAssemblyPaths(const svtkAssemblyPaths&) = delete;
  void operator=(const svtkAssemblyPaths&) = delete;
};

inline void svtkAssemblyPaths::AddItem(svtkAssemblyPath* p)
{
  this->svtkCollection::AddItem(p);
}

inline void svtkAssemblyPaths::RemoveItem(svtkAssemblyPath* p)
{
  this->svtkCollection::RemoveItem(p);
}

inline int svtkAssemblyPaths::IsItemPresent(svtkAssemblyPath* p)
{
  return this->svtkCollection::IsItemPresent(p);
}

inline svtkAssemblyPath* svtkAssemblyPaths::GetNextItem()
{
  return static_cast<svtkAssemblyPath*>(this->GetNextItemAsObject());
}

#endif
// SVTK-HeaderTest-Exclude: svtkAssemblyPaths.h
