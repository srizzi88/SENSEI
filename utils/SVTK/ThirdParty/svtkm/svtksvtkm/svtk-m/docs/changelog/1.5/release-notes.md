SVTK-m 1.5 Release Notes
=======================

# Table of Contents
1. [Core](#Core)
    - Provide pre-built filters in a `svtkm_filter` library
    - Provide pre-build sources in a `svtkm_source` library
    - Add aliases for common `svtkm::Vec` types
    - Improve `WorkletMapTopology` worklet names
    - `svtkm::cont::DataSet` only contain a single `svtkm::cont::CellSet`
    - `svtkm::cont::MultiBlock` renamed to `svtkm::cont::PartitionedDataSet`
2. [ArrayHandle](#ArrayHandle)
    - Add `svtkm::cont::ArrayGetValues` to retrieve a subset of ArrayHandle values from a device
    - Add `svtkm::cont::ArrayHandleMultiplexer`
    - Add `svtkm::cont::ArrayHandleDecorator`
    - Add `svtkm::cont::ArrayHandleSOA`
    - `svtkm::cont::ArrayHandleCast` is now writeable
    - Remove ArrayPortalShrink, behavior subsumed by `svtkm::cont::ArrayHandleView`
3. [Control Environment](#Control-Environment)
    - `svtkm::cont::CellSetExplicit` refactored to remove redundant array
    - `svtkm::cont::CellSets` now don't have a name field
    - `svtkm::cont::CellLocatorUniformGrid` and `svtkm::cont::CellLocatorRectilinearGrid` support 2D grids
    - `svtkm::cont::DataSet` queries for CoordinateSystem ondices don't throw exceptions
    - `svtkm::cont::Fields` now don't require the associated `svtkm::cont::CellSet` name
    - `Invoker` moved to svtkm::cont
    - Refactor of `svtkm::cont::CellSet` PrepareForInput signatures
    - Simplify creating `svtkm::cont::Fields` from `svtkm::cont::ArrayHandles`
4. [Execution Environment](#Execution-Environment)
    - Corrected cell derivatives for polygon cell shape
    - A `ScanExtended` device algorithm has been added
    - Provide base component queries to `svtkm::VecTraits`
5. [Worklets and Filters](#Worklets-and-Filters)
    - ExecutionSignatures are now optional for simple worklets
    - Refactor topology mappings to clarify meaning
    - Simplify creating results for `svtkm::filter::filters`
    - Provide a simplified way to state allowed value types for `svtkm::filter::filters`
    - `svtkm::cont::Invoker` is now a member of all SVTK-m filters
    - `svtkm::filter::Filter` now don't have an active `svtkm::cont::CellSet`
    - `svtkm::filter::FilterField` now provides all functionality of `svtkm::filter::FilterCell`
    - Add ability to get an array from a `svtkm::cont::Field` for a particular type
    - `svtkm::worklet::WorkletPointNeighborhood` can query exact neighbor offset locations
    - Add Lagrangian Coherent Structures (LCS) Filter for SVTK-m
    - `SurfaceNormals` filter can now orient normals
    - Particle advection components have better status query support
    - `svtkm::filter::Threshold` now outputs a `svtkm::cont::CellSetExplicit`
6. [Build](#Build)
    - Introduce `svtkm_add_target_information` cmake function to make using svtk-m easier
7. [Other](#Other)
    - Simplify examples
    -  `svtkm::Vec const& operator[]` is now `constexpr`


# Core

## Provide pre-built filters in a `svtkm_filter` library

SVTK-m now provides the following pre built versions of
the following filters as part of the svtkm_filter library,
when executed with the default types.
  - CellAverage
  - CleanGrid
  - ClipWithField
  - ClipWithImplicitFunction
  - Contour
  - ExternalFaces
  - ExtractStuctured
  - PointAverage
  - Threshold
  - VectorMagnitude

The decision on providing a subset of filters as a library
was based on balancing the resulting library size and cross domain
applicibaility of the filter. So the initial set of algorithms
have been selected by looking at what is commonly used by
current SVTK-m consuming applications.

By default types we mean that no explicit user policy has been
passed to the `Execute` method on these filters. For example
the following will use the pre-build `Threshold` and `CleanGrid`
filters:

```cpp
  svtkm::cont::DataSet input = ...;

  //convert input to an unstructured grid
  svtkm::filter::CleanGrid clean;
  auto cleaned = clean.Execute(input);

  svtkm::filter::Threshold threshold;
  threshold.SetLowerThreshold(60.1);
  threshold.SetUpperThreshold(60.1);
  threshold.SetActiveField("pointvar");
  threshold.SetFieldsToPass("cellvar");
  auto output = threshold.Execute(cleaned);
  ...
```

## Provide pre-build sources in a `svtkm_source` library

A new class hierarchy for dataset source was added. The intention is to
consolidate and refactor various (procedural) dataset generators for unit
tests, especially the multiple copy&past-ed implementations of the Tangle
field. As they are compiled into a library rather than as header files,
we also expect the overall compile time to decrease.

The public interface of dataset source is modeled after Filter. A new DataSet
is returned by calling the Execute() method of the dataset source, for example:

```cpp
svtkm::Id3 dims(4, 4, 4);
svtkm::source::Tangle tangle(dims);
svtkm::cont::DataSet dataSet = tangle.Execute();
```

## Add aliases for common `svtkm::Vec` types

Specifying `Vec` types can be verbose. For example, to simply express a
vector in 3-space, you would need a declaration like this:

``` cpp
svtkm::Vec<svtkm::FloatDefault, 3>
```

This is very verbose and frankly confusing to users. To make things easier,
we have introduced several aliases for common `Vec` types. For example, the
above type can now be referenced simply with `svtkm::Vec3f`, which is a
3-vec of floating point values of the default width. If you want to specify
the width, then you can use either `svtkm::Vec3f_32` or `svtkm::Vec3f_64`.

There are likewise types introduced for integers and unsigned integers
(e.g. `svtkm::Vec3i` and `svtkm::Vec3ui`). You can specify the width of these
all the way down to 8 bit (e.g. `svtkm::Vec3ui_8`, `svtkm::Vec3ui_16`,
`svtkm::Vec3ui_32`, and `svtkm::Vec3ui_64`).

For completeness, `svtkm::Id4` was added as well as `svtkm::IdComponent2`,
`svtkm::IdComponent3`, and `svtkm::IdComponent4`.

## Improve `WorkletMapTopology` worklet names

The convenience implementations of `WorkletMapTopology` have been renamed for
clarity as follows:

```
WorkletMapPointToCell --> WorkletVisitCellsWithPoints
WorkletMapCellToPoint --> WorkletVisitPointsWithCells
```

## `svtkm::cont::DataSet` only contain a single `svtkm::cont::CellSet`

Multiple `svtkm::cont::CellSets` on a datasets increased the
complexity of using SVTK-m correctly without any significant
benefits.

It had the effect that `svtkm::cont::Fields` that representing
cell fields needed to be associated with a given cellset. This
has to be a loose coupling to allow for filters to generate
new output cellsets. At the same time it introduced errors when
that output had a different name.

It raised questions about how should filters propagate cell fields.
Should a filter drop all cell fields not associated with the active
CellSet, or is that too aggressive given the fact that maybe the
algorithm just mistakenly named the field, or the IO routine added
a field with the wrong cellset name.

It increased the complexity of filters, as the developer needed to
determine if the algorithm should support execution on a single `CellSet` or
execution over all `CellSets`.

Given these issues it was deemed that removing multiple `CellSets` was
the correct way forward. People using multiple `CellSets` will need to
move over to `svtkm::cont::MultiBlock` which supports shared points and
fields between multiple blocks.

## `svtkm::cont::MultiBlock` renamed to `svtkm::cont::PartitionedDataSet`

The `MultiBlock` class has been renamed to `PartitionedDataSet`, and its API
has been refactored to refer to "partitions", rather than "blocks".
Additionally, the `AddBlocks` method has been changed to `AppendPartitions` to
more accurately reflect the operation performed. The associated
`AssignerMultiBlock` class has also been renamed to
`AssignerPartitionedDataSet`.

This change is motivated towards unifying SVTK-m's data model with SVTK. SVTK has
started to move away from `svtkMultiBlockDataSet`, which is a hierarchical tree
of nested datasets, to `svtkPartitionedDataSet`, which is always a flat vector
of datasets used to assist geometry distribution in multi-process environments.
This simplifies traversal during processing and clarifies the intent of the
container: The component datasets are partitions for distribution, not
organizational groupings (e.g. materials).



# ArrayHandle

## Add `svtkm::cont::ArrayGetValues` to retrieve a subset of ArrayHandle values from a device

An algorithm will often want to pull just a single value (or small subset of
values) back from a device to check the results of a computation. Previously,
there was no easy way to do this, and algorithm developers would often
transfer vast quantities of data back to the host just to check a single value.

The new `svtkm::cont::ArrayGetValue` and `svtkm::cont::ArrayGetValues` functions
simplify this operations and provide a method to just retrieve a portion of an
array.

This utility provides several convenient overloads:

A single id may be passed into ArrayGetValue, or multiple ids may be specified
to ArrayGetValues as an ArrayHandle<svtkm::Id>, a std::vector<svtkm::Id>, a
c-array (pointer and size), or as a brace-enclosed initializer list.

The single result from ArrayGetValue may be returned or written to an output
argument. Multiple results from ArrayGetValues may be returned as an
std::vector<T>, or written to an output argument as an ArrayHandle<T> or a
std::vector<T>.

Examples:

```cpp
svtkm::cont::ArrayHandle<T> data = ...;

// Fetch the first value in an array handle:
T firstVal = svtkm::cont::ArrayGetValue(0, data);

// Fetch the first and third values in an array handle:
std::vector<T> firstAndThird = svtkm::cont::ArrayGetValues({0, 2}, data);

// Fetch the first and last values in an array handle:
std::vector<T> firstAndLast =
    svtkm::cont::ArrayGetValues({0, data.GetNumberOfValues() - 1}, data);

// Fetch the first 4 values into an array handle:
const std::vector<svtkm::Id> ids{0, 1, 2, 3};
svtkm::cont::ArrayHandle<T> firstFour;
svtkm::cont::ArrayGetValues(ids, data, firstFour);
```

## Add `svtkm::cont::ArrayHandleMultiplexer`

`svtkm::cont::ArrayHandleMultiplexer` is a fancy `ArrayHandle` that can
mimic being any one of a list of other `ArrayHandle`s. When declared, a set
of a list of `ArrayHandle`s is given to `ArrayHandleMultiplexer`. To use
the `ArrayHandleMultiplexer` it is set to an instance of one of these other
`ArrayHandle`s. Thus, once you compile code to use an
`ArrayHandleMultiplexer`, you can at runtime select any of the types it
supports.

The intention is convert the data from a `svtkm::cont::VariantArrayHandle`
to a `svtkm::cont::ArrayHandleMultiplexer` of some known types. The
`ArrayHandleMultiplexer` can be compiled statically (that is, no virtual
methods are needed). Although the compiler must implement all possible
implementations of the multiplexer, two or more `ArrayHandleMultiplexer`s
can be used together without having to compile every possible combination
of all of them.

### Motivation

`ArrayHandle` is a very flexible templated class that allows us to use the
compiler to adapt our code to pretty much any type of memory layout or
on-line processing. Unfortunately, the template approach requires the code
to know the exact type during compile time.

That is a problem when retrieving data from a
`svtkm::cont::VariantArrayHandle`, which is the case, for example, when
getting data from a `svtkm::cont::DataSet`. The actual type of the array
stored in a `svtkm::cont::VariantArrayHandle` is generally not known at
compile time at the code location where the data is pulled.

Our first approach to this problem was to use metatemplate programming to
iterate over all possible types in the `VariantArrayHandle`. Although this
works, it means that if two or more `VariantArrayHandle`s are dispatched in
a function call, the compiler needs to generate all possible combinations
of the two. This causes long compile times and large executable sizes. It
has lead us to limit the number of types we support, which causes problems
with unsupported arrays.

Our second approach to this problem was to create `ArrayHandleVirtual` to
hide the array type behind a virtual method. This works very well, but is
causing us significant problems on accelerators. Although virtual methods
are supported by CUDA, there are numerous problems that can come up with
the compiled code (such as unknown stack depths or virtual methods
extending across libraries). It is also unknown what problems we will
encounter with other accelerator architectures.

`ArrayHandleMultiplexer` is meant to be a compromise between these two
approaches. Although we are still using metatemplate programming tricks to
iterate over multiple implementations, this compiler looping is localized
to the code to lookup values in the array. This, it is a small amount of
code that needs to be created for each version supported by the
`ArrayHandle`. Also, the code paths can be created independently for each
`ArrayHandleMultiplexer`. Thus, you do not get into the problem of a
combinatorial explosion of types that need to be addressed.

Although `ArrayHandleMultiplexer` still has the problem of being unable to
store a type that is not explicitly listed, the localized expression should
allow us to support many types. By default, we are adding lots of
`ArrayHandleCast`s to the list of supported types. The intention of this is
to allow a filter to specify a value type it operates on and then cast
everything to that type. This further allows us to reduce combination of
types that we have to support.

### Use

The `ArrayHandleMultiplexer` templated class takes a variable number of
template parameters. All the template parameters are expected to be types
of `ArrayHandle`s that the `ArrayHandleMultiplexer` can assume.

For example, let's say we have a use case where we need an array of
indices. Normally, the indices are sequential (0, 1, 2,...), but sometimes
we need to define a custom set of indices. When the indices are sequential,
then an `ArrayHandleIndex` is the best representation. Normally if you also
need to support general arrays you would first have to deep copy the
indices into a physical array. However, with an `ArrayHandleMultiplexer`
you can support both.

``` cpp
svtkm::cont::ArrayHandleMultiplexer<svtkm::cont::ArrayHandleIndex,
                                   svtkm::cont::ArrayHandle<svtkm::Id>> indices;
indices = svtkm::cont::ArrayHandleIndex(ARRAY_SIZE);
```

`indices` can now be used like any other `ArrayHandle`, but for now is
behaving like an `ArrayHandleIndex`. That is, it takes (almost) no actual
space. But if you need to use explicit indices, you can set the `indices`
array to an actual array of indices

``` cpp
svtkm::cont::ArrayHandle<svtkm::Id> indicesInMemory;
// Fill indicesInMemory...

indices = indicesInMemory;
```

All the code that uses `indices` will continue to work.

### Variant

To implement `ArrayHandleMultiplexer`, the class `svtkm::internal::Variant`
was introduced. Although this is an internal class that is not exposed
through the array handle, it is worth documenting its addition as it will
be useful to implement other multiplexing type of objects (such as for
cell sets and locators).

`svtkm::internal::Variant` is a simplified version of C++17's `std::variant`
or boost's `variant`. One of the significant differences between SVTK-m's
`Variant` and these other versions is that SVTK-m's version does not throw
exceptions on error. Instead, behavior becomes undefined. This is
intentional as not all platforms support exceptions and there could be
consequences on just the possibility for those that do.

Like the aforementioned classes that `svtkm::internal::Variant` is based on,
it behaves much like a `union` of a set of types. Those types are listed as
the `Variant`'s template parameters. The `Variant` can be set to any one of
these types either through construction or assignment. You can also use the
`Emplace` method to construct the object in a `Variant`.

``` cpp
svtkm::internal::Variant<int, float, std::string> variant(5);
// variant is now an int.

variant = 5.0f;
// variant is now a float.

variant.Emplace<std::string>("Hello world");
// variant is now an std::string.
```

The `Variant` maintains the index of which type it is holding. It has
several helpful items to manage the type and index of contained objects:

  * `GetIndex()`: A method to retrieve the template parameter index of the
    type currently held. In the previous example, the index starts at 0,
    becomes 1, then becomes 2.
  * `GetIndexOf<T>()`: A static method that returns a `constexpr` of the
    index of a given type. In the previous example,
    `variant.GetIndexOf<float>()` would return 1.
  * `Get<T or I>()`: Given a type, returns the contained object as that
    type. Given a number, returns the contained object as a type of the
    corresponding index. In the previous example, either `variant.Get<1>()`
    or `variant.Get<float>()` would return the `float` value. The behavior
    is undefined if the object is not the requested type.
  * `IsValid()`: A method that can be used to determine whether the
    `Variant` holds an object that can be operated on.
  * `Reset()`: A method to remove any contained object and restore to an
    invalid state.

Finally, `Variant` contains a `CastAndCall` method. This method takes a
functor followed by a list of optional arguments. The contained object is
cast to the appropriate type and the functor is called with the cast object
followed by the provided arguments. If the functor returns a value, that
value is returned by `CastAndCall`.

`CastAndCall` is an important functionality that makes it easy to wrap
multiplexer objects around a `Variant`. For example, here is how you could
implement executing the `Value` method in an implicit function multiplexer.

``` cpp
class ImplicitFunctionMultiplexer
{
  svtkm::internal::Variant<Box, Plane, Sphere> ImplicitFunctionVariant;

  // ...

  struct ValueFunctor
  {
    template <typename ImplicitFunctionType>
	svtkm::FloatDefault operator()(const ImplicitFunctionType& implicitFunction,
	                              const svtkm::Vec<svtkm::FloatDefault, 3>& point)
	{
	  return implicitFunction.Value(point);
	}
  };

  svtkm::FloatDefault Value(const svtkm::Vec<svtkm::FloatDefault, 3>& point) const
  {
    return this->ImplicitFunctionVariant.CastAndCall(ValueFunctor{}, point);
  }

```

## Add `svtkm::cont::ArrayHandleDecorator`

`ArrayHandleDecorator` is given a `DecoratorImpl` class and a list of one or
more source `ArrayHandle`s. There are no restrictions on the size or type of
the source `ArrayHandle`s.


The decorator implementation class is described below:

```cpp
struct ExampleDecoratorImplementation
{

  // Takes one portal for each source array handle (only two shown).
  // Returns a functor that defines:
  //
  // ValueType operator()(svtkm::Id id) const;
  //
  // which takes an index and returns a value which should be produced by
  // the source arrays somehow. This ValueType will be the ValueType of the
  // ArrayHandleDecorator.
  //
  // Both SomeFunctor::operator() and CreateFunctor must be const.
  //
  template <typename Portal1Type, typename Portal2Type>
  SomeFunctor CreateFunctor(Portal1Type portal1, Portal2Type portal2) const;

  // Takes one portal for each source array handle (only two shown).
  // Returns a functor that defines:
  //
  // void operator()(svtkm::Id id, ValueType val) const;
  //
  // which takes an index and a value, which should be used to modify one
  // or more of the source arrays.
  //
  // CreateInverseFunctor is optional; if not provided, the
  // ArrayHandleDecorator will be read-only. In addition, if all of the
  // source ArrayHandles are read-only, the inverse functor will not be used
  // and the ArrayHandleDecorator will be read only.
  //
  // Both SomeInverseFunctor::operator() and CreateInverseFunctor must be
  // const.
  //
  template <typename Portal1Type, typename Portal2Type>
  SomeInverseFunctor CreateInverseFunctor(Portal1Type portal1,
                                          Portal2Type portal2) const;

};
```

## Add `svtkm::cont::ArrayHandleSOA`

`ArrayHandleSOA` behaves like a regular `ArrayHandle` (with a basic
storage) except that if you specify a `ValueType` of a `Vec` or a
`Vec`-like, it will actually store each component in a separate physical
array. When data are retrieved from the array, they are reconstructed into
`Vec` objects as expected.

The intention of this array type is to help cover the most common ways data
is lain out in memory. Typically, arrays of data are either an "array of
structures" like the basic storage where you have a single array of
structures (like `Vec`) or a "structure of arrays" where you have an array
of a basic type (like `float`) for each component of the data being
represented. The `ArrayHandleSOA` makes it easy to cover this second case
without creating special types.

`ArrayHandleSOA` can be constructed from a collection of `ArrayHandle` with
basic storage. This allows you to construct `Vec` arrays from components
without deep copies.

``` cpp
std::vector<svtkm::Float32> accel0;
std::vector<svtkm::Float32> accel1;
std::vector<svtkm::Float32> accel2;

// Let's say accel arrays are set to some field of acceleration vectors by
// some other software.

svtkm::cont::ArrayHandle<svtkm::Float32> accelHandle0 = svtkm::cont::make_ArrayHandle(accel0);
svtkm::cont::ArrayHandle<svtkm::Float32> accelHandle1 = svtkm::cont::make_ArrayHandle(accel1);
svtkm::cont::ArrayHandle<svtkm::Float32> accelHandle2 = svtkm::cont::make_ArrayHandle(accel2);

svtkm::cont::ArrayHandleSOA<svtkm::Vec3f_32> accel = { accelHandle0, accelHandle1, accelHandle2 };
```

Also provided are constructors and versions of `make_ArrayHandleSOA` that
take `std::vector` or C arrays as either initializer lists or variable
arguments.

``` cpp
std::vector<svtkm::Float32> accel0;
std::vector<svtkm::Float32> accel1;
std::vector<svtkm::Float32> accel2;

// Let's say accel arrays are set to some field of acceleration vectors by
// some other software.

svtkm::cont::ArrayHandleSOA<svtkm::Vec3f_32> accel = { accel0, accel1, accel2 };
```

However, setting arrays is a little awkward because you also have to
specify the length. This is done either outside the initializer list or as
the first argument.

``` cpp
svtkm::cont::make_ArrayHandleSOA({ array0, array1, array2 }, ARRAY_SIZE);
```

``` cpp
svtkm::cont::make_ArrayHandleSOA(ARRAY_SIZE, array0, array1, array2);
```

## `svtkm::cont::ArrayHandleCast` is now writeable

Previously, `ArrayHandleCast` was considered a read-only array handle.
However, it is trivial to reverse the cast (now that `ArrayHandleTransform`
supports an inverse transform). So now you can write to a cast array
(assuming the underlying array is writable).

One trivial consequence of this change is that you can no longer make a
cast that cannot be reversed. For example, it was possible to cast a simple
scalar to a `Vec` even though it is not possible to convert a `Vec` to a
scalar value. This was of dubious correctness (it is more of a construction
than a cast) and is easy to recreate with `ArrayHandleTransform`.

## Remove ArrayPortalShrink, behavior subsumed by `svtkm::cont::ArrayHandleView`

`ArrayPortalShrink` originaly allowed a user to pass in a delegate array portal
and then shrink the reported array size without actually modifying the
underlying allocation.  An iterator was also provided that would
correctly iterate over the shrunken size of the stored array.

Instead of directly shrinking the original array, it is prefered
to create an ArrayHandleView from an ArrayHandle and then specify the
number of values to use in the ArrayHandleView constructor.

# Control Enviornment

## `svtkm::cont::CellSetExplicit` refactored to remove redundant array

The `CellSetExplicit` class has been refactored to remove the `NumIndices`
array. This information is now derived from the `Offsets` array, which has
been changed to contain `[numCells + 1]` entries.

```
Old Layout:
-----------
NumIndices:   [  2,  4,  3,  3,  2 ]
IndexOffset:  [  0,  2,  6,  9, 12 ]
Connectivity: [  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13 ]

New Layout:
-----------
Offsets:      [  0,  2,  6,  9, 12, 14 ]
Connectivity: [  0,  1,  2,  3,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13 ]
```

This will reduce the memory overhead of the cellset by roughly `[numCells * 4]`
bytes.

The `IndexOffset` array / typedefs / methods have been renamed to `Offsets` for
brevity and consistency (similar members were plural, e.g. `Shapes`).

The `NumIndices` array can be recovered from the `Offsets` array by using an
`ArrayHandleDecorator`. This is done automatically by the
`CellSetExplicit::GetNumIndicesArray` method.

The `CellSetExplicit::Fill` signature has changed to remove `numIndices` as a
parameter and to require the `offsets` array as a non-optional argument. To
assist in porting old code, an `offsets` array can be generated from
`numIndices` using the new `svtkm::cont::ConvertNumIndicesToOffsets` methods,
defined in `CellSetExplicit.h`.

```cpp
svtkm::Id numPoints = ...;
auto cellShapes = ...;
auto numIndices = ...;
auto connectivity = ...;
svtkm::cont::CellSetExplicit<> cellSet = ...;

// Old:
cellSet.Fill(numPoints, cellShapes, numIndices, connectivity);

// New:
auto offsets = svtkm::cont::ConvertNumIndicesToOffsets(numIndices);
cellSet.Fill(numPoints, cellShapes, connectivity, offsets);
```

Since the `offsets` array now contains an additional offset at the end, it
cannot be used directly with `ArrayHandleGroupVecVariable` with the cellset's
`connectivity` array to create an array handle containing cell definitions.
This now requires an `ArrayHandleView` to trim the last value off of the array:

```cpp
svtkm::cont::CellSetExplicit<> cellSet = ...;
auto offsets = cellSet.GetOffsetsArray(svtkm::TopologyElementTagCell{},
                                       svtkm::TopologyElementTagPoint{});
auto conn = cellSet.GetConnectivityArray(svtkm::TopologyElementTagCell{},
                                         svtkm::TopologyElementTagPoint{});

// Old:
auto cells = svtkm::cont::make_ArrayHandleGroupVecVariable(conn, offsets);

// New:
const svtkm::Id numCells = offsets.GetNumberOfValues - 1;
auto offsetsTrim = svtkm::cont::make_ArrayHandleView(offsets, 0, numCells);
auto cells = svtkm::cont::make_ArrayHandleGroupVecVariable(conn, offsetsTrim);
```

## `svtkm::cont::CellSets` now don't have a name field

The requirement that `svtkm::cont::CellSets` have a name was so
cell based `svtkm::cont::Field`'s could be associated with the
correct CellSet in a `svtkm::cont::DataSet`.

Now that `svtkm::cont::DataSet`'s don't support multiple CellSets, we can remove
the `CellSet` name member variable.


## `svtkm::cont::CellLocatorUniformGrid` and `svtkm::cont::CellLocatorRectilinearGrid` support 2D grids

SVTK-m will now allow locating containing cells for a point using `CellLocatorUniformGrid`
and `CellLocatorRectilinearGrid` for 2D grids.

Users are required to create the locator objects as they normally would.
However, the `FindCell` method in `svtkm::exec::CellLocator` still requires users
to pass a 3D point as an input.

Further, the structured grid locators no longer use the `svtkm::exec::WorldToParametricCoordinates`
method to return parametric coordinates, instead they use fast paths for locating
points in a cell of an axis-aligned grid.

Another change for the `CellLocatorRectilinearGrid` is that now it uses binary search
on individual component arrays to search for a point.

## `svtkm::cont::DataSet` queries for CoordinateSystem ondices don't throw exceptions

Asking for the index of a `svtkm::cont::CoordinateSystem` by
name now returns a `-1` when no matching item has been found instead of throwing
an exception.

This was done to make the interface of `svtkm::cont::DataSet` to follow the guideline
"Only unrepresentable things should raise exceptions". The index of a non-existent item
is representable by `-1` and therefore we shouldn't throw, like wise the methods that return
references can still throw exceptions as you can't have a reference to an non-existent item.

## `svtkm::cont::Fields` now don't require the associated `svtkm::cont::CellSet` name

Now that `svtkm::cont::DataSet` can only have a single `svtkm::cont::CellSet`
the requirement that cell based `svtkm::cont::Field`s need a CellSet name
has been lifted.

## `Invoker` moved to svtkm::cont

Previously, `Invoker` was located in the `svtkm::worklet` namespace to convey
it was a replacement for using `svtkm::worklet::Dispatcher*`. In actuality
it should be in `svtkm::cont` as it is the proper way to launch worklets
for execution, and that shouldn't exist inside the `worklet` namespace.

## Refactor of `svtkm::cont::CellSet` PrepareForInput signatures

The `From` and `To` nomenclature for topology mapping has been confusing for
both users and developers, especially at lower levels where the intention of
mapping attributes from one element to another is easily conflated with the
concept of mapping indices (which maps in the exact opposite direction).

These identifiers have been renamed to `VisitTopology` and `IncidentTopology`
to clarify the direction of the mapping. The order in which these template
parameters are specified for `PrepareForInput` have also been reversed,
since eventually there may be more than one `IncidentTopology`, and having
`IncidentTopology` at the end will allow us to replace it with a variadic
template parameter pack in the future.


## Simplify creating `svtkm::cont::Fields` from `svtkm::cont::ArrayHandles`

SVTK-m now offers `make_FieldPoint` and `make_FieldCell` functions
that reduce the complexity of construction `svtkm::cont::Fields`
from `svtkm::cont::ArrayHandles`.

Previously to construct a point and cell fields you would do:
```cpp
svtkm::cont::ArrayHandle<int> pointHandle;
svtkm::cont::ArrayHandle<int> cellHandle;
svtkm::cont::Field pointField("p", svtkm::cont::Field::Association::POINTS, pointHandle);
svtkm::cont::Field cellField("c", svtkm::cont::Field::Association::CELL_SET, "cells", cellHandle);
```

Now with the new `make_` functions you can do:
```cpp
svtkm::cont::ArrayHandle<int> pointHandle;
svtkm::cont::ArrayHandle<int> cellHandle;
auto pointField = svtkm::cont::make_FieldPoint("p", pointHandle);
auto cellField = svtkm::cont::make_FieldCell("c", "cells", cellHandle);
```

# Execution Environment

## Corrected cell derivatives for polygon cell shape

For polygon cell shapes (that are not triangles or quadrilaterals),
interpolations are done by finding the center point and creating a triangle
fan around that point. Previously, the gradient was computed in the same
way as interpolation: identifying the correct triangle and computing the
gradient for that triangle.

The problem with that approach is that makes the gradient discontinuous at
the boundaries of this implicit triangle fan. To make things worse, this
discontinuity happens right at each vertex where gradient calculations
happen frequently. This means that when you ask for the gradient at the
vertex, you might get wildly different answers based on floating point
imprecision.

Get around this problem by creating a small triangle around the point in
question, interpolating values to that triangle, and use that for the
gradient. This makes for a smoother gradient transition around these
internal boundaries.

## A `ScanExtended` device algorithm has been added

This new scan algorithm produces an array that contains both an inclusive scan
and an exclusive scan in the same array:

```cpp
#include <svtkm/cont/Algorithm.h>
#include <svtkm/cont/ArrayGetValue.h>
#include <svtkm/cont/ArrayHandle.h>
#include <svtkm/cont/ArrayHandleView.h>

svtkm::cont::ArrayHandle<T> inputData = ...;
const svtkm::Id size = inputData.GetNumberOfValues();

svtkm::cont::ArrayHandle<T> extendedScan;
svtkm::cont::Algorithm::ScanExtended(inputData, extendedScan);

// The exclusive scan is the first `inputSize` values starting at index 0:
auto exclusiveScan = svtkm::cont::make_ArrayHandleView(extendedScan, 0, size);

// The inclusive scan is the first `inputSize` values starting at index 1:
auto inclusiveScan = svtkm::cont::make_ArrayHandleView(extendedScan, 1, size);

// The total sum of the input data is the last value in the extended scan.
const T totalSum = svtkm::cont::ArrayGetValue(size, extendedScan);
```

This can also be thought of as an exclusive scan that appends the total sum,
rather than returning it.

## Provide base component queries to `svtkm::VecTraits`

This change adds a recursive `BaseComponentType` to `VecTraits` that
recursively finds the base (non-`Vec`) type of a `Vec`. This is useful when
dealing with potentially nested `Vec`s (e.g. `Vec<Vec<T, M>, N>`) and you
want to know the precision of the math being defined.

``` cpp
using NestedVec = svtkm::Vec<svtkm::Vec<svtkm::Float32, 3>, 8>;

// ComponentType becomes svtkm::Vec<svtkm::Float32, 3>
using ComponentType = typename svtkm::VecTraits<NestedVec>::ComponentType;

// BaseComponentType becomes svtkm::Float32
using BaseComponentType = typename svtkm::VecTraits<NestedVec>::BaseComponentType;
```

Also added the ability to `VecTraits` to change the component type of a
vector. The template `RepalceComponentType` resolves to a `Vec` of the same
type with the component replaced with a new type. The template
`ReplaceBaseComponentType` traverses down a nested type and replaces the
base type.

``` cpp
using NestedVec = svtkm::Vec<svtkm::Vec<svtkm::Float32, 3>, 8>;

// NewVec1 becomes svtkm::Vec<svtkm::Float64, 8>
using NewVec1 =
  typename svtkm::VecTraits<NestedVec>::template ReplaceComponentType<svtkm::Float64>;

// NewVec2 becomes svtkm::Vec<svtkm::Vec<svtkm::Float64, 3>, 8>
using NewVec1 =
  typename svtkm::VecTraits<NestedVec>::template ReplaceBaseComponentType<svtkm::Float64>;
```

This functionality replaces the functionality in `svtkm::BaseComponent`. Unfortunately,
`svtkm::BaseComponent` did not have the ability to replace the base component and
there was no straightforward way to implement that outside of `VecTraits`.

# Worklets and Filters

## ExecutionSignatures are now optional for simple worklets

If a worklet doesn't explicitly state an `ExecutionSignature`, SVTK-m
assumes the worklet has no return value, and each `ControlSignature`
argument is passed to the worklet in the same order.

For example if we had this worklet:
```cxx
struct DotProduct : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);
  using ExecutionSignature = void(_1, _2, _3);

  template <typename T, svtkm::IdComponent Size>
  SVTKM_EXEC void operator()(const svtkm::Vec<T, Size>& v1,
                            const svtkm::Vec<T, Size>& v2,
                            T& outValue) const
  {
    outValue = svtkm::Dot(v1, v2);
  }
};
```

It can be simplified to be:

```cxx
struct DotProduct : public svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldIn, FieldOut);

  template <typename T, svtkm::IdComponent Size>
  SVTKM_EXEC void operator()(const svtkm::Vec<T, Size>& v1,
                            const svtkm::Vec<T, Size>& v2,
                            T& outValue) const
  {
    outValue = svtkm::Dot(v1, v2);
  }
};
```

## Refactor topology mappings to clarify meaning

The `From` and `To` nomenclature for topology mapping has been confusing for
both users and developers, especially at lower levels where the intention of
mapping attributes from one element to another is easily conflated with the
concept of mapping indices (which maps in the exact opposite direction).

These identifiers have been renamed to `VisitTopology` and `IncidentTopology`
to clarify the direction of the mapping. The order in which these template
parameters are specified for `WorkletMapTopology` have also been reversed,
since eventually there may be more than one `IncidentTopology`, and having
`IncidentTopology` at the end will allow us to replace it with a variadic
template parameter pack in the future.

Other implementation details supporting these worklets, include `Fetch` tags,
`Connectivity` classes, and methods on the various `CellSet` classes (such as
`PrepareForInput` have also reversed their template arguments.

## Provide a simplified way to state allowed value types for `svtkm::filter::filters`

Previously SVTK-m filters used a specialization of `svtkm::filter::FilterTraits<>` to control
the acceptable input value types. For example if the `WarpVector` filter want to only allow
`svtkm::Vec3f_32` and `svtkm::Vec3f_64` it would use:

```cpp
namespace svtkm { namespace filter {
template <>
class FilterTraits<WarpVector>
{
public:
  // WarpVector can only applies to Float and Double Vec3 arrays
  using InputFieldTypeList = svtkm::TypeListTagFieldVec3;
};
}}
```

This increase the complexity of writing filters. To make this easier SVTK-m now looks for
a `SupportedTypes` define on the filter when a `svtkm::filter::FilterTraits` specialization
doesn't exist. This allows filters to succinctly specify supported types, such as seen below
for the `WarpVector` filter.

```cpp
class WarpVector : public svtkm::filter::FilterField<WarpVector>
{
public:
  using SupportedTypes = svtkm::TypeListTagFieldVec3;
...
};
## `svtkm::cont::Invoker` is now a member of all SVTK-m filters

To simplify how svtkm filters are written we have made each svtkm::filter
have a `svtkm::cont::Invoker` as member variable. The goal with this change
is provide an uniform API for launching all worklets from within a filter.

Lets consider the PointElevation filter. Previous to these changes the
`DoExecute` would need to construct the correct dispatcher with the
correct parameters as seen below:

```cpp
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet PointElevation::DoExecute(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  svtkm::filter::PolicyBase<DerivedPolicy>)
{
  svtkm::cont::ArrayHandle<svtkm::Float64> outArray;

  svtkm::worklet::DispatcherMapField<svtkm::worklet::PointElevation> dispatcher(this->Worklet);
  dispatcher.Invoke(field, outArray);
  ...
}
```

With these changes the filter can instead use `this->Invoke` and have
the correct dispatcher executed. This makes it easier to teach and
learn how to write new filters.
```cpp
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet PointElevation::DoExecute(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  svtkm::filter::PolicyBase<DerivedPolicy>)
{
  svtkm::cont::ArrayHandle<svtkm::Float64> outArray;

  this->Invoke(this->Worklet, field, outArray);
  ...
}
```



## Simplify creating results for `svtkm::filter::filters`

As part of the process of making SVTK-m filters easier to write for newcomers
whe have a couple of changes to make constructing the output `svtkm::cont::DataSet`
easier.

First we have moved the `CreateResult` functions out of the internals namespace
and directly into `svtkm::filter`. This makes it clearer to developers that this
was the 'proper' way to construct the output DataSet.

Second we have streamlined the collection of `svtkm::filter::CreateResult` methods to
require the user to provide less information and provide clearer names explaing what
they do.

To construct output identical to the input but with a new field you now just pass
the `svtkm::filter::FieldMetadata` as a paramter instead of explictly stating
the field association, and the possible cell set name:
```cpp
return CreateResult(input, newField, name, fieldMetadata);
```

To construct output identical to the input but with a cell field added you
can now pass the `svtkm::cont::CellSet` as a paramter instead of explictly stating
the field association, and the cell set name:
```cpp
return CreateResultFieldCell(input, newCellField, name, cellset);
```

## `svtkm::filter::Filter` now don't have an active CellSet

`svtkm::filter::FilterField` has removed the concept of `ActiveCellSetIndex`. This
has been done as `svtkm::cont::DataSet` now only contains a single `svtkm::cont::CellSet`.


## `svtkm::filter::FilterField` now provides all functionality of `svtkm::filter::FilterCell`

The FilterCell was a subclass of `svtkm::filter::FilterField` and behaves essentially the same
but provided the pair of methods `SetActiveCellSetIndex` and `GetActiveCellSetIndex`.
It was a common misconception that `FilterCell` was meant for Cell based algorithms, instead of
algorithms that required access to the active `svtkm::cont::CellSet`.

By moving `SetActiveCellSetIndex` and `GetActiveCellSetIndex` to FilterField, we remove this confusion.



## `svtkm::worklet::WorkletPointNeighborhood` can query exact neighbor offset locations

Add ability to test exact neighbor offset locations in BoundaryState.

The following methods:
```cpp
BoundaryState::InXBoundary
BoundaryState::InYBoundary
BoundaryState::InZBoundary
BoundaryState::InBoundary
```

have been renamed to:

```cpp
BoundaryState::IsRadiusInXBoundary
BoundaryState::IsRadiusInYBoundary
BoundaryState::IsRadiusInZBoundary
BoundaryState::IsRadiusInBoundary
```

to distinguish them from the new methods:

```cpp
BoundaryState::IsNeighborInXBoundary
BoundaryState::IsNeighborInYBoundary
BoundaryState::IsNeighborInZBoundary
BoundaryState::IsNeighborInBoundary
```

which check a specific neighbor sample offset instead of a full radius.

The method `BoundaryState::ClampNeighborIndex` has also been added, which clamps
a 3D neighbor offset vector to the dataset boundaries.

This allows iteration through only the valid points in a neighborhood using
either of the following patterns:

Using `ClampNeighborIndex` to restrict the iteration space:
```
struct MyWorklet : public svtkm::worklet::WorkletPointNeighborhood
{
public:
  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);
  using ExecutionSignature = void(_2, Boundary, _3);

  template <typename InNeighborhoodT, typename OutDataT>
  SVTKM_EXEC void operator()(const InNeighborhoodT& inData,
                            const svtkm::exec::BoundaryState &boundary,
                            OutDataT& outData) const
  {
    // Clamp the radius to the dataset bounds (discard out-of-bounds points).
    const auto minRadius = boundary.ClampNeighborIndex({-10, -10, -10});
    const auto maxRadius = boundary.ClampNeighborIndex({10, 10, 10});

    for (svtkm::IdComponent k = minRadius[2]; k <= maxRadius[2]; ++k)
    {
      for (svtkm::IdComponent j = minRadius[1]; j <= maxRadius[1]; ++j)
      {
        for (svtkm::IdComponent i = minRadius[0]; i <= maxRadius[0]; ++i)
        {
          outData = doSomeConvolution(i, j, k, outdata, inData.Get(i, j, k));
        }
      }
    }
  }
};
```

or, using `IsNeighborInBoundary` methods to skip out-of-bounds loops:

```
struct MyWorklet : public svtkm::worklet::WorkletPointNeighborhood
{
public:
  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);
  using ExecutionSignature = void(_2, Boundary, _3);

  template <typename InNeighborhoodT, typename OutDataT>
  SVTKM_EXEC void operator()(const InNeighborhoodT& inData,
                            const svtkm::exec::BoundaryState &boundary,
                            OutDataT& outData) const
  {
    for (svtkm::IdComponent k = -10; k <= 10; ++k)
    {
      if (!boundary.IsNeighborInZBoundary(k))
      {
        continue;
      }

      for (svtkm::IdComponent j = -10; j <= 10; ++j)
      {
        if (!boundary.IsNeighborInYBoundary(j))
        {
          continue;
        }

        for (svtkm::IdComponent i = -10; i <= 10; ++i)
        {
          if (!boundary.IsNeighborInXBoundary(i))
          {
            continue;
          }

          outData = doSomeConvolution(i, j, k, outdata, inData.Get(i, j, k));
        }
      }
    }
  }
};
```

The latter is useful for implementing a convolution that substitutes a constant
value for out-of-bounds indices:

```
struct MyWorklet : public svtkm::worklet::WorkletPointNeighborhood
{
public:
  using ControlSignature = void(CellSetIn, FieldInNeighborhood, FieldOut);
  using ExecutionSignature = void(_2, Boundary, _3);

  template <typename InNeighborhoodT, typename OutDataT>
  SVTKM_EXEC void operator()(const InNeighborhoodT& inData,
                            const svtkm::exec::BoundaryState &boundary,
                            OutDataT& outData) const
  {
    for (svtkm::IdComponent k = -10; k <= 10; ++k)
    {
      for (svtkm::IdComponent j = -10; j <= 10; ++j)
      {
        for (svtkm::IdComponent i = -10; i <= 10; ++i)
        {
          if (boundary.IsNeighborInBoundary({i, j, k}))
          {
            outData = doSomeConvolution(i, j, k, outdata, inData.Get(i, j, k));
          }
          else
          { // substitute zero for out-of-bounds samples:
            outData = doSomeConvolution(i, j, k, outdata, 0);
          }
        }
      }
    }
  }
};
```


## Add ability to get an array from a `svtkm::cont::Field` for a particular type

Previously, whenever you got an array from a `Field` object from a call to
an `ApplyPolicy`, you would get back a `VariantArrayHandle` that allows you
to cast to multiple types. To use that, you then have to cast it to
multiple different types and multiple different storage.

Often, this is what you want. If you are operating on a field, then you
want to cast to the native type. But there are also cases where you know a
specific type you want. For example, if you are operating on two fields, it
makes sense to find the exact type for the first field and then cast the
second field to that type if necessary rather than pointlessly unroll
templates for the cross of every possible combination. Also, we are not
unrolling for different storage types or attempting to create a virtual
array. Instead, we are using an `ArrayHandleMultiplexer` so that you only
have to compile for this array once.

This is done through a new version of `ApplyPolicy`. This version takes a
type of the array as its first template argument, which must be specified.

This requires having a list of potential storage to try. It will use that
to construct an `ArrayHandleMultiplexer` containing all potential types.
This list of storages comes from the policy. A `StorageList` item was added
to the policy. It is also sometimes necessary for a filter to provide its
own special storage types. Thus, an `AdditionalFieldStorage` type was added
to `Filter` which is set to a `ListTag` of storage types that should be
added to those specified by the policy.

Types are automatically converted. So if you ask for a `svtkm::Float64` and
field contains a `svtkm::Float32`, it will the array wrapped in an
`ArrayHandleCast` to give the expected type.

Here is an example where you are doing an operation on a field and
coordinate system. The superclass finds the correct type of the field. Your
result is just going to follow the type of the field.

``` cpp
template <typename T, typename StorageType, typename DerivedPolicy>
inline SVTKM_CONT svtkm::cont::DataSet MyFilter::DoExecute(
  const svtkm::cont::DataSet& inDataSet,
  const svtkm::cont::ArrayHandle<T, StorageType>& field,
  const svtkm::filter::FieldMetadata& fieldMetadata,
  svtkm::filter::PolicyBase<DerivedPolicy> policy)
{
  svtkm::cont::CoordinateSystem coords = inDataSet.GetCoordianteSystem();
  auto coordsArray = svtkm::filter::ApplyPolicy<T>(coords, policy, *this);
```

## Add Lagrangian Coherent Structures (LCS) Filter for SVTK-m

The new filter `svtkm::filter::LagrangianStructures` is meant for Finite Time
Lyapunov Exponent (FTLE) calculation using SVTK-m.
The filter allows users to calculate FTLE in two ways
1. Provide a dataset with a vector field, which will be used to generate a flow
   map.
2. Provide a dataset containing a flow map, which can be readily used for the
   FTLE field calculation.

The filter returns a dataset with a point field named FTLE.
Is the input is strucutred and an auxiliary grid was not used, the filter will
add the field to the original dataset set, else a new structured dataset is returned.

## `SurfaceNormals` filter can now orient normals

The `OrientNormals` worklet has been added to the `SurfaceNormals` filter, and
is enabled by turning on the `AutoOrientNormals` option. This feature ensures
that all normals generated by the filter will point out of the dataset (or
inward if the `FlipNormals` option is true). In addition,
`SurfaceNormals` now has a `Consistency` option that forces all triangle
windings to be consistent with the cell normal direction (the cell points are
specified in counter-clockwise order around the normal).

This functionality is provided by the following new worklets:

* OrientNormals
    * RunOrientCellNormals
    * RunOrientPointNormals
    * RunOrientPointAndCellNormals
    * RunFlipNormals
* TriangleWinding

## Particle advection components have better status query support

There are now special statuses for Particle, Integrator, and Evaluator.

The particle advection modules only supported statuses for particles and made it
difficult to handle advanced integtator statuses.
Now each of the three important modules return their own statuses

Particles have `svtkm::worklet::particleadvection::ParticleStatus`,
Integrators have `svtkm::worklet::particleadvection::IntegratorStatus`, and
Evaluators have `svtkm::worklet::particleadvection::EvaluatorStatus`.

Further, names of the statuses in `svtkm::worklet::particleadvection::ParticleStatus`
have changed

`ParticleStatus::STATUS_OK` is now `ParticleStatus::SUCCESS`, and there is another
status `ParticleStatus::TOOK_ANY_STEPS` which is active if the particle has taken
at least one step with the current data.

There are few more changes that allow particle advection in 2D structured grids.

## `svtkm::filter::Threshold` now outputs a `svtkm::cont::CellSetExplicit`

Perhaps a better title for this change would be "Make the Threshold filter
not totally useless."

A long standing issue with the Threshold filter is that its output CellSet
was stored in a CellSetPermutation. This made Threshold hyper- efficient
because it required hardly any data movement to implement. However, the
problem was that any other unit that had to use the CellSet failed. To have
SVTK-m handle that output correctly in other filters and writers, they all
would have to check for the existance of CellSetPermutation. And
CellSetPermutation is templated on the CellSet type it is permuting, so all
units would have to compile special cases for all these combinations. This
is not likely to be feasible in any static solution.

The simple solution, implemented here, is to deep copy the cells to a
CellSetExplicit, which is a known type that is already used everywhere in
SVTK-m. The solution is a bit disappointing since it requires more memory
and time to build. But it is on par with solutions in other libraries (like
SVTK). And it really does not matter how efficient the old solution was if
it was useless.

# Build

## Introduce `svtkm_add_target_information` cmake function to make using svtk-m easier

This higher order function allow build-systems that use SVTK-m
to use `add_library` or `add_executable` calls but still have an
easy to way to get the required information to have SVTK-m using
compilation units compile correctly.

```cmake
 svtkm_add_target_information(
   target[s]
   [ DROP_UNUSED_SYMBOLS ]
   [ MODIFY_CUDA_FLAGS ]
   [ EXTENDS_SVTKM ]
   [ DEVICE_SOURCES <source_list> ]
   )
```
 Usage:
 ```cmake
   add_library(lib_that_uses_svtkm STATIC a.cxx)
   svtkm_add_target_information(lib_that_uses_svtkm
                               MODIFY_CUDA_FLAGS
                               DEVICE_SOURCES a.cxx
                               )
   target_link_libraries(lib_that_uses_svtkm PRIVATE svtkm_filter)
```

### Options to svtkm_add_target_information

  - DROP_UNUSED_SYMBOLS: If enabled will apply the appropiate link
  flags to drop unused SVTK-m symbols. This works as SVTK-m is compiled with
  -ffunction-sections which allows for the linker to remove unused functions.
  If you are building a program that loads runtime plugins that can call
  SVTK-m this most likely shouldn't be used as symbols the plugin expects
  to exist will be removed.
  Enabling this will help keep library sizes down when using static builds
  of SVTK-m as only the functions you call will be kept. This can have a
  dramatic impact on the size of the resulting executable / shared library.
  - MODIFY_CUDA_FLAGS: If enabled will add the required -arch=<ver> flags
  that SVTK-m was compiled with. If you have multiple libraries that use
  SVTK-m calling `svtkm_add_target_information` multiple times with
  `MODIFY_CUDA_FLAGS` will cause duplicate compiler flags. To resolve this issue
  you can; pass all targets and sources to a single `svtkm_add_target_information`
  call, have the first one use `MODIFY_CUDA_FLAGS`, or use the provided
  standalone `svtkm_get_cuda_flags` function.

  - DEVICE_SOURCES: The collection of source files that are used by `target(s)` that
  need to be marked as going to a special compiler for certain device adapters
  such as CUDA.

  - EXTENDS_SVTKM: Some programming models have restrictions on how types can be used,
  passed across library boundaries, and derived from.
  For example CUDA doesn't allow device side calls across dynamic library boundaries,
  and requires all polymorphic classes to be reachable at dynamic library/executable
  link time.
  To accommodate these restrictions we need to handle the following allowable
  use-cases:
    - Object library: do nothing, zero restrictions
    - Executable: do nothing, zero restrictions
    - Static library: do nothing, zero restrictions
    - Dynamic library:
      - Wanting to use SVTK-m as implementation detail, doesn't expose SVTK-m
      types to consumers. This is supported no matter if CUDA is enabled.
      - Wanting to extend SVTK-m and provide these types to consumers.
      This is only supported when CUDA isn't enabled. Otherwise we need to ERROR!
      - Wanting to pass known SVTK-m types across library boundaries for others
      to use in filters/worklets. This is only supported when CUDA isn't enabled. Otherwise we need to ERROR!

    For most consumers they can ignore the `EXTENDS_SVTKM` property as the default will be correct.

The `svtkm_add_target_information` higher order function leverages the `svtkm_add_drop_unused_function_flags` and
`svtkm_get_cuda_flags` functions which can be used by SVTK-m consuming applications.

The `svtkm_add_drop_unused_function_flags` function implements all the behavior of `DROP_UNUSED_SYMBOLS` for a single
target.

The `svtkm_get_cuda_flags` function implements a general form of `MODIFY_CUDA_FLAGS` but instead of modiyfing
the `CMAKE_CUDA_FLAGS` it will add the flags to any variable passed to it.





# Other

## Simplify examples

Lots of the examples were out of date or way too verbose. The examples have
been simplified and brought up to modern SVTK-m conventions.

We have also added a "hello worklet" example to be a minimal example of
creating a working algorithm (wrapped in a filter) in SVTK-m (and used).

## `svtkm::Vec const& operator[]` is now `constexpr`


This was done to allow for developers to write normal operations on svtkm::Vec but have
the resolved at compile time, allowing for both readible code and no runtime cost.

Now you can do things such as:
```cxx
  constexpr svtkm::Id2 dims(16,16);
  constexpr svtkm::Float64 dx = svtkm::Float64(4.0 * svtkm::Pi()) / svtkm::Float64(dims[0] - 1);
```
