/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLagrangianBasicIntegrationModel.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLagrangianBasicIntegrationModel
 * @brief   svtkFunctionSet abstract implementation to be used
 * in the svtkLagrangianParticleTracker integrator.
 *
 * This svtkFunctionSet abstract implementation
 * is meant to be used as a parameter of svtkLagrangianParticleTracker.
 * It manages multiple dataset locators in order to evaluate the
 * svtkFunctionSet::FunctionValues method.
 * The actual FunctionValues implementation should be found in the class inheriting
 * this class.
 * Input Arrays to process are expected as follows:
 * Index 0 : "SurfaceType" array of surface input of the particle tracker
 *
 * Inherited classes MUST implement
 * int FunctionValues(svtkDataSet* detaSet, svtkIdType cellId, double* weights,
 *    double * x, double * f);
 * to define how the integration works.
 *
 * Inherited classes could reimplement InitializeVariablesParticleData and
 * InsertVariablesParticleData to add new UserVariables to integrate with.
 *
 * Inherited classes could reimplement InteractWithSurface or other surface interaction methods
 * to change the way particles interact with surfaces.
 *
 * Inherited classes could reimplement IntersectWithLine to use a specific algorithm
 * to intersect particles and surface cells.
 *
 * Inherited classes could reimplement CheckFreeFlightTermination to set
 * the way particles terminate in free flight.
 *
 * Inherited classes could reimplement Initialize*Data and Insert*Data in order
 * to customize the output of the tracker
 *
 * @sa
 * svtkLagrangianParticleTracker svtkLagrangianParticle
 * svtkLagrangianMatidaIntegrationModel
 */

#ifndef svtkLagrangianBasicIntegrationModel_h
#define svtkLagrangianBasicIntegrationModel_h

#include "svtkFiltersFlowPathsModule.h" // For export macro
#include "svtkFunctionSet.h"
#include "svtkNew.h"         // For arrays
#include "svtkWeakPointer.h" // For weak pointer

#include <map>   // for array indexes
#include <mutex> // for mutexes
#include <queue> // for new particles

class svtkAbstractArray;
class svtkAbstractCellLocator;
class svtkCell;
class svtkCellData;
class svtkDataArray;
class svtkDataObject;
class svtkDataSet;
class svtkDataSetsType;
class svtkDoubleArray;
class svtkFieldData;
class svtkGenericCell;
class svtkInitialValueProblemSolver;
class svtkIntArray;
class svtkLagrangianParticle;
class svtkLagrangianParticleTracker;
class svtkLocatorsType;
class svtkMultiBlockDataSet;
class svtkMultiPieceDataSet;
class svtkPointData;
class svtkPolyData;
class svtkStringArray;
class svtkSurfaceType;
struct svtkLagrangianThreadedData;

class SVTKFILTERSFLOWPATHS_EXPORT svtkLagrangianBasicIntegrationModel : public svtkFunctionSet
{
public:
  svtkTypeMacro(svtkLagrangianBasicIntegrationModel, svtkFunctionSet);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  typedef enum SurfaceType
  {
    SURFACE_TYPE_MODEL = 0,
    SURFACE_TYPE_TERM = 1,
    SURFACE_TYPE_BOUNCE = 2,
    SURFACE_TYPE_BREAK = 3,
    SURFACE_TYPE_PASS = 4
  } SurfaceType;

  typedef enum VariableStep
  {
    VARIABLE_STEP_PREV = -1,
    VARIABLE_STEP_CURRENT = 0,
    VARIABLE_STEP_NEXT = 1,
  } VariableStep;

  typedef std::pair<unsigned int, svtkLagrangianParticle*> PassThroughParticlesItem;
  typedef std::queue<PassThroughParticlesItem> PassThroughParticlesType;

  using Superclass::FunctionValues;
  /**
   * Evaluate integration model velocity f at position x.
   * Look for the cell containing the position x in all its added datasets
   * if found this will call
   * FunctionValues(svtkDataSet* detaSet, svtkIdType cellId, double* x, double* f)
   * This method is thread safe.
   */
  int FunctionValues(double* x, double* f, void* userData) override;

  //@{
  /**
   * Set/Get the locator used to locate cells in the datasets.
   * Only the locator class matter here, as it is used only to
   * create NewInstance of it.
   * Default is a svtkCellLocator.
   */
  virtual void SetLocator(svtkAbstractCellLocator* locator);
  svtkGetObjectMacro(Locator, svtkAbstractCellLocator);
  //@}

  //@{
  /**
   * Get the state of the current locators
   */
  svtkGetMacro(LocatorsBuilt, bool);
  svtkSetMacro(LocatorsBuilt, bool);
  //@}

  /**
   * Set the parent tracker.
   */
  virtual void SetTracker(svtkLagrangianParticleTracker* Tracker);

  //@{
  /**
   * Add a dataset to locate cells in
   * This create a specific locator for the provided dataset
   * using the Locator member of this class
   * The surface flag allow to manage surfaces datasets for surface interaction
   * instead of flow datasets
   * surfaceFlatIndex, used only with composite surface, in order to identify the
   * flatIndex of the surface for particle interaction
   */
  virtual void AddDataSet(
    svtkDataSet* dataset, bool surface = false, unsigned int surfaceFlatIndex = 0);
  virtual void ClearDataSets(bool surface = false);
  //@}

  //@{
  /**
   * Set/Get the Use of initial integration input array to process
   */
  svtkSetMacro(UseInitialIntegrationTime, bool);
  svtkGetMacro(UseInitialIntegrationTime, bool);
  svtkBooleanMacro(UseInitialIntegrationTime, bool);
  //@}

  //@{
  /**
   * Get the tolerance to use with this model.
   */
  svtkGetMacro(Tolerance, double);
  //@}

  /**
   * Interact the current particle with a surfaces
   * Return a particle to record as interaction point if not nullptr
   * Uses SurfaceType array from the intersected surface cell
   * to compute the interaction.
   * MODEL :
   * svtkLagrangianBasicIntegrationModel::InteractWithSurface
   * method will be used, usually defined in inherited classes
   * TERM :
   * svtkLagrangianBasicIntegrationModel::Terminate method will be used
   * BOUNCE :
   * svtkLagrangianBasicIntegrationModel::Bounce method will be used
   * BREAK_UP :
   * svtkLagrangianBasicIntegrationModel::BreakUp method will be used
   * PASS : The interaction will be recorded
   * with no effect on the particle
   */
  virtual svtkLagrangianParticle* ComputeSurfaceInteraction(svtkLagrangianParticle* particle,
    std::queue<svtkLagrangianParticle*>& particles, unsigned int& interactedSurfaceFlatIndex,
    PassThroughParticlesType& passThroughParticles);

  /**
   * Set a input array to process at a specific index, identified by a port,
   * connection, fieldAssociation and a name.
   * Each inherited class can specify their own input array to process
   */
  virtual void SetInputArrayToProcess(
    int idx, int port, int connection, int fieldAssociation, const char* name);

  /**
   * Look for a dataset in this integration model
   * containing the point x. return false if out of domain,
   * return true and data to recover the cell if in domain.
   * does not filter out ghost cells.
   * convenience method with less outputs.
   * Provide a particle if a dataset/locator cache can be used.
   * This method is thread-safe.
   */
  virtual bool FindInLocators(double* x, svtkLagrangianParticle* particle, svtkDataSet*& dataset,
    svtkIdType& cellId, svtkAbstractCellLocator*& loc, double*& weights);

  //@{
  /**
   * Convienience methods to call FindInLocators with less arguments
   * THESE METHODS ARE NOT THREAD-SAFE
   */
  virtual bool FindInLocators(
    double* x, svtkLagrangianParticle* particle, svtkDataSet*& dataset, svtkIdType& cellId);
  virtual bool FindInLocators(double* x, svtkLagrangianParticle* particle);
  //@}

  /**
   * Initialize a particle by setting user variables and perform any user
   * model specific operation. empty in basic implementation.
   */
  virtual void InitializeParticle(svtkLagrangianParticle* svtkNotUsed(particle)) {}

  /**
   * Method to be reimplemented if needed in inherited classes.
   * Allows a inherited class to check if adaptive step reintegration
   * should be done or not, this method is called just before
   * potentially performing adaptative step reintegration,
   * the current particle is passed as an argument.
   * This method always returns true in this basis class.
   */
  virtual bool CheckAdaptiveStepReintegration(svtkLagrangianParticle* svtkNotUsed(particle))
  {
    return true;
  }

  /**
   * Method to be reimplemented if needed in inherited classes.
   * Allows a inherited class to check if a particle
   * should be terminated only based on particle parameters.
   * This method should return true if the particle must be terminated, false otherwise.
   * It always returns false in this basis class.
   * This method is thread-safe, its reimplementation should still be thread-safe.
   */
  virtual bool CheckFreeFlightTermination(svtkLagrangianParticle* svtkNotUsed(particle))
  {
    return false;
  }

  //@{
  /**
   * Set/Get Non Planar Quad Support
   */
  svtkSetMacro(NonPlanarQuadSupport, bool);
  svtkGetMacro(NonPlanarQuadSupport, bool);
  svtkBooleanMacro(NonPlanarQuadSupport, bool);
  //@}

  /**
   * Get the seed arrays expected name
   * Used Only be the svtkLagrangianSeedHelper in ParaView plugins
   */
  virtual svtkStringArray* GetSeedArrayNames();

  /**
   * Get the seed arrays expected number of components
   * Used Only be the svtkLagrangianSeedHelper in ParaView plugins
   */
  virtual svtkIntArray* GetSeedArrayComps();

  /**
   * Get the seed arrays expected type
   * Used Only be the svtkLagrangianSeedHelper in ParaView plugins
   */
  virtual svtkIntArray* GetSeedArrayTypes();

  /**
   * Get the surface arrays expected name
   * Used Only be the svtkLagrangianSurfaceHelper in ParaView plugins
   */
  virtual svtkStringArray* GetSurfaceArrayNames();

  /**
   * Get the surface arrays expected type
   * Used Only be the svtkLagrangianSurfaceHelper in ParaView plugins
   */
  virtual svtkIntArray* GetSurfaceArrayTypes();

  /**
   * Get the surface arrays expected values and associated enums
   * Used Only be the svtkLagrangianSurfaceHelper in ParaView plugins
   */
  virtual svtkStringArray* GetSurfaceArrayEnumValues();

  /**
   * Get the surface arrays default values for each leaf
   * Used Only be the svtkLagrangianSurfaceHelper in ParaView plugins
   */
  virtual svtkDoubleArray* GetSurfaceArrayDefaultValues();

  /**
   * Get the seed array expected number of components
   * Used Only be the svtkLagrangianSurfaceHelper in ParaView plugins
   */
  virtual svtkIntArray* GetSurfaceArrayComps();

  //@{
  /**
   * Get the maximum weights size necessary for calling
   * FindInLocators with weights
   */
  virtual int GetWeightsSize();
  //@}

  /**
   * Let the model define it's own way to integrate
   * Signature is very close to the integrator method signature
   * output is expected to be the same,
   * see svtkInitialValueProblemSolver::ComputeNextStep for reference
   * xcur is the current particle variables
   * xnext is the next particle variable
   * t is the current integration time
   * delT is the timeStep, which is also an output for adaptative algorithm
   * delTActual is the time step output corresponding to the actual movement of the particle
   * minStep is the minimum step time for adaptative algorithm
   * maxStep is the maximum step time for adaptative algorithm
   * maxError is the maximum acceptable error
   * error is the output of actual error
   * integrationResult is the result of the integration, see
   * svtkInitialValueProblemSolver::ErrorCodes for error report
   * otherwise it should be zero. be aware that only stagnating OUT_OF_DOMAIN
   * will be considered actual out of domain error.
   * Return true if manual integration was used, false otherwise
   * Simply return false in svtkLagrangianBasicIntegrationModel
   * implementation.
   * This method is thread-safe, its reimplementation should still be thread-safe.
   */
  virtual bool ManualIntegration(svtkInitialValueProblemSolver* integrator, double* xcur,
    double* xnext, double t, double& delT, double& delTActual, double minStep, double maxStep,
    double maxError, double cellLength, double& error, int& integrationResult,
    svtkLagrangianParticle* particle);

  /**
   * Method called by parallel algorithm
   * after receiving a particle from stream if PManualShift flag has been set to true
   * on the particle. Does nothing in base implementation
   */
  virtual void ParallelManualShift(svtkLagrangianParticle* svtkNotUsed(particle)) {}

  /**
   * Let the model allocate and initialize a threaded data.
   * This method is thread-safe, its reimplementation should still be thread-safe.
   */
  virtual void InitializeThreadedData(svtkLagrangianThreadedData* svtkNotUsed(data)) {}

  /**
   * Let the model finalize and deallocate a user data at thread level
   * This method is called serially for each thread and does not require to be thread safe.
   */
  virtual void FinalizeThreadedData(svtkLagrangianThreadedData* svtkNotUsed(data)) {}

  /**
   * Enable model post process on output
   * Return true if successful, false otherwise
   * Empty and Always return true with basic model
   */
  virtual bool FinalizeOutputs(
    svtkPolyData* svtkNotUsed(particlePathsOutput), svtkDataObject* svtkNotUsed(interractionOutput))
  {
    return true;
  }

  /**
   * Enable model to modify particle before integration
   */
  virtual void PreIntegrate(std::queue<svtkLagrangianParticle*>& svtkNotUsed(particles)) {}

  /**
   * Get a seed array, as set in setInputArrayToProcess
   * from the provided seed point data
   */
  virtual svtkAbstractArray* GetSeedArray(int idx, svtkPointData* pointData);

  /**
   * Set/Get the number of tracked user data attached to the particles.
   * Tracked user data are data that are related to each particle position
   * but are not integrated like the user variables.
   * They are not saved in the particle path.
   * Default is 0.
   */
  svtkSetMacro(NumberOfTrackedUserData, int);
  svtkGetMacro(NumberOfTrackedUserData, int);

  /**
   * Method used by the LPT to initialize data insertion in the provided
   * svtkFieldData. It initializes Id, ParentID, SeedID and Termination.
   * Reimplement as needed in acccordance with InsertPathData.
   */
  virtual void InitializePathData(svtkFieldData* data);

  /**
   * Method used by the LPT to initialize data insertion in the provided
   * svtkFieldData. It initializes Interaction.
   * Reimplement as needed in acccordance with InsertInteractionData.
   */
  virtual void InitializeInteractionData(svtkFieldData* data);

  /**
   * Method used by the LPT to initialize data insertion in the provided
   * svtkFieldData. It initializes StepNumber, ParticleVelocity, IntegrationTime.
   * Reimplement as needed in acccordance with InsertParticleData.
   */
  virtual void InitializeParticleData(svtkFieldData* particleData, int maxTuples = 0);

  /**
   * Method used by the LPT to insert data from the partice into
   * the provided svtkFieldData. It inserts Id, ParentID, SeedID and Termination.
   * Reimplement as needed in acccordance with InitializePathData.
   */
  virtual void InsertPathData(svtkLagrangianParticle* particle, svtkFieldData* data);

  /**
   * Method used by the LPT to insert data from the partice into
   * the provided svtkFieldData. It inserts Interaction.
   * Reimplement as needed in acccordance with InitializeInteractionData.
   */
  virtual void InsertInteractionData(svtkLagrangianParticle* particle, svtkFieldData* data);

  /**
   * Method used by the LPT to insert data from the partice into
   * the provided svtkFieldData. It inserts StepNumber, ParticleVelocity, IntegrationTime.
   * stepEnum enables to select which data to insert, Prev, Current or Next.
   * Reimplement as needed in acccordance with InitializeParticleData.
   */
  virtual void InsertParticleData(
    svtkLagrangianParticle* particle, svtkFieldData* data, int stepEnum);

  /**
   * Method used by the LPT to insert data from the partice into
   * the provided svtkFieldData. It inserts all arrays from the original SeedData.
   * Reimplement as needed.
   */
  virtual void InsertParticleSeedData(svtkLagrangianParticle* particle, svtkFieldData* data);

  /**
   * Method to be reimplemented if needed in inherited classes.
   * Allows a inherited class to take action just before a particle is deleted
   * This can be practical when working with svtkLagrangianParticle::TemporaryUserData.
   * This can be called with not fully initialized particle.
   */
  virtual void ParticleAboutToBeDeleted(svtkLagrangianParticle* svtkNotUsed(particle)) {}

protected:
  svtkLagrangianBasicIntegrationModel();
  ~svtkLagrangianBasicIntegrationModel() override;

  /**
   * Actually compute the integration model velocity field
   * pure abstract, to be implemented in inherited class
   * This method implementation should be thread-safe
   */
  virtual int FunctionValues(svtkLagrangianParticle* particle, svtkDataSet* dataSet, svtkIdType cellId,
    double* weights, double* x, double* f) = 0;

  /**
   * Look in the given dataset and associated locator to see if it contains
   * the point x, if so return the cellId and output the cell containing the point
   * and the weights of the point in the cell
   * This method is thread-safe, its reimplementation should also be.
   */
  virtual svtkIdType FindInLocator(svtkDataSet* dataSet, svtkAbstractCellLocator* locator, double* x,
    svtkGenericCell* cell, double* weights);

  /**
   * Terminate a particle, by positioning flags.
   * Return true to record the interaction, false otherwise
   * This method is thread-safe.
   */
  virtual bool TerminateParticle(svtkLagrangianParticle* particle);

  /**
   * Bounce a particle, using the normal of the cell it bounces on.
   * Return true to record the interaction, false otherwise
   * This method is thread-safe.
   */
  virtual bool BounceParticle(
    svtkLagrangianParticle* particle, svtkDataSet* surface, svtkIdType cellId);

  /**
   * Breakup a particle at intersection point, by terminating it and creating two
   * new particle using the intersected cells normals
   * Return true to record the interaction, false otherwise
   * This method is thread-safe and uses svtkLagrangianBasicIntegrationModel::ParticleQueueMutex
   * to access the particles queue, its reimplementation should also be.
   */
  virtual bool BreakParticle(svtkLagrangianParticle* particle, svtkDataSet* surface, svtkIdType cellId,
    std::queue<svtkLagrangianParticle*>& particles);

  /**
   * Call svtkLagrangianBasicIntegrationModel::Terminate
   * This method is to be reimplemented in inherited classes willing
   * to implement specific particle surface interactions
   * Return true to record the interaction, false otherwise
   * This method is thread-safe and should use
   * svtkLagrangianBasicIntegrationModel::ParticleQueueMutex
   *  to add particles to the particles queue, see BreakParticle for an example.
   */
  virtual bool InteractWithSurface(int surfaceType, svtkLagrangianParticle* particle,
    svtkDataSet* surface, svtkIdType cellId, std::queue<svtkLagrangianParticle*>& particles);

  /**
   * Call svtkCell::IntersectWithLine
   * This method is to be reimplemented in inherited classes willing
   * to implement specific line/surface intersection
   * This method is thread-safe.
   */
  virtual bool IntersectWithLine(svtkLagrangianParticle* particle, svtkCell* cell, double p1[3],
    double p2[3], double tol, double& t, double x[3]);

  /**
   * compute all particle variables using interpolation factor
   * This method is thread-safe.
   */
  virtual void InterpolateNextParticleVariables(
    svtkLagrangianParticle* particle, double interpolationFactor, bool forceInside = false);

  /**
   * Given a particle, check if it perforate a surface cell
   * ie : interact with next step after interacting with it
   * This method is thread-safe.
   */
  virtual bool CheckSurfacePerforation(
    svtkLagrangianParticle* particle, svtkDataSet* surface, svtkIdType cellId);

  /**
   * Get a seed array, as set in setInputArrayToProcess
   * from the provided particle seed data
   * Access then the first tuple to access the data
   * This method is thread-safe.
   */
  virtual svtkAbstractArray* GetSeedArray(int idx, svtkLagrangianParticle* particle);

  /**
   * Directly get a double value from flow or surface data
   * as defined in SetInputArrayToProcess.
   * Make sure that data pointer is large enough using
   * GetFlowOrSurfaceDataNumberOfComponents if needed.
   * This method is thread-safe.
   */
  virtual bool GetFlowOrSurfaceData(svtkLagrangianParticle* particle, int idx,
    svtkDataSet* flowDataSet, svtkIdType tupleId, double* weights, double* data);

  /**
   * Recover the number of components for a specified array index
   * if it has been set using SetInputArrayToProcess,
   * with provided dataset.
   * Returns -1 in case of error.
   * This method is thread-safe.
   */
  virtual int GetFlowOrSurfaceDataNumberOfComponents(int idx, svtkDataSet* dataSet);

  /**
   * Recover a field association for a specified array index
   * if it has been set using SetInputArrayToProcess
   * This method is thread-safe.
   */
  virtual int GetFlowOrSurfaceDataFieldAssociation(int idx);

  /**
   * Method used by ParaView surface helper to get default
   * values for each leaf of each dataset of surface
   * nComponents could be retrieved with arrayName but is
   * given for simplication purposes.
   * it is your responsibility to initialize all components of
   * defaultValues[nComponent]
   */
  virtual void ComputeSurfaceDefaultValues(
    const char* arrayName, svtkDataSet* dataset, int nComponent, double* defaultValues);

  svtkAbstractCellLocator* Locator;
  bool LocatorsBuilt;
  svtkLocatorsType* Locators;
  svtkDataSetsType* DataSets;
  std::vector<double> SharedWeights;

  struct ArrayVal
  {
    int val[3];
  };
  typedef std::pair<ArrayVal, std::string> ArrayMapVal;
  std::map<int, ArrayMapVal> InputArrays;

  typedef struct SurfaceArrayDescription
  {
    int nComp;
    int type;
    std::vector<std::pair<int, std::string> > enumValues;
  } SurfaceArrayDescription;
  std::map<std::string, SurfaceArrayDescription> SurfaceArrayDescriptions;

  svtkSurfaceType* Surfaces;
  svtkLocatorsType* SurfaceLocators;

  double Tolerance;
  bool NonPlanarQuadSupport;
  bool UseInitialIntegrationTime;
  int NumberOfTrackedUserData = 0;

  svtkNew<svtkStringArray> SeedArrayNames;
  svtkNew<svtkIntArray> SeedArrayComps;
  svtkNew<svtkIntArray> SeedArrayTypes;
  svtkNew<svtkStringArray> SurfaceArrayNames;
  svtkNew<svtkIntArray> SurfaceArrayComps;
  svtkNew<svtkIntArray> SurfaceArrayTypes;
  svtkNew<svtkStringArray> SurfaceArrayEnumValues;
  svtkNew<svtkDoubleArray> SurfaceArrayDefaultValues;

  svtkWeakPointer<svtkLagrangianParticleTracker> Tracker;
  std::mutex ParticleQueueMutex;

private:
  svtkLagrangianBasicIntegrationModel(const svtkLagrangianBasicIntegrationModel&) = delete;
  void operator=(const svtkLagrangianBasicIntegrationModel&) = delete;
};

#endif
