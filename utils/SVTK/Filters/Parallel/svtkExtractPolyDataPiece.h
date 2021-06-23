/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractPolyDataPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractPolyDataPiece
 * @brief   Return specified piece, including specified
 * number of ghost levels.
 */

#ifndef svtkExtractPolyDataPiece_h
#define svtkExtractPolyDataPiece_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkIdList;
class svtkIntArray;

class SVTKFILTERSPARALLEL_EXPORT svtkExtractPolyDataPiece : public svtkPolyDataAlgorithm
{
public:
  static svtkExtractPolyDataPiece* New();
  svtkTypeMacro(svtkExtractPolyDataPiece, svtkPolyDataAlgorithm);
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
  svtkExtractPolyDataPiece();
  ~svtkExtractPolyDataPiece() override {}

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // A method for labeling which piece the cells belong to.
  void ComputeCellTags(
    svtkIntArray* cellTags, svtkIdList* pointOwnership, int piece, int numPieces, svtkPolyData* input);

  void AddGhostLevel(svtkPolyData* input, svtkIntArray* cellTags, int ghostLevel);

  svtkTypeBool CreateGhostCells;

private:
  svtkExtractPolyDataPiece(const svtkExtractPolyDataPiece&) = delete;
  void operator=(const svtkExtractPolyDataPiece&) = delete;
};

#endif
