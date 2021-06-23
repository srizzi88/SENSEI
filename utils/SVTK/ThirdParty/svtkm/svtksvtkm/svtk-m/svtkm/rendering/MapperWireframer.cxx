//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/Assert.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>

#include <svtkm/cont/TryExecute.h>
#include <svtkm/exec/CellEdge.h>
#include <svtkm/filter/ExternalFaces.h>
#include <svtkm/rendering/CanvasRayTracer.h>
#include <svtkm/rendering/MapperRayTracer.h>
#include <svtkm/rendering/MapperWireframer.h>
#include <svtkm/rendering/Wireframer.h>
#include <svtkm/worklet/DispatcherMapField.h>
#include <svtkm/worklet/DispatcherMapTopology.h>
#include <svtkm/worklet/ScatterCounting.h>
#include <svtkm/worklet/WorkletMapField.h>
#include <svtkm/worklet/WorkletMapTopology.h>

namespace svtkm
{
namespace rendering
{
namespace
{

class CreateConnectivity : public svtkm::worklet::WorkletMapField
{
public:
  SVTKM_CONT
  CreateConnectivity() {}

  using ControlSignature = void(FieldIn, WholeArrayOut);

  using ExecutionSignature = void(_1, _2);

  template <typename ConnPortalType>
  SVTKM_EXEC void operator()(const svtkm::Id& i, ConnPortalType& connPortal) const
  {
    connPortal.Set(i * 2 + 0, i);
    connPortal.Set(i * 2 + 1, i + 1);
  }
}; // conn

class Convert1DCoordinates : public svtkm::worklet::WorkletMapField
{
private:
  bool LogY;
  bool LogX;

public:
  SVTKM_CONT
  Convert1DCoordinates(bool logY, bool logX)
    : LogY(logY)
    , LogX(logX)
  {
  }

  using ControlSignature = void(FieldIn, FieldIn, FieldOut, FieldOut);

  using ExecutionSignature = void(_1, _2, _3, _4);
  template <typename ScalarType>
  SVTKM_EXEC void operator()(const svtkm::Vec3f_32& inCoord,
                            const ScalarType& scalar,
                            svtkm::Vec3f_32& outCoord,
                            svtkm::Float32& fieldOut) const
  {
    //
    // Rendering supports lines based on a cellSetStructured<1>
    // where only the x coord matters. It creates a y based on
    // the scalar values and connects all the points with lines.
    // So, we need to convert it back to something that can
    // actually be rendered.
    //
    outCoord[0] = inCoord[0];
    outCoord[1] = static_cast<svtkm::Float32>(scalar);
    outCoord[2] = 0.f;
    if (LogY)
    {
      outCoord[1] = svtkm::Log10(outCoord[1]);
    }
    if (LogX)
    {
      outCoord[0] = svtkm::Log10(outCoord[0]);
    }
    // all lines have the same color
    fieldOut = 1.f;
  }
}; // convert coords

#if defined(SVTKM_MSVC)
#pragma warning(push)
#pragma warning(disable : 4127) //conditional expression is constant
#endif
struct EdgesCounter : public svtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet, FieldOutCell numEdges);
  using ExecutionSignature = _2(CellShape shape, PointCount numPoints);
  using InputDomain = _1;

  template <typename CellShapeTag>
  SVTKM_EXEC svtkm::IdComponent operator()(CellShapeTag shape, svtkm::IdComponent numPoints) const
  {
    //TODO: Remove the if/then with templates.
    if (shape.Id == svtkm::CELL_SHAPE_LINE)
    {
      return 1;
    }
    else
    {
      return svtkm::exec::CellEdgeNumberOfEdges(numPoints, shape, *this);
    }
  }
}; // struct EdgesCounter

struct EdgesExtracter : public svtkm::worklet::WorkletVisitCellsWithPoints
{
  using ControlSignature = void(CellSetIn cellSet, FieldOutCell edgeIndices);
  using ExecutionSignature = void(CellShape, PointIndices, VisitIndex, _2);
  using InputDomain = _1;
  using ScatterType = svtkm::worklet::ScatterCounting;

  SVTKM_CONT
  template <typename CountArrayType>
  static ScatterType MakeScatter(const CountArrayType& counts)
  {
    return ScatterType(counts);
  }

  template <typename CellShapeTag, typename PointIndexVecType, typename EdgeIndexVecType>
  SVTKM_EXEC void operator()(CellShapeTag shape,
                            const PointIndexVecType& pointIndices,
                            svtkm::IdComponent visitIndex,
                            EdgeIndexVecType& edgeIndices) const
  {
    //TODO: Remove the if/then with templates.
    svtkm::Id p1, p2;
    if (shape.Id == svtkm::CELL_SHAPE_LINE)
    {
      p1 = pointIndices[0];
      p2 = pointIndices[1];
    }
    else
    {
      p1 = pointIndices[svtkm::exec::CellEdgeLocalIndex(
        pointIndices.GetNumberOfComponents(), 0, visitIndex, shape, *this)];
      p2 = pointIndices[svtkm::exec::CellEdgeLocalIndex(
        pointIndices.GetNumberOfComponents(), 1, visitIndex, shape, *this)];
    }
    // These indices need to be arranged in a definite order, as they will later be sorted to
    // detect duplicates
    edgeIndices[0] = p1 < p2 ? p1 : p2;
    edgeIndices[1] = p1 < p2 ? p2 : p1;
  }
}; // struct EdgesExtracter

#if defined(SVTKM_MSVC)
#pragma warning(pop)
#endif
} // namespace

struct MapperWireframer::InternalsType
{
  InternalsType()
    : InternalsType(nullptr, false, false)
  {
  }

  InternalsType(svtkm::rendering::Canvas* canvas, bool showInternalZones, bool isOverlay)
    : Canvas(canvas)
    , ShowInternalZones(showInternalZones)
    , IsOverlay(isOverlay)
    , CompositeBackground(true)
  {
  }

  svtkm::rendering::Canvas* Canvas;
  bool ShowInternalZones;
  bool IsOverlay;
  bool CompositeBackground;
}; // struct MapperWireframer::InternalsType

MapperWireframer::MapperWireframer()
  : Internals(new InternalsType(nullptr, false, false))
{
}

MapperWireframer::~MapperWireframer()
{
}

svtkm::rendering::Canvas* MapperWireframer::GetCanvas() const
{
  return this->Internals->Canvas;
}

void MapperWireframer::SetCanvas(svtkm::rendering::Canvas* canvas)
{
  this->Internals->Canvas = canvas;
}

bool MapperWireframer::GetShowInternalZones() const
{
  return this->Internals->ShowInternalZones;
}

void MapperWireframer::SetShowInternalZones(bool showInternalZones)
{
  this->Internals->ShowInternalZones = showInternalZones;
}

bool MapperWireframer::GetIsOverlay() const
{
  return this->Internals->IsOverlay;
}

void MapperWireframer::SetIsOverlay(bool isOverlay)
{
  this->Internals->IsOverlay = isOverlay;
}

void MapperWireframer::StartScene()
{
  // Nothing needs to be done.
}

void MapperWireframer::EndScene()
{
  // Nothing needs to be done.
}

void MapperWireframer::RenderCells(const svtkm::cont::DynamicCellSet& inCellSet,
                                   const svtkm::cont::CoordinateSystem& coords,
                                   const svtkm::cont::Field& inScalarField,
                                   const svtkm::cont::ColorTable& colorTable,
                                   const svtkm::rendering::Camera& camera,
                                   const svtkm::Range& scalarRange)
{
  svtkm::cont::DynamicCellSet cellSet = inCellSet;

  bool is1D = cellSet.IsSameType(svtkm::cont::CellSetStructured<1>());

  svtkm::cont::CoordinateSystem actualCoords = coords;
  svtkm::cont::Field actualField = inScalarField;

  if (is1D)
  {

    const bool isSupportedField = inScalarField.IsFieldPoint();
    if (!isSupportedField)
    {
      throw svtkm::cont::ErrorBadValue(
        "WireFramer: field must be associated with points for 1D cell set");
    }
    svtkm::cont::ArrayHandle<svtkm::Vec3f_32> newCoords;
    svtkm::cont::ArrayHandle<svtkm::Float32> newScalars;
    //
    // Convert the cell set into something we can draw
    //
    svtkm::worklet::DispatcherMapField<Convert1DCoordinates>(
      Convert1DCoordinates(this->LogarithmY, this->LogarithmX))
      .Invoke(coords.GetData(),
              inScalarField.GetData().ResetTypes(svtkm::TypeListFieldScalar()),
              newCoords,
              newScalars);

    actualCoords = svtkm::cont::CoordinateSystem("coords", newCoords);
    actualField = svtkm::cont::Field(
      inScalarField.GetName(), svtkm::cont::Field::Association::POINTS, newScalars);

    svtkm::Id numCells = cellSet.GetNumberOfCells();
    svtkm::cont::ArrayHandle<svtkm::Id> conn;
    svtkm::cont::ArrayHandleCounting<svtkm::Id> iter =
      svtkm::cont::make_ArrayHandleCounting(svtkm::Id(0), svtkm::Id(1), numCells);
    conn.Allocate(numCells * 2);
    svtkm::worklet::DispatcherMapField<CreateConnectivity>(CreateConnectivity()).Invoke(iter, conn);

    svtkm::cont::CellSetSingleType<> newCellSet;
    newCellSet.Fill(newCoords.GetNumberOfValues(), svtkm::CELL_SHAPE_LINE, 2, conn);
    cellSet = svtkm::cont::DynamicCellSet(newCellSet);
  }
  bool isLines = false;
  // Check for a cell set that is already lines
  // Since there is no need to de external faces or
  // render the depth of the mesh to hide internal zones
  if (cellSet.IsSameType(svtkm::cont::CellSetSingleType<>()))
  {
    auto singleType = cellSet.Cast<svtkm::cont::CellSetSingleType<>>();
    isLines = singleType.GetCellShape(0) == svtkm::CELL_SHAPE_LINE;
  }

  bool doExternalFaces = !(this->Internals->ShowInternalZones) && !isLines && !is1D;
  if (doExternalFaces)
  {
    // If internal zones are to be hidden, the number of edges processed can be reduced by
    // running the external faces filter on the input cell set.
    svtkm::cont::DataSet dataSet;
    dataSet.AddCoordinateSystem(actualCoords);
    dataSet.SetCellSet(inCellSet);
    dataSet.AddField(inScalarField);
    svtkm::filter::ExternalFaces externalFaces;
    externalFaces.SetCompactPoints(false);
    externalFaces.SetPassPolyData(true);
    svtkm::cont::DataSet output = externalFaces.Execute(dataSet);
    cellSet = output.GetCellSet();
    actualField = output.GetField(0);
  }

  // Extract unique edges from the cell set.
  svtkm::cont::ArrayHandle<svtkm::IdComponent> counts;
  svtkm::cont::ArrayHandle<svtkm::Id2> edgeIndices;
  svtkm::worklet::DispatcherMapTopology<EdgesCounter>().Invoke(cellSet, counts);
  svtkm::worklet::DispatcherMapTopology<EdgesExtracter> extractDispatcher(
    EdgesExtracter::MakeScatter(counts));
  extractDispatcher.Invoke(cellSet, edgeIndices);
  svtkm::cont::Algorithm::template Sort<svtkm::Id2>(edgeIndices);
  svtkm::cont::Algorithm::template Unique<svtkm::Id2>(edgeIndices);

  Wireframer renderer(
    this->Internals->Canvas, this->Internals->ShowInternalZones, this->Internals->IsOverlay);
  // Render the cell set using a raytracer, on a separate canvas, and use the generated depth
  // buffer, which represents the solid mesh, to avoid drawing on the internal zones
  bool renderDepth =
    !(this->Internals->ShowInternalZones) && !(this->Internals->IsOverlay) && !isLines && !is1D;
  if (renderDepth)
  {
    CanvasRayTracer canvas(this->Internals->Canvas->GetWidth(),
                           this->Internals->Canvas->GetHeight());
    canvas.SetBackgroundColor(svtkm::rendering::Color::white);
    canvas.Initialize();
    canvas.Activate();
    canvas.Clear();
    MapperRayTracer raytracer;
    raytracer.SetCanvas(&canvas);
    raytracer.SetActiveColorTable(colorTable);
    raytracer.RenderCells(cellSet, actualCoords, actualField, colorTable, camera, scalarRange);
    renderer.SetSolidDepthBuffer(canvas.GetDepthBuffer());
  }
  else
  {
    renderer.SetSolidDepthBuffer(this->Internals->Canvas->GetDepthBuffer());
  }

  renderer.SetCamera(camera);
  renderer.SetColorMap(this->ColorMap);
  renderer.SetData(actualCoords, edgeIndices, actualField, scalarRange);
  renderer.Render();

  if (this->Internals->CompositeBackground)
  {
    this->Internals->Canvas->BlendBackground();
  }
}

void MapperWireframer::SetCompositeBackground(bool on)
{
  this->Internals->CompositeBackground = on;
}

svtkm::rendering::Mapper* MapperWireframer::NewCopy() const
{
  return new svtkm::rendering::MapperWireframer(*this);
}
}
} // namespace svtkm::rendering
