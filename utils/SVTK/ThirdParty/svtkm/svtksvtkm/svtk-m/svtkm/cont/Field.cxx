//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/Field.h>

namespace svtkm
{
namespace cont
{

/// constructors for points / whole mesh
SVTKM_CONT
Field::Field(std::string name, Association association, const svtkm::cont::VariantArrayHandle& data)
  : Name(name)
  , FieldAssociation(association)
  , Data(data)
  , Range()
  , ModifiedFlag(true)
{
}

SVTKM_CONT
Field::Field(const svtkm::cont::Field& src)
  : Name(src.Name)
  , FieldAssociation(src.FieldAssociation)
  , Data(src.Data)
  , Range(src.Range)
  , ModifiedFlag(src.ModifiedFlag)
{
}

SVTKM_CONT
Field::Field(svtkm::cont::Field&& src) noexcept : Name(std::move(src.Name)),
                                                 FieldAssociation(std::move(src.FieldAssociation)),
                                                 Data(std::move(src.Data)),
                                                 Range(std::move(src.Range)),
                                                 ModifiedFlag(std::move(src.ModifiedFlag))
{
}

SVTKM_CONT
Field& Field::operator=(const svtkm::cont::Field& src)
{
  this->Name = src.Name;
  this->FieldAssociation = src.FieldAssociation;
  this->Data = src.Data;
  this->Range = src.Range;
  this->ModifiedFlag = src.ModifiedFlag;
  return *this;
}

SVTKM_CONT
Field& Field::operator=(svtkm::cont::Field&& src) noexcept
{
  this->Name = std::move(src.Name);
  this->FieldAssociation = std::move(src.FieldAssociation);
  this->Data = std::move(src.Data);
  this->Range = std::move(src.Range);
  this->ModifiedFlag = std::move(src.ModifiedFlag);
  return *this;
}


SVTKM_CONT
void Field::PrintSummary(std::ostream& out) const
{
  out << "   " << this->Name;
  out << " assoc= ";
  switch (this->GetAssociation())
  {
    case Association::ANY:
      out << "Any ";
      break;
    case Association::WHOLE_MESH:
      out << "Mesh ";
      break;
    case Association::POINTS:
      out << "Points ";
      break;
    case Association::CELL_SET:
      out << "Cells ";
      break;
  }
  this->Data.PrintSummary(out);
}

SVTKM_CONT
Field::~Field()
{
}


SVTKM_CONT
const svtkm::cont::VariantArrayHandle& Field::GetData() const
{
  return this->Data;
}

SVTKM_CONT
svtkm::cont::VariantArrayHandle& Field::GetData()
{
  this->ModifiedFlag = true;
  return this->Data;
}
}
} // namespace svtkm::cont
