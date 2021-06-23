/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMarchingContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMarchingContourFilter
 * @brief   generate isosurfaces/isolines from scalar values
 *
 * svtkMarchingContourFilter is a filter that takes as input any dataset and
 * generates on output isosurfaces and/or isolines. The exact form
 * of the output depends upon the dimensionality of the input data.
 * Data consisting of 3D cells will generate isosurfaces, data
 * consisting of 2D cells will generate isolines, and data with 1D
 * or 0D cells will generate isopoints. Combinations of output type
 * are possible if the input dimension is mixed.
 *
 * This filter will identify special dataset types (e.g., structured
 * points) and use the appropriate specialized filter to process the
 * data. For examples, if the input dataset type is a volume, this
 * filter will create an internal svtkMarchingCubes instance and use
 * it. This gives much better performance.
 *
 * To use this filter you must specify one or more contour values.
 * You can either use the method SetValue() to specify each contour
 * value, or use GenerateValues() to generate a series of evenly
 * spaced contours. It is also possible to accelerate the operation of
 * this filter (at the cost of extra memory) by using a
 * svtkScalarTree. A scalar tree is used to quickly locate cells that
 * contain a contour surface. This is especially effective if multiple
 * contours are being extracted. If you want to use a scalar tree,
 * invoke the method UseScalarTreeOn().
 *
 * @warning
 * For unstructured data or structured grids, normals and gradients
 * are not computed.  This calculation will be implemented in the
 * future. In the mean time, use svtkPolyDataNormals to compute the surface
 * normals.
 *
 * @sa
 * svtkMarchingCubes svtkSliceCubes svtkDividingCubes svtkMarchingSquares
 * svtkImageMarchingCubes
 */

#ifndef svtkMarchingContourFilter_h
#define svtkMarchingContourFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkContourValues.h" // Needed for direct access to ContourValues

class svtkIncrementalPointLocator;
class svtkScalarTree;

class SVTKFILTERSGENERAL_EXPORT svtkMarchingContourFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkMarchingContourFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with initial range (0,1) and single contour value
   * of 0.0.
   */
  static svtkMarchingContourFilter* New();

  //@{
  /**
   * Methods to set / get contour values.
   */
  void SetValue(int i, double value);
  double GetValue(int i);
  double* GetValues();
  void GetValues(double* contourValues);
  void SetNumberOfContours(int number);
  svtkIdType GetNumberOfContours();
  void GenerateValues(int numContours, double range[2]);
  void GenerateValues(int numContours, double rangeStart, double rangeEnd);
  //@}

  /**
   * Modified GetMTime Because we delegate to svtkContourValues
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/Get the computation of normals. Normal computation is fairly
   * expensive in both time and storage. If the output data will be
   * processed by filters that modify topology or geometry, it may be
   * wise to turn Normals and Gradients off.
   */
  svtkSetMacro(ComputeNormals, svtkTypeBool);
  svtkGetMacro(ComputeNormals, svtkTypeBool);
  svtkBooleanMacro(ComputeNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the computation of gradients. Gradient computation is
   * fairly expensive in both time and storage. Note that if
   * ComputeNormals is on, gradients will have to be calculated, but
   * will not be stored in the output dataset.  If the output data
   * will be processed by filters that modify topology or geometry, it
   * may be wise to turn Normals and Gradients off.
   */
  svtkSetMacro(ComputeGradients, svtkTypeBool);
  svtkGetMacro(ComputeGradients, svtkTypeBool);
  svtkBooleanMacro(ComputeGradients, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the computation of scalars.
   */
  svtkSetMacro(ComputeScalars, svtkTypeBool);
  svtkGetMacro(ComputeScalars, svtkTypeBool);
  svtkBooleanMacro(ComputeScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * Enable the use of a scalar tree to accelerate contour extraction.
   */
  svtkSetMacro(UseScalarTree, svtkTypeBool);
  svtkGetMacro(UseScalarTree, svtkTypeBool);
  svtkBooleanMacro(UseScalarTree, svtkTypeBool);
  //@}

  //@{
  /**
   * Set / get a spatial locator for merging points. By default,
   * an instance of svtkMergePoints is used.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  /**
   * Create default locator. Used to create one when none is
   * specified. The locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

protected:
  svtkMarchingContourFilter();
  ~svtkMarchingContourFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkContourValues* ContourValues;
  svtkTypeBool ComputeNormals;
  svtkTypeBool ComputeGradients;
  svtkTypeBool ComputeScalars;
  svtkIncrementalPointLocator* Locator;
  svtkTypeBool UseScalarTree;
  svtkScalarTree* ScalarTree;

  // special contouring for structured points
  void StructuredPointsContour(int dim, svtkDataSet* input, svtkPolyData* output);
  // special contouring for image data
  void ImageContour(int dim, svtkDataSet* input, svtkPolyData* output);
  // default if not structured data
  void DataSetContour(svtkDataSet* input, svtkPolyData* output);

private:
  svtkMarchingContourFilter(const svtkMarchingContourFilter&) = delete;
  void operator=(const svtkMarchingContourFilter&) = delete;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void svtkMarchingContourFilter::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

/**
 * Get the ith contour value.
 */
inline double svtkMarchingContourFilter::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double* svtkMarchingContourFilter::GetValues()
{
  return this->ContourValues->GetValues();
}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void svtkMarchingContourFilter::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void svtkMarchingContourFilter::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

/**
 * Get the number of contours in the list of contour values.
 */
inline svtkIdType svtkMarchingContourFilter::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkMarchingContourFilter::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkMarchingContourFilter::GenerateValues(
  int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

#endif
