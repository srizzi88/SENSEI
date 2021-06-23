/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContourGrid.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkContourGrid
 * @brief   generate isosurfaces/isolines from scalar values (specialized for unstructured grids)
 *
 * svtkContourGrid is a filter that takes as input datasets of type
 * svtkUnstructuredGrid and generates on output isosurfaces and/or
 * isolines. The exact form of the output depends upon the dimensionality of
 * the input data.  Data consisting of 3D cells will generate isosurfaces,
 * data consisting of 2D cells will generate isolines, and data with 1D or 0D
 * cells will generate isopoints. Combinations of output type are possible if
 * the input dimension is mixed.
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
 * If the input svtkUnstructuredGrid contains 3D linear cells, the class
 * svtkContour3DLinearGrid is much faster and may be preferred in certain
 * applications.
 *
 * @warning
 * For unstructured data or structured grids, normals and gradients
 * are not computed. Use svtkPolyDataNormals to compute the surface
 * normals of the resulting isosurface.
 *
 * @sa
 * svtkContour3DLinearGrid svtkContourFilter svtkMarchingContourFilter
 * svtkFlyingEdges3D svtkMarchingCubes svtkSliceCubes svtkDividingCubes
 * svtkMarchingSquares svtkImageMarchingCubes
 */

#ifndef svtkContourGrid_h
#define svtkContourGrid_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkContourValues.h" // Needed for inline methods

class svtkEdgeTable;
class svtkScalarTree;
class svtkIncrementalPointLocator;

class SVTKFILTERSCORE_EXPORT svtkContourGrid : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkContourGrid, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with initial range (0,1) and single contour value
   * of 0.0.
   */
  static svtkContourGrid* New();

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
   * may be wise to turn Normals and Gradients off.  @deprecated
   * ComputeGradients is not used so these methods don't affect
   * anything (SVTK 6.0).
   */
#ifndef SVTK_LEGACY_REMOVE
  svtkSetMacro(ComputeGradients, svtkTypeBool);
  svtkGetMacro(ComputeGradients, svtkTypeBool);
  svtkBooleanMacro(ComputeGradients, svtkTypeBool);
#endif
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
   * Specify the instance of svtkScalarTree to use. If not specified
   * and UseScalarTree is enabled, then a svtkSimpleScalarTree will be used.
   */
  void SetScalarTree(svtkScalarTree* sTree);
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

  //@{
  /**
   * If this is enabled (by default), the output will be triangles otherwise,
   * the output may be represented by one or more polygons. WARNING: if the
   * resulting isocontour is not planar, and GenerateTriangles is false, the
   * output may consist of some 3D polygons (i.e., which may be non-planar) -
   * which might be nice to look at but hard to compute with downstream.
   */
  svtkSetMacro(GenerateTriangles, svtkTypeBool);
  svtkGetMacro(GenerateTriangles, svtkTypeBool);
  svtkBooleanMacro(GenerateTriangles, svtkTypeBool);
  //@}

  /**
   * Create default locator. Used to create one when none is
   * specified. The locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  void SetOutputPointsPrecision(int precision);
  int GetOutputPointsPrecision() const;
  //@}

protected:
  svtkContourGrid();
  ~svtkContourGrid() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkContourValues* ContourValues;
  svtkTypeBool ComputeNormals;
#ifndef SVTK_LEGACY_REMOVE
  svtkTypeBool ComputeGradients;
#endif
  svtkTypeBool ComputeScalars;
  svtkTypeBool GenerateTriangles;

  svtkIncrementalPointLocator* Locator;

  svtkTypeBool UseScalarTree;
  svtkScalarTree* ScalarTree;

  int OutputPointsPrecision;
  svtkEdgeTable* EdgeTable;

private:
  svtkContourGrid(const svtkContourGrid&) = delete;
  void operator=(const svtkContourGrid&) = delete;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void svtkContourGrid::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

/**
 * Get the ith contour value.
 */
inline double svtkContourGrid::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double* svtkContourGrid::GetValues()
{
  return this->ContourValues->GetValues();
}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void svtkContourGrid::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void svtkContourGrid::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

/**
 * Get the number of contours in the list of contour values.
 */
inline svtkIdType svtkContourGrid::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkContourGrid::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkContourGrid::GenerateValues(int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

#endif
