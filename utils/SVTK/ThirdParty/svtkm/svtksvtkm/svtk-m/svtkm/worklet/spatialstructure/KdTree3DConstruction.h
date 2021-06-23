//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#ifndef svtk_m_worklet_KdTree3DConstruction_h
#define svtk_m_worklet_KdTree3DConstruction_h

#include <svtkm/Math.h>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/ArrayHandleReverse.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/arg/ControlSignatureTagBase.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>
#include <svtkm/cont/testing/Testing.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/internal/DispatcherBase.h>
#include <svtkm/worklet/internal/WorkletBase.h>

namespace svtkm
{
namespace worklet
{
namespace spatialstructure
{

class KdTree3DConstruction
{
public:
  ////////// General WORKLET for Kd-tree  //////
  class ComputeFlag : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn rank, FieldIn pointCountInSeg, FieldOut flag);
    using ExecutionSignature = void(_1, _2, _3);

    SVTKM_CONT
    ComputeFlag() {}

    template <typename T>
    SVTKM_EXEC void operator()(const T& rank, const T& pointCountInSeg, T& flag) const
    {
      if (static_cast<float>(rank) >= static_cast<float>(pointCountInSeg) / 2.0f)
        flag = 1; //right subtree
      else
        flag = 0; //left subtree
    }
  };

  class InverseArray : public svtkm::worklet::WorkletMapField
  { //only for 0/1 array
  public:
    using ControlSignature = void(FieldIn in, FieldOut out);
    using ExecutionSignature = void(_1, _2);

    SVTKM_CONT
    InverseArray() {}

    template <typename T>
    SVTKM_EXEC void operator()(const T& in, T& out) const
    {
      if (in == 0)
        out = 1;
      else
        out = 0;
    }
  };

  class SegmentedSplitTransform : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature =
      void(FieldIn B, FieldIn D, FieldIn F, FieldIn G, FieldIn H, FieldOut I);
    using ExecutionSignature = void(_1, _2, _3, _4, _5, _6);

    SVTKM_CONT
    SegmentedSplitTransform() {}

    template <typename T>
    SVTKM_EXEC void operator()(const T& B, const T& D, const T& F, const T& G, const T& H, T& I)
      const
    {
      if (B == 1)
      {
        I = F + H + D;
      }
      else
      {
        I = F + G - 1;
      }
    }
  };

  class ScatterArray : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn in, FieldIn index, WholeArrayOut out);
    using ExecutionSignature = void(_1, _2, _3);

    SVTKM_CONT
    ScatterArray() {}

    template <typename T, typename OutputArrayPortalType>
    SVTKM_EXEC void operator()(const T& in,
                              const T& index,
                              const OutputArrayPortalType& outputPortal) const
    {
      outputPortal.Set(index, in);
    }
  };

  class NewSegmentId : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn inSegmentId, FieldIn flag, FieldOut outSegmentId);
    using ExecutionSignature = void(_1, _2, _3);

    SVTKM_CONT
    NewSegmentId() {}

    template <typename T>
    SVTKM_EXEC void operator()(const T& oldSegId, const T& flag, T& newSegId) const
    {
      if (flag == 0)
        newSegId = oldSegId * 2;
      else
        newSegId = oldSegId * 2 + 1;
    }
  };

  class SaveSplitPointId : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn pointId,
                                  FieldIn flag,
                                  FieldIn oldSplitPointId,
                                  FieldOut newSplitPointId);
    using ExecutionSignature = void(_1, _2, _3, _4);

    SVTKM_CONT
    SaveSplitPointId() {}

    template <typename T>
    SVTKM_EXEC void operator()(const T& pointId,
                              const T& flag,
                              const T& oldSplitPointId,
                              T& newSplitPointId) const
    {
      if (flag == 0) //do not change
        newSplitPointId = oldSplitPointId;
      else //split point id
        newSplitPointId = pointId;
    }
  };

  class FindSplitPointId : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn pointId, FieldIn rank, FieldOut splitIdInsegment);
    using ExecutionSignature = void(_1, _2, _3);

    SVTKM_CONT
    FindSplitPointId() {}

    template <typename T>
    SVTKM_EXEC void operator()(const T& pointId, const T& rank, T& splitIdInsegment) const
    {
      if (rank == 0) //do not change
        splitIdInsegment = pointId;
      else                     //split point id
        splitIdInsegment = -1; //indicate this is not split point
    }
  };

  class ArrayAdd : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn inArray0, FieldIn inArray1, FieldOut outArray);
    using ExecutionSignature = void(_1, _2, _3);

    SVTKM_CONT
    ArrayAdd() {}

    template <typename T>
    SVTKM_EXEC void operator()(const T& in0, const T& in1, T& out) const
    {
      out = in0 + in1;
    }
  };

  class SeprateVec3AryHandle : public svtkm::worklet::WorkletMapField
  {
  public:
    using ControlSignature = void(FieldIn inVec3, FieldOut out0, FieldOut out1, FieldOut out2);
    using ExecutionSignature = void(_1, _2, _3, _4);

    SVTKM_CONT
    SeprateVec3AryHandle() {}

    template <typename T>
    SVTKM_EXEC void operator()(const Vec<T, 3>& inVec3, T& out0, T& out1, T& out2) const
    {
      out0 = inVec3[0];
      out1 = inVec3[1];
      out2 = inVec3[2];
    }
  };

  ////////// General worklet WRAPPER for Kd-tree //////
  template <typename T, class BinaryFunctor>
  svtkm::cont::ArrayHandle<T> ReverseScanInclusiveByKey(svtkm::cont::ArrayHandle<T>& keyHandle,
                                                       svtkm::cont::ArrayHandle<T>& dataHandle,
                                                       BinaryFunctor binary_functor)
  {
    using Algorithm = svtkm::cont::Algorithm;

    svtkm::cont::ArrayHandle<T> resultHandle;

    auto reversedResultHandle = svtkm::cont::make_ArrayHandleReverse(resultHandle);

    Algorithm::ScanInclusiveByKey(svtkm::cont::make_ArrayHandleReverse(keyHandle),
                                  svtkm::cont::make_ArrayHandleReverse(dataHandle),
                                  reversedResultHandle,
                                  binary_functor);

    return resultHandle;
  }

  template <typename T>
  svtkm::cont::ArrayHandle<T> Inverse01ArrayWrapper(svtkm::cont::ArrayHandle<T>& inputHandle)
  {
    svtkm::cont::ArrayHandle<T> InverseHandle;
    InverseArray invWorklet;
    svtkm::worklet::DispatcherMapField<InverseArray> inverseArrayDispatcher(invWorklet);
    inverseArrayDispatcher.Invoke(inputHandle, InverseHandle);
    return InverseHandle;
  }

  template <typename T>
  svtkm::cont::ArrayHandle<T> ScatterArrayWrapper(svtkm::cont::ArrayHandle<T>& inputHandle,
                                                 svtkm::cont::ArrayHandle<T>& indexHandle)
  {
    svtkm::cont::ArrayHandle<T> outputHandle;
    outputHandle.Allocate(inputHandle.GetNumberOfValues());
    ScatterArray scatterWorklet;
    svtkm::worklet::DispatcherMapField<ScatterArray> scatterArrayDispatcher(scatterWorklet);
    scatterArrayDispatcher.Invoke(inputHandle, indexHandle, outputHandle);
    return outputHandle;
  }

  template <typename T>
  svtkm::cont::ArrayHandle<T> NewKeyWrapper(svtkm::cont::ArrayHandle<T>& oldSegIdHandle,
                                           svtkm::cont::ArrayHandle<T>& flagHandle)
  {
    svtkm::cont::ArrayHandle<T> newSegIdHandle;
    NewSegmentId newsegidWorklet;
    svtkm::worklet::DispatcherMapField<NewSegmentId> newSegIdDispatcher(newsegidWorklet);
    newSegIdDispatcher.Invoke(oldSegIdHandle, flagHandle, newSegIdHandle);
    return newSegIdHandle;
  }

  template <typename T>
  svtkm::cont::ArrayHandle<T> SaveSplitPointIdWrapper(svtkm::cont::ArrayHandle<T>& pointIdHandle,
                                                     svtkm::cont::ArrayHandle<T>& flagHandle,
                                                     svtkm::cont::ArrayHandle<T>& rankHandle,
                                                     svtkm::cont::ArrayHandle<T>& oldSplitIdHandle)
  {
    svtkm::cont::ArrayHandle<T> splitIdInSegmentHandle;
    FindSplitPointId findSplitPointIdWorklet;
    svtkm::worklet::DispatcherMapField<FindSplitPointId> findSplitPointIdWorkletDispatcher(
      findSplitPointIdWorklet);
    findSplitPointIdWorkletDispatcher.Invoke(pointIdHandle, rankHandle, splitIdInSegmentHandle);

    svtkm::cont::ArrayHandle<T> splitIdInSegmentByScanHandle =
      ReverseScanInclusiveByKey(flagHandle, splitIdInSegmentHandle, svtkm::Maximum());

    svtkm::cont::ArrayHandle<T> splitIdHandle;
    SaveSplitPointId saveSplitPointIdWorklet;
    svtkm::worklet::DispatcherMapField<SaveSplitPointId> saveSplitPointIdWorkletDispatcher(
      saveSplitPointIdWorklet);
    saveSplitPointIdWorkletDispatcher.Invoke(
      splitIdInSegmentByScanHandle, flagHandle, oldSplitIdHandle, splitIdHandle);

    return splitIdHandle;
  }

  template <typename T>
  svtkm::cont::ArrayHandle<T> ArrayAddWrapper(svtkm::cont::ArrayHandle<T>& array0Handle,
                                             svtkm::cont::ArrayHandle<T>& array1Handle)
  {
    svtkm::cont::ArrayHandle<T> resultHandle;
    ArrayAdd arrayAddWorklet;
    svtkm::worklet::DispatcherMapField<ArrayAdd> arrayAddDispatcher(arrayAddWorklet);
    arrayAddDispatcher.Invoke(array0Handle, array1Handle, resultHandle);
    return resultHandle;
  }

  ///////////////////////////////////////////////////
  ////////General Kd tree function //////////////////
  ///////////////////////////////////////////////////
  template <typename T>
  svtkm::cont::ArrayHandle<T> ComputeFlagProcedure(svtkm::cont::ArrayHandle<T>& rankHandle,
                                                  svtkm::cont::ArrayHandle<T>& segIdHandle)
  {
    using Algorithm = svtkm::cont::Algorithm;

    svtkm::cont::ArrayHandle<T> segCountAryHandle;
    {
      svtkm::cont::ArrayHandle<T> tmpAryHandle;
      svtkm::cont::ArrayHandleConstant<T> constHandle(1, rankHandle.GetNumberOfValues());
      Algorithm::ScanInclusiveByKey(
        segIdHandle, constHandle, tmpAryHandle, svtkm::Add()); //compute ttl segs in segment

      segCountAryHandle = ReverseScanInclusiveByKey(segIdHandle, tmpAryHandle, svtkm::Maximum());
    }

    svtkm::cont::ArrayHandle<T> flagHandle;
    svtkm::worklet::DispatcherMapField<ComputeFlag> computeFlagDispatcher;
    computeFlagDispatcher.Invoke(rankHandle, segCountAryHandle, flagHandle);

    return flagHandle;
  }

  template <typename T>
  svtkm::cont::ArrayHandle<T> SegmentedSplitProcedure(svtkm::cont::ArrayHandle<T>& A_Handle,
                                                     svtkm::cont::ArrayHandle<T>& B_Handle,
                                                     svtkm::cont::ArrayHandle<T>& C_Handle)
  {
    using Algorithm = svtkm::cont::Algorithm;

    svtkm::cont::ArrayHandle<T> D_Handle;
    T initValue = 0;
    Algorithm::ScanExclusiveByKey(C_Handle, B_Handle, D_Handle, initValue, svtkm::Add());

    svtkm::cont::ArrayHandleCounting<T> Ecouting_Handle(0, 1, A_Handle.GetNumberOfValues());
    svtkm::cont::ArrayHandle<T> E_Handle;
    Algorithm::Copy(Ecouting_Handle, E_Handle);

    svtkm::cont::ArrayHandle<T> F_Handle;
    Algorithm::ScanInclusiveByKey(C_Handle, E_Handle, F_Handle, svtkm::Minimum());

    svtkm::cont::ArrayHandle<T> InvB_Handle = Inverse01ArrayWrapper(B_Handle);
    svtkm::cont::ArrayHandle<T> G_Handle;
    Algorithm::ScanInclusiveByKey(C_Handle, InvB_Handle, G_Handle, svtkm::Add());

    svtkm::cont::ArrayHandle<T> H_Handle =
      ReverseScanInclusiveByKey(C_Handle, G_Handle, svtkm::Maximum());

    svtkm::cont::ArrayHandle<T> I_Handle;
    SegmentedSplitTransform sstWorklet;
    svtkm::worklet::DispatcherMapField<SegmentedSplitTransform> segmentedSplitTransformDispatcher(
      sstWorklet);
    segmentedSplitTransformDispatcher.Invoke(
      B_Handle, D_Handle, F_Handle, G_Handle, H_Handle, I_Handle);

    return ScatterArrayWrapper(A_Handle, I_Handle);
  }

  template <typename T>
  void RenumberRanksProcedure(svtkm::cont::ArrayHandle<T>& A_Handle,
                              svtkm::cont::ArrayHandle<T>& B_Handle,
                              svtkm::cont::ArrayHandle<T>& C_Handle,
                              svtkm::cont::ArrayHandle<T>& D_Handle)
  {
    using Algorithm = svtkm::cont::Algorithm;

    svtkm::Id nPoints = A_Handle.GetNumberOfValues();

    svtkm::cont::ArrayHandleCounting<T> Ecouting_Handle(0, 1, nPoints);
    svtkm::cont::ArrayHandle<T> E_Handle;
    Algorithm::Copy(Ecouting_Handle, E_Handle);

    svtkm::cont::ArrayHandle<T> F_Handle;
    Algorithm::ScanInclusiveByKey(D_Handle, E_Handle, F_Handle, svtkm::Minimum());

    svtkm::cont::ArrayHandle<T> G_Handle;
    G_Handle = ArrayAddWrapper(A_Handle, F_Handle);

    svtkm::cont::ArrayHandleConstant<T> HConstant_Handle(1, nPoints);
    svtkm::cont::ArrayHandle<T> H_Handle;
    Algorithm::Copy(HConstant_Handle, H_Handle);

    svtkm::cont::ArrayHandle<T> I_Handle;
    T initValue = 0;
    Algorithm::ScanExclusiveByKey(C_Handle, H_Handle, I_Handle, initValue, svtkm::Add());

    svtkm::cont::ArrayHandle<T> J_Handle;
    J_Handle = ScatterArrayWrapper(I_Handle, G_Handle);

    svtkm::cont::ArrayHandle<T> K_Handle;
    K_Handle = ScatterArrayWrapper(B_Handle, G_Handle);

    svtkm::cont::ArrayHandle<T> L_Handle;
    L_Handle = SegmentedSplitProcedure(J_Handle, K_Handle, D_Handle);

    svtkm::cont::ArrayHandle<T> M_Handle;
    Algorithm::ScanInclusiveByKey(C_Handle, E_Handle, M_Handle, svtkm::Minimum());

    svtkm::cont::ArrayHandle<T> N_Handle;
    N_Handle = ArrayAddWrapper(L_Handle, M_Handle);

    A_Handle = ScatterArrayWrapper(I_Handle, N_Handle);
  }

  /////////////3D construction      /////////////////////
  /// \brief Segmented split for 3D x, y, z coordinates
  ///
  /// Split \c pointId_Handle, \c X_Handle, \c Y_Handle and \c Z_Handle within each segment
  /// as indicated by \c segId_Handle according to flags in \c flag_Handle.
  ///
  /// \tparam T
  /// \param pointId_Handle
  /// \param flag_Handle
  /// \param segId_Handle
  /// \param X_Handle
  /// \param Y_Handle
  /// \param Z_Handle
  template <typename T>
  void SegmentedSplitProcedure3D(svtkm::cont::ArrayHandle<T>& pointId_Handle,
                                 svtkm::cont::ArrayHandle<T>& flag_Handle,
                                 svtkm::cont::ArrayHandle<T>& segId_Handle,
                                 svtkm::cont::ArrayHandle<T>& X_Handle,
                                 svtkm::cont::ArrayHandle<T>& Y_Handle,
                                 svtkm::cont::ArrayHandle<T>& Z_Handle)
  {
    using Algorithm = svtkm::cont::Algorithm;

    svtkm::cont::ArrayHandle<T> D_Handle;
    T initValue = 0;
    Algorithm::ScanExclusiveByKey(segId_Handle, flag_Handle, D_Handle, initValue, svtkm::Add());

    svtkm::cont::ArrayHandleCounting<T> Ecouting_Handle(0, 1, pointId_Handle.GetNumberOfValues());
    svtkm::cont::ArrayHandle<T> E_Handle;
    Algorithm::Copy(Ecouting_Handle, E_Handle);

    svtkm::cont::ArrayHandle<T> F_Handle;
    Algorithm::ScanInclusiveByKey(segId_Handle, E_Handle, F_Handle, svtkm::Minimum());

    svtkm::cont::ArrayHandle<T> InvB_Handle = Inverse01ArrayWrapper(flag_Handle);
    svtkm::cont::ArrayHandle<T> G_Handle;
    Algorithm::ScanInclusiveByKey(segId_Handle, InvB_Handle, G_Handle, svtkm::Add());

    svtkm::cont::ArrayHandle<T> H_Handle =
      ReverseScanInclusiveByKey(segId_Handle, G_Handle, svtkm::Maximum());

    svtkm::cont::ArrayHandle<T> I_Handle;
    SegmentedSplitTransform sstWorklet;
    svtkm::worklet::DispatcherMapField<SegmentedSplitTransform> segmentedSplitTransformDispatcher(
      sstWorklet);
    segmentedSplitTransformDispatcher.Invoke(
      flag_Handle, D_Handle, F_Handle, G_Handle, H_Handle, I_Handle);

    pointId_Handle = ScatterArrayWrapper(pointId_Handle, I_Handle);

    flag_Handle = ScatterArrayWrapper(flag_Handle, I_Handle);

    X_Handle = ScatterArrayWrapper(X_Handle, I_Handle);

    Y_Handle = ScatterArrayWrapper(Y_Handle, I_Handle);

    Z_Handle = ScatterArrayWrapper(Z_Handle, I_Handle);
  }

  /// \brief Perform one level of KD-Tree construction
  ///
  /// Construct a level of KD-Tree by segemeted splits (partitioning) of \c pointId_Handle,
  /// \c xrank_Handle, \c yrank_Handle and \c zrank_Handle according to the medium element
  /// in each segment as indicated by \c segId_Handle alone the axis determined by \c level.
  /// The split point of each segment will be updated in \c splitId_Handle.
  template <typename T>
  void OneLevelSplit3D(svtkm::cont::ArrayHandle<T>& pointId_Handle,
                       svtkm::cont::ArrayHandle<T>& xrank_Handle,
                       svtkm::cont::ArrayHandle<T>& yrank_Handle,
                       svtkm::cont::ArrayHandle<T>& zrank_Handle,
                       svtkm::cont::ArrayHandle<T>& segId_Handle,
                       svtkm::cont::ArrayHandle<T>& splitId_Handle,
                       svtkm::Int32 level)
  {
    using Algorithm = svtkm::cont::Algorithm;

    svtkm::cont::ArrayHandle<T> flag_Handle;
    if (level % 3 == 0)
    {
      flag_Handle = ComputeFlagProcedure(xrank_Handle, segId_Handle);
    }
    else if (level % 3 == 1)
    {
      flag_Handle = ComputeFlagProcedure(yrank_Handle, segId_Handle);
    }
    else
    {
      flag_Handle = ComputeFlagProcedure(zrank_Handle, segId_Handle);
    }

    SegmentedSplitProcedure3D(
      pointId_Handle, flag_Handle, segId_Handle, xrank_Handle, yrank_Handle, zrank_Handle);

    svtkm::cont::ArrayHandle<T> segIdOld_Handle;
    Algorithm::Copy(segId_Handle, segIdOld_Handle);
    segId_Handle = NewKeyWrapper(segIdOld_Handle, flag_Handle);

    RenumberRanksProcedure(xrank_Handle, flag_Handle, segId_Handle, segIdOld_Handle);
    RenumberRanksProcedure(yrank_Handle, flag_Handle, segId_Handle, segIdOld_Handle);
    RenumberRanksProcedure(zrank_Handle, flag_Handle, segId_Handle, segIdOld_Handle);

    if (level % 3 == 0)
    {
      splitId_Handle =
        SaveSplitPointIdWrapper(pointId_Handle, flag_Handle, xrank_Handle, splitId_Handle);
    }
    else if (level % 3 == 1)
    {
      splitId_Handle =
        SaveSplitPointIdWrapper(pointId_Handle, flag_Handle, yrank_Handle, splitId_Handle);
    }
    else
    {
      splitId_Handle =
        SaveSplitPointIdWrapper(pointId_Handle, flag_Handle, zrank_Handle, splitId_Handle);
    }
  }

  /// \brief Construct KdTree from x y z coordinate vector.
  ///
  /// This method constructs an array based KD-Tree from x, y, z coordinates of points in \c
  /// coordi_Handle. The method rotates between x, y and z axis and splits input points into
  /// equal halves with respect to the split axis at each level of construction. The indices to
  /// the leaf nodes are returned in \c pointId_Handle and indices to internal nodes (splits)
  /// are returned in splitId_handle.
  ///
  /// \param coordi_Handle (in) x, y, z coordinates of input points
  /// \param pointId_Handle (out) returns indices to leaf nodes of the KD-tree
  /// \param splitId_Handle (out) returns indices to internal nodes of the KD-tree
  // Leaf Node vector and internal node (split) vectpr
  template <typename CoordType, typename CoordStorageTag>
  void Run(const svtkm::cont::ArrayHandle<svtkm::Vec<CoordType, 3>, CoordStorageTag>& coordi_Handle,
           svtkm::cont::ArrayHandle<svtkm::Id>& pointId_Handle,
           svtkm::cont::ArrayHandle<svtkm::Id>& splitId_Handle)
  {
    using Algorithm = svtkm::cont::Algorithm;

    svtkm::Id nTrainingPoints = coordi_Handle.GetNumberOfValues();
    svtkm::cont::ArrayHandleCounting<svtkm::Id> counting_Handle(0, 1, nTrainingPoints);
    Algorithm::Copy(counting_Handle, pointId_Handle);
    svtkm::cont::ArrayHandle<svtkm::Id> xorder_Handle;
    Algorithm::Copy(counting_Handle, xorder_Handle);
    svtkm::cont::ArrayHandle<svtkm::Id> yorder_Handle;
    Algorithm::Copy(counting_Handle, yorder_Handle);
    svtkm::cont::ArrayHandle<svtkm::Id> zorder_Handle;
    Algorithm::Copy(counting_Handle, zorder_Handle);

    splitId_Handle.Allocate(nTrainingPoints);

    svtkm::cont::ArrayHandle<CoordType> xcoordi_Handle;
    svtkm::cont::ArrayHandle<CoordType> ycoordi_Handle;
    svtkm::cont::ArrayHandle<CoordType> zcoordi_Handle;

    SeprateVec3AryHandle sepVec3Worklet;
    svtkm::worklet::DispatcherMapField<SeprateVec3AryHandle> sepVec3Dispatcher(sepVec3Worklet);
    sepVec3Dispatcher.Invoke(coordi_Handle, xcoordi_Handle, ycoordi_Handle, zcoordi_Handle);

    Algorithm::SortByKey(xcoordi_Handle, xorder_Handle);
    svtkm::cont::ArrayHandle<svtkm::Id> xrank_Handle =
      ScatterArrayWrapper(pointId_Handle, xorder_Handle);

    Algorithm::SortByKey(ycoordi_Handle, yorder_Handle);
    svtkm::cont::ArrayHandle<svtkm::Id> yrank_Handle =
      ScatterArrayWrapper(pointId_Handle, yorder_Handle);

    Algorithm::SortByKey(zcoordi_Handle, zorder_Handle);
    svtkm::cont::ArrayHandle<svtkm::Id> zrank_Handle =
      ScatterArrayWrapper(pointId_Handle, zorder_Handle);

    svtkm::cont::ArrayHandle<svtkm::Id> segId_Handle;
    svtkm::cont::ArrayHandleConstant<svtkm::Id> constHandle(0, nTrainingPoints);
    Algorithm::Copy(constHandle, segId_Handle);

    ///// build kd tree /////
    svtkm::Int32 maxLevel = static_cast<svtkm::Int32>(ceil(svtkm::Log2(nTrainingPoints) + 1));
    for (svtkm::Int32 i = 0; i < maxLevel - 1; i++)
    {
      OneLevelSplit3D(
        pointId_Handle, xrank_Handle, yrank_Handle, zrank_Handle, segId_Handle, splitId_Handle, i);
    }
  }
};
}
}
} // namespace svtkm::worklet

#endif // svtk_m_worklet_KdTree3DConstruction_h
