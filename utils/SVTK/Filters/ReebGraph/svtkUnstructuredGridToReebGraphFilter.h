/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUnstructuredGridToReebGraphFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkUnstructuredGridToReebGraphFilter
 * @brief   generate a Reeb graph from a
 * scalar field defined on a svtkUnstructuredGrid.
 *
 * The filter will first try to pull as a scalar field the svtkDataArray with
 * Id 'fieldId' of the mesh's svtkPointData.
 * If this field does not exist, the filter will use the svtkElevationFilter to
 * generate a default scalar field.
 */

#ifndef svtkUnstructuredGridToReebGraphFilter_h
#define svtkUnstructuredGridToReebGraphFilter_h

#include "svtkDirectedGraphAlgorithm.h"
#include "svtkFiltersReebGraphModule.h" // For export macro

class svtkReebGraph;

class SVTKFILTERSREEBGRAPH_EXPORT svtkUnstructuredGridToReebGraphFilter
  : public svtkDirectedGraphAlgorithm
{
public:
  static svtkUnstructuredGridToReebGraphFilter* New();
  svtkTypeMacro(svtkUnstructuredGridToReebGraphFilter, svtkDirectedGraphAlgorithm);
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
  svtkUnstructuredGridToReebGraphFilter();
  ~svtkUnstructuredGridToReebGraphFilter() override;

  int FieldId;

  int FillInputPortInformation(int portNumber, svtkInformation*) override;
  int FillOutputPortInformation(int, svtkInformation*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkUnstructuredGridToReebGraphFilter(const svtkUnstructuredGridToReebGraphFilter&) = delete;
  void operator=(const svtkUnstructuredGridToReebGraphFilter&) = delete;
};

#endif
