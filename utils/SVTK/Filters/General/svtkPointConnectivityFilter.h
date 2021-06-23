/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPointConnectivityFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPointConnectivityFilter
 * @brief   output a scalar field indicating point connectivity
 *
 *
 * svtkPointConnectivityFilter is a filter the produces a point scalar field
 * that characerizes the connectivity of the point. What is meant by
 * connectivity is the number of cells that use each point. The output
 * scalar array is represented by a 16-bit integral value. A value of zero
 * means that no cells use a particular point.
 */

#ifndef svtkPointConnectivityFilter_h
#define svtkPointConnectivityFilter_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class SVTKFILTERSGENERAL_EXPORT svtkPointConnectivityFilter : public svtkDataSetAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, obtaining type information and
   * printing.
   */
  static svtkPointConnectivityFilter* New();
  svtkTypeMacro(svtkPointConnectivityFilter, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

protected:
  svtkPointConnectivityFilter();
  ~svtkPointConnectivityFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkPointConnectivityFilter(const svtkPointConnectivityFilter&) = delete;
  void operator=(const svtkPointConnectivityFilter&) = delete;
};

#endif
