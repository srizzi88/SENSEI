/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThreadedSynchronizedTemplates3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkThreadedSynchronizedTemplates3D
 * @brief   generate isosurface from structured points
 *
 *
 * svtkThreadedSynchronizedTemplates3D is a 3D implementation of the synchronized
 * template algorithm. Note that svtkContourFilter will automatically
 * use this class when appropriate.
 *
 * @warning
 * This filter is specialized to 3D images (aka volumes).
 *
 * @sa
 * svtkContourFilter svtkThreadedSynchronizedTemplates2D
 */

#ifndef svtkThreadedSynchronizedTemplates3D_h
#define svtkThreadedSynchronizedTemplates3D_h

#include "svtkContourValues.h"    // Passes calls through
#include "svtkFiltersSMPModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class svtkImageData;

#if !defined(SVTK_LEGACY_REMOVE)
class SVTKFILTERSSMP_EXPORT svtkThreadedSynchronizedTemplates3D : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkThreadedSynchronizedTemplates3D* New();

  svtkTypeMacro(svtkThreadedSynchronizedTemplates3D, svtkMultiBlockDataSetAlgorithm);
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

  void ThreadedExecute(
    svtkImageData* data, svtkInformation* inInfo, svtkInformation* outInfo, svtkDataArray* inScalars);

  //@{
  /**
   * Determines the chunk size for streaming.  This filter will act like a
   * collector: ask for many input pieces, but generate one output.  Limit is
   * in KBytes
   */
  void SetInputMemoryLimit(unsigned long limit);
  unsigned long GetInputMemoryLimit();
  //@}

  //@{
  /**
   * Set/get which component of the scalar array to contour on; defaults to 0.
   */
  svtkSetMacro(ArrayComponent, int);
  svtkGetMacro(ArrayComponent, int);
  //@}

protected:
  svtkThreadedSynchronizedTemplates3D();
  ~svtkThreadedSynchronizedTemplates3D() override;

  svtkTypeBool ComputeNormals;
  svtkTypeBool ComputeGradients;
  svtkTypeBool ComputeScalars;
  svtkContourValues* ContourValues;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int ArrayComponent;

  svtkTypeBool GenerateTriangles;

private:
  svtkThreadedSynchronizedTemplates3D(const svtkThreadedSynchronizedTemplates3D&) = delete;
  void operator=(const svtkThreadedSynchronizedTemplates3D&) = delete;
};

// template table.

extern int SVTKFILTERSSMP_EXPORT SVTK_TSYNCHRONIZED_TEMPLATES_3D_TABLE_1[];
extern int SVTKFILTERSSMP_EXPORT SVTK_TSYNCHRONIZED_TEMPLATES_3D_TABLE_2[];

#endif // SVTK_LEGACY_REMOVE
#endif
