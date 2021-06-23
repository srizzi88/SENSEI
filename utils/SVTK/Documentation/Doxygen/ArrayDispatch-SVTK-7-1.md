@page SVTK-7-1-ArrayDispatch svtkArrayDispatch and Related Tools
@tableofcontents

# Background # {#SVTKAD-Background}

SVTK datasets store most of their important information in subclasses of
`svtkDataArray`. Vertex locations (`svtkPoints::Data`), cell topology
(`svtkCellArray::Ia`), and numeric point, cell, and generic attributes
(`svtkFieldData::Data`) are the dataset features accessed most frequently by SVTK
algorithms, and these all rely on the `svtkDataArray` API.

# Terminology # {#SVTKAD-Terminology}

This page uses the following terms:

A __ValueType__ is the element type of an array. For instance, `svtkFloatArray`
has a ValueType of `float`.

An __ArrayType__ is a subclass of `svtkDataArray`. It specifies not only a
ValueType, but an array implementation as well. This becomes important as
`svtkDataArray` subclasses will begin to stray from the typical
"array-of-structs" ordering that has been exclusively used in the past.

A __dispatch__ is a runtime-resolution of a `svtkDataArray`'s ArrayType, and is
used to call a section of executable code that has been tailored for that
ArrayType. Dispatching has compile-time and run-time components. At
compile-time, the possible ArrayTypes to be used are determined and a worker
code template is generated for each type. At run-time, the type of a specific
array is determined and the proper worker instantiation is called.

__Template explosion__ refers to a sharp increase in the size of a compiled
binary that results from instantiating a template function or class on many
different types.

## svtkDataArray ## {#SVTKAD-svtkDataArray}

The data array type hierarchy in SVTK has a unique feature when compared to
typical C++ containers: a non-templated base class. All arrays containing
numeric data inherit `svtkDataArray`, a common interface that sports a very
useful API. Without knowing the underlying ValueType stored in data array, an
algorithm or user may still work with any `svtkDataArray` in meaningful ways:
The array can be resized, reshaped, read, and rewritten easily using a generic
API that substitutes double-precision floating point numbers for the array's
actual ValueType. For instance, we can write a simple function that computes
the magnitudes for a set of vectors in one array and store the results in
another using nothing but the typeless `svtkDataArray` API:

~~~{.cpp}
// 3 component magnitude calculation using the svtkDataArray API.
// Inefficient, but easy to write:
void calcMagnitude(svtkDataArray *vectors, svtkDataArray *magnitude)
{
  svtkIdType numVectors = vectors->GetNumberOfTuples();
  for (svtkIdType tupleIdx = 0; tupleIdx < numVectors; ++tupleIdx)
    {
    // What data types are magnitude and vectors using?
    // We don't care! These methods all use double.
    magnitude->SetComponent(tupleIdx, 0,
      std::sqrt(vectors->GetComponent(tupleIdx, 0) *
                vectors->GetComponent(tupleIdx, 0) +
                vectors->GetComponent(tupleIdx, 1) *
                vectors->GetComponent(tupleIdx, 1) +
                vectors->GetComponent(tupleIdx, 2) *
                vectors->GetComponent(tupleIdx, 2));
    }
}
~~~

## The Costs of Flexiblity ## {#SVTKAD-TheCostsOfFlexiblity}

However, this flexibility comes at a cost. Passing data through a generic API
has a number of issues:

__Accuracy__

Not all ValueTypes are fully expressible as a `double`. The truncation of
integers with > 52 bits of precision can be a particularly nasty issue.

__Performance__

__Virtual overhead__: The only way to implement such a system is to route the
`svtkDataArray` calls through a run-time resolution of ValueTypes. This is
implemented through the virtual override mechanism of C++, which adds a small
overhead to each API call.

__Missed optimization__: The virtual indirection described above also prevents
the compiler from being able to make assumptions about the layout of the data
in-memory. This information could be used to perform advanced optimizations,
such as vectorization.

So what can one do if they want fast, optimized, type-safe access to the data
stored in a `svtkDataArray`? What options are available?

## The Old Solution: svtkTemplateMacro ##  {#SVTKAD-svtkTemplateMacro}

The `svtkTemplateMacro` is described in this section. While it is no longer
considered a best practice to use this construct in new code, it is still
usable and likely to be encountered when reading the SVTK source code. Newer
code should use the `svtkArrayDispatch` mechanism, which is detailed later. The
discussion of `svtkTemplateMacro` will help illustrate some of the practical
issues with array dispatching.

With a few minor exceptions that we won't consider here, prior to SVTK 7.1 it
was safe to assume that all numeric `svtkDataArray` objects were also subclasses
of `svtkDataArrayTemplate`. This template class provided the implementation of
all documented numeric data arrays such as `svtkDoubleArray`, `svtkIdTypeArray`,
etc, and stores the tuples in memory as a contiguous array-of-structs (AOS).
For example, if we had an array that stored 3-component tuples as floating
point numbers, we could define a tuple as:

~~~{.cpp}
struct Tuple { float x; float y; float z; };
~~~

An array-of-structs, or AOS, memory buffer containing this data could be
described as:

~~~{.cpp}
Tuple ArrayOfStructsBuffer[NumTuples];
~~~

As a result, `ArrayOfStructsBuffer` will have the following memory layout:

~~~{.cpp}
{ x1, y1, z1, x2, y2, z2, x3, y3, z3, ...}
~~~

That is, the components of each tuple are stored in adjacent memory locations,
one tuple after another. While this is not exactly how `svtkDataArrayTemplate`
implemented its memory buffers, it accurately describes the resulting memory
layout.

`svtkDataArray` also defines a `GetDataType` method, which returns an enumerated
value describing a type. We can used to discover the ValueType stored in the
array.

Combine the AOS memory convention and `GetDataType()` with a horrific little
method on the data arrays named `GetVoidPointer()`, and a path to efficient,
type-safe access was available. `GetVoidPointer()` does what it says on the
tin: it returns the memory address for the array data's base location as a
`void*`. While this breaks encapsulation and sets off warning bells for the
more pedantic among us, the following technique was safe and efficient when
used correctly:

~~~{.cpp}
// 3-component magnitude calculation using GetVoidPointer.
// Efficient and fast, but assumes AOS memory layout
template <typename ValueType>
void calcMagnitudeWorker(ValueType *vectors, ValueType *magnitude,
                         svtkIdType numVectors)
{
  for (svtkIdType tupleIdx = 0; tupleIdx < numVectors; ++tupleIdx)
    {
    // We now have access to the raw memory buffers, and assuming
    // AOS memory layout, we know how to access them.
    magnitude[tupleIdx] =
      std::sqrt(vectors[3 * tupleIdx + 0] *
                vectors[3 * tupleIdx + 0] +
                vectors[3 * tupleIdx + 1] *
                vectors[3 * tupleIdx + 1] +
                vectors[3 * tupleIdx + 2] *
                vectors[3 * tupleIdx + 2]);
    }
}

void calcMagnitude(svtkDataArray *vectors, svtkDataArray *magnitude)
{
  assert("Arrays must have same datatype!" &&
         svtkDataTypesCompare(vectors->GetDataType(),
                             magnitude->GetDataType()));
  switch (vectors->GetDataType())
    {
    svtkTemplateMacro(calcMagnitudeWorker<SVTK_TT*>(
      static_cast<SVTK_TT*>(vectors->GetVoidPointer(0)),
      static_cast<SVTK_TT*>(magnitude->GetVoidPointer(0)),
      vectors->GetNumberOfTuples());
    }
}
~~~

The `svtkTemplateMacro`, as you may have guessed, expands into a series of case
statements that determine an array's ValueType from the `int GetDataType()`
return value. The ValueType is then `typedef`'d to `SVTK_TT`, and the macro's
argument is called for each numeric type returned from `GetDataType`. In this
case, the call to `calcMagnitudeWorker` is made by the macro, with `SVTK_TT`
`typedef`'d to the array's ValueType.

This is the typical usage pattern for `svtkTemplateMacro`. The `calcMagnitude`
function calls a templated worker implementation that uses efficient, raw
memory access to a typesafe memory buffer so that the worker's code can be as
efficient as possible. But this assumes AOS memory ordering, and as we'll
mention, this assumption may no longer be valid as SVTK moves further into the
field of in-situ analysis.

But first, you may have noticed that the above example using `svtkTemplateMacro`
has introduced a step backwards in terms of functionality. In the
`svtkDataArray` implementation, we didn't care if both arrays were the same
ValueType, but now we have to ensure this, since we cast both arrays' `void`
pointers to `SVTK_TT`*. What if vectors is an array of integers, but we want to
calculate floating point magnitudes?

## svtkTemplateMacro with Multiple Arrays ## {#SVTKAD-Dual-svtkTemplateMacro}

The best solution prior to SVTK 7.1 was to use two worker functions. The first
is templated on vector's ValueType, and the second is templated on both array
ValueTypes:

~~~{.cpp}
// 3-component magnitude calculation using GetVoidPointer and a
// double-dispatch to resolve ValueTypes of both arrays.
// Efficient and fast, but assumes AOS memory layout, lots of boilerplate
// code, and the sensitivity to template explosion issues increases.
template <typename VectorType, typename MagnitudeType>
void calcMagnitudeWorker2(VectorType *vectors, MagnitudeType *magnitude,
                          svtkIdType numVectors)
{
  for (svtkIdType tupleIdx = 0; tupleIdx < numVectors; ++tupleIdx)
    {
    // We now have access to the raw memory buffers, and assuming
    // AOS memory layout, we know how to access them.
    magnitude[tupleIdx] =
      std::sqrt(vectors[3 * tupleIdx + 0] *
                vectors[3 * tupleIdx + 0] +
                vectors[3 * tupleIdx + 1] *
                vectors[3 * tupleIdx + 1] +
                vectors[3 * tupleIdx + 2] *
                vectors[3 * tupleIdx + 2]);
    }
}

// Vector ValueType is known (VectorType), now use svtkTemplateMacro on
// magnitude:
template <typename VectorType>
void calcMagnitudeWorker1(VectorType *vectors, svtkDataArray *magnitude,
                          svtkIdType numVectors)
{
  switch (magnitude->GetDataType())
    {
    svtkTemplateMacro(calcMagnitudeWorker2(vectors,
      static_cast<SVTK_TT*>(magnitude->GetVoidPointer(0)), numVectors);
    }
}

void calcMagnitude(svtkDataArray *vectors, svtkDataArray *magnitude)
{
  // Dispatch vectors first:
  switch (vectors->GetDataType())
    {
    svtkTemplateMacro(calcMagnitudeWorker1<SVTK_TT*>(
      static_cast<SVTK_TT*>(vectors->GetVoidPointer(0)),
      magnitude, vectors->GetNumberOfTuples());
    }
}
~~~

This works well, but it's a bit ugly and has the same issue as before regarding
memory layout. Double dispatches using this method will also see more problems
regarding binary size. The number of template instantiations that the compiler
needs to generate is determined by `I = T^D`, where `I` is the number of
template instantiations, `T` is the number of types considered, and `D` is the
number of dispatches. As of SVTK 7.1, `svtkTemplateMacro` considers 14 data
types, so this double-dispatch will produce 14 instantiations of
`calcMagnitudeWorker1` and 196 instantiations of `calcMagnitudeWorker2`. If we
tried to resolve 3 `svtkDataArray`s into raw C arrays, 2744 instantiations of
the final worker function would be generated. As more arrays are considered,
the need for some form of restricted dispatch becomes very important to keep
this template explosion in check.

## Data Array Changes in SVTK 7.1 ## {#SVTKAD-Changes-in-SVTK-71}

Starting with SVTK 7.1, the Array-Of-Structs (AOS) memory layout is no longer
the only `svtkDataArray` implementation provided by the library. The
Struct-Of-Arrays (SOA) memory layout is now available through the
`svtkSOADataArrayTemplate` class. The SOA layout assumes that the components of
an array are stored separately, as in:

~~~{.cpp}
struct StructOfArraysBuffer
{
  float *x; // Pointer to array containing x components
  float *y; // Same for y
  float *z; // Same for z
};
~~~

The new SOA arrays were added to improve interoperability between SVTK and
simulation packages for live visualization of in-situ results. Many simulations
use the SOA layout for their data, and natively supporting these arrays in SVTK
will allow analysis of live data without the need to explicitly copy it into a
SVTK data structure.

As a result of this change, a new mechanism is needed to efficiently access
array data. `svtkTemplateMacro` and `GetVoidPointer` are no longer an acceptable
solution -- implementing `GetVoidPointer` for SOA arrays requires creating a
deep copy of the data into a new AOS buffer, a waste of both processor time and
memory.

So we need a replacement for `svtkTemplateMacro` that can abstract away things
like storage details while providing performance that is on-par with raw memory
buffer operations. And while we're at it, let's look at removing the tedium of
multi-array dispatch and reducing the problem of 'template explosion'. The
remainder of this page details such a system.

# Best Practices for svtkDataArray Post-7.1 # {#SVTKAD-BestPractices}

We'll describe a new set of tools that make managing template instantiations
for efficient array access both easy and extensible. As an overview, the
following new features will be discussed:

* `svtkGenericDataArray`: The new templated base interface for all numeric
`svtkDataArray` subclasses.
* `svtkArrayDispatch`: Collection of code generation tools that allow concise
and precise specification of restrictable dispatch for up to 3 arrays
simultaneously.
* `svtkArrayDownCast`: Access to specialized downcast implementations from code
templates.
* `svtkDataArrayAccessor`: Provides `Get` and `Set` methods for
accessing/modifying array data as efficiently as possible. Allows a single
worker implementation to work efficiently with `svtkGenericDataArray`
subclasses, or fallback to use the `svtkDataArray` API if needed.
* `SVTK_ASSUME`: New abstraction for the compiler `__assume` directive to
provide optimization hints.

These will be discussed more fully, but as a preview, here's our familiar
`calcMagnitude` example implemented using these new tools:

~~~{.cpp}
// Modern implementation of calcMagnitude using new concepts in SVTK 7.1:
// A worker functor. The calculation is implemented in the function template
// for operator().
struct CalcMagnitudeWorker
{
  // The worker accepts SVTK array objects now, not raw memory buffers.
  template <typename VectorArray, typename MagnitudeArray>
  void operator()(VectorArray *vectors, MagnitudeArray *magnitude)
  {
    // This allows the compiler to optimize for the AOS array stride.
    SVTK_ASSUME(vectors->GetNumberOfComponents() == 3);
    SVTK_ASSUME(magnitude->GetNumberOfComponents() == 1);

    // These allow this single worker function to be used with both
    // the svtkDataArray 'double' API and the more efficient
    // svtkGenericDataArray APIs, depending on the template parameters:
    svtkDataArrayAccessor<VectorArray> v(vectors);
    svtkDataArrayAccessor<MagnitudeArray> m(magnitude);

    svtkIdType numVectors = vectors->GetNumberOfTuples();
    for (svtkIdType tupleIdx = 0; tupleIdx < numVectors; ++tupleIdx)
      {
      // Set and Get compile to inlined optimizable raw memory accesses for
      // svtkGenericDataArray subclasses.
      m.Set(tupleIdx, 0, std::sqrt(v.Get(tupleIdx, 0) * v.Get(tupleIdx, 0) +
                                   v.Get(tupleIdx, 1) * v.Get(tupleIdx, 1) +
                                   v.Get(tupleIdx, 2) * v.Get(tupleIdx, 2)));
      }
  }
};

void calcMagnitude(svtkDataArray *vectors, svtkDataArray *magnitude)
{
  // Create our worker functor:
  CalcMagnitudeWorker worker;

  // Define our dispatcher. We'll let vectors have any ValueType, but only
  // consider float/double arrays for magnitudes. These combinations will
  // use a 'fast-path' implementation generated by the dispatcher:
  typedef svtkArrayDispatch::Dispatch2ByValueType
    <
      svtkArrayDispatch::AllTypes, // ValueTypes allowed by first array
      svtkArrayDispatch::Reals // ValueTypes allowed by second array
    > Dispatcher;

  // Execute the dispatcher:
  if (!Dispatcher::Execute(vectors, magnitude, worker))
    {
    // If Execute() fails, it means the dispatch failed due to an
    // unsupported array type. In this case, it's likely that the magnitude
    // array is using an integral type. This is an uncommon case, so we won't
    // generate a fast path for these, but instead call an instantiation of
    // CalcMagnitudeWorker::operator()<svtkDataArray, svtkDataArray>.
    // Through the use of svtkDataArrayAccessor, this falls back to using the
    // svtkDataArray double API:
    worker(vectors, magnitude);
    }
}
~~~

# svtkGenericDataArray # {#SVTKAD-svtkGenericDataArray}

The `svtkGenericDataArray` class template drives the new `svtkDataArray` class
hierarchy. The ValueType is introduced here, both as a template parameter and a
class-scope `typedef`. This allows a typed API to be written that doesn't
require conversion to/from a common type (as `svtkDataArray` does with double).
It does not implement any storage details, however. Instead, it uses the CRTP
idiom to forward key method calls to a derived class without using a virtual
function call. By eliminating this indirection, `svtkGenericDataArray` defines
an interface that can be used to implement highly efficient code, because the
compiler is able to see past the method calls and optimize the underlying
memory accesses instead.

There are two main subclasses of `svtkGenericDataArray`:
`svtkAOSDataArrayTemplate` and `svtkSOADataArrayTemplate`. These implement
array-of-structs and struct-of-arrays storage, respectively.

# svtkTypeList # {#SVTKAD-svtkTypeList}

Type lists are a metaprogramming construct used to generate a list of C++
types. They are used in SVTK to implement restricted array dispatching. As we'll
see, `svtkArrayDispatch` offers ways to reduce the number of generated template
instantiations by enforcing constraints on the arrays used to dispatch. For
instance, if one wanted to only generate templated worker implementations for
`svtkFloatArray` and `svtkIntArray`, a typelist is used to specify this:

~~~{.cpp}
// Create a typelist of 2 types, svtkFloatArray and svtkIntArray:
typedef svtkTypeList::Create<svtkFloatArray, svtkIntArray> MyArrays;

Worker someWorker = ...;
svtkDataArray *someArray = ...;

// Use svtkArrayDispatch to generate code paths for these arrays:
svtkArrayDispatch::DispatchByArray<MyArrays>(someArray, someWorker);
~~~

There's not much to know about type lists as a user, other than how to create
them. As seen above, there is a set of macros named `svtkTypeList::Create<...>`,
where X is the number of types in the created list, and the arguments are the
types to place in the list. In the example above, the new type list is
typically bound to a friendlier name using a local `typedef`, which is a common
practice.

The `svtkTypeList.h` header defines some additional type list operations that
may be useful, such as deleting and appending types, looking up indices, etc.
`svtkArrayDispatch::FilterArraysByValueType` may come in handy, too. But for
working with array dispatches, most users will only need to create new ones, or
use one of the following predefined `svtkTypeLists`:

* `svtkArrayDispatch::Reals`: All floating point ValueTypes.
* `svtkArrayDispatch::Integrals`: All integral ValueTypes.
* `svtkArrayDispatch::AllTypes`: Union of Reals and Integrals.
* `svtkArrayDispatch::Arrays`: Default list of ArrayTypes to use in dispatches.

The last one is special -- `svtkArrayDispatch::Arrays` is a typelist of
ArrayTypes set application-wide when SVTK is built. This `svtkTypeList` of
`svtkDataArray` subclasses is used for unrestricted dispatches, and is the list
that gets filtered when restricting a dispatch to specific ValueTypes.

Refining this list allows the user building SVTK to have some control over the
dispatch process. If SOA arrays are never going to be used, they can be removed
from dispatch calls, reducing compile times and binary size. On the other hand,
a user applying in-situ techniques may want them available, because they'll be
used to import views of intermediate results.

By default, `svtkArrayDispatch::Arrays` contains all AOS arrays. The `CMake`
option `SVTK_DISPATCH_SOA_ARRAYS` will enable SOA array dispatch as well. More
advanced possibilities exist and are described in
`SVTK/Common/Core/svtkCreateArrayDispatchArrayList.cmake`.

# svtkArrayDownCast # {#SVTKAD-svtkArrayDownCast}

In SVTK, all subclasses of `svtkObject` (including the data arrays) support a
downcast method called `SafeDownCast`. It is used similarly to the C++
`dynamic_cast` -- given an object, try to cast it to a more derived type or
return `NULL` if the object is not the requested type. Say we have a
`svtkDataArray` and want to test if it is actually a `svtkFloatArray`. We can do
this:

~~~{.cpp}
void DoSomeAction(svtkDataArray *dataArray)
{
  svtkFloatArray *floatArray = svtkFloatArray::SafeDownCast(dataArray);
  if (floatArray)
    {
    // ... (do work with float array)
    }
}
~~~

This works, but it can pose a serious problem if `DoSomeAction` is called
repeatedly. `SafeDownCast` works by performing a series of virtual calls and
string comparisons to determine if an object falls into a particular class
hierarchy. These string comparisons add up and can actually dominate
computational resources if an algorithm implementation calls `SafeDownCast` in
a tight loop.

In such situations, it's ideal to restructure the algorithm so that the
downcast only happens once and the same result is used repeatedly, but
sometimes this is not possible. To lessen the cost of downcasting arrays, a
`FastDownCast` method exists for common subclasses of `svtkAbstractArray`. This
replaces the string comparisons with a single virtual call and a few integer
comparisons and is far cheaper than the more general SafeDownCast. However, not
all array implementations support the `FastDownCast` method.

This creates a headache for templated code. Take the following example:

~~~{.cpp}
template <typename ArrayType>
void DoSomeAction(svtkAbstractArray *array)
{
  ArrayType *myArray = ArrayType::SafeDownCast(array);
  if (myArray)
    {
    // ... (do work with myArray)
    }
}
~~~

We cannot use `FastDownCast` here since not all possible ArrayTypes support it.
But we really want that performance increase for the ones that do --
`SafeDownCast`s are really slow! `svtkArrayDownCast` fixes this issue:

~~~{.cpp}
template <typename ArrayType>
void DoSomeAction(svtkAbstractArray *array)
{
  ArrayType *myArray = svtkArrayDownCast<ArrayType>(array);
  if (myArray)
    {
    // ... (do work with myArray)
    }
}
~~~

`svtkArrayDownCast` automatically selects `FastDownCast` when it is defined for
the ArrayType, and otherwise falls back to `SafeDownCast`. This is the
preferred array downcast method for performance, uniformity, and reliability.

# svtkDataArrayAccessor # {#SVTKAD-svtkDataArrayAccessor}

Array dispatching relies on having templated worker code carry out some
operation. For instance, take this `svtkArrayDispatch` code that locates the
maximum value in an array:

~~~{.cpp}
// Stores the tuple/component coordinates of the maximum value:
struct FindMax
{
  svtkIdType Tuple; // Result
  int Component; // Result

  FindMax() : Tuple(-1), Component(-1) {}

  template <typename ArrayT>
  void operator()(ArrayT *array)
  {
    // The type to use for temporaries, and a temporary to store
    // the current maximum value:
    typedef typename ArrayT::ValueType ValueType;
    ValueType max = std::numeric_limits<ValueType>::min();

    // Iterate through all tuples and components, noting the location
    // of the largest element found.
    svtkIdType numTuples = array->GetNumberOfTuples();
    int numComps = array->GetNumberOfComponents();
    for (svtkIdType tupleIdx = 0; tupleIdx < numTuples; ++tupleIdx)
      {
      for (int compIdx = 0; compIdx < numComps; ++compIdx)
        {
        if (max < array->GetTypedComponent(tupleIdx, compIdx))
          {
          max = array->GetTypedComponent(tupleIdx, compIdx);
          this->Tuple = tupleIdx;
          this->Component = compIdx;
          }
        }
      }
  }
};

void someFunction(svtkDataArray *array)
{
  FindMax maxWorker;
  svtkArrayDispatch::Dispatch::Execute(array, maxWorker);
  // Do work using maxWorker.Tuple and maxWorker.Component...
}
~~~

There's a problem, though. Recall that only the arrays in
`svtkArrayDispatch::Arrays` are tested for dispatching. What happens if the
array passed into someFunction wasn't on that list?

The dispatch will fail, and `maxWorker.Tuple` and `maxWorker.Component` will be
left to their initial values of -1. That's no good. What if `someFunction` is a
critical path where we want to use a fast dispatched worker if possible, but
still have valid results to use if dispatching fails? Well, we can fall back on
the `svtkDataArray` API and do things the slow way in that case. When a
dispatcher is given an unsupported array, Execute() returns false, so let's
just add a backup implementation:

~~~{.cpp}
// Stores the tuple/component coordinates of the maximum value:
struct FindMax
{ /* As before... */ };

void someFunction(svtkDataArray *array)
{
  FindMax maxWorker;
  if (!svtkArrayDispatch::Dispatch::Execute(array, maxWorker))
    {
    // Reimplement FindMax::operator(), but use the svtkDataArray API's
    // "virtual double GetComponent()" instead of the more efficient
    // "ValueType GetTypedComponent()" from svtkGenericDataArray.
    }
}
~~~

Ok, that works. But ugh...why write the same algorithm twice? That's extra
debugging, extra testing, extra maintenance burden, and just plain not fun.

Enter `svtkDataArrayAccessor`. This utility template does a very simple, yet
useful, job. It provides component and tuple based `Get` and `Set` methods that
will call the corresponding method on the array using either the `svtkDataArray`
or `svtkGenericDataArray` API, depending on the class's template parameter. It
also defines an `APIType`, which can be used to allocate temporaries, etc. This
type is double for `svtkDataArray`s and `svtkGenericDataArray::ValueType` for
`svtkGenericDataArray`s.

Another nice benefit is that `svtkDataArrayAccessor` has a more compact API. The
only defined methods are Get and Set, and they're overloaded to work on either
tuples or components (though component access is encouraged as it is much, much
more efficient). Note that all non-element access operations (such as
`GetNumberOfTuples`) should still be called on the array pointer using
`svtkDataArray` API.

Using `svtkDataArrayAccessor`, we can write a single worker template that works
for both `svtkDataArray` and `svtkGenericDataArray`, without a loss of
performance in the latter case. That worker looks like this:

~~~{.cpp}
// Better, uses svtkDataArrayAccessor:
struct FindMax
{
  svtkIdType Tuple; // Result
  int Component; // Result

  FindMax() : Tuple(-1), Component(-1) {}

  template <typename ArrayT>
  void operator()(ArrayT *array)
  {
    // Create the accessor:
    svtkDataArrayAccessor<ArrayT> access(array);

    // Prepare the temporary. We'll use the accessor's APIType instead of
    // ArrayT::ValueType, since that is appropriate for the svtkDataArray
    // fallback:
    typedef typename svtkDataArrayAccessor<ArrayT>::APIType ValueType;
    ValueType max = std::numeric_limits<ValueType>::min();

    // Iterate as before, but use access.Get instead of
    // array->GetTypedComponent. GetTypedComponent is still used
    // when ArrayT is a svtkGenericDataArray, but
    // svtkDataArray::GetComponent is now used as a fallback when ArrayT
    // is svtkDataArray.
    svtkIdType numTuples = array->GetNumberOfTuples();
    int numComps = array->GetNumberOfComponents();
    for (svtkIdType tupleIdx = 0; tupleIdx < numTuples; ++tupleIdx)
      {
      for (int compIdx = 0; compIdx < numComps; ++compIdx)
        {
        if (max < access.Get(tupleIdx, compIdx))
          {
          max = access.Get(tupleIdx, compIdx);
          this->Tuple = tupleIdx;
          this->Component = compIdx;
          }
        }
      }
  }
};
~~~

Now when we call `operator()` with say, `ArrayT=svtkFloatArray`, we'll get an
optimized, efficient code path. But we can also call this same implementation
with `ArrayT=svtkDataArray` and still get a correct result (assuming that the
`svtkDataArray`'s double API represents the data well enough).

Using the `svtkDataArray` fallback path is straightforward. At the call site:

~~~{.cpp}
void someFunction(svtkDataArray *array)
{
  FindMax maxWorker;
  if (!svtkArrayDispatch::Dispatch::Execute(array, maxWorker))
    {
    maxWorker(array); // Dispatch failed, call svtkDataArray fallback
    }
  // Do work using maxWorker.Tuple and maxWorker.Component -- now we know
  // for sure that they're initialized!
}
~~~

Using the above pattern for calling a worker and always going through
`svtkDataArrayAccessor` to `Get`/`Set` array elements ensures that any worker
implementation can be its own fallback path.

# SVTK_ASSUME # {#SVTKAD-SVTK_ASSUME}

While performance testing the new array classes, we compared the performance of
a dispatched worker using the `svtkDataArrayAccessor` class to the same
algorithm using raw memory buffers. We managed to achieve the same performance
out of the box for most cases, using both AOS and SOA array implementations. In
fact, with `--ffast-math` optimizations on GCC 4.9, the optimizer is able to
remove all function calls and apply SIMD vectorized instructions in the
dispatched worker, showing that the new array API is thin enough that the
compiler can see the algorithm in terms of memory access.

But there was one case where performance suffered. If iterating through an AOS
data array with a known number of components using `GetTypedComponent`, the raw
pointer implementation initially outperformed the dispatched array. To
understand why, note that the AOS implementation of `GetTypedComponent` is along
the lines of:

~~~{.cpp}
ValueType svtkAOSDataArrayTemplate::GetTypedComponent(svtkIdType tuple,
                                                     int comp) const
{
  // AOSData is a ValueType* pointing at the base of the array data.
  return this->AOSData[tuple * this->NumberOfComponents + comp];
}
~~~

Because `NumberOfComponents` is unknown at compile time, the optimizer cannot
assume anything about the stride of the components in the array. This leads to
missed optimizations for vectorized read/writes and increased complexity in the
instructions used to iterate through the data.

For such cases where the number of components is, in fact, known at compile
time (due to a calling function performing some validation, for instance), it
is possible to tell the compiler about this fact using `SVTK_ASSUME`.

`SVTK_ASSUME` wraps a compiler-specific `__assume` statement, which is used to
pass such optimization hints. Its argument is an expression of some condition
that is guaranteed to always be true. This allows more aggressive optimizations
when used correctly, but be forewarned that if the condition is not met at
runtime, the results are unpredictable and likely catastrophic.

But if we're writing a filter that only operates on 3D point sets, we know the
number of components in the point array will always be 3. In this case we can
write:

~~~{.cpp}
SVTK_ASSUME(pointsArray->GetNumberOfComponents() == 3);
~~~

in the worker function and this instructs the compiler that the array's
internal `NumberOfComponents` variable will always be 3, and thus the stride of
the array is known. Of course, the caller of this worker function should ensure
that this is a 3-component array and fail gracefully if it is not.

There are many scenarios where `SVTK_ASSUME` can offer a serious performance
boost, the case of known tuple size is a common one that's really worth
remembering.

# svtkArrayDispatch # {#SVTKAD-svtkArrayDispatch}

The dispatchers implemented in the svtkArrayDispatch namespace provide array
dispatching with customizable restrictions on code generation and a simple
syntax that hides the messy details of type resolution and multi-array
dispatch. There are several "flavors" of dispatch available that operate on up
to three arrays simultaneously.

## Components Of A Dispatch ## {#SVTKAD-ComponentsOfADispatch}

Using the `svtkArrayDispatch` system requires three elements: the array(s), the
worker, and the dispatcher.

### The Arrays ### {#SVTKAD-TheArrays}

All dispatched arrays must be subclasses of `svtkDataArray`. It is important to
identify as many restrictions as possible. Must every ArrayType be considered
during dispatch, or is the array's ValueType (or even the ArrayType itself)
restricted? If dispatching multiple arrays at once, are they expected to have
the same ValueType? These scenarios are common, and these conditions can be
used to reduce the number of instantiations of the worker template.

### The Worker ### {#SVTKAD-TheWorker}

The worker is some generic callable. In C++98, a templated functor is a good
choice. In C++14, a generic lambda is a usable option as well. For our
purposes, we'll only consider the functor approach, as C++14 is a long ways off
for core SVTK code.

At a minimum, the worker functor should define `operator()` to make it
callable. This should be a function template with a template parameter for each
array it should handle. For a three array dispatch, it should look something
like this:

~~~{.cpp}
struct ThreeArrayWorker
{
  template <typename Array1T, typename Array2T, typename Array3T>
  void operator()(Array1T *array1, Array2T *array2, Array3T *array3)
  {
  /* Do stuff... */
  }
};
~~~

At runtime, the dispatcher will call `ThreeWayWorker::operator()` with a set of
`Array1T`, `Array2T`, and `Array3T` that satisfy any dispatch restrictions.

Workers can be stateful, too, as seen in the `FindMax` worker earlier where the
worker simply identified the component and tuple id of the largest value in the
array. The functor stored them for the caller to use in further analysis:

~~~{.cpp}
// Example of a stateful dispatch functor:
struct FindMax
{
  // Functor state, holds results that are accessible to the caller:
  svtkIdType Tuple;
  int Component;

  // Set initial values:
  FindMax() : Tuple(-1), Component(-1) {}

  // Template method to set Tuple and Component ivars:
  template <typename ArrayT>
  void operator()(ArrayT *array)
  {
    /* Do stuff... */
  }
};
~~~

### The Dispatcher ### {#SVTKAD-TheDispatcher}

The dispatcher is the workhorse of the system. It is responsible for applying
restrictions, resolving array types, and generating the requested template
instantiations. It has responsibilities both at run-time and compile-time.

During compilation, the dispatcher will identify the valid combinations of
arrays that can be used according to the restrictions. This is done by starting
with a typelist of arrays, either supplied as a template parameter or by
defaulting to `svtkArrayDispatch::Arrays`, and filtering them by ValueType if
needed. For multi-array dispatches, additional restrictions may apply, such as
forcing the second and third arrays to have the same ValueType as the first. It
must then generate the required code for the dispatch -- that is, the templated
worker implementation must be instantiated for each valid combination of
arrays.

At runtime, it tests each of the dispatched arrays to see if they match one of
the generated code paths. Runtime type resolution is carried out using
`svtkArrayDownCast` to get the best performance available for the arrays of
interest. If it finds a match, it calls the worker's `operator()` method with
the properly typed arrays. If no match is found, it returns `false` without
executing the worker.

## Restrictions: Why They Matter ## {#SVTKAD-RestrictionsWhyTheyMatter}

We've made several mentions of using restrictions to reduce the number of
template instantiations during a dispatch operation. You may be wondering if it
really matters so much. Let's consider some numbers.

SVTK is configured to use 13 ValueTypes for numeric data. These are the standard
numeric types `float`, `int`, `unsigned char`, etc. By default, SVTK will define
`svtkArrayDispatch::Arrays` to use all 13 types with `svtkAOSDataArrayTemplate`
for the standard set of dispatchable arrays. If enabled during compilation, the
SOA data arrays are added to this list for a total of 26 arrays.

Using these 26 arrays in a single, unrestricted dispatch will result in 26
instantiations of the worker template. A double dispatch will generate 676
workers. A triple dispatch with no restrictions creates a whopping 17,576
functions to handle the possible combinations of arrays. That's a _lot_ of
instructions to pack into the final binary object.

Applying some simple restrictions can reduce this immensely. Say we know that
the arrays will only contain `float`s or `double`s. This would reduce the
single dispatch to 4 instantiations, the double dispatch to 16, and the triple
to 64. We've just reduced the generated code size significantly. We could even
apply such a restriction to just create some 'fast-paths' and let the integral
types fallback to using the `svtkDataArray` API by using
`svtkDataArrayAccessor`s. Dispatch restriction is a powerful tool for reducing
the compiled size of a binary object.

Another common restriction is that all arrays in a multi-array dispatch have
the same ValueType, even if that ValueType is not known at compile time. By
specifying this restriction, a double dispatch on all 26 AOS/SOA arrays will
only produce 52 worker instantiations, down from 676. The triple dispatch drops
to 104 instantiations from 17,576.

Always apply restrictions when they are known, especially for multi-array
dispatches. The savings are worth it.

## Types of Dispatchers ## {#SVTKAD-TypesOfDispatchers}

Now that we've discussed the components of a dispatch operation, what the
dispatchers do, and the importance of restricting dispatches, let's take a look
at the types of dispatchers available.

---

### svtkArrayDispatch::Dispatch ### {#SVTKAD-Dispatch}

This family of dispatchers take no parameters and perform an unrestricted
dispatch over all arrays in `svtkArrayDispatch::Arrays`.

__Variations__:
* `svtkArrayDispatch::Dispatch`: Single dispatch.
* `svtkArrayDispatch::Dispatch2`: Double dispatch.
* `svtkArrayDispatch::Dispatch3`: Triple dispatch.

__Arrays considered__: All arrays in `svtkArrayDispatch::Arrays`.

__Restrictions__: None.

__Usecase__: Used when no useful information exists that can be used to apply
restrictions.

__Example Usage__:

~~~{.cpp}
svtkArrayDispatch::Dispatch::Execute(array, worker);
~~~

---

### svtkArrayDispatch::DispatchByArray ### {#SVTKAD-DispatchByArray}

This family of dispatchers takes a `svtkTypeList` of explicit array types to use
during dispatching. They should only be used when an array's exact type is
restricted. If dispatching multiple arrays and only one has such type
restrictions, use `svtkArrayDispatch::Arrays` (or a filtered version) for the
unrestricted arrays.

__Variations__:
* `svtkArrayDispatch::DispatchByArray`: Single dispatch.
* `svtkArrayDispatch::Dispatch2ByArray`: Double dispatch.
* `svtkArrayDispatch::Dispatch3ByArray`: Triple dispatch.

__Arrays considered__: All arrays explicitly listed in the parameter lists.

__Restrictions__: Array must be explicitly listed in the dispatcher's type.

__Usecase__: Used when one or more arrays have known implementations.

__Example Usage__:

An example here would be a filter that processes an input array of some
integral type and produces either a `svtkDoubleArray` or a `svtkFloatArray`,
depending on some condition. Since the input array's implementation is unknown
(it comes from outside the filter), we'll rely on a ValueType-filtered version
of `svtkArrayDispatch::Arrays` for its type. However, we know the output array
is either `svtkDoubleArray` or `svtkFloatArray`, so we'll want to be sure to
apply that restriction:

~~~{.cpp}
// input has an unknown implementation, but an integral ValueType.
svtkDataArray *input = ...;

// Output is always either svtkFloatArray or svtkDoubleArray:
svtkDataArray *output = someCondition ? svtkFloatArray::New()
                                     : svtkDoubleArray::New();

// Define the valid ArrayTypes for input by filtering
// svtkArrayDispatch::Arrays to remove non-integral types:
typedef typename svtkArrayDispatch::FilterArraysByValueType
  <
  svtkArrayDispatch::Arrays,
  svtkArrayDispatch::Integrals
  >::Result InputTypes;

// For output, create a new svtkTypeList with the only two possibilities:
typedef svtkTypeList::Create<svtkFloatArray, svtkDoubleArray> OutputTypes;

// Typedef the dispatch to a more manageable name:
typedef svtkArrayDispatch::Dispatch2ByArray
  <
  InputTypes,
  OutputTypes
  > MyDispatch;

// Execute the dispatch:
MyDispatch::Execute(input, output, someWorker);
~~~

---

### svtkArrayDispatch::DispatchByValueType ### {#SVTKAD-DispatchByValueType}

This family of dispatchers takes a svtkTypeList of ValueTypes for each array and
restricts dispatch to only arrays in svtkArrayDispatch::Arrays that have one of
the specified value types.

__Variations__:
* `svtkArrayDispatch::DispatchByValueType`: Single dispatch.
* `svtkArrayDispatch::Dispatch2ByValueType`: Double dispatch.
* `svtkArrayDispatch::Dispatch3ByValueType`: Triple dispatch.

__Arrays considered__: All arrays in `svtkArrayDispatch::Arrays` that meet the
ValueType requirements.

__Restrictions__: Arrays that do not satisfy the ValueType requirements are
eliminated.

__Usecase__: Used when one or more of the dispatched arrays has an unknown
implementation, but a known (or restricted) ValueType.

__Example Usage__:

Here we'll consider a filter that processes three arrays. The first is a
complete unknown. The second is known to hold `unsigned char`, but we don't
know the implementation. The third holds either `double`s or `float`s, but its
implementation is also unknown.

~~~{.cpp}
// Complete unknown:
svtkDataArray *array1 = ...;
// Some array holding unsigned chars:
svtkDataArray *array2 = ...;
// Some array holding either floats or doubles:
svtkDataArray *array3 = ...;

// Typedef the dispatch to a more manageable name:
typedef svtkArrayDispatch::Dispatch3ByValueType
  <
  svtkArrayDispatch::AllTypes,
  svtkTypeList::Create<unsigned char>,
  svtkArrayDispatch::Reals
  > MyDispatch;

// Execute the dispatch:
MyDispatch::Execute(array1, array2, array3, someWorker);
~~~

---

### svtkArrayDispatch::DispatchByArrayWithSameValueType ### {#SVTKAD-DispatchByArrayWithSameValueType}

This family of dispatchers takes a `svtkTypeList` of ArrayTypes for each array
and restricts dispatch to only consider arrays from those typelists, with the
added requirement that all dispatched arrays share a ValueType.

__Variations__:
* `svtkArrayDispatch::Dispatch2ByArrayWithSameValueType`: Double dispatch.
* `svtkArrayDispatch::Dispatch3ByArrayWithSameValueType`: Triple dispatch.

__Arrays considered__: All arrays in the explicit typelists that meet the
ValueType requirements.

__Restrictions__: Combinations of arrays with differing ValueTypes are
eliminated.

__Usecase__: When one or more arrays are known to belong to a restricted set of
ArrayTypes, and all arrays are known to share the same ValueType, regardless of
implementation.

__Example Usage__:

Let's consider a double array dispatch, with `array1` known to be one of four
common array types (AOS `float`, `double`, `int`, and `svtkIdType` arrays), and
the other is a complete unknown, although we know that it holds the same
ValueType as `array1`.

~~~{.cpp}
// AOS float, double, int, or svtkIdType array:
svtkDataArray *array1 = ...;
// Unknown implementation, but the ValueType matches array1:
svtkDataArray *array2 = ...;

// array1's possible types:
typedef svtkTypeList;:Create<svtkFloatArray, svtkDoubleArray,
                            svtkIntArray, svtkIdTypeArray> Array1Types;

// array2's possible types:
typedef typename svtkArrayDispatch::FilterArraysByValueType
  <
  svtkArrayDispatch::Arrays,
  svtkTypeList::Create<float, double, int, svtkIdType>
  > Array2Types;

// Typedef the dispatch to a more manageable name:
typedef svtkArrayDispatch::Dispatch2ByArrayWithSameValueType
  <
  Array1Types,
  Array2Types
  > MyDispatch;

// Execute the dispatch:
MyDispatch::Execute(array1, array2, someWorker);
~~~

---

### svtkArrayDispatch::DispatchBySameValueType ### {#SVTKAD-DispatchBySameValueType}

This family of dispatchers takes a single `svtkTypeList` of ValueType and
restricts dispatch to only consider arrays from `svtkArrayDispatch::Arrays` with
those ValueTypes, with the added requirement that all dispatched arrays share a
ValueType.

__Variations__:
* `svtkArrayDispatch::Dispatch2BySameValueType`: Double dispatch.
* `svtkArrayDispatch::Dispatch3BySameValueType`: Triple dispatch.
* `svtkArrayDispatch::Dispatch2SameValueType`: Double dispatch using
`svtkArrayDispatch::AllTypes`.
* `svtkArrayDispatch::Dispatch3SameValueType`: Triple dispatch using
`svtkArrayDispatch::AllTypes`.

__Arrays considered__: All arrays in `svtkArrayDispatch::Arrays` that meet the
ValueType requirements.

__Restrictions__: Combinations of arrays with differing ValueTypes are
eliminated.

__Usecase__: When one or more arrays are known to belong to a restricted set of
ValueTypes, and all arrays are known to share the same ValueType, regardless of
implementation.

__Example Usage__:

Let's consider a double array dispatch, with `array1` known to be one of four
common ValueTypes (`float`, `double`, `int`, and `svtkIdType` arrays), and
`array2` known to have the same ValueType as `array1`.

~~~{.cpp}
// Some float, double, int, or svtkIdType array:
svtkDataArray *array1 = ...;
// Unknown, but the ValueType matches array1:
svtkDataArray *array2 = ...;

// The allowed ValueTypes:
typedef svtkTypeList::Create<float, double, int, svtkIdType> ValidValueTypes;

// Typedef the dispatch to a more manageable name:
typedef svtkArrayDispatch::Dispatch2BySameValueType
  <
  ValidValueTypes
  > MyDispatch;

// Execute the dispatch:
MyDispatch::Execute(array1, array2, someWorker);
~~~

---

# Advanced Usage # {#SVTKAD-AdvancedUsage}

## Accessing Memory Buffers ## {#SVTKAD-AccessingMemoryBuffers}

Despite the thin `svtkGenericDataArray` API's nice feature that compilers can
optimize memory accesses, sometimes there are still legitimate reasons to
access the underlying memory buffer. This can still be done safely by providing
overloads to your worker's `operator()` method. For instance,
`svtkDataArray::DeepCopy` uses a generic implementation when mixed array
implementations are used, but has optimized overloads for copying between
arrays with the same ValueType and implementation. The worker for this dispatch
is shown below as an example:

~~~{.cpp}
// Copy tuples from src to dest:
struct DeepCopyWorker
{
  // AoS --> AoS same-type specialization:
  template <typename ValueType>
  void operator()(svtkAOSDataArrayTemplate<ValueType> *src,
                  svtkAOSDataArrayTemplate<ValueType> *dst)
  {
    std::copy(src->Begin(), src->End(), dst->Begin());
  }

  // SoA --> SoA same-type specialization:
  template <typename ValueType>
  void operator()(svtkSOADataArrayTemplate<ValueType> *src,
                  svtkSOADataArrayTemplate<ValueType> *dst)
  {
    svtkIdType numTuples = src->GetNumberOfTuples();
    for (int comp; comp < src->GetNumberOfComponents(); ++comp)
      {
      ValueType *srcBegin = src->GetComponentArrayPointer(comp);
      ValueType *srcEnd = srcBegin + numTuples;
      ValueType *dstBegin = dst->GetComponentArrayPointer(comp);

      std::copy(srcBegin, srcEnd, dstBegin);
      }
  }

  // Generic implementation:
  template <typename Array1T, typename Array2T>
  void operator()(Array1T *src, Array2T *dst)
  {
    svtkDataArrayAccessor<Array1T> s(src);
    svtkDataArrayAccessor<Array2T> d(dst);

    typedef typename svtkDataArrayAccessor<Array2T>::APIType DestType;

    svtkIdType tuples = src->GetNumberOfTuples();
    int comps = src->GetNumberOfComponents();

    for (svtkIdType t = 0; t < tuples; ++t)
      {
      for (int c = 0; c < comps; ++c)
        {
        d.Set(t, c, static_cast<DestType>(s.Get(t, c)));
        }
      }
  }
};
~~~

# Putting It All Together # {#SVTKAD-PuttingItAllTogether}

Now that we've explored the new tools introduced with SVTK 7.1 that allow
efficient, implementation agnostic array access, let's take another look at the
`calcMagnitude` example from before and identify the key features of the
implementation:

~~~{.cpp}
// Modern implementation of calcMagnitude using new concepts in SVTK 7.1:
struct CalcMagnitudeWorker
{
  template <typename VectorArray, typename MagnitudeArray>
  void operator()(VectorArray *vectors, MagnitudeArray *magnitude)
  {
    SVTK_ASSUME(vectors->GetNumberOfComponents() == 3);
    SVTK_ASSUME(magnitude->GetNumberOfComponents() == 1);

    svtkDataArrayAccessor<VectorArray> v(vectors);
    svtkDataArrayAccessor<MagnitudeArray> m(magnitude);

    svtkIdType numVectors = vectors->GetNumberOfTuples();
    for (svtkIdType tupleIdx = 0; tupleIdx < numVectors; ++tupleIdx)
      {
      m.Set(tupleIdx, 0, std::sqrt(v.Get(tupleIdx, 0) * v.Get(tupleIdx, 0) +
                                   v.Get(tupleIdx, 1) * v.Get(tupleIdx, 1) +
                                   v.Get(tupleIdx, 2) * v.Get(tupleIdx, 2)));
      }
  }
};

void calcMagnitude(svtkDataArray *vectors, svtkDataArray *magnitude)
{
  CalcMagnitudeWorker worker;
  typedef svtkArrayDispatch::Dispatch2ByValueType
    <
      svtkArrayDispatch::AllTypes,
      svtkArrayDispatch::Reals
    > Dispatcher;

  if (!Dispatcher::Execute(vectors, magnitude, worker))
    {
    worker(vectors, magnitude); // svtkDataArray fallback
    }
}
~~~

This implementation:

* Uses dispatch restrictions to reduce the number of instantiated templated
worker functions.
 * Assuming 26 types are in `svtkArrayDispatch::Arrays` (13 AOS + 13 SOA).
 * The first array is unrestricted. All 26 array types are considered.
 * The second array is restricted to `float` or `double` ValueTypes, which
 translates to 4 array types (one each, SOA and AOS).
 * 26 * 4 = 104 possible combinations exist. We've eliminated 26 * 22 = 572
 combinations that an unrestricted double-dispatch would have generated (it
 would create 676 instantiations).
* The calculation is still carried out at `double` precision when the ValueType
restrictions are not met.
 * Just because we don't want those other 572 cases to have special code
 generated doesn't necessarily mean that we wouldn't want them to run.
 * Thanks to `svtkDataArrayAccessor`, we have a fallback implementation that
 reuses our templated worker code.
 * In this case, the dispatch is really just a fast-path implementation for
 floating point output types.
* The performance should be identical to iterating through raw memory buffers.
 * The `svtkGenericDataArray` API is transparent to the compiler. The
 specialized instantiations of `operator()` can be heavily optimized since the
 memory access patterns are known and well-defined.
 * Using `SVTK_ASSUME` tells the compiler that the arrays have known strides,
 allowing further compile-time optimizations.

Hopefully this has convinced you that the `svtkArrayDispatch` and related tools
are worth using to create flexible, efficient, typesafe implementations for
your work with SVTK. Please direct any questions you may have on the subject to
the [SVTK Discourse][] forum.

[SVTK Discourse]: https://discourse.svtk.org
