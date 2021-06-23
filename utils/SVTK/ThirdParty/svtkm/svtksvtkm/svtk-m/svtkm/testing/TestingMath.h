//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================
#ifndef svtk_m_testing_TestingMath_h
#define svtk_m_testing_TestingMath_h

#include <svtkm/Math.h>

#include <svtkm/TypeList.h>
#include <svtkm/VecTraits.h>

#include <svtkm/exec/FunctorBase.h>

#include <svtkm/cont/DeviceAdapterAlgorithm.h>

#include <limits>

#define SVTKM_MATH_ASSERT(condition, message)                                                       \
  if (!(condition))                                                                                \
  {                                                                                                \
    this->RaiseError(message);                                                                     \
  }

//-----------------------------------------------------------------------------
namespace UnitTestMathNamespace
{

class Lists
{
public:
  static constexpr svtkm::IdComponent NUM_NUMBERS = 5;

  SVTKM_EXEC_CONT svtkm::Float64 NumberList(svtkm::Int32 i) const
  {
    svtkm::Float64 numberList[NUM_NUMBERS] = { 0.25, 0.5, 1.0, 2.0, 3.75 };
    return numberList[i];
  }
  SVTKM_EXEC_CONT svtkm::Float64 AngleList(svtkm::Int32 i) const
  {
    svtkm::Float64 angleList[NUM_NUMBERS] = { 0.643501108793284, // angle for 3, 4, 5 triangle.
                                             0.78539816339745,  // pi/4
                                             0.5235987755983,   // pi/6
                                             1.0471975511966,   // pi/3
                                             0.0 };
    return angleList[i];
  }
  SVTKM_EXEC_CONT svtkm::Float64 OppositeList(svtkm::Int32 i) const
  {
    svtkm::Float64 oppositeList[NUM_NUMBERS] = { 3.0, 1.0, 1.0, 1.732050807568877 /*sqrt(3)*/, 0.0 };
    return oppositeList[i];
  }
  SVTKM_EXEC_CONT svtkm::Float64 AdjacentList(svtkm::Int32 i) const
  {
    svtkm::Float64 adjacentList[NUM_NUMBERS] = { 4.0, 1.0, 1.732050807568877 /*sqrt(3)*/, 1.0, 1.0 };
    return adjacentList[i];
  }
  SVTKM_EXEC_CONT svtkm::Float64 HypotenuseList(svtkm::Int32 i) const
  {
    svtkm::Float64 hypotenuseList[NUM_NUMBERS] = {
      5.0, 1.414213562373095 /*sqrt(2)*/, 2.0, 2.0, 1.0
    };
    return hypotenuseList[i];
  }
  SVTKM_EXEC_CONT svtkm::Float64 NumeratorList(svtkm::Int32 i) const
  {
    svtkm::Float64 numeratorList[NUM_NUMBERS] = { 6.5, 5.8, 9.3, 77.0, 0.1 };
    return numeratorList[i];
  }
  SVTKM_EXEC_CONT svtkm::Float64 DenominatorList(svtkm::Int32 i) const
  {
    svtkm::Float64 denominatorList[NUM_NUMBERS] = { 2.3, 1.6, 3.1, 19.0, 0.4 };
    return denominatorList[i];
  }
  SVTKM_EXEC_CONT svtkm::Float64 FModRemainderList(svtkm::Int32 i) const
  {
    svtkm::Float64 fModRemainderList[NUM_NUMBERS] = { 1.9, 1.0, 0.0, 1.0, 0.1 };
    return fModRemainderList[i];
  }
  SVTKM_EXEC_CONT svtkm::Float64 RemainderList(svtkm::Int32 i) const
  {
    svtkm::Float64 remainderList[NUM_NUMBERS] = { -0.4, -0.6, 0.0, 1.0, 0.1 };
    return remainderList[i];
  }
  SVTKM_EXEC_CONT svtkm::Int64 QuotientList(svtkm::Int32 i) const
  {
    svtkm::Int64 quotientList[NUM_NUMBERS] = { 3, 4, 3, 4, 0 };
    return quotientList[i];
  }
  SVTKM_EXEC_CONT svtkm::Float64 XList(svtkm::Int32 i) const
  {
    svtkm::Float64 xList[NUM_NUMBERS] = { 4.6, 0.1, 73.4, 55.0, 3.75 };
    return xList[i];
  }
  SVTKM_EXEC_CONT svtkm::Float64 FractionalList(svtkm::Int32 i) const
  {
    svtkm::Float64 fractionalList[NUM_NUMBERS] = { 0.6, 0.1, 0.4, 0.0, 0.75 };
    return fractionalList[i];
  }
  SVTKM_EXEC_CONT svtkm::Float64 FloorList(svtkm::Int32 i) const
  {
    svtkm::Float64 floorList[NUM_NUMBERS] = { 4.0, 0.0, 73.0, 55.0, 3.0 };
    return floorList[i];
  }
  SVTKM_EXEC_CONT svtkm::Float64 CeilList(svtkm::Int32 i) const
  {
    svtkm::Float64 ceilList[NUM_NUMBERS] = { 5.0, 1.0, 74.0, 55.0, 4.0 };
    return ceilList[i];
  }
  SVTKM_EXEC_CONT svtkm::Float64 RoundList(svtkm::Int32 i) const
  {
    svtkm::Float64 roundList[NUM_NUMBERS] = { 5.0, 0.0, 73.0, 55.0, 4.0 };
    return roundList[i];
  }
};

//-----------------------------------------------------------------------------
template <typename T>
struct ScalarFieldTests : public svtkm::exec::FunctorBase
{
  SVTKM_EXEC
  void TestPi() const
  {
    //    std::cout << "Testing Pi" << std::endl;
    SVTKM_MATH_ASSERT(test_equal(svtkm::Pi(), 3.14159265), "Pi not correct.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::Pif(), 3.14159265f), "Pif not correct.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::Pi<svtkm::Float64>(), 3.14159265),
                     "Pi template function not correct.");
  }

  SVTKM_EXEC
  void TestArcTan2() const
  {
    //    std::cout << "Testing arc tan 2" << std::endl;

    SVTKM_MATH_ASSERT(test_equal(svtkm::ATan2(T(0.0), T(1.0)), T(0.0)), "ATan2 x+ axis.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::ATan2(T(1.0), T(0.0)), T(0.5 * svtkm::Pi())),
                     "ATan2 y+ axis.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::ATan2(T(-1.0), T(0.0)), T(-0.5 * svtkm::Pi())),
                     "ATan2 y- axis.");

    SVTKM_MATH_ASSERT(test_equal(svtkm::ATan2(T(1.0), T(1.0)), T(0.25 * svtkm::Pi())),
                     "ATan2 Quadrant 1");
    SVTKM_MATH_ASSERT(test_equal(svtkm::ATan2(T(1.0), T(-1.0)), T(0.75 * svtkm::Pi())),
                     "ATan2 Quadrant 2");
    SVTKM_MATH_ASSERT(test_equal(svtkm::ATan2(T(-1.0), T(-1.0)), T(-0.75 * svtkm::Pi())),
                     "ATan2 Quadrant 3");
    SVTKM_MATH_ASSERT(test_equal(svtkm::ATan2(T(-1.0), T(1.0)), T(-0.25 * svtkm::Pi())),
                     "ATan2 Quadrant 4");
  }

  SVTKM_EXEC
  void TestPow() const
  {
    //    std::cout << "Running power tests." << std::endl;
    for (svtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS; index++)
    {
      T x = static_cast<T>(Lists{}.NumberList(index));
      T powx = svtkm::Pow(x, static_cast<T>(2.0));
      T sqrx = x * x;
      SVTKM_MATH_ASSERT(test_equal(powx, sqrx), "Power gave wrong result.");
    }
  }

  SVTKM_EXEC
  void TestLog2() const
  {
    //    std::cout << "Testing Log2" << std::endl;
    SVTKM_MATH_ASSERT(test_equal(svtkm::Log2(T(0.25)), T(-2.0)), "Bad value from Log2");
    SVTKM_MATH_ASSERT(test_equal(svtkm::Log2(svtkm::Vec<T, 4>(0.5, 1.0, 2.0, 4.0)),
                                svtkm::Vec<T, 4>(-1.0, 0.0, 1.0, 2.0)),
                     "Bad value from Log2");
  }

  SVTKM_EXEC
  void TestNonFinites() const
  {
    //    std::cout << "Testing non-finites." << std::endl;

    T zero = 0.0;
    T finite = 1.0;
    T nan = svtkm::Nan<T>();
    T inf = svtkm::Infinity<T>();
    T neginf = svtkm::NegativeInfinity<T>();
    T epsilon = svtkm::Epsilon<T>();

    // General behavior.
    SVTKM_MATH_ASSERT(nan != svtkm::Nan<T>(), "Nan not equal itself.");
    SVTKM_MATH_ASSERT(!(nan >= zero), "Nan not greater or less.");
    SVTKM_MATH_ASSERT(!(nan <= zero), "Nan not greater or less.");
    SVTKM_MATH_ASSERT(!(nan >= finite), "Nan not greater or less.");
    SVTKM_MATH_ASSERT(!(nan <= finite), "Nan not greater or less.");

    SVTKM_MATH_ASSERT(neginf < inf, "Infinity big");
    SVTKM_MATH_ASSERT(zero < inf, "Infinity big");
    SVTKM_MATH_ASSERT(finite < inf, "Infinity big");
    SVTKM_MATH_ASSERT(zero > -inf, "-Infinity small");
    SVTKM_MATH_ASSERT(finite > -inf, "-Infinity small");
    SVTKM_MATH_ASSERT(zero > neginf, "-Infinity small");
    SVTKM_MATH_ASSERT(finite > neginf, "-Infinity small");

    SVTKM_MATH_ASSERT(zero < epsilon, "Negative epsilon");
    SVTKM_MATH_ASSERT(finite > epsilon, "Large epsilon");

    // Math check functions.
    SVTKM_MATH_ASSERT(!svtkm::IsNan(zero), "Bad IsNan check.");
    SVTKM_MATH_ASSERT(!svtkm::IsNan(finite), "Bad IsNan check.");
    SVTKM_MATH_ASSERT(svtkm::IsNan(nan), "Bad IsNan check.");
    SVTKM_MATH_ASSERT(!svtkm::IsNan(inf), "Bad IsNan check.");
    SVTKM_MATH_ASSERT(!svtkm::IsNan(neginf), "Bad IsNan check.");
    SVTKM_MATH_ASSERT(!svtkm::IsNan(epsilon), "Bad IsNan check.");

    SVTKM_MATH_ASSERT(!svtkm::IsInf(zero), "Bad infinity check.");
    SVTKM_MATH_ASSERT(!svtkm::IsInf(finite), "Bad infinity check.");
    SVTKM_MATH_ASSERT(!svtkm::IsInf(nan), "Bad infinity check.");
    SVTKM_MATH_ASSERT(svtkm::IsInf(inf), "Bad infinity check.");
    SVTKM_MATH_ASSERT(svtkm::IsInf(neginf), "Bad infinity check.");
    SVTKM_MATH_ASSERT(!svtkm::IsInf(epsilon), "Bad infinity check.");

    SVTKM_MATH_ASSERT(svtkm::IsFinite(zero), "Bad finite check.");
    SVTKM_MATH_ASSERT(svtkm::IsFinite(finite), "Bad finite check.");
    SVTKM_MATH_ASSERT(!svtkm::IsFinite(nan), "Bad finite check.");
    SVTKM_MATH_ASSERT(!svtkm::IsFinite(inf), "Bad finite check.");
    SVTKM_MATH_ASSERT(!svtkm::IsFinite(neginf), "Bad finite check.");
    SVTKM_MATH_ASSERT(svtkm::IsFinite(epsilon), "Bad finite check.");
  }

  SVTKM_EXEC
  void TestRemainders() const
  {
    //    std::cout << "Testing remainders." << std::endl;
    Lists table;
    for (svtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS; index++)
    {
      T numerator = static_cast<T>(table.NumeratorList(index));
      T denominator = static_cast<T>(table.DenominatorList(index));
      T fmodremainder = static_cast<T>(table.FModRemainderList(index));
      T remainder = static_cast<T>(table.RemainderList(index));
      svtkm::Int64 quotient = table.QuotientList(index);

      SVTKM_MATH_ASSERT(test_equal(svtkm::FMod(numerator, denominator), fmodremainder),
                       "Bad FMod remainder.");
      SVTKM_MATH_ASSERT(test_equal(svtkm::Remainder(numerator, denominator), remainder),
                       "Bad remainder.");
      svtkm::Int64 q;
      SVTKM_MATH_ASSERT(test_equal(svtkm::RemainderQuotient(numerator, denominator, q), remainder),
                       "Bad remainder-quotient remainder.");
      SVTKM_MATH_ASSERT(test_equal(q, quotient), "Bad reminder-quotient quotient.");
    }
  }

  SVTKM_EXEC
  void TestRound() const
  {
    //    std::cout << "Testing round." << std::endl;
    Lists table;
    for (svtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS; index++)
    {
      T x = static_cast<T>(table.XList(index));
      T fractional = static_cast<T>(table.FractionalList(index));
      T floor = static_cast<T>(table.FloorList(index));
      T ceil = static_cast<T>(table.CeilList(index));
      T round = static_cast<T>(table.RoundList(index));

      T intPart;
      SVTKM_MATH_ASSERT(test_equal(svtkm::ModF(x, intPart), fractional),
                       "ModF returned wrong fractional part.");
      SVTKM_MATH_ASSERT(test_equal(intPart, floor), "ModF returned wrong integral part.");
      SVTKM_MATH_ASSERT(test_equal(svtkm::Floor(x), floor), "Bad floor.");
      SVTKM_MATH_ASSERT(test_equal(svtkm::Ceil(x), ceil), "Bad ceil.");
      SVTKM_MATH_ASSERT(test_equal(svtkm::Round(x), round), "Bad round.");
    }
  }

  SVTKM_EXEC
  void TestIsNegative() const
  {
    //    std::cout << "Testing SignBit and IsNegative." << std::endl;
    T x = 0;
    SVTKM_MATH_ASSERT(svtkm::SignBit(x) == 0, "SignBit wrong for 0.");
    SVTKM_MATH_ASSERT(!svtkm::IsNegative(x), "IsNegative wrong for 0.");

    x = 20;
    SVTKM_MATH_ASSERT(svtkm::SignBit(x) == 0, "SignBit wrong for 20.");
    SVTKM_MATH_ASSERT(!svtkm::IsNegative(x), "IsNegative wrong for 20.");

    x = -20;
    SVTKM_MATH_ASSERT(svtkm::SignBit(x) != 0, "SignBit wrong for -20.");
    SVTKM_MATH_ASSERT(svtkm::IsNegative(x), "IsNegative wrong for -20.");

    x = 0.02f;
    SVTKM_MATH_ASSERT(svtkm::SignBit(x) == 0, "SignBit wrong for 0.02.");
    SVTKM_MATH_ASSERT(!svtkm::IsNegative(x), "IsNegative wrong for 0.02.");

    x = -0.02f;
    SVTKM_MATH_ASSERT(svtkm::SignBit(x) != 0, "SignBit wrong for -0.02.");
    SVTKM_MATH_ASSERT(svtkm::IsNegative(x), "IsNegative wrong for -0.02.");
  }

  SVTKM_EXEC
  void operator()(svtkm::Id) const
  {
    this->TestPi();
    this->TestArcTan2();
    this->TestPow();
    this->TestLog2();
    this->TestNonFinites();
    this->TestRemainders();
    this->TestRound();
    this->TestIsNegative();
  }
};

template <typename Device>
struct TryScalarFieldTests
{
  template <typename T>
  void operator()(const T&) const
  {
    svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(ScalarFieldTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------
template <typename VectorType>
struct ScalarVectorFieldTests : public svtkm::exec::FunctorBase
{
  using Traits = svtkm::VecTraits<VectorType>;
  using ComponentType = typename Traits::ComponentType;
  enum
  {
    NUM_COMPONENTS = Traits::NUM_COMPONENTS
  };

  SVTKM_EXEC
  void TestTriangleTrig() const
  {
    //    std::cout << "Testing normal trig functions." << std::endl;
    Lists table;
    for (svtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType angle;
      VectorType opposite;
      VectorType adjacent;
      VectorType hypotenuse;
      for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
      {
        Traits::SetComponent(angle,
                             componentIndex,
                             static_cast<ComponentType>(table.AngleList(componentIndex + index)));
        Traits::SetComponent(
          opposite,
          componentIndex,
          static_cast<ComponentType>(table.OppositeList(componentIndex + index)));
        Traits::SetComponent(
          adjacent,
          componentIndex,
          static_cast<ComponentType>(table.AdjacentList(componentIndex + index)));
        Traits::SetComponent(
          hypotenuse,
          componentIndex,
          static_cast<ComponentType>(table.HypotenuseList(componentIndex + index)));
      }

      SVTKM_MATH_ASSERT(test_equal(svtkm::Sin(angle), opposite / hypotenuse), "Sin failed test.");
      SVTKM_MATH_ASSERT(test_equal(svtkm::Cos(angle), adjacent / hypotenuse), "Cos failed test.");
      SVTKM_MATH_ASSERT(test_equal(svtkm::Tan(angle), opposite / adjacent), "Tan failed test.");

      SVTKM_MATH_ASSERT(test_equal(svtkm::ASin(opposite / hypotenuse), angle),
                       "Arc Sin failed test.");

#if defined(SVTKM_ICC)
      // When the intel compiler has vectorization enabled ( -O2/-O3 ) it converts the
      // `adjacent/hypotenuse` divide operation into reciprocal (rcpps) and
      // multiply (mulps) operations. This causes a change in the expected result that
      // is larger than the default tolerance of test_equal.
      //
      SVTKM_MATH_ASSERT(test_equal(svtkm::ACos(adjacent / hypotenuse), angle, 0.0004),
                       "Arc Cos failed test.");
#else
      SVTKM_MATH_ASSERT(test_equal(svtkm::ACos(adjacent / hypotenuse), angle),
                       "Arc Cos failed test.");
#endif
      SVTKM_MATH_ASSERT(test_equal(svtkm::ATan(opposite / adjacent), angle), "Arc Tan failed test.");
    }
  }

  SVTKM_EXEC
  void TestHyperbolicTrig() const
  {
    //    std::cout << "Testing hyperbolic trig functions." << std::endl;

    const VectorType zero(0);
    const VectorType half(0.5);
    Lists table;
    for (svtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType x;
      for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
      {
        Traits::SetComponent(
          x, componentIndex, static_cast<ComponentType>(table.AngleList(componentIndex + index)));
      }

      const VectorType minusX = zero - x;

      SVTKM_MATH_ASSERT(test_equal(svtkm::SinH(x), half * (svtkm::Exp(x) - svtkm::Exp(minusX))),
                       "SinH does not match definition.");
      SVTKM_MATH_ASSERT(test_equal(svtkm::CosH(x), half * (svtkm::Exp(x) + svtkm::Exp(minusX))),
                       "SinH does not match definition.");
      SVTKM_MATH_ASSERT(test_equal(svtkm::TanH(x), svtkm::SinH(x) / svtkm::CosH(x)),
                       "TanH does not match definition");

      SVTKM_MATH_ASSERT(test_equal(svtkm::ASinH(svtkm::SinH(x)), x), "SinH not inverting.");
      SVTKM_MATH_ASSERT(test_equal(svtkm::ACosH(svtkm::CosH(x)), x), "CosH not inverting.");
      SVTKM_MATH_ASSERT(test_equal(svtkm::ATanH(svtkm::TanH(x)), x), "TanH not inverting.");
    }
  }

  template <typename FunctionType>
  SVTKM_EXEC void RaiseToTest(FunctionType function, ComponentType exponent) const
  {
    Lists table;
    for (svtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType original;
      VectorType raiseresult;
      for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
      {
        ComponentType x = static_cast<ComponentType>(table.NumberList(componentIndex + index));
        Traits::SetComponent(original, componentIndex, x);
        Traits::SetComponent(raiseresult, componentIndex, svtkm::Pow(x, exponent));
      }

      VectorType mathresult = function(original);

      SVTKM_MATH_ASSERT(test_equal(mathresult, raiseresult), "Exponent functions do not agree.");
    }
  }

  struct SqrtFunctor
  {
    SVTKM_EXEC
    VectorType operator()(VectorType x) const { return svtkm::Sqrt(x); }
  };
  SVTKM_EXEC
  void TestSqrt() const
  {
    //    std::cout << "  Testing Sqrt" << std::endl;
    RaiseToTest(SqrtFunctor(), 0.5);
  }

  struct RSqrtFunctor
  {
    SVTKM_EXEC
    VectorType operator()(VectorType x) const { return svtkm::RSqrt(x); }
  };
  SVTKM_EXEC
  void TestRSqrt() const
  {
    //    std::cout << "  Testing RSqrt"<< std::endl;
    RaiseToTest(RSqrtFunctor(), -0.5);
  }

  struct CbrtFunctor
  {
    SVTKM_EXEC
    VectorType operator()(VectorType x) const { return svtkm::Cbrt(x); }
  };
  SVTKM_EXEC
  void TestCbrt() const
  {
    //    std::cout << "  Testing Cbrt" << std::endl;
    RaiseToTest(CbrtFunctor(), svtkm::Float32(1.0 / 3.0));
  }

  struct RCbrtFunctor
  {
    SVTKM_EXEC
    VectorType operator()(VectorType x) const { return svtkm::RCbrt(x); }
  };
  SVTKM_EXEC
  void TestRCbrt() const
  {
    //    std::cout << "  Testing RCbrt" << std::endl;
    RaiseToTest(RCbrtFunctor(), svtkm::Float32(-1.0 / 3.0));
  }

  template <typename FunctionType>
  SVTKM_EXEC void RaiseByTest(FunctionType function,
                             ComponentType base,
                             ComponentType exponentbias = 0.0,
                             ComponentType resultbias = 0.0) const
  {
    Lists table;
    for (svtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType original;
      VectorType raiseresult;
      for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
      {
        ComponentType x = static_cast<ComponentType>(table.NumberList(componentIndex + index));
        Traits::SetComponent(original, componentIndex, x);
        Traits::SetComponent(
          raiseresult, componentIndex, svtkm::Pow(base, x + exponentbias) + resultbias);
      }

      VectorType mathresult = function(original);

      SVTKM_MATH_ASSERT(test_equal(mathresult, raiseresult), "Exponent functions do not agree.");
    }
  }

  struct ExpFunctor
  {
    SVTKM_EXEC
    VectorType operator()(VectorType x) const { return svtkm::Exp(x); }
  };
  SVTKM_EXEC
  void TestExp() const
  {
    //    std::cout << "  Testing Exp" << std::endl;
    RaiseByTest(ExpFunctor(), svtkm::Float32(2.71828183));
  }

  struct Exp2Functor
  {
    SVTKM_EXEC
    VectorType operator()(VectorType x) const { return svtkm::Exp2(x); }
  };
  SVTKM_EXEC
  void TestExp2() const
  {
    //    std::cout << "  Testing Exp2" << std::endl;
    RaiseByTest(Exp2Functor(), 2.0);
  }

  struct ExpM1Functor
  {
    SVTKM_EXEC
    VectorType operator()(VectorType x) const { return svtkm::ExpM1(x); }
  };
  SVTKM_EXEC
  void TestExpM1() const
  {
    //    std::cout << "  Testing ExpM1" << std::endl;
    RaiseByTest(ExpM1Functor(), ComponentType(2.71828183), 0.0, -1.0);
  }

  struct Exp10Functor
  {
    SVTKM_EXEC
    VectorType operator()(VectorType x) const { return svtkm::Exp10(x); }
  };
  SVTKM_EXEC
  void TestExp10() const
  {
    //    std::cout << "  Testing Exp10" << std::endl;
    RaiseByTest(Exp10Functor(), 10.0);
  }

  template <typename FunctionType>
  SVTKM_EXEC void LogBaseTest(FunctionType function,
                             ComponentType base,
                             ComponentType bias = 0.0) const
  {
    Lists table;
    for (svtkm::IdComponent index = 0; index < Lists::NUM_NUMBERS - NUM_COMPONENTS + 1; index++)
    {
      VectorType basevector(base);
      VectorType original;
      VectorType biased;
      for (svtkm::IdComponent componentIndex = 0; componentIndex < NUM_COMPONENTS; componentIndex++)
      {
        ComponentType x = static_cast<ComponentType>(table.NumberList(componentIndex + index));
        Traits::SetComponent(original, componentIndex, x);
        Traits::SetComponent(biased, componentIndex, x + bias);
      }

      VectorType logresult = svtkm::Log2(biased) / svtkm::Log2(basevector);

      VectorType mathresult = function(original);

      SVTKM_MATH_ASSERT(test_equal(mathresult, logresult), "Exponent functions do not agree.");
    }
  }

  struct LogFunctor
  {
    SVTKM_EXEC
    VectorType operator()(VectorType x) const { return svtkm::Log(x); }
  };
  SVTKM_EXEC
  void TestLog() const
  {
    //    std::cout << "  Testing Log" << std::endl;
    LogBaseTest(LogFunctor(), svtkm::Float32(2.71828183));
  }

  struct Log10Functor
  {
    SVTKM_EXEC
    VectorType operator()(VectorType x) const { return svtkm::Log10(x); }
  };
  SVTKM_EXEC
  void TestLog10() const
  {
    //    std::cout << "  Testing Log10" << std::endl;
    LogBaseTest(Log10Functor(), 10.0);
  }

  struct Log1PFunctor
  {
    SVTKM_EXEC
    VectorType operator()(VectorType x) const { return svtkm::Log1P(x); }
  };
  SVTKM_EXEC
  void TestLog1P() const
  {
    //    std::cout << "  Testing Log1P" << std::endl;
    LogBaseTest(Log1PFunctor(), ComponentType(2.71828183), 1.0);
  }

  SVTKM_EXEC
  void TestCopySign() const
  {
    //    std::cout << "Testing CopySign." << std::endl;
    // Assuming all TestValues positive.
    VectorType positive1 = TestValue(1, VectorType());
    VectorType positive2 = TestValue(2, VectorType());
    VectorType negative1 = -positive1;
    VectorType negative2 = -positive2;

    SVTKM_MATH_ASSERT(test_equal(svtkm::CopySign(positive1, positive2), positive1),
                     "CopySign failed.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::CopySign(negative1, positive2), positive1),
                     "CopySign failed.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::CopySign(positive1, negative2), negative1),
                     "CopySign failed.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::CopySign(negative1, negative2), negative1),
                     "CopySign failed.");
  }

  SVTKM_EXEC
  void operator()(svtkm::Id) const
  {
    this->TestTriangleTrig();
    this->TestHyperbolicTrig();
    this->TestSqrt();
    this->TestRSqrt();
    this->TestCbrt();
    this->TestRCbrt();
    this->TestExp();
    this->TestExp2();
    this->TestExpM1();
    this->TestExp10();
    this->TestLog();
    this->TestLog10();
    this->TestLog1P();
    this->TestCopySign();
  }
};

template <typename Device>
struct TryScalarVectorFieldTests
{
  template <typename VectorType>
  void operator()(const VectorType&) const
  {
    svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(ScalarVectorFieldTests<VectorType>(), 1);
  }
};

//-----------------------------------------------------------------------------
template <typename T>
struct AllTypesTests : public svtkm::exec::FunctorBase
{
  SVTKM_EXEC
  void TestMinMax() const
  {
    T low = TestValue(2, T());
    T high = TestValue(10, T());
    //    std::cout << "Testing min/max " << low << " " << high << std::endl;
    SVTKM_MATH_ASSERT(test_equal(svtkm::Min(low, high), low), "Wrong min.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::Min(high, low), low), "Wrong min.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::Max(low, high), high), "Wrong max.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::Max(high, low), high), "Wrong max.");

    using Traits = svtkm::VecTraits<T>;
    T mixed1 = low;
    T mixed2 = high;
    Traits::SetComponent(mixed1, 0, Traits::GetComponent(high, 0));
    Traits::SetComponent(mixed2, 0, Traits::GetComponent(low, 0));
    SVTKM_MATH_ASSERT(test_equal(svtkm::Min(mixed1, mixed2), low), "Wrong min.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::Min(mixed2, mixed1), low), "Wrong min.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::Max(mixed1, mixed2), high), "Wrong max.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::Max(mixed2, mixed1), high), "Wrong max.");
  }

  SVTKM_EXEC
  void operator()(svtkm::Id) const { this->TestMinMax(); }
};

template <typename Device>
struct TryAllTypesTests
{
  template <typename T>
  void operator()(const T&) const
  {
    svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(AllTypesTests<T>(), 1);
  }
};

//-----------------------------------------------------------------------------
template <typename T>
struct AbsTests : public svtkm::exec::FunctorBase
{
  SVTKM_EXEC
  void operator()(svtkm::Id index) const
  {
    //    std::cout << "Testing Abs." << std::endl;
    T positive = TestValue(index, T()); // Assuming all TestValues positive.
    T negative = -positive;

    SVTKM_MATH_ASSERT(test_equal(svtkm::Abs(positive), positive), "Abs returned wrong value.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::Abs(negative), positive), "Abs returned wrong value.");
  }
};

template <typename Device>
struct TryAbsTests
{
  template <typename T>
  void operator()(const T&) const
  {
    svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(AbsTests<T>(), 10);
  }
};

using TypeListAbs =
  svtkm::ListAppend<svtkm::List<svtkm::Int32, svtkm::Int64>, svtkm::TypeListIndex, svtkm::TypeListField>;

//-----------------------------------------------------------------------------
static constexpr svtkm::Id BitOpSamples = 1024 * 1024;

template <typename T>
struct BitOpTests : public svtkm::exec::FunctorBase
{
  static constexpr T MaxT = std::numeric_limits<T>::max();
  static constexpr T Offset = MaxT / BitOpSamples;

  SVTKM_EXEC void operator()(svtkm::Id i) const
  {
    const T idx = static_cast<T>(i);
    const T word = idx * this->Offset;

    TestWord(word - idx);
    TestWord(word);
    TestWord(word + idx);
  }

  SVTKM_EXEC void TestWord(T word) const
  {
    SVTKM_MATH_ASSERT(test_equal(svtkm::CountSetBits(word), this->DumbCountBits(word)),
                     "CountBits returned wrong value.");
    SVTKM_MATH_ASSERT(test_equal(svtkm::FindFirstSetBit(word), this->DumbFindFirstSetBit(word)),
                     "FindFirstSetBit returned wrong value.")
  }

  SVTKM_EXEC svtkm::Int32 DumbCountBits(T word) const
  {
    svtkm::Int32 bits = 0;
    while (word)
    {
      if (word & 0x1)
      {
        ++bits;
      }
      word >>= 1;
    }
    return bits;
  }

  SVTKM_EXEC svtkm::Int32 DumbFindFirstSetBit(T word) const
  {
    if (word == 0)
    {
      return 0;
    }

    svtkm::Int32 bit = 1;
    while ((word & 0x1) == 0)
    {
      word >>= 1;
      ++bit;
    }
    return bit;
  }
};

template <typename Device>
struct TryBitOpTests
{
  template <typename T>
  void operator()(const T&) const
  {
    svtkm::cont::DeviceAdapterAlgorithm<Device>::Schedule(BitOpTests<T>(), BitOpSamples);
  }
};

using TypeListBitOp = svtkm::List<svtkm::UInt32, svtkm::UInt64>;

//-----------------------------------------------------------------------------
template <typename Device>
void RunMathTests()
{
  std::cout << "Tests for scalar types." << std::endl;
  svtkm::testing::Testing::TryTypes(TryScalarFieldTests<Device>(), svtkm::TypeListFieldScalar());
  std::cout << "Test for scalar and vector types." << std::endl;
  svtkm::testing::Testing::TryTypes(TryScalarVectorFieldTests<Device>(), svtkm::TypeListField());
  std::cout << "Test for exemplar types." << std::endl;
  svtkm::testing::Testing::TryTypes(TryAllTypesTests<Device>());
  std::cout << "Test all Abs types" << std::endl;
  svtkm::testing::Testing::TryTypes(TryAbsTests<Device>(), TypeListAbs());
  std::cout << "Test all bit operations" << std::endl;
  svtkm::testing::Testing::TryTypes(TryBitOpTests<Device>(), TypeListBitOp());
}

} // namespace UnitTestMathNamespace

#endif //svtk_m_testing_TestingMath_h
