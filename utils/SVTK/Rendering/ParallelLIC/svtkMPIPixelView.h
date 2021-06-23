/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMPIPixelView.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMPIPixelView
 * MPI datatypes that describe a svtkPixelExtent.
 */

#ifndef svtkMPIPixelView_h
#define svtkMPIPixelView_h

#include "svtkMPI.h"         // for mpi
#include "svtkMPIPixelTT.h"  // for type traits
#include "svtkPixelExtent.h" // for pixel extent
#include <iostream>         // for cerr

//-----------------------------------------------------------------------------
template <typename T>
int svtkMPIPixelViewNew(
  const svtkPixelExtent& domain, const svtkPixelExtent& decomp, int nComps, MPI_Datatype& view)
{
#ifndef NDEBUG
  int mpiOk = 0;
  MPI_Initialized(&mpiOk);
  if (!mpiOk)
  {
    std::cerr << "This class requires the MPI runtime." << std::endl;
    return -1;
  }
#endif

  int iErr;

  MPI_Datatype nativeType;
  iErr = MPI_Type_contiguous(nComps, svtkMPIPixelTT<T>::MPIType, &nativeType);
  if (iErr)
  {
    return -2;
  }

  int domainDims[2];
  domain.Size(domainDims);

  int domainStart[2];
  domain.GetStartIndex(domainStart);

  int decompDims[2];
  decomp.Size(decompDims);

  int decompStart[2];
  decomp.GetStartIndex(decompStart, domainStart);

  // use a contiguous type when possible.
  if (domain == decomp)
  {
    unsigned long long nCells = decomp.Size();
    iErr = MPI_Type_contiguous((int)nCells, nativeType, &view);
    if (iErr)
    {
      MPI_Type_free(&nativeType);
      return -3;
    }
  }
  else
  {
    iErr = MPI_Type_create_subarray(
      2, domainDims, decompDims, decompStart, MPI_ORDER_FORTRAN, nativeType, &view);
    if (iErr)
    {
      MPI_Type_free(&nativeType);
      return -4;
    }
  }
  iErr = MPI_Type_commit(&view);
  if (iErr)
  {
    MPI_Type_free(&nativeType);
    return -5;
  }

  MPI_Type_free(&nativeType);

  return 0;
}

#endif
// SVTK-HeaderTest-Exclude: svtkMPIPixelView.h
