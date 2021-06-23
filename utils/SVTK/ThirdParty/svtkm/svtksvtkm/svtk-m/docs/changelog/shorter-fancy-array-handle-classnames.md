# Shorter fancy array handle classnames

Many of the fancy `ArrayHandle`s use the generic builders like
`ArrayHandleTransform` and `ArrayHandleImplicit` for their implementation.
Such is fine, but because they use functors and other such generic items to
template their `Storage`, you can end up with very verbose classnames. This
is an issue for humans trying to discern classnames. It can also be an
issue for compilers that end up with very long resolved classnames that
might get truncated if they extend past what was expected.

The fix was for these classes to declare their own `Storage` tag and then
implement their `Storage` and `ArrayTransport` classes as trivial
subclasses of the generic `ArrayHandleImplicit` or `ArrayHandleTransport`.

As an added bonus, a lot of this shortening also means that storage that
relies on other array handles now are just typed by the storage of the
decorated type, not the array itself. This should make the types a little
more robust.

Here is a list of classes that were updated.

#### `ArrayHandleCast<TargetT, svtkm::cont::ArrayHandle<SourceT, SourceStorage>>`

Old storage: 
``` cpp
svtkm::cont::internal::StorageTagTransform<
  svtkm::cont::ArrayHandle<SourceT, SourceStorage>,
  svtkm::cont::internal::Cast<TargetT, SourceT>,
  svtkm::cont::internal::Cast<SourceT, TargetT>>
```

New Storage:
``` cpp
svtkm::cont::StorageTagCast<SourceT, SourceStorage>
```

(Developer's note: Implementing this change to `ArrayHandleCast` was a much bigger PITA than expected.)

#### `ArrayHandleCartesianProduct<AH1, AH2, AH3>`

Old storage:
``` cpp
svtkm::cont::internal::StorageTagCartesianProduct<
  svtkm::cont::ArrayHandle<ValueType, StorageTag1,
  svtkm::cont::ArrayHandle<ValueType, StorageTag2,
  svtkm::cont::ArrayHandle<ValueType, StorageTag3>>
```

New storage:
``` cpp
svtkm::cont::StorageTagCartesianProduct<StorageTag1, StorageTag2, StorageTag3>
```

#### `ArrayHandleCompositeVector<AH1, AH2, ...>`

Old storage:
``` cpp
svtkm::cont::internal::StorageTagCompositeVector<
  tao::tuple<
    svtkm::cont::ArrayHandle<ValueType, StorageType1>, 
	svtkm::cont::ArrayHandle<ValueType, StorageType2>,
	...
  >
>
```

New storage:
``` cpp
svtkm::cont::StorageTagCompositeVec<StorageType1, StorageType2>
```

#### `ArrayHandleConcatinate`

First an example with two simple types.

Old storage:
``` cpp
svtkm::cont::StorageTagConcatenate<
  svtkm::cont::ArrayHandle<ValueType, StorageTag1>,
  svtkm::cont::ArrayHandle<ValueType, StorageTag2>>
```

New storage:
``` cpp
svtkm::cont::StorageTagConcatenate<StorageTag1, StorageTag2>
```

Now a more specific example taken from the unit test of a concatination of a concatination.

Old storage:
``` cpp
svtkm::cont::StorageTagConcatenate<
  svtkm::cont::ArrayHandleConcatenate<
    svtkm::cont::ArrayHandle<ValueType, StorageTag1>,
	svtkm::cont::ArrayHandle<ValueType, StorageTag2>>,
  svtkm::cont::ArrayHandle<ValueType, StorageTag3>>
```

New storage:
``` cpp
svtkm::cont::StorageTagConcatenate<
  svtkm::cont::StorageTagConcatenate<StorageTag1, StorageTag2>, StorageTag3>
```

#### `ArrayHandleConstant`

Old storage:
``` cpp
svtkm::cont::StorageTagImplicit<
  svtkm::cont::detail::ArrayPortalImplicit<
    svtkm::cont::detail::ConstantFunctor<ValueType>>>
```

New storage:
``` cpp
svtkm::cont::StorageTagConstant
```

#### `ArrayHandleCounting`

Old storage:
``` cpp
svtkm::cont::StorageTagImplicit<svtkm::cont::internal::ArrayPortalCounting<ValueType>>
```

New storage:
``` cpp
svtkm::cont::StorageTagCounting
```

#### `ArrayHandleGroupVec`

Old storage:
``` cpp
svtkm::cont::internal::StorageTagGroupVec<
  svtkm::cont::ArrayHandle<ValueType, StorageTag>, N>
```

New storage:
``` cpp
svtkm::cont::StorageTagGroupVec<StorageTag, N>
```

#### `ArrayHandleGroupVecVariable`

Old storage:
``` cpp
svtkm::cont::internal::StorageTagGroupVecVariable<
  svtkm::cont::ArrayHandle<ValueType, StorageTag1>, 
  svtkm::cont::ArrayHandle<svtkm::Id, StorageTag2>>
```

New storage:
``` cpp
svtkm::cont::StorageTagGroupVecVariable<StorageTag1, StorageTag2>
```

#### `ArrayHandleIndex`

Old storage:
``` cpp
svtkm::cont::StorageTagImplicit<
  svtkm::cont::detail::ArrayPortalImplicit<svtkm::cont::detail::IndexFunctor>>
```

New storage:
``` cpp
svtkm::cont::StorageTagIndex
```

#### `ArrayHandlePermutation`

Old storage:
``` cpp
svtkm::cont::internal::StorageTagPermutation<
  svtkm::cont::ArrayHandle<svtkm::Id, StorageTag1>,
  svtkm::cont::ArrayHandle<ValueType, StorageTag2>>
```

New storage:
``` cpp
svtkm::cont::StorageTagPermutation<StorageTag1, StorageTag2>
```

#### `ArrayHandleReverse`

Old storage:
``` cpp
svtkm::cont::StorageTagReverse<svtkm::cont::ArrayHandle<ValueType, svtkm::cont::StorageTag>>
```

New storage:
``` cpp
svtkm::cont::StorageTagReverse<StorageTag>
```

#### `ArrayHandleUniformPointCoordinates`

Old storage:
``` cpp
svtkm::cont::StorageTagImplicit<svtkm::internal::ArrayPortalUniformPointCoordinates>
```

New Storage:
``` cpp
svtkm::cont::StorageTagUniformPoints
```

#### `ArrayHandleView`

Old storage:
``` cpp
svtkm::cont::StorageTagView<svtkm::cont::ArrayHandle<ValueType, StorageTag>>
```

New storage:
``` cpp
'svtkm::cont::StorageTagView<StorageTag>
```


#### `ArrayPortalZip`

Old storage:
``` cpp
svtkm::cont::internal::StorageTagZip<
  svtkm::cont::ArrayHandle<ValueType1, StorageTag1>,
  svtkm::cont::ArrayHandle<ValueType2, StorageTag2>>
```

New storage:
``` cpp
svtkm::cont::StorageTagZip<StorageTag1, StorageTag2>
```
