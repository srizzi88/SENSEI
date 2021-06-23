//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_CellTraits_h
#define svtk_m_CellTraits_h

#include <svtkm/CellShape.h>

namespace svtkm
{

/// \c svtkm::CellTraits::TopologyDimensionType is typedef to this with the
/// template parameter set to \c TOPOLOGICAL_DIMENSIONS. See \c
/// svtkm::CellTraits for more information.
///
template <svtkm::IdComponent dimension>
struct CellTopologicalDimensionsTag
{
};

/// \brief Tag for cell shapes with a fixed number of points.
///
struct CellTraitsTagSizeFixed
{
};

/// \brief Tag for cell shapes that can have a variable number of points.
///
struct CellTraitsTagSizeVariable
{
};

/// \brief Information about a cell based on its tag.
///
/// The templated CellTraits struct provides the basic high level information
/// about cells (like the number of vertices in the cell or its
/// dimensionality).
///
template <class CellTag>
struct CellTraits
#ifdef SVTKM_DOXYGEN_ONLY
{
  /// This defines the topological dimensions of the cell type. 3 for
  /// polyhedra, 2 for polygons, 1 for lines, 0 for points.
  ///
  static const svtkm::IdComponent TOPOLOGICAL_DIMENSIONS = 3;

  /// This tag is typedef'ed to
  /// svtkm::CellTopologicalDimensionsTag<TOPOLOGICAL_DIMENSIONS>. This provides
  /// a convenient way to overload a function based on topological dimensions
  /// (which is usually more efficient than conditionals).
  ///
  using TopologicalDimensionsTag = svtkm::CellTopologicalDimensionsTag<TOPOLOGICAL_DIMENSIONS>;

  /// \brief A tag specifying whether the number of points is fixed.
  ///
  /// If set to \c CellTraitsTagSizeFixed, then \c NUM_POINTS is set. If set to
  /// \c CellTraitsTagSizeVariable, then the number of points is not known at
  /// compile time.
  ///
  using IsSizeFixed = svtkm::CellTraitsTagSizeFixed;

  /// \brief Number of points in the cell.
  ///
  /// This is only defined for cell shapes of a fixed number of points (i.e.
  /// \c IsSizedFixed is set to \c CellTraitsTagSizeFixed.
  ///
  static constexpr svtkm::IdComponent NUM_POINTS = 3;
};
#else  // SVTKM_DOXYGEN_ONLY
  ;
#endif // SVTKM_DOXYGEN_ONLY

//-----------------------------------------------------------------------------

// Define traits for every cell type.

#define SVTKM_DEFINE_CELL_TRAITS(name, dimensions, numPoints)                                       \
  template <>                                                                                      \
  struct CellTraits<svtkm::CellShapeTag##name>                                                      \
  {                                                                                                \
    static constexpr svtkm::IdComponent TOPOLOGICAL_DIMENSIONS = dimensions;                        \
    using TopologicalDimensionsTag = svtkm::CellTopologicalDimensionsTag<TOPOLOGICAL_DIMENSIONS>;   \
    using IsSizeFixed = svtkm::CellTraitsTagSizeFixed;                                              \
    static constexpr svtkm::IdComponent NUM_POINTS = numPoints;                                     \
  }

#define SVTKM_DEFINE_CELL_TRAITS_VARIABLE(name, dimensions)                                         \
  template <>                                                                                      \
  struct CellTraits<svtkm::CellShapeTag##name>                                                      \
  {                                                                                                \
    static constexpr svtkm::IdComponent TOPOLOGICAL_DIMENSIONS = dimensions;                        \
    using TopologicalDimensionsTag = svtkm::CellTopologicalDimensionsTag<TOPOLOGICAL_DIMENSIONS>;   \
    using IsSizeFixed = svtkm::CellTraitsTagSizeVariable;                                           \
  }

SVTKM_DEFINE_CELL_TRAITS(Empty, 0, 0);
SVTKM_DEFINE_CELL_TRAITS(Vertex, 0, 1);
//SVTKM_DEFINE_CELL_TRAITS_VARIABLE(PolyVertex, 0);
SVTKM_DEFINE_CELL_TRAITS(Line, 1, 2);
SVTKM_DEFINE_CELL_TRAITS_VARIABLE(PolyLine, 1);
SVTKM_DEFINE_CELL_TRAITS(Triangle, 2, 3);
//SVTKM_DEFINE_CELL_TRAITS_VARIABLE(TriangleStrip, 2);
SVTKM_DEFINE_CELL_TRAITS_VARIABLE(Polygon, 2);
//SVTKM_DEFINE_CELL_TRAITS(Pixel, 2, 4);
SVTKM_DEFINE_CELL_TRAITS(Quad, 2, 4);
SVTKM_DEFINE_CELL_TRAITS(Tetra, 3, 4);
//SVTKM_DEFINE_CELL_TRAITS(Voxel, 3, 8);
SVTKM_DEFINE_CELL_TRAITS(Hexahedron, 3, 8);
SVTKM_DEFINE_CELL_TRAITS(Wedge, 3, 6);
SVTKM_DEFINE_CELL_TRAITS(Pyramid, 3, 5);

#undef SVTKM_DEFINE_CELL_TRAITS

} // namespace svtkm

#endif //svtk_m_CellTraits_h
