//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/exec/CellEdge.h>
#include <svtkm/exec/CellFace.h>

#include <svtkm/CellShape.h>
#include <svtkm/CellTraits.h>

#include <svtkm/exec/FunctorBase.h>

#include <svtkm/testing/Testing.h>

#include <set>
#include <vector>

namespace
{

using EdgeType = svtkm::IdComponent2;

void MakeEdgeCanonical(EdgeType& edge)
{
  if (edge[1] < edge[0])
  {
    std::swap(edge[0], edge[1]);
  }
}

struct TestCellFacesFunctor
{
  template <typename CellShapeTag>
  void DoTest(svtkm::IdComponent numPoints,
              CellShapeTag shape,
              svtkm::CellTopologicalDimensionsTag<3>) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    svtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    svtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    std::vector<svtkm::Id> pointIndexProxyBuffer(static_cast<std::size_t>(numPoints));
    for (std::size_t index = 0; index < pointIndexProxyBuffer.size(); ++index)
    {
      pointIndexProxyBuffer[index] = static_cast<svtkm::Id>(1000000 - index);
    }
    svtkm::VecCConst<svtkm::Id> pointIndexProxy(&pointIndexProxyBuffer.at(0), numPoints);

    svtkm::IdComponent numEdges = svtkm::exec::CellEdgeNumberOfEdges(numPoints, shape, workletProxy);
    SVTKM_TEST_ASSERT(numEdges > 0, "No edges?");

    std::set<EdgeType> edgeSet;
    for (svtkm::IdComponent edgeIndex = 0; edgeIndex < numEdges; edgeIndex++)
    {
      EdgeType edge(svtkm::exec::CellEdgeLocalIndex(numPoints, 0, edgeIndex, shape, workletProxy),
                    svtkm::exec::CellEdgeLocalIndex(numPoints, 1, edgeIndex, shape, workletProxy));
      SVTKM_TEST_ASSERT(edge[0] >= 0, "Bad index in edge.");
      SVTKM_TEST_ASSERT(edge[0] < numPoints, "Bad index in edge.");
      SVTKM_TEST_ASSERT(edge[1] >= 0, "Bad index in edge.");
      SVTKM_TEST_ASSERT(edge[1] < numPoints, "Bad index in edge.");
      SVTKM_TEST_ASSERT(edge[0] != edge[1], "Degenerate edge.");
      MakeEdgeCanonical(edge);
      SVTKM_TEST_ASSERT(edge[0] < edge[1], "Internal test error: MakeEdgeCanonical failed");
      SVTKM_TEST_ASSERT(edgeSet.find(edge) == edgeSet.end(), "Found duplicate edge");
      edgeSet.insert(edge);

      svtkm::Id2 canonicalEdgeId =
        svtkm::exec::CellEdgeCanonicalId(numPoints, edgeIndex, shape, pointIndexProxy, workletProxy);
      SVTKM_TEST_ASSERT(canonicalEdgeId[0] > 0, "Not using global ids?");
      SVTKM_TEST_ASSERT(canonicalEdgeId[0] < canonicalEdgeId[1], "Bad order.");
    }

    svtkm::IdComponent numFaces = svtkm::exec::CellFaceNumberOfFaces(shape, workletProxy);
    SVTKM_TEST_ASSERT(numFaces > 0, "No faces?");

    std::set<EdgeType> edgesFoundInFaces;
    for (svtkm::IdComponent faceIndex = 0; faceIndex < numFaces; faceIndex++)
    {
      const svtkm::IdComponent numPointsInFace =
        svtkm::exec::CellFaceNumberOfPoints(faceIndex, shape, workletProxy);

      SVTKM_TEST_ASSERT(numPointsInFace >= 3, "Face has fewer points than a triangle.");

      for (svtkm::IdComponent pointIndex = 0; pointIndex < numPointsInFace; pointIndex++)
      {
        svtkm::IdComponent localFaceIndex =
          svtkm::exec::CellFaceLocalIndex(pointIndex, faceIndex, shape, workletProxy);
        SVTKM_TEST_ASSERT(localFaceIndex >= 0, "Invalid point index for face.");
        SVTKM_TEST_ASSERT(localFaceIndex < numPoints, "Invalid point index for face.");
        EdgeType edge;
        if (pointIndex < numPointsInFace - 1)
        {
          edge = EdgeType(
            svtkm::exec::CellFaceLocalIndex(pointIndex, faceIndex, shape, workletProxy),
            svtkm::exec::CellFaceLocalIndex(pointIndex + 1, faceIndex, shape, workletProxy));
        }
        else
        {
          edge =
            EdgeType(svtkm::exec::CellFaceLocalIndex(0, faceIndex, shape, workletProxy),
                     svtkm::exec::CellFaceLocalIndex(pointIndex, faceIndex, shape, workletProxy));
        }
        MakeEdgeCanonical(edge);
        SVTKM_TEST_ASSERT(edgeSet.find(edge) != edgeSet.end(), "Edge in face not in cell's edges");
        edgesFoundInFaces.insert(edge);
      }

      svtkm::Id3 canonicalFaceId =
        svtkm::exec::CellFaceCanonicalId(faceIndex, shape, pointIndexProxy, workletProxy);
      SVTKM_TEST_ASSERT(canonicalFaceId[0] > 0, "Not using global ids?");
      SVTKM_TEST_ASSERT(canonicalFaceId[0] < canonicalFaceId[1], "Bad order.");
      SVTKM_TEST_ASSERT(canonicalFaceId[1] < canonicalFaceId[2], "Bad order.");
    }
    SVTKM_TEST_ASSERT(edgesFoundInFaces.size() == edgeSet.size(),
                     "Faces did not contain all edges in cell");
  }

  // Case of cells that have 2 dimensions (no faces)
  template <typename CellShapeTag>
  void DoTest(svtkm::IdComponent numPoints,
              CellShapeTag shape,
              svtkm::CellTopologicalDimensionsTag<2>) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    svtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    svtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    std::vector<svtkm::Id> pointIndexProxyBuffer(static_cast<std::size_t>(numPoints));
    for (std::size_t index = 0; index < pointIndexProxyBuffer.size(); ++index)
    {
      pointIndexProxyBuffer[index] = static_cast<svtkm::Id>(1000000 - index);
    }
    svtkm::VecCConst<svtkm::Id> pointIndexProxy(&pointIndexProxyBuffer.at(0), numPoints);

    svtkm::IdComponent numEdges = svtkm::exec::CellEdgeNumberOfEdges(numPoints, shape, workletProxy);
    SVTKM_TEST_ASSERT(numEdges == numPoints, "Polygons should have same number of points and edges");

    std::set<EdgeType> edgeSet;
    for (svtkm::IdComponent edgeIndex = 0; edgeIndex < numEdges; edgeIndex++)
    {
      EdgeType edge(svtkm::exec::CellEdgeLocalIndex(numPoints, 0, edgeIndex, shape, workletProxy),
                    svtkm::exec::CellEdgeLocalIndex(numPoints, 1, edgeIndex, shape, workletProxy));
      SVTKM_TEST_ASSERT(edge[0] >= 0, "Bad index in edge.");
      SVTKM_TEST_ASSERT(edge[0] < numPoints, "Bad index in edge.");
      SVTKM_TEST_ASSERT(edge[1] >= 0, "Bad index in edge.");
      SVTKM_TEST_ASSERT(edge[1] < numPoints, "Bad index in edge.");
      SVTKM_TEST_ASSERT(edge[0] != edge[1], "Degenerate edge.");
      MakeEdgeCanonical(edge);
      SVTKM_TEST_ASSERT(edge[0] < edge[1], "Internal test error: MakeEdgeCanonical failed");
      SVTKM_TEST_ASSERT(edgeSet.find(edge) == edgeSet.end(), "Found duplicate edge");
      edgeSet.insert(edge);

      svtkm::Id2 canonicalEdgeId =
        svtkm::exec::CellEdgeCanonicalId(numPoints, edgeIndex, shape, pointIndexProxy, workletProxy);
      SVTKM_TEST_ASSERT(canonicalEdgeId[0] > 0, "Not using global ids?");
      SVTKM_TEST_ASSERT(canonicalEdgeId[0] < canonicalEdgeId[1], "Bad order.");
    }

    svtkm::IdComponent numFaces = svtkm::exec::CellFaceNumberOfFaces(shape, workletProxy);
    SVTKM_TEST_ASSERT(numFaces == 0, "Non 3D shape should have no faces");
  }

  // Less important case of cells that have less than 2 dimensions
  // (no faces or edges)
  template <typename CellShapeTag, svtkm::IdComponent NumDimensions>
  void DoTest(svtkm::IdComponent numPoints,
              CellShapeTag shape,
              svtkm::CellTopologicalDimensionsTag<NumDimensions>) const
  {
    // Stuff to fake running in the execution environment.
    char messageBuffer[256];
    messageBuffer[0] = '\0';
    svtkm::exec::internal::ErrorMessageBuffer errorMessage(messageBuffer, 256);
    svtkm::exec::FunctorBase workletProxy;
    workletProxy.SetErrorMessageBuffer(errorMessage);

    svtkm::IdComponent numEdges = svtkm::exec::CellEdgeNumberOfEdges(numPoints, shape, workletProxy);
    SVTKM_TEST_ASSERT(numEdges == 0, "0D or 1D shape should have no edges");

    svtkm::IdComponent numFaces = svtkm::exec::CellFaceNumberOfFaces(shape, workletProxy);
    SVTKM_TEST_ASSERT(numFaces == 0, "Non 3D shape should have no faces");
  }

  template <typename CellShapeTag>
  void TryShapeWithNumPoints(svtkm::IdComponent numPoints, CellShapeTag) const
  {
    std::cout << "--- Test shape tag directly"
              << " (" << numPoints << " points)" << std::endl;
    this->DoTest(numPoints,
                 CellShapeTag(),
                 typename svtkm::CellTraits<CellShapeTag>::TopologicalDimensionsTag());

    std::cout << "--- Test generic shape tag"
              << " (" << numPoints << " points)" << std::endl;
    this->DoTest(numPoints,
                 svtkm::CellShapeTagGeneric(CellShapeTag::Id),
                 typename svtkm::CellTraits<CellShapeTag>::TopologicalDimensionsTag());
  }

  template <typename CellShapeTag>
  void operator()(CellShapeTag) const
  {
    this->TryShapeWithNumPoints(svtkm::CellTraits<CellShapeTag>::NUM_POINTS, CellShapeTag());
  }

  void operator()(svtkm::CellShapeTagPolyLine) const
  {
    for (svtkm::IdComponent numPoints = 3; numPoints < 7; numPoints++)
    {
      this->TryShapeWithNumPoints(numPoints, svtkm::CellShapeTagPolyLine());
    }
  }

  void operator()(svtkm::CellShapeTagPolygon) const
  {
    for (svtkm::IdComponent numPoints = 3; numPoints < 7; numPoints++)
    {
      this->TryShapeWithNumPoints(numPoints, svtkm::CellShapeTagPolygon());
    }
  }
};

void TestAllShapes()
{
  svtkm::testing::Testing::TryAllCellShapes(TestCellFacesFunctor());
}

} // anonymous namespace

int UnitTestCellEdgeFace(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestAllShapes, argc, argv);
}
