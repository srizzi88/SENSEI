/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStreamTracer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkStreamTracer
 * @brief   Streamline generator
 *
 * svtkStreamTracer is a filter that integrates a vector field to generate
 * streamlines. The integration is performed using a specified integrator,
 * by default Runge-Kutta2.
 *
 * svtkStreamTracer produces polylines as the output, with each cell (i.e.,
 * polyline) representing a streamline. The attribute values associated
 * with each streamline are stored in the cell data, whereas those
 * associated with streamline-points are stored in the point data.
 *
 * svtkStreamTracer supports forward (the default), backward, and combined
 * (i.e., BOTH) integration. The length of a streamline is governed by
 * specifying a maximum value either in physical arc length or in (local)
 * cell length. Otherwise, the integration terminates upon exiting the
 * flow field domain, or if the particle speed is reduced to a value less
 * than a specified terminal speed, or when a maximum number of steps is
 * completed. The specific reason for the termination is stored in a cell
 * array named ReasonForTermination.
 *
 * Note that normalized vectors are adopted in streamline integration,
 * which achieves high numerical accuracy/smoothness of flow lines that is
 * particularly guaranteed for Runge-Kutta45 with adaptive step size and
 * error control). In support of this feature, the underlying step size is
 * ALWAYS in arc length unit (LENGTH_UNIT) while the 'real' time interval
 * (virtual for steady flows) that a particle actually takes to trave in a
 * single step is obtained by dividing the arc length by the LOCAL speed.
 * The overall elapsed time (i.e., the life span) of the particle is the
 * sum of those individual step-wise time intervals.
 *
 * The quality of streamline integration can be controlled by setting the
 * initial integration step (InitialIntegrationStep), particularly for
 * Runge-Kutta2 and Runge-Kutta4 (with a fixed step size), and in the case
 * of Runge-Kutta45 (with an adaptive step size and error control) the
 * minimum integration step, the maximum integration step, and the maximum
 * error. These steps are in either LENGTH_UNIT or CELL_LENGTH_UNIT while
 * the error is in physical arc length. For the former two integrators,
 * there is a trade-off between integration speed and streamline quality.
 *
 * The integration time, vorticity, rotation and angular velocity are stored
 * in point data arrays named "IntegrationTime", "Vorticity", "Rotation" and
 * "AngularVelocity", respectively (vorticity, rotation and angular velocity
 * are computed only when ComputeVorticity is on). All point data attributes
 * in the source dataset are interpolated on the new streamline points.
 *
 * svtkStreamTracer supports integration through any type of dataset. Thus if
 * the dataset contains 2D cells like polygons or triangles, the integration
 * is constrained to lie on the surface defined by 2D cells.
 *
 * The starting point, or the so-called 'seed', of a streamline may be set
 * in two different ways. Starting from global x-y-z "position" allows you
 * to start a single trace at a specified x-y-z coordinate. If you specify
 * a source object, traces will be generated from each point in the source
 * that is inside the dataset.
 *
 * @sa
 * svtkRibbonFilter svtkRuledSurfaceFilter svtkInitialValueProblemSolver
 * svtkRungeKutta2 svtkRungeKutta4 svtkRungeKutta45 svtkParticleTracerBase
 * svtkParticleTracer svtkParticlePathFilter svtkStreaklineFilter
 * svtkAbstractInterpolatedVelocityField svtkInterpolatedVelocityField
 * svtkCellLocatorInterpolatedVelocityField
 *
 */

#ifndef svtkStreamTracer_h
#define svtkStreamTracer_h

#include "svtkFiltersFlowPathsModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkInitialValueProblemSolver.h" // Needed for constants

class svtkAbstractInterpolatedVelocityField;
class svtkCompositeDataSet;
class svtkDataArray;
class svtkDataSetAttributes;
class svtkDoubleArray;
class svtkExecutive;
class svtkGenericCell;
class svtkIdList;
class svtkIntArray;
class svtkPoints;

#include <vector>

class SVTKFILTERSFLOWPATHS_EXPORT svtkStreamTracer : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkStreamTracer, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object to start from position (0,0,0), with forward
   * integration, terminal speed 1.0E-12, vorticity computation on,
   * integration step size 0.5 (in cell length unit), maximum number
   * of steps 2000, using Runge-Kutta2, and maximum propagation 1.0
   * (in arc length unit).
   */
  static svtkStreamTracer* New();

  //@{
  /**
   * Specify the starting point (seed) of a streamline in the global
   * coordinate system. Search must be performed to find the initial cell
   * from which to start integration.
   */
  svtkSetVector3Macro(StartPosition, double);
  svtkGetVector3Macro(StartPosition, double);
  //@}

  //@{
  /**
   * Specify the source object used to generate starting points (seeds).
   * Note that this method does not connect the pipeline. The algorithm will
   * work on the input data as it is without updating the producer of the data.
   * See SetSourceConnection for connecting the pipeline.
   */
  void SetSourceData(svtkDataSet* source);
  svtkDataSet* GetSource();
  //@}

  /**
   * Specify the source object used to generate starting points (seeds).
   * New style.
   */
  void SetSourceConnection(svtkAlgorithmOutput* algOutput);

  // The previously-supported TIME_UNIT is excluded in this current
  // enumeration definition because the underlying step size is ALWAYS in
  // arc length unit (LENGTH_UNIT) while the 'real' time interval (virtual
  // for steady flows) that a particle actually takes to trave in a single
  // step is obtained by dividing the arc length by the LOCAL speed. The
  // overall elapsed time (i.e., the life span) of the particle is the sum
  // of those individual step-wise time intervals. The arc-length-to-time
  // conversion only occurs for vorticity computation and for generating a
  // point data array named 'IntegrationTime'.
  enum Units
  {
    LENGTH_UNIT = 1,
    CELL_LENGTH_UNIT = 2
  };

  enum Solvers
  {
    RUNGE_KUTTA2,
    RUNGE_KUTTA4,
    RUNGE_KUTTA45,
    NONE,
    UNKNOWN
  };

  enum ReasonForTermination
  {
    OUT_OF_DOMAIN = svtkInitialValueProblemSolver::OUT_OF_DOMAIN,
    NOT_INITIALIZED = svtkInitialValueProblemSolver::NOT_INITIALIZED,
    UNEXPECTED_VALUE = svtkInitialValueProblemSolver::UNEXPECTED_VALUE,
    OUT_OF_LENGTH = 4,
    OUT_OF_STEPS = 5,
    STAGNATION = 6,
    FIXED_REASONS_FOR_TERMINATION_COUNT
  };

  //@{
  /**
   * Set/get the integrator type to be used for streamline generation.
   * The object passed is not actually used but is cloned with
   * NewInstance in the process of integration  (prototype pattern).
   * The default is Runge-Kutta2. The integrator can also be changed
   * using SetIntegratorType. The recognized solvers are:
   * RUNGE_KUTTA2  = 0
   * RUNGE_KUTTA4  = 1
   * RUNGE_KUTTA45 = 2
   */
  void SetIntegrator(svtkInitialValueProblemSolver*);
  svtkGetObjectMacro(Integrator, svtkInitialValueProblemSolver);
  void SetIntegratorType(int type);
  int GetIntegratorType();
  void SetIntegratorTypeToRungeKutta2() { this->SetIntegratorType(RUNGE_KUTTA2); }
  void SetIntegratorTypeToRungeKutta4() { this->SetIntegratorType(RUNGE_KUTTA4); }
  void SetIntegratorTypeToRungeKutta45() { this->SetIntegratorType(RUNGE_KUTTA45); }
  //@}

  /**
   * Set the velocity field interpolator type to the one involving
   * a dataset point locator.
   */
  void SetInterpolatorTypeToDataSetPointLocator();

  /**
   * Set the velocity field interpolator type to the one involving
   * a cell locator.
   */
  void SetInterpolatorTypeToCellLocator();

  //@{
  /**
   * Specify the maximum length of a streamline expressed in LENGTH_UNIT.
   */
  svtkSetMacro(MaximumPropagation, double);
  svtkGetMacro(MaximumPropagation, double);
  //@}

  /**
   * Specify a uniform integration step unit for MinimumIntegrationStep,
   * InitialIntegrationStep, and MaximumIntegrationStep. NOTE: The valid
   * unit is now limited to only LENGTH_UNIT (1) and CELL_LENGTH_UNIT (2),
   * EXCLUDING the previously-supported TIME_UNIT.
   */
  void SetIntegrationStepUnit(int unit);
  int GetIntegrationStepUnit() { return this->IntegrationStepUnit; }

  //@{
  /**
   * Specify the Initial step size used for line integration, expressed in:
   * LENGTH_UNIT      = 1
   * CELL_LENGTH_UNIT = 2
   * (either the starting size for an adaptive integrator, e.g., RK45,
   * or the constant / fixed size for non-adaptive ones, i.e., RK2 and RK4)
   */
  svtkSetMacro(InitialIntegrationStep, double);
  svtkGetMacro(InitialIntegrationStep, double);
  //@}

  //@{
  /**
   * Specify the Minimum step size used for line integration, expressed in:
   * LENGTH_UNIT      = 1
   * CELL_LENGTH_UNIT = 2
   * (Only valid for an adaptive integrator, e.g., RK45)
   */
  svtkSetMacro(MinimumIntegrationStep, double);
  svtkGetMacro(MinimumIntegrationStep, double);
  //@}

  //@{
  /**
   * Specify the Maximum step size used for line integration, expressed in:
   * LENGTH_UNIT      = 1
   * CELL_LENGTH_UNIT = 2
   * (Only valid for an adaptive integrator, e.g., RK45)
   */
  svtkSetMacro(MaximumIntegrationStep, double);
  svtkGetMacro(MaximumIntegrationStep, double);
  //@}

  //@{
  /**
   * Specify the maximum error tolerated throughout streamline integration.
   */
  svtkSetMacro(MaximumError, double);
  svtkGetMacro(MaximumError, double);
  //@}

  //@{
  /**
   * Specify the maximum number of steps for integrating a streamline.
   */
  svtkSetMacro(MaximumNumberOfSteps, svtkIdType);
  svtkGetMacro(MaximumNumberOfSteps, svtkIdType);
  //@}

  //@{
  /**
   * Specify the terminal speed value, below which integration is terminated.
   */
  svtkSetMacro(TerminalSpeed, double);
  svtkGetMacro(TerminalSpeed, double);
  //@}

  //@{
  /**
   * Set/Unset the streamlines to be computed on a surface
   */
  svtkGetMacro(SurfaceStreamlines, bool);
  svtkSetMacro(SurfaceStreamlines, bool);
  svtkBooleanMacro(SurfaceStreamlines, bool);
  //@}

  enum
  {
    FORWARD,
    BACKWARD,
    BOTH
  };

  enum
  {
    INTERPOLATOR_WITH_DATASET_POINT_LOCATOR,
    INTERPOLATOR_WITH_CELL_LOCATOR
  };

  //@{
  /**
   * Specify whether the streamline is integrated in the upstream or
   * downstream direction.
   */
  svtkSetClampMacro(IntegrationDirection, int, FORWARD, BOTH);
  svtkGetMacro(IntegrationDirection, int);
  void SetIntegrationDirectionToForward() { this->SetIntegrationDirection(FORWARD); }
  void SetIntegrationDirectionToBackward() { this->SetIntegrationDirection(BACKWARD); }
  void SetIntegrationDirectionToBoth() { this->SetIntegrationDirection(BOTH); }
  //@}

  //@{
  /**
   * Turn on/off vorticity computation at streamline points
   * (necessary for generating proper stream-ribbons using the
   * svtkRibbonFilter.
   */
  svtkSetMacro(ComputeVorticity, bool);
  svtkGetMacro(ComputeVorticity, bool);
  //@}

  //@{
  /**
   * This can be used to scale the rate with which the streamribbons
   * twist. The default is 1.
   */
  svtkSetMacro(RotationScale, double);
  svtkGetMacro(RotationScale, double);
  //@}

  /**
   * The object used to interpolate the velocity field during
   * integration is of the same class as this prototype.
   */
  void SetInterpolatorPrototype(svtkAbstractInterpolatedVelocityField* ivf);

  /**
   * Set the type of the velocity field interpolator to determine whether
   * svtkInterpolatedVelocityField (INTERPOLATOR_WITH_DATASET_POINT_LOCATOR) or
   * svtkCellLocatorInterpolatedVelocityField (INTERPOLATOR_WITH_CELL_LOCATOR)
   * is employed for locating cells during streamline integration. The latter
   * (adopting svtkAbstractCellLocator sub-classes such as svtkCellLocator and
   * svtkModifiedBSPTree) is more robust then the former (through svtkDataSet /
   * svtkPointSet::FindCell() coupled with svtkPointLocator).
   */
  void SetInterpolatorType(int interpType);

  /**
   * Asks the user if the current streamline should be terminated.
   * clientdata is set by the client when setting up the callback.
   * points is the array of points integrated so far
   * velocity velocity vector integrated to produce the streamline
   * integrationDirection FORWARD of BACKWARD
   * The function returns true if the streamline should be terminated
   * and false otherwise.
   */
  typedef bool (*CustomTerminationCallbackType)(
    void* clientdata, svtkPoints* points, svtkDataArray* velocity, int integrationDirection);
  /**
   * Adds a custom termination callback.
   * callback is a function provided by the user that says if the streamline
   *         should be terminated.
   * clientdata user specific data passed to the callback
   * reasonForTermination this value will be set in the ReasonForTermination cell
   *          array if the streamline is terminated by this callback.
   */
  void AddCustomTerminationCallback(
    CustomTerminationCallbackType callback, void* clientdata, int reasonForTermination);

protected:
  svtkStreamTracer();
  ~svtkStreamTracer() override;

  // Create a default executive.
  svtkExecutive* CreateDefaultExecutive() override;

  // hide the superclass' AddInput() from the user and the compiler
  void AddInput(svtkDataObject*)
  {
    svtkErrorMacro(<< "AddInput() must be called with a svtkDataSet not a svtkDataObject.");
  }

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

  void CalculateVorticity(
    svtkGenericCell* cell, double pcoords[3], svtkDoubleArray* cellVectors, double vorticity[3]);
  void Integrate(svtkPointData* inputData, svtkPolyData* output, svtkDataArray* seedSource,
    svtkIdList* seedIds, svtkIntArray* integrationDirections, double lastPoint[3],
    svtkAbstractInterpolatedVelocityField* func, int maxCellSize, int vecType,
    const char* vecFieldName, double& propagation, svtkIdType& numSteps, double& integrationTime);
  double SimpleIntegrate(double seed[3], double lastPoint[3], double stepSize,
    svtkAbstractInterpolatedVelocityField* func);
  int CheckInputs(svtkAbstractInterpolatedVelocityField*& func, int* maxCellSize);
  void GenerateNormals(svtkPolyData* output, double* firstNormal, const char* vecName);

  bool GenerateNormalsInIntegrate;

  // starting from global x-y-z position
  double StartPosition[3];

  static const double EPSILON;
  double TerminalSpeed;

  double LastUsedStepSize;

  struct IntervalInformation
  {
    double Interval;
    int Unit;
  };

  double MaximumPropagation;
  double MinimumIntegrationStep;
  double MaximumIntegrationStep;
  double InitialIntegrationStep;

  void ConvertIntervals(
    double& step, double& minStep, double& maxStep, int direction, double cellLength);
  static double ConvertToLength(double interval, int unit, double cellLength);
  static double ConvertToLength(IntervalInformation& interval, double cellLength);

  int SetupOutput(svtkInformation* inInfo, svtkInformation* outInfo);
  void InitializeSeeds(svtkDataArray*& seeds, svtkIdList*& seedIds,
    svtkIntArray*& integrationDirections, svtkDataSet* source);

  int IntegrationStepUnit;
  int IntegrationDirection;

  // Prototype showing the integrator type to be set by the user.
  svtkInitialValueProblemSolver* Integrator;

  double MaximumError;
  svtkIdType MaximumNumberOfSteps;

  bool ComputeVorticity;
  double RotationScale;

  // Compute streamlines only on surface.
  bool SurfaceStreamlines;

  svtkAbstractInterpolatedVelocityField* InterpolatorPrototype;

  svtkCompositeDataSet* InputData;
  bool
    HasMatchingPointAttributes; // does the point data in the multiblocks have the same attributes?
  std::vector<CustomTerminationCallbackType> CustomTerminationCallback;
  std::vector<void*> CustomTerminationClientData;
  std::vector<int> CustomReasonForTermination;

  friend class PStreamTracerUtils;

private:
  svtkStreamTracer(const svtkStreamTracer&) = delete;
  void operator=(const svtkStreamTracer&) = delete;
};

#endif
// SVTK-HeaderTest-Exclude: svtkStreamTracer.h
