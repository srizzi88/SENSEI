/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOutlineCornerFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkOutlineCornerFilter
 * @brief   create wireframe outline corners for arbitrary data set
 *
 * svtkOutlineCornerFilter is a filter that generates wireframe outline corners of any
 * data set. The outline consists of the eight corners of the dataset
 * bounding box.
 */

#ifndef svtkOutlineCornerFilter_h
#define svtkOutlineCornerFilter_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"
class svtkOutlineCornerSource;

class SVTKFILTERSSOURCES_EXPORT svtkOutlineCornerFilter : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkOutlineCornerFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct outline corner filter with default corner factor = 0.2
   */
  static svtkOutlineCornerFilter* New();

  //@{
  /**
   * Set/Get the factor that controls the relative size of the corners
   * to the length of the corresponding bounds
   */
  svtkSetClampMacro(CornerFactor, double, 0.001, 0.5);
  svtkGetMacro(CornerFactor, double);
  //@}

protected:
  svtkOutlineCornerFilter();
  ~svtkOutlineCornerFilter() override;

  svtkOutlineCornerSource* OutlineCornerSource;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  double CornerFactor;

private:
  svtkOutlineCornerFilter(const svtkOutlineCornerFilter&) = delete;
  void operator=(const svtkOutlineCornerFilter&) = delete;
};

#endif
