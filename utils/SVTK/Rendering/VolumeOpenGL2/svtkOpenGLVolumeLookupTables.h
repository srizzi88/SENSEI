/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLVolumeLookupTables.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*=============================================================================
Copyright and License information
=============================================================================*/
/**
 * @class svtkOpenGLVolumeLookupTables
 * @brief Internal class that manages multiple lookup tables
 *
 */

#ifndef svtkOpenGLVolumeLookupTables_h
#define svtkOpenGLVolumeLookupTables_h
#ifndef __SVTK_WRAP__

#include "svtkObject.h"

// STL includes
#include <vector>

// Forward declarations
class svtkWindow;

template <class T>
class svtkOpenGLVolumeLookupTables : public svtkObject
{
public:
  svtkTypeMacro(svtkOpenGLVolumeLookupTables, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkOpenGLVolumeLookupTables<T>* New();

  /**
   * Create internal lookup tables
   */
  virtual void Create(std::size_t numberOfTables);

  /**
   * Get access to the raw table pointer
   */
  T* GetTable(std::size_t i) const;

  /**
   * Get number of tables
   */
  std::size_t GetNumberOfTables() const;

  /**
   * Release graphics resources
   */
  void ReleaseGraphicsResources(svtkWindow* win);

protected:
  svtkOpenGLVolumeLookupTables() = default;
  virtual ~svtkOpenGLVolumeLookupTables() override;

  std::vector<T*> Tables;

private:
  svtkOpenGLVolumeLookupTables(const svtkOpenGLVolumeLookupTables&) = delete;
  void operator=(const svtkOpenGLVolumeLookupTables&) = delete;
};

#include "svtkOpenGLVolumeLookupTables.txx"

#endif // __SVTK_WRAP__
#endif // svtkOpenGLVolumeLookupTables_h
// SVTK-HeaderTest-Exclude: svtkOpenGLVolumeLookupTables.h
