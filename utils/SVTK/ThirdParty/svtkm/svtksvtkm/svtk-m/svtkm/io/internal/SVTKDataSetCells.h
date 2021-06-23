//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_io_internal_SVTKDataSetCells_h
#define svtk_m_io_internal_SVTKDataSetCells_h

#include <svtkm/CellShape.h>
#include <svtkm/Types.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayPortalToIterators.h>
#include <svtkm/io/ErrorIO.h>

#include <algorithm>
#include <vector>

namespace svtkm
{
namespace io
{
namespace internal
{

enum UnsupportedSVTKCells
{
  CELL_SHAPE_POLY_VERTEX = 2,
  CELL_SHAPE_POLY_LINE = 4,
  CELL_SHAPE_TRIANGLE_STRIP = 6,
  CELL_SHAPE_PIXEL = 8,
  CELL_SHAPE_VOXEL = 11
};

inline void FixupCellSet(svtkm::cont::ArrayHandle<svtkm::Id>& connectivity,
                         svtkm::cont::ArrayHandle<svtkm::IdComponent>& numIndices,
                         svtkm::cont::ArrayHandle<svtkm::UInt8>& shapes,
                         svtkm::cont::ArrayHandle<svtkm::Id>& permutation)
{
  std::vector<svtkm::Id> newConnectivity;
  std::vector<svtkm::IdComponent> newNumIndices;
  std::vector<svtkm::UInt8> newShapes;
  std::vector<svtkm::Id> permutationVec;

  svtkm::Id connIdx = 0;
  for (svtkm::Id i = 0; i < shapes.GetNumberOfValues(); ++i)
  {
    svtkm::UInt8 shape = shapes.GetPortalConstControl().Get(i);
    svtkm::IdComponent numInds = numIndices.GetPortalConstControl().Get(i);
    auto connPortal = connectivity.GetPortalConstControl();
    switch (shape)
    {
      case svtkm::CELL_SHAPE_VERTEX:
      case svtkm::CELL_SHAPE_LINE:
      case svtkm::CELL_SHAPE_TRIANGLE:
      case svtkm::CELL_SHAPE_QUAD:
      case svtkm::CELL_SHAPE_TETRA:
      case svtkm::CELL_SHAPE_HEXAHEDRON:
      case svtkm::CELL_SHAPE_WEDGE:
      case svtkm::CELL_SHAPE_PYRAMID:
      {
        newShapes.push_back(shape);
        newNumIndices.push_back(numInds);
        for (svtkm::IdComponent j = 0; j < numInds; ++j)
        {
          newConnectivity.push_back(connPortal.Get(connIdx++));
        }
        permutationVec.push_back(i);
        break;
      }
      case svtkm::CELL_SHAPE_POLYGON:
      {
        svtkm::IdComponent numVerts = numInds;
        svtkm::UInt8 newShape = svtkm::CELL_SHAPE_POLYGON;
        if (numVerts == 3)
        {
          newShape = svtkm::CELL_SHAPE_TRIANGLE;
        }
        else if (numVerts == 4)
        {
          newShape = svtkm::CELL_SHAPE_QUAD;
        }
        newShapes.push_back(newShape);
        newNumIndices.push_back(numVerts);
        for (svtkm::IdComponent j = 0; j < numVerts; ++j)
        {
          newConnectivity.push_back(connPortal.Get(connIdx++));
        }
        permutationVec.push_back(i);
        break;
      }
      case CELL_SHAPE_POLY_VERTEX:
      {
        svtkm::IdComponent numVerts = numInds;
        for (svtkm::IdComponent j = 0; j < numVerts; ++j)
        {
          newShapes.push_back(svtkm::CELL_SHAPE_VERTEX);
          newNumIndices.push_back(1);
          newConnectivity.push_back(connPortal.Get(connIdx));
          permutationVec.push_back(i);
          ++connIdx;
        }
        break;
      }
      case CELL_SHAPE_POLY_LINE:
      {
        svtkm::IdComponent numLines = numInds - 1;
        for (svtkm::IdComponent j = 0; j < numLines; ++j)
        {
          newShapes.push_back(svtkm::CELL_SHAPE_LINE);
          newNumIndices.push_back(2);
          newConnectivity.push_back(connPortal.Get(connIdx));
          newConnectivity.push_back(connPortal.Get(connIdx + 1));
          permutationVec.push_back(i);
          ++connIdx;
        }
        connIdx += 1;
        break;
      }
      case CELL_SHAPE_TRIANGLE_STRIP:
      {
        svtkm::IdComponent numTris = numInds - 2;
        for (svtkm::IdComponent j = 0; j < numTris; ++j)
        {
          newShapes.push_back(svtkm::CELL_SHAPE_TRIANGLE);
          newNumIndices.push_back(3);
          if (j % 2)
          {
            newConnectivity.push_back(connPortal.Get(connIdx));
            newConnectivity.push_back(connPortal.Get(connIdx + 1));
            newConnectivity.push_back(connPortal.Get(connIdx + 2));
          }
          else
          {
            newConnectivity.push_back(connPortal.Get(connIdx + 2));
            newConnectivity.push_back(connPortal.Get(connIdx + 1));
            newConnectivity.push_back(connPortal.Get(connIdx));
          }
          permutationVec.push_back(i);
          ++connIdx;
        }
        connIdx += 2;
        break;
      }
      case CELL_SHAPE_PIXEL:
      {
        newShapes.push_back(svtkm::CELL_SHAPE_QUAD);
        newNumIndices.push_back(numInds);
        newConnectivity.push_back(connPortal.Get(connIdx + 0));
        newConnectivity.push_back(connPortal.Get(connIdx + 1));
        newConnectivity.push_back(connPortal.Get(connIdx + 3));
        newConnectivity.push_back(connPortal.Get(connIdx + 2));
        permutationVec.push_back(i);
        connIdx += 4;
        break;
      }
      case CELL_SHAPE_VOXEL:
      {
        newShapes.push_back(svtkm::CELL_SHAPE_HEXAHEDRON);
        newNumIndices.push_back(numInds);
        newConnectivity.push_back(connPortal.Get(connIdx + 0));
        newConnectivity.push_back(connPortal.Get(connIdx + 1));
        newConnectivity.push_back(connPortal.Get(connIdx + 3));
        newConnectivity.push_back(connPortal.Get(connIdx + 2));
        newConnectivity.push_back(connPortal.Get(connIdx + 4));
        newConnectivity.push_back(connPortal.Get(connIdx + 5));
        newConnectivity.push_back(connPortal.Get(connIdx + 7));
        newConnectivity.push_back(connPortal.Get(connIdx + 6));
        permutationVec.push_back(i);
        connIdx += 6;
        break;
      }
      default:
      {
        throw svtkm::io::ErrorIO("Encountered unsupported cell type");
      }
    }
  }

  if (newShapes.size() == static_cast<std::size_t>(shapes.GetNumberOfValues()))
  {
    permutationVec.clear();
  }
  else
  {
    permutation.Allocate(static_cast<svtkm::Id>(permutationVec.size()));
    std::copy(permutationVec.begin(),
              permutationVec.end(),
              svtkm::cont::ArrayPortalToIteratorBegin(permutation.GetPortalControl()));
  }

  shapes.Allocate(static_cast<svtkm::Id>(newShapes.size()));
  std::copy(newShapes.begin(),
            newShapes.end(),
            svtkm::cont::ArrayPortalToIteratorBegin(shapes.GetPortalControl()));
  numIndices.Allocate(static_cast<svtkm::Id>(newNumIndices.size()));
  std::copy(newNumIndices.begin(),
            newNumIndices.end(),
            svtkm::cont::ArrayPortalToIteratorBegin(numIndices.GetPortalControl()));
  connectivity.Allocate(static_cast<svtkm::Id>(newConnectivity.size()));
  std::copy(newConnectivity.begin(),
            newConnectivity.end(),
            svtkm::cont::ArrayPortalToIteratorBegin(connectivity.GetPortalControl()));
}

inline bool IsSingleShape(const svtkm::cont::ArrayHandle<svtkm::UInt8>& shapes)
{
  auto shapesPortal = shapes.GetPortalConstControl();
  svtkm::UInt8 shape0 = shapesPortal.Get(0);
  for (svtkm::Id i = 1; i < shapes.GetNumberOfValues(); ++i)
  {
    if (shapesPortal.Get(i) != shape0)
      return false;
  }

  return true;
}
}
}
} // svtkm::io::internal

#endif // svtk_m_io_internal_SVTKDataSetCells_h
