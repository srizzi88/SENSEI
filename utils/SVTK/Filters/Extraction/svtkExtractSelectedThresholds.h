/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedThresholds.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractSelectedThresholds
 * @brief   extract a cells or points from a
 * dataset that have values within a set of thresholds.
 *
 *
 * svtkExtractSelectedThresholds extracts all cells and points with attribute
 * values that lie within a svtkSelection's THRESHOLD contents. The selecion
 * can specify to threshold a particular array within either the point or cell
 * attribute data of the input. This is similar to svtkThreshold
 * but allows multiple thresholds ranges.
 * This filter adds a scalar array called svtkOriginalCellIds that says what
 * input cell produced each output cell. This is an example of a Pedigree ID
 * which helps to trace back results.
 *
 * @sa
 * svtkSelection svtkExtractSelection svtkThreshold
 */

#ifndef svtkExtractSelectedThresholds_h
#define svtkExtractSelectedThresholds_h

#include "svtkExtractSelectionBase.h"
#include "svtkFiltersExtractionModule.h" // For export macro

class svtkDataArray;
class svtkSelection;
class svtkSelectionNode;
class svtkTable;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractSelectedThresholds : public svtkExtractSelectionBase
{
public:
  svtkTypeMacro(svtkExtractSelectedThresholds, svtkExtractSelectionBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Constructor
   */
  static svtkExtractSelectedThresholds* New();

  /**
   * Function for determining whether a value in a data array passes
   * the threshold test(s) provided in lims.  Returns 1 if the value
   * passes at least one of the threshold tests.
   * If \c scalars is nullptr, then the id itself is used as the scalar value.
   */
  static int EvaluateValue(svtkDataArray* scalars, svtkIdType id, svtkDataArray* lims)
  {
    return svtkExtractSelectedThresholds::EvaluateValue(scalars, 0, id, lims);
  }

  /**
   * Same as the other EvaluateValue except that the component to be compared
   * can be picked using array_component_no (use -1 for magnitude).
   * If \c scalars is nullptr, then the id itself is used as the scalar value.
   */
  static int EvaluateValue(
    svtkDataArray* array, int array_component_no, svtkIdType id, svtkDataArray* lims);

  /**
   * Function for determining whether a value in a data array passes
   * the threshold test(s) provided in lims.  Returns 1 if the value
   * passes at least one of the threshold tests.  Also returns in
   * AboveCount, BelowCount and InsideCount the number of tests where
   * the value was above, below or inside the interval.
   * If \c scalars is nullptr, then the id itself is used as the scalar value.
   */
  static int EvaluateValue(svtkDataArray* scalars, svtkIdType id, svtkDataArray* lims, int* AboveCount,
    int* BelowCount, int* InsideCount)
  {
    return svtkExtractSelectedThresholds::EvaluateValue(
      scalars, 0, id, lims, AboveCount, BelowCount, InsideCount);
  }

  /**
   * Same as the other EvaluateValue except that the component to be compared
   * can be picked using array_component_no (use -1 for magnitude).
   * If \c scalars is nullptr, then the id itself is used as the scalar value.
   */
  static int EvaluateValue(svtkDataArray* scalars, int array_component_no, svtkIdType id,
    svtkDataArray* lims, int* AboveCount, int* BelowCount, int* InsideCount);

protected:
  svtkExtractSelectedThresholds();
  ~svtkExtractSelectedThresholds() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int ExtractCells(
    svtkSelectionNode* sel, svtkDataSet* input, svtkDataSet* output, int usePointScalars);
  int ExtractPoints(svtkSelectionNode* sel, svtkDataSet* input, svtkDataSet* output);

  int ExtractRows(svtkSelectionNode* sel, svtkTable* input, svtkTable* output);

private:
  svtkExtractSelectedThresholds(const svtkExtractSelectedThresholds&) = delete;
  void operator=(const svtkExtractSelectedThresholds&) = delete;
};

#endif
