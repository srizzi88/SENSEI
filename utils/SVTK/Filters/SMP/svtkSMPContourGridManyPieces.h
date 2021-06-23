/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSMPContourGridManyPiece.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSMPContourGridManyPieces
 * @brief   a subclass of svtkContourGrid that works in parallel
 * svtkSMPContourGridManyPieces performs the same functionaliy as svtkContourGrid but does
 * it using multiple threads. This filter generates a multi-block of svtkPolyData. It
 * will generate a relatively large number of pieces - the number is dependent on
 * the input size and number of threads available. See svtkSMPContourGrid is you are
 * interested in a filter that merges the piece. This will probably be merged with
 * svtkContourGrid in the future.
 */

#ifndef svtkSMPContourGridManyPieces_h
#define svtkSMPContourGridManyPieces_h

#include "svtkContourGrid.h"
#include "svtkFiltersSMPModule.h" // For export macro

class svtkPolyData;

#if !defined(SVTK_LEGACY_REMOVE)
class SVTKFILTERSSMP_EXPORT svtkSMPContourGridManyPieces : public svtkContourGrid
{
public:
  svtkTypeMacro(svtkSMPContourGridManyPieces, svtkContourGrid);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Constructor.
   */
  static svtkSMPContourGridManyPieces* New();

protected:
  svtkSMPContourGridManyPieces();
  ~svtkSMPContourGridManyPieces() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillOutputPortInformation(int port, svtkInformation* info) override;

private:
  svtkSMPContourGridManyPieces(const svtkSMPContourGridManyPieces&) = delete;
  void operator=(const svtkSMPContourGridManyPieces&) = delete;
};
#endif // SVTK_LEGACY_REMOVE
#endif
