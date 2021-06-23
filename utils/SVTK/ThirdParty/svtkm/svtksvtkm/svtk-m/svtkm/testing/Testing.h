//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_testing_Testing_h
#define svtk_m_testing_Testing_h

#include <svtkm/Bitset.h>
#include <svtkm/Bounds.h>
#include <svtkm/CellShape.h>
#include <svtkm/List.h>
#include <svtkm/Math.h>
#include <svtkm/Matrix.h>
#include <svtkm/Pair.h>
#include <svtkm/Range.h>
#include <svtkm/TypeList.h>
#include <svtkm/TypeTraits.h>
#include <svtkm/Types.h>
#include <svtkm/VecTraits.h>

#include <svtkm/cont/Logging.h>

#include <exception>
#include <iostream>
#include <sstream>
#include <string>
#include <type_traits>

#include <math.h>

// Try to enforce using the correct testing version. (Those that include the
// control environment have more possible exceptions.) This is not guaranteed
// to work. To make it more likely, place the Testing.h include last.
#ifdef svtk_m_cont_Error_h
#ifndef svtk_m_cont_testing_Testing_h
#error Use svtkm::cont::testing::Testing instead of svtkm::testing::Testing.
#else
#define SVTKM_TESTING_IN_CONT
#endif
#endif

/// \def SVTKM_STRINGIFY_FIRST(...)
///
/// A utility macro that takes 1 or more arguments and converts it into the C string version
/// of the first argument.

#define SVTKM_STRINGIFY_FIRST(...) SVTKM_EXPAND(SVTK_M_STRINGIFY_FIRST_IMPL(__VA_ARGS__, dummy))
#define SVTK_M_STRINGIFY_FIRST_IMPL(first, ...) #first

/// \def SVTKM_TEST_ASSERT(condition, messages..)
///
/// Asserts a condition for a test to pass. A passing condition is when \a
/// condition resolves to true. If \a condition is false, then the test is
/// aborted and failure is returned. If one or more message arguments are
/// given, they are printed out by concatinating them. If no messages are
/// given, a generic message is given. In any case, the condition that failed
/// is written out.

#define SVTKM_TEST_ASSERT(...)                                                                      \
  ::svtkm::testing::Testing::Assert(                                                                \
    SVTKM_STRINGIFY_FIRST(__VA_ARGS__), __FILE__, __LINE__, __VA_ARGS__)

/// \def SVTKM_TEST_FAIL(messages..)
///
/// Causes a test to fail with the given \a messages. At least one argument must be given.

#define SVTKM_TEST_FAIL(...) ::svtkm::testing::Testing::TestFail(__FILE__, __LINE__, __VA_ARGS__)

namespace svtkm
{
namespace testing
{

// If you get an error about this class definition being incomplete, it means
// that you tried to get the name of a type that is not specified. You can
// either not use that type, not try to get the string name, or add it to the
// list.
template <typename T>
struct TypeName;

#define SVTK_M_BASIC_TYPE(type, name)                                                               \
  template <>                                                                                      \
  struct TypeName<type>                                                                            \
  {                                                                                                \
    static std::string Name() { return #name; }                                                    \
  }

SVTK_M_BASIC_TYPE(svtkm::Float32, F32);
SVTK_M_BASIC_TYPE(svtkm::Float64, F64);
SVTK_M_BASIC_TYPE(svtkm::Int8, I8);
SVTK_M_BASIC_TYPE(svtkm::UInt8, UI8);
SVTK_M_BASIC_TYPE(svtkm::Int16, I16);
SVTK_M_BASIC_TYPE(svtkm::UInt16, UI16);
SVTK_M_BASIC_TYPE(svtkm::Int32, I32);
SVTK_M_BASIC_TYPE(svtkm::UInt32, UI32);
SVTK_M_BASIC_TYPE(svtkm::Int64, I64);
SVTK_M_BASIC_TYPE(svtkm::UInt64, UI64);

// types without svtkm::typedefs:
SVTK_M_BASIC_TYPE(char, char);
SVTK_M_BASIC_TYPE(long, long);
SVTK_M_BASIC_TYPE(unsigned long, unsigned long);

#define SVTK_M_BASIC_TYPE_HELPER(type) SVTK_M_BASIC_TYPE(svtkm::type, type)

// Special containers:
SVTK_M_BASIC_TYPE_HELPER(Bounds);
SVTK_M_BASIC_TYPE_HELPER(Range);

// Special Vec types:
SVTK_M_BASIC_TYPE_HELPER(Vec2f_32);
SVTK_M_BASIC_TYPE_HELPER(Vec2f_64);
SVTK_M_BASIC_TYPE_HELPER(Vec2i_8);
SVTK_M_BASIC_TYPE_HELPER(Vec2i_16);
SVTK_M_BASIC_TYPE_HELPER(Vec2i_32);
SVTK_M_BASIC_TYPE_HELPER(Vec2i_64);
SVTK_M_BASIC_TYPE_HELPER(Vec2ui_8);
SVTK_M_BASIC_TYPE_HELPER(Vec2ui_16);
SVTK_M_BASIC_TYPE_HELPER(Vec2ui_32);
SVTK_M_BASIC_TYPE_HELPER(Vec2ui_64);
SVTK_M_BASIC_TYPE_HELPER(Vec3f_32);
SVTK_M_BASIC_TYPE_HELPER(Vec3f_64);
SVTK_M_BASIC_TYPE_HELPER(Vec3i_8);
SVTK_M_BASIC_TYPE_HELPER(Vec3i_16);
SVTK_M_BASIC_TYPE_HELPER(Vec3i_32);
SVTK_M_BASIC_TYPE_HELPER(Vec3i_64);
SVTK_M_BASIC_TYPE_HELPER(Vec3ui_8);
SVTK_M_BASIC_TYPE_HELPER(Vec3ui_16);
SVTK_M_BASIC_TYPE_HELPER(Vec3ui_32);
SVTK_M_BASIC_TYPE_HELPER(Vec3ui_64);
SVTK_M_BASIC_TYPE_HELPER(Vec4f_32);
SVTK_M_BASIC_TYPE_HELPER(Vec4f_64);
SVTK_M_BASIC_TYPE_HELPER(Vec4i_8);
SVTK_M_BASIC_TYPE_HELPER(Vec4i_16);
SVTK_M_BASIC_TYPE_HELPER(Vec4i_32);
SVTK_M_BASIC_TYPE_HELPER(Vec4i_64);
SVTK_M_BASIC_TYPE_HELPER(Vec4ui_8);
SVTK_M_BASIC_TYPE_HELPER(Vec4ui_16);
SVTK_M_BASIC_TYPE_HELPER(Vec4ui_32);
SVTK_M_BASIC_TYPE_HELPER(Vec4ui_64);

#undef SVTK_M_BASIC_TYPE

template <typename T, svtkm::IdComponent Size>
struct TypeName<svtkm::Vec<T, Size>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "Vec<" << TypeName<T>::Name() << ", " << Size << ">";
    return stream.str();
  }
};

template <typename T, svtkm::IdComponent numRows, svtkm::IdComponent numCols>
struct TypeName<svtkm::Matrix<T, numRows, numCols>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "Matrix<" << TypeName<T>::Name() << ", " << numRows << ", " << numCols << ">";
    return stream.str();
  }
};

template <typename T, typename U>
struct TypeName<svtkm::Pair<T, U>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "Pair<" << TypeName<T>::Name() << ", " << TypeName<U>::Name() << ">";
    return stream.str();
  }
};

template <typename T>
struct TypeName<svtkm::Bitset<T>>
{
  static std::string Name()
  {
    std::stringstream stream;
    stream << "Bitset<" << TypeName<T>::Name() << ">";
    return stream.str();
  }
};

template <typename T0, typename... Ts>
struct TypeName<svtkm::List<T0, Ts...>>
{
  static std::string Name()
  {
    std::initializer_list<std::string> subtypeStrings = { TypeName<Ts>::Name()... };

    std::stringstream stream;
    stream << "List<" << TypeName<T0>::Name();
    for (auto&& subtype : subtypeStrings)
    {
      stream << ", " << subtype;
    }
    stream << ">";
    return stream.str();
  }
};
template <>
struct TypeName<svtkm::ListEmpty>
{
  static std::string Name() { return "ListEmpty"; }
};
template <>
struct TypeName<svtkm::ListUniversal>
{
  static std::string Name() { return "ListUniversal"; }
};

namespace detail
{

template <svtkm::IdComponent cellShapeId>
struct InternalTryCellShape
{
  template <typename FunctionType>
  void operator()(const FunctionType& function) const
  {
    this->PrintAndInvoke(function, typename svtkm::CellShapeIdToTag<cellShapeId>::valid());
    InternalTryCellShape<cellShapeId + 1>()(function);
  }

private:
  template <typename FunctionType>
  void PrintAndInvoke(const FunctionType& function, std::true_type) const
  {
    using CellShapeTag = typename svtkm::CellShapeIdToTag<cellShapeId>::Tag;
    std::cout << "*** " << svtkm::GetCellShapeName(CellShapeTag()) << " ***************"
              << std::endl;
    function(CellShapeTag());
  }

  template <typename FunctionType>
  void PrintAndInvoke(const FunctionType&, std::false_type) const
  {
    // Not a valid cell shape. Do nothing.
  }
};

template <>
struct InternalTryCellShape<svtkm::NUMBER_OF_CELL_SHAPES>
{
  template <typename FunctionType>
  void operator()(const FunctionType&) const
  {
    // Done processing cell sets. Do nothing and return.
  }
};

} // namespace detail

struct Testing
{
public:
  class TestFailure
  {
  public:
    template <typename... Ts>
    SVTKM_CONT TestFailure(const std::string& file, svtkm::Id line, Ts&&... messages)
      : File(file)
      , Line(line)
    {
      std::stringstream messageStream;
      this->AppendMessages(messageStream, std::forward<Ts>(messages)...);
      this->Message = messageStream.str();
    }

    SVTKM_CONT const std::string& GetFile() const { return this->File; }
    SVTKM_CONT svtkm::Id GetLine() const { return this->Line; }
    SVTKM_CONT const std::string& GetMessage() const { return this->Message; }
  private:
    template <typename T1>
    SVTKM_CONT void AppendMessages(std::stringstream& messageStream, T1&& m1)
    {
      messageStream << m1;
    }
    template <typename T1, typename T2>
    SVTKM_CONT void AppendMessages(std::stringstream& messageStream, T1&& m1, T2&& m2)
    {
      messageStream << m1 << m2;
    }
    template <typename T1, typename T2, typename T3>
    SVTKM_CONT void AppendMessages(std::stringstream& messageStream, T1&& m1, T2&& m2, T3&& m3)
    {
      messageStream << m1 << m2 << m3;
    }
    template <typename T1, typename T2, typename T3, typename T4>
    SVTKM_CONT void AppendMessages(std::stringstream& messageStream,
                                  T1&& m1,
                                  T2&& m2,
                                  T3&& m3,
                                  T4&& m4)
    {
      messageStream << m1 << m2 << m3 << m4;
    }
    template <typename T1, typename T2, typename T3, typename T4, typename... Ts>
    SVTKM_CONT void AppendMessages(std::stringstream& messageStream,
                                  T1&& m1,
                                  T2&& m2,
                                  T3&& m3,
                                  T4&& m4,
                                  Ts&&... ms)
    {
      messageStream << m1 << m2 << m3 << m4;
      this->AppendMessages(messageStream, std::forward<Ts>(ms)...);
    }

    std::string File;
    svtkm::Id Line;
    std::string Message;
  };

  template <typename... Ts>
  static SVTKM_CONT void Assert(const std::string& conditionString,
                               const std::string& file,
                               svtkm::Id line,
                               bool condition,
                               Ts&&... messages)
  {
    if (condition)
    {
      // Do nothing.
    }
    else
    {
      throw TestFailure(file, line, std::forward<Ts>(messages)..., " (", conditionString, ")");
    }
  }

  static SVTKM_CONT void Assert(const std::string& conditionString,
                               const std::string& file,
                               svtkm::Id line,
                               bool condition)
  {
    Assert(conditionString, file, line, condition, "Test assertion failed");
  }

  template <typename... Ts>
  static SVTKM_CONT void TestFail(const std::string& file, svtkm::Id line, Ts&&... messages)
  {
    throw TestFailure(file, line, std::forward<Ts>(messages)...);
  }

#ifndef SVTKM_TESTING_IN_CONT
  /// Calls the test function \a function with no arguments. Catches any errors
  /// generated by SVTKM_TEST_ASSERT or SVTKM_TEST_FAIL, reports the error, and
  /// returns "1" (a failure status for a program's main). Returns "0" (a
  /// success status for a program's main).
  ///
  /// The intention is to implement a test's main function with this. For
  /// example, the implementation of UnitTestFoo might look something like
  /// this.
  ///
  /// \code
  /// #include <svtkm/testing/Testing.h>
  ///
  /// namespace {
  ///
  /// void TestFoo()
  /// {
  ///    // Do actual test, which checks in SVTKM_TEST_ASSERT or SVTKM_TEST_FAIL.
  /// }
  ///
  /// } // anonymous namespace
  ///
  /// int UnitTestFoo(int, char *[])
  /// {
  ///   return svtkm::testing::Testing::Run(TestFoo);
  /// }
  /// \endcode
  ///
  template <class Func>
  static SVTKM_CONT int Run(Func function, int& argc, char* argv[])
  {
    if (argc == 0 || argv == nullptr)
    {
      svtkm::cont::InitLogging();
    }
    else
    {
      svtkm::cont::InitLogging(argc, argv);
    }

    try
    {
      function();
    }
    catch (TestFailure& error)
    {
      std::cout << "***** Test failed @ " << error.GetFile() << ":" << error.GetLine() << std::endl
                << error.GetMessage() << std::endl;
      return 1;
    }
    catch (std::exception& error)
    {
      std::cout << "***** STL exception throw." << std::endl << error.what() << std::endl;
    }
    catch (...)
    {
      std::cout << "***** Unidentified exception thrown." << std::endl;
      return 1;
    }
    return 0;
  }
#endif

  template <typename FunctionType>
  struct InternalPrintTypeAndInvoke
  {
    InternalPrintTypeAndInvoke(FunctionType function)
      : Function(function)
    {
    }

    template <typename T>
    void operator()(T t) const
    {
      std::cout << "*** " << svtkm::testing::TypeName<T>::Name() << " ***************" << std::endl;
      this->Function(t);
    }

  private:
    FunctionType Function;
  };

  /// Runs template \p function on all the types in the given list. If no type
  /// list is given, then an exemplar list of types is used.
  ///
  template <typename FunctionType, typename TypeList>
  static void TryTypes(const FunctionType& function, TypeList)
  {
    svtkm::ListForEach(InternalPrintTypeAndInvoke<FunctionType>(function), TypeList());
  }

  using TypeListExemplarTypes =
    svtkm::List<svtkm::UInt8, svtkm::Id, svtkm::FloatDefault, svtkm::Vec3f_64>;

  template <typename FunctionType>
  static void TryTypes(const FunctionType& function)
  {
    TryTypes(function, TypeListExemplarTypes());
  }

  // Disabled: This very long list results is very long compile times.
  //  /// Runs templated \p function on all the basic types defined in SVTK-m. This
  //  /// is helpful to test templated functions that should work on all types. If
  //  /// the function is supposed to work on some subset of types, then use
  //  /// \c TryTypes to restrict the call to some other list of types.
  //  ///
  //  template<typename FunctionType>
  //  static void TryAllTypes(const FunctionType &function)
  //  {
  //    TryTypes(function, svtkm::TypeListAll());
  //  }

  /// Runs templated \p function on all cell shapes defined in SVTK-m. This is
  /// helpful to test templated functions that should work on all cell types.
  ///
  template <typename FunctionType>
  static void TryAllCellShapes(const FunctionType& function)
  {
    detail::InternalTryCellShape<0>()(function);
  }
};
}
} // namespace svtkm::internal

namespace detail
{

// Forward declaration
template <typename T1, typename T2>
struct TestEqualImpl;

} // namespace detail

/// Helper function to test two quanitites for equality accounting for slight
/// variance due to floating point numerical inaccuracies.
///
template <typename T1, typename T2>
static inline SVTKM_EXEC_CONT bool test_equal(T1 value1,
                                             T2 value2,
                                             svtkm::Float64 tolerance = 0.00001)
{
  return detail::TestEqualImpl<T1, T2>()(value1, value2, tolerance);
}

namespace detail
{

template <typename T1, typename T2>
struct TestEqualImpl
{
  SVTKM_EXEC_CONT bool DoIt(T1 vector1,
                           T2 vector2,
                           svtkm::Float64 tolerance,
                           svtkm::TypeTraitsVectorTag) const
  {
    // If you get a compiler error here, it means you are comparing a vector to
    // a scalar, in which case the types are non-comparable.
    SVTKM_STATIC_ASSERT_MSG((std::is_same<typename svtkm::TypeTraits<T2>::DimensionalityTag,
                                         svtkm::TypeTraitsVectorTag>::type::value) ||
                             (std::is_same<typename svtkm::TypeTraits<T2>::DimensionalityTag,
                                           svtkm::TypeTraitsMatrixTag>::type::value),
                           "Trying to compare a vector with a scalar.");

    using Traits1 = svtkm::VecTraits<T1>;
    using Traits2 = svtkm::VecTraits<T2>;

    // If vectors have different number of components, then they cannot be equal.
    if (Traits1::GetNumberOfComponents(vector1) != Traits2::GetNumberOfComponents(vector2))
    {
      return false;
    }

    for (svtkm::IdComponent component = 0; component < Traits1::GetNumberOfComponents(vector1);
         ++component)
    {
      bool componentEqual = test_equal(Traits1::GetComponent(vector1, component),
                                       Traits2::GetComponent(vector2, component),
                                       tolerance);
      if (!componentEqual)
      {
        return false;
      }
    }

    return true;
  }

  SVTKM_EXEC_CONT bool DoIt(T1 matrix1,
                           T2 matrix2,
                           svtkm::Float64 tolerance,
                           svtkm::TypeTraitsMatrixTag) const
  {
    // For the purposes of comparison, treat matrices the same as vectors.
    return this->DoIt(matrix1, matrix2, tolerance, svtkm::TypeTraitsVectorTag());
  }

  SVTKM_EXEC_CONT bool DoIt(T1 scalar1,
                           T2 scalar2,
                           svtkm::Float64 tolerance,
                           svtkm::TypeTraitsScalarTag) const
  {
    // If you get a compiler error here, it means you are comparing a scalar to
    // a vector, in which case the types are non-comparable.
    SVTKM_STATIC_ASSERT_MSG((std::is_same<typename svtkm::TypeTraits<T2>::DimensionalityTag,
                                         svtkm::TypeTraitsScalarTag>::type::value),
                           "Trying to compare a scalar with a vector.");

    // Do all comparisons using 64-bit floats.
    svtkm::Float64 value1 = svtkm::Float64(scalar1);
    svtkm::Float64 value2 = svtkm::Float64(scalar2);

    if (svtkm::Abs(value1 - value2) <= tolerance)
    {
      return true;
    }

    // We are using a ratio to compare the relative tolerance of two numbers.
    // Using an ULP based comparison (comparing the bits as integers) might be
    // a better way to go, but this has been working pretty well so far.
    svtkm::Float64 ratio;
    if ((svtkm::Abs(value2) > tolerance) && (value2 != 0))
    {
      ratio = value1 / value2;
    }
    else
    {
      // If we are here, it means that value2 is close to 0 but value1 is not.
      // These cannot be within tolerance, so just return false.
      return false;
    }
    if ((ratio > svtkm::Float64(1.0) - tolerance) && (ratio < svtkm::Float64(1.0) + tolerance))
    {
      // This component is OK. The condition is checked in this way to
      // correctly handle non-finites that fail all comparisons. Thus, if a
      // non-finite is encountered, this condition will fail and false will be
      // returned.
      return true;
    }
    else
    {
      return false;
    }
  }

  SVTKM_EXEC_CONT bool operator()(T1 value1, T2 value2, svtkm::Float64 tolerance) const
  {
    return this->DoIt(
      value1, value2, tolerance, typename svtkm::TypeTraits<T1>::DimensionalityTag());
  }
};

// Special cases of test equal where a scalar is compared with a Vec of size 1,
// which we will allow.
template <typename T>
struct TestEqualImpl<svtkm::Vec<T, 1>, T>
{
  SVTKM_EXEC_CONT bool operator()(svtkm::Vec<T, 1> value1, T value2, svtkm::Float64 tolerance) const
  {
    return test_equal(value1[0], value2, tolerance);
  }
};
template <typename T>
struct TestEqualImpl<T, svtkm::Vec<T, 1>>
{
  SVTKM_EXEC_CONT bool operator()(T value1, svtkm::Vec<T, 1> value2, svtkm::Float64 tolerance) const
  {
    return test_equal(value1, value2[0], tolerance);
  }
};

/// Special implementation of test_equal for strings, which don't fit a model
/// of fixed length vectors of numbers.
///
template <>
struct TestEqualImpl<std::string, std::string>
{
  SVTKM_CONT bool operator()(const std::string& string1,
                            const std::string& string2,
                            svtkm::Float64 svtkmNotUsed(tolerance)) const
  {
    return string1 == string2;
  }
};
template <typename T>
struct TestEqualImpl<const char*, T>
{
  SVTKM_CONT bool operator()(const char* string1, T value2, svtkm::Float64 tolerance) const
  {
    return TestEqualImpl<std::string, T>()(string1, value2, tolerance);
  }
};
template <typename T>
struct TestEqualImpl<T, const char*>
{
  SVTKM_CONT bool operator()(T value1, const char* string2, svtkm::Float64 tolerance) const
  {
    return TestEqualImpl<T, std::string>()(value1, string2, tolerance);
  }
};
template <>
struct TestEqualImpl<const char*, const char*>
{
  SVTKM_CONT bool operator()(const char* string1, const char* string2, svtkm::Float64 tolerance) const
  {
    return TestEqualImpl<std::string, std::string>()(string1, string2, tolerance);
  }
};

/// Special implementation of test_equal for Pairs, which are a bit different
/// than a vector of numbers of the same type.
///
template <typename T1, typename T2, typename T3, typename T4>
struct TestEqualImpl<svtkm::Pair<T1, T2>, svtkm::Pair<T3, T4>>
{
  SVTKM_EXEC_CONT bool operator()(const svtkm::Pair<T1, T2>& pair1,
                                 const svtkm::Pair<T3, T4>& pair2,
                                 svtkm::Float64 tolerance) const
  {
    return test_equal(pair1.first, pair2.first, tolerance) &&
      test_equal(pair1.second, pair2.second, tolerance);
  }
};

/// Special implementation of test_equal for Ranges.
///
template <>
struct TestEqualImpl<svtkm::Range, svtkm::Range>
{
  SVTKM_EXEC_CONT bool operator()(const svtkm::Range& range1,
                                 const svtkm::Range& range2,
                                 svtkm::Float64 tolerance) const
  {
    return (test_equal(range1.Min, range2.Min, tolerance) &&
            test_equal(range1.Max, range2.Max, tolerance));
  }
};

/// Special implementation of test_equal for Bounds.
///
template <>
struct TestEqualImpl<svtkm::Bounds, svtkm::Bounds>
{
  SVTKM_EXEC_CONT bool operator()(const svtkm::Bounds& bounds1,
                                 const svtkm::Bounds& bounds2,
                                 svtkm::Float64 tolerance) const
  {
    return (test_equal(bounds1.X, bounds2.X, tolerance) &&
            test_equal(bounds1.Y, bounds2.Y, tolerance) &&
            test_equal(bounds1.Z, bounds2.Z, tolerance));
  }
};

/// Special implementation of test_equal for booleans.
///
template <>
struct TestEqualImpl<bool, bool>
{
  SVTKM_EXEC_CONT bool operator()(bool bool1, bool bool2, svtkm::Float64 svtkmNotUsed(tolerance))
  {
    return bool1 == bool2;
  }
};

} // namespace detail

namespace detail
{

template <typename T>
struct TestValueImpl;

} // namespace detail

/// Many tests involve getting and setting values in some index-based structure
/// (like an array). These tests also often involve trying many types. The
/// overloaded TestValue function returns some unique value for an index for a
/// given type. Different types might give different values.
///
template <typename T>
static inline SVTKM_EXEC_CONT T TestValue(svtkm::Id index, T)
{
  return detail::TestValueImpl<T>()(index);
}

namespace detail
{

template <typename T>
struct TestValueImpl
{
  SVTKM_EXEC_CONT T DoIt(svtkm::Id index, svtkm::TypeTraitsIntegerTag) const
  {
    constexpr bool larger_than_2bytes = sizeof(T) > 2;
    if (larger_than_2bytes)
    {
      return T(index * 100);
    }
    else
    {
      return T(index + 100);
    }
  }

  SVTKM_EXEC_CONT T DoIt(svtkm::Id index, svtkm::TypeTraitsRealTag) const
  {
    return T(0.01f * static_cast<float>(index) + 1.001f);
  }

  SVTKM_EXEC_CONT T operator()(svtkm::Id index) const
  {
    return this->DoIt(index, typename svtkm::TypeTraits<T>::NumericTag());
  }
};

template <typename T, svtkm::IdComponent N>
struct TestValueImpl<svtkm::Vec<T, N>>
{
  SVTKM_EXEC_CONT svtkm::Vec<T, N> operator()(svtkm::Id index) const
  {
    svtkm::Vec<T, N> value;
    for (svtkm::IdComponent i = 0; i < N; i++)
    {
      value[i] = TestValue(index * N + i, T());
    }
    return value;
  }
};

template <typename U, typename V>
struct TestValueImpl<svtkm::Pair<U, V>>
{
  SVTKM_EXEC_CONT svtkm::Pair<U, V> operator()(svtkm::Id index) const
  {
    return svtkm::Pair<U, V>(TestValue(2 * index, U()), TestValue(2 * index + 1, V()));
  }
};

template <typename T, svtkm::IdComponent NumRow, svtkm::IdComponent NumCol>
struct TestValueImpl<svtkm::Matrix<T, NumRow, NumCol>>
{
  SVTKM_EXEC_CONT svtkm::Matrix<T, NumRow, NumCol> operator()(svtkm::Id index) const
  {
    svtkm::Matrix<T, NumRow, NumCol> value;
    svtkm::Id runningIndex = index * NumRow * NumCol;
    for (svtkm::IdComponent row = 0; row < NumRow; ++row)
    {
      for (svtkm::IdComponent col = 0; col < NumCol; ++col)
      {
        value(row, col) = TestValue(runningIndex, T());
        ++runningIndex;
      }
    }
    return value;
  }
};

template <>
struct TestValueImpl<std::string>
{
  SVTKM_CONT std::string operator()(svtkm::Id index) const
  {
    std::stringstream stream;
    stream << index;
    return stream.str();
  }
};

} // namespace detail

/// Verifies that the contents of the given array portal match the values
/// returned by svtkm::testing::TestValue.
///
template <typename PortalType>
static inline SVTKM_CONT void CheckPortal(const PortalType& portal)
{
  using ValueType = typename PortalType::ValueType;

  for (svtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
  {
    ValueType expectedValue = TestValue(index, ValueType());
    ValueType foundValue = portal.Get(index);
    if (!test_equal(expectedValue, foundValue))
    {
      std::stringstream message;
      message << "Got unexpected value in array." << std::endl
              << "Expected: " << expectedValue << ", Found: " << foundValue << std::endl;
      SVTKM_TEST_FAIL(message.str().c_str());
    }
  }
}

/// Sets all the values in a given array portal to be the values returned
/// by svtkm::testing::TestValue. The ArrayPortal must be allocated first.
///
template <typename PortalType>
static inline SVTKM_CONT void SetPortal(const PortalType& portal)
{
  using ValueType = typename PortalType::ValueType;

  for (svtkm::Id index = 0; index < portal.GetNumberOfValues(); index++)
  {
    portal.Set(index, TestValue(index, ValueType()));
  }
}

/// Verifies that the contents of the two portals are the same.
///
template <typename PortalType1, typename PortalType2>
static inline SVTKM_CONT bool test_equal_portals(const PortalType1& portal1,
                                                const PortalType2& portal2)
{
  if (portal1.GetNumberOfValues() != portal2.GetNumberOfValues())
  {
    return false;
  }

  for (svtkm::Id index = 0; index < portal1.GetNumberOfValues(); index++)
  {
    if (!test_equal(portal1.Get(index), portal2.Get(index)))
    {
      return false;
    }
  }

  return true;
}

#endif //svtk_m_testing_Testing_h
