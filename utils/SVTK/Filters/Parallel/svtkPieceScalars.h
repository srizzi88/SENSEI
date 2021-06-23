/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPieceScalars.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPieceScalars
 * @brief   Sets all cell scalars from the update piece.
 *
 *
 * svtkPieceScalars is meant to display which piece is being requested
 * as scalar values.  It is useful for visualizing the partitioning for
 * streaming or distributed pipelines.
 *
 * @sa
 * svtkPolyDataStreamer
 */

#ifndef svtkPieceScalars_h
#define svtkPieceScalars_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersParallelModule.h" // For export macro

class svtkFloatArray;
class svtkIntArray;

class SVTKFILTERSPARALLEL_EXPORT svtkPieceScalars : public svtkDataSetAlgorithm
{
public:
  static svtkPieceScalars* New();

  svtkTypeMacro(svtkPieceScalars, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Option to centerate cell scalars of points scalars.  Default is point scalars.
   */
  void SetScalarModeToCellData() { this->SetCellScalarsFlag(1); }
  void SetScalarModeToPointData() { this->SetCellScalarsFlag(0); }
  int GetScalarMode() { return this->CellScalarsFlag; }

  // Dscription:
  // This option uses a random mapping between pieces and scalar values.
  // The scalar values are chosen between 0 and 1.  By default, random mode is off.
  svtkSetMacro(RandomMode, svtkTypeBool);
  svtkGetMacro(RandomMode, svtkTypeBool);
  svtkBooleanMacro(RandomMode, svtkTypeBool);

protected:
  svtkPieceScalars();
  ~svtkPieceScalars() override;

  // Append the pieces.
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkIntArray* MakePieceScalars(int piece, svtkIdType numScalars);
  svtkFloatArray* MakeRandomScalars(int piece, svtkIdType numScalars);

  svtkSetMacro(CellScalarsFlag, int);
  int CellScalarsFlag;
  svtkTypeBool RandomMode;

private:
  svtkPieceScalars(const svtkPieceScalars&) = delete;
  void operator=(const svtkPieceScalars&) = delete;
};

#endif
