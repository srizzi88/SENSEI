/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkBandedPolyDataContourFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkBandedPolyDataContourFilter
 * @brief   generate filled contours for svtkPolyData
 *
 * svtkBandedPolyDataContourFilter is a filter that takes as input svtkPolyData
 * and produces as output filled contours (also represented as svtkPolyData).
 * Filled contours are bands of cells that all have the same cell scalar
 * value, and can therefore be colored the same. The method is also referred
 * to as filled contour generation.
 *
 * To use this filter you must specify one or more contour values.  You can
 * either use the method SetValue() to specify each contour value, or use
 * GenerateValues() to generate a series of evenly spaced contours.  Each
 * contour value divides (or clips) the data into two pieces, values below
 * the contour value, and values above it. The scalar values of each
 * band correspond to the specified contour value.  Note that if the first and
 * last contour values are not the minimum/maximum contour range, then two
 * extra contour values are added corresponding to the minimum and maximum
 * range values. These extra contour bands can be prevented from being output
 * by turning clipping on.
 *
 * @sa
 * svtkClipDataSet svtkClipPolyData svtkClipVolume svtkContourFilter
 *
 */

#ifndef svtkBandedPolyDataContourFilter_h
#define svtkBandedPolyDataContourFilter_h

#include "svtkFiltersModelingModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkContourValues.h" // Needed for inline methods

class svtkPoints;
class svtkCellArray;
class svtkPointData;
class svtkDataArray;
class svtkFloatArray;
class svtkDoubleArray;
struct svtkBandedPolyDataContourFilterInternals;

#define SVTK_SCALAR_MODE_INDEX 0
#define SVTK_SCALAR_MODE_VALUE 1

class SVTKFILTERSMODELING_EXPORT svtkBandedPolyDataContourFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkBandedPolyDataContourFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with no contours defined.
   */
  static svtkBandedPolyDataContourFilter* New();

  //@{
  /**
   * Methods to set / get contour values. A single value at a time can be
   * set with SetValue(). Multiple contour values can be set with
   * GenerateValues(). Note that GenerateValues() generates n values
   * inclusive of the start and end range values.
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

  //@{
  /**
   * Indicate whether to clip outside the range specified by the user.
   * (The range is contour value[0] to contour value[numContours-1].)
   * Clipping means all cells outside of the range specified are not
   * sent to the output.
   */
  svtkSetMacro(Clipping, svtkTypeBool);
  svtkGetMacro(Clipping, svtkTypeBool);
  svtkBooleanMacro(Clipping, svtkTypeBool);
  //@}

  //@{
  /**
   * Control whether the cell scalars are output as an integer index or
   * a scalar value. If an index, the index refers to the bands produced
   * by the clipping range. If a value, then a scalar value which is a
   * value between clip values is used.
   */
  svtkSetClampMacro(ScalarMode, int, SVTK_SCALAR_MODE_INDEX, SVTK_SCALAR_MODE_VALUE);
  svtkGetMacro(ScalarMode, int);
  void SetScalarModeToIndex() { this->SetScalarMode(SVTK_SCALAR_MODE_INDEX); }
  void SetScalarModeToValue() { this->SetScalarMode(SVTK_SCALAR_MODE_VALUE); }
  //@}

  //@{
  /**
   * Turn on/off a flag to control whether contour edges are generated.
   * Contour edges are the edges between bands. If enabled, they are
   * generated from polygons/triangle strips and placed into the second
   * output (the ContourEdgesOutput).
   */
  svtkSetMacro(GenerateContourEdges, svtkTypeBool);
  svtkGetMacro(GenerateContourEdges, svtkTypeBool);
  svtkBooleanMacro(GenerateContourEdges, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the clip tolerance. Warning: setting this too large will
   * certainly cause numerical issues. Change from the default value
   * of FLT_EPSILON at your own risk. The actual internal clip tolerance
   * is computed by multiplying ClipTolerance by the scalar range.
   */
  svtkSetMacro(ClipTolerance, double);
  svtkGetMacro(ClipTolerance, double);
  //@}

  //@{
  /**
   * Set/Get the component to use of an input scalars array with more than one
   * component. Default is 0.
   */
  svtkSetMacro(Component, int);
  svtkGetMacro(Component, int);
  //@}

  /**
   * Get the second output which contains the edges dividing the contour
   * bands. This output is empty unless GenerateContourEdges is enabled.
   */
  svtkPolyData* GetContourEdgesOutput();

  /**
   * Overload GetMTime because we delegate to svtkContourValues so its
   * modified time must be taken into account.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkBandedPolyDataContourFilter();
  ~svtkBandedPolyDataContourFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int ClipEdge(int v1, int v2, svtkPoints* pts, svtkDataArray* inScalars, svtkDoubleArray* outScalars,
    svtkPointData* inPD, svtkPointData* outPD, svtkIdType edgePts[]);
  int InsertCell(
    svtkCellArray* cells, int npts, const svtkIdType* pts, int cellId, double s, svtkFloatArray* newS);
  int InsertLine(
    svtkCellArray* cells, svtkIdType pt1, svtkIdType pt2, int cellId, double s, svtkFloatArray* newS);
  int ComputeClippedIndex(double s);
  int InsertNextScalar(svtkFloatArray* scalars, int cellId, int idx);
  // data members
  svtkContourValues* ContourValues;

  svtkTypeBool Clipping;
  int ScalarMode;
  int Component;
  double ClipTolerance; // specify numerical accuracy during clipping
  // the second output
  svtkTypeBool GenerateContourEdges;

  svtkBandedPolyDataContourFilterInternals* Internal;

private:
  svtkBandedPolyDataContourFilter(const svtkBandedPolyDataContourFilter&) = delete;
  void operator=(const svtkBandedPolyDataContourFilter&) = delete;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void svtkBandedPolyDataContourFilter::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

/**
 * Get the ith contour value.
 */
inline double svtkBandedPolyDataContourFilter::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double* svtkBandedPolyDataContourFilter::GetValues()
{
  return this->ContourValues->GetValues();
}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void svtkBandedPolyDataContourFilter::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void svtkBandedPolyDataContourFilter::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

/**
 * Get the number of contours in the list of contour values.
 */
inline svtkIdType svtkBandedPolyDataContourFilter::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkBandedPolyDataContourFilter::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkBandedPolyDataContourFilter::GenerateValues(
  int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

#endif
