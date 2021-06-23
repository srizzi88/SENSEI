/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDiscreteFlyingEdgesClipper2D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDiscreteFlyingEdgesClipper2D
 * @brief   generate filled regions from segmented 2D image data
 *
 * svtkDiscreteFlyingEdgesClipper2D creates filled polygons from a label map
 * (e.g., segmented image) using a variation of the flying edges algorithm
 * adapted for 2D clipping. The input is a 2D image where each pixel is
 * labeled (integer labels are preferred to real values), and the output data
 * is polygonal data representing labeled regions. (Note that on output each
 * region [corresponding to a different contour value] may share points on a
 * shared boundary.)
 *
 * While this filter is similar to a contouring operation, label maps do not
 * provide continuous function values meaning that usual interpolation along
 * edges is not possible. Instead, when the edge endpoints are labeled in
 * differing regions, the edge is split at its midpoint. In addition, besides
 * producing intersection points at the mid-point of edges, the filter may
 * also generate points interior to the pixel cells. For example, if the four
 * vertices of a pixel cell are labeled with different regions, then an
 * interior point is created and four rectangular "regions" are produced.
 *
 * Note that one nice feature of this filter is that algorithm execution
 * occurs only one time no matter the number of contour values. In many
 * contouring-like algorithms, each separate contour value requires an
 * additional algorithm execution with a new contour value. So in this filter
 * large numbers of contour values do not significantly affect overall speed.
 *
 * @warning This filter is specialized to 2D images.
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkDiscreteFlyingEdges2D svtkDiscreteMarchingCubes svtkContourLoopExtraction
 * svtkFlyingEdges2D svtkFlyingEdges3D
 */

#ifndef svtkDiscreteFlyingEdgesClipper2D_h
#define svtkDiscreteFlyingEdgesClipper2D_h

#include "svtkContourValues.h"        // Needed for direct access to ContourValues
#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkImageData;

class SVTKFILTERSGENERAL_EXPORT svtkDiscreteFlyingEdgesClipper2D : public svtkPolyDataAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, printing, and type information.
   */
  static svtkDiscreteFlyingEdgesClipper2D* New();
  svtkTypeMacro(svtkDiscreteFlyingEdgesClipper2D, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  /**
   * The modified time is a function of the contour values because we delegate to
   * svtkContourValues.
   */
  svtkMTimeType GetMTime() override;

  /**
   * Set a particular contour value at contour number i. The index i ranges
   * between 0 <= i <NumberOfContours. (Note: while contour values are
   * expressed as doubles, the underlying scalar data may be a different
   * type. During execution the contour values are static cast to the type of
   * the scalar values.)
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
   * Generate numContours equally spaced contour values between the specified
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
   * Option to set the cell scalars of the output. The scalars will be the
   * contour values. By default this flag is on.
   */
  svtkSetMacro(ComputeScalars, int);
  svtkGetMacro(ComputeScalars, int);
  svtkBooleanMacro(ComputeScalars, int);
  //@}

  //@{
  /**
   * Set/get which component of a multi-component scalar array to contour on;
   * defaults to 0.
   */
  svtkSetMacro(ArrayComponent, int);
  svtkGetMacro(ArrayComponent, int);
  //@}

protected:
  svtkDiscreteFlyingEdgesClipper2D();
  ~svtkDiscreteFlyingEdgesClipper2D() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkContourValues* ContourValues;
  int ComputeScalars;
  int ArrayComponent;

private:
  svtkDiscreteFlyingEdgesClipper2D(const svtkDiscreteFlyingEdgesClipper2D&) = delete;
  void operator=(const svtkDiscreteFlyingEdgesClipper2D&) = delete;
};

#endif
