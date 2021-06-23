//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/cont/ArrayHandleDecorator.h>

#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayCopy.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleConstant.h>
#include <svtkm/cont/ArrayHandleCounting.h>

#include <svtkm/BinaryOperators.h>

#include <svtkm/cont/testing/Testing.h>

namespace
{

struct DecoratorTests
{
  static constexpr svtkm::Id ARRAY_SIZE = 10;

  // Decorator implementation that demonstrates how to write invertible functors
  // that combine three array handles with complex access logic. The resulting
  // ArrayHandleDecorator can be both read from and written to.
  //
  // Constructs functors that take three portals.
  //
  // The first portal's values are accessed in reverse order.
  // The second portal's values are accessed in normal order.
  // The third portal's values are accessed via ((idx + 3) % size).
  //
  // Functor will return the max of the first two added to the third.
  //
  // InverseFunctor will update the third portal such that the Functor would
  // return the indicated value.
  struct InvertibleDecorImpl
  {

    // The functor used for reading data from the three portals.
    template <typename Portal1Type, typename Portal2Type, typename Portal3Type>
    struct Functor
    {
      using ValueType = typename Portal1Type::ValueType;

      Portal1Type Portal1;
      Portal2Type Portal2;
      Portal3Type Portal3;

      SVTKM_EXEC_CONT ValueType operator()(svtkm::Id idx) const
      {
        const auto idx1 = this->Portal1.GetNumberOfValues() - idx - 1;
        const auto idx2 = idx;
        const auto idx3 = (idx + 3) % this->Portal3.GetNumberOfValues();

        const auto v1 = this->Portal1.Get(idx1);
        const auto v2 = this->Portal2.Get(idx2);
        const auto v3 = this->Portal3.Get(idx3);

        return svtkm::Max(v1, v2) + v3;
      }
    };

    // The functor used for writing. Only Portal3 is written to, the other
    // portals may be read-only.
    template <typename Portal1Type, typename Portal2Type, typename Portal3Type>
    struct InverseFunctor
    {
      using ValueType = typename Portal1Type::ValueType;

      Portal1Type Portal1;
      Portal2Type Portal2;
      Portal3Type Portal3;

      SVTKM_EXEC_CONT void operator()(svtkm::Id idx, const ValueType& vIn) const
      {
        const auto v1 = this->Portal1.Get(this->Portal1.GetNumberOfValues() - idx - 1);
        const auto v2 = this->Portal2.Get(idx);
        const auto vNew = static_cast<ValueType>(vIn - svtkm::Max(v1, v2));
        this->Portal3.Set((idx + 3) % this->Portal3.GetNumberOfValues(), vNew);
      }
    };

    // Factory function that takes 3 portals as input and creates an instance
    // of Functor with them. Variadic template parameters are used here, but are
    // not necessary.
    template <typename... PortalTs>
    Functor<typename std::decay<PortalTs>::type...> CreateFunctor(PortalTs&&... portals) const
    {
      SVTKM_STATIC_ASSERT(sizeof...(PortalTs) == 3);
      return { std::forward<PortalTs>(portals)... };
    }

    // Factory function that takes 3 portals as input and creates an instance
    // of InverseFunctor with them. Variadic template parameters are used here,
    // but are not necessary.
    template <typename... PortalTs>
    InverseFunctor<typename std::decay<PortalTs>::type...> CreateInverseFunctor(
      PortalTs&&... portals) const
    {
      SVTKM_STATIC_ASSERT(sizeof...(PortalTs) == 3);
      return { std::forward<PortalTs>(portals)... };
    }
  };

  // Same as above, but cannot be inverted. The resulting ArrayHandleDecorator
  // will be read-only.
  struct NonInvertibleDecorImpl
  {
    template <typename Portal1Type, typename Portal2Type, typename Portal3Type>
    struct Functor
    {
      using ValueType = typename Portal1Type::ValueType;

      Portal1Type Portal1;
      Portal2Type Portal2;
      Portal3Type Portal3;

      SVTKM_EXEC_CONT ValueType operator()(svtkm::Id idx) const
      {
        const auto v1 = this->Portal1.Get(this->Portal1.GetNumberOfValues() - idx - 1);
        const auto v2 = this->Portal2.Get(idx);
        const auto v3 = this->Portal3.Get((idx + 3) % this->Portal3.GetNumberOfValues());
        return svtkm::Max(v1, v2) + v3;
      }
    };

    template <typename... PortalTs>
    Functor<typename std::decay<PortalTs>::type...> CreateFunctor(PortalTs&&... portals) const
    {
      SVTKM_STATIC_ASSERT(sizeof...(PortalTs) == 3);
      return { std::forward<PortalTs>(portals)... };
    }
  };

  // Decorator implementation that demonstrates how to create functors that
  // hold custom state. Here, the functors have a customizable Operation
  // member.
  //
  // This implementation is used to create a read-only ArrayHandleDecorator
  // that combines the values in two other ArrayHandles using an arbitrary
  // binary operation (e.g. svtkm::Maximum, svtkm::Add, etc).
  template <typename ValueType, typename OperationType>
  struct BinaryOperationDecorImpl
  {
    OperationType Operation;

    // The functor use to read values. Note that it holds extra state in
    // addition to the portals.
    template <typename Portal1Type, typename Portal2Type>
    struct Functor
    {
      Portal1Type Portal1;
      Portal2Type Portal2;
      OperationType Operation;

      SVTKM_EXEC_CONT
      ValueType operator()(svtkm::Id idx) const
      {
        return this->Operation(static_cast<ValueType>(this->Portal1.Get(idx)),
                               static_cast<ValueType>(this->Portal2.Get(idx)));
      }
    };

    // A non-variadic example of a factory function to produce a functor. This
    // is where the extra state is passed into the functor.
    template <typename P1T, typename P2T>
    Functor<P1T, P2T> CreateFunctor(P1T p1, P2T p2) const
    {
      return { p1, p2, this->Operation };
    }
  };

  // Decorator implementation that reverses the ScanExtended operation.
  //
  // The resulting ArrayHandleDecorator will take an array produced by the
  // ScanExtended algorithm and return the original ScanExtended input.
  //
  // Some interesting things about this:
  // - The ArrayHandleDecorator's ValueType will not be the same as the
  //   ScanPortal's ValueType. The Decorator ValueType is determined by the
  //   return type of Functor::operator().
  // - The ScanPortal has more values than the ArrayHandleDecorator. The
  //   number of values the ArrayHandleDecorator should hold is set during
  //   construction and may differ from the arrays it holds.
  template <typename ValueType>
  struct ScanExtendedToNumIndicesDecorImpl
  {
    template <typename ScanPortalType>
    struct Functor
    {
      ScanPortalType ScanPortal;

      SVTKM_EXEC_CONT
      ValueType operator()(svtkm::Id idx) const
      {
        return static_cast<ValueType>(this->ScanPortal.Get(idx + 1) - this->ScanPortal.Get(idx));
      }
    };

    template <typename ScanPortalType>
    Functor<ScanPortalType> CreateFunctor(ScanPortalType portal) const
    {
      return { portal };
    }
  };

  // Decorator implementation that combines two source arrays using the formula
  // `[source1] * 10 + [source2]` and supports resizing.
  template <typename ValueType>
  struct DecompositionDecorImpl
  {
    template <typename Portal1T, typename Portal2T>
    struct Functor
    {
      Portal1T Portal1;
      Portal2T Portal2;

      SVTKM_EXEC_CONT
      ValueType operator()(svtkm::Id idx) const
      {
        return static_cast<ValueType>(this->Portal1.Get(idx) * 10 + this->Portal2.Get(idx));
      }
    };

    template <typename Portal1T, typename Portal2T>
    struct InverseFunctor
    {
      Portal1T Portal1;
      Portal2T Portal2;

      SVTKM_EXEC_CONT
      void operator()(svtkm::Id idx, const ValueType& val) const
      {
        this->Portal1.Set(idx, static_cast<ValueType>(std::floor(val / 10)));
        this->Portal2.Set(idx, static_cast<ValueType>(std::fmod(val, 10)));
      }
    };

    template <typename Portal1T, typename Portal2T>
    SVTKM_CONT Functor<typename std::decay<Portal1T>::type, typename std::decay<Portal2T>::type>
    CreateFunctor(Portal1T&& p1, Portal2T&& p2) const
    {
      return { std::forward<Portal1T>(p1), std::forward<Portal2T>(p2) };
    }

    template <typename Portal1T, typename Portal2T>
    SVTKM_CONT
      InverseFunctor<typename std::decay<Portal1T>::type, typename std::decay<Portal2T>::type>
      CreateInverseFunctor(Portal1T&& p1, Portal2T&& p2) const
    {
      return { std::forward<Portal1T>(p1), std::forward<Portal2T>(p2) };
    }

    // Resize methods:
    template <typename Array1T, typename Array2T>
    SVTKM_CONT void AllocateSourceArrays(svtkm::Id numVals, Array1T&& array1, Array2T&& array2) const
    {
      array1.Allocate(numVals);
      array2.Allocate(numVals);
    }

    template <typename Array1T, typename Array2T>
    SVTKM_CONT void ShrinkSourceArrays(svtkm::Id numVals, Array1T&& array1, Array2T&& array2) const
    {
      array1.Shrink(numVals);
      array2.Shrink(numVals);
    }
  };

  template <typename ValueType>
  void InversionTest() const
  {
    auto ah1 = svtkm::cont::make_ArrayHandleCounting(ValueType{ 0 }, ValueType{ 2 }, ARRAY_SIZE);
    auto ah2 = svtkm::cont::make_ArrayHandleConstant(ValueType{ ARRAY_SIZE }, ARRAY_SIZE);
    svtkm::cont::ArrayHandle<ValueType> ah3;
    svtkm::cont::Algorithm::Fill(ah3, ValueType{ ARRAY_SIZE / 2 }, ARRAY_SIZE);

    auto ah3Const = svtkm::cont::make_ArrayHandleConstant(ValueType{ ARRAY_SIZE / 2 }, ARRAY_SIZE);

    { // Has a writable handle and an invertible functor:
      auto ahInv =
        svtkm::cont::make_ArrayHandleDecorator(ARRAY_SIZE, InvertibleDecorImpl{}, ah1, ah2, ah3);
      SVTKM_TEST_ASSERT(svtkm::cont::internal::IsWritableArrayHandle<decltype(ahInv)>::value);
    }

    { // Has no writable handles and an invertible functor:
      auto ahNInv = svtkm::cont::make_ArrayHandleDecorator(
        ARRAY_SIZE, InvertibleDecorImpl{}, ah1, ah2, ah3Const);
      SVTKM_TEST_ASSERT(!svtkm::cont::internal::IsWritableArrayHandle<decltype(ahNInv)>::value);
    }

    { // Has writable handles, but the functor cannot be inverted:
      auto ahNInv =
        svtkm::cont::make_ArrayHandleDecorator(ARRAY_SIZE, NonInvertibleDecorImpl{}, ah1, ah2, ah3);
      SVTKM_TEST_ASSERT(!svtkm::cont::internal::IsWritableArrayHandle<decltype(ahNInv)>::value);
    }

    { // Has no writable handles and the functor cannot be inverted:
      auto ahNInv = svtkm::cont::make_ArrayHandleDecorator(
        ARRAY_SIZE, NonInvertibleDecorImpl{}, ah1, ah2, ah3Const);
      SVTKM_TEST_ASSERT(!svtkm::cont::internal::IsWritableArrayHandle<decltype(ahNInv)>::value);
    }

    { // Test reading/writing to an invertible handle:
      // Copy ah3 since we'll be modifying it:
      svtkm::cont::ArrayHandle<ValueType> ah3Copy;
      svtkm::cont::ArrayCopy(ah3, ah3Copy);

      auto ahDecor =
        svtkm::cont::make_ArrayHandleDecorator(ARRAY_SIZE, InvertibleDecorImpl{}, ah1, ah2, ah3Copy);

      {
        auto portalDecor = ahDecor.GetPortalConstControl();
        SVTKM_TEST_ASSERT(ahDecor.GetNumberOfValues() == ARRAY_SIZE);
        SVTKM_TEST_ASSERT(portalDecor.GetNumberOfValues() == ARRAY_SIZE);
        SVTKM_TEST_ASSERT(portalDecor.Get(0) == ValueType{ 23 });
        SVTKM_TEST_ASSERT(portalDecor.Get(1) == ValueType{ 21 });
        SVTKM_TEST_ASSERT(portalDecor.Get(2) == ValueType{ 19 });
        SVTKM_TEST_ASSERT(portalDecor.Get(3) == ValueType{ 17 });
        SVTKM_TEST_ASSERT(portalDecor.Get(4) == ValueType{ 15 });
        SVTKM_TEST_ASSERT(portalDecor.Get(5) == ValueType{ 15 });
        SVTKM_TEST_ASSERT(portalDecor.Get(6) == ValueType{ 15 });
        SVTKM_TEST_ASSERT(portalDecor.Get(7) == ValueType{ 15 });
        SVTKM_TEST_ASSERT(portalDecor.Get(8) == ValueType{ 15 });
        SVTKM_TEST_ASSERT(portalDecor.Get(9) == ValueType{ 15 });
      }

      // Copy a constant array into the decorator. This should modify ah3Copy.
      svtkm::cont::ArrayCopy(svtkm::cont::make_ArrayHandleConstant(ValueType{ 25 }, ARRAY_SIZE),
                            ahDecor);

      { // Accessing portal should give all 25s:
        auto portalDecor = ahDecor.GetPortalConstControl();
        SVTKM_TEST_ASSERT(ahDecor.GetNumberOfValues() == ARRAY_SIZE);
        SVTKM_TEST_ASSERT(portalDecor.GetNumberOfValues() == ARRAY_SIZE);
        SVTKM_TEST_ASSERT(portalDecor.Get(0) == ValueType{ 25 });
        SVTKM_TEST_ASSERT(portalDecor.Get(1) == ValueType{ 25 });
        SVTKM_TEST_ASSERT(portalDecor.Get(2) == ValueType{ 25 });
        SVTKM_TEST_ASSERT(portalDecor.Get(3) == ValueType{ 25 });
        SVTKM_TEST_ASSERT(portalDecor.Get(4) == ValueType{ 25 });
        SVTKM_TEST_ASSERT(portalDecor.Get(5) == ValueType{ 25 });
        SVTKM_TEST_ASSERT(portalDecor.Get(6) == ValueType{ 25 });
        SVTKM_TEST_ASSERT(portalDecor.Get(7) == ValueType{ 25 });
        SVTKM_TEST_ASSERT(portalDecor.Get(8) == ValueType{ 25 });
        SVTKM_TEST_ASSERT(portalDecor.Get(9) == ValueType{ 25 });
      }

      { // ah3Copy should have updated values:
        auto portalAH3Copy = ah3Copy.GetPortalConstControl();
        SVTKM_TEST_ASSERT(ahDecor.GetNumberOfValues() == ARRAY_SIZE);
        SVTKM_TEST_ASSERT(portalAH3Copy.GetNumberOfValues() == ARRAY_SIZE);
        SVTKM_TEST_ASSERT(portalAH3Copy.Get(0) == ValueType{ 15 });
        SVTKM_TEST_ASSERT(portalAH3Copy.Get(1) == ValueType{ 15 });
        SVTKM_TEST_ASSERT(portalAH3Copy.Get(2) == ValueType{ 15 });
        SVTKM_TEST_ASSERT(portalAH3Copy.Get(3) == ValueType{ 7 });
        SVTKM_TEST_ASSERT(portalAH3Copy.Get(4) == ValueType{ 9 });
        SVTKM_TEST_ASSERT(portalAH3Copy.Get(5) == ValueType{ 11 });
        SVTKM_TEST_ASSERT(portalAH3Copy.Get(6) == ValueType{ 13 });
        SVTKM_TEST_ASSERT(portalAH3Copy.Get(7) == ValueType{ 15 });
        SVTKM_TEST_ASSERT(portalAH3Copy.Get(8) == ValueType{ 15 });
        SVTKM_TEST_ASSERT(portalAH3Copy.Get(9) == ValueType{ 15 });
      }
    }
  }

  template <typename ValueType, typename OperationType>
  void BinaryOperatorTest() const
  {
    auto ahCount = svtkm::cont::make_ArrayHandleCounting(ValueType{ 0 }, ValueType{ 1 }, ARRAY_SIZE);
    auto ahConst = svtkm::cont::make_ArrayHandleConstant(ValueType{ ARRAY_SIZE / 2 }, ARRAY_SIZE);

    const OperationType op;
    BinaryOperationDecorImpl<ValueType, OperationType> impl{ op };

    auto decorArray = svtkm::cont::make_ArrayHandleDecorator(ARRAY_SIZE, impl, ahCount, ahConst);

    {
      auto decorPortal = decorArray.GetPortalConstControl();
      auto countPortal = ahCount.GetPortalConstControl();
      auto constPortal = ahConst.GetPortalConstControl();
      for (svtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        SVTKM_TEST_ASSERT(decorPortal.Get(i) == op(countPortal.Get(i), constPortal.Get(i)));
      }
    }

    svtkm::cont::ArrayHandle<ValueType> copiedInExec;
    svtkm::cont::ArrayCopy(decorArray, copiedInExec);
    {
      auto copiedPortal = copiedInExec.GetPortalConstControl();
      auto countPortal = ahCount.GetPortalConstControl();
      auto constPortal = ahConst.GetPortalConstControl();
      for (svtkm::Id i = 0; i < ARRAY_SIZE; ++i)
      {
        SVTKM_TEST_ASSERT(copiedPortal.Get(i) == op(countPortal.Get(i), constPortal.Get(i)));
      }
    }
  }


  template <typename ValueType>
  void ScanExtendedToNumIndicesTest() const
  {
    auto numIndicesOrig =
      svtkm::cont::make_ArrayHandleCounting(ValueType{ 0 }, ValueType{ 1 }, ARRAY_SIZE);
    svtkm::cont::ArrayHandle<svtkm::Id> scan;
    svtkm::cont::Algorithm::ScanExtended(svtkm::cont::make_ArrayHandleCast<svtkm::Id>(numIndicesOrig),
                                        scan);

    // Some interesting things to notice:
    // - `numIndicesDecor` will have `ARRAY_SIZE` entries, while `scan` has
    //   `ARRAY_SIZE + 1`.
    // - `numIndicesDecor` uses the current function scope `ValueType`, since
    //   that is what the functor from the implementation class returns. `scan`
    //   uses `svtkm::Id`.
    auto numIndicesDecor = svtkm::cont::make_ArrayHandleDecorator(
      ARRAY_SIZE, ScanExtendedToNumIndicesDecorImpl<ValueType>{}, scan);

    {
      auto origPortal = numIndicesOrig.GetPortalConstControl();
      auto decorPortal = numIndicesDecor.GetPortalConstControl();
      SVTKM_STATIC_ASSERT(SVTKM_PASS_COMMAS(
        std::is_same<decltype(origPortal.Get(0)), decltype(decorPortal.Get(0))>::value));
      SVTKM_TEST_ASSERT(origPortal.GetNumberOfValues() == decorPortal.GetNumberOfValues());
      for (svtkm::Id i = 0; i < origPortal.GetNumberOfValues(); ++i)
      {
        SVTKM_TEST_ASSERT(origPortal.Get(i) == decorPortal.Get(i));
      }
    }
  }

  template <typename ValueType>
  void DecompositionTest() const
  {
    svtkm::cont::ArrayHandle<ValueType> a1;
    svtkm::cont::ArrayHandle<ValueType> a2;
    auto decor =
      svtkm::cont::make_ArrayHandleDecorator(0, DecompositionDecorImpl<ValueType>{}, a1, a2);

    SVTKM_TEST_ASSERT(decor.GetNumberOfValues() == 0);

    decor.Allocate(5);
    SVTKM_TEST_ASSERT(decor.GetNumberOfValues() == 5);
    {
      auto decorPortal = decor.GetPortalControl();
      decorPortal.Set(0, 13);
      decorPortal.Set(1, 8);
      decorPortal.Set(2, 43);
      decorPortal.Set(3, 92);
      decorPortal.Set(4, 117);
    }

    SVTKM_TEST_ASSERT(a1.GetNumberOfValues() == 5);
    {
      auto a1Portal = a1.GetPortalConstControl();
      SVTKM_TEST_ASSERT(test_equal(a1Portal.Get(0), 1));
      SVTKM_TEST_ASSERT(test_equal(a1Portal.Get(1), 0));
      SVTKM_TEST_ASSERT(test_equal(a1Portal.Get(2), 4));
      SVTKM_TEST_ASSERT(test_equal(a1Portal.Get(3), 9));
      SVTKM_TEST_ASSERT(test_equal(a1Portal.Get(4), 11));
    }

    SVTKM_TEST_ASSERT(a2.GetNumberOfValues() == 5);
    {
      auto a2Portal = a2.GetPortalConstControl();
      SVTKM_TEST_ASSERT(test_equal(a2Portal.Get(0), 3));
      SVTKM_TEST_ASSERT(test_equal(a2Portal.Get(1), 8));
      SVTKM_TEST_ASSERT(test_equal(a2Portal.Get(2), 3));
      SVTKM_TEST_ASSERT(test_equal(a2Portal.Get(3), 2));
      SVTKM_TEST_ASSERT(test_equal(a2Portal.Get(4), 7));
    }

    decor.Shrink(3);
    SVTKM_TEST_ASSERT(decor.GetNumberOfValues() == 3);
    {
      auto decorPortal = decor.GetPortalConstControl();
      SVTKM_TEST_ASSERT(test_equal(decorPortal.Get(0), 13));
      SVTKM_TEST_ASSERT(test_equal(decorPortal.Get(1), 8));
      SVTKM_TEST_ASSERT(test_equal(decorPortal.Get(2), 43));
    }

    SVTKM_TEST_ASSERT(a1.GetNumberOfValues() == 3);
    {
      auto a1Portal = a1.GetPortalConstControl();
      SVTKM_TEST_ASSERT(test_equal(a1Portal.Get(0), 1));
      SVTKM_TEST_ASSERT(test_equal(a1Portal.Get(1), 0));
      SVTKM_TEST_ASSERT(test_equal(a1Portal.Get(2), 4));
    }

    SVTKM_TEST_ASSERT(a2.GetNumberOfValues() == 3);
    {
      auto a2Portal = a2.GetPortalConstControl();
      SVTKM_TEST_ASSERT(test_equal(a2Portal.Get(0), 3));
      SVTKM_TEST_ASSERT(test_equal(a2Portal.Get(1), 8));
      SVTKM_TEST_ASSERT(test_equal(a2Portal.Get(2), 3));
    }
  }

  template <typename ValueType>
  void operator()(const ValueType) const
  {
    this->InversionTest<ValueType>();

    this->BinaryOperatorTest<ValueType, svtkm::Maximum>();
    this->BinaryOperatorTest<ValueType, svtkm::Minimum>();
    this->BinaryOperatorTest<ValueType, svtkm::Add>();
    this->BinaryOperatorTest<ValueType, svtkm::Subtract>();
    this->BinaryOperatorTest<ValueType, svtkm::Multiply>();

    this->ScanExtendedToNumIndicesTest<ValueType>();

    this->DecompositionTest<ValueType>();
  }
};

// ArrayHandleDecorator that implements AllocateSourceArrays and ShrinkSourceArrays, thus allowing
// it to be resized.
struct ResizableDecorImpl
{
  // We don't actually read/write from this, so use a dummy functor:
  struct Functor
  {
    SVTKM_EXEC_CONT svtkm::Id operator()(svtkm::Id) const { return 0; }
  };

  template <typename... PortalTs>
  SVTKM_CONT Functor CreateFunctor(PortalTs...) const
  {
    return Functor{};
  }

  template <typename Array1T, typename Array2T>
  void ShrinkSourceArrays(svtkm::Id newSize, Array1T& a1, Array2T& a2) const
  {
    SVTKM_IS_ARRAY_HANDLE(Array1T);
    SVTKM_IS_ARRAY_HANDLE(Array2T);

    // Resize each to 2*newSize:
    a1.Shrink(2 * newSize);
    a2.Shrink(2 * newSize);
  }

  template <typename Array1T, typename Array2T>
  void AllocateSourceArrays(svtkm::Id newSize, Array1T& a1, Array2T& a2) const
  {
    SVTKM_IS_ARRAY_HANDLE(Array1T);
    SVTKM_IS_ARRAY_HANDLE(Array2T);

    // Resize each to 3*newSize:
    a1.Allocate(3 * newSize);
    a2.Allocate(3 * newSize);
  }
};

// ArrayHandleDecorator that implements AllocateSourceArrays and ShrinkSourceArrays, thus allowing
// it to be resized.
struct NonResizableDecorImpl
{
  // We don't actually read/write from this, so use a dummy functor:
  struct Functor
  {
    SVTKM_EXEC_CONT svtkm::Id operator()(svtkm::Id) const { return 0; }
  };

  template <typename... PortalTs>
  SVTKM_CONT Functor CreateFunctor(PortalTs...) const
  {
    return Functor{};
  }
};

void ResizeTest()
{
  {
    svtkm::cont::ArrayHandle<svtkm::Id> a1;
    svtkm::cont::ArrayHandle<svtkm::Id> a2;
    ResizableDecorImpl impl;

    auto decor = svtkm::cont::make_ArrayHandleDecorator(5, impl, a1, a2);
    SVTKM_TEST_ASSERT(decor.GetNumberOfValues() == 5);

    decor.Allocate(10); // Should allocate a1&a2 to have 30 values:
    SVTKM_TEST_ASSERT(a1.GetNumberOfValues() == 30);
    SVTKM_TEST_ASSERT(a2.GetNumberOfValues() == 30);
    SVTKM_TEST_ASSERT(decor.GetNumberOfValues() == 10);
    decor.Shrink(3); // Should resize a1&a2 to have 6 values:
    SVTKM_TEST_ASSERT(a1.GetNumberOfValues() == 6);
    SVTKM_TEST_ASSERT(a2.GetNumberOfValues() == 6);
    SVTKM_TEST_ASSERT(decor.GetNumberOfValues() == 3);
  }

  {
    svtkm::cont::ArrayHandle<svtkm::Id> a1;
    a1.Allocate(20);
    svtkm::cont::ArrayHandle<svtkm::Id> a2;
    a2.Allocate(20);
    NonResizableDecorImpl impl;

    auto decor = svtkm::cont::make_ArrayHandleDecorator(5, impl, a1, a2);
    SVTKM_TEST_ASSERT(decor.GetNumberOfValues() == 5);

    // Allocate and Shrink should throw an ErrorBadType:
    bool threw = false;
    try
    {
      decor.Allocate(10);
    }
    catch (svtkm::cont::ErrorBadType& e)
    {
      std::cerr << "Caught expected exception: " << e.what() << "\n";
      threw = true;
    }
    SVTKM_TEST_ASSERT(threw, "Allocate did not throw as expected.");
    SVTKM_TEST_ASSERT(decor.GetNumberOfValues() == 5);

    threw = false;
    try
    {
      decor.Shrink(3);
    }
    catch (svtkm::cont::ErrorBadType& e)
    {
      std::cerr << "Caught expected exception: " << e.what() << "\n";
      threw = true;
    }
    SVTKM_TEST_ASSERT(threw, "Allocate did not throw as expected.");
    SVTKM_TEST_ASSERT(decor.GetNumberOfValues() == 5);
  }
}

void TestArrayHandleDecorator()
{
  svtkm::testing::Testing::TryTypes(DecoratorTests{}, svtkm::TypeListScalarAll{});

  ResizeTest();
}

} // anonymous namespace

int UnitTestArrayHandleDecorator(int argc, char* argv[])
{
  return svtkm::cont::testing::Testing::Run(TestArrayHandleDecorator, argc, argv);
}
