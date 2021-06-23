/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPParticlePathFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPParticlePathFilter
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * svtkPParticlePathFilter is a filter that integrates a vector field to generate
 * path lines.
 *
 * @sa
 * svtkPParticlePathFilterBase has the details of the algorithms
 */

#ifndef svtkPParticlePathFilter_h
#define svtkPParticlePathFilter_h

#include "svtkPParticleTracerBase.h"
#include "svtkParticlePathFilter.h" //for utility

#include "svtkFiltersParallelFlowPathsModule.h" // For export macro
class SVTKFILTERSPARALLELFLOWPATHS_EXPORT svtkPParticlePathFilter : public svtkPParticleTracerBase
{
public:
  svtkTypeMacro(svtkPParticlePathFilter, svtkPParticleTracerBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkPParticlePathFilter* New();

protected:
  svtkPParticlePathFilter();
  ~svtkPParticlePathFilter() override;

  virtual void ResetCache() override;
  virtual int OutputParticles(svtkPolyData* poly) override;
  virtual void InitializeExtraPointDataArrays(svtkPointData* outputPD) override;
  virtual void AppendToExtraPointDataArrays(
    svtkParticleTracerBaseNamespace::ParticleInformation&) override;
  void Finalize() override;

  //
  // Store any information we need in the output and fetch what we can
  // from the input
  //
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  ParticlePathFilterInternal It;
  svtkDoubleArray* SimulationTime;
  svtkIntArray* SimulationTimeStep;

private:
  svtkPParticlePathFilter(const svtkPParticlePathFilter&) = delete;
  void operator=(const svtkPParticlePathFilter&) = delete;
};
#endif
