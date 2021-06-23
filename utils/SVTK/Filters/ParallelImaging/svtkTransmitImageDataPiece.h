/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTransmitImageDataPiece.h

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

#ifndef svtkTransmitImageDataPiece_h
#define svtkTransmitImageDataPiece_h

#include "svtkFiltersParallelImagingModule.h" // For export macro
#include "svtkTransmitStructuredDataPiece.h"

class svtkMultiProcessController;

class SVTKFILTERSPARALLELIMAGING_EXPORT svtkTransmitImageDataPiece
  : public svtkTransmitStructuredDataPiece
{
public:
  static svtkTransmitImageDataPiece* New();
  svtkTypeMacro(svtkTransmitImageDataPiece, svtkTransmitStructuredDataPiece);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkTransmitImageDataPiece();
  ~svtkTransmitImageDataPiece() override;

private:
  svtkTransmitImageDataPiece(const svtkTransmitImageDataPiece&) = delete;
  void operator=(const svtkTransmitImageDataPiece&) = delete;
};

#endif
