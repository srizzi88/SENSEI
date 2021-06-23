/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOSPRayCache.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOSPRayCache
 * @brief   temporal cache ospray structures to speed flipbooks
 *
 * A temporal cache of templated objects that are created on the first
 * playthrough and reused afterward to speed up animations. Cache is
 * first come first serve. In other words the first 'Size' Set()
 * calls will succeed, later calls will be silently ignored. Decreasing
 * the size of the cache frees all previously held contents.
 *
 * This class is internal.
 */

#ifndef svtkOSPRayCache_h
#define svtkOSPRayCache_h

#include "svtkRenderingRayTracingModule.h" // For export macro
#include "svtkSystemIncludes.h"            //dll warning suppression
#include <map>                            // for stl
#include <memory>

#include "RTWrapper/RTWrapper.h" // for handle types

template <class T>
class SVTKRENDERINGRAYTRACING_EXPORT svtkOSPRayCache
{
public:
  svtkOSPRayCache() { this->Size = 0; }

  ~svtkOSPRayCache() { this->Empty(); }

  /**
   * Insert a new object into the cache.
   */
  void Set(double tstep, std::shared_ptr<T> payload)
  {
    if (this->Contents.size() >= this->Size)
    {
      return;
    }
    this->Contents[tstep] = payload;
  }

  /**
   * Obtain an object from the cache.
   * Return nullptr if none present at tstep.
   */
  std::shared_ptr<T> Get(double tstep)
  {
    auto ret = this->Contents.find(tstep);
    if (ret != this->Contents.end())
    {
      return ret->second;
    }
    return nullptr;
  }

  //@{
  /**
   * Set/Get the number of slots available in the cache.
   * Default is 0.
   */
  void SetSize(size_t sz)
  {
    if (sz == this->Size)
    {
      return;
    }
    if (sz < this->Size)
    {
      this->Empty();
    }
    this->Size = sz;
  }
  size_t GetSize() { return this->Size; }
  //@}

  /**
   * Query whether cache contains tstep
   */
  bool Contains(double tstep) { return this->Get(tstep) != nullptr; }

  /**
   * Check if the cache has space left.
   */
  bool HasRoom() { return this->Contents.size() < this->Size; }

private:
  // deletes all of the content in the cache
  void Empty()
  {
    this->Contents.clear();
    this->Size = 0;
  }

  size_t Size;

  std::map<double, std::shared_ptr<T> > Contents;
};

class svtkOSPRayCacheItemObject
{
public:
  svtkOSPRayCacheItemObject(RTW::Backend* be, OSPObject obj)
    : backend(be)
  {
    object = obj;
  }
  ~svtkOSPRayCacheItemObject() { ospRelease(object); }
  OSPObject object{ nullptr };
  size_t size{ 0 };
  RTW::Backend* backend = nullptr;
};

#endif // svtkOSPRayCache_h
// SVTK-HeaderTest-Exclude: svtkOSPRayCache.h
