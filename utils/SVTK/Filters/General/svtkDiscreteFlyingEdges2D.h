/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDiscreteFlyingEdges2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDiscreteFlyingEdges2D
 * @brief   generate isoline(s) from 2D image data
 *
 * svtkDiscreteFlyingEdges2D creates output representations of label maps
 * (e.g., segmented images) using a variation of the flying edges
 * algorithm. The input is a 2D image where each point is labeled (integer
 * labels are preferred to real values), and the output data is polygonal
 * data representing labeled regions. (Note that on output each region
 * [corresponding to a different contour value] is represented independently;
 * i.e., points are not shared between regions even if they are coincident.)
 *
 * @warning
 * This filter is specialized to 2D images. This implementation can produce
 * degenerate line segments (i.e., zero-length line segments).
 *
 * @warning
 * Use svtkContourLoopExtraction if you wish to create polygons from the line
 * segments.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkDiscreteMarchingCubes svtkContourLoopExtraction
 */

#ifndef svtkDiscreteFlyingEdges2D_h
#define svtkDiscreteFlyingEdges2D_h

#include "svtkContourValues.h"        // Needed for direct access to ContourValues
#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkImageData;

class SVTKFILTERSGENERAL_EXPORT svtkDiscreteFlyingEdges2D : public svtkPolyDataAlgorithm
{
public:
  /**
   * Standard methods for instantiation, printing, and type information.
   */
  static svtkDiscreteFlyingEdges2D* New();
  svtkTypeMacro(svtkDiscreteFlyingEdges2D, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Because we delegate to svtkContourValues.
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

  //@{
  /**
   * Generate numContours equally spaced contour values between specified
   * range. Contour values will include min/max range values.
   */
  void GenerateValues(int numContours, double range[2])
  {
    this->ContourValues->GenerateValues(numContours, range);
  }
  void GenerateValues(int numContours, double rangeStart, double rangeEnd)
  {
    this->ContourValues->GenerateValues(numContours, rangeStart, rangeEnd);
  }
  //@}

  //@{
  /**
   * Option to set the point scalars of the output.  The scalars will be the
   * label values.  By default this flag is on.
   */
  svtkSetMacro(ComputeScalars, int);
  svtkGetMacro(ComputeScalars, int);
  svtkBooleanMacro(ComputeScalars, int);
  //@}

  //@{
  /**
   * Set/get which component of the scalar array to contour on; defaults to 0.
   */
  svtkSetMacro(ArrayComponent, int);
  svtkGetMacro(ArrayComponent, int);
  //@}

protected:
  svtkDiscreteFlyingEdges2D();
  ~svtkDiscreteFlyingEdges2D() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkContourValues* ContourValues;
  int ComputeScalars;
  int ArrayComponent;

private:
  svtkDiscreteFlyingEdges2D(const svtkDiscreteFlyingEdges2D&) = delete;
  void operator=(const svtkDiscreteFlyingEdges2D&) = delete;
};

#endif
