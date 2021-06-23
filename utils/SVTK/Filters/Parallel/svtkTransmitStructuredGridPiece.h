/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransmitStructuredGridPiece.h

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
 *
 * Note that this class is legacy. The superclass does all the work and
 * can be used directly instead.
 */

#ifndef svtkTransmitStructuredGridPiece_h
#define svtkTransmitStructuredGridPiece_h

#include "svtkFiltersParallelModule.h" // For export macro
#include "svtkTransmitStructuredDataPiece.h"

class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkTransmitStructuredGridPiece
  : public svtkTransmitStructuredDataPiece
{
public:
  static svtkTransmitStructuredGridPiece* New();
  svtkTypeMacro(svtkTransmitStructuredGridPiece, svtkTransmitStructuredDataPiece);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkTransmitStructuredGridPiece();
  ~svtkTransmitStructuredGridPiece() override;

private:
  svtkTransmitStructuredGridPiece(const svtkTransmitStructuredGridPiece&) = delete;
  void operator=(const svtkTransmitStructuredGridPiece&) = delete;
};

#endif
