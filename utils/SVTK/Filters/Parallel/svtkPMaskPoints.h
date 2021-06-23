/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPMaskPoints.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPMaskPoints
 * @brief   parallel Mask Points
 *
 * The difference between this implementation and svtkMaskPoints is
 * the use of the svtkMultiProcessController and that
 * ProportionalMaximumNumberOfPoints is obeyed.
 */

#ifndef svtkPMaskPoints_h
#define svtkPMaskPoints_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkMaskPoints.h"

class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkPMaskPoints : public svtkMaskPoints
{
public:
  static svtkPMaskPoints* New();
  svtkTypeMacro(svtkPMaskPoints, svtkMaskPoints);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the communicator object for interprocess communication
   */
  virtual svtkMultiProcessController* GetController();
  virtual void SetController(svtkMultiProcessController*);
  //@}

protected:
  svtkPMaskPoints();
  ~svtkPMaskPoints() override;

  void InternalScatter(unsigned long*, unsigned long*, int, int) override;
  void InternalGather(unsigned long*, unsigned long*, int, int) override;
  int InternalGetNumberOfProcesses() override;
  int InternalGetLocalProcessId() override;
  void InternalBarrier() override;
  void InternalSplitController(int color, int key) override;
  void InternalResetController() override;

  svtkMultiProcessController* Controller;
  svtkMultiProcessController* OriginalController;

private:
  svtkPMaskPoints(const svtkPMaskPoints&) = delete;
  void operator=(const svtkPMaskPoints&) = delete;
};

#endif
