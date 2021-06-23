/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiBlockDataGroupFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMultiBlockDataGroupFilter
 * @brief   collects multiple inputs into one multi-group dataset
 *
 * svtkMultiBlockDataGroupFilter is an M to 1 filter that merges multiple
 * input into one multi-group dataset. It will assign each input to
 * one group of the multi-group dataset and will assign each update piece
 * as a sub-block. For example, if there are two inputs and four update
 * pieces, the output contains two groups with four datasets each.
 */

#ifndef svtkMultiBlockDataGroupFilter_h
#define svtkMultiBlockDataGroupFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkMultiBlockDataGroupFilter : public svtkMultiBlockDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkMultiBlockDataGroupFilter, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with PointIds and CellIds on; and ids being generated
   * as scalars.
   */
  static svtkMultiBlockDataGroupFilter* New();

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(svtkDataObject*);
  void AddInputData(int, svtkDataObject*);
  //@}

protected:
  svtkMultiBlockDataGroupFilter();
  ~svtkMultiBlockDataGroupFilter() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkMultiBlockDataGroupFilter(const svtkMultiBlockDataGroupFilter&) = delete;
  void operator=(const svtkMultiBlockDataGroupFilter&) = delete;
};

#endif
