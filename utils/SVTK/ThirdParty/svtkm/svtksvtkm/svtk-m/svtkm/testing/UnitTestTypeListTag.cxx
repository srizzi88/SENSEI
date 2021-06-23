//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

// This tests deprecated code until it is deleted.

#include <svtkm/TypeListTag.h>

#include <svtkm/Types.h>

#include <svtkm/testing/Testing.h>

#include <set>
#include <string>

SVTKM_DEPRECATED_SUPPRESS_BEGIN

namespace
{

class TypeSet
{
  using NameSetType = std::set<std::string>;
  NameSetType NameSet;

public:
  template <typename T>
  void AddExpected(T)
  {
    this->NameSet.insert(svtkm::testing::TypeName<T>::Name());
  }

  template <typename T>
  void Found(T)
  {
    std::string name = svtkm::testing::TypeName<T>::Name();
    //std::cout << "  found " << name << std::endl;
    NameSetType::iterator typeLocation = this->NameSet.find(name);
    if (typeLocation != this->NameSet.end())
    {
      // This type is expected. Remove it to mark it found.
      this->NameSet.erase(typeLocation);
    }
    else
    {
      std::cout << "**** Did not expect to get type " << name << std::endl;
      SVTKM_TEST_FAIL("Got unexpected type.");
    }
  }

  void CheckFound()
  {
    for (NameSetType::iterator typeP = this->NameSet.begin(); typeP != this->NameSet.end(); typeP++)
    {
      std::cout << "**** Failed to find " << *typeP << std::endl;
    }
    SVTKM_TEST_ASSERT(this->NameSet.empty(), "List did not call functor on all expected types.");
  }
};

struct TestFunctor
{
  TypeSet ExpectedTypes;

  TestFunctor(const TypeSet& expectedTypes)
    : ExpectedTypes(expectedTypes)
  {
  }

  template <typename T>
  SVTKM_CONT void operator()(T)
  {
    this->ExpectedTypes.Found(T());
  }
};

template <typename ListTag>
void TryList(const TypeSet& expected, ListTag)
{
  TestFunctor functor(expected);
  svtkm::ListForEach(functor, ListTag());
  functor.ExpectedTypes.CheckFound();
}

void TestLists()
{
  std::cout << "TypeListTagId" << std::endl;
  TypeSet id;
  id.AddExpected(svtkm::Id());
  TryList(id, svtkm::TypeListTagId());

  std::cout << "TypeListTagId2" << std::endl;
  TypeSet id2;
  id2.AddExpected(svtkm::Id2());
  TryList(id2, svtkm::TypeListTagId2());

  std::cout << "TypeListTagId3" << std::endl;
  TypeSet id3;
  id3.AddExpected(svtkm::Id3());
  TryList(id3, svtkm::TypeListTagId3());

  std::cout << "TypeListTagIndex" << std::endl;
  TypeSet index;
  index.AddExpected(svtkm::Id());
  index.AddExpected(svtkm::Id2());
  index.AddExpected(svtkm::Id3());
  TryList(index, svtkm::TypeListTagIndex());

  std::cout << "TypeListTagFieldScalar" << std::endl;
  TypeSet scalar;
  scalar.AddExpected(svtkm::Float32());
  scalar.AddExpected(svtkm::Float64());
  TryList(scalar, svtkm::TypeListTagFieldScalar());

  std::cout << "TypeListTagFieldVec2" << std::endl;
  TypeSet vec2;
  vec2.AddExpected(svtkm::Vec2f_32());
  vec2.AddExpected(svtkm::Vec2f_64());
  TryList(vec2, svtkm::TypeListTagFieldVec2());

  std::cout << "TypeListTagFieldVec3" << std::endl;
  TypeSet vec3;
  vec3.AddExpected(svtkm::Vec3f_32());
  vec3.AddExpected(svtkm::Vec3f_64());
  TryList(vec3, svtkm::TypeListTagFieldVec3());

  std::cout << "TypeListTagFieldVec4" << std::endl;
  TypeSet vec4;
  vec4.AddExpected(svtkm::Vec4f_32());
  vec4.AddExpected(svtkm::Vec4f_64());
  TryList(vec4, svtkm::TypeListTagFieldVec4());

  std::cout << "TypeListTagField" << std::endl;
  TypeSet field;
  field.AddExpected(svtkm::Float32());
  field.AddExpected(svtkm::Float64());
  field.AddExpected(svtkm::Vec2f_32());
  field.AddExpected(svtkm::Vec2f_64());
  field.AddExpected(svtkm::Vec3f_32());
  field.AddExpected(svtkm::Vec3f_64());
  field.AddExpected(svtkm::Vec4f_32());
  field.AddExpected(svtkm::Vec4f_64());
  TryList(field, svtkm::TypeListTagField());

  std::cout << "TypeListTagCommon" << std::endl;
  TypeSet common;
  common.AddExpected(svtkm::Float32());
  common.AddExpected(svtkm::Float64());
  common.AddExpected(svtkm::UInt8());
  common.AddExpected(svtkm::Int32());
  common.AddExpected(svtkm::Int64());
  common.AddExpected(svtkm::Vec3f_32());
  common.AddExpected(svtkm::Vec3f_64());
  TryList(common, svtkm::TypeListTagCommon());

  std::cout << "TypeListTagScalarAll" << std::endl;
  TypeSet scalarsAll;
  scalarsAll.AddExpected(svtkm::Float32());
  scalarsAll.AddExpected(svtkm::Float64());
  scalarsAll.AddExpected(svtkm::Int8());
  scalarsAll.AddExpected(svtkm::UInt8());
  scalarsAll.AddExpected(svtkm::Int16());
  scalarsAll.AddExpected(svtkm::UInt16());
  scalarsAll.AddExpected(svtkm::Int32());
  scalarsAll.AddExpected(svtkm::UInt32());
  scalarsAll.AddExpected(svtkm::Int64());
  scalarsAll.AddExpected(svtkm::UInt64());
  TryList(scalarsAll, svtkm::TypeListTagScalarAll());

  std::cout << "TypeListTagVecCommon" << std::endl;
  TypeSet vecCommon;
  vecCommon.AddExpected(svtkm::Vec2f_32());
  vecCommon.AddExpected(svtkm::Vec2f_64());
  vecCommon.AddExpected(svtkm::Vec2ui_8());
  vecCommon.AddExpected(svtkm::Vec2i_32());
  vecCommon.AddExpected(svtkm::Vec2i_64());
  vecCommon.AddExpected(svtkm::Vec3f_32());
  vecCommon.AddExpected(svtkm::Vec3f_64());
  vecCommon.AddExpected(svtkm::Vec3ui_8());
  vecCommon.AddExpected(svtkm::Vec3i_32());
  vecCommon.AddExpected(svtkm::Vec3i_64());
  vecCommon.AddExpected(svtkm::Vec4f_32());
  vecCommon.AddExpected(svtkm::Vec4f_64());
  vecCommon.AddExpected(svtkm::Vec4ui_8());
  vecCommon.AddExpected(svtkm::Vec4i_32());
  vecCommon.AddExpected(svtkm::Vec4i_64());
  TryList(vecCommon, svtkm::TypeListTagVecCommon());

  std::cout << "TypeListTagVecAll" << std::endl;
  TypeSet vecAll;
  vecAll.AddExpected(svtkm::Vec2f_32());
  vecAll.AddExpected(svtkm::Vec2f_64());
  vecAll.AddExpected(svtkm::Vec2i_8());
  vecAll.AddExpected(svtkm::Vec2i_16());
  vecAll.AddExpected(svtkm::Vec2i_32());
  vecAll.AddExpected(svtkm::Vec2i_64());
  vecAll.AddExpected(svtkm::Vec2ui_8());
  vecAll.AddExpected(svtkm::Vec2ui_16());
  vecAll.AddExpected(svtkm::Vec2ui_32());
  vecAll.AddExpected(svtkm::Vec2ui_64());
  vecAll.AddExpected(svtkm::Vec3f_32());
  vecAll.AddExpected(svtkm::Vec3f_64());
  vecAll.AddExpected(svtkm::Vec3i_8());
  vecAll.AddExpected(svtkm::Vec3i_16());
  vecAll.AddExpected(svtkm::Vec3i_32());
  vecAll.AddExpected(svtkm::Vec3i_64());
  vecAll.AddExpected(svtkm::Vec3ui_8());
  vecAll.AddExpected(svtkm::Vec3ui_16());
  vecAll.AddExpected(svtkm::Vec3ui_32());
  vecAll.AddExpected(svtkm::Vec3ui_64());
  vecAll.AddExpected(svtkm::Vec4f_32());
  vecAll.AddExpected(svtkm::Vec4f_64());
  vecAll.AddExpected(svtkm::Vec4i_8());
  vecAll.AddExpected(svtkm::Vec4i_16());
  vecAll.AddExpected(svtkm::Vec4i_32());
  vecAll.AddExpected(svtkm::Vec4i_64());
  vecAll.AddExpected(svtkm::Vec4ui_8());
  vecAll.AddExpected(svtkm::Vec4ui_16());
  vecAll.AddExpected(svtkm::Vec4ui_32());
  vecAll.AddExpected(svtkm::Vec4ui_64());
  TryList(vecAll, svtkm::TypeListTagVecAll());

  std::cout << "TypeListTagAll" << std::endl;
  TypeSet all;
  all.AddExpected(svtkm::Float32());
  all.AddExpected(svtkm::Float64());
  all.AddExpected(svtkm::Int8());
  all.AddExpected(svtkm::UInt8());
  all.AddExpected(svtkm::Int16());
  all.AddExpected(svtkm::UInt16());
  all.AddExpected(svtkm::Int32());
  all.AddExpected(svtkm::UInt32());
  all.AddExpected(svtkm::Int64());
  all.AddExpected(svtkm::UInt64());
  all.AddExpected(svtkm::Vec2f_32());
  all.AddExpected(svtkm::Vec2f_64());
  all.AddExpected(svtkm::Vec2i_8());
  all.AddExpected(svtkm::Vec2i_16());
  all.AddExpected(svtkm::Vec2i_32());
  all.AddExpected(svtkm::Vec2i_64());
  all.AddExpected(svtkm::Vec2ui_8());
  all.AddExpected(svtkm::Vec2ui_16());
  all.AddExpected(svtkm::Vec2ui_32());
  all.AddExpected(svtkm::Vec2ui_64());
  all.AddExpected(svtkm::Vec3f_32());
  all.AddExpected(svtkm::Vec3f_64());
  all.AddExpected(svtkm::Vec3i_8());
  all.AddExpected(svtkm::Vec3i_16());
  all.AddExpected(svtkm::Vec3i_32());
  all.AddExpected(svtkm::Vec3i_64());
  all.AddExpected(svtkm::Vec3ui_8());
  all.AddExpected(svtkm::Vec3ui_16());
  all.AddExpected(svtkm::Vec3ui_32());
  all.AddExpected(svtkm::Vec3ui_64());
  all.AddExpected(svtkm::Vec4f_32());
  all.AddExpected(svtkm::Vec4f_64());
  all.AddExpected(svtkm::Vec4i_8());
  all.AddExpected(svtkm::Vec4i_16());
  all.AddExpected(svtkm::Vec4i_32());
  all.AddExpected(svtkm::Vec4i_64());
  all.AddExpected(svtkm::Vec4ui_8());
  all.AddExpected(svtkm::Vec4ui_16());
  all.AddExpected(svtkm::Vec4ui_32());
  all.AddExpected(svtkm::Vec4ui_64());
  TryList(all, svtkm::TypeListTagAll());
}

} // anonymous namespace

int UnitTestTypeListTag(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestLists, argc, argv);
}

SVTKM_DEPRECATED_SUPPRESS_END
