/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPPixelTransfer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPPixelTransfer
 *
 * class to handle inter-process communication of pixel data from
 * non-contiguous regions of a shared index space. For example copying
 * a subset of one image to a subset of another. The class can be used
 * for purely local(no MPI) non-contigious data transfers by setting
 * the source and destination ranks to the same id. In that case
 * memcpy is used.
 *
 * @sa
 * svtkPixelExtent
 */

#ifndef svtkPPixelTransfer_h
#define svtkPPixelTransfer_h

#include "svtkMPI.h"          // for mpi
#include "svtkMPIPixelTT.h"   // for type traits
#include "svtkMPIPixelView.h" // for mpi subarrays
#include "svtkPixelExtent.h"  // for pixel extent
#include "svtkPixelTransfer.h"
#include "svtkRenderingParallelLICModule.h" // for export
#include "svtkSetGet.h"                     // for macros

// included svtkSystemIncludes.h in the base class.
#include <cstring>  // for memcpy
#include <iostream> // for ostream
#include <vector>   // for vector

// #define svtkPPixelTransferDEBUG

class SVTKRENDERINGPARALLELLIC_EXPORT svtkPPixelTransfer : public svtkPixelTransfer
{
public:
  svtkPPixelTransfer()
    : SrcRank(0)
    , DestRank(0)
    , UseBlockingSend(0)
    , UseBlockingRecv(0)
  {
  }

  /**
   * Initialize a transaction from sub extent of source to sub extent
   * of dest, where the subsets are different.
   */
  svtkPPixelTransfer(int srcRank, const svtkPixelExtent& srcWholeExt, const svtkPixelExtent& srcExt,
    int destRank, const svtkPixelExtent& destWholeExt, const svtkPixelExtent& destExt, int id = 0)
    : Id(id)
    , SrcRank(srcRank)
    , SrcWholeExt(srcWholeExt)
    , SrcExt(srcExt)
    , DestRank(destRank)
    , DestWholeExt(destWholeExt)
    , DestExt(destExt)
    , UseBlockingSend(0)
    , UseBlockingRecv(0)
  {
  }

  /**
   * Initialize a transaction from sub extent of source to sub extent
   * of dest, where the subsets are the same.
   */
  svtkPPixelTransfer(int srcRank, const svtkPixelExtent& srcWholeExt, const svtkPixelExtent& targetExt,
    int destRank, const svtkPixelExtent& destWholeExt, int id)
    : Id(id)
    , SrcRank(srcRank)
    , SrcWholeExt(srcWholeExt)
    , SrcExt(targetExt)
    , DestRank(destRank)
    , DestWholeExt(destWholeExt)
    , DestExt(targetExt)
    , UseBlockingSend(0)
    , UseBlockingRecv(0)
  {
  }

  /**
   * Initialize a transaction from sub extent of source to sub extent
   * of dest, both the whole and the subsets are the same.
   */
  svtkPPixelTransfer(int srcRank, int destRank, const svtkPixelExtent& wholeExt,
    const svtkPixelExtent& targetExt, int id = 0)
    : Id(id)
    , SrcRank(srcRank)
    , SrcWholeExt(wholeExt)
    , SrcExt(targetExt)
    , DestRank(destRank)
    , DestWholeExt(wholeExt)
    , DestExt(targetExt)
    , UseBlockingSend(0)
    , UseBlockingRecv(0)
  {
  }

  /**
   * Initialize a transaction from sub extent of source to sub extent
   * of dest, both the whole and the subsets are the same.
   */
  svtkPPixelTransfer(int srcRank, int destRank, const svtkPixelExtent& ext, int id = 0)
    : Id(id)
    , SrcRank(srcRank)
    , SrcWholeExt(ext)
    , SrcExt(ext)
    , DestRank(destRank)
    , DestWholeExt(ext)
    , DestExt(ext)
    , UseBlockingSend(0)
    , UseBlockingRecv(0)
  {
  }

  /**
   * Initialize a transaction from whole extent of source to whole extent
   * of dest, where source and destination have different whole extents.
   */
  svtkPPixelTransfer(int srcRank, const svtkPixelExtent& srcWholeExt, int destRank,
    const svtkPixelExtent& destWholeExt, int id = 0)
    : Id(id)
    , SrcRank(srcRank)
    , SrcWholeExt(srcWholeExt)
    , SrcExt(srcWholeExt)
    , DestRank(destRank)
    , DestWholeExt(destWholeExt)
    , DestExt(destWholeExt)
    , UseBlockingSend(0)
    , UseBlockingRecv(0)
  {
  }

  /**
   * Initialize a transaction from sub extent of source to sub extent
   * of dest, where the subsets are different. This is a local
   * operation there will be no communication.
   */
  svtkPPixelTransfer(const svtkPixelExtent& srcWholeExt, const svtkPixelExtent& srcExt,
    const svtkPixelExtent& destWholeExt, const svtkPixelExtent& destExt)
    : Id(0)
    , SrcRank(0)
    , SrcWholeExt(srcWholeExt)
    , SrcExt(srcExt)
    , DestRank(0)
    , DestWholeExt(destWholeExt)
    , DestExt(destExt)
    , UseBlockingSend(0)
    , UseBlockingRecv(0)
  {
  }

  ~svtkPPixelTransfer() {}

  /**
   * Set/Get the MPI rank of source and destination
   * processes.
   */
  void SetSourceRank(int rank) { this->SrcRank = rank; }

  int GetSourceRank() const { return this->SrcRank; }

  void SetDestinationRank(int rank) { this->DestRank = rank; }

  int GetDestinationRank() const { return this->DestRank; }

  /**
   * Tests to determine a given rank's role in this transaction.
   * If both Sender and Receiver are true then the operation
   * is local and no mpi calls are made.
   */
  bool Sender(int rank) const { return (this->SrcRank == rank); }
  bool Receiver(int rank) const { return (this->DestRank == rank); }
  bool Local(int rank) const { return (this->Sender(rank) && this->Receiver(rank)); }

  /**
   * Set/Get the source extent. This is the extent of the
   * array that data will be copied from.
   */
  void SetSourceWholeExtent(svtkPixelExtent& srcExt) { this->SrcWholeExt = srcExt; }

  svtkPixelExtent& GetSourceWholeExtent() { return this->SrcWholeExt; }

  const svtkPixelExtent& GetSourceWholeExtent() const { return this->SrcWholeExt; }

  /**
   * Set/Get the source extent. This is the subset extent in the
   * array that data will be copied from.
   */
  void SetSourceExtent(svtkPixelExtent& srcExt) { this->SrcExt = srcExt; }

  svtkPixelExtent& GetSourceExtent() { return this->SrcExt; }

  const svtkPixelExtent& GetSourceExtent() const { return this->SrcExt; }

  /**
   * Set/get the destination extent. This is the extent
   * of array that will recveive the data.
   */
  void SetDestinationWholeExtent(svtkPixelExtent& destExt) { this->DestWholeExt = destExt; }

  svtkPixelExtent& GetDestinationWholeExtent() { return this->DestWholeExt; }

  const svtkPixelExtent& GetDestinationWholeExtent() const { return this->DestWholeExt; }

  /**
   * Set/get the destination extent. This is the subset extent
   * in the array that will recveive the data.
   */
  void SetDestinationExtent(svtkPixelExtent& destExt) { this->DestExt = destExt; }

  svtkPixelExtent& GetDestinationExtent() { return this->DestExt; }

  const svtkPixelExtent& GetDestinationExtent() const { return this->DestExt; }

  /**
   * Set/get the transaction id.
   */
  void SetTransactionId(int id) { this->Id = id; }

  int GetTransactionId() const { return this->Id; }

  /**
   * Enable/diasable non-blocking communication
   */
  void SetUseBlockingSend(int val) { this->UseBlockingSend = val; }

  int GetUseBlockingSend() const { return this->UseBlockingSend; }

  void SetUseBlockingRecv(int val) { this->UseBlockingRecv = val; }

  int GetUseBlockingRecv() const { return this->UseBlockingRecv; }

  /**
   * Transfer data from source to destination.
   */
  template <typename SOURCE_TYPE, typename DEST_TYPE>
  int Execute(MPI_Comm comm, int rank, int nComps, SOURCE_TYPE* srcData, DEST_TYPE* destData,
    std::vector<MPI_Request>& reqs, std::deque<MPI_Datatype>& types, int tag);

  /**
   * Transfer data from source to destination. convenience for working
   * with svtk data type enum rather than c types.
   */
  int Execute(MPI_Comm comm, int rank, int nComps, int srcType, void* srcData, int destType,
    void* destData, std::vector<MPI_Request>& reqs, std::deque<MPI_Datatype>& types, int tag);

  /**
   * Block transfer for local memory to memory transfers, without using mpi.
   */
  int Blit(int nComps, int srcType, void* srcData, int destType, void* destData);

private:
  // distpatch helper for svtk data type enum
  template <typename SOURCE_TYPE>
  int Execute(MPI_Comm comm, int rank, int nComps, SOURCE_TYPE* srcData, int destType,
    void* destData, std::vector<MPI_Request>& reqs, std::deque<MPI_Datatype>& types, int tag);

private:
  int Id;                      // transaction id
  int SrcRank;                 // rank who owns source memory
  svtkPixelExtent SrcWholeExt;  // source extent
  svtkPixelExtent SrcExt;       // source subset to transfer
  int DestRank;                // rank who owns destination memory
  svtkPixelExtent DestWholeExt; // destination extent
  svtkPixelExtent DestExt;      // destination subset
  int UseBlockingSend;         // controls for non-blocking comm
  int UseBlockingRecv;
};

//-----------------------------------------------------------------------------
template <typename SOURCE_TYPE>
int svtkPPixelTransfer::Execute(MPI_Comm comm, int rank, int nComps, SOURCE_TYPE* srcData,
  int destType, void* destData, std::vector<MPI_Request>& reqs, std::deque<MPI_Datatype>& types,
  int tag)
{
  // second layer of dispatch
  switch (destType)
  {
    svtkTemplateMacro(
      return this->Execute(comm, rank, nComps, srcData, (SVTK_TT*)destData, reqs, types, tag));
  }
  return 0;
}

//-----------------------------------------------------------------------------
template <typename SOURCE_TYPE, typename DEST_TYPE>
int svtkPPixelTransfer::Execute(MPI_Comm comm, int rank, int nComps, SOURCE_TYPE* srcData,
  DEST_TYPE* destData, std::vector<MPI_Request>& reqs, std::deque<MPI_Datatype>& types, int tag)
{
  int iErr = 0;
  if ((comm == MPI_COMM_NULL) || (this->Local(rank)))
  {
    // transaction is local, bypass mpi in favor of memcpy
    return svtkPixelTransfer::Blit(this->SrcWholeExt, this->SrcExt, this->DestWholeExt,
      this->DestExt, nComps, srcData, nComps, destData);
  }

  if (rank == this->DestRank)
  {
    // use mpi to receive the data
    if (destData == NULL)
    {
      return -1;
    }

    MPI_Datatype subarray;
    iErr = svtkMPIPixelViewNew<DEST_TYPE>(this->DestWholeExt, this->DestExt, nComps, subarray);
    if (iErr)
    {
      return -4;
    }

    if (this->UseBlockingRecv)
    {
      MPI_Status stat;
      iErr = MPI_Recv(destData, 1, subarray, this->SrcRank, tag, comm, &stat);
    }
    else
    {
      reqs.push_back(MPI_REQUEST_NULL);
      iErr = MPI_Irecv(destData, 1, subarray, this->SrcRank, tag, comm, &reqs.back());
    }

#define HOLD_RECV_TYPES
#ifdef HOLD_RECV_YPES
    types.push_back(subarray);
#else
    MPI_Type_free(&subarray);
#endif

    if (iErr)
    {
      return -5;
    }
  }

  if (rank == this->SrcRank)
  {
    // use mpi to send the data
    if (srcData == NULL)
    {
      return -1;
    }

    MPI_Datatype subarray;
    iErr = svtkMPIPixelViewNew<SOURCE_TYPE>(this->SrcWholeExt, this->SrcExt, nComps, subarray);
    if (iErr)
    {
      return -2;
    }

    if (this->UseBlockingSend)
    {
      iErr = MPI_Ssend(srcData, 1, subarray, this->DestRank, tag, comm);
    }
    else
    {
      MPI_Request req;
      iErr = MPI_Isend(srcData, 1, subarray, this->DestRank, tag, comm, &req);
#define SAVE_SEND_REQS
#ifdef SAVE_SEND_REQS
      reqs.push_back(req);
#else
      MPI_Request_free(&req);
#endif
    }

#define HOLD_SEND_TYPES
#ifdef HOLD_SEND_TYPES
    types.push_back(subarray);
#else
    MPI_Type_free(&subarray);
#endif

    if (iErr)
    {
      return -3;
    }
  }

  return iErr;
}

SVTKRENDERINGPARALLELLIC_EXPORT
ostream& operator<<(std::ostream& os, const svtkPPixelTransfer& gt);

#endif
// SVTK-HeaderTest-Exclude: svtkPPixelTransfer.h
