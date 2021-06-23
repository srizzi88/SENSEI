/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransmitUnstructuredGridPiece.h

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

#ifndef svtkTransmitUnstructuredGridPiece_h
#define svtkTransmitUnstructuredGridPiece_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkUnstructuredGridAlgorithm.h"

class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkTransmitUnstructuredGridPiece
  : public svtkUnstructuredGridAlgorithm
{
public:
  static svtkTransmitUnstructuredGridPiece* New();
  svtkTypeMacro(svtkTransmitUnstructuredGridPiece, svtkUnstructuredGridAlgorithm);
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
  svtkTransmitUnstructuredGridPiece();
  ~svtkTransmitUnstructuredGridPiece() override;

  // Data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void RootExecute(
    svtkUnstructuredGrid* input, svtkUnstructuredGrid* output, svtkInformation* outInfo);
  void SatelliteExecute(int procId, svtkUnstructuredGrid* output, svtkInformation* outInfo);

  svtkTypeBool CreateGhostCells;
  svtkMultiProcessController* Controller;

private:
  svtkTransmitUnstructuredGridPiece(const svtkTransmitUnstructuredGridPiece&) = delete;
  void operator=(const svtkTransmitUnstructuredGridPiece&) = delete;
};

#endif
