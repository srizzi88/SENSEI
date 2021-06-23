/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMarchingSquares.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMarchingSquares
 * @brief   generate isoline(s) from structured points set
 *
 * svtkMarchingSquares is a filter that takes as input a structured points set
 * and generates on output one or more isolines.  One or more contour values
 * must be specified to generate the isolines.  Alternatively, you can specify
 * a min/max scalar range and the number of contours to generate a series of
 * evenly spaced contour values.
 *
 * To generate contour lines the input data must be of topological dimension 2
 * (i.e., an image). If not, you can use the ImageRange ivar to select an
 * image plane from an input volume. This avoids having to extract a plane first
 * (using svtkExtractSubVolume).  The filter deals with this by first
 * trying to use the input data directly, and if not a 2D image, then uses the
 * ImageRange ivar to reduce it to an image.
 *
 * @warning
 * This filter is specialized to images. If you are interested in
 * contouring other types of data, use the general svtkContourFilter.
 * @sa
 * svtkContourFilter svtkMarchingCubes svtkSliceCubes svtkDividingCubes
 */

#ifndef svtkMarchingSquares_h
#define svtkMarchingSquares_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkContourValues.h" // Passes calls to svtkContourValues

class svtkImageData;
class svtkIncrementalPointLocator;

class SVTKFILTERSCORE_EXPORT svtkMarchingSquares : public svtkPolyDataAlgorithm
{
public:
  static svtkMarchingSquares* New();
  svtkTypeMacro(svtkMarchingSquares, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the i-j-k index range which define a plane on which to generate
   * contour lines. Using this ivar it is possible to input a 3D volume
   * directly and then generate contour lines on one of the i-j-k planes, or
   * a portion of a plane.
   */
  svtkSetVectorMacro(ImageRange, int, 6);
  svtkGetVectorMacro(ImageRange, int, 6);
  void SetImageRange(int imin, int imax, int jmin, int jmax, int kmin, int kmax);
  //@}

  //@{
  /**
   * Methods to set contour values
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
   * Because we delegate to svtkContourValues
   */
  svtkMTimeType GetMTime() override;

  void SetLocator(svtkIncrementalPointLocator* locator);
  svtkGetObjectMacro(Locator, svtkIncrementalPointLocator);

  /**
   * Create default locator. Used to create one when none is specified.
   * The locator is used to merge coincident points.
   */
  void CreateDefaultLocator();

protected:
  svtkMarchingSquares();
  ~svtkMarchingSquares() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkContourValues* ContourValues;
  int ImageRange[6];
  svtkIncrementalPointLocator* Locator;

private:
  svtkMarchingSquares(const svtkMarchingSquares&) = delete;
  void operator=(const svtkMarchingSquares&) = delete;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void svtkMarchingSquares::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

/**
 * Get the ith contour value.
 */
inline double svtkMarchingSquares::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double* svtkMarchingSquares::GetValues()
{
  return this->ContourValues->GetValues();
}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void svtkMarchingSquares::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void svtkMarchingSquares::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

/**
 * Get the number of contours in the list of contour values.
 */
inline svtkIdType svtkMarchingSquares::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkMarchingSquares::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkMarchingSquares::GenerateValues(int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

#endif
