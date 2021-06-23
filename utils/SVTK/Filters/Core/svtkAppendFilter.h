/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAppendFilter
 * @brief   appends one or more datasets together into a single unstructured grid
 *
 * svtkAppendFilter is a filter that appends one of more datasets into a single
 * unstructured grid. All geometry is extracted and appended, but point
 * attributes (i.e., scalars, vectors, normals, field data, etc.) are extracted
 * and appended only if all datasets have the point attributes available.
 * (For example, if one dataset has scalars but another does not, scalars will
 * not be appended.)
 *
 * @sa
 * svtkAppendPolyData
 */

#ifndef svtkAppendFilter_h
#define svtkAppendFilter_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkDataSetAttributes;
class svtkDataSetCollection;

class SVTKFILTERSCORE_EXPORT svtkAppendFilter : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkAppendFilter* New();
  svtkTypeMacro(svtkAppendFilter, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get any input of this filter.
   */
  svtkDataSet* GetInput(int idx);
  svtkDataSet* GetInput() { return this->GetInput(0); }
  //@}

  //@{
  /**
   * Get/Set if the filter should merge coincidental points
   * Note: The filter will only merge points if the ghost cell array doesn't exist
   * Defaults to Off
   */
  svtkGetMacro(MergePoints, svtkTypeBool);
  svtkSetMacro(MergePoints, svtkTypeBool);
  svtkBooleanMacro(MergePoints, svtkTypeBool);
  //@}

  //@{
  /**
   * Get/Set the tolerance to use to find coincident points when `MergePoints`
   * is `true`. Default is 0.0.
   *
   * This is simply passed on to the internal svtkLocator used to merge points.
   * @sa `svtkLocator::SetTolerance`.
   */
  svtkSetClampMacro(Tolerance, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(Tolerance, double);
  //@}

  //@{
  /**
   * Get/Set whether Tolerance is treated as an absolute or relative tolerance.
   * The default is to treat it as an absolute tolerance. When off, the
   * tolerance is multiplied by the diagonal of the bounding box of the input.
   */
  svtkSetMacro(ToleranceIsAbsolute, bool);
  svtkGetMacro(ToleranceIsAbsolute, bool);
  svtkBooleanMacro(ToleranceIsAbsolute, bool);
  //@}

  /**
   * Remove a dataset from the list of data to append.
   */
  void RemoveInputData(svtkDataSet* in);

  /**
   * Returns a copy of the input array.  Modifications to this list
   * will not be reflected in the actual inputs.
   */
  svtkDataSetCollection* GetInputList();

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::Precision enum for an explanation of the available
   * precision settings.
   */
  svtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkAppendFilter();
  ~svtkAppendFilter() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  // list of data sets to append together.
  // Here as a convenience.  It is a copy of the input array.
  svtkDataSetCollection* InputList;

  // If true we will attempt to merge points. Must also not have
  // ghost cells defined.
  svtkTypeBool MergePoints;

  int OutputPointsPrecision;
  double Tolerance;

  // If true, tolerance is used as is. If false, tolerance is multiplied by
  // the diagonal of the bounding box of the input.
  bool ToleranceIsAbsolute;

private:
  svtkAppendFilter(const svtkAppendFilter&) = delete;
  void operator=(const svtkAppendFilter&) = delete;

  // Get all input data sets that have points, cells, or both.
  // Caller must delete the returned svtkDataSetCollection.
  svtkDataSetCollection* GetNonEmptyInputs(svtkInformationVector** inputVector);

  void AppendArrays(int attributesType, svtkInformationVector** inputVector, svtkIdType* globalIds,
    svtkUnstructuredGrid* output, svtkIdType totalNumberOfElements);
};

#endif
