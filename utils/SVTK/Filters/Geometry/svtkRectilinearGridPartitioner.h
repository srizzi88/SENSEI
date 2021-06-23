/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkRectilinearGridPartitioner.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkRectilinearGridPartitioner
 *
 *
 *  A concrete implementation of svtkMultiBlockDataSetAlgorithm that provides
 *  functionality for partitioning a SVTK rectilinear dataset. The partitioning
 *  methd used is Recursive Coordinate Bisection (RCB) where each time the
 *  longest dimension is split.
 *
 * @sa
 *  svtkUniformGridPartitioner svtkStructuredGridPartitioner
 */

#ifndef svtkRectilinearGridPartitioner_h
#define svtkRectilinearGridPartitioner_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

class svtkInformation;
class svtkInformationVector;
class svtkIndent;
class svtkDoubleArray;
class svtkRectilinearGrid;

class SVTKFILTERSGEOMETRY_EXPORT svtkRectilinearGridPartitioner : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkRectilinearGridPartitioner* New();
  svtkTypeMacro(svtkRectilinearGridPartitioner, svtkMultiBlockDataSetAlgorithm);
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
  svtkRectilinearGridPartitioner();
  ~svtkRectilinearGridPartitioner() override;

  /**
   * Extracts the coordinates
   */
  void ExtractGridCoordinates(svtkRectilinearGrid* grd, int subext[6], svtkDoubleArray* xcoords,
    svtkDoubleArray* ycoords, svtkDoubleArray* zcoords);

  // Standard Pipeline methods
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  int NumberOfPartitions;
  int NumberOfGhostLayers;
  svtkTypeBool DuplicateNodes;

private:
  svtkRectilinearGridPartitioner(const svtkRectilinearGridPartitioner&) = delete;
  void operator=(const svtkRectilinearGridPartitioner&) = delete;
};

#endif /* svtkRectilinearGridPartitioner_h */
