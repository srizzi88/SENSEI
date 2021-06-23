/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractUserDefinedPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/

/**
 * @class   svtkExtractUserDefinedPiece
 * @brief   Return user specified piece with ghost cells
 *
 *
 * Provided a function that determines which cells are zero-level
 * cells ("the piece"), this class outputs the piece with the
 * requested number of ghost levels.  The only difference between
 * this class and the class it is derived from is that the
 * zero-level cells are specified by a function you provide,
 * instead of determined by dividing up the cells based on cell Id.
 *
 * @sa
 * svtkExtractUnstructuredGridPiece
 */

#ifndef svtkExtractUserDefinedPiece_h
#define svtkExtractUserDefinedPiece_h

#include "svtkExtractUnstructuredGridPiece.h"
#include "svtkFiltersParallelModule.h" // For export macro

class SVTKFILTERSPARALLEL_EXPORT svtkExtractUserDefinedPiece : public svtkExtractUnstructuredGridPiece
{
public:
  svtkTypeMacro(svtkExtractUserDefinedPiece, svtkExtractUnstructuredGridPiece);
  static svtkExtractUserDefinedPiece* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  typedef int (*UserDefFunc)(svtkIdType cellID, svtkUnstructuredGrid* grid, void* constantData);

  // Set the function used to identify the piece.  The function should
  // return 1 if the cell is in the piece, and 0 otherwise.
  void SetPieceFunction(UserDefFunc func)
  {
    this->InPiece = func;
    this->Modified();
  }

  // Set constant data to be used by the piece identifying function.
  void SetConstantData(void* data, int len);

  // Get constant data to be used by the piece identifying function.
  // Return the length of the data buffer.
  int GetConstantData(void** data);

  // The function should return 1 if the cell
  // is in the piece, and 0 otherwise.

protected:
  svtkExtractUserDefinedPiece();
  ~svtkExtractUserDefinedPiece() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void ComputeCellTagsWithFunction(
    svtkIntArray* tags, svtkIdList* pointOwnership, svtkUnstructuredGrid* input);

private:
  svtkExtractUserDefinedPiece(const svtkExtractUserDefinedPiece&) = delete;
  void operator=(const svtkExtractUserDefinedPiece&) = delete;

  void* ConstantData;
  int ConstantDataLen;

  UserDefFunc InPiece;
};
#endif
