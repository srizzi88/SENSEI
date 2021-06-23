/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalStreamTracer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTemporalStreamTracer
 * @brief   A Parallel Particle tracer for unsteady vector fields
 *
 * svtkTemporalStreamTracer is a filter that integrates a vector field to generate
 *
 *
 * @sa
 * svtkRibbonFilter svtkRuledSurfaceFilter svtkInitialValueProblemSolver
 * svtkRungeKutta2 svtkRungeKutta4 svtkRungeKutta45 svtkStreamTracer
 *
 * This class is deprecated.
 * Use instead one of the following classes: svtkParticleTracerBase
 * svtkParticleTracer svtkParticlePathFilter svtkStreaklineFilter
 * See https://blog.kitware.com/improvements-in-path-tracing-in-svtk/
 */

#ifndef svtkPTemporalStreamTracer_h
#define svtkPTemporalStreamTracer_h

#include "svtkConfigure.h" // For legacy defines
#include "svtkSetGet.h"    // For legacy macros
#ifndef SVTK_LEGACY_REMOVE

#include "svtkFiltersParallelFlowPathsModule.h" // For export macro
#include "svtkSmartPointer.h"                   // For protected ivars.
#include "svtkTemporalStreamTracer.h"

#include <list>   // STL Header
#include <vector> // STL Header

class svtkMultiProcessController;

class svtkMultiBlockDataSet;
class svtkDataArray;
class svtkDoubleArray;
class svtkGenericCell;
class svtkIntArray;
class svtkTemporalInterpolatedVelocityField;
class svtkPoints;
class svtkCellArray;
class svtkDoubleArray;
class svtkFloatArray;
class svtkIntArray;
class svtkCharArray;
class svtkAbstractParticleWriter;

class SVTKFILTERSPARALLELFLOWPATHS_EXPORT svtkPTemporalStreamTracer : public svtkTemporalStreamTracer
{
public:
  svtkTypeMacro(svtkPTemporalStreamTracer, svtkTemporalStreamTracer);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object using 2nd order Runge Kutta
   */
  static svtkPTemporalStreamTracer* New();

  //@{
  /**
   * Set/Get the controller used when sending particles between processes
   * The controller must be an instance of svtkMPIController.
   */
  virtual void SetController(svtkMultiProcessController* controller);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  SVTK_LEGACY(svtkPTemporalStreamTracer());
  ~svtkPTemporalStreamTracer();

  //
  // Generate output
  //
  virtual int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  //

  //

  /**
   * all the injection/seed points according to which processor
   * they belong to. This saves us retesting at every injection time
   * providing 1) The volumes are static, 2) the seed points are static
   * If either are non static, then this step is skipped.
   */
  virtual void AssignSeedsToProcessors(svtkDataSet* source, int sourceID, int ptId,
    svtkTemporalStreamTracerNamespace::ParticleVector& LocalSeedPoints,
    int& LocalAssignedCount) override;

  /**
   * give each one a unique ID. We need to use MPI to find out
   * who is using which numbers.
   */
  virtual void AssignUniqueIds(
    svtkTemporalStreamTracerNamespace::ParticleVector& LocalSeedPoints) override;

  /**
   * this is used during classification of seed points and also between iterations
   * of the main loop as particles leave each processor domain
   */
  virtual void TransmitReceiveParticles(
    svtkTemporalStreamTracerNamespace::ParticleVector& outofdomain,
    svtkTemporalStreamTracerNamespace::ParticleVector& received, bool removeself) override;

  void AddParticleToMPISendList(svtkTemporalStreamTracerNamespace::ParticleInformation& info);

  //

  //

  // MPI controller needed when running in parallel
  svtkMultiProcessController* Controller;

private:
  svtkPTemporalStreamTracer(const svtkPTemporalStreamTracer&) = delete;
  void operator=(const svtkPTemporalStreamTracer&) = delete;
};

#endif // SVTK_LEGACY_REMOVE

#endif
