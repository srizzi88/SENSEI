/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPReflectionFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPReflectionFilter
 * @brief   parallel version of svtkReflectionFilter
 *
 * svtkPReflectionFilter is a parallel version of svtkReflectionFilter which takes
 * into consideration the full dataset bounds for performing the reflection.
 */

#ifndef svtkPReflectionFilter_h
#define svtkPReflectionFilter_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkReflectionFilter.h"

class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkPReflectionFilter : public svtkReflectionFilter
{
public:
  static svtkPReflectionFilter* New();
  svtkTypeMacro(svtkPReflectionFilter, svtkReflectionFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the parallel controller.
   */
  void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkPReflectionFilter();
  ~svtkPReflectionFilter() override;

  /**
   * Internal method to compute bounds.
   */
  int ComputeBounds(svtkDataObject* input, double bounds[6]) override;

  svtkMultiProcessController* Controller;

private:
  svtkPReflectionFilter(const svtkPReflectionFilter&) = delete;
  void operator=(const svtkPReflectionFilter&) = delete;
};

#endif
