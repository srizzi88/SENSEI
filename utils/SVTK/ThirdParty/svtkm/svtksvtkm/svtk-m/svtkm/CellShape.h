//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_CellShape_h
#define svtk_m_CellShape_h

#include <svtkm/StaticAssert.h>
#include <svtkm/Types.h>

#include <lcl/Polygon.h>
#include <lcl/Shapes.h>

// Vtk-c does not have tags for Empty and PolyLine. Define dummy tags here to
// avoid compilation errors. These tags are not used anywhere.
namespace lcl
{
struct Empty;
struct PolyLine;
}

namespace svtkm
{

/// CellShapeId identifies the type of each cell. Currently these are designed
/// to match up with SVTK cell types.
///
enum CellShapeIdEnum
{
  // Linear cells
  CELL_SHAPE_EMPTY = lcl::ShapeId::EMPTY,
  CELL_SHAPE_VERTEX = lcl::ShapeId::VERTEX,
  //CELL_SHAPE_POLY_VERTEX      = 2,
  CELL_SHAPE_LINE = lcl::ShapeId::LINE,
  CELL_SHAPE_POLY_LINE = 4,
  CELL_SHAPE_TRIANGLE = lcl::ShapeId::TRIANGLE,
  //CELL_SHAPE_TRIANGLE_STRIP   = 6,
  CELL_SHAPE_POLYGON = lcl::ShapeId::POLYGON,
  //CELL_SHAPE_PIXEL            = 8,
  CELL_SHAPE_QUAD = lcl::ShapeId::QUAD,
  CELL_SHAPE_TETRA = lcl::ShapeId::TETRA,
  //CELL_SHAPE_VOXEL            = 11,
  CELL_SHAPE_HEXAHEDRON = lcl::ShapeId::HEXAHEDRON,
  CELL_SHAPE_WEDGE = lcl::ShapeId::WEDGE,
  CELL_SHAPE_PYRAMID = lcl::ShapeId::PYRAMID,

  NUMBER_OF_CELL_SHAPES
};

// If you wish to add cell shapes to this list, in addition to adding an index
// to the enum above, you at a minimum need to define an associated tag with
// SVTKM_DEFINE_CELL_TAG and add a condition to the svtkmGenericCellShapeMacro.
// There are also many other cell-specific features that code might expect such
// as \c CellTraits and interpolations.

namespace internal
{

/// A class that can be used to determine if a class is a CellShapeTag or not.
/// The class will be either std::true_type or std::false_type.
///
template <typename T>
struct CellShapeTagCheck : std::false_type
{
};

/// Convert SVTK-m tag to SVTK-c tag
template <typename VtkmCellShapeTag>
struct CellShapeTagVtkmToVtkc;

} // namespace internal

/// Checks that the argument is a proper cell shape tag. This is a handy
/// concept check to make sure that a template argument is a proper cell shape
/// tag.
///
#define SVTKM_IS_CELL_SHAPE_TAG(tag)                                                                \
  SVTKM_STATIC_ASSERT_MSG(::svtkm::internal::CellShapeTagCheck<tag>::value,                          \
                         "Provided type is not a valid SVTK-m cell shape tag.")

/// A traits-like class to get an CellShapeId known at compile time to a tag.
///
template <svtkm::IdComponent Id>
struct CellShapeIdToTag
{
  // If you get a compile error for this class about Id not being defined, that
  // probably means you are using an ID that does not have a defined cell
  // shape.

  using valid = std::false_type;
};

// Define a tag for each cell shape as well as the support structs to go
// between tags and ids. The following macro is only valid here.

#define SVTKM_DEFINE_CELL_TAG(name, idname)                                                         \
  struct CellShapeTag##name                                                                        \
  {                                                                                                \
    static constexpr svtkm::UInt8 Id = svtkm::idname;                                                \
  };                                                                                               \
  namespace internal                                                                               \
  {                                                                                                \
  template <>                                                                                      \
  struct CellShapeTagCheck<svtkm::CellShapeTag##name> : std::true_type                              \
  {                                                                                                \
  };                                                                                               \
  template <>                                                                                      \
  struct CellShapeTagVtkmToVtkc<svtkm::CellShapeTag##name>                                          \
  {                                                                                                \
    using Type = lcl::name;                                                                        \
  };                                                                                               \
  }                                                                                                \
  static inline SVTKM_EXEC_CONT const char* GetCellShapeName(svtkm::CellShapeTag##name)              \
  {                                                                                                \
    return #name;                                                                                  \
  }                                                                                                \
  template <>                                                                                      \
  struct CellShapeIdToTag<svtkm::idname>                                                            \
  {                                                                                                \
    using valid = std::true_type;                                                                  \
    using Tag = svtkm::CellShapeTag##name;                                                          \
  }

SVTKM_DEFINE_CELL_TAG(Empty, CELL_SHAPE_EMPTY);
SVTKM_DEFINE_CELL_TAG(Vertex, CELL_SHAPE_VERTEX);
//SVTKM_DEFINE_CELL_TAG(PolyVertex, CELL_SHAPE_POLY_VERTEX);
SVTKM_DEFINE_CELL_TAG(Line, CELL_SHAPE_LINE);
SVTKM_DEFINE_CELL_TAG(PolyLine, CELL_SHAPE_POLY_LINE);
SVTKM_DEFINE_CELL_TAG(Triangle, CELL_SHAPE_TRIANGLE);
//SVTKM_DEFINE_CELL_TAG(TriangleStrip, CELL_SHAPE_TRIANGLE_STRIP);
SVTKM_DEFINE_CELL_TAG(Polygon, CELL_SHAPE_POLYGON);
//SVTKM_DEFINE_CELL_TAG(Pixel, CELL_SHAPE_PIXEL);
SVTKM_DEFINE_CELL_TAG(Quad, CELL_SHAPE_QUAD);
SVTKM_DEFINE_CELL_TAG(Tetra, CELL_SHAPE_TETRA);
//SVTKM_DEFINE_CELL_TAG(Voxel, CELL_SHAPE_VOXEL);
SVTKM_DEFINE_CELL_TAG(Hexahedron, CELL_SHAPE_HEXAHEDRON);
SVTKM_DEFINE_CELL_TAG(Wedge, CELL_SHAPE_WEDGE);
SVTKM_DEFINE_CELL_TAG(Pyramid, CELL_SHAPE_PYRAMID);

#undef SVTKM_DEFINE_CELL_TAG

/// A special cell shape tag that holds a cell shape that is not known at
/// compile time. Unlike other cell set tags, the Id field is set at runtime
/// so its value cannot be used in template parameters. You need to use
/// \c svtkmGenericCellShapeMacro to specialize on the cell type.
///
struct CellShapeTagGeneric
{
  SVTKM_EXEC_CONT
  CellShapeTagGeneric(svtkm::UInt8 shape)
    : Id(shape)
  {
  }

  svtkm::UInt8 Id;
};

namespace internal
{

template <typename VtkmCellShapeTag>
SVTKM_EXEC_CONT inline typename CellShapeTagVtkmToVtkc<VtkmCellShapeTag>::Type make_VtkcCellShapeTag(
  const VtkmCellShapeTag&,
  svtkm::IdComponent numPoints = 0)
{
  using VtkcCellShapeTag = typename CellShapeTagVtkmToVtkc<VtkmCellShapeTag>::Type;
  static_cast<void>(numPoints); // unused
  return VtkcCellShapeTag{};
}

SVTKM_EXEC_CONT
inline lcl::Polygon make_VtkcCellShapeTag(const svtkm::CellShapeTagPolygon&,
                                          svtkm::IdComponent numPoints = 0)
{
  return lcl::Polygon(numPoints);
}

SVTKM_EXEC_CONT
inline lcl::Cell make_VtkcCellShapeTag(const svtkm::CellShapeTagGeneric& tag,
                                       svtkm::IdComponent numPoints = 0)
{
  return lcl::Cell(static_cast<std::int8_t>(tag.Id), numPoints);
}

} // namespace internal

#define svtkmGenericCellShapeMacroCase(cellShapeId, call)                                           \
  case svtkm::cellShapeId:                                                                          \
  {                                                                                                \
    using CellShapeTag = svtkm::CellShapeIdToTag<svtkm::cellShapeId>::Tag;                           \
    call;                                                                                          \
  }                                                                                                \
  break

/// \brief A macro used in a \c switch statement to determine cell shape.
///
/// The \c svtkmGenericCellShapeMacro is a series of case statements for all
/// of the cell shapes supported by SVTK-m. This macro is intended to be used
/// inside of a switch statement on a cell type. For each cell shape condition,
/// a \c CellShapeTag typedef is created and the given \c call is executed.
///
/// A typical use case of this class is to create the specialization of a
/// function overloaded on a cell shape tag for the generic cell shape like as
/// following.
///
/// \code{.cpp}
/// template<typename WorkletType>
/// SVTKM_EXEC
/// void MyCellOperation(svtkm::CellShapeTagGeneric cellShape,
///                      const svtkm::exec::FunctorBase &worklet)
/// {
///   switch(cellShape.CellShapeId)
///   {
///     svtkmGenericCellShapeMacro(
///       MyCellOperation(CellShapeTag())
///       );
///     default: worklet.RaiseError("Encountered unknown cell shape."); break
///   }
/// }
/// \endcode
///
/// Note that \c svtkmGenericCellShapeMacro does not have a default case. You
/// should consider adding one that gives a
///
#define svtkmGenericCellShapeMacro(call)                                                            \
  svtkmGenericCellShapeMacroCase(CELL_SHAPE_EMPTY, call);                                           \
  svtkmGenericCellShapeMacroCase(CELL_SHAPE_VERTEX, call);                                          \
  svtkmGenericCellShapeMacroCase(CELL_SHAPE_LINE, call);                                            \
  svtkmGenericCellShapeMacroCase(CELL_SHAPE_POLY_LINE, call);                                       \
  svtkmGenericCellShapeMacroCase(CELL_SHAPE_TRIANGLE, call);                                        \
  svtkmGenericCellShapeMacroCase(CELL_SHAPE_POLYGON, call);                                         \
  svtkmGenericCellShapeMacroCase(CELL_SHAPE_QUAD, call);                                            \
  svtkmGenericCellShapeMacroCase(CELL_SHAPE_TETRA, call);                                           \
  svtkmGenericCellShapeMacroCase(CELL_SHAPE_HEXAHEDRON, call);                                      \
  svtkmGenericCellShapeMacroCase(CELL_SHAPE_WEDGE, call);                                           \
  svtkmGenericCellShapeMacroCase(CELL_SHAPE_PYRAMID, call)

} // namespace svtkm

#endif //svtk_m_CellShape_h
