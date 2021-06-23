/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericProbeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGenericProbeFilter
 * @brief   sample data values at specified point locations
 *
 * svtkGenericProbeFilter is a filter that computes point attributes (e.g., scalars,
 * vectors, etc.) at specified point positions. The filter has two inputs:
 * the Input and Source. The Input geometric structure is passed through the
 * filter. The point attributes are computed at the Input point positions
 * by interpolating into the source data. For example, we can compute data
 * values on a plane (plane specified as Input) from a volume (Source).
 *
 * This filter can be used to resample data, or convert one dataset form into
 * another. For example, a generic dataset can be probed with a volume
 * (three-dimensional svtkImageData), and then volume rendering techniques can
 * be used to visualize the results. Another example: a line or curve can be
 * used to probe data to produce x-y plots along that line or curve.
 *
 * This filter has been implemented to operate on generic datasets, rather
 * than the typical svtkDataSet (and subclasses). svtkGenericDataSet is a more
 * complex cousin of svtkDataSet, typically consisting of nonlinear,
 * higher-order cells. To process this type of data, generic cells are
 * automatically tessellated into linear cells prior to isocontouring.
 *
 * @sa
 * svtkGenericProbeFilter svtkProbeFilter svtkGenericDataSet
 */

#ifndef svtkGenericProbeFilter_h
#define svtkGenericProbeFilter_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersGenericModule.h" // For export macro

class svtkIdTypeArray;
class svtkGenericDataSet;

class SVTKFILTERSGENERIC_EXPORT svtkGenericProbeFilter : public svtkDataSetAlgorithm
{
public:
  static svtkGenericProbeFilter* New();
  svtkTypeMacro(svtkGenericProbeFilter, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the point locations used to probe input. A generic dataset
   * type is assumed.
   */
  void SetSourceData(svtkGenericDataSet* source);
  svtkGenericDataSet* GetSource();
  //@}

  //@{
  /**
   * Get the list of point ids in the output that contain attribute data
   * interpolated from the source.
   */
  svtkGetObjectMacro(ValidPoints, svtkIdTypeArray);
  //@}

protected:
  svtkGenericProbeFilter();
  ~svtkGenericProbeFilter() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int, svtkInformation*) override;

  svtkIdTypeArray* ValidPoints;

private:
  svtkGenericProbeFilter(const svtkGenericProbeFilter&) = delete;
  void operator=(const svtkGenericProbeFilter&) = delete;
};

#endif
