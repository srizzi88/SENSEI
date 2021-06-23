/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPParticleTracer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPParticleTracer
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * svtkPParticleTracer is a filter that integrates a vector field to generate
 *
 *
 * @sa
 * svtkPParticleTracerBase has the details of the algorithms
 */

#ifndef svtkPParticleTracer_h
#define svtkPParticleTracer_h

#include "svtkPParticleTracerBase.h"
#include "svtkSmartPointer.h" // For protected ivars.

#include "svtkFiltersParallelFlowPathsModule.h" // For export macro

class SVTKFILTERSPARALLELFLOWPATHS_EXPORT svtkPParticleTracer : public svtkPParticleTracerBase
{
public:
  svtkTypeMacro(svtkPParticleTracer, svtkPParticleTracerBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPParticleTracer* New();

protected:
  svtkPParticleTracer();
  ~svtkPParticleTracer() {}
  virtual int OutputParticles(svtkPolyData* poly) override;

private:
  svtkPParticleTracer(const svtkPParticleTracer&) = delete;
  void operator=(const svtkPParticleTracer&) = delete;
};

#endif
