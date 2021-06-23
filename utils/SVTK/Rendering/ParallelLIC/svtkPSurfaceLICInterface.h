/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPSurfaceLICInterface.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPSurfaceLICInterface
 * @brief   parallel parts of the svtkSurfaceLICInterface
 *
 *
 * Parallel parts of the svtkSurfaceLICInterface, see that class for
 * documentation.
 */

#ifndef svtkPSurfaceLICInterface_h
#define svtkPSurfaceLICInterface_h

#include "svtkRenderingParallelLICModule.h" // For export macro
#include "svtkSurfaceLICInterface.h"
#include <string> // for string

class svtkPainterCommunicator;

class SVTKRENDERINGPARALLELLIC_EXPORT svtkPSurfaceLICInterface : public svtkSurfaceLICInterface
{
public:
  static svtkPSurfaceLICInterface* New();
  svtkTypeMacro(svtkPSurfaceLICInterface, svtkSurfaceLICInterface);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Methods used for parallel benchmarks. Use cmake to define
   * svtkSurfaceLICInterfaceTIME to enable benchmarks. During each
   * update timing information is stored, it can be written to
   * disk by calling WriteLog.
   */
  virtual void WriteTimerLog(const char* fileName) override;

protected:
  svtkPSurfaceLICInterface();
  ~svtkPSurfaceLICInterface();

  /**
   * Get the min/max across all ranks. min/max are in/out.
   * In serial operation this is a no-op, in parallel it
   * is a global collective reduction.
   */
  virtual void GetGlobalMinMax(svtkPainterCommunicator* comm, float& min, float& max) override;

  /**
   * Creates a new communicator with/without the calling processes
   * as indicated by the passed in flag, if not 0 the calling process
   * is included in the new communicator. In parallel this call is mpi
   * collective on the world communicator. In serial this is a no-op.
   */
  virtual svtkPainterCommunicator* CreateCommunicator(int include) override;

  /**
   * Ensure that if any rank updates the communicator they all
   * do. This is a global collective operation.
   */
  virtual bool NeedToUpdateCommunicator() override;

  //@{
  /**
   * Methods used for parallel benchmarks. Use cmake to define
   * svtkSurfaceLICInterfaceTIME to enable benchmarks. During each
   * update timing information is stored, it can be written to
   * disk by calling WriteLog.
   */
  virtual void StartTimerEvent(const char* name);
  virtual void EndTimerEvent(const char* name);
  //@}

private:
  std::string LogFileName;

private:
  svtkPSurfaceLICInterface(const svtkPSurfaceLICInterface&) = delete;
  void operator=(const svtkPSurfaceLICInterface&) = delete;
};

#endif
