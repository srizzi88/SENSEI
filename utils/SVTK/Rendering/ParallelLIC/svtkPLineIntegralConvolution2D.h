/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPLineIntegralConvolution2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPLineIntegralConvolution2D
 * @brief   parallel part of GPU-based
 * implementation of Line Integral Convolution (LIC)
 *
 *
 * Implements the parallel parts of the algorithm.
 *
 * @sa
 *  svtkPLineIntegralConvolution2D
 */

#ifndef svtkPLineIntegralConvolution2D_h
#define svtkPLineIntegralConvolution2D_h

#include "svtkLineIntegralConvolution2D.h"
#include "svtkRenderingParallelLICModule.h" // for export macro
#include <string>                          // for string

class svtkPainterCommunicator;
class svtkPPainterCommunicator;

class SVTKRENDERINGPARALLELLIC_EXPORT svtkPLineIntegralConvolution2D
  : public svtkLineIntegralConvolution2D
{
public:
  static svtkPLineIntegralConvolution2D* New();
  svtkTypeMacro(svtkPLineIntegralConvolution2D, svtkLineIntegralConvolution2D);
  virtual void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the communicator to use during parallel operation
   * The communicator will not be duplicated or reference
   * counted for performance reasons thus caller should
   * hold/manage reference to the communicator during use
   * of the LIC object.
   */
  virtual void SetCommunicator(svtkPainterCommunicator*) override;
  virtual svtkPainterCommunicator* GetCommunicator() override;
  //@}

  /**
   * For parallel operation, find global min/max
   * min/max are in/out.
   */
  virtual void GetGlobalMinMax(svtkPainterCommunicator* comm, float& min, float& max) override;

  /**
   * Methods used for parallel benchmarks. Use cmake to define
   * svtkLineIntegralConviolution2DTIME to enable benchmarks.
   * During each update timing information is stored, it can
   * be written to disk by calling WriteLog.
   */
  virtual void WriteTimerLog(const char* fileName) override;

protected:
  svtkPLineIntegralConvolution2D();
  ~svtkPLineIntegralConvolution2D() override;

  //@{
  /**
   * Methods used for parallel benchmarks. Use cmake to define
   * svtkSurfaceLICPainterTIME to enable benchmarks. During each
   * update timing information is stored, it can be written to
   * disk by calling WriteLog. Note: Some of the timings are
   * enabled by the surface lic painter.
   */
  virtual void StartTimerEvent(const char* name) override;
  virtual void EndTimerEvent(const char* name) override;
  //@}

private:
  std::string LogFileName;

private:
  svtkPLineIntegralConvolution2D(const svtkPLineIntegralConvolution2D&) = delete;
  void operator=(const svtkPLineIntegralConvolution2D&) = delete;
};

#endif
