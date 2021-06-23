/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCutter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCutter
 * @brief   Cut svtkDataSet with user-specified implicit function
 *
 * svtkCutter is a filter to cut through data using any subclass of
 * svtkImplicitFunction. That is, a polygonal surface is created
 * corresponding to the implicit function F(x,y,z) = value(s), where
 * you can specify one or more values used to cut with.
 *
 * In SVTK, cutting means reducing a cell of dimension N to a cut surface
 * of dimension N-1. For example, a tetrahedron when cut by a plane (i.e.,
 * svtkPlane implicit function) will generate triangles. (In comparison,
 * clipping takes a N dimensional cell and creates N dimension primitives.)
 *
 * svtkCutter is generally used to "slice-through" a dataset, generating
 * a surface that can be visualized. It is also possible to use svtkCutter
 * to do a form of volume rendering. svtkCutter does this by generating
 * multiple cut surfaces (usually planes) which are ordered (and rendered)
 * from back-to-front. The surfaces are set translucent to give a
 * volumetric rendering effect.
 *
 * Note that data can be cut using either 1) the scalar values associated
 * with the dataset or 2) an implicit function associated with this class.
 * By default, if an implicit function is set it is used to clip the data
 * set, otherwise the dataset scalars are used to perform the clipping.
 *
 * @sa
 * svtkImplicitFunction svtkClipPolyData
 */

#ifndef svtkCutter_h
#define svtkCutter_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkContourValues.h" // Needed for inline methods

#define SVTK_SORT_BY_VALUE 0
#define SVTK_SORT_BY_CELL 1

class svtkImplicitFunction;
class svtkIncrementalPointLocator;
class svtkSynchronizedTemplates3D;
class svtkSynchronizedTemplatesCutter3D;
class svtkGridSynchronizedTemplates3D;
class svtkRectilinearSynchronizedTemplates;

class SVTKFILTERSCORE_EXPORT svtkCutter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkCutter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with user-specified implicit function; initial value of 0.0; and
   * generating cut scalars turned off.
   */
  static svtkCutter* New();

  /**
   * Set a particular contour value at contour number i. The index i ranges
   * between 0<=i<NumberOfContours.
   */
  void SetValue(int i, double value) { this->ContourValues->SetValue(i, value); }

  /**
   * Get the ith contour value.
   */
  double GetValue(int i) { return this->ContourValues->GetValue(i); }

  /**
   * Get a pointer to an array of contour values. There will be
   * GetNumberOfContours() values in the list.
   */
  double* GetValues() { return this->ContourValues->GetValues(); }

  /**
   * Fill a supplied list with contour values. There will be
   * GetNumberOfContours() values in the list. Make sure you allocate
   * enough memory to hold the list.
   */
  void GetValues(double* contourValues) { this->ContourValues->GetValues(contourValues); }

  /**
   * Set the number of contours to place into the list. You only really
   * need to use this method to reduce list size. The method SetValue()
   * will automatically increase list size as needed.
   */
  void SetNumberOfContours(int number) { this->ContourValues->SetNumberOfContours(number); }

  /**
   * Get the number of contours in the list of contour values.
   */
  svtkIdType GetNumberOfContours() { return this->ContourValues->GetNumberOfContours(); }

  /**
   * Generate numContours equally spaced contour values between specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double range[2])
  {
    this->ContourValues->GenerateValues(numContours, range);
  }

  /**
   * Generate numContours equally spaced contour values between specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double rangeStart, double rangeEnd)
  {
    this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
  }

  /**
   * Override GetMTime because we delegate to svtkContourValues and refer to
   * svtkImplicitFunction.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Specify the implicit function to perform the cutting.
   */
  virtual void SetCutFunction(svtkImplicitFunction*);
  svtkGetObjectMacro(CutFunction, svtkImplicitFunction);
  //@}

  //@{
  /**
   * If this flag is enabled, then the output scalar values will be
   * interpolated from the implicit function values, and not the input scalar
   * data.
   */
  svtkSetMacro(GenerateCutScalars, svtkTypeBool);
  svtkGetMacro(GenerateCutScalars, svtkTypeBool);
  svtkBooleanMacro(GenerateCutScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * If this is enabled (by default), the output will be triangles
   * otherwise, the output will be the intersection polygons
   * WARNING: if the cutting function is not a plane, the output
   * will be 3D poygons, which might be nice to look at but hard
   * to compute with downstream.
   */
  svtkSetMacro(GenerateTriangles, svtkTypeBool);
  svtkGetMacro(GenerateTriangles, svtkTypeBool);
  svtkBooleanMacro(GenerateTriangles, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify a spatial locator for merging points. By default,
   * an instance of svtkMergePoints is used.
   */
  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);
  //@}

  //@{
  /**
   * Set the sorting order for the generated polydata. There are two
   * possibilities:
   * Sort by value = 0 - This is the most efficient sort. For each cell,
   * all contour values are processed. This is the default.
   * Sort by cell = 1 - For each contour value, all cells are processed.
   * This order should be used if the extracted polygons must be rendered
   * in a back-to-front or front-to-back order. This is very problem
   * dependent.
   * For most applications, the default order is fine (and faster).

   * Sort by cell is going to have a problem if the input has 2D and 3D cells.
   * Cell data will be scrambled because with
   * svtkPolyData output, verts and lines have lower cell ids than triangles.
   */
  svtkSetClampMacro(SortBy, int, SVTK_SORT_BY_VALUE, SVTK_SORT_BY_CELL);
  svtkGetMacro(SortBy, int);
  void SetSortByToSortByValue() { this->SetSortBy(SVTK_SORT_BY_VALUE); }
  void SetSortByToSortByCell() { this->SetSortBy(SVTK_SORT_BY_CELL); }
  const char* GetSortByAsString();
  //@}

  /**
   * Create default locator. Used to create one when none is specified. The
   * locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

  /**
   * Normally I would put this in a different class, but since
   * This is a temporary fix until we convert this class and contour filter
   * to generate unstructured grid output instead of poly data, I am leaving it here.
   */
  static void GetCellTypeDimensions(unsigned char* cellTypeDimensions);

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetClampMacro(OutputPointsPrecision, int, SINGLE_PRECISION, DEFAULT_PRECISION);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkCutter(svtkImplicitFunction* cf = nullptr);
  ~svtkCutter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  void UnstructuredGridCutter(svtkDataSet* input, svtkPolyData* output);
  void DataSetCutter(svtkDataSet* input, svtkPolyData* output);
  void StructuredPointsCutter(
    svtkDataSet*, svtkPolyData*, svtkInformation*, svtkInformationVector**, svtkInformationVector*);
  void StructuredGridCutter(svtkDataSet*, svtkPolyData*);
  void RectilinearGridCutter(svtkDataSet*, svtkPolyData*);
  svtkImplicitFunction* CutFunction;
  svtkTypeBool GenerateTriangles;

  svtkSynchronizedTemplates3D* SynchronizedTemplates3D;
  svtkSynchronizedTemplatesCutter3D* SynchronizedTemplatesCutter3D;
  svtkGridSynchronizedTemplates3D* GridSynchronizedTemplates;
  svtkRectilinearSynchronizedTemplates* RectilinearSynchronizedTemplates;

  svtkIncrementalPointLocator* Locator;
  int SortBy;
  svtkContourValues* ContourValues;
  svtkTypeBool GenerateCutScalars;
  int OutputPointsPrecision;

private:
  svtkCutter(const svtkCutter&) = delete;
  void operator=(const svtkCutter&) = delete;
};

//@{
/**
 * Return the sorting procedure as a descriptive character string.
 */
inline const char* svtkCutter::GetSortByAsString(void)
{
  if (this->SortBy == SVTK_SORT_BY_VALUE)
  {
    return "SortByValue";
  }
  else
  {
    return "SortByCell";
  }
}
//@}

#endif
