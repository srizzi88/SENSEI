/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGPUInfo.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkGPUInfo
 * @brief   Stores GPU VRAM information.
 *
 * svtkGPUInfo stores information about GPU Video RAM. An host can have
 * several GPUs. The values are set by svtkGPUInfoList.
 * @sa
 * svtkGPUInfoList svtkDirectXGPUInfoList svtkCoreGraphicsGPUInfoList
 */

#ifndef svtkGPUInfo_h
#define svtkGPUInfo_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkGPUInfo : public svtkObject
{
public:
  static svtkGPUInfo* New();
  svtkTypeMacro(svtkGPUInfo, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get dedicated video memory in bytes. Initial value is 0.
   * Usually the fastest one. If it is not 0, it should be taken into
   * account first and DedicatedSystemMemory or SharedSystemMemory should be
   * ignored.
   */
  svtkSetMacro(DedicatedVideoMemory, svtkTypeUInt64);
  svtkGetMacro(DedicatedVideoMemory, svtkTypeUInt64);
  //@}

  //@{
  /**
   * Set/Get dedicated system memory in bytes. Initial value is 0.
   * This is slow memory. If it is not 0, this value should be taken into
   * account only if there is no DedicatedVideoMemory and SharedSystemMemory
   * should be ignored.
   */
  svtkSetMacro(DedicatedSystemMemory, svtkTypeUInt64);
  svtkGetMacro(DedicatedSystemMemory, svtkTypeUInt64);
  //@}

  //@{
  /**
   * Set/Get shared system memory in bytes. Initial value is 0.
   * Slowest memory. This value should be taken into account only if there is
   * neither DedicatedVideoMemory nor DedicatedSystemMemory.
   */
  svtkSetMacro(SharedSystemMemory, svtkTypeUInt64);
  svtkGetMacro(SharedSystemMemory, svtkTypeUInt64);
  //@}

protected:
  svtkGPUInfo();
  ~svtkGPUInfo() override;

  svtkTypeUInt64 DedicatedVideoMemory;
  svtkTypeUInt64 DedicatedSystemMemory;
  svtkTypeUInt64 SharedSystemMemory;

private:
  svtkGPUInfo(const svtkGPUInfo&) = delete;
  void operator=(const svtkGPUInfo&) = delete;
};

#endif
