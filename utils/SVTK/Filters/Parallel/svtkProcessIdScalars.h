/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProcessIdScalars.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProcessIdScalars
 * @brief   Sets cell or point scalars to the processor rank.
 *
 *
 * svtkProcessIdScalars is meant to display which processor owns which cells
 * and points.  It is useful for visualizing the partitioning for
 * streaming or distributed pipelines.
 *
 * @sa
 * svtkPolyDataStreamer
 */

#ifndef svtkProcessIdScalars_h
#define svtkProcessIdScalars_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersParallelModule.h" // For export macro

class svtkFloatArray;
class svtkIntArray;
class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkProcessIdScalars : public svtkDataSetAlgorithm
{
public:
  static svtkProcessIdScalars* New();

  svtkTypeMacro(svtkProcessIdScalars, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Option to centerate cell scalars of points scalars.  Default is point
   * scalars.
   */
  void SetScalarModeToCellData() { this->SetCellScalarsFlag(1); }
  void SetScalarModeToPointData() { this->SetCellScalarsFlag(0); }
  int GetScalarMode() { return this->CellScalarsFlag; }

  // Dscription:
  // This option uses a random mapping between pieces and scalar values.
  // The scalar values are chosen between 0 and 1.  By default, random
  // mode is off.
  svtkSetMacro(RandomMode, svtkTypeBool);
  svtkGetMacro(RandomMode, svtkTypeBool);
  svtkBooleanMacro(RandomMode, svtkTypeBool);

  //@{
  /**
   * By default this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkProcessIdScalars();
  ~svtkProcessIdScalars() override;

  // Append the pieces.
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkIntArray* MakeProcessIdScalars(int piece, svtkIdType numScalars);
  svtkFloatArray* MakeRandomScalars(int piece, svtkIdType numScalars);

  svtkSetMacro(CellScalarsFlag, int);
  int CellScalarsFlag;
  svtkTypeBool RandomMode;

  svtkMultiProcessController* Controller;

private:
  svtkProcessIdScalars(const svtkProcessIdScalars&) = delete;
  void operator=(const svtkProcessIdScalars&) = delete;
};

#endif
