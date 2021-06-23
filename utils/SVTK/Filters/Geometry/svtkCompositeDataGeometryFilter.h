/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeDataGeometryFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeDataGeometryFilter
 * @brief   extract geometry from multi-group data
 *
 * svtkCompositeDataGeometryFilter applies svtkGeometryFilter to all
 * leaves in svtkCompositeDataSet. Place this filter at the end of a
 * pipeline before a polydata consumer such as a polydata mapper to extract
 * geometry from all blocks and append them to one polydata object.
 */

#ifndef svtkCompositeDataGeometryFilter_h
#define svtkCompositeDataGeometryFilter_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkPolyData;

class SVTKFILTERSGEOMETRY_EXPORT svtkCompositeDataGeometryFilter : public svtkPolyDataAlgorithm
{
public:
  static svtkCompositeDataGeometryFilter* New();
  svtkTypeMacro(svtkCompositeDataGeometryFilter, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

protected:
  svtkCompositeDataGeometryFilter();
  ~svtkCompositeDataGeometryFilter() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  // Create a default executive.
  svtkExecutive* CreateDefaultExecutive() override;

  virtual int RequestCompositeData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

private:
  svtkCompositeDataGeometryFilter(const svtkCompositeDataGeometryFilter&) = delete;
  void operator=(const svtkCompositeDataGeometryFilter&) = delete;
};

#endif
