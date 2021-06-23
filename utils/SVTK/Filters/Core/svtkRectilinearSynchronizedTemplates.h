/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRectilinearSynchronizedTemplates.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRectilinearSynchronizedTemplates
 * @brief   generate isosurface from rectilinear grid
 *
 *
 * svtkRectilinearSynchronizedTemplates is a 3D implementation (for rectilinear
 * grids) of the synchronized template algorithm. Note that svtkContourFilter
 * will automatically use this class when appropriate.
 *
 * @warning
 * This filter is specialized to rectilinear grids.
 *
 * @sa
 * svtkContourFilter svtkSynchronizedTemplates2D svtkSynchronizedTemplates3D
 */

#ifndef svtkRectilinearSynchronizedTemplates_h
#define svtkRectilinearSynchronizedTemplates_h

#include "svtkContourValues.h"     // Passes calls through
#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkRectilinearGrid;
class svtkDataArray;

class SVTKFILTERSCORE_EXPORT svtkRectilinearSynchronizedTemplates : public svtkPolyDataAlgorithm
{
public:
  static svtkRectilinearSynchronizedTemplates* New();

  svtkTypeMacro(svtkRectilinearSynchronizedTemplates, svtkPolyDataAlgorithm);
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
   * Compute the spacing between this point and its 6 neighbors.  This method
   * needs to be public so it can be accessed from a templated function.
   */
  void ComputeSpacing(
    svtkRectilinearGrid* data, int i, int j, int k, int extent[6], double spacing[6]);

protected:
  svtkRectilinearSynchronizedTemplates();
  ~svtkRectilinearSynchronizedTemplates() override;

  svtkTypeBool ComputeNormals;
  svtkTypeBool ComputeGradients;
  svtkTypeBool ComputeScalars;
  svtkTypeBool GenerateTriangles;

  svtkContourValues* ContourValues;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int ArrayComponent;

  void* GetScalarsForExtent(svtkDataArray* array, int extent[6], svtkRectilinearGrid* input);

private:
  svtkRectilinearSynchronizedTemplates(const svtkRectilinearSynchronizedTemplates&) = delete;
  void operator=(const svtkRectilinearSynchronizedTemplates&) = delete;
};

// template table.

extern int SVTK_RECTILINEAR_SYNCHONIZED_TEMPLATES_TABLE_1[];
extern int SVTK_RECTILINEAR_SYNCHONIZED_TEMPLATES_TABLE_2[];

#endif
