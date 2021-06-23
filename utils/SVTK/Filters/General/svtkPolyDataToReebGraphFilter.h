/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPolyDataToReebGraphFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPolyDataToReebGraphFilter
 * @brief   generate a Reeb graph from a scalar
 * field defined on a svtkPolyData.
 *
 * The filter will first try to pull as a scalar field the svtkDataArray with
 * Id 'fieldId' of the mesh's svtkPointData.
 * If this field does not exist, the filter will use the svtkElevationFilter to
 * generate a default scalar field.
 */

#ifndef svtkPolyDataToReebGraphFilter_h
#define svtkPolyDataToReebGraphFilter_h

#include "svtkDirectedGraphAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class svtkReebGraph;

class SVTKFILTERSGENERAL_EXPORT svtkPolyDataToReebGraphFilter : public svtkDirectedGraphAlgorithm
{
public:
  static svtkPolyDataToReebGraphFilter* New();
  svtkTypeMacro(svtkPolyDataToReebGraphFilter, svtkDirectedGraphAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the scalar field id (default = 0).
   */
  svtkSetMacro(FieldId, int);
  svtkGetMacro(FieldId, int);
  //@}

  svtkReebGraph* GetOutput();

protected:
  svtkPolyDataToReebGraphFilter();
  ~svtkPolyDataToReebGraphFilter() override;

  int FieldId;

  int FillInputPortInformation(int portNumber, svtkInformation*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkPolyDataToReebGraphFilter(const svtkPolyDataToReebGraphFilter&) = delete;
  void operator=(const svtkPolyDataToReebGraphFilter&) = delete;
};

#endif
