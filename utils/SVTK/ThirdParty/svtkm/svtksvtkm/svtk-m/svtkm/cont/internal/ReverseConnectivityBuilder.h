//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_internal_ReverseConnectivityBuilder_h
#define svtk_m_cont_internal_ReverseConnectivityBuilder_h

#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>

#include <svtkm/cont/AtomicArray.h>
#include <svtkm/exec/FunctorBase.h>

#include <utility>

namespace svtkm
{
namespace cont
{
namespace internal
{

namespace rcb
{

template <typename AtomicHistogram, typename ConnInPortal, typename RConnToConnIdxCalc>
struct BuildHistogram : public svtkm::exec::FunctorBase
{
  AtomicHistogram Histo;
  ConnInPortal Conn;
  RConnToConnIdxCalc IdxCalc;

  SVTKM_CONT
  BuildHistogram(const AtomicHistogram& histo,
                 const ConnInPortal& conn,
                 const RConnToConnIdxCalc& idxCalc)
    : Histo(histo)
    , Conn(conn)
    , IdxCalc(idxCalc)
  {
  }

  SVTKM_EXEC
  void operator()(svtkm::Id rconnIdx) const
  {
    // Compute the connectivity array index (skipping cell length entries)
    const svtkm::Id connIdx = this->IdxCalc(rconnIdx);
    const svtkm::Id ptId = this->Conn.Get(connIdx);
    this->Histo.Add(ptId, 1);
  }
};

template <typename AtomicHistogram,
          typename ConnInPortal,
          typename ROffsetInPortal,
          typename RConnOutPortal,
          typename RConnToConnIdxCalc,
          typename ConnIdxToCellIdxCalc>
struct GenerateRConn : public svtkm::exec::FunctorBase
{
  AtomicHistogram Histo;
  ConnInPortal Conn;
  ROffsetInPortal ROffsets;
  RConnOutPortal RConn;
  RConnToConnIdxCalc IdxCalc;
  ConnIdxToCellIdxCalc CellIdCalc;

  SVTKM_CONT
  GenerateRConn(const AtomicHistogram& histo,
                const ConnInPortal& conn,
                const ROffsetInPortal& rOffsets,
                const RConnOutPortal& rconn,
                const RConnToConnIdxCalc& idxCalc,
                const ConnIdxToCellIdxCalc& cellIdCalc)
    : Histo(histo)
    , Conn(conn)
    , ROffsets(rOffsets)
    , RConn(rconn)
    , IdxCalc(idxCalc)
    , CellIdCalc(cellIdCalc)
  {
  }

  SVTKM_EXEC
  void operator()(svtkm::Id inputIdx) const
  {
    // Compute the connectivity array index (skipping cell length entries)
    const svtkm::Id connIdx = this->IdxCalc(inputIdx);
    const svtkm::Id ptId = this->Conn.Get(connIdx);

    // Compute the cell id:
    const svtkm::Id cellId = this->CellIdCalc(connIdx);

    // Find the base offset for this point id:
    const svtkm::Id baseOffset = this->ROffsets.Get(ptId);

    // Find the next unused index for this point id
    const svtkm::Id nextAvailable = this->Histo.Add(ptId, 1);

    // Update the final location in the RConn table with the cellId
    const svtkm::Id rconnIdx = baseOffset + nextAvailable;
    this->RConn.Set(rconnIdx, cellId);
  }
};
}
/// Takes a connectivity array handle (conn) and constructs a reverse
/// connectivity table suitable for use by SVTK-m (rconn).
///
/// This code is generalized for use by SVTK and SVTK-m.
///
/// The Run(...) method is the main entry point. The template parameters are:
/// @param RConnToConnIdxCalc defines `svtkm::Id operator()(svtkm::Id in) const`
/// which computes the index of the in'th point id in conn. This is necessary
/// for SVTK-style cell arrays that need to skip the cell length entries. In
/// svtk-m, this is a no-op passthrough.
/// @param ConnIdxToCellIdxCalc Functor that computes the cell id from an
/// index into conn.
/// @param ConnTag is the StorageTag for the input connectivity array.
///
/// See usages in svtkmCellSetExplicit and svtkmCellSetSingleType for examples.
class ReverseConnectivityBuilder
{
public:
  SVTKM_CONT
  template <typename ConnArray,
            typename RConnArray,
            typename ROffsetsArray,
            typename RConnToConnIdxCalc,
            typename ConnIdxToCellIdxCalc,
            typename Device>
  inline void Run(const ConnArray& conn,
                  RConnArray& rConn,
                  ROffsetsArray& rOffsets,
                  const RConnToConnIdxCalc& rConnToConnCalc,
                  const ConnIdxToCellIdxCalc& cellIdCalc,
                  svtkm::Id numberOfPoints,
                  svtkm::Id rConnSize,
                  Device)
  {
    using Algo = svtkm::cont::DeviceAdapterAlgorithm<Device>;

    auto connPortal = conn.PrepareForInput(Device());
    auto zeros = svtkm::cont::make_ArrayHandleConstant(svtkm::IdComponent{ 0 }, numberOfPoints);

    // Compute RConn offsets by atomically building a histogram and doing an
    // extended scan.
    //
    // Example:
    // (in)  Conn:  | 3  0  1  2  |  3  0  1  3  |  3  0  3  4  |  3  3  4  5  |
    // (out) RNumIndices:  3  2  1  3  2  1
    // (out) RIdxOffsets:  0  3  5  6  9 11 12
    svtkm::cont::ArrayHandle<svtkm::IdComponent> rNumIndices;
    { // allocate and zero the numIndices array:
      Algo::Copy(zeros, rNumIndices);
    }

    { // Build histogram:
      svtkm::cont::AtomicArray<svtkm::IdComponent> atomicCounter{ rNumIndices };
      auto ac = atomicCounter.PrepareForExecution(Device());
      using BuildHisto =
        rcb::BuildHistogram<decltype(ac), decltype(connPortal), RConnToConnIdxCalc>;
      BuildHisto histoGen{ ac, connPortal, rConnToConnCalc };

      Algo::Schedule(histoGen, rConnSize);
    }

    { // Compute offsets:
      Algo::ScanExtended(svtkm::cont::make_ArrayHandleCast<svtkm::Id>(rNumIndices), rOffsets);
    }

    { // Reset the numIndices array to 0's:
      Algo::Copy(zeros, rNumIndices);
    }

    // Fill the connectivity table:
    // 1) Lookup each point idx base offset.
    // 2) Use the atomic histogram to find the next available slot for this
    //    pt id in RConn.
    // 3) Compute the cell id from the connectivity index
    // 4) Update RConn[nextSlot] = cellId
    //
    // Example:
    // (in)    Conn:  | 3  0  1  2  |  3  0  1  3  |  3  0  3  4  |  3  3  4  5  |
    // (inout) RNumIndices:  0  0  0  0  0  0  (Initial)
    // (inout) RNumIndices:  3  2  1  3  2  1  (Final)
    // (in)    RIdxOffsets:  0  3  5  6  9  11
    // (out)   RConn: | 0  1  2  |  0  1  |  0  |  1  2  3  |  2  3  |  3  |
    {
      svtkm::cont::AtomicArray<svtkm::IdComponent> atomicCounter{ rNumIndices };
      auto ac = atomicCounter.PrepareForExecution(Device());
      auto rOffsetPortal = rOffsets.PrepareForInput(Device());
      auto rConnPortal = rConn.PrepareForOutput(rConnSize, Device());

      using GenRConnT = rcb::GenerateRConn<decltype(ac),
                                           decltype(connPortal),
                                           decltype(rOffsetPortal),
                                           decltype(rConnPortal),
                                           RConnToConnIdxCalc,
                                           ConnIdxToCellIdxCalc>;
      GenRConnT rConnGen{ ac, connPortal, rOffsetPortal, rConnPortal, rConnToConnCalc, cellIdCalc };

      Algo::Schedule(rConnGen, rConnSize);
    }
  }
};
}
}
} // end namespace svtkm::cont::internal

#endif // ReverseConnectivityBuilder_h
