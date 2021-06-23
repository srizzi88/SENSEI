/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGridSynchronizedTemplates3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGridSynchronizedTemplates3D
 * @brief   generate isosurface from structured grids
 *
 *
 * svtkGridSynchronizedTemplates3D is a 3D implementation of the synchronized
 * template algorithm.
 *
 * @warning
 * This filter is specialized to 3D grids.
 *
 * @sa
 * svtkContourFilter svtkSynchronizedTemplates3D
 */

#ifndef svtkGridSynchronizedTemplates3D_h
#define svtkGridSynchronizedTemplates3D_h

#include "svtkContourValues.h"     // Because it passes all the calls to it
#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkStructuredGrid;

class SVTKFILTERSCORE_EXPORT svtkGridSynchronizedTemplates3D : public svtkPolyDataAlgorithm
{
public:
  static svtkGridSynchronizedTemplates3D* New();
  svtkTypeMacro(svtkGridSynchronizedTemplates3D, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Because we delegate to svtkContourValues
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
   * If this is enabled (by default), the output will be triangles
   * otherwise, the output will be the intersection polygons
   */
  svtkSetMacro(GenerateTriangles, svtkTypeBool);
  svtkGetMacro(GenerateTriangles, svtkTypeBool);
  svtkBooleanMacro(GenerateTriangles, svtkTypeBool);
  //@}

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
   * Main execution.
   */
  void ThreadedExecute(
    svtkStructuredGrid* input, svtkInformationVector** inVec, svtkInformation* outInfo);

  /**
   * This filter will initiate streaming so that no piece requested
   * from the input will be larger than this value (KiloBytes).
   */
  void SetInputMemoryLimit(long limit);

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
  svtkGridSynchronizedTemplates3D();
  ~svtkGridSynchronizedTemplates3D() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkTypeBool ComputeNormals;
  svtkTypeBool ComputeGradients;
  svtkTypeBool ComputeScalars;
  svtkTypeBool GenerateTriangles;

  svtkContourValues* ContourValues;

  int MinimumPieceSize[3];
  int OutputPointsPrecision;

private:
  svtkGridSynchronizedTemplates3D(const svtkGridSynchronizedTemplates3D&) = delete;
  void operator=(const svtkGridSynchronizedTemplates3D&) = delete;
};

#endif
