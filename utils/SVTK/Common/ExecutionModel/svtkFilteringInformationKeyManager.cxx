/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFilteringInformationKeyManager.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkFilteringInformationKeyManager.h"

#include "svtkInformationKey.h"

#include <vector>

// Subclass vector so we can directly call constructor.  This works
// around problems on Borland C++.
struct svtkFilteringInformationKeyManagerKeysType : public std::vector<svtkInformationKey*>
{
  typedef std::vector<svtkInformationKey*> Superclass;
  typedef Superclass::iterator iterator;
};

//----------------------------------------------------------------------------
// Must NOT be initialized.  Default initialization to zero is
// necessary.
static unsigned int svtkFilteringInformationKeyManagerCount;
static svtkFilteringInformationKeyManagerKeysType* svtkFilteringInformationKeyManagerKeys;

//----------------------------------------------------------------------------
svtkFilteringInformationKeyManager::svtkFilteringInformationKeyManager()
{
  if (++svtkFilteringInformationKeyManagerCount == 1)
  {
    svtkFilteringInformationKeyManager::ClassInitialize();
  }
}

//----------------------------------------------------------------------------
svtkFilteringInformationKeyManager::~svtkFilteringInformationKeyManager()
{
  if (--svtkFilteringInformationKeyManagerCount == 0)
  {
    svtkFilteringInformationKeyManager::ClassFinalize();
  }
}

//----------------------------------------------------------------------------
void svtkFilteringInformationKeyManager::Register(svtkInformationKey* key)
{
  // Register this instance for deletion by the singleton.
  svtkFilteringInformationKeyManagerKeys->push_back(key);
}

//----------------------------------------------------------------------------
void svtkFilteringInformationKeyManager::ClassInitialize()
{
  // Allocate the singleton storing pointers to information keys.
  // This must be a malloc/free pair instead of new/delete to work
  // around problems on MachO (Mac OS X) runtime systems that do lazy
  // symbol loading.  Calling operator new here causes static
  // initialization to occur in other translation units immediately,
  // which then may try to access the vector before it is set here.
  void* keys = malloc(sizeof(svtkFilteringInformationKeyManagerKeysType));
  svtkFilteringInformationKeyManagerKeys = new (keys) svtkFilteringInformationKeyManagerKeysType;
}

//----------------------------------------------------------------------------
void svtkFilteringInformationKeyManager::ClassFinalize()
{
  if (svtkFilteringInformationKeyManagerKeys)
  {
    // Delete information keys.
    for (svtkFilteringInformationKeyManagerKeysType::iterator i =
           svtkFilteringInformationKeyManagerKeys->begin();
         i != svtkFilteringInformationKeyManagerKeys->end(); ++i)
    {
      svtkInformationKey* key = *i;
      delete key;
    }

    // Delete the singleton storing pointers to information keys.  See
    // ClassInitialize above for why this is a free instead of a
    // delete.
    svtkFilteringInformationKeyManagerKeys->~svtkFilteringInformationKeyManagerKeysType();
    free(svtkFilteringInformationKeyManagerKeys);
    svtkFilteringInformationKeyManagerKeys = nullptr;
  }
}
