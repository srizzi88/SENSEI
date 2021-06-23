/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPSphereSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPSphereSource
 * @brief   sphere source that supports pieces
 */

#ifndef svtkPSphereSource_h
#define svtkPSphereSource_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkSphereSource.h"

class SVTKFILTERSPARALLEL_EXPORT svtkPSphereSource : public svtkSphereSource
{
public:
  svtkTypeMacro(svtkPSphereSource, svtkSphereSource);

  //@{
  /**
   * Construct sphere with radius=0.5 and default resolution 8 in both Phi
   * and Theta directions. Theta ranges from (0,360) and phi (0,180) degrees.
   */
  static svtkPSphereSource* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * Get the estimated memory size in kibibytes (1024 bytes).
   */
  unsigned long GetEstimatedMemorySize();

protected:
  svtkPSphereSource() {}
  ~svtkPSphereSource() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkPSphereSource(const svtkPSphereSource&) = delete;
  void operator=(const svtkPSphereSource&) = delete;
};

#endif
