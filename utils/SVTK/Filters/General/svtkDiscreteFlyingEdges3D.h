/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDiscreteFlyingEdges3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDiscreteFlyingEdges3D
 * @brief   generate isosurface from 3D image data (volume)
 *
 * svtkDiscreteFlyingEdges3D creates output representations of label maps
 * (e.g., segmented volumes) using a variation of the flying edges
 * algorithm. The input is a 3D image (volume( where each point is labeled
 * (integer labels are preferred to real values), and the output data is
 * polygonal data representing labeled regions. (Note that on output each
 * region [corresponding to a different contour value] is represented
 * independently; i.e., points are not shared between regions even if they
 * are coincident.)
 *
 * This filter is similar to but produces different results than the filter
 * svtkDiscreteMarchingCubes. This filter can produce output normals, and each
 * labeled region is completely disconnected from neighboring regions
 * (coincident points are not merged). Both algorithms interpolate edges at
 * the halfway point between vertices with different segmentation labels.
 *
 * See the paper "Flying Edges: A High-Performance Scalable Isocontouring
 * Algorithm" by Schroeder, Maynard, Geveci. Proc. of LDAV 2015. Chicago, IL.
 *
 * @warning
 * This filter is specialized to 3D volumes. This implementation can produce
 * degenerate triangles (i.e., zero-area triangles).
 *
 * @warning
 * This class has been threaded with svtkSMPTools. Using TBB or other
 * non-sequential type (set in the CMake variable
 * SVTK_SMP_IMPLEMENTATION_TYPE) may improve performance significantly.
 *
 * @sa
 * svtkDiscreteMarchingCubes svtkDiscreteFlyingEdges2D svtkDiscreteFlyingEdges3D
 */

#ifndef svtkDiscreteFlyingEdges3D_h
#define svtkDiscreteFlyingEdges3D_h

#include "svtkContourValues.h"        // Passes calls through
#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkImageData;

class SVTKFILTERSGENERAL_EXPORT svtkDiscreteFlyingEdges3D : public svtkPolyDataAlgorithm
{
public:
  static svtkDiscreteFlyingEdges3D* New();
  svtkTypeMacro(svtkDiscreteFlyingEdges3D, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Because we delegate to svtkContourValues.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/Get the computation of normals. Normal computation is fairly
   * expensive in both time and storage. If the output data will be processed
   * by filters that modify topology or geometry, it may be wise to turn
   * Normals and Gradients off.
   */
  svtkSetMacro(ComputeNormals, int);
  svtkGetMacro(ComputeNormals, int);
  svtkBooleanMacro(ComputeNormals, int);
  //@}

  //@{
  /**
   * Set/Get the computation of gradients. Gradient computation is fairly
   * expensive in both time and storage. Note that if ComputeNormals is on,
   * gradients will have to be calculated, but will not be stored in the
   * output dataset. If the output data will be processed by filters that
   * modify topology or geometry, it may be wise to turn Normals and
   * Gradients off.
   */
  svtkSetMacro(ComputeGradients, int);
  svtkGetMacro(ComputeGradients, int);
  svtkBooleanMacro(ComputeGradients, int);
  //@}

  //@{
  /**
   * Set/Get the computation of scalars.
   */
  svtkSetMacro(ComputeScalars, int);
  svtkGetMacro(ComputeScalars, int);
  svtkBooleanMacro(ComputeScalars, int);
  //@}

  //@{
  /**
   * Indicate whether to interpolate other attribute data. That is, as the
   * isosurface is generated, interpolate all point attribute data across
   * the edge. This is independent of scalar interpolation, which is
   * controlled by the ComputeScalars flag.
   */
  svtkSetMacro(InterpolateAttributes, int);
  svtkGetMacro(InterpolateAttributes, int);
  svtkBooleanMacro(InterpolateAttributes, int);
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

  //@{
  /**
   * Set/get which component of the scalar array to contour on; defaults to 0.
   */
  svtkSetMacro(ArrayComponent, int);
  svtkGetMacro(ArrayComponent, int);
  //@}

protected:
  svtkDiscreteFlyingEdges3D();
  ~svtkDiscreteFlyingEdges3D() override;

  int ComputeNormals;
  int ComputeGradients;
  int ComputeScalars;
  int InterpolateAttributes;
  int ArrayComponent;
  svtkContourValues* ContourValues;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkDiscreteFlyingEdges3D(const svtkDiscreteFlyingEdges3D&) = delete;
  void operator=(const svtkDiscreteFlyingEdges3D&) = delete;
};

#endif
