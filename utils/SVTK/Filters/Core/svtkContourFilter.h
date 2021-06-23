/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkContourFilter
 * @brief   generate isosurfaces/isolines from scalar values
 *
 * svtkContourFilter is a filter that takes as input any dataset and
 * generates on output isosurfaces and/or isolines. The exact form
 * of the output depends upon the dimensionality of the input data.
 * Data consisting of 3D cells will generate isosurfaces, data
 * consisting of 2D cells will generate isolines, and data with 1D
 * or 0D cells will generate isopoints. Combinations of output type
 * are possible if the input dimension is mixed.
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
 * are not computed. Use svtkPolyDataNormals to compute the surface
 * normals.
 *
 * @sa
 * svtkFlyingEdges3D svtkFlyingEdges2D svtkDiscreteFlyingEdges3D
 * svtkDiscreteFlyingEdges2D svtkMarchingContourFilter svtkMarchingCubes
 * svtkSliceCubes svtkMarchingSquares svtkImageMarchingCubes
 */

#ifndef svtkContourFilter_h
#define svtkContourFilter_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkContourValues.h" // Needed for inline methods

class svtkIncrementalPointLocator;
class svtkScalarTree;
class svtkSynchronizedTemplates2D;
class svtkSynchronizedTemplates3D;
class svtkGridSynchronizedTemplates3D;
class svtkRectilinearSynchronizedTemplates;
class svtkCallbackCommand;

class SVTKFILTERSCORE_EXPORT svtkContourFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkContourFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with initial range (0,1) and single contour value
   * of 0.0.
   */
  static svtkContourFilter* New();

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
   * This setting defaults to On for svtkImageData, svtkRectilinearGrid,
   * svtkStructuredGrid, and svtkUnstructuredGrid inputs, and Off for all others.
   * This default behavior is to preserve the behavior of an older version of
   * this filter, which would ignore this setting for certain inputs.
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
   * Enable the use of a scalar tree to accelerate contour extraction.
   */
  virtual void SetScalarTree(svtkScalarTree*);
  svtkGetObjectMacro(ScalarTree, svtkScalarTree);
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

  //@{
  /**
   * Set/get which component of the scalar array to contour on; defaults to 0.
   * Currently this feature only works if the input is a svtkImageData.
   */
  void SetArrayComponent(int);
  int GetArrayComponent();
  //@}

  //@{
  /**
   * If this is enabled (by default), the output will be triangles
   * otherwise, the output will be the intersection polygon
   * WARNING: if the contour surface is not planar, the output
   * polygon will not be planar, which might be nice to look at but hard
   * to compute with downstream.
   */
  svtkSetMacro(GenerateTriangles, svtkTypeBool);
  svtkGetMacro(GenerateTriangles, svtkTypeBool);
  svtkBooleanMacro(GenerateTriangles, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::Precision enum for an explanation of the available
   * precision settings.
   */
  void SetOutputPointsPrecision(int precision);
  int GetOutputPointsPrecision() const;
  //@}

protected:
  svtkContourFilter();
  ~svtkContourFilter() override;

  void ReportReferences(svtkGarbageCollector*) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkContourValues* ContourValues;
  svtkTypeBool ComputeNormals;
  svtkTypeBool ComputeGradients;
  svtkTypeBool ComputeScalars;
  svtkIncrementalPointLocator* Locator;
  svtkTypeBool UseScalarTree;
  svtkScalarTree* ScalarTree;
  int OutputPointsPrecision;
  svtkTypeBool GenerateTriangles;

  svtkSynchronizedTemplates2D* SynchronizedTemplates2D;
  svtkSynchronizedTemplates3D* SynchronizedTemplates3D;
  svtkGridSynchronizedTemplates3D* GridSynchronizedTemplates;
  svtkRectilinearSynchronizedTemplates* RectilinearSynchronizedTemplates;
  svtkCallbackCommand* InternalProgressCallbackCommand;

  static void InternalProgressCallbackFunction(
    svtkObject* caller, unsigned long eid, void* clientData, void* callData);

private:
  svtkContourFilter(const svtkContourFilter&) = delete;
  void operator=(const svtkContourFilter&) = delete;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void svtkContourFilter::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

/**
 * Get the ith contour value.
 */
inline double svtkContourFilter::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double* svtkContourFilter::GetValues()
{
  return this->ContourValues->GetValues();
}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void svtkContourFilter::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void svtkContourFilter::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

/**
 * Get the number of contours in the list of contour values.
 */
inline svtkIdType svtkContourFilter::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkContourFilter::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkContourFilter::GenerateValues(int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

#endif
