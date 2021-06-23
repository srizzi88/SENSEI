/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractUnstructuredGridPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractUnstructuredGridPiece
 * @brief   Return specified piece, including specified
 * number of ghost levels.
 */

#ifndef svtkExtractUnstructuredGridPiece_h
#define svtkExtractUnstructuredGridPiece_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkIdList;
class svtkIntArray;

class SVTKFILTERSPARALLEL_EXPORT svtkExtractUnstructuredGridPiece
  : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkExtractUnstructuredGridPiece* New();
  svtkTypeMacro(svtkExtractUnstructuredGridPiece, svtkUnstructuredGridAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Turn on/off creating ghost cells (on by default).
   */
  svtkSetMacro(CreateGhostCells, svtkTypeBool);
  svtkGetMacro(CreateGhostCells, svtkTypeBool);
  svtkBooleanMacro(CreateGhostCells, svtkTypeBool);
  //@}

protected:
  svtkExtractUnstructuredGridPiece();
  ~svtkExtractUnstructuredGridPiece() override {}

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // A method for labeling which piece the cells belong to.
  void ComputeCellTags(svtkIntArray* cellTags, svtkIdList* pointOwnership, int piece, int numPieces,
    svtkUnstructuredGrid* input);

  void AddGhostLevel(svtkUnstructuredGrid* input, svtkIntArray* cellTags, int ghostLevel);

  svtkTypeBool CreateGhostCells;

private:
  void AddFirstGhostLevel(
    svtkUnstructuredGrid* input, svtkIntArray* cellTags, int piece, int numPieces);

  svtkExtractUnstructuredGridPiece(const svtkExtractUnstructuredGridPiece&) = delete;
  void operator=(const svtkExtractUnstructuredGridPiece&) = delete;
};

#endif
