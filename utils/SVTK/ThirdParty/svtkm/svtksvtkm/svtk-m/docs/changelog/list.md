# Replaced `svtkm::ListTag` with `svtkm::List`

The original `svtkm::ListTag` was designed when we had to support compilers
that did not provide C++11's variadic templates. Thus, the design hides
type lists, which were complicated to support.

Now that we support C++11, variadic templates are trivial and we can easily
create templated type aliases with `using`. Thus, it is now simpler to deal
with a template that lists types directly.

Hence, `svtkm::ListTag` is deprecated and `svtkm::List` is now supported. The
main difference between the two is that whereas `svtkm::ListTag` allowed you
to create a list by subclassing another list, `svtkm::List` cannot be
subclassed. (Well, it can be subclassed, but the subclass ceases to be
considered a list.) Thus, where before you would declare a list like

``` cpp
struct MyList : svtkm::ListTagBase<Type1, Type2, Type3>
{
};
```

you now make an alias

``` cpp
using MyList = svtkm::List<Type1, Type2, Type3>;
```

If the compiler reports the `MyList` type in an error or warning, it
actually uses the fully qualified `svtkm::List<Type1, Type2, Type3>`.
Although this makes errors more verbose, it makes it easier to diagnose
problems because the types are explicitly listed.

The new `svtkm::List` comes with a list of utility templates to manipulate
lists that mostly mirrors those in `svtkm::ListTag`: `SVTKM_IS_LIST`,
`ListApply`, `ListSize`, `ListAt`, `ListIndexOf`, `ListHas`, `ListAppend`,
`ListIntersect`, `ListTransform`, `ListRemoveIf`, and `ListCross`. All of
these utilities become `svtkm::List<>` types (where applicable), which makes
them more consistent than the old `svtkm::ListTag` versions.

Thus, if you have a declaration like

``` cpp
svtkm::ListAppend(svtkm::List<Type1a, Type2a>, svtkm::List<Type1b, Type2b>>
```

this gets changed automatically to

``` cpp
svtkm::List<Type1a, Type2a, Type1b, Type2b>
```

This is in contrast to the equivalent old version, which would create a new
type for `svtkm::ListTagAppend` in addition to the ultimate actual list it
constructs.
