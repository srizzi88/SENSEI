/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPeriodicFiler.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

    This software is distributed WITHOUT ANY WARRANTY; without even
    the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
    PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkAngularPeriodicFilter
 * @brief   A filter to produce mapped angular periodic
 * multiblock dataset from a single block, by rotation.
 *
 *
 * Generate angular periodic dataset by rotating points, vectors and tensors
 * data arrays from an original data array.
 * The generated dataset is of the same type than the input (float or double).
 * To compute the rotation this filter needs
 * i) a number of periods, which can be the maximum, i.e. a full period,
 * ii) an angle, which can be fetched from a field data array in radian or directly
 * in degrees; iii) the axis (X, Y or Z) and the center of rotation.
 * Point coordinates are transformed, as well as all vectors (3-components) and
 * tensors (9 components) in points and cell data arrays.
 * The generated multiblock will have the same tree architecture than the input,
 * except transformed leaves are replaced by a svtkMultipieceDataSet.
 * Supported input leaf dataset type are: svtkPolyData, svtkStructuredGrid
 * and svtkUnstructuredGrid. Other data objects are rotated using the
 * transform filter (at a high cost!).
 */

#ifndef svtkAngularPeriodicFilter_h
#define svtkAngularPeriodicFilter_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkPeriodicFilter.h"

class svtkDataSetAttributes;
class svtkMultiPieceDataSet;
class svtkPointSet;

#define SVTK_ROTATION_MODE_DIRECT_ANGLE 0 // Use user-provided angle
#define SVTK_ROTATION_MODE_ARRAY_VALUE 1  // Use array from input data as angle

class SVTKFILTERSPARALLEL_EXPORT svtkAngularPeriodicFilter : public svtkPeriodicFilter
{
public:
  static svtkAngularPeriodicFilter* New();
  svtkTypeMacro(svtkAngularPeriodicFilter, svtkPeriodicFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get whether the rotated array values should be computed
   * on-the-fly (default), which is compute-intensive, or the arrays should be
   * explicitly generated and stored, at the cost of using more memory.
   */
  svtkSetMacro(ComputeRotationsOnTheFly, bool);
  svtkGetMacro(ComputeRotationsOnTheFly, bool);
  svtkBooleanMacro(ComputeRotationsOnTheFly, bool);
  //@}

  //@{
  /**
   * Set/Get The rotation mode.
   * SVTK_ROTATION_MODE_DIRECT_ANGLE to specify an angle value (default),
   * SVTK_ROTATION_MODE_ARRAY_VALUE to use value from an array in the input dataset.
   */
  svtkSetClampMacro(
    RotationMode, int, SVTK_ROTATION_MODE_DIRECT_ANGLE, SVTK_ROTATION_MODE_ARRAY_VALUE);
  svtkGetMacro(RotationMode, int);
  void SetRotationModeToDirectAngle() { this->SetRotationMode(SVTK_ROTATION_MODE_DIRECT_ANGLE); }
  void SetRotationModeToArrayValue() { this->SetRotationMode(SVTK_ROTATION_MODE_ARRAY_VALUE); }
  //@}

  //@{
  /**
   * Set/Get Rotation angle, in degrees.
   * Used only with SVTK_ROTATION_MODE_DIRECT_ANGLE.
   * Default is 180.
   */
  svtkSetMacro(RotationAngle, double);
  svtkGetMacro(RotationAngle, double);
  //@}

  //@{
  /**
   * Set/Get Name of array to get the angle from.
   * Used only with SVTK_ROTATION_MODE_ARRAY_VALUE.
   */
  svtkSetStringMacro(RotationArrayName);
  svtkGetStringMacro(RotationArrayName);
  //@}

  //@{
  /**
   * Set/Get Rotation Axis, 0 for X, 1 for Y, 2 for Z
   */
  svtkSetClampMacro(RotationAxis, int, 0, 2);
  svtkGetMacro(RotationAxis, int);
  void SetRotationAxisToX();
  void SetRotationAxisToY();
  void SetRotationAxisToZ();
  //@}

  //@{
  /**
   * Set/Get Rotation Center
   */
  svtkSetVector3Macro(Center, double);
  svtkGetVector3Macro(Center, double);
  //@}

protected:
  svtkAngularPeriodicFilter();
  ~svtkAngularPeriodicFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Create a transform copy of the provided data array
   */
  svtkDataArray* TransformDataArray(
    svtkDataArray* inputArray, double angle, bool useCenter = true, bool normalize = false);

  /**
   * Append a periodic piece to dataset, by computing rotated mesh and data
   */
  void AppendPeriodicPiece(
    double angle, svtkIdType iPiece, svtkDataObject* inputNode, svtkMultiPieceDataSet* multiPiece);

  /**
   * Manually set the number of period on a specific leaf
   */
  void SetPeriodNumber(
    svtkCompositeDataIterator* loc, svtkCompositeDataSet* output, int nbPeriod) override;

  /**
   * Compute periodic pointset, rotating point, using provided angle
   */
  void ComputePeriodicMesh(svtkPointSet* dataset, svtkPointSet* rotatedDataset, double angle);

  /**
   * Compute periodic point/cell data, using provided angle
   */
  void ComputeAngularPeriodicData(
    svtkDataSetAttributes* data, svtkDataSetAttributes* rotatedData, double angle);

  /**
   * Create a periodic data, leaf of the tree
   */
  void CreatePeriodicDataSet(svtkCompositeDataIterator* loc, svtkCompositeDataSet* output,
    svtkCompositeDataSet* input) override;

  /**
   * Generate a name for a piece in the periodic dataset from the input dataset
   */
  virtual void GeneratePieceName(svtkCompositeDataSet* input, svtkCompositeDataIterator* inputLoc,
    svtkMultiPieceDataSet* output, svtkIdType outputId);

private:
  svtkAngularPeriodicFilter(const svtkAngularPeriodicFilter&) = delete;
  void operator=(const svtkAngularPeriodicFilter&) = delete;

  bool ComputeRotationsOnTheFly;

  int RotationMode;
  char*
    RotationArrayName; // user-provided array name to use as angle, for ROTATION_MODE_ARRAY_VALUE

  // Transform parameters
  double RotationAngle;
  int RotationAxis; // Axis to rotate around, 0 for X, 1 for Y, 2 for Z
  double Center[3]; // Center of rotation
};

#endif
