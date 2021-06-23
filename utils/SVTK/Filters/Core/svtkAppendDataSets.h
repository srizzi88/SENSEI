/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendDataSets.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAppendDataSets
 * @brief   Appends one or more datasets together into a single output svtkPointSet.
 *
 * svtkAppendDataSets is a filter that appends one of more datasets into a single output
 * point set. The type of the output is set with the OutputDataSetType option. Only inputs
 * that can be converted to the selected output dataset type are appended to the output.
 * By default, the output is svtkUnstructuredGrid, and all input types can be appended to it.
 * If the OutputDataSetType is set to produce svtkPolyData, then only datasets that can be
 * converted to svtkPolyData (i.e., svtkPolyData) are appended to the output.
 *
 * All cells are extracted and appended, but point and cell attributes (i.e., scalars,
 * vectors, normals, field data, etc.) are extracted and appended only if all datasets
 * have the same point and/or cell attributes available. (For example, if one dataset
 * has scalars but another does not, scalars will not be appended.)
 *
 * @sa
 * svtkAppendFilter svtkAppendPolyData
 */

#ifndef svtkAppendDataSets_h
#define svtkAppendDataSets_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPointSetAlgorithm.h"

class svtkDataSet;
class svtkDataSetCollection;

class SVTKFILTERSCORE_EXPORT svtkAppendDataSets : public svtkPointSetAlgorithm
{
public:
  static svtkAppendDataSets* New();
  svtkTypeMacro(svtkAppendDataSets, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set if the filter should merge coincidental points
   * Note: The filter will only merge points if the ghost cell array doesn't exist
   * Defaults to Off
   */
  svtkGetMacro(MergePoints, bool);
  svtkSetMacro(MergePoints, bool);
  svtkBooleanMacro(MergePoints, bool);
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

  //@{
  /**
   * Get/Set the output type produced by this filter. Only input datasets compatible with the
   * output type will be merged in the output. For example, if the output type is svtkPolyData, then
   * blocks of type svtkImageData, svtkStructuredGrid, etc. will not be merged - only svtkPolyData
   * can be merged into a svtkPolyData. On the other hand, if the output type is
   * svtkUnstructuredGrid, then blocks of almost any type will be merged in the output.
   * Valid values are SVTK_POLY_DATA and SVTK_UNSTRUCTURED_GRID defined in svtkType.h.
   * Defaults to SVTK_UNSTRUCTURED_GRID.
   */
  svtkSetMacro(OutputDataSetType, int);
  svtkGetMacro(OutputDataSetType, int);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::Precision enum for an explanation of the available
   * precision settings.
   */
  svtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkAppendDataSets();
  ~svtkAppendDataSets() override;

  // Usual data generation method
  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  int FillInputPortInformation(int port, svtkInformation* info) override;

  // If true we will attempt to merge points. Must also not have
  // ghost cells defined.
  bool MergePoints;

  // Tolerance used for point merging
  double Tolerance;

  // If true, tolerance is used as is. If false, tolerance is multiplied by
  // the diagonal of the bounding box of the input.
  bool ToleranceIsAbsolute;

  // Output data set type.
  int OutputDataSetType;

  // Precision of output points.
  int OutputPointsPrecision;

private:
  svtkAppendDataSets(const svtkAppendDataSets&) = delete;
  void operator=(const svtkAppendDataSets&) = delete;

  // Get all input data sets that have points, cells, or both.
  // Caller must delete the returned svtkDataSetCollection.
  svtkDataSetCollection* GetNonEmptyInputs(svtkInformationVector** inputVector);
};

#endif
