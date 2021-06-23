/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSynchronizedTemplates2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSynchronizedTemplates2D
 * @brief   generate isoline(s) from a structured points set
 *
 * svtkSynchronizedTemplates2D is a 2D implementation of the synchronized
 * template algorithm. Note that svtkContourFilter will automatically
 * use this class when appropriate.
 *
 * @warning
 * This filter is specialized to 2D images.
 *
 * @sa
 * svtkContourFilter svtkFlyingEdges2D svtkMarchingSquares
 * svtkSynchronizedTemplates3D svtkDiscreteFlyingEdges2D
 */

#ifndef svtkSynchronizedTemplates2D_h
#define svtkSynchronizedTemplates2D_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

#include "svtkContourValues.h" // Needed for direct access to ContourValues

class svtkImageData;

class SVTKFILTERSCORE_EXPORT svtkSynchronizedTemplates2D : public svtkPolyDataAlgorithm
{
public:
  static svtkSynchronizedTemplates2D* New();
  svtkTypeMacro(svtkSynchronizedTemplates2D, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Because we delegate to svtkContourValues
   */
  svtkMTimeType GetMTime() override;

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

  //@{
  /**
   * Option to set the point scalars of the output.  The scalars will be the
   * iso value of course.  By default this flag is on.
   */
  svtkSetMacro(ComputeScalars, svtkTypeBool);
  svtkGetMacro(ComputeScalars, svtkTypeBool);
  svtkBooleanMacro(ComputeScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get which component of the scalar array to contour on; defaults to 0.
   */
  svtkSetMacro(ArrayComponent, int);
  svtkGetMacro(ArrayComponent, int);
  //@}

protected:
  svtkSynchronizedTemplates2D();
  ~svtkSynchronizedTemplates2D() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  svtkContourValues* ContourValues;

  svtkTypeBool ComputeScalars;
  int ArrayComponent;

private:
  svtkSynchronizedTemplates2D(const svtkSynchronizedTemplates2D&) = delete;
  void operator=(const svtkSynchronizedTemplates2D&) = delete;
};

#endif
