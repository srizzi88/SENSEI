# Add SVTKM_DEPRECATED macro

The `SVTKM_DEPRECATED` macro allows us to remove (and usually replace)
features from SVTK-m in minor releases while still following the conventions
of semantic versioning. The idea is that when we want to remove or replace
a feature, we first mark the old feature as deprecated. The old feature
will continue to work, but compilers that support it will start to issue a
warning that the use is deprecated and should stop being used. The
deprecated features should remain viable until at least the next major
version. At the next major version, deprecated features from the previous
version may be removed.

## Declaring things deprecated

Classes and methods are marked deprecated using the `SVTKM_DEPRECATED`
macro. The first argument of `SVTKM_DEPRECATED` should be set to the first
version in which the feature is deprecated. For example, if the last
released version of SVTK-m was 1.5, and on the master branch a developer
wants to deprecate a class foo, then the `SVTKM_DEPRECATED` release version
should be given as 1.6, which will be the next minor release of SVTK-m. The
second argument of `SVTKM_DEPRECATED`, which is optional but highly
encouraged, is a short message that should clue developers on how to update
their code to the new changes. For example, it could point to the
replacement class or method for the changed feature.

`SVTKM_DEPRECATED` can be used to deprecate a class by adding it between the
`struct` or `class` keyword and the class name.

``` cpp
struct SVTKM_DEPRECATED(1.6, "OldClass replaced with NewClass.") OldClass
{
};
```

Aliases can similarly be depreciated, except the `SVTKM_DEPRECATED` macro
goes after the name in this case.

``` cpp
using OldAlias SVTKM_DEPRECATED(1.6, "Use NewClass instead.") = NewClass;
```

Functions and methods are marked as deprecated by adding `SVTKM_DEPRECATED`
as a modifier before the return value.

``` cpp
SVTKM_EXEC_CONT
SVTKM_DEPRECATED(1.6, "You must now specify a tolerance.") void ImportantMethod(double x)
{
  this->ImportantMethod(x, 1e-6);
}
```

`enum`s can be deprecated like classes using similar syntax.

``` cpp
enum struct SVTKM_DEPRECATED(1.7, "Use NewEnum instead.") OldEnum
{
  OLD_VALUE
};
```

Individual items in an `enum` can also be marked as deprecated and
intermixed with regular items.

``` cpp
enum struct NewEnum
{
  OLD_VALUE1 SVTKM_DEPRECATED(1.7, "Use NEW_VALUE instead."),
  NEW_VALUE,
  OLD_VALUE2 SVTKM_DEPRECATED(1.7) = 42
};
```

## Using deprecated items

Using deprecated items should work, but the compiler will give a warning.
That is the point. However, sometimes you need to legitimately use a
deprecated item without a warning. This is usually because you are
implementing another deprecated item or because you have a test for a
deprecated item (that can be easily removed with the deprecated bit). To
support this a pair of macros, `SVTKM_DEPRECATED_SUPPRESS_BEGIN` and
`SVTKM_DEPRECATED_SUPPRESS_END` are provided. Code that legitimately uses
deprecated items should be wrapped in these macros.

``` cpp
SVTKM_EXEC_CONT
SVTKM_DEPRECATED(1.6, "You must now specify both a value and tolerance.")
void ImportantMethod()
{
  // It can be the case that to implement a deprecated method you need to
  // use other deprecated features. To do that, just temporarily suppress
  // those warnings.
  SVTKM_DEPRECATED_SUPPRESS_BEGIN
  this->ImportantMethod(0.0);
  SVTKM_DEPRECATED_SUPPRESS_END
}
```
