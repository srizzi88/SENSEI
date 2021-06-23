/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMarchingCubes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMarchingCubes
 * @brief   generate isosurface(s) from volume
 *
 * svtkMarchingCubes is a filter that takes as input a volume (e.g., 3D
 * structured point set) and generates on output one or more isosurfaces.
 * One or more contour values must be specified to generate the isosurfaces.
 * Alternatively, you can specify a min/max scalar range and the number of
 * contours to generate a series of evenly spaced contour values.
 *
 * @warning
 * This filter is specialized to volumes. If you are interested in
 * contouring other types of data, use the general svtkContourFilter. If you
 * want to contour an image (i.e., a volume slice), use svtkMarchingSquares.
 *
 * @sa
 * Much faster implementations for isocontouring are available. In
 * particular, svtkFlyingEdges3D and svtkFlyingEdges2D are much faster
 * and if built with the right options, multithreaded, and scale well
 * with additional processors.
 *
 * @sa
 * If you are interested in extracting surfaces from label maps,
 * consider using svtkDiscreteFlyingEdges3D, svtkDiscreteFlyingEdges2D, or
 * svtkDiscreteMarchingCubes.
 *
 * @sa
 * svtkFlyingEdges3D svtkFlyingEdges2D svtkSynchronizedTemplates3D
 * svtkSynchronizedTemplates2D svtkContourFilter svtkSliceCubes
 * svtkMarchingSquares svtkDividingCubes svtkDiscreteMarchingCubes
 */

#ifndef svtkMarchingCubes_h
#define svtkMarchingCubes_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkContourValues.h" // Needed for direct access to ContourValues

class svtkIncrementalPointLocator;

class SVTKFILTERSCORE_EXPORT svtkMarchingCubes : public svtkPolyDataAlgorithm
{
public:
  static svtkMarchingCubes* New();
  svtkTypeMacro(svtkMarchingCubes, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  // Methods to set contour values
  void SetValue(int i, double value);
  double GetValue(int i);
  double* GetValues();
  void GetValues(double* contourValues);
  void SetNumberOfContours(int number);
  svtkIdType GetNumberOfContours();
  void GenerateValues(int numContours, double range[2]);
  void GenerateValues(int numContours, double rangeStart, double rangeEnd);

  // Because we delegate to svtkContourValues
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
   * override the default locator.  Useful for changing the number of
   * bins for performance or specifying a more aggressive locator.
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
  svtkMarchingCubes();
  ~svtkMarchingCubes() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkContourValues* ContourValues;
  svtkTypeBool ComputeNormals;
  svtkTypeBool ComputeGradients;
  svtkTypeBool ComputeScalars;
  svtkIncrementalPointLocator* Locator;

private:
  svtkMarchingCubes(const svtkMarchingCubes&) = delete;
  void operator=(const svtkMarchingCubes&) = delete;
};

/**
 * Set a particular contour value at contour number i. The index i ranges
 * between 0<=i<NumberOfContours.
 */
inline void svtkMarchingCubes::SetValue(int i, double value)
{
  this->ContourValues->SetValue(i, value);
}

/**
 * Get the ith contour value.
 */
inline double svtkMarchingCubes::GetValue(int i)
{
  return this->ContourValues->GetValue(i);
}

/**
 * Get a pointer to an array of contour values. There will be
 * GetNumberOfContours() values in the list.
 */
inline double* svtkMarchingCubes::GetValues()
{
  return this->ContourValues->GetValues();
}

/**
 * Fill a supplied list with contour values. There will be
 * GetNumberOfContours() values in the list. Make sure you allocate
 * enough memory to hold the list.
 */
inline void svtkMarchingCubes::GetValues(double* contourValues)
{
  this->ContourValues->GetValues(contourValues);
}

/**
 * Set the number of contours to place into the list. You only really
 * need to use this method to reduce list size. The method SetValue()
 * will automatically increase list size as needed.
 */
inline void svtkMarchingCubes::SetNumberOfContours(int number)
{
  this->ContourValues->SetNumberOfContours(number);
}

/**
 * Get the number of contours in the list of contour values.
 */
inline svtkIdType svtkMarchingCubes::GetNumberOfContours()
{
  return this->ContourValues->GetNumberOfContours();
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkMarchingCubes::GenerateValues(int numContours, double range[2])
{
  this->ContourValues->GenerateValues(numContours, range);
}

/**
 * Generate numContours equally spaced contour values between specified
 * range. Contour values will include min/max range values.
 */
inline void svtkMarchingCubes::GenerateValues(int numContours, double rangeStart, double rangeEnd)
{
  this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
}

#endif
