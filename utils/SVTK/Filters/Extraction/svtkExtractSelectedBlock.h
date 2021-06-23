/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedBlock.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkExtractSelectedBlock
 * @brief Extract-Selection filter to extract blocks.
 *
 * svtkExtractSelectedBlock extracts blocks from a composite dataset on input 0
 * using a svtkSelection on input 1.
 *
 * IDs extracted can refer to leaf nodes or non-leaf nodes. When they refer to
 * non-leaf nodes, the entire subtree is extracted.
 *
 * Note: this filter uses `svtkCompositeDataSet::ShallowCopy`, as a result, datasets at
 * leaf nodes are simply passed through, rather than being shallow-copied
 * themselves.
 */

#ifndef svtkExtractSelectedBlock_h
#define svtkExtractSelectedBlock_h

#include "svtkExtractSelectionBase.h"
#include "svtkFiltersExtractionModule.h" // For export macro

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractSelectedBlock : public svtkExtractSelectionBase
{
public:
  static svtkExtractSelectedBlock* New();
  svtkTypeMacro(svtkExtractSelectedBlock, svtkExtractSelectionBase);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkExtractSelectedBlock();
  ~svtkExtractSelectedBlock() override;

  // Generate the output.
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Sets up empty output dataset
   */
  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkExtractSelectedBlock(const svtkExtractSelectedBlock&) = delete;
  void operator=(const svtkExtractSelectedBlock&) = delete;
};

#endif
