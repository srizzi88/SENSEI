/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGenericContourFilter
 * @brief   generate isocontours from input dataset
 *
 * svtkGenericContourFilter is a filter that takes as input any (generic)
 * dataset and generates on output isosurfaces and/or isolines. The exact
 * form of the output depends upon the dimensionality of the input data.
 * Data consisting of 3D cells will generate isosurfaces, data consisting of
 * 2D cells will generate isolines, and data with 1D or 0D cells will
 * generate isopoints. Combinations of output type are possible if the input
 * dimension is mixed.
 *
 * To use this filter you must specify one or more contour values.
 * You can either use the method SetValue() to specify each contour
 * value, or use GenerateValues() to generate a series of evenly
 * spaced contours. You can use ComputeNormalsOn to compute the normals
 * without the need of a svtkPolyDataNormals
 *
 * This filter has been implemented to operate on generic datasets, rather
 * than the typical svtkDataSet (and subclasses). svtkGenericDataSet is a more
 * complex cousin of svtkDataSet, typically consisting of nonlinear,
 * higher-order cells. To process this type of data, generic cells are
 * automatically tessellated into linear cells prior to isocontouring.
 *
 * @sa
 * svtkContourFilter svtkGenericDataSet
 */

#ifndef svtkGenericContourFilter_h
#define svtkGenericContourFilter_h

#include "svtkFiltersGenericModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkContourValues;
class svtkIncrementalPointLocator;
class svtkPointData;
class svtkCellData;

class SVTKFILTERSGENERIC_EXPORT svtkGenericContourFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkGenericContourFilter, svtkPolyDataAlgorithm);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with initial range (0,1) and single contour value
   * of 0.0.
   */
  static svtkGenericContourFilter* New();

  typedef double PointType[3]; // Arbitrary definition of a point

  //@{
  /**
   * Methods to set / get contour values.
   */
  void SetValue(int i, float value);
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
   * If you want to contour by an arbitrary scalar attribute, then set its
   * name here.
   * By default this in nullptr and the filter will use the active scalar array.
   */
  svtkGetStringMacro(InputScalarsSelection);
  virtual void SelectInputScalars(const char* fieldName);
  //@}

protected:
  svtkGenericContourFilter();
  ~svtkGenericContourFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int, svtkInformation*) override;

  svtkContourValues* ContourValues;
  svtkTypeBool ComputeNormals;
  svtkTypeBool ComputeGradients;
  svtkTypeBool ComputeScalars;
  svtkIncrementalPointLocator* Locator;

  char* InputScalarsSelection;
  svtkSetStringMacro(InputScalarsSelection);

  // Used internal by svtkGenericAdaptorCell::Contour()
  svtkPointData* InternalPD;
  svtkPointData* SecondaryPD;
  svtkCellData* SecondaryCD;

private:
  svtkGenericContourFilter(const svtkGenericContourFilter&) = delete;
  void operator=(const svtkGenericContourFilter&) = delete;
};
#endif
