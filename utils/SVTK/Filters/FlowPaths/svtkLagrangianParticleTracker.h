/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLagrangianParticleTracker.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLagrangianParticleTracker
 * @brief   Filter to inject and track particles in a flow
 *
 *
 *
 * Introduce LagrangianParticleTracker
 *
 * This is a very flexible and adaptive filter to inject and track particles in a
 * flow. It takes three inputs :
 * * port 0 : Flow Input, a volumic dataset containing data to integrate with,
 *     any kind of data object, support distributed input.
 * * port 1 : Seed (source) Input, a dataset containing point to generate particles
 *     with, any kind of data object, support distributed input. Only first leaf
 *     of composite dataset is used.
 * * port 2 : Optional Surface Input, containing dataset to interact with, any
 *     kind of data object, support distributed input.
 *
 * It has two outputs :
 * * port 0 : ParticlePaths : a multipiece of polyData (one per thread) of polyLines showing the
 * paths of particles in the flow
 * * port 1 : ParticleInteractions : empty if no surface input, contains a
 *     a multiblock with as many children as the number of threads, each children containing a
 * multiblock with the same structure as the surfaces. The leafs of these structures contain a
 * polydata of vertexes corresponding to the interactions. with the same composite layout of surface
 * input if any, showing all interactions between particles and the surface input.
 *
 * It has a parallel implementation which streams particle between domains.
 *
 * The most important parameters of this filter is it's integrationModel.
 * Only one integration model implementation exist currently in ParaView
 * ,svtkLagrangianMatidaIntegrationModel but the design enables plugin developers
 * to expand this tracker by creating new models.
 * A model can define  :
 * * The number of integration variable and new user defined integration variable
 * * the way the particle are integrated
 * * the way particles intersect and interact with the surface
 * * the way freeFlight termination is handled
 * * PreProcess and PostProcess methods
 * * Manual Integration, Manual partichle shifting
 * see svtkLagrangianBasicIntegrationModel and svtkLagrangianMatidaIntegrationModel
 * for more information
 *
 * It also let the user choose the Locator to use when integrating in the flow,
 * as well as the Integrator to use. Integration steps are also highly configurable,
 * step, step min and step max are passed down to the integrator (hence min and max
 * does not matter with a non adaptive integrator like RK4/5)
 *
 *  An IntegrationModel is a very specific svtkFunctionSet with a lot of features
 * allowing inherited classes
 * to concentrate on the mathematical part of the code.
 *  a Particle is basically a class wrapper around three table containing variables
 * about the particle at previous, current and next position.
 *  The particle is passed to the integrator, which use the integration model to
 * integrate the particle in the flow.
 *
 * It has other features also, including :
 *  * Adaptative Step Reintegration, to retry the step with different time step
 *      when the next position is too far
 *  * Different kind of cell length computation, including a divergence theorem
 *      based computation
 *  * Optional lines rendering controlled by a threshold
 *  * Ghost cell support
 *  * Non planar quad interaction support
 *  * Built-in support for surface interaction including, terminate, bounce,
 *      break-up and pass-through surface
 * The serial and parallel filters are fully tested.
 *
 * @sa
 * svtkLagrangianMatidaIntegrationModel svtkLagrangianParticle
 * svtkLagrangianBasicIntegrationModel
 */

#ifndef svtkLagrangianParticleTracker_h
#define svtkLagrangianParticleTracker_h

#include "svtkBoundingBox.h" // For cached bounds
#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersFlowPathsModule.h" // For export macro

#include <atomic> // for atomic
#include <mutex>  // for mutexes
#include <queue>  // for particle queue

class svtkBoundingBox;
class svtkCellArray;
class svtkDataSet;
class svtkDoubleArray;
class svtkIdList;
class svtkInformation;
class svtkInitialValueProblemSolver;
class svtkLagrangianBasicIntegrationModel;
class svtkLagrangianParticle;
class svtkMultiBlockDataSet;
class svtkMultiPieceDataSet;
class svtkPointData;
class svtkPoints;
class svtkPolyData;
class svtkPolyLine;
struct IntegratingFunctor;

class SVTKFILTERSFLOWPATHS_EXPORT svtkLagrangianParticleTracker : public svtkDataObjectAlgorithm
{
public:
  svtkTypeMacro(svtkLagrangianParticleTracker, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkLagrangianParticleTracker* New();

  typedef enum CellLengthComputation
  {
    STEP_LAST_CELL_LENGTH = 0,
    STEP_CUR_CELL_LENGTH = 1,
    STEP_LAST_CELL_VEL_DIR = 2,
    STEP_CUR_CELL_VEL_DIR = 3,
    STEP_LAST_CELL_DIV_THEO = 4,
    STEP_CUR_CELL_DIV_THEO = 5
  } CellLengthComputation;

  //@{
  /**
   * Set/Get the integration model.
   * Default is svtkLagrangianMatidaIntegrationModel
   */
  void SetIntegrationModel(svtkLagrangianBasicIntegrationModel* integrationModel);
  svtkGetObjectMacro(IntegrationModel, svtkLagrangianBasicIntegrationModel);
  //@}

  //@{
  /**
   * Set/Get the integrator.
   * Default is svtkRungeKutta2
   */
  void SetIntegrator(svtkInitialValueProblemSolver* integrator);
  svtkGetObjectMacro(Integrator, svtkInitialValueProblemSolver);
  //@}

  //@{
  /**
   * Set/Get whether or not to use PolyVertex cell type
   * for the interaction output
   * Default is false
   */
  svtkSetMacro(GeneratePolyVertexInteractionOutput, bool);
  svtkGetMacro(GeneratePolyVertexInteractionOutput, bool);
  //@}

  //@{
  /**
   * Set/Get the cell length computation mode.
   * Available modes are :
   * - STEP_LAST_CELL_LENGTH :
   * Compute cell length using getLength method
   * on the last cell the particle was in
   * - STEP_CUR_CELL_LENGTH :
   * Compute cell length using getLength method
   * on the current cell the particle is in
   * - STEP_LAST_CELL_VEL_DIR :
   * Compute cell length using the particle velocity
   * and the edges of the last cell the particle was in.
   * - STEP_CUR_CELL_VEL_DIR :
   * Compute cell length using the particle velocity
   * and the edges of the last cell the particle was in.
   * - STEP_LAST_CELL_DIV_THEO :
   * Compute cell length using the particle velocity
   * and the divergence theorem.
   * - STEP_CUR_CELL_DIV_THEO :
   * Compute cell length using the particle velocity
   * and the divergence theorem.
   * Default is STEP_LAST_CELL_LENGTH.
   */
  svtkSetMacro(CellLengthComputationMode, int);
  svtkGetMacro(CellLengthComputationMode, int);
  //@}

  //@{
  /**
   * Set/Get the integration step factor. Default is 1.0.
   */
  svtkSetMacro(StepFactor, double);
  svtkGetMacro(StepFactor, double);
  //@}

  //@{
  /**
   * Set/Get the integration step factor min. Default is 0.5.
   */
  svtkSetMacro(StepFactorMin, double);
  svtkGetMacro(StepFactorMin, double);
  //@}

  //@{
  /**
   * Set/Get the integration step factor max. Default is 1.5.
   */
  svtkSetMacro(StepFactorMax, double);
  svtkGetMacro(StepFactorMax, double);
  //@}

  //@{
  /**
   * Set/Get the maximum number of steps. -1 means no limit. Default is 100.
   */
  svtkSetMacro(MaximumNumberOfSteps, int);
  svtkGetMacro(MaximumNumberOfSteps, int);
  //@}

  //@{
  /**
   * Set/Get the maximum integration time. A negative value means no limit.
   * Default is -1.
   */
  svtkSetMacro(MaximumIntegrationTime, double);
  svtkGetMacro(MaximumIntegrationTime, double);
  //@}

  //@{
  /**
   * Set/Get the Adaptive Step Reintegration feature.
   * it checks the step size after the integration
   * and if it is too big will retry with a smaller step
   * Default is false.
   */
  svtkSetMacro(AdaptiveStepReintegration, bool);
  svtkGetMacro(AdaptiveStepReintegration, bool);
  svtkBooleanMacro(AdaptiveStepReintegration, bool);
  //@}

  //@{
  /**
   * Set/Get the generation of the particle path output,
   * Default is true.
   */
  svtkSetMacro(GenerateParticlePathsOutput, bool);
  svtkGetMacro(GenerateParticlePathsOutput, bool);
  svtkBooleanMacro(GenerateParticlePathsOutput, bool);
  //@}

  //@{
  /**
   * Specify the source object used to generate particle initial position (seeds).
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSourceConnection for connecting the pipeline.
   */
  void SetSourceData(svtkDataObject* source);
  svtkDataObject* GetSource();
  //@}

  /**
   * Specify the source object used to generate particle initial position (seeds).
   */
  void SetSourceConnection(svtkAlgorithmOutput* algOutput);

  //@{
  /**
   * Specify the source object used to compute surface interaction with
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSurfaceConnection for connecting the pipeline.
   */
  void SetSurfaceData(svtkDataObject* source);
  svtkDataObject* GetSurface();
  //@}

  /**
   * Specify the object used to compute surface interaction with.
   */
  void SetSurfaceConnection(svtkAlgorithmOutput* algOutput);

  /**
   * Declare input port type
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * Declare output port type
   */
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  /**
   * Create outputs objects.
   */
  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Process inputs to integrate particle and generate output.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Get the tracker modified time taking into account the integration model
   * and the integrator.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Get an unique id for a particle
   * This method is thread safe
   */
  virtual svtkIdType GetNewParticleId();

protected:
  svtkLagrangianParticleTracker();
  ~svtkLagrangianParticleTracker() override;

  virtual bool InitializeFlow(svtkDataObject* flow, svtkBoundingBox* bounds);
  virtual bool InitializeParticles(const svtkBoundingBox* bounds, svtkDataSet* seeds,
    std::queue<svtkLagrangianParticle*>& particles, svtkPointData* seedData);
  virtual void GenerateParticles(const svtkBoundingBox* bounds, svtkDataSet* seeds,
    svtkDataArray* initialVelocities, svtkDataArray* initialIntegrationTimes, svtkPointData* seedData,
    int nVar, std::queue<svtkLagrangianParticle*>& particles);
  virtual bool UpdateSurfaceCacheIfNeeded(svtkDataObject*& surfaces);
  virtual void InitializeSurface(svtkDataObject*& surfaces);

  /**
   * This method is thread safe
   */
  virtual bool InitializePathsOutput(
    svtkPointData* seedData, svtkIdType numberOfSeeds, svtkPolyData*& particlePathsOutput);

  /**
   * This method is thread safe
   */
  virtual bool InitializeInteractionOutput(
    svtkPointData* seedData, svtkDataObject* surfaces, svtkDataObject*& interractionOutput);

  virtual bool FinalizeOutputs(svtkPolyData* particlePathsOutput, svtkDataObject* interactionOutput);

  static void InsertPolyVertexCell(svtkPolyData* polydata);
  static void InsertVertexCells(svtkPolyData* polydata);

  virtual void GetParticleFeed(std::queue<svtkLagrangianParticle*>& particleQueue);

  /**
   * This method is thread safe
   */
  virtual int Integrate(svtkInitialValueProblemSolver* integrator, svtkLagrangianParticle*,
    std::queue<svtkLagrangianParticle*>&, svtkPolyData* particlePathsOutput,
    svtkPolyLine* particlePath, svtkDataObject* interactionOutput);

  /**
   * This method is thread safe
   */
  void InsertPathOutputPoint(svtkLagrangianParticle* particle, svtkPolyData* particlePathsOutput,
    svtkIdList* particlePathPointId, bool prev = false);

  /**
   * This method is thread safe
   */
  void InsertInteractionOutputPoint(svtkLagrangianParticle* particle,
    unsigned int interactedSurfaceFlatIndex, svtkDataObject* interactionOutput);

  double ComputeCellLength(svtkLagrangianParticle* particle);

  /**
   * This method is thread safe
   */
  bool ComputeNextStep(svtkInitialValueProblemSolver* integrator, double* xprev, double* xnext,
    double t, double& delT, double& delTActual, double minStep, double maxStep, double cellLength,
    int& integrationRes, svtkLagrangianParticle* particle);

  svtkLagrangianBasicIntegrationModel* IntegrationModel;
  svtkInitialValueProblemSolver* Integrator;

  int CellLengthComputationMode;
  double StepFactor;
  double StepFactorMin;
  double StepFactorMax;
  int MaximumNumberOfSteps;
  double MaximumIntegrationTime;
  bool AdaptiveStepReintegration;
  bool GenerateParticlePathsOutput = true;
  bool GeneratePolyVertexInteractionOutput;
  std::atomic<svtkIdType> ParticleCounter;
  std::atomic<svtkIdType> IntegratedParticleCounter;
  svtkIdType IntegratedParticleCounterIncrement;
  svtkPointData* SeedData;

  // internal parameters use for step computation
  double MinimumVelocityMagnitude;
  double MinimumReductionFactor;

  // Cache related parameters
  svtkDataObject* FlowCache;
  svtkMTimeType FlowTime;
  svtkBoundingBox FlowBoundsCache;
  svtkDataObject* SurfacesCache;
  svtkMTimeType SurfacesTime;

  std::mutex ProgressMutex;
  friend struct IntegratingFunctor;

private:
  svtkLagrangianParticleTracker(const svtkLagrangianParticleTracker&) = delete;
  void operator=(const svtkLagrangianParticleTracker&) = delete;
};

#endif
