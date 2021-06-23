/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkObjectIdMap.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkObjectIdMap
 * @brief   class used to assign Id to any SVTK object and be able
 * to retrieve it base on its id.
 */

#ifndef svtkObjectIdMap_h
#define svtkObjectIdMap_h

#include "svtkObject.h"
#include "svtkWebCoreModule.h" // needed for exports

class SVTKWEBCORE_EXPORT svtkObjectIdMap : public svtkObject
{
public:
  static svtkObjectIdMap* New();
  svtkTypeMacro(svtkObjectIdMap, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Retrieve a unique identifier for the given object or generate a new one
   * if its global id was never requested.
   */
  svtkTypeUInt32 GetGlobalId(svtkObject* obj);

  /**
   * Retrieve a svtkObject based on its global id. If not found return nullptr
   */
  svtkObject* GetSVTKObject(svtkTypeUInt32 globalId);

  /**
   * Assign an active key (string) to an existing object.
   * This is usually used to provide another type of access to specific
   * svtkObject that we want to retrieve easily using a string.
   * Return the global Id of the given registered object
   */
  svtkTypeUInt32 SetActiveObject(const char* objectType, svtkObject* obj);

  /**
   * Retrieve a previously stored object based on a name
   */
  svtkObject* GetActiveObject(const char* objectType);

  /**
   * Remove any internal reference count due to internal Id/Object mapping
   */
  void FreeObject(svtkObject* obj);

protected:
  svtkObjectIdMap();
  ~svtkObjectIdMap() override;

private:
  svtkObjectIdMap(const svtkObjectIdMap&) = delete;
  void operator=(const svtkObjectIdMap&) = delete;

  struct svtkInternals;
  svtkInternals* Internals;
};

#endif
