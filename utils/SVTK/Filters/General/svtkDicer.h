/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDicer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDicer
 * @brief   abstract superclass to divide dataset into pieces
 *
 * Subclasses of svtkDicer divides the input dataset into separate
 * pieces.  These pieces can then be operated on by other filters
 * (e.g., svtkThreshold). One application is to break very large
 * polygonal models into pieces and performing viewing and occlusion
 * culling on the pieces. Multiple pieces can also be streamed through
 * the visualization pipeline.
 *
 * To use this filter, you must specify the execution mode of the
 * filter; i.e., set the way that the piece size is controlled (do
 * this by setting the DiceMode ivar). The filter does not change the
 * geometry or topology of the input dataset, rather it generates
 * integer numbers that indicate which piece a particular point
 * belongs to (i.e., it modifies the point and cell attribute
 * data). The integer number can be placed into the output scalar
 * data, or the output field data.
 *
 * @warning
 * The number of pieces generated may not equal the specified number
 * of pieces. Use the method GetNumberOfActualPieces() after filter
 * execution to get the actual number of pieces generated.
 *
 * @sa
 * svtkOBBDicer svtkConnectedDicer svtkSpatialDicer
 */

#ifndef svtkDicer_h
#define svtkDicer_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

#define SVTK_DICE_MODE_NUMBER_OF_POINTS 0
#define SVTK_DICE_MODE_SPECIFIED_NUMBER 1
#define SVTK_DICE_MODE_MEMORY_LIMIT 2

class SVTKFILTERSGENERAL_EXPORT svtkDicer : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkDicer, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the flag which controls whether to generate point scalar
   * data or point field data. If this flag is off, scalar data is
   * generated.  Otherwise, field data is generated. Note that the
   * generated the data are integer numbers indicating which piece a
   * particular point belongs to.
   */
  svtkSetMacro(FieldData, svtkTypeBool);
  svtkGetMacro(FieldData, svtkTypeBool);
  svtkBooleanMacro(FieldData, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the method to determine how many pieces the data should be
   * broken into. By default, the number of points per piece is used.
   */
  svtkSetClampMacro(DiceMode, int, SVTK_DICE_MODE_NUMBER_OF_POINTS, SVTK_DICE_MODE_MEMORY_LIMIT);
  svtkGetMacro(DiceMode, int);
  void SetDiceModeToNumberOfPointsPerPiece() { this->SetDiceMode(SVTK_DICE_MODE_NUMBER_OF_POINTS); }
  void SetDiceModeToSpecifiedNumberOfPieces() { this->SetDiceMode(SVTK_DICE_MODE_SPECIFIED_NUMBER); }
  void SetDiceModeToMemoryLimitPerPiece() { this->SetDiceMode(SVTK_DICE_MODE_MEMORY_LIMIT); }
  //@}

  //@{
  /**
   * Use the following method after the filter has updated to
   * determine the actual number of pieces the data was separated
   * into.
   */
  svtkGetMacro(NumberOfActualPieces, int);
  //@}

  //@{
  /**
   * Control piece size based on the maximum number of points per piece.
   * (This ivar has effect only when the DiceMode is set to
   * SetDiceModeToNumberOfPoints().)
   */
  svtkSetClampMacro(NumberOfPointsPerPiece, int, 1000, SVTK_INT_MAX);
  svtkGetMacro(NumberOfPointsPerPiece, int);
  //@}

  //@{
  /**
   * Set/Get the number of pieces the object is to be separated into.
   * (This ivar has effect only when the DiceMode is set to
   * SetDiceModeToSpecifiedNumber()). Note that the ivar
   * NumberOfPieces is a target - depending on the particulars of the
   * data, more or less number of pieces than the target value may be
   * created.
   */
  svtkSetClampMacro(NumberOfPieces, int, 1, SVTK_INT_MAX);
  svtkGetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * Control piece size based on a memory limit.  (This ivar has
   * effect only when the DiceMode is set to
   * SetDiceModeToMemoryLimit()). The memory limit should be set in
   * kibibytes (1024 bytes).
   */
  svtkSetClampMacro(MemoryLimit, unsigned long, 100, SVTK_INT_MAX);
  svtkGetMacro(MemoryLimit, unsigned long);
  //@}

protected:
  svtkDicer();
  ~svtkDicer() override {}

  virtual void UpdatePieceMeasures(svtkDataSet* input);

  int NumberOfPointsPerPiece;
  int NumberOfPieces;
  unsigned long MemoryLimit;
  int NumberOfActualPieces;
  svtkTypeBool FieldData;
  int DiceMode;

private:
  svtkDicer(const svtkDicer&) = delete;
  void operator=(const svtkDicer&) = delete;
};

#endif
