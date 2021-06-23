/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkUniformGridPartitioner.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkUniformGridPartitioner
 *
 *
 *  A concrete implementation of svtkMultiBlockDataSetAlgorithm that provides
 *  functionality for partitioning a uniform grid. The partitioning method
 *  that is used is Recursive Coordinate Bisection (RCB) where each time
 *  the longest dimension is split.
 *
 * @sa
 * svtkStructuredGridPartitioner svtkRectilinearGridPartitioner
 */

#ifndef svtkUniformGridPartitioner_h
#define svtkUniformGridPartitioner_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class svtkInformation;
class svtkInformationVector;
class svtkIndent;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkUniformGridPartitioner
  : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkUniformGridPartitioner* New();
  svtkTypeMacro(svtkUniformGridPartitioner, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& oss, svtkIndent indent) override;

  //@{
  /**
   * Set/Get macro for the number of subdivisions.
   */
  svtkGetMacro(NumberOfPartitions, int);
  svtkSetMacro(NumberOfPartitions, int);
  //@}

  //@{
  /**
   * Set/Get macro for the number of ghost layers.
   */
  svtkGetMacro(NumberOfGhostLayers, int);
  svtkSetMacro(NumberOfGhostLayers, int);
  //@}

  //@{
  svtkGetMacro(DuplicateNodes, svtkTypeBool);
  svtkSetMacro(DuplicateNodes, svtkTypeBool);
  svtkBooleanMacro(DuplicateNodes, svtkTypeBool);
  //@}

protected:
  svtkUniformGridPartitioner();
  ~svtkUniformGridPartitioner() override;

  // Standard Pipeline methods
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  int NumberOfPartitions;
  int NumberOfGhostLayers;
  svtkTypeBool DuplicateNodes;

private:
  svtkUniformGridPartitioner(const svtkUniformGridPartitioner&) = delete;
  void operator=(const svtkUniformGridPartitioner&) = delete;
};

#endif /* SVTKUNIFORMGRIDPARTITIONER_H_ */
