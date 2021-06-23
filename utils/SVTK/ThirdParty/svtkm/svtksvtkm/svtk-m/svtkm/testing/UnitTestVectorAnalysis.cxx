//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/VectorAnalysis.h>

#include <svtkm/Types.h>
#include <svtkm/VecTraits.h>

#include <svtkm/testing/Testing.h>

#include <math.h>

namespace
{

namespace internal
{

template <typename VectorType>
typename svtkm::VecTraits<VectorType>::ComponentType MyMag(const VectorType& vt)
{
  using Traits = svtkm::VecTraits<VectorType>;
  double total = 0.0;
  for (svtkm::IdComponent index = 0; index < Traits::NUM_COMPONENTS; ++index)
  {
    total += Traits::GetComponent(vt, index) * Traits::GetComponent(vt, index);
  }
  return static_cast<typename Traits::ComponentType>(sqrt(total));
}

template <typename VectorType>
VectorType MyNormal(const VectorType& vt)
{
  using Traits = svtkm::VecTraits<VectorType>;
  typename Traits::ComponentType mag = internal::MyMag(vt);
  VectorType temp = vt;
  for (svtkm::IdComponent index = 0; index < Traits::NUM_COMPONENTS; ++index)
  {
    Traits::SetComponent(temp, index, Traits::GetComponent(vt, index) / mag);
  }
  return temp;
}

template <typename T, typename W>
T MyLerp(const T& a, const T& b, const W& w)
{
  return (W(1) - w) * a + w * b;
}
}

template <typename VectorType>
void TestVector(const VectorType& vector)
{
  using ComponentType = typename svtkm::VecTraits<VectorType>::ComponentType;

  std::cout << "Testing " << vector << std::endl;

  //to do have to implement a norm and normalized call to verify the math ones
  //against
  std::cout << "  Magnitude" << std::endl;
  ComponentType magnitude = svtkm::Magnitude(vector);
  ComponentType magnitudeCompare = internal::MyMag(vector);
  SVTKM_TEST_ASSERT(test_equal(magnitude, magnitudeCompare), "Magnitude failed test.");

  std::cout << "  Magnitude squared" << std::endl;
  ComponentType magnitudeSquared = svtkm::MagnitudeSquared(vector);
  SVTKM_TEST_ASSERT(test_equal(magnitude * magnitude, magnitudeSquared),
                   "Magnitude squared test failed.");

  if (magnitudeSquared > 0)
  {
    std::cout << "  Reciprocal magnitude" << std::endl;
    ComponentType rmagnitude = svtkm::RMagnitude(vector);
    SVTKM_TEST_ASSERT(test_equal(1 / magnitude, rmagnitude), "Reciprical magnitude failed.");

    std::cout << "  Normal" << std::endl;
    SVTKM_TEST_ASSERT(test_equal(svtkm::Normal(vector), internal::MyNormal(vector)),
                     "Normalized vector failed test.");

    std::cout << "  Normalize" << std::endl;
    VectorType normalizedVector = vector;
    svtkm::Normalize(normalizedVector);
    SVTKM_TEST_ASSERT(test_equal(normalizedVector, internal::MyNormal(vector)),
                     "Inplace Normalized vector failed test.");
  }
}

template <typename VectorType>
void TestLerp(const VectorType& a,
              const VectorType& b,
              const VectorType& w,
              const typename svtkm::VecTraits<VectorType>::ComponentType& wS)
{
  std::cout << "Linear interpolation: " << a << "-" << b << ": " << w << std::endl;
  VectorType svtkmLerp = svtkm::Lerp(a, b, w);
  VectorType otherLerp = internal::MyLerp(a, b, w);
  SVTKM_TEST_ASSERT(test_equal(svtkmLerp, otherLerp),
                   "Vectors with Vector weight do not lerp() correctly");

  std::cout << "Linear interpolation: " << a << "-" << b << ": " << wS << std::endl;
  VectorType lhsS = internal::MyLerp(a, b, wS);
  VectorType rhsS = svtkm::Lerp(a, b, wS);
  SVTKM_TEST_ASSERT(test_equal(lhsS, rhsS), "Vectors with Scalar weight do not lerp() correctly");
}

template <typename T>
void TestCross(const svtkm::Vec<T, 3>& x, const svtkm::Vec<T, 3>& y)
{
  std::cout << "Testing " << x << " x " << y << std::endl;

  using Vec3 = svtkm::Vec<T, 3>;
  Vec3 cross = svtkm::Cross(x, y);

  std::cout << "  = " << cross << std::endl;

  std::cout << "  Orthogonality" << std::endl;
  // The cross product result should be perpendicular to input vectors.
  SVTKM_TEST_ASSERT(test_equal(svtkm::Dot(cross, x), T(0.0)), "Cross product not perpendicular.");
  SVTKM_TEST_ASSERT(test_equal(svtkm::Dot(cross, y), T(0.0)), "Cross product not perpendicular.");

  std::cout << "  Length" << std::endl;
  // The length of cross product should be the lengths of the input vectors
  // times the sin of the angle between them.
  T sinAngle = svtkm::Magnitude(cross) * svtkm::RMagnitude(x) * svtkm::RMagnitude(y);

  // The dot product is likewise the lengths of the input vectors times the
  // cos of the angle between them.
  T cosAngle = svtkm::Dot(x, y) * svtkm::RMagnitude(x) * svtkm::RMagnitude(y);

  // Test that these are the actual sin and cos of the same angle with a
  // basic trigonometric identity.
  SVTKM_TEST_ASSERT(test_equal(sinAngle * sinAngle + cosAngle * cosAngle, T(1.0)),
                   "Bad cross product length.");

  std::cout << "  Triangle normal" << std::endl;
  // Test finding the normal to a triangle (similar to cross product).
  Vec3 normal = svtkm::TriangleNormal(x, y, Vec3(0, 0, 0));
  SVTKM_TEST_ASSERT(test_equal(svtkm::Dot(normal, x - y), T(0.0)),
                   "Triangle normal is not really normal.");
}

template <typename VectorBasisType>
void TestOrthonormalize(const VectorBasisType& inputs, int expectedRank)
{
  VectorBasisType outputs;
  int actualRank = svtkm::Orthonormalize(inputs, outputs);
  std::cout << "Testing orthonormalize\n"
            << "  Rank " << actualRank << " expected " << expectedRank << "\n"
            << "  Basis vectors:\n";
  for (int i = 0; i < actualRank; ++i)
  {
    std::cout << "    " << i << "  " << outputs[i] << "\n";
  }
  SVTKM_TEST_ASSERT(test_equal(actualRank, expectedRank), "Orthonormalized rank is unexpected.");
}

struct TestLinearFunctor
{
  template <typename T>
  void operator()(const T&) const
  {
    using Traits = svtkm::VecTraits<T>;
    const svtkm::IdComponent NUM_COMPONENTS = Traits::NUM_COMPONENTS;
    using ComponentType = typename Traits::ComponentType;

    T zeroVector = T(ComponentType(0));
    T normalizedVector = T(svtkm::RSqrt(ComponentType(NUM_COMPONENTS)));
    T posVec = TestValue(1, T());
    T negVec = -TestValue(2, T());

    TestVector(zeroVector);
    TestVector(normalizedVector);
    TestVector(posVec);
    TestVector(negVec);

    T weight(ComponentType(0.5));
    ComponentType weightS(0.5);
    TestLerp(zeroVector, normalizedVector, weight, weightS);
    TestLerp(zeroVector, posVec, weight, weightS);
    TestLerp(zeroVector, negVec, weight, weightS);

    TestLerp(normalizedVector, zeroVector, weight, weightS);
    TestLerp(normalizedVector, posVec, weight, weightS);
    TestLerp(normalizedVector, negVec, weight, weightS);

    TestLerp(posVec, zeroVector, weight, weightS);
    TestLerp(posVec, normalizedVector, weight, weightS);
    TestLerp(posVec, negVec, weight, weightS);

    TestLerp(negVec, zeroVector, weight, weightS);
    TestLerp(negVec, normalizedVector, weight, weightS);
    TestLerp(negVec, posVec, weight, weightS);
  }
};

struct TestCrossFunctor
{
  template <typename VectorType>
  void operator()(const VectorType&) const
  {
    TestCross(VectorType(1.0f, 0.0f, 0.0f), VectorType(0.0f, 1.0f, 0.0f));
    TestCross(VectorType(1.0f, 2.0f, 3.0f), VectorType(-3.0f, -1.0f, 1.0f));
    TestCross(VectorType(0.0f, 0.0f, 1.0f), VectorType(0.001f, 0.01f, 2.0f));
  }
};

struct TestVectorFunctor
{
  template <typename VectorType>
  void operator()(const VectorType&) const
  {
    constexpr int NUM_COMPONENTS = VectorType::NUM_COMPONENTS;
    using Traits = svtkm::VecTraits<VectorType>;
    using ComponentType = typename Traits::ComponentType;
    svtkm::Vec<VectorType, NUM_COMPONENTS> basis;
    VectorType normalizedVector = VectorType(svtkm::RSqrt(ComponentType(NUM_COMPONENTS)));
    VectorType zeroVector = VectorType(ComponentType(0));
    // Test with a degenerate set of inputs:
    basis[0] = zeroVector;
    basis[1] = normalizedVector;
    for (int ii = 2; ii < NUM_COMPONENTS; ++ii)
    {
      basis[ii] = zeroVector;
    }
    TestOrthonormalize(basis, 1);
    // Now test with a valid set of inputs:
    for (int ii = 0; ii < NUM_COMPONENTS; ++ii)
    {
      for (int jj = 0; jj < NUM_COMPONENTS; ++jj)
      {
        basis[ii][jj] =
          ComponentType(jj == ii ? 1.0 : 0.0) + ComponentType(0.05) * ComponentType(jj);
      }
    }
    TestOrthonormalize(basis, NUM_COMPONENTS);
  }
};

void TestVectorAnalysis()
{
  svtkm::testing::Testing::TryTypes(TestLinearFunctor(), svtkm::TypeListField());
  svtkm::testing::Testing::TryTypes(TestCrossFunctor(), svtkm::TypeListFieldVec3());
  svtkm::testing::Testing::TryTypes(TestVectorFunctor(), svtkm::TypeListFloatVec());
}

} // anonymous namespace

int UnitTestVectorAnalysis(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(TestVectorAnalysis, argc, argv);
}
