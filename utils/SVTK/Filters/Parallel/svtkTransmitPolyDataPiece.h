/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransmitPolyDataPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTransmitRectilinearGridPiece
 * @brief   Redistributes data produced
 * by serial readers
 *
 *
 * This filter can be used to redistribute data from producers that can't
 * produce data in parallel. All data is produced on first process and
 * the distributed to others using the multiprocess controller.
 */

#ifndef svtkTransmitPolyDataPiece_h
#define svtkTransmitPolyDataPiece_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkTransmitPolyDataPiece : public svtkPolyDataAlgorithm
{
public:
  static svtkTransmitPolyDataPiece* New();
  svtkTypeMacro(svtkTransmitPolyDataPiece, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * By default this filter uses the global controller,
   * but this method can be used to set another instead.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  //@{
  /**
   * Turn on/off creating ghost cells (on by default).
   */
  svtkSetMacro(CreateGhostCells, svtkTypeBool);
  svtkGetMacro(CreateGhostCells, svtkTypeBool);
  svtkBooleanMacro(CreateGhostCells, svtkTypeBool);
  //@}

protected:
  svtkTransmitPolyDataPiece();
  ~svtkTransmitPolyDataPiece() override;

  // Data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void RootExecute(svtkPolyData* input, svtkPolyData* output, svtkInformation* outInfo);
  void SatelliteExecute(int procId, svtkPolyData* output, svtkInformation* outInfo);

  svtkTypeBool CreateGhostCells;
  svtkMultiProcessController* Controller;

private:
  svtkTransmitPolyDataPiece(const svtkTransmitPolyDataPiece&) = delete;
  void operator=(const svtkTransmitPolyDataPiece&) = delete;
};

#endif
