/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPipelineSize.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPipelineSize
 * @brief   compute the memory required by a pipeline
 */

#ifndef svtkPipelineSize_h
#define svtkPipelineSize_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkObject.h"
class svtkAlgorithm;

class SVTKFILTERSPARALLEL_EXPORT svtkPipelineSize : public svtkObject
{
public:
  static svtkPipelineSize* New();
  svtkTypeMacro(svtkPipelineSize, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Compute an estimate of how much memory a pipline will require in
   * kibibytes (1024 bytes) This is only an estimate and the
   * calculations in here do not take into account the specifics of many
   * sources and filters.
   */
  unsigned long GetEstimatedSize(svtkAlgorithm* input, int inputPort, int connection);

  /**
   * Determine how many subpieces a mapper should use to fit a target memory
   * limit. The piece and numPieces can be queried from the mapper using
   * `svtkPolyDataMapper::GetPiece` and
   * `svtkPolyDataMapper::GetNumberOfSubPieces`.
   */
  unsigned long GetNumberOfSubPieces(
    unsigned long memoryLimit, svtkAlgorithm* mapper, int piece, int numPieces);

protected:
  svtkPipelineSize() {}
  void GenericComputeSourcePipelineSize(svtkAlgorithm* src, int outputPort, unsigned long size[3]);
  void ComputeSourcePipelineSize(svtkAlgorithm* src, int outputPort, unsigned long size[3]);
  void ComputeOutputMemorySize(
    svtkAlgorithm* src, int outputPort, unsigned long* inputSize, unsigned long size[2]);
  void GenericComputeOutputMemorySize(
    svtkAlgorithm* src, int outputPort, unsigned long* inputSize, unsigned long size[2]);

private:
  svtkPipelineSize(const svtkPipelineSize&) = delete;
  void operator=(const svtkPipelineSize&) = delete;
};

#endif
