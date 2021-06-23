/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParticleTracerBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParticleTracerBase
 * @brief   A parallel particle tracer for vector fields
 *
 * svtkPParticleTracerBase is the base class for parallel filters that advect particles
 * in a vector field. Note that the input svtkPointData structure must
 * be identical on all datasets.
 * @sa
 * svtkRibbonFilter svtkRuledSurfaceFilter svtkInitialValueProblemSolver
 * svtkRungeKutta2 svtkRungeKutta4 svtkRungeKutta45 svtkStreamTracer
 */

#ifndef svtkPParticleTracerBase_h
#define svtkPParticleTracerBase_h

#include "svtkParticleTracerBase.h"
#include "svtkSmartPointer.h" // For protected ivars.

#include <vector> // STL Header

#include "svtkFiltersParallelFlowPathsModule.h" // For export macro

class SVTKFILTERSPARALLELFLOWPATHS_EXPORT svtkPParticleTracerBase : public svtkParticleTracerBase
{
public:
  svtkTypeMacro(svtkPParticleTracerBase, svtkParticleTracerBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the controller used when sending particles between processes
   * The controller must be an instance of svtkMPIController.
   */
  virtual void SetController(svtkMultiProcessController* controller);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  struct RemoteParticleInfo
  {
    svtkParticleTracerBaseNamespace::ParticleInformation Current;
    svtkParticleTracerBaseNamespace::ParticleInformation Previous;
    svtkSmartPointer<svtkPointData> PreviousPD;
  };

  typedef std::vector<RemoteParticleInfo> RemoteParticleVector;

  svtkPParticleTracerBase();
  ~svtkPParticleTracerBase();

  virtual int RequestUpdateExtent(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  //

  virtual svtkPolyData* Execute(svtkInformationVector** inputVector) override;
  virtual bool SendParticleToAnotherProcess(
    svtkParticleTracerBaseNamespace::ParticleInformation& info,
    svtkParticleTracerBaseNamespace::ParticleInformation& previous, svtkPointData*) override;

  /**
   * Before starting the particle trace, classify
   * all the injection/seed points according to which processor
   * they belong to. This saves us retesting at every injection time
   * providing 1) The volumes are static, 2) the seed points are static
   * If either are non static, then this step is skipped.
   */
  virtual void AssignSeedsToProcessors(double time, svtkDataSet* source, int sourceID, int ptId,
    svtkParticleTracerBaseNamespace::ParticleVector& localSeedPoints,
    int& localAssignedCount) override;

  /**
   * give each one a unique ID. We need to use MPI to find out
   * who is using which numbers.
   */
  virtual void AssignUniqueIds(
    svtkParticleTracerBaseNamespace::ParticleVector& localSeedPoints) override;

  /**
   * this is used during classification of seed points and also between iterations
   * of the main loop as particles leave each processor domain. Returns
   * true if particles were migrated to any new process.
   */
  virtual bool SendReceiveParticles(
    RemoteParticleVector& outofdomain, RemoteParticleVector& received);

  virtual bool UpdateParticleListFromOtherProcesses() override;

  /**
   * Method that checks that the input arrays are ordered the
   * same on all data sets. This needs to be true for all
   * blocks in a composite data set as well as across all processes.
   */
  virtual bool IsPointDataValid(svtkDataObject* input) override;

  //

  //

  // MPI controller needed when running in parallel
  svtkMultiProcessController* Controller;

  // List used for transmitting between processors during parallel operation
  RemoteParticleVector MPISendList;

  RemoteParticleVector Tail; // this is to receive the "tails" of traces from other processes
private:
  svtkPParticleTracerBase(const svtkPParticleTracerBase&) = delete;
  void operator=(const svtkPParticleTracerBase&) = delete;
};
#endif
