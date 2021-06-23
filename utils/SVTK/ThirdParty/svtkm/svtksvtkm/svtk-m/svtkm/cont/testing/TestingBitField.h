//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_cont_testing_TestingBitFields_h
#define svtk_m_cont_testing_TestingBitFields_h

#include <svtkm/cont/ArrayHandleBitField.h>
#include <svtkm/cont/ArrayHandleCounting.h>
#include <svtkm/cont/BitField.h>
#include <svtkm/cont/DeviceAdapterAlgorithm.h>
#include <svtkm/cont/Invoker.h>
#include <svtkm/cont/RuntimeDeviceTracker.h>

#include <svtkm/cont/testing/Testing.h>

#include <svtkm/exec/FunctorBase.h>

#include <cstdio>

#define DEVICE_ASSERT_MSG(cond, message)                                                           \
  do                                                                                               \
  {                                                                                                \
    if (!(cond))                                                                                   \
    {                                                                                              \
      printf("Testing assert failed at %s:%d\n\t- Condition: %s\n\t- Subtest: %s\n",               \
             __FILE__,                                                                             \
             __LINE__,                                                                             \
             #cond,                                                                                \
             message);                                                                             \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

#define DEVICE_ASSERT(cond)                                                                        \
  do                                                                                               \
  {                                                                                                \
    if (!(cond))                                                                                   \
    {                                                                                              \
      printf("Testing assert failed at %s:%d\n\t- Condition: %s\n", __FILE__, __LINE__, #cond);    \
      return false;                                                                                \
    }                                                                                              \
  } while (false)

// Test with some trailing bits in partial last word:
#define NUM_BITS                                                                                   \
  svtkm::Id { 7681 }

using svtkm::cont::BitField;

namespace svtkm
{
namespace cont
{
namespace testing
{

// Takes an ArrayHandleBitField as the boolean condition field
class ConditionalMergeWorklet : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(FieldIn cond, FieldIn trueVals, FieldIn falseVals, FieldOut result);
  using ExecutionSignature = _4(_1, _2, _3);

  template <typename T>
  SVTKM_EXEC T operator()(bool cond, const T& trueVal, const T& falseVal) const
  {
    return cond ? trueVal : falseVal;
  }
};

// Takes a BitFieldInOut as the condition information, and reverses
// the bits in place after performing the merge.
class ConditionalMergeWorklet2 : public svtkm::worklet::WorkletMapField
{
public:
  using ControlSignature = void(BitFieldInOut bits,
                                FieldIn trueVals,
                                FieldIn falseVal,
                                FieldOut result);
  using ExecutionSignature = _4(InputIndex, _1, _2, _3);
  using InputDomain = _2;

  template <typename BitPortal, typename T>
  SVTKM_EXEC T
  operator()(const svtkm::Id i, BitPortal& bits, const T& trueVal, const T& falseVal) const
  {
    return bits.XorBitAtomic(i, true) ? trueVal : falseVal;
  }
};

/// This class has a single static member, Run, that runs all tests with the
/// given DeviceAdapter.
template <class DeviceAdapterTag>
struct TestingBitField
{
  using Algo = svtkm::cont::DeviceAdapterAlgorithm<DeviceAdapterTag>;
  using AtomicInterface = svtkm::cont::internal::AtomicInterfaceExecution<DeviceAdapterTag>;
  using Traits = svtkm::cont::detail::BitFieldTraits;
  using WordTypes = typename AtomicInterface::WordTypes;
  using WordTypesControl = svtkm::cont::internal::AtomicInterfaceControl::WordTypes;

  SVTKM_EXEC_CONT
  static bool RandomBitFromIndex(svtkm::Id idx) noexcept
  {
    // Some random operations that will give a pseudorandom stream of bits:
    auto m = idx + (idx * 2) - (idx / 3) + (idx * 5 / 7) - (idx * 11 / 13);
    return (m % 2) == 1;
  }

  template <typename WordType>
  SVTKM_EXEC_CONT static WordType RandomWordFromIndex(svtkm::Id idx) noexcept
  {
    svtkm::UInt64 m = static_cast<svtkm::UInt64>(idx * (NUM_BITS - 1) + (idx + 1) * NUM_BITS);
    m ^= m << 3;
    m ^= m << 7;
    m ^= m << 15;
    m ^= m << 31;
    m = (m << 32) | (m >> 32);

    const size_t mBits = 64;
    const size_t wordBits = sizeof(WordType) * CHAR_BIT;

    const WordType highWord = static_cast<WordType>(m >> (mBits - wordBits));
    return highWord;
  }

  SVTKM_CONT
  static BitField RandomBitField(svtkm::Id numBits = NUM_BITS)
  {
    BitField field;
    field.Allocate(numBits);
    auto portal = field.GetPortalControl();
    for (svtkm::Id i = 0; i < numBits; ++i)
    {
      portal.SetBit(i, RandomBitFromIndex(i));
    }

    return field;
  }

  SVTKM_CONT
  static void TestBlockAllocation()
  {
    BitField field;
    field.Allocate(NUM_BITS);

    // NumBits should be rounded up to the nearest block of bytes, as defined in
    // the traits:
    const svtkm::Id bytesInFieldData =
      field.GetData().GetNumberOfValues() * static_cast<svtkm::Id>(sizeof(svtkm::WordTypeDefault));

    const svtkm::Id blockSize = svtkm::cont::detail::BitFieldTraits::BlockSize;
    const svtkm::Id numBytes = (NUM_BITS + CHAR_BIT - 1) / CHAR_BIT;
    const svtkm::Id numBlocks = (numBytes + blockSize - 1) / blockSize;
    const svtkm::Id expectedBytes = numBlocks * blockSize;

    SVTKM_TEST_ASSERT(bytesInFieldData == expectedBytes,
                     "The BitField allocation does not round up to the nearest "
                     "block. This can cause access-by-word to read/write invalid "
                     "memory.");
  }

  template <typename PortalType, typename PortalConstType>
  SVTKM_EXEC_CONT static bool HelpTestBit(svtkm::Id i, PortalType portal, PortalConstType portalConst)
  {
    const auto origBit = RandomBitFromIndex(i);
    auto bit = origBit;

    const auto mod = RandomBitFromIndex(i + NUM_BITS);

    auto testValues = [&](const char* op) -> bool {
      auto expected = bit;
      auto result = portal.GetBitAtomic(i);
      auto resultConst = portalConst.GetBitAtomic(i);
      DEVICE_ASSERT_MSG(result == expected, op);
      DEVICE_ASSERT_MSG(resultConst == expected, op);

      // Reset:
      bit = origBit;
      portal.SetBitAtomic(i, bit);
      return true;
    };

    portal.SetBit(i, bit);
    DEVICE_ASSERT(testValues("SetBit"));

    bit = mod;
    portal.SetBitAtomic(i, mod);
    DEVICE_ASSERT(testValues("SetBitAtomic"));

    bit = !bit;
    portal.NotBitAtomic(i);
    DEVICE_ASSERT(testValues("NotBitAtomic"));

    bit = bit && mod;
    portal.AndBitAtomic(i, mod);
    DEVICE_ASSERT(testValues("AndBitAtomic"));

    bit = bit || mod;
    portal.OrBitAtomic(i, mod);
    DEVICE_ASSERT(testValues("OrBitAtomic"));

    bit = bit != mod;
    portal.XorBitAtomic(i, mod);
    DEVICE_ASSERT(testValues("XorBitAtomic"));

    const auto notBit = !bit;
    bool casResult = portal.CompareAndSwapBitAtomic(i, bit, notBit);
    DEVICE_ASSERT(casResult == bit);
    DEVICE_ASSERT(portal.GetBit(i) == bit);
    DEVICE_ASSERT(portalConst.GetBit(i) == bit);
    casResult = portal.CompareAndSwapBitAtomic(i, notBit, bit);
    DEVICE_ASSERT(casResult == bit);
    DEVICE_ASSERT(portal.GetBit(i) == notBit);
    DEVICE_ASSERT(portalConst.GetBit(i) == notBit);

    return true;
  }

  template <typename WordType, typename PortalType, typename PortalConstType>
  SVTKM_EXEC_CONT static bool HelpTestWord(svtkm::Id i,
                                          PortalType portal,
                                          PortalConstType portalConst)
  {
    const auto origWord = RandomWordFromIndex<WordType>(i);
    auto word = origWord;

    const auto mod = RandomWordFromIndex<WordType>(i + NUM_BITS);

    auto testValues = [&](const char* op) -> bool {
      auto expected = word;
      auto result = portal.template GetWordAtomic<WordType>(i);
      auto resultConst = portalConst.template GetWordAtomic<WordType>(i);
      DEVICE_ASSERT_MSG(result == expected, op);
      DEVICE_ASSERT_MSG(resultConst == expected, op);

      // Reset:
      word = origWord;
      portal.SetWordAtomic(i, word);

      return true;
    };

    portal.SetWord(i, word);
    DEVICE_ASSERT(testValues("SetWord"));

    word = mod;
    portal.SetWordAtomic(i, mod);
    DEVICE_ASSERT(testValues("SetWordAtomic"));

    // C++ promotes e.g. uint8 to int32 when performing bitwise not. Silence
    // conversion warning and mask unimportant bits:
    word = static_cast<WordType>(~word);
    portal.template NotWordAtomic<WordType>(i);
    DEVICE_ASSERT(testValues("NotWordAtomic"));

    word = word & mod;
    portal.AndWordAtomic(i, mod);
    DEVICE_ASSERT(testValues("AndWordAtomic"));

    word = word | mod;
    portal.OrWordAtomic(i, mod);
    DEVICE_ASSERT(testValues("OrWordAtomic"));

    word = word ^ mod;
    portal.XorWordAtomic(i, mod);
    DEVICE_ASSERT(testValues("XorWordAtomic"));

    const WordType notWord = static_cast<WordType>(~word);
    auto casResult = portal.CompareAndSwapWordAtomic(i, word, notWord);
    DEVICE_ASSERT(casResult == word);
    DEVICE_ASSERT(portal.template GetWord<WordType>(i) == word);
    DEVICE_ASSERT(portalConst.template GetWord<WordType>(i) == word);
    casResult = portal.CompareAndSwapWordAtomic(i, notWord, word);
    DEVICE_ASSERT(casResult == word);
    DEVICE_ASSERT(portal.template GetWord<WordType>(i) == notWord);
    DEVICE_ASSERT(portalConst.template GetWord<WordType>(i) == notWord);

    return true;
  }

  template <typename PortalType, typename PortalConstType>
  struct HelpTestWordOpsControl
  {
    PortalType Portal;
    PortalConstType PortalConst;

    SVTKM_CONT
    HelpTestWordOpsControl(PortalType portal, PortalConstType portalConst)
      : Portal(portal)
      , PortalConst(portalConst)
    {
    }

    template <typename WordType>
    SVTKM_CONT void operator()(WordType)
    {
      const auto numWords = this->Portal.template GetNumberOfWords<WordType>();
      SVTKM_TEST_ASSERT(numWords == this->PortalConst.template GetNumberOfWords<WordType>());
      for (svtkm::Id i = 0; i < numWords; ++i)
      {
        SVTKM_TEST_ASSERT(HelpTestWord<WordType>(i, this->Portal, this->PortalConst));
      }
    }
  };

  template <typename Portal, typename PortalConst>
  SVTKM_CONT static void HelpTestPortalsControl(Portal portal, PortalConst portalConst)
  {
    const auto numWords8 = (NUM_BITS + 7) / 8;
    const auto numWords16 = (NUM_BITS + 15) / 16;
    const auto numWords32 = (NUM_BITS + 31) / 32;
    const auto numWords64 = (NUM_BITS + 63) / 64;

    SVTKM_TEST_ASSERT(portal.GetNumberOfBits() == NUM_BITS);
    SVTKM_TEST_ASSERT(portal.template GetNumberOfWords<svtkm::UInt8>() == numWords8);
    SVTKM_TEST_ASSERT(portal.template GetNumberOfWords<svtkm::UInt16>() == numWords16);
    SVTKM_TEST_ASSERT(portal.template GetNumberOfWords<svtkm::UInt32>() == numWords32);
    SVTKM_TEST_ASSERT(portal.template GetNumberOfWords<svtkm::UInt64>() == numWords64);
    SVTKM_TEST_ASSERT(portalConst.GetNumberOfBits() == NUM_BITS);
    SVTKM_TEST_ASSERT(portalConst.template GetNumberOfWords<svtkm::UInt8>() == numWords8);
    SVTKM_TEST_ASSERT(portalConst.template GetNumberOfWords<svtkm::UInt16>() == numWords16);
    SVTKM_TEST_ASSERT(portalConst.template GetNumberOfWords<svtkm::UInt32>() == numWords32);
    SVTKM_TEST_ASSERT(portalConst.template GetNumberOfWords<svtkm::UInt64>() == numWords64);

    for (svtkm::Id i = 0; i < NUM_BITS; ++i)
    {
      HelpTestBit(i, portal, portalConst);
    }

    HelpTestWordOpsControl<Portal, PortalConst> test(portal, portalConst);
    svtkm::ListForEach(test, typename Portal::AtomicInterface::WordTypes{});
  }

  SVTKM_CONT
  static void TestControlPortals()
  {
    auto field = RandomBitField();
    auto portal = field.GetPortalControl();
    auto portalConst = field.GetPortalConstControl();

    HelpTestPortalsControl(portal, portalConst);
  }

  template <typename Portal>
  SVTKM_EXEC_CONT static bool HelpTestPortalSanityExecution(Portal portal)
  {
    const auto numWords8 = (NUM_BITS + 7) / 8;
    const auto numWords16 = (NUM_BITS + 15) / 16;
    const auto numWords32 = (NUM_BITS + 31) / 32;
    const auto numWords64 = (NUM_BITS + 63) / 64;

    DEVICE_ASSERT(portal.GetNumberOfBits() == NUM_BITS);
    DEVICE_ASSERT(portal.template GetNumberOfWords<svtkm::UInt8>() == numWords8);
    DEVICE_ASSERT(portal.template GetNumberOfWords<svtkm::UInt16>() == numWords16);
    DEVICE_ASSERT(portal.template GetNumberOfWords<svtkm::UInt32>() == numWords32);
    DEVICE_ASSERT(portal.template GetNumberOfWords<svtkm::UInt64>() == numWords64);

    return true;
  }

  template <typename WordType, typename PortalType, typename PortalConstType>
  struct HelpTestPortalsExecutionWordsFunctor : svtkm::exec::FunctorBase
  {
    PortalType Portal;
    PortalConstType PortalConst;

    HelpTestPortalsExecutionWordsFunctor(PortalType portal, PortalConstType portalConst)
      : Portal(portal)
      , PortalConst(portalConst)
    {
    }

    SVTKM_EXEC_CONT
    void operator()(svtkm::Id i) const
    {
      if (i == 0)
      {
        if (!HelpTestPortalSanityExecution(this->Portal))
        {
          this->RaiseError("Testing Portal sanity failed.");
          return;
        }
        if (!HelpTestPortalSanityExecution(this->PortalConst))
        {
          this->RaiseError("Testing PortalConst sanity failed.");
          return;
        }
      }

      if (!HelpTestWord<WordType>(i, this->Portal, this->PortalConst))
      {
        this->RaiseError("Testing word operations failed.");
        return;
      }
    }
  };

  template <typename PortalType, typename PortalConstType>
  struct HelpTestPortalsExecutionBitsFunctor : svtkm::exec::FunctorBase
  {
    PortalType Portal;
    PortalConstType PortalConst;

    HelpTestPortalsExecutionBitsFunctor(PortalType portal, PortalConstType portalConst)
      : Portal(portal)
      , PortalConst(portalConst)
    {
    }

    SVTKM_EXEC_CONT
    void operator()(svtkm::Id i) const
    {
      if (!HelpTestBit(i, this->Portal, this->PortalConst))
      {
        this->RaiseError("Testing bit operations failed.");
        return;
      }
    }
  };

  template <typename PortalType, typename PortalConstType>
  struct HelpTestWordOpsExecution
  {
    PortalType Portal;
    PortalConstType PortalConst;

    SVTKM_CONT
    HelpTestWordOpsExecution(PortalType portal, PortalConstType portalConst)
      : Portal(portal)
      , PortalConst(portalConst)
    {
    }

    template <typename WordType>
    SVTKM_CONT void operator()(WordType)
    {
      const auto numWords = this->Portal.template GetNumberOfWords<WordType>();
      SVTKM_TEST_ASSERT(numWords == this->PortalConst.template GetNumberOfWords<WordType>());

      using WordFunctor =
        HelpTestPortalsExecutionWordsFunctor<WordType, PortalType, PortalConstType>;
      WordFunctor test{ this->Portal, this->PortalConst };
      Algo::Schedule(test, numWords);
    }
  };

  template <typename Portal, typename PortalConst>
  SVTKM_CONT static void HelpTestPortalsExecution(Portal portal, PortalConst portalConst)
  {
    HelpTestPortalsExecutionBitsFunctor<Portal, PortalConst> bitTest{ portal, portalConst };
    Algo::Schedule(bitTest, portal.GetNumberOfBits());


    HelpTestWordOpsExecution<Portal, PortalConst> test(portal, portalConst);
    svtkm::ListForEach(test, typename Portal::AtomicInterface::WordTypes{});
  }

  SVTKM_CONT
  static void TestExecutionPortals()
  {
    auto field = RandomBitField();
    auto portal = field.PrepareForInPlace(DeviceAdapterTag{});
    auto portalConst = field.PrepareForInput(DeviceAdapterTag{});

    HelpTestPortalsExecution(portal, portalConst);
  }

  SVTKM_CONT
  static void TestFinalWordMask()
  {
    auto testMask32 = [](svtkm::Id numBits, svtkm::UInt32 expectedMask) {
      svtkm::cont::BitField field;
      field.Allocate(numBits);
      auto mask = field.GetPortalConstControl().GetFinalWordMask<svtkm::UInt32>();

      SVTKM_TEST_ASSERT(expectedMask == mask,
                       "Unexpected mask for BitField size ",
                       numBits,
                       ": Expected 0x",
                       std::hex,
                       expectedMask,
                       " got 0x",
                       mask);
    };

    auto testMask64 = [](svtkm::Id numBits, svtkm::UInt64 expectedMask) {
      svtkm::cont::BitField field;
      field.Allocate(numBits);
      auto mask = field.GetPortalConstControl().GetFinalWordMask<svtkm::UInt64>();

      SVTKM_TEST_ASSERT(expectedMask == mask,
                       "Unexpected mask for BitField size ",
                       numBits,
                       ": Expected 0x",
                       std::hex,
                       expectedMask,
                       " got 0x",
                       mask);
    };

    testMask32(0, 0x00000000);
    testMask32(1, 0x00000001);
    testMask32(2, 0x00000003);
    testMask32(3, 0x00000007);
    testMask32(4, 0x0000000f);
    testMask32(5, 0x0000001f);
    testMask32(8, 0x000000ff);
    testMask32(16, 0x0000ffff);
    testMask32(24, 0x00ffffff);
    testMask32(25, 0x01ffffff);
    testMask32(31, 0x7fffffff);
    testMask32(32, 0xffffffff);
    testMask32(64, 0xffffffff);
    testMask32(128, 0xffffffff);
    testMask32(129, 0x00000001);

    testMask64(0, 0x0000000000000000);
    testMask64(1, 0x0000000000000001);
    testMask64(2, 0x0000000000000003);
    testMask64(3, 0x0000000000000007);
    testMask64(4, 0x000000000000000f);
    testMask64(5, 0x000000000000001f);
    testMask64(8, 0x00000000000000ff);
    testMask64(16, 0x000000000000ffff);
    testMask64(24, 0x0000000000ffffff);
    testMask64(25, 0x0000000001ffffff);
    testMask64(31, 0x000000007fffffff);
    testMask64(32, 0x00000000ffffffff);
    testMask64(40, 0x000000ffffffffff);
    testMask64(48, 0x0000ffffffffffff);
    testMask64(56, 0x00ffffffffffffff);
    testMask64(64, 0xffffffffffffffff);
    testMask64(128, 0xffffffffffffffff);
    testMask64(129, 0x0000000000000001);
  }

  struct ArrayHandleBitFieldChecker : svtkm::exec::FunctorBase
  {
    using PortalType = typename ArrayHandleBitField::ExecutionTypes<DeviceAdapterTag>::Portal;

    PortalType Portal;
    bool InvertReference;

    SVTKM_EXEC_CONT
    ArrayHandleBitFieldChecker(PortalType portal, bool invert)
      : Portal(portal)
      , InvertReference(invert)
    {
    }

    SVTKM_EXEC
    void operator()(svtkm::Id i) const
    {
      const bool ref = this->InvertReference ? !RandomBitFromIndex(i) : RandomBitFromIndex(i);
      if (this->Portal.Get(i) != ref)
      {
        this->RaiseError("Unexpected value from ArrayHandleBitField portal.");
        return;
      }

      // Flip the bit for the next kernel launch, which tests that the bitfield
      // is inverted.
      this->Portal.Set(i, !ref);
    }
  };

  SVTKM_CONT
  static void TestArrayHandleBitField()
  {
    auto handle = svtkm::cont::make_ArrayHandleBitField(RandomBitField());
    const svtkm::Id numBits = handle.GetNumberOfValues();

    SVTKM_TEST_ASSERT(numBits == NUM_BITS,
                     "ArrayHandleBitField returned the wrong number of values. "
                     "Expected: ",
                     NUM_BITS,
                     " got: ",
                     numBits);

    Algo::Schedule(
      ArrayHandleBitFieldChecker{ handle.PrepareForInPlace(DeviceAdapterTag{}), false }, numBits);
    Algo::Schedule(ArrayHandleBitFieldChecker{ handle.PrepareForInPlace(DeviceAdapterTag{}), true },
                   numBits);
  }

  SVTKM_CONT
  static void TestArrayInvokeWorklet()
  {
    auto condArray = svtkm::cont::make_ArrayHandleBitField(RandomBitField());
    auto trueArray = svtkm::cont::make_ArrayHandleCounting<svtkm::Id>(20, 2, NUM_BITS);
    auto falseArray = svtkm::cont::make_ArrayHandleCounting<svtkm::Id>(13, 2, NUM_BITS);
    svtkm::cont::ArrayHandle<svtkm::Id> output;

    svtkm::cont::Invoker invoke;
    invoke(ConditionalMergeWorklet{}, condArray, trueArray, falseArray, output);

    auto condVals = condArray.GetPortalConstControl();
    auto trueVals = trueArray.GetPortalConstControl();
    auto falseVals = falseArray.GetPortalConstControl();
    auto outVals = output.GetPortalConstControl();

    SVTKM_TEST_ASSERT(condVals.GetNumberOfValues() == trueVals.GetNumberOfValues());
    SVTKM_TEST_ASSERT(condVals.GetNumberOfValues() == falseVals.GetNumberOfValues());
    SVTKM_TEST_ASSERT(condVals.GetNumberOfValues() == outVals.GetNumberOfValues());

    for (svtkm::Id i = 0; i < condVals.GetNumberOfValues(); ++i)
    {
      SVTKM_TEST_ASSERT(outVals.Get(i) == (condVals.Get(i) ? trueVals.Get(i) : falseVals.Get(i)));
    }
  }

  SVTKM_CONT
  static void TestArrayInvokeWorklet2()
  {
    auto condBits = RandomBitField();
    auto trueArray = svtkm::cont::make_ArrayHandleCounting<svtkm::Id>(20, 2, NUM_BITS);
    auto falseArray = svtkm::cont::make_ArrayHandleCounting<svtkm::Id>(13, 2, NUM_BITS);
    svtkm::cont::ArrayHandle<svtkm::Id> output;

    svtkm::cont::Invoker invoke;
    invoke(ConditionalMergeWorklet2{}, condBits, trueArray, falseArray, output);

    auto condVals = condBits.GetPortalConstControl();
    auto trueVals = trueArray.GetPortalConstControl();
    auto falseVals = falseArray.GetPortalConstControl();
    auto outVals = output.GetPortalConstControl();

    SVTKM_TEST_ASSERT(condVals.GetNumberOfBits() == trueVals.GetNumberOfValues());
    SVTKM_TEST_ASSERT(condVals.GetNumberOfBits() == falseVals.GetNumberOfValues());
    SVTKM_TEST_ASSERT(condVals.GetNumberOfBits() == outVals.GetNumberOfValues());

    for (svtkm::Id i = 0; i < condVals.GetNumberOfBits(); ++i)
    {
      // The worklet flips the bitfield in place after choosing true/false paths
      SVTKM_TEST_ASSERT(condVals.GetBit(i) == !RandomBitFromIndex(i));
      SVTKM_TEST_ASSERT(outVals.Get(i) ==
                       (!condVals.GetBit(i) ? trueVals.Get(i) : falseVals.Get(i)));
    }
  }

  struct TestRunner
  {
    SVTKM_CONT
    void operator()() const
    {
      TestingBitField::TestBlockAllocation();
      TestingBitField::TestControlPortals();
      TestingBitField::TestExecutionPortals();
      TestingBitField::TestFinalWordMask();
      TestingBitField::TestArrayHandleBitField();
      TestingBitField::TestArrayInvokeWorklet();
      TestingBitField::TestArrayInvokeWorklet2();
    }
  };

public:
  static SVTKM_CONT int Run(int argc, char* argv[])
  {
    svtkm::cont::GetRuntimeDeviceTracker().ForceDevice(DeviceAdapterTag());
    return svtkm::cont::testing::Testing::Run(TestRunner{}, argc, argv);
  }
};
}
}
} // namespace svtkm::cont::testing

#endif // svtk_m_cont_testing_TestingBitFields_h
