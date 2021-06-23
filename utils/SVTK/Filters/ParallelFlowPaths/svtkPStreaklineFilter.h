/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPStreaklineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPStreaklineFilter
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * svtkPStreaklineFilter is a filter that integrates a vector field to generate
 *
 *
 * @sa
 * svtkPStreaklineFilterBase has the details of the algorithms
 */

#ifndef svtkPStreaklineFilter_h
#define svtkPStreaklineFilter_h

#include "svtkFiltersParallelFlowPathsModule.h" // For export macro
#include "svtkPParticleTracerBase.h"
#include "svtkSmartPointer.h"     // For protected ivars.
#include "svtkStreaklineFilter.h" //for utility

class SVTKFILTERSPARALLELFLOWPATHS_EXPORT svtkPStreaklineFilter : public svtkPParticleTracerBase
{
public:
  svtkTypeMacro(svtkPStreaklineFilter, svtkPParticleTracerBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPStreaklineFilter* New();

protected:
  svtkPStreaklineFilter();
  ~svtkPStreaklineFilter() {}
  svtkPStreaklineFilter(const svtkPStreaklineFilter&) = delete;
  void operator=(const svtkPStreaklineFilter&) = delete;
  virtual int OutputParticles(svtkPolyData* poly) override;
  virtual void Finalize() override;

  StreaklineFilterInternal It;
};

#endif
