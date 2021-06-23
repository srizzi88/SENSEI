/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFilteringInformationKeyManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFilteringInformationKeyManager
 * @brief   Manages key types in svtkFiltering.
 *
 * svtkFilteringInformationKeyManager is included in the header of any
 * subclass of svtkInformationKey defined in the svtkFiltering library.
 * It makes sure that the table of keys is created before and
 * destroyed after it is used.
 */

#ifndef svtkFilteringInformationKeyManager_h
#define svtkFilteringInformationKeyManager_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkSystemIncludes.h"

#include "svtkDebugLeaksManager.h" // DebugLeaks exists longer than info keys.

class svtkInformationKey;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkFilteringInformationKeyManager
{
public:
  svtkFilteringInformationKeyManager();
  ~svtkFilteringInformationKeyManager();

  /**
   * Called by constructors of svtkInformationKey subclasses defined in
   * svtkFiltering to register themselves with the manager.  The
   * instances will be deleted when svtkFiltering is unloaded on
   * program exit.
   */
  static void Register(svtkInformationKey* key);

private:
  // Unimplemented
  svtkFilteringInformationKeyManager(const svtkFilteringInformationKeyManager&);
  svtkFilteringInformationKeyManager& operator=(const svtkFilteringInformationKeyManager&);

  static void ClassInitialize();
  static void ClassFinalize();
};

// This instance will show up in any translation unit that uses key
// types defined in svtkFiltering or that has a singleton.  It will
// make sure svtkFilteringInformationKeyManager's vector of keys is
// initialized before and destroyed after it is used.
static svtkFilteringInformationKeyManager svtkFilteringInformationKeyManagerInstance;

#endif
// SVTK-HeaderTest-Exclude: svtkFilteringInformationKeyManager.h
