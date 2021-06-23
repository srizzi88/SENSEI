// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPSLACReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/

/**
 * @class   svtkPSLACReader
 *
 *
 *
 * Extends the svtkSLACReader to read in partitioned pieces.  Due to the nature
 * of the data layout, this reader only works in a data parallel mode where
 * each process in a parallel job simultaneously attempts to read the piece
 * corresponding to the local process id.
 *
 */

#ifndef svtkPSLACReader_h
#define svtkPSLACReader_h

#include "svtkIOParallelNetCDFModule.h" // For export macro
#include "svtkSLACReader.h"

class svtkMultiProcessController;

class SVTKIOPARALLELNETCDF_EXPORT svtkPSLACReader : public svtkSLACReader
{
public:
  svtkTypeMacro(svtkPSLACReader, svtkSLACReader);
  static svtkPSLACReader* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The controller used to communicate partition data.  The number of pieces
   * requested must agree with the number of processes, the piece requested must
   * agree with the local process id, and all process must invoke
   * ProcessRequests of this filter simultaneously.
   */
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  virtual void SetController(svtkMultiProcessController*);
  //@}

protected:
  svtkPSLACReader();
  ~svtkPSLACReader() override;

  svtkMultiProcessController* Controller;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int CheckTetrahedraWinding(int meshFD) override;
  int ReadConnectivity(
    int meshFD, svtkMultiBlockDataSet* surfaceOutput, svtkMultiBlockDataSet* volumeOutput) override;
  int ReadCoordinates(int meshFD, svtkMultiBlockDataSet* output) override;
  int ReadMidpointCoordinates(
    int meshFD, svtkMultiBlockDataSet* output, MidpointCoordinateMap& map) override;
  int ReadMidpointData(int meshFD, svtkMultiBlockDataSet* output, MidpointIdMap& map) override;
  int RestoreMeshCache(svtkMultiBlockDataSet* surfaceOutput, svtkMultiBlockDataSet* volumeOutput,
    svtkMultiBlockDataSet* compositeOutput) override;
  int ReadFieldData(const int* modeFDArray, int numModeFDs, svtkMultiBlockDataSet* output) override;

  int ReadTetrahedronInteriorArray(int meshFD, svtkIdTypeArray* connectivity) override;
  int ReadTetrahedronExteriorArray(int meshFD, svtkIdTypeArray* connectivity) override;

  int MeshUpToDate() override;

  /**
   * Reads point data arrays.  Called by ReadCoordinates and ReadFieldData.
   */
  svtkSmartPointer<svtkDataArray> ReadPointDataArray(int ncFD, int varId) override;

  class svtkInternal;
  svtkInternal* PInternal;

  //@{
  /**
   * The number of pieces and the requested piece to load.  Synonymous with
   * the number of processes and the local process id.
   */
  int NumberOfPieces;
  int RequestedPiece;
  //@}

  /**
   * The number of points defined in the mesh file.
   */
  svtkIdType NumberOfGlobalPoints;

  /**
   * The number of midpoints defined in the mesh file
   */
  svtkIdType NumberOfGlobalMidpoints;

  //@{
  /**
   * The start/end points read by the given process.
   */
  svtkIdType StartPointRead(int process)
  {
    return process * (this->NumberOfGlobalPoints / this->NumberOfPieces + 1);
  }
  svtkIdType EndPointRead(int process)
  {
    svtkIdType result = this->StartPointRead(process + 1);
    if (result > this->NumberOfGlobalPoints)
      result = this->NumberOfGlobalPoints;
    return result;
  }
  //@}

  //@{
  /**
   * Piece information from the last call.
   */
  int NumberOfPiecesCache;
  int RequestedPieceCache;
  //@}

private:
  svtkPSLACReader(const svtkPSLACReader&) = delete;
  void operator=(const svtkPSLACReader&) = delete;
};

#endif // svtkPSLACReader_h
