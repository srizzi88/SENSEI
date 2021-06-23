//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/rendering/Actor.h>

#include <svtkm/Assert.h>
#include <svtkm/cont/TryExecute.h>

namespace svtkm
{
namespace rendering
{

struct Actor::InternalsType
{
  svtkm::cont::DynamicCellSet Cells;
  svtkm::cont::CoordinateSystem Coordinates;
  svtkm::cont::Field ScalarField;
  svtkm::cont::ColorTable ColorTable;

  svtkm::Range ScalarRange;
  svtkm::Bounds SpatialBounds;

  SVTKM_CONT
  InternalsType(const svtkm::cont::DynamicCellSet& cells,
                const svtkm::cont::CoordinateSystem& coordinates,
                const svtkm::cont::Field& scalarField,
                const svtkm::rendering::Color& color)
    : Cells(cells)
    , Coordinates(coordinates)
    , ScalarField(scalarField)
    , ColorTable(svtkm::Range{ 0, 1 }, color.Components, color.Components)
  {
  }

  SVTKM_CONT
  InternalsType(const svtkm::cont::DynamicCellSet& cells,
                const svtkm::cont::CoordinateSystem& coordinates,
                const svtkm::cont::Field& scalarField,
                const svtkm::cont::ColorTable& colorTable = svtkm::cont::ColorTable::Preset::DEFAULT)
    : Cells(cells)
    , Coordinates(coordinates)
    , ScalarField(scalarField)
    , ColorTable(colorTable)
  {
  }
};

Actor::Actor(const svtkm::cont::DynamicCellSet& cells,
             const svtkm::cont::CoordinateSystem& coordinates,
             const svtkm::cont::Field& scalarField)
  : Internals(new InternalsType(cells, coordinates, scalarField))
{
  this->Init(coordinates, scalarField);
}

Actor::Actor(const svtkm::cont::DynamicCellSet& cells,
             const svtkm::cont::CoordinateSystem& coordinates,
             const svtkm::cont::Field& scalarField,
             const svtkm::rendering::Color& color)
  : Internals(new InternalsType(cells, coordinates, scalarField, color))
{
  this->Init(coordinates, scalarField);
}

Actor::Actor(const svtkm::cont::DynamicCellSet& cells,
             const svtkm::cont::CoordinateSystem& coordinates,
             const svtkm::cont::Field& scalarField,
             const svtkm::cont::ColorTable& colorTable)
  : Internals(new InternalsType(cells, coordinates, scalarField, colorTable))
{
  this->Init(coordinates, scalarField);
}

void Actor::Init(const svtkm::cont::CoordinateSystem& coordinates,
                 const svtkm::cont::Field& scalarField)
{
  SVTKM_ASSERT(scalarField.GetData().GetNumberOfComponents() == 1);

  scalarField.GetRange(&this->Internals->ScalarRange);
  this->Internals->SpatialBounds = coordinates.GetBounds();
}

void Actor::Render(svtkm::rendering::Mapper& mapper,
                   svtkm::rendering::Canvas& canvas,
                   const svtkm::rendering::Camera& camera) const
{
  mapper.SetCanvas(&canvas);
  mapper.SetActiveColorTable(this->Internals->ColorTable);
  mapper.RenderCells(this->Internals->Cells,
                     this->Internals->Coordinates,
                     this->Internals->ScalarField,
                     this->Internals->ColorTable,
                     camera,
                     this->Internals->ScalarRange);
}

const svtkm::cont::DynamicCellSet& Actor::GetCells() const
{
  return this->Internals->Cells;
}

const svtkm::cont::CoordinateSystem& Actor::GetCoordinates() const
{
  return this->Internals->Coordinates;
}

const svtkm::cont::Field& Actor::GetScalarField() const
{
  return this->Internals->ScalarField;
}

const svtkm::cont::ColorTable& Actor::GetColorTable() const
{
  return this->Internals->ColorTable;
}

const svtkm::Range& Actor::GetScalarRange() const
{
  return this->Internals->ScalarRange;
}

const svtkm::Bounds& Actor::GetSpatialBounds() const
{
  return this->Internals->SpatialBounds;
}

void Actor::SetScalarRange(const svtkm::Range& scalarRange)
{
  this->Internals->ScalarRange = scalarRange;
}
}
} // namespace svtkm::rendering
