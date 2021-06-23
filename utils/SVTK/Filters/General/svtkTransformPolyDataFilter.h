/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransformPolyDataFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTransformPolyDataFilter
 * @brief   transform points and associated normals and vectors for polygonal dataset
 *
 * svtkTransformPolyDataFilter is a filter to transform point
 * coordinates and associated point and cell normals and
 * vectors. Other point and cell data is passed through the filter
 * unchanged. This filter is specialized for polygonal data. See
 * svtkTransformFilter for more general data.
 *
 * An alternative method of transformation is to use svtkActor's methods
 * to scale, rotate, and translate objects. The difference between the
 * two methods is that svtkActor's transformation simply effects where
 * objects are rendered (via the graphics pipeline), whereas
 * svtkTransformPolyDataFilter actually modifies point coordinates in the
 * visualization pipeline. This is necessary for some objects
 * (e.g., svtkProbeFilter) that require point coordinates as input.
 *
 * @sa
 * svtkTransform svtkTransformFilter svtkActor
 */

#ifndef svtkTransformPolyDataFilter_h
#define svtkTransformPolyDataFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkAbstractTransform;

class SVTKFILTERSGENERAL_EXPORT svtkTransformPolyDataFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkTransformPolyDataFilter* New();
  svtkTypeMacro(svtkTransformPolyDataFilter, svtkPolyDataAlgorithm);
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

  //@{
  /**
   * Set/get the desired precision for the output types. See the documentation
   * for the svtkAlgorithm::DesiredOutputPrecision enum for an explanation of
   * the available precision settings.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkTransformPolyDataFilter();
  ~svtkTransformPolyDataFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkAbstractTransform* Transform;
  int OutputPointsPrecision;

private:
  svtkTransformPolyDataFilter(const svtkTransformPolyDataFilter&) = delete;
  void operator=(const svtkTransformPolyDataFilter&) = delete;
};

#endif
