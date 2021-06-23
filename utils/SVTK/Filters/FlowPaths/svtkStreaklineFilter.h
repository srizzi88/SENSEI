/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStreaklineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkStreaklineFilter
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * svtkStreaklineFilter is a filter that integrates a vector field to generate streak lines
 *
 *
 * @sa
 * svtkParticleTracerBase has the details of the algorithms
 */

#ifndef svtkStreaklineFilter_h
#define svtkStreaklineFilter_h

#include "svtkFiltersFlowPathsModule.h" // For export macro
#include "svtkParticleTracerBase.h"
#include "svtkSmartPointer.h" // For protected ivars.

class SVTKFILTERSFLOWPATHS_EXPORT StreaklineFilterInternal
{
public:
  StreaklineFilterInternal()
    : Filter(nullptr)
  {
  }
  void Initialize(svtkParticleTracerBase* filter);
  virtual ~StreaklineFilterInternal() {}
  virtual int OutputParticles(svtkPolyData* poly);
  void Finalize();
  void Reset();

private:
  svtkParticleTracerBase* Filter;
};

class SVTKFILTERSFLOWPATHS_EXPORT svtkStreaklineFilter : public svtkParticleTracerBase
{
public:
  svtkTypeMacro(svtkStreaklineFilter, svtkParticleTracerBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkStreaklineFilter* New();

protected:
  svtkStreaklineFilter();
  ~svtkStreaklineFilter() override {}
  svtkStreaklineFilter(const svtkStreaklineFilter&) = delete;
  void operator=(const svtkStreaklineFilter&) = delete;
  int OutputParticles(svtkPolyData* poly) override;
  void Finalize() override;

  StreaklineFilterInternal It;
};

#endif
