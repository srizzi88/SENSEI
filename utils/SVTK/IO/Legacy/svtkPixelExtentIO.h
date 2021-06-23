/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPixelExtentIO.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPixelExtentIO
 *
 * A small collection of I/O routines that can write svtkPixelExtent's
 * or collections of them to disk for visualization as unstructured
 * grids.
 */

#ifndef svtkPixelExtentIO_h
#define svtkPixelExtentIO_h

#include "svtkIOLegacyModule.h" // for export
#include "svtkPixelExtent.h"    // for pixel extent
#include <deque>               // for std::deque

class svtkUnstructuredGrid;

class SVTKIOLEGACY_EXPORT svtkPixelExtentIO
{
public:
  /**
   * Writes deque of extents for each MPI rank to disk
   * as an unstructured grid. Each extent is converted to
   * a QUAD cell. Rank is encoded in a cell data array.
   * It's aassumed that the data is duplicated on all
   * ranks thus only rank 0 writes the data to disk.
   */
  static void Write(
    int commRank, const char* fileName, const std::deque<std::deque<svtkPixelExtent> >& exts);

  /**
   * Writes an extent for each MPI rank to disk as an
   * unstructured grid. It's expected that the index into
   * the deque identifies the rank. Each extent is converted
   * to a QUAD cell. Rank is encoded in a cell data array.
   * It's aassumed that the data is duplicated on all
   * ranks thus only rank 0 writes the data to disk.
   */
  static void Write(int commRank, const char* fileName, const std::deque<svtkPixelExtent>& exts);

  //@{
  /**
   * Write an extent per MPI rank to disk. All ranks
   * write. It's assumed that each rank passes a unique
   * filename.
   */
  static void Write(int commRank, const char* fileName, const svtkPixelExtent& ext);
  //@}
};

/**
 * Insert the extent into an unstructured grid.
 */
SVTKIOLEGACY_EXPORT
svtkUnstructuredGrid& operator<<(svtkUnstructuredGrid& data, const svtkPixelExtent& ext);

#endif
// SVTK-HeaderTest-Exclude: svtkPixelExtentIO.h
