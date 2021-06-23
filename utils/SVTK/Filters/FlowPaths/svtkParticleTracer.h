/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParticleTracer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParticleTracer
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * svtkParticleTracer is a filter that integrates a vector field to advect particles
 *
 *
 * @sa
 * svtkParticleTracerBase has the details of the algorithms
 */

#ifndef svtkParticleTracer_h
#define svtkParticleTracer_h

#include "svtkFiltersFlowPathsModule.h" // For export macro
#include "svtkParticleTracerBase.h"
#include "svtkSmartPointer.h" // For protected ivars.

class SVTKFILTERSFLOWPATHS_EXPORT svtkParticleTracer : public svtkParticleTracerBase
{
public:
  svtkTypeMacro(svtkParticleTracer, svtkParticleTracerBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkParticleTracer* New();

protected:
  svtkParticleTracer();
  ~svtkParticleTracer() override {}
  svtkParticleTracer(const svtkParticleTracer&) = delete;
  void operator=(const svtkParticleTracer&) = delete;
  int OutputParticles(svtkPolyData* poly) override;
};

#endif
