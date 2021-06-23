//============================================================================
//  Copyright (c) Kitware, Inc.
//  All rights reserved.
//  See LICENSE.txt for details.
//
//  This software is distributed WITHOUT ANY WARRANTY; without even
//  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
//  PURPOSE.  See the above copyright notice for more information.
//============================================================================

#include <svtkm/Deprecated.h>

#include <svtkm/testing/Testing.h>

namespace
{

struct NewClass
{
  SVTKM_EXEC_CONT
  void ImportantMethod(double x, double tolerance)
  {
    std::cout << "Using " << x << " with tolerance " << tolerance << std::endl;
  }

  SVTKM_EXEC_CONT
  SVTKM_DEPRECATED(1.7, "You must now specify a tolerance.") void ImportantMethod(double x)
  {
    this->ImportantMethod(x, 1e-6);
  }

  SVTKM_EXEC_CONT
  SVTKM_DEPRECATED(1.6, "You must now specify both a value and tolerance.")
  void ImportantMethod()
  {
    // It can be the case that to implement a deprecated method you need to use other
    // deprecated features. To do that, just temporarily suppress those warnings.
    SVTKM_DEPRECATED_SUPPRESS_BEGIN
    this->ImportantMethod(0.0);
    SVTKM_DEPRECATED_SUPPRESS_END
  }
};

struct SVTKM_DEPRECATED(1.6, "OldClass replaced with NewClass.") OldClass
{
};

using OldAlias SVTKM_DEPRECATED(1.6, "Use NewClass instead.") = NewClass;

// Should be OK for one deprecated alias to use another deprecated thing, but most compilers
// do not think so. So, when implementing deprecated things, you might need to suppress
// warnings for that part of the code.
SVTKM_DEPRECATED_SUPPRESS_BEGIN
using OlderAlias SVTKM_DEPRECATED(1.6, "Update your code to NewClass.") = OldAlias;
SVTKM_DEPRECATED_SUPPRESS_END

enum struct SVTKM_DEPRECATED(1.7, "Use NewEnum instead.") OldEnum
{
  OLD_VALUE
};

enum struct NewEnum
{
  OLD_VALUE1 SVTKM_DEPRECATED(1.7, "Use NEW_VALUE instead."),
  NEW_VALUE,
  OLD_VALUE2 SVTKM_DEPRECATED(1.7) = 42
};

template <typename T>
void DoSomethingWithObject(T)
{
  std::cout << "Looking at " << typeid(T).name() << std::endl;
}

static void DoTest()
{
  std::cout << "C++14 [[deprecated]] supported: "
#ifdef SVTK_M_DEPRECATED_ATTRIBUTE_SUPPORTED
            << "yes"
#else
            << "no"
#endif
            << std::endl;
  std::cout << "Deprecated warnings can be suppressed: "
#ifdef SVTKM_DEPRECATED_SUPPRESS_SUPPORTED
            << "yes"
#else
            << "no"
#endif
            << std::endl;
  std::cout << "Deprecation is: " << SVTKM_STRINGIFY_FIRST(SVTKM_DEPRECATED(X.Y, "Message."))
            << std::endl;

  SVTKM_TEST_ASSERT(test_equal(SVTK_M_DEPRECATED_MAKE_MESSAGE(X.Y), " Deprecated in version X.Y."));
  SVTKM_TEST_ASSERT(test_equal(SVTK_M_DEPRECATED_MAKE_MESSAGE(X.Y.Z, "Use feature foo instead."),
                              "Use feature foo instead. Deprecated in version X.Y.Z."));

  // Using valid classes with unused deprecated parts should be fine.
  NewClass useIt;
  DoSomethingWithObject(useIt);
  useIt.ImportantMethod(1.1, 1e-8);
  DoSomethingWithObject(NewEnum::NEW_VALUE);

// These should each give compiler warnings.
#if 0
  OldClass useOldClass;
  DoSomethingWithObject(useOldClass);
  OldAlias useOldAlias;
  DoSomethingWithObject(useOldAlias);
  OlderAlias useOlderAlias;
  DoSomethingWithObject(useOlderAlias);
  useIt.ImportantMethod(1.1);
  useIt.ImportantMethod();
  DoSomethingWithObject(OldEnum::OLD_VALUE);
  DoSomethingWithObject(NewEnum::OLD_VALUE1);
  DoSomethingWithObject(NewEnum::OLD_VALUE2);
#endif
}

} // anonymous namespace

int UnitTestDeprecated(int argc, char* argv[])
{
  return svtkm::testing::Testing::Run(DoTest, argc, argv);
}
