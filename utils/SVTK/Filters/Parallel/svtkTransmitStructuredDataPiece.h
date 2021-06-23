/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransmitStructuredDataPiece.h

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

#ifndef svtkTransmitStructuredDataPiece_h
#define svtkTransmitStructuredDataPiece_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersParallelModule.h" // For export macro

class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkTransmitStructuredDataPiece : public svtkDataSetAlgorithm
{
public:
  static svtkTransmitStructuredDataPiece* New();
  svtkTypeMacro(svtkTransmitStructuredDataPiece, svtkDataSetAlgorithm);
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
  svtkTransmitStructuredDataPiece();
  ~svtkTransmitStructuredDataPiece() override;

  // Data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void RootExecute(svtkDataSet* input, svtkDataSet* output, svtkInformation* outInfo);
  void SatelliteExecute(int procId, svtkDataSet* output, svtkInformation* outInfo);
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool CreateGhostCells;
  svtkMultiProcessController* Controller;

private:
  svtkTransmitStructuredDataPiece(const svtkTransmitStructuredDataPiece&) = delete;
  void operator=(const svtkTransmitStructuredDataPiece&) = delete;
};

#endif
