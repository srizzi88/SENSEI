/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParticlePathFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParticlePathFilter
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * svtkParticlePathFilter is a filter that integrates a vector field to generate particle paths
 *
 *
 * @sa
 * svtkParticlePathFilterBase has the details of the algorithms
 */

#ifndef svtkParticlePathFilter_h
#define svtkParticlePathFilter_h

#include "svtkFiltersFlowPathsModule.h" // For export macro
#include "svtkParticleTracerBase.h"
#include "svtkSmartPointer.h" // For protected ivars.
#include <vector>            // For protected ivars

class SVTKFILTERSFLOWPATHS_EXPORT ParticlePathFilterInternal
{
public:
  ParticlePathFilterInternal()
    : Filter(nullptr)
  {
  }
  void Initialize(svtkParticleTracerBase* filter);
  virtual ~ParticlePathFilterInternal() {}
  virtual int OutputParticles(svtkPolyData* poly);
  void SetClearCache(bool clearCache) { this->ClearCache = clearCache; }
  bool GetClearCache() { return this->ClearCache; }
  void Finalize();
  void Reset();

private:
  svtkParticleTracerBase* Filter;
  // Paths doesn't seem to work properly. it is meant to make connecting lines
  // for the particles
  std::vector<svtkSmartPointer<svtkIdList> > Paths;
  bool ClearCache; // false by default
};

class SVTKFILTERSFLOWPATHS_EXPORT svtkParticlePathFilter : public svtkParticleTracerBase
{
public:
  svtkTypeMacro(svtkParticlePathFilter, svtkParticleTracerBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkParticlePathFilter* New();

protected:
  svtkParticlePathFilter();
  ~svtkParticlePathFilter() override;
  svtkParticlePathFilter(const svtkParticlePathFilter&) = delete;
  void operator=(const svtkParticlePathFilter&) = delete;

  void ResetCache() override;
  int OutputParticles(svtkPolyData* poly) override;
  void InitializeExtraPointDataArrays(svtkPointData* outputPD) override;
  void AppendToExtraPointDataArrays(svtkParticleTracerBaseNamespace::ParticleInformation&) override;

  void Finalize() override;

  //
  // Store any information we need in the output and fetch what we can
  // from the input
  //
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  ParticlePathFilterInternal It;

private:
  svtkDoubleArray* SimulationTime;
  svtkIntArray* SimulationTimeStep;
};

#endif
