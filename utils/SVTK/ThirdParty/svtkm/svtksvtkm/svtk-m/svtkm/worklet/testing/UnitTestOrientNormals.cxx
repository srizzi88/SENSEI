//=============================================================================
//
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//
//  Copyright 2019 National Technology & Engineering Solutions of Sandia, LLC (NTESS).
//  Copyright 2019 UT-Battelle, LLC.
//  Copyright 2019 Los Alamos National Security.
//
//  Under the terms of Contract DE-NA0003525 with NTESS,
//  the U.S. Government retains certain rights in this software.
//  Under the terms of Contract DE-AC52-06NA25396 with Los Alamos National
//  Laboratory (LANL), the U.S. Government retains certain rights in
//  this software.
//
//=============================================================================

#include <svtkm/worklet/OrientNormals.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleBitField.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleVirtual.h>
#include <svtkm/cont/BitField.h>
#include <svtkm/cont/CellSet.h>
#include <svtkm/cont/CellSetSingleType.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>

#include <svtkm/filter/Contour.h>
#include <svtkm/filter/PolicyBase.h>
#include <svtkm/filter/SurfaceNormals.h>

#include <svtkm/source/Wavelet.h>

#include <svtkm/cont/serial/DeviceAdapterSerial.h>

#include <svtkm/cont/testing/Testing.h>

#include <svtkm/cont/RuntimeDeviceTracker.h>

namespace
{

struct TestPolicy : public svtkm::filter::PolicyBase<TestPolicy>
{
  using StructuredCellSetList = svtkm::List<svtkm::cont::CellSetStructured<3>>;
  using UnstructuredCellSetList = svtkm::List<svtkm::cont::CellSetSingleType<>>;
  using AllCellSetList = svtkm::ListAppend<StructuredCellSetList, UnstructuredCellSetList>;
  using FieldTypeList = svtkm::List<svtkm::FloatDefault, svtkm::Vec<svtkm::FloatDefault, 3>>;
};

SVTKM_CONT
svtkm::cont::DataSet CreateDataSet(bool pointNormals, bool cellNormals)
{
  svtkm::source::Wavelet wavelet({ -25 }, { 25 });
  wavelet.SetFrequency({ 20, 15, 25 });
  wavelet.SetMagnitude({ 5 });
  auto dataSet = wavelet.Execute();

  // Cut a contour
  svtkm::filter::Contour contour;
  contour.SetActiveField("scalars", svtkm::cont::Field::Association::POINTS);
  contour.SetNumberOfIsoValues(1);
  contour.SetIsoValue(192);
  contour.SetMergeDuplicatePoints(true);
  contour.SetGenerateNormals(false);
  dataSet = contour.Execute(dataSet);

  svtkm::filter::SurfaceNormals normals;
  normals.SetGeneratePointNormals(pointNormals);
  normals.SetGenerateCellNormals(cellNormals);
  normals.SetPointNormalsName("normals");
  normals.SetCellNormalsName("normals");
  normals.SetAutoOrientNormals(false);
  dataSet = normals.Execute(dataSet, TestPolicy{});

  return dataSet;
}

struct ValidateNormals
{
  using CellSetType = svtkm::cont::CellSetSingleType<>;
  using NormalType = svtkm::Vec<svtkm::FloatDefault, 3>;
  using NormalsArrayType = svtkm::cont::ArrayHandleVirtual<NormalType>;
  using NormalsPortalType = decltype(std::declval<NormalsArrayType>().GetPortalConstControl());
  using ConnType =
    decltype(std::declval<CellSetType>().PrepareForInput(svtkm::cont::DeviceAdapterTagSerial{},
                                                         svtkm::TopologyElementTagCell{},
                                                         svtkm::TopologyElementTagPoint{}));
  using RConnType =
    decltype(std::declval<CellSetType>().PrepareForInput(svtkm::cont::DeviceAdapterTagSerial{},
                                                         svtkm::TopologyElementTagPoint{},
                                                         svtkm::TopologyElementTagCell{}));
  using PointsType =
    decltype(std::declval<svtkm::cont::CoordinateSystem>().GetData().GetPortalConstControl());

  svtkm::cont::CoordinateSystem Coords;
  CellSetType Cells;

  PointsType Points;
  ConnType Conn;
  RConnType RConn;

  NormalsArrayType PointNormalsArray;
  NormalsPortalType PointNormals;
  NormalsArrayType CellNormalsArray;
  NormalsPortalType CellNormals;

  svtkm::cont::BitField VisitedCellsField;
  svtkm::cont::BitField VisitedPointsField;
  svtkm::cont::BitField::PortalControl VisitedCells;
  svtkm::cont::BitField::PortalControl VisitedPoints;

  bool CheckPoints;
  bool CheckCells;

  SVTKM_CONT
  static void Run(svtkm::cont::DataSet& dataset,
                  bool checkPoints,
                  bool checkCells,
                  const std::string& normalsName = "normals")
  {
    // Temporarily enable the serial device for workaround in ValidateNormals,
    // which requires the serial device. This can be refactored once #377 is
    // fixed.
    svtkm::cont::ScopedRuntimeDeviceTracker tracker(svtkm::cont::DeviceAdapterTagSerial{},
                                                   svtkm::cont::RuntimeDeviceTrackerMode::Enable);

    svtkm::cont::Field pointNormals;
    svtkm::cont::Field cellNormals;

    if (checkPoints)
    {
      pointNormals = dataset.GetPointField(normalsName);
    }
    if (checkCells)
    {
      cellNormals = dataset.GetCellField(normalsName);
    }

    ValidateNormals obj{ dataset, checkPoints, checkCells, pointNormals, cellNormals };
    obj.Validate();
  }

  SVTKM_CONT
  ValidateNormals(const svtkm::cont::DataSet& dataset,
                  bool checkPoints,
                  bool checkCells,
                  const svtkm::cont::Field& pointNormalsField,
                  const svtkm::cont::Field& cellNormalsField)
    : Coords{ dataset.GetCoordinateSystem() }
    , Cells{ dataset.GetCellSet().Cast<CellSetType>() }
    , Points{ this->Coords.GetData().GetPortalConstControl() }
    , CheckPoints(checkPoints)
    , CheckCells(checkCells)
  {
    // FIXME This would be much simplier if we had a GetPointCells() method on
    // cell sets.... #377 will simplify this.
    // Build the connectivity table on any device, then get a portal for serial
    // so we can do lookups on the CPU.
    this->Cells.GetConnectivityArray(svtkm::TopologyElementTagCell{},
                                     svtkm::TopologyElementTagPoint{});
    this->Cells.GetConnectivityArray(svtkm::TopologyElementTagCell{},
                                     svtkm::TopologyElementTagPoint{});
    this->Conn = this->Cells.PrepareForInput(svtkm::cont::DeviceAdapterTagSerial{},
                                             svtkm::TopologyElementTagCell{},
                                             svtkm::TopologyElementTagPoint{});
    this->RConn = this->Cells.PrepareForInput(svtkm::cont::DeviceAdapterTagSerial{},
                                              svtkm::TopologyElementTagPoint{},
                                              svtkm::TopologyElementTagCell{});

    if (this->CheckPoints)
    {
      this->PointNormalsArray = pointNormalsField.GetData().AsVirtual<NormalType>();
      this->PointNormals = this->PointNormalsArray.GetPortalConstControl();
    }
    if (this->CheckCells)
    {
      this->CellNormalsArray = cellNormalsField.GetData().AsVirtual<NormalType>();
      this->CellNormals = this->CellNormalsArray.GetPortalConstControl();
    }
  }

  SVTKM_CONT
  void Validate()
  {
    // Locate a point with the minimum x coordinate:
    const svtkm::Id startPoint = [&]() -> svtkm::Id {
      const svtkm::Float64 xMin = this->Coords.GetBounds().X.Min;
      const auto points = this->Coords.GetData().GetPortalConstControl();
      const svtkm::Id numPoints = points.GetNumberOfValues();
      svtkm::Id resultIdx = -1;
      for (svtkm::Id pointIdx = 0; pointIdx < numPoints; ++pointIdx)
      {
        const auto point = points.Get(pointIdx);
        if (static_cast<double>(point[0]) <= xMin)
        {
          resultIdx = pointIdx;
          break;
        }
      }
      if (resultIdx < 0)
      {
        throw svtkm::cont::ErrorBadValue("Minimum point not found!");
      }

      return resultIdx;
    }();

    // Start recursive validation.
    this->Prepare();
    this->ValidateImpl(startPoint, NormalType{ -1, 0, 0 });

    svtkm::Id numPoints = this->Points.GetNumberOfValues();
    svtkm::Id numCells = this->Cells.GetNumberOfCells();
    svtkm::Id numVisitedPoints = svtkm::cont::Algorithm::CountSetBits(this->VisitedPointsField);
    svtkm::Id numVisitedCells = svtkm::cont::Algorithm::CountSetBits(this->VisitedCellsField);
    if (numPoints != numVisitedPoints)
    {
      throw svtkm::cont::ErrorBadValue("Unvisited point!");
    }
    if (numCells != numVisitedCells)
    {
      throw svtkm::cont::ErrorBadValue("Unvisited cell!");
    }
  }

private:
  static bool SameHemisphere(const NormalType& a, const NormalType& b)
  {
    return svtkm::Dot(a, b) >= 0;
  }

  void Prepare()
  {
    svtkm::cont::Algorithm::Fill(this->VisitedPointsField, false, this->Coords.GetNumberOfPoints());
    svtkm::cont::Algorithm::Fill(this->VisitedCellsField, false, this->Cells.GetNumberOfCells());

    this->VisitedPoints = this->VisitedPointsField.GetPortalControl();
    this->VisitedCells = this->VisitedCellsField.GetPortalControl();
  }

  void ValidateImpl(svtkm::Id startPtIdx, const NormalType& startRefNormal)
  {
    using Entry = svtkm::Pair<svtkm::Id, NormalType>;
    std::vector<Entry> queue;
    queue.emplace_back(startPtIdx, startRefNormal);
    this->VisitedPoints.SetBit(startPtIdx, true);

    while (!queue.empty())
    {
      const svtkm::Id curPtIdx = queue.back().first;
      NormalType refNormal = queue.back().second;
      queue.pop_back();

      if (this->CheckPoints)
      {
        const NormalType curNormal = this->PointNormals.Get(curPtIdx);
        if (!this->SameHemisphere(curNormal, refNormal))
        {
          const auto coord = this->Points.Get(curPtIdx);
          std::cerr << "BadPointNormal PtId: " << curPtIdx << "\n\t"
                    << "- Normal: {" << curNormal[0] << ", " << curNormal[1] << ", " << curNormal[2]
                    << "}\n\t"
                    << "- Reference: {" << refNormal[0] << ", " << refNormal[1] << ", "
                    << refNormal[2] << "}\n\t"
                    << "- Coord: {" << coord[0] << ", " << coord[1] << ", " << coord[2] << "}\n";
          throw svtkm::cont::ErrorBadValue("Bad point normal found!");
        }
        refNormal = curNormal;
      }

      // Lookup and visit neighbor cells:
      const auto neighborCells = this->RConn.GetIndices(curPtIdx);
      const auto numNeighborCells = neighborCells.GetNumberOfComponents();
      for (svtkm::IdComponent nCellIdx = 0; nCellIdx < numNeighborCells; ++nCellIdx)
      {
        const svtkm::Id curCellIdx = neighborCells[nCellIdx];

        // Skip this cell if already visited:
        if (this->VisitedCells.GetBit(curCellIdx))
        {
          continue;
        }
        this->VisitedCells.SetBit(curCellIdx, true);

        if (this->CheckCells)
        {
          const NormalType curNormal = this->CellNormals.Get(curCellIdx);
          if (!this->SameHemisphere(curNormal, refNormal))
          {
            std::cerr << "BadCellNormal: CellId: " << curCellIdx << "\n\t"
                      << "- Normal: {" << curNormal[0] << ", " << curNormal[1] << ", "
                      << curNormal[2] << "}\n\t"
                      << "- Reference: {" << refNormal[0] << ", " << refNormal[1] << ", "
                      << refNormal[2] << "}\n";
            throw svtkm::cont::ErrorBadValue("Bad cell normal found!");
          }
          refNormal = curNormal;
        }

        // Lookup and visit points in this cell:
        const auto neighborPoints = this->Conn.GetIndices(curCellIdx);
        const auto numNeighborPoints = neighborPoints.GetNumberOfComponents();
        for (svtkm::IdComponent nPtIdx = 0; nPtIdx < numNeighborPoints; ++nPtIdx)
        {
          const svtkm::Id nextPtIdx = neighborPoints[nPtIdx];

          // Skip if already visited:
          if (this->VisitedPoints.GetBit(nextPtIdx))
          {
            continue;
          }

          // Otherwise, queue next point using current normal as reference:
          queue.emplace_back(nextPtIdx, refNormal);
          this->VisitedPoints.SetBit(nextPtIdx, true);
        }
      }
    }
  }
};

SVTKM_CONT
void TestOrientNormals(bool testPoints, bool testCells)
{
  using NormalArrayT = svtkm::cont::ArrayHandle<svtkm::Vec<svtkm::FloatDefault, 3>>;

  auto dataset = CreateDataSet(testPoints, testCells);

  // Check that the input actually has bad normals:
  const bool inputValid = [&]() -> bool {
    try
    {
      std::cerr << "Expecting to throw a BadNormal exception...\n";
      ValidateNormals::Run(dataset, testPoints, testCells);
      return true; // Dataset is already oriented
    }
    catch (svtkm::cont::ErrorBadValue&)
    {
      std::cerr << "Expected exception caught! All is well. "
                   "Future exceptions are errors.\n";
      return false; // Dataset is unoriented
    }
  }();

  if (inputValid)
  {
    throw svtkm::cont::ErrorBadValue("Error: Input doesn't have bad normals.");
  }

  // modify normals in place
  const auto coords = dataset.GetCoordinateSystem().GetData();
  const auto cells = dataset.GetCellSet();
  if (testPoints && testCells)
  {
    std::cerr << "Testing point and cell normals...\n";

    const auto pointNormalField = dataset.GetPointField("normals");
    const auto cellNormalField = dataset.GetCellField("normals");
    auto pointNormals = pointNormalField.GetData().Cast<NormalArrayT>();
    auto cellNormals = cellNormalField.GetData().Cast<NormalArrayT>();

    svtkm::worklet::OrientNormals::RunPointAndCellNormals(cells, coords, pointNormals, cellNormals);
  }
  else if (testPoints)
  {
    std::cerr << "Testing point normals...\n";
    const auto pointNormalField = dataset.GetPointField("normals");
    auto pointNormals = pointNormalField.GetData().Cast<NormalArrayT>();

    svtkm::worklet::OrientNormals::RunPointNormals(cells, coords, pointNormals);
  }
  else if (testCells)
  {
    std::cerr << "Testing cell normals...\n";
    const auto cellNormalField = dataset.GetCellField("normals");
    auto cellNormals = cellNormalField.GetData().Cast<NormalArrayT>();

    svtkm::worklet::OrientNormals::RunCellNormals(cells, coords, cellNormals);
  }
  else
  {
    throw "Nothing tested...";
  }

  ValidateNormals::Run(dataset, testPoints, testCells);
}

void DoTest()
{
  TestOrientNormals(true, false);
  TestOrientNormals(false, true);
  TestOrientNormals(true, true);
}

} // end anon namespace

int UnitTestOrientNormals(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(DoTest, argc, argv);
}
