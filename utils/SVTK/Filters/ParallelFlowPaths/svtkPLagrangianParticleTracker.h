/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPLagrangianParticleTracker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPLagrangianParticleTracker
 * @brief    parallel Lagrangian particle tracker
 *
 * This class implements parallel Lagrangian particle tracker.
 * The implementation is as follows:
 * First seeds input is parsed to create particle in each rank
 * Particles which are not contained by the flow in a rank are sent to other ranks
 * which can potentially contain it and will grab only if they actually contain it
 * Then each rank begin integrating.
 * When a particle goes out of domain, the particle will be sent to other ranks
 * the same way.
 * When a rank runs out of particle, it waits for other potential particles
 * from other ranks.
 * When all ranks run out of particles, integration is over.
 * The master rank takes care of communications between rank regarding integration termination
 * particles are directly streamed rank to rank, without going through the master
 *
 * @sa
 * svtkStreamTracer
 */

#ifndef svtkPLagrangianParticleTracker_h
#define svtkPLagrangianParticleTracker_h

#include "svtkFiltersParallelFlowPathsModule.h" // For export macro
#include "svtkLagrangianParticleTracker.h"
#include "svtkNew.h" // for ivars

class MasterFlagManager;
class ParticleStreamManager;
class RankFlagManager;
class svtkMPIController;
class svtkMultiBlockDataSet;
class svtkUnstructuredGrid;

class SVTKFILTERSPARALLELFLOWPATHS_EXPORT svtkPLagrangianParticleTracker
  : public svtkLagrangianParticleTracker
{
public:
  svtkTypeMacro(svtkPLagrangianParticleTracker, svtkLagrangianParticleTracker);
  virtual void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkPLagrangianParticleTracker* New();

protected:
  svtkPLagrangianParticleTracker();
  ~svtkPLagrangianParticleTracker() override;

  virtual int RequestUpdateExtent(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void GenerateParticles(const svtkBoundingBox* bounds, svtkDataSet* seeds,
    svtkDataArray* initialVelocities, svtkDataArray* initialIntegrationTimes, svtkPointData* seedData,
    int nVar, std::queue<svtkLagrangianParticle*>& particles) override;

  /**
   * Flags description :
   * Worker flag working : the worker has at least one particle in it's queue and
      is currently integrating it.
   * Worker flag empty : the worker has no more particle in it's queue and is
       activelly waiting for more particle to integrate from other ranks.
   * Worker flag finished : the worker has received a master empty flag and after
       checking one last time, still doesn't have any particle to integrate. It is
       now just waiting for master to send the master finished flag.
   * Master flag working : there is at least one worker or the master that have one
       or more particle to integrate.
   * Master flag empty : all ranks, including master, have no more particles to integrate
   * Master flag finished : all workers ranks have sent the worker flag finished
   */
  virtual void GetParticleFeed(std::queue<svtkLagrangianParticle*>& particleQueue) override;
  virtual int Integrate(svtkInitialValueProblemSolver* integrator, svtkLagrangianParticle*,
    std::queue<svtkLagrangianParticle*>& particleQueue, svtkPolyData* particlePathsOutput,
    svtkPolyLine* particlePath, svtkDataObject* interactionOutput) override;

  //@{
  /**
   * Non threadsafe methods to send and receive particles
   */
  void SendParticle(svtkLagrangianParticle* particle);
  void ReceiveParticles(std::queue<svtkLagrangianParticle*>& particleQueue);
  //@}

  bool FinalizeOutputs(svtkPolyData* particlePathsOutput, svtkDataObject* interactionOutput) override;

  bool UpdateSurfaceCacheIfNeeded(svtkDataObject*& surfaces) override;

  /**
   * Get an unique id for a particle
   * This method is thread safe
   */
  virtual svtkIdType GetNewParticleId() override;

  /**
   * Get the complete number of created particles
   */
  svtkGetMacro(ParticleCounter, svtkIdType);

  svtkNew<svtkUnstructuredGrid> TmpSurfaceInput;
  svtkNew<svtkMultiBlockDataSet> TmpSurfaceInputMB;
  svtkMPIController* Controller;
  ParticleStreamManager* StreamManager;
  MasterFlagManager* MFlagManager;
  RankFlagManager* RFlagManager;

  std::mutex StreamManagerMutex;

private:
  svtkPLagrangianParticleTracker(const svtkPLagrangianParticleTracker&) = delete;
  void operator=(const svtkPLagrangianParticleTracker&) = delete;
};
#endif
