//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/DataSet.h>

namespace svtkm
{
namespace cont
{
void DataSet::Clear()
{
  this->CoordSystems.clear();
  this->Fields.clear();
  this->CellSet = this->CellSet.NewInstance();
}

svtkm::Id DataSet::GetNumberOfCells() const
{
  return this->CellSet.GetNumberOfCells();
}

svtkm::Id DataSet::GetNumberOfPoints() const
{
  if (this->CoordSystems.empty())
  {
    return 0;
  }
  return this->CoordSystems[0].GetNumberOfPoints();
}

void DataSet::CopyStructure(const svtkm::cont::DataSet& source)
{
  this->CoordSystems = source.CoordSystems;
  this->CellSet = source.CellSet;
}

const svtkm::cont::Field& DataSet::GetField(svtkm::Id index) const
{
  SVTKM_ASSERT((index >= 0) && (index < this->GetNumberOfFields()));
  return this->Fields[static_cast<std::size_t>(index)];
}

svtkm::cont::Field& DataSet::GetField(svtkm::Id index)
{
  SVTKM_ASSERT((index >= 0) && (index < this->GetNumberOfFields()));
  return this->Fields[static_cast<std::size_t>(index)];
}

svtkm::Id DataSet::GetFieldIndex(const std::string& name, svtkm::cont::Field::Association assoc) const
{
  bool found;
  svtkm::Id index = this->FindFieldIndex(name, assoc, found);
  if (found)
  {
    return index;
  }
  else
  {
    throw svtkm::cont::ErrorBadValue("No field with requested name: " + name);
  }
}

const svtkm::cont::CoordinateSystem& DataSet::GetCoordinateSystem(svtkm::Id index) const
{
  SVTKM_ASSERT((index >= 0) && (index < this->GetNumberOfCoordinateSystems()));
  return this->CoordSystems[static_cast<std::size_t>(index)];
}

svtkm::cont::CoordinateSystem& DataSet::GetCoordinateSystem(svtkm::Id index)
{
  SVTKM_ASSERT((index >= 0) && (index < this->GetNumberOfCoordinateSystems()));
  return this->CoordSystems[static_cast<std::size_t>(index)];
}

svtkm::Id DataSet::GetCoordinateSystemIndex(const std::string& name) const
{
  svtkm::Id index = -1;
  for (auto i = this->CoordSystems.begin(); i != this->CoordSystems.end(); ++i)
  {
    if (i->GetName() == name)
    {
      index = static_cast<svtkm::Id>(std::distance(this->CoordSystems.begin(), i));
      break;
    }
  }
  return index;
}

const svtkm::cont::CoordinateSystem& DataSet::GetCoordinateSystem(const std::string& name) const
{
  svtkm::Id index = this->GetCoordinateSystemIndex(name);
  if (index < 0)
  {
    std::string error_message("No coordinate system with the name " + name +
                              " valid names are: \n");
    for (const auto& cs : this->CoordSystems)
    {
      error_message += cs.GetName() + "\n";
    }
    throw svtkm::cont::ErrorBadValue(error_message);
  }
  return this->GetCoordinateSystem(index);
}

svtkm::cont::CoordinateSystem& DataSet::GetCoordinateSystem(const std::string& name)
{
  svtkm::Id index = this->GetCoordinateSystemIndex(name);
  if (index < 0)
  {
    std::string error_message("No coordinate system with the name " + name +
                              " valid names are: \n");
    for (const auto& cs : this->CoordSystems)
    {
      error_message += cs.GetName() + "\n";
    }
    throw svtkm::cont::ErrorBadValue(error_message);
  }
  return this->GetCoordinateSystem(index);
}

void DataSet::PrintSummary(std::ostream& out) const
{
  out << "DataSet:\n";
  out << "  CoordSystems[" << this->CoordSystems.size() << "]\n";
  for (std::size_t index = 0; index < this->CoordSystems.size(); index++)
  {
    this->CoordSystems[index].PrintSummary(out);
  }

  out << "  CellSet \n";
  this->GetCellSet().PrintSummary(out);

  out << "  Fields[" << this->GetNumberOfFields() << "]\n";
  for (svtkm::Id index = 0; index < this->GetNumberOfFields(); index++)
  {
    this->GetField(index).PrintSummary(out);
  }

  out.flush();
}

svtkm::Id DataSet::FindFieldIndex(const std::string& name,
                                 svtkm::cont::Field::Association association,
                                 bool& found) const
{
  for (std::size_t index = 0; index < this->Fields.size(); ++index)
  {
    if ((association == svtkm::cont::Field::Association::ANY ||
         association == this->Fields[index].GetAssociation()) &&
        this->Fields[index].GetName() == name)
    {
      found = true;
      return static_cast<svtkm::Id>(index);
    }
  }
  found = false;
  return -1;
}

} // namespace cont
} // namespace svtkm
