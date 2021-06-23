/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGenericOutlineFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkGenericOutlineFilter
 * @brief   create wireframe outline for arbitrary
 * generic data set
 *
 *
 * svtkGenericOutlineFilter is a filter that generates a wireframe outline of
 * any generic data set. The outline consists of the twelve edges of the
 * generic dataset bounding box.
 *
 * @sa
 * svtkGenericDataSet
 */

#ifndef svtkGenericOutlineFilter_h
#define svtkGenericOutlineFilter_h

#include "svtkFiltersGenericModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkOutlineSource;

class SVTKFILTERSGENERIC_EXPORT svtkGenericOutlineFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkGenericOutlineFilter* New();
  svtkTypeMacro(svtkGenericOutlineFilter, svtkPolyDataAlgorithm);

protected:
  svtkGenericOutlineFilter();
  ~svtkGenericOutlineFilter() override;

  svtkOutlineSource* OutlineSource;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int, svtkInformation*) override;

private:
  svtkGenericOutlineFilter(const svtkGenericOutlineFilter&) = delete;
  void operator=(const svtkGenericOutlineFilter&) = delete;
};

#endif
