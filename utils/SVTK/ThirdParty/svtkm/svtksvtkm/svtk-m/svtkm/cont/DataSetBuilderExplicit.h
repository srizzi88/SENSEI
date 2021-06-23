//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_DataSetBuilderExplicit_h
#define svtk_m_cont_DataSetBuilderExplicit_h

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayHandleCast.h>
#include <svtkm/cont/ArrayHandleCompositeVector.h>
#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/cont/CoordinateSystem.h>
#include <svtkm/cont/DataSet.h>

namespace svtkm
{
namespace cont
{

//Coordinates builder??

class SVTKM_CONT_EXPORT DataSetBuilderExplicit
{
public:
  SVTKM_CONT
  DataSetBuilderExplicit() {}

  //Single cell explicits.
  //TODO

  //Zoo explicit cell
  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(const std::vector<T>& xVals,
                                              const std::vector<svtkm::UInt8>& shapes,
                                              const std::vector<svtkm::IdComponent>& numIndices,
                                              const std::vector<svtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords")
  {
    std::vector<T> yVals(xVals.size(), 0), zVals(xVals.size(), 0);
    return DataSetBuilderExplicit::Create(
      xVals, yVals, zVals, shapes, numIndices, connectivity, coordsNm);
  }

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(const std::vector<T>& xVals,
                                              const std::vector<T>& yVals,
                                              const std::vector<svtkm::UInt8>& shapes,
                                              const std::vector<svtkm::IdComponent>& numIndices,
                                              const std::vector<svtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords")
  {
    std::vector<T> zVals(xVals.size(), 0);
    return DataSetBuilderExplicit::Create(
      xVals, yVals, zVals, shapes, numIndices, connectivity, coordsNm);
  }

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(const std::vector<T>& xVals,
                                              const std::vector<T>& yVals,
                                              const std::vector<T>& zVals,
                                              const std::vector<svtkm::UInt8>& shapes,
                                              const std::vector<svtkm::IdComponent>& numIndices,
                                              const std::vector<svtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords");

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(
    const svtkm::cont::ArrayHandle<T>& xVals,
    const svtkm::cont::ArrayHandle<T>& yVals,
    const svtkm::cont::ArrayHandle<T>& zVals,
    const svtkm::cont::ArrayHandle<svtkm::UInt8>& shapes,
    const svtkm::cont::ArrayHandle<svtkm::IdComponent>& numIndices,
    const svtkm::cont::ArrayHandle<svtkm::Id>& connectivity,
    const std::string& coordsNm = "coords")
  {
    auto offsets = svtkm::cont::ConvertNumIndicesToOffsets(numIndices);

    return DataSetBuilderExplicit::BuildDataSet(
      xVals, yVals, zVals, shapes, offsets, connectivity, coordsNm);
  }

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(const std::vector<svtkm::Vec<T, 3>>& coords,
                                              const std::vector<svtkm::UInt8>& shapes,
                                              const std::vector<svtkm::IdComponent>& numIndices,
                                              const std::vector<svtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords");

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet Create(
    const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>>& coords,
    const svtkm::cont::ArrayHandle<svtkm::UInt8>& shapes,
    const svtkm::cont::ArrayHandle<svtkm::IdComponent>& numIndices,
    const svtkm::cont::ArrayHandle<svtkm::Id>& connectivity,
    const std::string& coordsNm = "coords")
  {
    auto offsets = svtkm::cont::ConvertNumIndicesToOffsets(numIndices);
    return DataSetBuilderExplicit::BuildDataSet(coords, shapes, offsets, connectivity, coordsNm);
  }

  template <typename T, typename CellShapeTag>
  SVTKM_CONT static svtkm::cont::DataSet Create(const std::vector<svtkm::Vec<T, 3>>& coords,
                                              CellShapeTag tag,
                                              svtkm::IdComponent numberOfPointsPerCell,
                                              const std::vector<svtkm::Id>& connectivity,
                                              const std::string& coordsNm = "coords");

  template <typename T, typename CellShapeTag>
  SVTKM_CONT static svtkm::cont::DataSet Create(
    const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>>& coords,
    CellShapeTag tag,
    svtkm::IdComponent numberOfPointsPerCell,
    const svtkm::cont::ArrayHandle<svtkm::Id>& connectivity,
    const std::string& coordsNm = "coords")
  {
    return DataSetBuilderExplicit::BuildDataSet(
      coords, tag, numberOfPointsPerCell, connectivity, coordsNm);
  }

private:
  template <typename T>
  static svtkm::cont::DataSet BuildDataSet(const svtkm::cont::ArrayHandle<T>& X,
                                          const svtkm::cont::ArrayHandle<T>& Y,
                                          const svtkm::cont::ArrayHandle<T>& Z,
                                          const svtkm::cont::ArrayHandle<svtkm::UInt8>& shapes,
                                          const svtkm::cont::ArrayHandle<svtkm::Id>& offsets,
                                          const svtkm::cont::ArrayHandle<svtkm::Id>& connectivity,
                                          const std::string& coordsNm);

  template <typename T>
  SVTKM_CONT static svtkm::cont::DataSet BuildDataSet(
    const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>>& coords,
    const svtkm::cont::ArrayHandle<svtkm::UInt8>& shapes,
    const svtkm::cont::ArrayHandle<svtkm::Id>& offsets,
    const svtkm::cont::ArrayHandle<svtkm::Id>& connectivity,
    const std::string& coordsNm);

  template <typename T, typename CellShapeTag>
  SVTKM_CONT static svtkm::cont::DataSet BuildDataSet(
    const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>>& coords,
    CellShapeTag tag,
    svtkm::IdComponent numberOfPointsPerCell,
    const svtkm::cont::ArrayHandle<svtkm::Id>& connectivity,
    const std::string& coordsNm);
};

template <typename T>
inline SVTKM_CONT svtkm::cont::DataSet DataSetBuilderExplicit::Create(
  const std::vector<T>& xVals,
  const std::vector<T>& yVals,
  const std::vector<T>& zVals,
  const std::vector<svtkm::UInt8>& shapes,
  const std::vector<svtkm::IdComponent>& numIndices,
  const std::vector<svtkm::Id>& connectivity,
  const std::string& coordsNm)
{
  SVTKM_ASSERT(xVals.size() == yVals.size() && yVals.size() == zVals.size() && xVals.size() > 0);

  auto xArray = svtkm::cont::make_ArrayHandle(xVals, svtkm::CopyFlag::On);
  auto yArray = svtkm::cont::make_ArrayHandle(yVals, svtkm::CopyFlag::On);
  auto zArray = svtkm::cont::make_ArrayHandle(zVals, svtkm::CopyFlag::On);

  auto shapesArray = svtkm::cont::make_ArrayHandle(shapes, svtkm::CopyFlag::On);
  auto connArray = svtkm::cont::make_ArrayHandle(connectivity, svtkm::CopyFlag::On);

  auto offsetsArray = svtkm::cont::ConvertNumIndicesToOffsets(
    svtkm::cont::make_ArrayHandle(numIndices, svtkm::CopyFlag::Off));

  return DataSetBuilderExplicit::BuildDataSet(
    xArray, yArray, zArray, shapesArray, offsetsArray, connArray, coordsNm);
}

template <typename T>
inline SVTKM_CONT svtkm::cont::DataSet DataSetBuilderExplicit::BuildDataSet(
  const svtkm::cont::ArrayHandle<T>& X,
  const svtkm::cont::ArrayHandle<T>& Y,
  const svtkm::cont::ArrayHandle<T>& Z,
  const svtkm::cont::ArrayHandle<svtkm::UInt8>& shapes,
  const svtkm::cont::ArrayHandle<svtkm::Id>& offsets,
  const svtkm::cont::ArrayHandle<svtkm::Id>& connectivity,
  const std::string& coordsNm)
{
  SVTKM_ASSERT(X.GetNumberOfValues() == Y.GetNumberOfValues() &&
              Y.GetNumberOfValues() == Z.GetNumberOfValues() && X.GetNumberOfValues() > 0 &&
              shapes.GetNumberOfValues() + 1 == offsets.GetNumberOfValues());

  svtkm::cont::DataSet dataSet;
  dataSet.AddCoordinateSystem(
    svtkm::cont::CoordinateSystem(coordsNm, make_ArrayHandleCompositeVector(X, Y, Z)));
  svtkm::Id nPts = X.GetNumberOfValues();
  svtkm::cont::CellSetExplicit<> cellSet;

  cellSet.Fill(nPts, shapes, connectivity, offsets);
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

template <typename T>
inline SVTKM_CONT svtkm::cont::DataSet DataSetBuilderExplicit::Create(
  const std::vector<svtkm::Vec<T, 3>>& coords,
  const std::vector<svtkm::UInt8>& shapes,
  const std::vector<svtkm::IdComponent>& numIndices,
  const std::vector<svtkm::Id>& connectivity,
  const std::string& coordsNm)
{
  auto coordsArray = svtkm::cont::make_ArrayHandle(coords, svtkm::CopyFlag::On);
  auto shapesArray = svtkm::cont::make_ArrayHandle(shapes, svtkm::CopyFlag::On);
  auto connArray = svtkm::cont::make_ArrayHandle(connectivity, svtkm::CopyFlag::On);

  auto offsetsArray = svtkm::cont::ConvertNumIndicesToOffsets(
    svtkm::cont::make_ArrayHandle(numIndices, svtkm::CopyFlag::Off));

  return DataSetBuilderExplicit::BuildDataSet(
    coordsArray, shapesArray, offsetsArray, connArray, coordsNm);
}

template <typename T>
inline SVTKM_CONT svtkm::cont::DataSet DataSetBuilderExplicit::BuildDataSet(
  const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>>& coords,
  const svtkm::cont::ArrayHandle<svtkm::UInt8>& shapes,
  const svtkm::cont::ArrayHandle<svtkm::Id>& offsets,
  const svtkm::cont::ArrayHandle<svtkm::Id>& connectivity,
  const std::string& coordsNm)
{
  svtkm::cont::DataSet dataSet;

  dataSet.AddCoordinateSystem(svtkm::cont::CoordinateSystem(coordsNm, coords));
  svtkm::Id nPts = static_cast<svtkm::Id>(coords.GetNumberOfValues());
  svtkm::cont::CellSetExplicit<> cellSet;

  cellSet.Fill(nPts, shapes, connectivity, offsets);
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

template <typename T, typename CellShapeTag>
inline SVTKM_CONT svtkm::cont::DataSet DataSetBuilderExplicit::Create(
  const std::vector<svtkm::Vec<T, 3>>& coords,
  CellShapeTag tag,
  svtkm::IdComponent numberOfPointsPerCell,
  const std::vector<svtkm::Id>& connectivity,
  const std::string& coordsNm)
{
  auto coordsArray = svtkm::cont::make_ArrayHandle(coords, svtkm::CopyFlag::On);
  auto connArray = svtkm::cont::make_ArrayHandle(connectivity, svtkm::CopyFlag::On);

  return DataSetBuilderExplicit::Create(
    coordsArray, tag, numberOfPointsPerCell, connArray, coordsNm);
}

template <typename T, typename CellShapeTag>
inline SVTKM_CONT svtkm::cont::DataSet DataSetBuilderExplicit::BuildDataSet(
  const svtkm::cont::ArrayHandle<svtkm::Vec<T, 3>>& coords,
  CellShapeTag tag,
  svtkm::IdComponent numberOfPointsPerCell,
  const svtkm::cont::ArrayHandle<svtkm::Id>& connectivity,
  const std::string& coordsNm)
{
  (void)tag; //C4100 false positive workaround
  svtkm::cont::DataSet dataSet;

  dataSet.AddCoordinateSystem(svtkm::cont::CoordinateSystem(coordsNm, coords));
  svtkm::cont::CellSetSingleType<> cellSet;

  cellSet.Fill(coords.GetNumberOfValues(), tag.Id, numberOfPointsPerCell, connectivity);
  dataSet.SetCellSet(cellSet);

  return dataSet;
}

class SVTKM_CONT_EXPORT DataSetBuilderExplicitIterative
{
public:
  SVTKM_CONT
  DataSetBuilderExplicitIterative();

  SVTKM_CONT
  void Begin(const std::string& coordName = "coords");

  //Define points.
  SVTKM_CONT
  svtkm::cont::DataSet Create();

  SVTKM_CONT
  svtkm::Id AddPoint(const svtkm::Vec3f& pt);

  SVTKM_CONT
  svtkm::Id AddPoint(const svtkm::FloatDefault& x,
                    const svtkm::FloatDefault& y,
                    const svtkm::FloatDefault& z = 0);

  template <typename T>
  SVTKM_CONT svtkm::Id AddPoint(const T& x, const T& y, const T& z = 0)
  {
    return AddPoint(static_cast<svtkm::FloatDefault>(x),
                    static_cast<svtkm::FloatDefault>(y),
                    static_cast<svtkm::FloatDefault>(z));
  }

  template <typename T>
  SVTKM_CONT svtkm::Id AddPoint(const svtkm::Vec<T, 3>& pt)
  {
    return AddPoint(static_cast<svtkm::Vec3f>(pt));
  }

  //Define cells.
  SVTKM_CONT
  void AddCell(svtkm::UInt8 shape);

  SVTKM_CONT
  void AddCell(const svtkm::UInt8& shape, const std::vector<svtkm::Id>& conn);

  SVTKM_CONT
  void AddCell(const svtkm::UInt8& shape, const svtkm::Id* conn, const svtkm::IdComponent& n);

  SVTKM_CONT
  void AddCellPoint(svtkm::Id pointIndex);

private:
  std::string coordNm;

  std::vector<svtkm::Vec3f> points;
  std::vector<svtkm::UInt8> shapes;
  std::vector<svtkm::IdComponent> numIdx;
  std::vector<svtkm::Id> connectivity;
};
}
}

#endif //svtk_m_cont_DataSetBuilderExplicit_h
