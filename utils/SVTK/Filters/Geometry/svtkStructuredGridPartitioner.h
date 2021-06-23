/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkStructuredGridPartitioner.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkStructuredGridPartitioner
 *
 *
 *  A concrete implementation of svtkMultiBlockDataSetAlgorithm that provides
 *  functionality for partitioning a SVTK structured grid dataset. The partition-
 *  ing method used is Recursive Coordinate Bisection (RCB) where each time the
 *  longest dimension is split.
 *
 * @sa
 *  svtkUniformGridPartitioner svtkRectilinearGridPartitioner
 */

#ifndef svtkStructuredGridPartitioner_h
#define svtkStructuredGridPartitioner_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class svtkInformation;
class svtkInformationVector;
class svtkIndent;
class svtkStructuredGrid;
class svtkPoints;

class SVTKFILTERSGEOMETRY_EXPORT svtkStructuredGridPartitioner : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkStructuredGridPartitioner* New();
  svtkTypeMacro(svtkStructuredGridPartitioner, svtkMultiBlockDataSetAlgorithm);
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
  /**
   * Set/Get & boolean macro for the DuplicateNodes property.
   */
  svtkGetMacro(DuplicateNodes, svtkTypeBool);
  svtkSetMacro(DuplicateNodes, svtkTypeBool);
  svtkBooleanMacro(DuplicateNodes, svtkTypeBool);
  //@}

protected:
  svtkStructuredGridPartitioner();
  ~svtkStructuredGridPartitioner() override;

  /**
   * Extracts the coordinates of the sub-grid from the whole grid.
   */
  svtkPoints* ExtractSubGridPoints(svtkStructuredGrid* wholeGrid, int subext[6]);

  // Standard Pipeline methods
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  int NumberOfPartitions;
  int NumberOfGhostLayers;
  svtkTypeBool DuplicateNodes;

private:
  svtkStructuredGridPartitioner(const svtkStructuredGridPartitioner&) = delete;
  void operator=(const svtkStructuredGridPartitioner&) = delete;
};

#endif /* svtkStructuredGridPartitioner_h */
