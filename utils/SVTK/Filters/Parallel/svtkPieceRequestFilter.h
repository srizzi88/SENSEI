/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPieceRequestFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPieceRequestFilter
 * @brief   Sets the piece request for upstream filters.
 *
 * Sends the piece and number of pieces to upstream filters; passes the input
 * to the output unmodified.
 */

#ifndef svtkPieceRequestFilter_h
#define svtkPieceRequestFilter_h

#include "svtkAlgorithm.h"
#include "svtkFiltersParallelModule.h" // For export macro

class svtkDataObject;

class SVTKFILTERSPARALLEL_EXPORT svtkPieceRequestFilter : public svtkAlgorithm
{
public:
  static svtkPieceRequestFilter* New();
  svtkTypeMacro(svtkPieceRequestFilter, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The total number of pieces.
   */
  svtkSetClampMacro(NumberOfPieces, int, 0, SVTK_INT_MAX);
  svtkGetMacro(NumberOfPieces, int);
  //@}

  //@{
  /**
   * The piece to extract.
   */
  svtkSetClampMacro(Piece, int, 0, SVTK_INT_MAX);
  svtkGetMacro(Piece, int);
  //@}

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkDataObject* GetOutput();
  svtkDataObject* GetOutput(int);
  //@}

  //@{
  /**
   * Set an input of this algorithm.
   */
  void SetInputData(svtkDataObject*);
  void SetInputData(int, svtkDataObject*);
  //@}

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

protected:
  svtkPieceRequestFilter();
  ~svtkPieceRequestFilter() override {}

  virtual int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int NumberOfPieces;
  int Piece;

private:
  svtkPieceRequestFilter(const svtkPieceRequestFilter&) = delete;
  void operator=(const svtkPieceRequestFilter&) = delete;
};

#endif
