//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#include <svtkm/cont/ArrayHandleCompositeVector.h>

#include <svtkm/VecTraits.h>

#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleExtractComponent.h>
#include <svtkm/cont/ArrayHandleIndex.h>
#include <svtkm/cont/StorageBasic.h>
#include <svtkm/cont/serial/DeviceAdapterSerial.h>

#include <svtkm/cont/testing/Testing.h>

#include <vector>

namespace
{

const svtkm::Id ARRAY_SIZE = 10;

using StorageTag = svtkm::cont::StorageTagBasic;

svtkm::FloatDefault TestValue3Ids(svtkm::Id index, svtkm::IdComponent inComponentIndex, int inArrayId)
{
  return (svtkm::FloatDefault(index) + 0.1f * svtkm::FloatDefault(inComponentIndex) +
          0.01f * svtkm::FloatDefault(inArrayId));
}

template <typename ValueType>
svtkm::cont::ArrayHandle<ValueType, StorageTag> MakeInputArray(int arrayId)
{
  using VTraits = svtkm::VecTraits<ValueType>;

  // Create a buffer with valid test values.
  ValueType buffer[ARRAY_SIZE];
  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    for (svtkm::IdComponent componentIndex = 0; componentIndex < VTraits::NUM_COMPONENTS;
         componentIndex++)
    {
      VTraits::SetComponent(
        buffer[index], componentIndex, TestValue3Ids(index, componentIndex, arrayId));
    }
  }

  // Make an array handle that points to this buffer.
  using ArrayHandleType = svtkm::cont::ArrayHandle<ValueType, StorageTag>;
  ArrayHandleType bufferHandle = svtkm::cont::make_ArrayHandle(buffer, ARRAY_SIZE);

  // When this function returns, the array is going to go out of scope, which
  // will invalidate the array handle we just created. So copy to a new buffer
  // that will stick around after we return.
  ArrayHandleType copyHandle;
  svtkm::cont::ArrayCopy(bufferHandle, copyHandle);

  return copyHandle;
}

template <typename ValueType, typename C>
void CheckArray(const svtkm::cont::ArrayHandle<ValueType, C>& outArray,
                const svtkm::IdComponent* inComponents,
                const int* inArrayIds)
{
  // ArrayHandleCompositeVector currently does not implement the ability to
  // get to values on the control side, so copy to an array that is accessible.
  using ArrayHandleType = svtkm::cont::ArrayHandle<ValueType, StorageTag>;
  ArrayHandleType arrayCopy;
  svtkm::cont::ArrayCopy(outArray, arrayCopy);

  typename ArrayHandleType::PortalConstControl portal = arrayCopy.GetPortalConstControl();
  using VTraits = svtkm::VecTraits<ValueType>;
  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    ValueType retreivedValue = portal.Get(index);
    for (svtkm::IdComponent componentIndex = 0; componentIndex < VTraits::NUM_COMPONENTS;
         componentIndex++)
    {
      svtkm::FloatDefault retrievedComponent = VTraits::GetComponent(retreivedValue, componentIndex);
      svtkm::FloatDefault expectedComponent =
        TestValue3Ids(index, inComponents[componentIndex], inArrayIds[componentIndex]);
      SVTKM_TEST_ASSERT(retrievedComponent == expectedComponent, "Got bad value.");
    }
  }
}

template <svtkm::IdComponent inComponents>
void TryScalarArray()
{
  std::cout << "Creating a scalar array from one of " << inComponents << " components."
            << std::endl;

  using InValueType = svtkm::Vec<svtkm::FloatDefault, inComponents>;
  using InArrayType = svtkm::cont::ArrayHandle<InValueType, StorageTag>;
  int inArrayId = 0;
  InArrayType inArray = MakeInputArray<InValueType>(inArrayId);

  for (svtkm::IdComponent inComponentIndex = 0; inComponentIndex < inComponents; inComponentIndex++)
  {
    auto c1 = svtkm::cont::make_ArrayHandleExtractComponent(inArray, inComponentIndex);
    auto composite = svtkm::cont::make_ArrayHandleCompositeVector(c1);
    CheckArray(composite, &inComponentIndex, &inArrayId);
  }
}

template <typename T1, typename T2, typename T3, typename T4>
void TryVector4(svtkm::cont::ArrayHandle<T1, StorageTag> array1,
                svtkm::cont::ArrayHandle<T2, StorageTag> array2,
                svtkm::cont::ArrayHandle<T3, StorageTag> array3,
                svtkm::cont::ArrayHandle<T4, StorageTag> array4)
{
  int arrayIds[4] = { 0, 1, 2, 3 };
  svtkm::IdComponent inComponents[4];

  for (inComponents[0] = 0; inComponents[0] < svtkm::VecTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    auto c1 = svtkm::cont::make_ArrayHandleExtractComponent(array1, inComponents[0]);
    for (inComponents[1] = 0; inComponents[1] < svtkm::VecTraits<T2>::NUM_COMPONENTS;
         inComponents[1]++)
    {
      auto c2 = svtkm::cont::make_ArrayHandleExtractComponent(array2, inComponents[1]);
      for (inComponents[2] = 0; inComponents[2] < svtkm::VecTraits<T3>::NUM_COMPONENTS;
           inComponents[2]++)
      {
        auto c3 = svtkm::cont::make_ArrayHandleExtractComponent(array3, inComponents[2]);
        for (inComponents[3] = 0; inComponents[3] < svtkm::VecTraits<T4>::NUM_COMPONENTS;
             inComponents[3]++)
        {
          auto c4 = svtkm::cont::make_ArrayHandleExtractComponent(array4, inComponents[3]);
          CheckArray(
            svtkm::cont::make_ArrayHandleCompositeVector(c1, c2, c3, c4), inComponents, arrayIds);
        }
      }
    }
  }
}

template <typename T1, typename T2, typename T3>
void TryVector3(svtkm::cont::ArrayHandle<T1, StorageTag> array1,
                svtkm::cont::ArrayHandle<T2, StorageTag> array2,
                svtkm::cont::ArrayHandle<T3, StorageTag> array3)
{
  int arrayIds[3] = { 0, 1, 2 };
  svtkm::IdComponent inComponents[3];

  for (inComponents[0] = 0; inComponents[0] < svtkm::VecTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    auto c1 = svtkm::cont::make_ArrayHandleExtractComponent(array1, inComponents[0]);
    for (inComponents[1] = 0; inComponents[1] < svtkm::VecTraits<T2>::NUM_COMPONENTS;
         inComponents[1]++)
    {
      auto c2 = svtkm::cont::make_ArrayHandleExtractComponent(array2, inComponents[1]);
      for (inComponents[2] = 0; inComponents[2] < svtkm::VecTraits<T3>::NUM_COMPONENTS;
           inComponents[2]++)
      {
        auto c3 = svtkm::cont::make_ArrayHandleExtractComponent(array3, inComponents[2]);
        CheckArray(svtkm::cont::make_ArrayHandleCompositeVector(c1, c2, c3), inComponents, arrayIds);
      }
    }
  }

  std::cout << "        Fourth component from Scalar." << std::endl;
  TryVector4(array1, array2, array3, MakeInputArray<svtkm::FloatDefault>(3));
  std::cout << "        Fourth component from Vector4." << std::endl;
  TryVector4(array1, array2, array3, MakeInputArray<svtkm::Vec4f>(3));
}

template <typename T1, typename T2>
void TryVector2(svtkm::cont::ArrayHandle<T1, StorageTag> array1,
                svtkm::cont::ArrayHandle<T2, StorageTag> array2)
{
  int arrayIds[2] = { 0, 1 };
  svtkm::IdComponent inComponents[2];

  for (inComponents[0] = 0; inComponents[0] < svtkm::VecTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    auto c1 = svtkm::cont::make_ArrayHandleExtractComponent(array1, inComponents[0]);
    for (inComponents[1] = 0; inComponents[1] < svtkm::VecTraits<T2>::NUM_COMPONENTS;
         inComponents[1]++)
    {
      auto c2 = svtkm::cont::make_ArrayHandleExtractComponent(array2, inComponents[1]);
      CheckArray(svtkm::cont::make_ArrayHandleCompositeVector(c1, c2), inComponents, arrayIds);
    }
  }

  std::cout << "      Third component from Scalar." << std::endl;
  TryVector3(array1, array2, MakeInputArray<svtkm::FloatDefault>(2));
  std::cout << "      Third component from Vector2." << std::endl;
  TryVector3(array1, array2, MakeInputArray<svtkm::Vec2f>(2));
}

template <typename T1>
void TryVector1(svtkm::cont::ArrayHandle<T1, StorageTag> array1)
{
  int arrayIds[1] = { 0 };
  svtkm::IdComponent inComponents[1];

  for (inComponents[0] = 0; inComponents[0] < svtkm::VecTraits<T1>::NUM_COMPONENTS;
       inComponents[0]++)
  {
    auto testArray = svtkm::cont::make_ArrayHandleExtractComponent(array1, inComponents[0]);
    CheckArray(svtkm::cont::make_ArrayHandleCompositeVector(testArray), inComponents, arrayIds);
  }

  std::cout << "    Second component from Scalar." << std::endl;
  TryVector2(array1, MakeInputArray<svtkm::FloatDefault>(1));
  std::cout << "    Second component from Vector4." << std::endl;
  TryVector2(array1, MakeInputArray<svtkm::Vec4f>(1));
}

void TryVector()
{
  std::cout << "Trying many permutations of composite vectors." << std::endl;

  std::cout << "  First component from Scalar." << std::endl;
  TryVector1(MakeInputArray<svtkm::FloatDefault>(0));
  std::cout << "  First component from Vector3." << std::endl;
  TryVector1(MakeInputArray<svtkm::Vec3f>(0));
}

void TrySpecialArrays()
{
  std::cout << "Trying special arrays." << std::endl;

  using ArrayType1 = svtkm::cont::ArrayHandleIndex;
  ArrayType1 array1(ARRAY_SIZE);

  using ArrayType2 = svtkm::cont::ArrayHandleConstant<svtkm::Id>;
  ArrayType2 array2(295, ARRAY_SIZE);

  auto compositeArray = svtkm::cont::make_ArrayHandleCompositeVector(array1, array2);

  svtkm::cont::printSummary_ArrayHandle(compositeArray, std::cout);
  std::cout << std::endl;

  SVTKM_TEST_ASSERT(compositeArray.GetNumberOfValues() == ARRAY_SIZE, "Wrong array size.");

  auto compositePortal = compositeArray.GetPortalConstControl();
  for (svtkm::Id index = 0; index < ARRAY_SIZE; index++)
  {
    SVTKM_TEST_ASSERT(test_equal(compositePortal.Get(index), svtkm::Id2(index, 295)), "Bad value.");
  }
}

void TestBadArrayLengths()
{
  std::cout << "Checking behavior when size of input arrays do not agree." << std::endl;

  using InArrayType = svtkm::cont::ArrayHandle<svtkm::FloatDefault, StorageTag>;
  InArrayType longInArray = MakeInputArray<svtkm::FloatDefault>(0);
  InArrayType shortInArray = MakeInputArray<svtkm::FloatDefault>(1);
  shortInArray.Shrink(ARRAY_SIZE / 2);

  try
  {
    svtkm::cont::make_ArrayHandleCompositeVector(longInArray, shortInArray);
    SVTKM_TEST_FAIL("Did not get exception like expected.");
  }
  catch (svtkm::cont::ErrorBadValue& error)
  {
    std::cout << "Got expected error: " << std::endl << error.GetMessage() << std::endl;
  }
}

void TestCompositeVector()
{
  TryScalarArray<2>();
  TryScalarArray<3>();
  TryScalarArray<4>();

  TryVector();

  TrySpecialArrays();

  TestBadArrayLengths();
}

} // anonymous namespace

int UnitTestArrayHandleCompositeVector(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestCompositeVector, argc, argv);
}
