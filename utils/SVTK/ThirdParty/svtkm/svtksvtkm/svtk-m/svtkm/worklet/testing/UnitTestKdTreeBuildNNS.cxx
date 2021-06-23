//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <random>
#include <svtkm/cont/Algorithm.h>
#include <svtkm/worklet/KdTree3D.h>

namespace
{

using Algorithm = svtkm::cont::Algorithm;

////brute force method /////
template <typename CoordiVecT, typename CoordiPortalT, typename CoordiT>
SVTKM_EXEC_CONT svtkm::Id NNSVerify3D(CoordiVecT qc, CoordiPortalT coordiPortal, CoordiT& dis)
{
  dis = std::numeric_limits<CoordiT>::max();
  svtkm::Id nnpIdx = 0;

  for (svtkm::Int32 i = 0; i < coordiPortal.GetNumberOfValues(); i++)
  {
    CoordiT splitX = coordiPortal.Get(i)[0];
    CoordiT splitY = coordiPortal.Get(i)[1];
    CoordiT splitZ = coordiPortal.Get(i)[2];
    CoordiT _dis =
      svtkm::Sqrt((splitX - qc[0]) * (splitX - qc[0]) + (splitY - qc[1]) * (splitY - qc[1]) +
                 (splitZ - qc[2]) * (splitZ - qc[2]));
    if (_dis < dis)
    {
      dis = _dis;
      nnpIdx = i;
    }
  }
  return nnpIdx;
}

class NearestNeighborSearchBruteForce3DWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn qcIn,
                                WholeArrayIn treeCoordiIn,
                                FieldOut nnIdOut,
                                FieldOut nnDisOut);
  using ExecutionSignature = void(_1, _2, _3, _4);

  SVTKM_CONT
  NearestNeighborSearchBruteForce3DWorklet() {}

  template <typename CoordiVecType, typename CoordiPortalType, typename IdType, typename CoordiType>
  SVTKM_EXEC void operator()(const CoordiVecType& qc,
                            const CoordiPortalType& coordiPortal,
                            IdType& nnId,
                            CoordiType& nnDis) const
  {
    nnDis = std::numeric_limits<CoordiType>::max();

    nnId = NNSVerify3D(qc, coordiPortal, nnDis);
  }
};

void TestKdTreeBuildNNS(svtkm::cont::DeviceAdapterId deviceId)
{
  svtkm::Int32 nTrainingPoints = 1000;
  svtkm::Int32 nTestingPoint = 1000;

  std::vector<svtkm::Vec3f_32> coordi;

  ///// randomly generate training points/////
  std::default_random_engine dre;
  std::uniform_real_distribution<svtkm::Float32> dr(0.0f, 10.0f);

  for (svtkm::Int32 i = 0; i < nTrainingPoints; i++)
  {
    coordi.push_back(svtkm::make_Vec(dr(dre), dr(dre), dr(dre)));
  }

  ///// preprare data to build 3D kd tree /////
  auto coordi_Handle = svtkm::cont::make_ArrayHandle(coordi);

  // Run data
  svtkm::worklet::KdTree3D kdtree3d;
  kdtree3d.Build(coordi_Handle);

  //Nearest Neighbor worklet Testing
  /// randomly generate testing points /////
  std::vector<svtkm::Vec3f_32> qcVec;
  for (svtkm::Int32 i = 0; i < nTestingPoint; i++)
  {
    qcVec.push_back(svtkm::make_Vec(dr(dre), dr(dre), dr(dre)));
  }

  ///// preprare testing data /////
  auto qc_Handle = svtkm::cont::make_ArrayHandle(qcVec);

  svtkm::cont::ArrayHandle<svtkm::Id> nnId_Handle;
  svtkm::cont::ArrayHandle<svtkm::Float32> nnDis_Handle;

  kdtree3d.Run(coordi_Handle, qc_Handle, nnId_Handle, nnDis_Handle, deviceId);

  svtkm::cont::ArrayHandle<svtkm::Id> bfnnId_Handle;
  svtkm::cont::ArrayHandle<svtkm::Float32> bfnnDis_Handle;
  NearestNeighborSearchBruteForce3DWorklet nnsbf3dWorklet;
  svtkm::worklet::DispatcherMapField<NearestNeighborSearchBruteForce3DWorklet> nnsbf3DDispatcher(
    nnsbf3dWorklet);
  nnsbf3DDispatcher.Invoke(
    qc_Handle, svtkm::cont::make_ArrayHandle(coordi), bfnnId_Handle, bfnnDis_Handle);

  ///// verfity search result /////
  bool passTest = true;
  for (svtkm::Int32 i = 0; i < nTestingPoint; i++)
  {
    svtkm::Id workletIdx = nnId_Handle.GetPortalControl().Get(i);
    svtkm::Id bfworkletIdx = bfnnId_Handle.GetPortalControl().Get(i);

    if (workletIdx != bfworkletIdx)
    {
      passTest = false;
    }
  }

  SVTKM_TEST_ASSERT(passTest, "Kd tree NN search result incorrect.");
}

} // anonymous namespace

int UnitTestKdTreeBuildNNS(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::RunOnDevice(TestKdTreeBuildNNS, argc, argv);
}
