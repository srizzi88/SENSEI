/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkDataSetGhostGenerator.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkDataSetGhostGenerator
 *
 *
 *  An abstract class that provides common functionality and implements an
 *  interface for all ghost data generators. Ghost data generators accept as
 *  input a partitioned data-set, defined by a svtkMultiBlockDataSet, where each
 *  block corresponds to a partition. The output consists of svtkMultiBlockDataSet
 *  where each block holds the corresponding ghosted data-set. For more details,
 *  see concrete implementations.
 *
 * @sa
 * svtkUniformGridGhostDataGenerator, svtkStructuredGridGhostDataGenerator,
 * svtkRectilinearGridGhostDataGenerator
 */

#ifndef svtkDataSetGhostGenerator_h
#define svtkDataSetGhostGenerator_h

#include "svtkFiltersGeometryModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"

// Forward Declarations
class svtkInformation;
class svtkInformationVector;
class svtkMultiBlockDataSet;

class SVTKFILTERSGEOMETRY_EXPORT svtkDataSetGhostGenerator : public svtkMultiBlockDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkDataSetGhostGenerator, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get for number of ghost layers to generate.
   */
  svtkSetMacro(NumberOfGhostLayers, int);
  svtkGetMacro(NumberOfGhostLayers, int);
  //@}

  // Standard SVTK pipeline routines
  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation* rqst, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

protected:
  svtkDataSetGhostGenerator();
  ~svtkDataSetGhostGenerator() override;

  /**
   * Generate ghost layers. Implemented by concrete implementations.
   */
  virtual void GenerateGhostLayers(svtkMultiBlockDataSet* in, svtkMultiBlockDataSet* out) = 0;

  int NumberOfGhostLayers;

private:
  svtkDataSetGhostGenerator(const svtkDataSetGhostGenerator&) = delete;
  void operator=(const svtkDataSetGhostGenerator&) = delete;
};

#endif /* svtkDataSetGhostGenerator_h */
