/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkInterpolateDataSetAttributes.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkInterpolateDataSetAttributes
 * @brief   interpolate scalars, vectors, etc. and other dataset attributes
 *
 * svtkInterpolateDataSetAttributes is a filter that interpolates data set
 * attribute values between input data sets. The input to the filter
 * must be datasets of the same type, same number of cells, and same
 * number of points. The output of the filter is a data set of the same
 * type as the input dataset and whose attribute values have been
 * interpolated at the parametric value specified.
 *
 * The filter is used by specifying two or more input data sets (total of N),
 * and a parametric value t (0 <= t <= N-1). The output will contain
 * interpolated data set attributes common to all input data sets. (For
 * example, if one input has scalars and vectors, and another has just
 * scalars, then only scalars will be interpolated and output.)
 */

#ifndef svtkInterpolateDataSetAttributes_h
#define svtkInterpolateDataSetAttributes_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class svtkDataSetCollection;

class SVTKFILTERSGENERAL_EXPORT svtkInterpolateDataSetAttributes : public svtkDataSetAlgorithm
{
public:
  static svtkInterpolateDataSetAttributes* New();
  svtkTypeMacro(svtkInterpolateDataSetAttributes, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Return the list of inputs to this filter.
   */
  svtkDataSetCollection* GetInputList();

  //@{
  /**
   * Specify interpolation parameter t.
   */
  svtkSetClampMacro(T, double, 0.0, SVTK_DOUBLE_MAX);
  svtkGetMacro(T, double);
  //@}

protected:
  svtkInterpolateDataSetAttributes();
  ~svtkInterpolateDataSetAttributes() override;

  void ReportReferences(svtkGarbageCollector*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkDataSetCollection* InputList; // list of data sets to interpolate
  double T;                        // interpolation parameter

private:
  svtkInterpolateDataSetAttributes(const svtkInterpolateDataSetAttributes&) = delete;
  void operator=(const svtkInterpolateDataSetAttributes&) = delete;
};

#endif
