/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiBlockMergeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMultiBlockMergeFilter
 * @brief   merges multiblock inputs into a single multiblock output
 *
 * svtkMultiBlockMergeFilter is an M to 1 filter similar to
 * svtkMultiBlockDataGroupFilter. However where as that class creates N groups
 * in the output for N inputs, this creates 1 group in the output with N
 * datasets inside it. In actuality if the inputs have M blocks, this will
 * produce M blocks, each of which has N datasets. Inside the merged group,
 * the i'th data set comes from the i'th data set in the i'th input.
 */

#ifndef svtkMultiBlockMergeFilter_h
#define svtkMultiBlockMergeFilter_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkMultiBlockMergeFilter : public svtkMultiBlockDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkMultiBlockMergeFilter, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object with PointIds and CellIds on; and ids being generated
   * as scalars.
   */
  static svtkMultiBlockMergeFilter* New();

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
  svtkMultiBlockMergeFilter();
  ~svtkMultiBlockMergeFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int IsMultiPiece(svtkMultiBlockDataSet*);

  int Merge(unsigned int numPieces, unsigned int pieceNo, svtkMultiBlockDataSet* output,
    svtkMultiBlockDataSet* input);

private:
  svtkMultiBlockMergeFilter(const svtkMultiBlockMergeFilter&) = delete;
  void operator=(const svtkMultiBlockMergeFilter&) = delete;
};

#endif
