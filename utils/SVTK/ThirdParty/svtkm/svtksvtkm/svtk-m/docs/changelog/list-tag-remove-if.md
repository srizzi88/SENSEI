# Add `ListTagRemoveIf`

It is sometimes useful to remove types from `ListTag`s. This is especially
the case when combining lists of types together where some of the type
combinations may be invalid and should be removed. To handle this
situation, a new `ListTag` type is added: `ListTagRemoveIf`.

`ListTagRemoveIf` is a template structure that takes two arguments. The
first argument is another `ListTag` type to operate on. The second argument
is a template that acts as a predicate. The predicate takes a type and
declares a Boolean `value` that should be `true` if the type should be
removed and `false` if the type should remain.

Here is an example of using `ListTagRemoveIf` to get basic types that hold
only integral values.

``` cpp
template <typename T>
using IsRealValue =
  std::is_same<
    typename svtkm::TypeTraits<typename svtkm::VecTraits<T>::BaseComponentType>::NumericTag,
    svtkm::TypeTraitsRealTag>;

using MixedTypes =
  svtkm::ListTagBase<svtkm::Id, svtkm::FloatDefault, svtkm::Id3, svtkm::Vec3f>;

using IntegralTypes = svtkm::ListTagRemoveIf<MixedTypes, IsRealValue>;
// IntegralTypes now equivalent to svtkm::ListTagBase<svtkm::Id, svtkm::Id3>
```
