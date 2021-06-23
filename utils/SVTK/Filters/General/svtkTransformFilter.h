/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransformFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTransformFilter
 * @brief   transform points and associated normals and vectors
 *
 * svtkTransformFilter is a filter to transform point coordinates, and
 * associated point normals and vectors, as well as cell normals and vectors.
 * Transformed data array will be stored in a float array or a double array.
 * Other point and cel data are passed through the filter, unless TransformAllInputVectors
 * is set to true, in this case all other 3 components arrays from point and cell data
 * will be transformed as well.
 *
 * An alternative method of transformation is to use svtkActor's methods
 * to scale, rotate, and translate objects. The difference between the
 * two methods is that svtkActor's transformation simply effects where
 * objects are rendered (via the graphics pipeline), whereas
 * svtkTransformFilter actually modifies point coordinates in the
 * visualization pipeline. This is necessary for some objects
 * (e.g., svtkProbeFilter) that require point coordinates as input.
 *
 * @sa
 * svtkAbstractTransform svtkTransformPolyDataFilter svtkActor
 */

#ifndef svtkTransformFilter_h
#define svtkTransformFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPointSetAlgorithm.h"

class svtkAbstractTransform;

class SVTKFILTERSGENERAL_EXPORT svtkTransformFilter : public svtkPointSetAlgorithm
{
public:
  static svtkTransformFilter* New();
  svtkTypeMacro(svtkTransformFilter, svtkPointSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Return the MTime also considering the transform.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Specify the transform object used to transform points.
   */
  virtual void SetTransform(svtkAbstractTransform*);
  svtkGetObjectMacro(Transform, svtkAbstractTransform);
  //@}

  int FillInputPortInformation(int port, svtkInformation* info) override;

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

  //@{
  /**
   * If off (the default), only Vectors and Normals will be transformed.
   * If on, all 3-component data arrays ( considered as 3D vectors) will be transformed
   * All other won't be flipped and will only be copied.
   */
  svtkSetMacro(TransformAllInputVectors, bool);
  svtkGetMacro(TransformAllInputVectors, bool);
  svtkBooleanMacro(TransformAllInputVectors, bool);
  //@}

protected:
  svtkTransformFilter();
  ~svtkTransformFilter() override;

  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkDataArray* CreateNewDataArray(svtkDataArray* input = nullptr);

  svtkAbstractTransform* Transform;
  int OutputPointsPrecision;
  bool TransformAllInputVectors;

private:
  svtkTransformFilter(const svtkTransformFilter&) = delete;
  void operator=(const svtkTransformFilter&) = delete;
};

#endif
