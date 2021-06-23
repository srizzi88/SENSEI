/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHyperStreamline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHyperStreamline
 * @brief   generate hyperstreamline in arbitrary dataset
 *
 * svtkHyperStreamline is a filter that integrates through a tensor field to
 * generate a hyperstreamline. The integration is along the maximum eigenvector
 * and the cross section of the hyperstreamline is defined by the two other
 * eigenvectors. Thus the shape of the hyperstreamline is "tube-like", with
 * the cross section being elliptical. Hyperstreamlines are used to visualize
 * tensor fields.
 *
 * The starting point of a hyperstreamline can be defined in one of two ways.
 * First, you may specify an initial position. This is a x-y-z global
 * coordinate. The second option is to specify a starting location. This is
 * cellId, subId, and cell parametric coordinates.
 *
 * The integration of the hyperstreamline occurs through the major eigenvector
 * field. IntegrationStepLength controls the step length within each cell
 * (i.e., this is the fraction of the cell length). The length of the
 * hyperstreamline is controlled by MaximumPropagationDistance. This parameter
 * is the length of the hyperstreamline in units of distance. The tube itself
 * is composed of many small sub-tubes - NumberOfSides controls the number of
 * sides in the tube, and StepLength controls the length of the sub-tubes.
 *
 * Because hyperstreamlines are often created near regions of singularities, it
 * is possible to control the scaling of the tube cross section by using a
 * logarithmic scale. Use LogScalingOn to turn this capability on. The Radius
 * value controls the initial radius of the tube.
 *
 * @sa
 * svtkTensorGlyph
 */

#ifndef svtkHyperStreamline_h
#define svtkHyperStreamline_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#define SVTK_INTEGRATE_FORWARD 0
#define SVTK_INTEGRATE_BACKWARD 1
#define SVTK_INTEGRATE_BOTH_DIRECTIONS 2

#define SVTK_INTEGRATE_MAJOR_EIGENVECTOR 0
#define SVTK_INTEGRATE_MEDIUM_EIGENVECTOR 1
#define SVTK_INTEGRATE_MINOR_EIGENVECTOR 2

class svtkHyperArray;

class SVTKFILTERSGENERAL_EXPORT svtkHyperStreamline : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkHyperStreamline, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with initial starting position (0,0,0); integration
   * step length 0.2; step length 0.01; forward integration; terminal
   * eigenvalue 0.0; number of sides 6; radius 0.5; and logarithmic scaling
   * off.
   */
  static svtkHyperStreamline* New();

  /**
   * Specify the start of the hyperstreamline in the cell coordinate system.
   * That is, cellId and subId (if composite cell), and parametric coordinates.
   */
  void SetStartLocation(svtkIdType cellId, int subId, double pcoords[3]);

  /**
   * Specify the start of the hyperstreamline in the cell coordinate system.
   * That is, cellId and subId (if composite cell), and parametric coordinates.
   */
  void SetStartLocation(svtkIdType cellId, int subId, double r, double s, double t);

  /**
   * Get the starting location of the hyperstreamline in the cell coordinate
   * system. Returns the cell that the starting point is in.
   */
  svtkIdType GetStartLocation(int& subId, double pcoords[3]);

  /**
   * Specify the start of the hyperstreamline in the global coordinate system.
   * Starting from position implies that a search must be performed to find
   * initial cell to start integration from.
   */
  void SetStartPosition(double x[3]);

  /**
   * Specify the start of the hyperstreamline in the global coordinate system.
   * Starting from position implies that a search must be performed to find
   * initial cell to start integration from.
   */
  void SetStartPosition(double x, double y, double z);

  /**
   * Get the start position of the hyperstreamline in global x-y-z coordinates.
   */
  double* GetStartPosition() SVTK_SIZEHINT(3);

  //@{
  /**
   * Set / get the maximum length of the hyperstreamline expressed as absolute
   * distance (i.e., arc length) value.
   */
  svtkSetClampMacro(MaximumPropagationDistance, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(MaximumPropagationDistance, double);
  //@}

  //@{
  /**
   * Set / get the eigenvector field through which to ingrate. It is
   * possible to integrate using the major, medium or minor
   * eigenvector field.  The major eigenvector is the eigenvector
   * whose corresponding eigenvalue is closest to positive infinity.
   * The minor eigenvector is the eigenvector whose corresponding
   * eigenvalue is closest to negative infinity.  The medium
   * eigenvector is the eigenvector whose corresponding eigenvalue is
   * between the major and minor eigenvalues.
   */
  svtkSetClampMacro(
    IntegrationEigenvector, int, SVTK_INTEGRATE_MAJOR_EIGENVECTOR, SVTK_INTEGRATE_MINOR_EIGENVECTOR);
  svtkGetMacro(IntegrationEigenvector, int);
  void SetIntegrationEigenvectorToMajor()
  {
    this->SetIntegrationEigenvector(SVTK_INTEGRATE_MAJOR_EIGENVECTOR);
  }
  void SetIntegrationEigenvectorToMedium()
  {
    this->SetIntegrationEigenvector(SVTK_INTEGRATE_MEDIUM_EIGENVECTOR);
  }
  void SetIntegrationEigenvectorToMinor()
  {
    this->SetIntegrationEigenvector(SVTK_INTEGRATE_MINOR_EIGENVECTOR);
  }
  //@}

  /**
   * Use the major eigenvector field as the vector field through which
   * to integrate.  The major eigenvector is the eigenvector whose
   * corresponding eigenvalue is closest to positive infinity.
   */
  void IntegrateMajorEigenvector() { this->SetIntegrationEigenvectorToMajor(); }

  /**
   * Use the medium eigenvector field as the vector field through which
   * to integrate. The medium eigenvector is the eigenvector whose
   * corresponding eigenvalue is between the major and minor
   * eigenvalues.
   */
  void IntegrateMediumEigenvector() { this->SetIntegrationEigenvectorToMedium(); }

  /**
   * Use the minor eigenvector field as the vector field through which
   * to integrate. The minor eigenvector is the eigenvector whose
   * corresponding eigenvalue is closest to negative infinity.
   */
  void IntegrateMinorEigenvector() { this->SetIntegrationEigenvectorToMinor(); }

  //@{
  /**
   * Set / get a nominal integration step size (expressed as a fraction of
   * the size of each cell).
   */
  svtkSetClampMacro(IntegrationStepLength, double, 0.001, 0.5);
  svtkGetMacro(IntegrationStepLength, double);
  //@}

  //@{
  /**
   * Set / get the length of a tube segment composing the
   * hyperstreamline. The length is specified as a fraction of the
   * diagonal length of the input bounding box.
   */
  svtkSetClampMacro(StepLength, double, 0.000001, 1.0);
  svtkGetMacro(StepLength, double);
  //@}

  //@{
  /**
   * Specify the direction in which to integrate the hyperstreamline.
   */
  svtkSetClampMacro(IntegrationDirection, int, SVTK_INTEGRATE_FORWARD, SVTK_INTEGRATE_BOTH_DIRECTIONS);
  svtkGetMacro(IntegrationDirection, int);
  void SetIntegrationDirectionToForward() { this->SetIntegrationDirection(SVTK_INTEGRATE_FORWARD); }
  void SetIntegrationDirectionToBackward()
  {
    this->SetIntegrationDirection(SVTK_INTEGRATE_BACKWARD);
  }
  void SetIntegrationDirectionToIntegrateBothDirections()
  {
    this->SetIntegrationDirection(SVTK_INTEGRATE_BOTH_DIRECTIONS);
  }
  //@}

  //@{
  /**
   * Set/get terminal eigenvalue.  If major eigenvalue falls below this
   * value, hyperstreamline terminates propagation.
   */
  svtkSetClampMacro(TerminalEigenvalue, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(TerminalEigenvalue, double);
  //@}

  //@{
  /**
   * Set / get the number of sides for the hyperstreamlines. At a minimum,
   * number of sides is 3.
   */
  svtkSetClampMacro(NumberOfSides, int, 3, SVTK_INT_MAX);
  svtkGetMacro(NumberOfSides, int);
  //@}

  //@{
  /**
   * Set / get the initial tube radius. This is the maximum "elliptical"
   * radius at the beginning of the tube. Radius varies based on ratio of
   * eigenvalues.  Note that tube section is actually elliptical and may
   * become a point or line in cross section in some cases.
   */
  svtkSetClampMacro(Radius, double, 0.0001, SVTK_DOUBLE_MAX);
  svtkGetMacro(Radius, double);
  //@}

  //@{
  /**
   * Turn on/off logarithmic scaling. If scaling is on, the log base 10
   * of the computed eigenvalues are used to scale the cross section radii.
   */
  svtkSetMacro(LogScaling, svtkTypeBool);
  svtkGetMacro(LogScaling, svtkTypeBool);
  svtkBooleanMacro(LogScaling, svtkTypeBool);
  //@}

protected:
  svtkHyperStreamline();
  ~svtkHyperStreamline() override;

  // Integrate data
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int BuildTube(svtkDataSet* input, svtkPolyData* output);

  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Flag indicates where streamlines start from (either position or location)
  int StartFrom;

  // Starting from cell location
  svtkIdType StartCell;
  int StartSubId;
  double StartPCoords[3];

  // starting from global x-y-z position
  double StartPosition[3];

  // array of hyperstreamlines
  svtkHyperArray* Streamers;
  int NumberOfStreamers;

  // length of hyperstreamline in absolute distance
  double MaximumPropagationDistance;

  // integration direction
  int IntegrationDirection;

  // the length (fraction of cell size) of integration steps
  double IntegrationStepLength;

  // the length of the tube segments composing the hyperstreamline
  double StepLength;

  // terminal propagation speed
  double TerminalEigenvalue;

  // number of sides of tube
  int NumberOfSides;

  // maximum radius of tube
  double Radius;

  // boolean controls whether scaling is clamped
  svtkTypeBool LogScaling;

  // which eigenvector to use as integration vector field
  int IntegrationEigenvector;

private:
  svtkHyperStreamline(const svtkHyperStreamline&) = delete;
  void operator=(const svtkHyperStreamline&) = delete;
};

#endif
