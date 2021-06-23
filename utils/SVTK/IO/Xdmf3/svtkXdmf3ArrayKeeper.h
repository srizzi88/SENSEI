/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkXdmf3ArrayKeeper.h
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkXdmf3ArrayKeeper
 * @brief   LRU cache of XDMF Arrays
 *
 * svtkXdmf3ArrayKeeper maintains the in memory cache of recently used XdmfArrays.
 * Each array that is loaded from XDMF is put in the cache and/or marked with the
 * current timestep. A release method frees arrays that have not been recently
 * used.
 *
 * This file is a helper for the svtkXdmf3Reader and not intended to be
 * part of SVTK public API
 */

#ifndef svtkXdmf3ArrayKeeper_h
#define svtkXdmf3ArrayKeeper_h

#include "svtkIOXdmf3Module.h" // For export macro
#include <map>

class XdmfArray;

#ifdef _MSC_VER
#pragma warning(push)           // save
#pragma warning(disable : 4251) // needs to have dll-interface to be used by clients of class
#endif
class SVTKIOXDMF3_EXPORT svtkXdmf3ArrayKeeper : public std::map<XdmfArray*, unsigned int>
{
public:
  /**
   * Constructor
   */
  svtkXdmf3ArrayKeeper();

  /**
   * Destructor
   */
  ~svtkXdmf3ArrayKeeper();

  /**
   * Call to mark arrays that will be accessed with a new timestamp
   */
  void BumpGeneration();

  /**
   * Call whenever you a new XDMF array is accessed.
   */
  void Insert(XdmfArray* val);

  /**
   * Call to free all open arrays that are currently open but not in use.
   * Force argument frees all arrays.
   */
  void Release(bool force);

  svtkXdmf3ArrayKeeper(const svtkXdmf3ArrayKeeper&) = delete;

private:
  unsigned int generation;
};
#ifdef _MSC_VER
#pragma warning(pop) // restore
#endif

#endif // svtkXdmf3ArrayKeeper_h
// SVTK-HeaderTest-Exclude: svtkXdmf3ArrayKeeper.h
