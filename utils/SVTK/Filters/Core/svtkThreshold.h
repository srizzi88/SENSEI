/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThreshold.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkThreshold
 * @brief   extracts cells where scalar value in cell satisfies threshold criterion
 *
 * svtkThreshold is a filter that extracts cells from any dataset type that
 * satisfy a threshold criterion. A cell satisfies the criterion if the
 * scalar value of (every or any) point satisfies the criterion. The
 * criterion can take three forms: 1) greater than a particular value; 2)
 * less than a particular value; or 3) between two values. The output of this
 * filter is an unstructured grid.
 *
 * Note that scalar values are available from the point and cell attribute
 * data.  By default, point data is used to obtain scalars, but you can
 * control this behavior. See the AttributeMode ivar below.
 *
 * By default only the first scalar value is used in the decision. Use the ComponentMode
 * and SelectedComponent ivars to control this behavior.
 *
 * @sa
 * svtkThresholdPoints svtkThresholdTextureCoords
 */

#ifndef svtkThreshold_h
#define svtkThreshold_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

#define SVTK_ATTRIBUTE_MODE_DEFAULT 0
#define SVTK_ATTRIBUTE_MODE_USE_POINT_DATA 1
#define SVTK_ATTRIBUTE_MODE_USE_CELL_DATA 2

// order / values are important because of the SetClampMacro
#define SVTK_COMPONENT_MODE_USE_SELECTED 0
#define SVTK_COMPONENT_MODE_USE_ALL 1
#define SVTK_COMPONENT_MODE_USE_ANY 2

class svtkDataArray;
class svtkIdList;

class SVTKFILTERSCORE_EXPORT svtkThreshold : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkThreshold* New();
  svtkTypeMacro(svtkThreshold, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Criterion is cells whose scalars are less or equal to lower threshold.
   */
  void ThresholdByLower(double lower);

  /**
   * Criterion is cells whose scalars are greater or equal to upper threshold.
   */
  void ThresholdByUpper(double upper);

  /**
   * Criterion is cells whose scalars are between lower and upper thresholds
   * (inclusive of the end values).
   */
  void ThresholdBetween(double lower, double upper);

  //@{
  /**
   * Get the Upper and Lower thresholds.
   */
  svtkGetMacro(UpperThreshold, double);
  svtkGetMacro(LowerThreshold, double);
  //@}

  //@{
  /**
   * Control how the filter works with scalar point data and cell attribute
   * data.  By default (AttributeModeToDefault), the filter will use point
   * data, and if no point data is available, then cell data is
   * used. Alternatively you can explicitly set the filter to use point data
   * (AttributeModeToUsePointData) or cell data (AttributeModeToUseCellData).
   */
  svtkSetMacro(AttributeMode, int);
  svtkGetMacro(AttributeMode, int);
  void SetAttributeModeToDefault() { this->SetAttributeMode(SVTK_ATTRIBUTE_MODE_DEFAULT); }
  void SetAttributeModeToUsePointData()
  {
    this->SetAttributeMode(SVTK_ATTRIBUTE_MODE_USE_POINT_DATA);
  }
  void SetAttributeModeToUseCellData() { this->SetAttributeMode(SVTK_ATTRIBUTE_MODE_USE_CELL_DATA); }
  const char* GetAttributeModeAsString();
  //@}

  //@{
  /**
   * Control how the decision of in / out is made with multi-component data.
   * The choices are to use the selected component (specified in the
   * SelectedComponent ivar), or to look at all components. When looking at
   * all components, the evaluation can pass if all the components satisfy
   * the rule (UseAll) or if any satisfy is (UseAny). The default value is
   * UseSelected.
   */
  svtkSetClampMacro(ComponentMode, int, SVTK_COMPONENT_MODE_USE_SELECTED, SVTK_COMPONENT_MODE_USE_ANY);
  svtkGetMacro(ComponentMode, int);
  void SetComponentModeToUseSelected() { this->SetComponentMode(SVTK_COMPONENT_MODE_USE_SELECTED); }
  void SetComponentModeToUseAll() { this->SetComponentMode(SVTK_COMPONENT_MODE_USE_ALL); }
  void SetComponentModeToUseAny() { this->SetComponentMode(SVTK_COMPONENT_MODE_USE_ANY); }
  const char* GetComponentModeAsString();
  //@}

  //@{
  /**
   * When the component mode is UseSelected, this ivar indicated the selected
   * component. The default value is 0.
   */
  svtkSetClampMacro(SelectedComponent, int, 0, SVTK_INT_MAX);
  svtkGetMacro(SelectedComponent, int);
  //@}

  //@{
  /**
   * If using scalars from point data, all scalars for all points in a cell
   * must satisfy the threshold criterion if AllScalars is set. Otherwise,
   * just a single scalar value satisfying the threshold criterion enables
   * will extract the cell.
   */
  svtkSetMacro(AllScalars, svtkTypeBool);
  svtkGetMacro(AllScalars, svtkTypeBool);
  svtkBooleanMacro(AllScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * If this is on (default is off), we will use the continuous interval
   * [minimum cell scalar, maxmimum cell scalar] to intersect the threshold bound
   * , rather than the set of discrete scalar values from the vertices
   * *WARNING*: For higher order cells, the scalar range of the cell is
   * not the same as the vertex scalar interval used here, so the
   * result will not be accurate.
   */
  svtkSetMacro(UseContinuousCellRange, svtkTypeBool);
  svtkGetMacro(UseContinuousCellRange, svtkTypeBool);
  svtkBooleanMacro(UseContinuousCellRange, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the data type of the output points (See the data types defined in
   * svtkType.h). The default data type is float.

   * These methods are deprecated. Please use the SetOutputPointsPrecision()
   * and GetOutputPointsPrecision() methods instead.
   */
  void SetPointsDataTypeToDouble() { this->SetPointsDataType(SVTK_DOUBLE); }
  void SetPointsDataTypeToFloat() { this->SetPointsDataType(SVTK_FLOAT); }
  void SetPointsDataType(int type);
  int GetPointsDataType();
  //@}

  //@{
  /**
   * Invert the threshold results. That is, cells that would have been in the output with this
   * option off are excluded, while cells that would have been excluded from the output are
   * included.
   */
  svtkSetMacro(Invert, bool);
  svtkGetMacro(Invert, bool);
  svtkBooleanMacro(Invert, bool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  void SetOutputPointsPrecision(int precision);
  int GetOutputPointsPrecision() const;
  //@}

  //@{
  /**
   * Methods used for thresholding. svtkThreshold::Lower returns true if s is lower than threshold,
   * svtkThreshold::Upper returns true if s is upper than treshold, and svtkThreshold::Between returns
   * true if s is between two threshold.
   *
   * @warning svtkThreshold::Lower and svtkThreshold::Upper use different thresholds which are set
   * using the methods svtkThreshold::ThresholdByLower and svtkThreshold::ThresholdByUpper
   * respectively. svtkThreshold::ThresholdBetween sets both thresholds. Do not use those methods
   * without priorly setting the corresponding threshold.
   *
   * @note They are not protected member for inheritance purposes. The addresses of those methods is
   * stored in one of this class attributes to figure out which version of the threshold to apply,
   * which are inaccessible if protected.
   */
  int Lower(double s) const;
  int Upper(double s) const;
  int Between(double s) const;
  //@}
protected:
  svtkThreshold();
  ~svtkThreshold() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkTypeBool AllScalars;
  double LowerThreshold;
  double UpperThreshold;
  int AttributeMode;
  int ComponentMode;
  int SelectedComponent;
  int OutputPointsPrecision;
  svtkTypeBool UseContinuousCellRange;
  bool Invert;

  int (svtkThreshold::*ThresholdFunction)(double s) const;

  int EvaluateComponents(svtkDataArray* scalars, svtkIdType id);
  int EvaluateCell(svtkDataArray* scalars, svtkIdList* cellPts, int numCellPts);
  int EvaluateCell(svtkDataArray* scalars, int c, svtkIdList* cellPts, int numCellPts);

private:
  svtkThreshold(const svtkThreshold&) = delete;
  void operator=(const svtkThreshold&) = delete;
};

#endif
