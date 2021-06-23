SVTK-m 1.4 Release Notes
=======================

# Table of Contents
1. [Core](#Core)
    - Remove templates from `ControlSignature` field tags
    - Worklets can now be specialized for a specific device adapter
    - Worklets now support an execution mask
    - Redesign SVTK-m Runtime Device Tracking
    - `svtkm::cont::Initialize` added to make setting up SVTK-m runtime state easier
2. [ArrayHandle](#ArrayHandle)
    - Add `svtkm::cont::ArrayHandleVirtual`
    - `svtkm::cont::ArrayHandleZip` provides a consistent API even with non-writable handles
    - `svtkm::cont::VariantArrayHandle` replaces `svtkm::cont::DynamicArrayHandle`
    - `svtkm::cont::VariantArrayHandle` CastAndCall supports casting to concrete types
    - `svtkm::cont::VariantArrayHandle::AsVirtual<T>()` performs casting
    - `StorageBasic::StealArray()` now provides delete function to new owner
3. [Control Environment](#Control-Environment)
    - `svtkm::cont::CellLocatorGeneral` has been added
    - `svtkm::cont::CellLocatorTwoLevelUniformGrid` has been renamed to `svtkm::cont::CellLocatorUniformBins`
    - `svtkm::cont::Timer` now supports  asynchronous and device independent timers
    - `svtkm::cont::DeviceAdapterId` construction from strings are now case-insensitive
    - `svtkm::cont::Initialize` will only parse known arguments
4. [Execution Environment](#Execution-Environment)
    - SVTK-m logs details about each CUDA kernel launch
    - SVTK-m CUDA allocations can have managed memory (cudaMallocManaged) enabled/disabled from C++
    - SVTK-m CUDA kernel scheduling improved including better defaults, and user customization support
    - SVTK-m Reduction algorithm now supports differing input and output types
    - Added specialized operators for ArrayPortalValueReference
5. [Worklets and Filters](#Worklets-and-Filters)
    - `svtkm::worklet::Invoker` now supports worklets which require a Scatter object
    - `BitFields` are now a support field input/out type for SVTK-m worklets
    - Added a Point Merging worklet
    - `svtkm::filter::CleanGrid` now can do point merging
    - Added a connected component worklets and filters
6. [Build](#Build)
    - CMake 3.8+ now required to build SVTK-m
    - SVTK-m now can verify that it installs itself correctly
    - SVTK-m now requires `CUDA` separable compilation to build
    - SVTK-m provides a `svtkm_filter` CMake target
    - `svtkm::cont::CellLocatorBoundingIntervalHierarchy` is compiled into `svtkm_cont`
7. [Other](#Other)
    - LodePNG added as a thirdparty package
    - Optionparser added as a thirdparty package
    - Thirdparty diy now can coexist with external diy
    - Merge benchmark executables into a device dependent shared library
    - Merge rendering testing executables to a shared library
    - Merge worklet testing executables into a device dependent shared library
    - SVTK-m runtime device detection properly handles busy CUDA devices

# Core

## Remove templates from `ControlSignature` field tags

Previously, several of the `ControlSignature` tags had a template to
specify a type list. This was to specify potential valid value types for an
input array. The importance of this typelist was to limit the number of
code paths created when resolving a `svtkm::cont::VariantArrayHandle`
(formerly a `DynamicArrayHandle`). This (potentially) reduced the compile
time, the size of libraries/executables, and errors from unexpected types.

Much has changed since this feature was originally implemented. Since then,
the filter infrastructure has been created, and it is through this that
most dynamic worklet invocations happen. However, since the filter
infrastrcture does its own type resolution (and has its own policies) the
type arguments in `ControlSignature` are now of little value.

### Script to update code

This update requires changes to just about all code implementing a SVTK-m
worklet. To facilitate the update of this code to these new changes (not to
mention all the code in SVTK-m) a script is provided to automatically remove
these template parameters from SVTK-m code.

This script is at
[Utilities/Scripts/update-control-signature-tags.sh](../../Utilities/Scripts/update-control-signature-tags.sh).
It needs to be run in a Unix-compatible shell. It takes a single argument,
which is a top level directory to modify files. The script processes all C++
source files recursively from that directory.

### Selecting data types for auxiliary filter fields

The main rational for making these changes is that the types of the inputs
to worklets is almost always already determined by the calling filter.
However, although it is straightforward to specify the type of the "main"
(active) scalars in a filter, it is less clear what to do for additional
fields if a filter needs a second or third field.

Typically, in the case of a second or third field, it is up to the
`DoExecute` method in the filter implementation to apply a policy to that
field. When applying a policy, you give it a policy object (nominally
passed by the user) and a traits of the filter. Generally, the accepted
list of types for a field should be part of the filter's traits. For
example, consider the `WarpVector` filter. This filter only works on
`Vec`s of size 3, so its traits class looks like this.

``` cpp
template <>
class FilterTraits<WarpVector>
{
public:
  // WarpVector can only applies to Float and Double Vec3 arrays
  using InputFieldTypeList = svtkm::TypeListTagFieldVec3;
};
```

However, the `WarpVector` filter also requires two fields instead of one.
The first (active) field is handled by its superclass (`FilterField`), but
the second (auxiliary) field must be managed in the `DoExecute`. Generally,
this can be done by simply applying the policy with the filter traits.

### The corner cases

Most of the calls to worklets happen within filter implementations, which
have their own way of narrowing down potential types (as previously
described). The majority of the remainder either use static types or work
with a variety of types.

However, there is a minority of corner cases that require a reduction of
types. Since the type argument of the worklet `ControlSignature` arguments
are no longer available, the narrowing of types must be done before the
call to `Invoke`.

This narrowing of arguments is not particularly difficult. Such type-unsure
arguments usually come from a `VariantArrayHandle` (or something that uses
one). You can select the types from a `VariantArrayHandle` simply by using
the `ResetTypes` method. For example, say you know that a variant array is
supposed to be a scalar.

``` cpp
dispatcher.Invoke(variantArray.ResetTypes(svtkm::TypeListTagFieldScalar()),
                  staticArray);
```

Even more common is to have a `svtkm::cont::Field` object. A `Field` object
internally holds a `VariantArrayHandle`, which is accessible via the
`GetData` method.

``` cpp
dispatcher.Invoke(field.GetData().ResetTypes(svtkm::TypeListTagFieldScalar()),
                  staticArray);
```

### Change in executable size

The whole intention of these template parameters in the first place was to
reduce the number of code paths compiled. The hypothesis of this change was
that in the current structure the code paths were not being reduced much
if at all. If that is true, the size of executables and libraries should
not change.

Here is a recording of the library and executable sizes before this change
(using `ds -h`).

```
3.0M    libsvtkm_cont-1.2.1.dylib
6.2M    libsvtkm_rendering-1.2.1.dylib
312K    Rendering_SERIAL
312K    Rendering_TBB
 22M    Worklets_SERIAL
 23M    Worklets_TBB
 22M    UnitTests_svtkm_filter_testing
5.7M    UnitTests_svtkm_cont_serial_testing
6.0M    UnitTests_svtkm_cont_tbb_testing
7.1M    UnitTests_svtkm_cont_testing
```

After the changes, the executable sizes are as follows.

```
3.0M    libsvtkm_cont-1.2.1.dylib
6.0M    libsvtkm_rendering-1.2.1.dylib
312K    Rendering_SERIAL
312K    Rendering_TBB
 21M    Worklets_SERIAL
 21M    Worklets_TBB
 22M    UnitTests_svtkm_filter_testing
5.6M    UnitTests_svtkm_cont_serial_testing
6.0M    UnitTests_svtkm_cont_tbb_testing
7.1M    UnitTests_svtkm_cont_testing
```

As we can see, the built sizes have not changed significantly. (If
anything, the build is a little smaller.)


## Worklets can now be specialized for a specific device adapter

This change adds an execution signature tag named `Device` that passes
a `DeviceAdapterTag` to the worklet's parenthesis operator. This allows the
worklet to specialize its operation. This features is available in all
worklets.

The following example shows a worklet that specializes itself for the CUDA
device.

```cpp
struct DeviceSpecificWorklet : svtkm::worklet::WorkletMapField
{
  using ControlSignature = void(FieldIn, FieldOut);
  using ExecutionSignature = _2(_1, Device);

  // Specialization for the Cuda device.
  template <typename T>
  T operator()(T x, svtkm::cont::DeviceAdapterTagCuda) const
  {
    // Special cuda implementation
  }

  // General implementation
  template <typename T, typename Device>
  T operator()(T x, Device) const
  {
    // General implementation
  }
};
```

### Effect on compile time and binary size

This change necessitated adding a template parameter for the device that
followed at least from the schedule all the way down. This has the
potential for duplicating several of the support methods (like
`DoWorkletInvokeFunctor`) that would otherwise have the same type. This is
especially true between the devices that run on the CPU as they should all
be sharing the same portals from `ArrayHandle`s. So the question is whether
it causes compile to take longer or cause a significant increase in
binaries.

To informally test, I first ran a clean debug compile on my Windows machine
with the serial and tbb devices. The build itself took **3 minutes, 50
seconds**. Here is a list of the binary sizes in the bin directory:

```
kmorel2 0> du -sh *.exe *.dll
200K    BenchmarkArrayTransfer_SERIAL.exe
204K    BenchmarkArrayTransfer_TBB.exe
424K    BenchmarkAtomicArray_SERIAL.exe
424K    BenchmarkAtomicArray_TBB.exe
440K    BenchmarkCopySpeeds_SERIAL.exe
580K    BenchmarkCopySpeeds_TBB.exe
4.1M    BenchmarkDeviceAdapter_SERIAL.exe
5.3M    BenchmarkDeviceAdapter_TBB.exe
7.9M    BenchmarkFieldAlgorithms_SERIAL.exe
7.9M    BenchmarkFieldAlgorithms_TBB.exe
22M     BenchmarkFilters_SERIAL.exe
22M     BenchmarkFilters_TBB.exe
276K    BenchmarkRayTracing_SERIAL.exe
276K    BenchmarkRayTracing_TBB.exe
4.4M    BenchmarkTopologyAlgorithms_SERIAL.exe
4.4M    BenchmarkTopologyAlgorithms_TBB.exe
712K    Rendering_SERIAL.exe
712K    Rendering_TBB.exe
708K    UnitTests_svtkm_cont_arg_testing.exe
1.7M    UnitTests_svtkm_cont_internal_testing.exe
13M     UnitTests_svtkm_cont_serial_testing.exe
14M     UnitTests_svtkm_cont_tbb_testing.exe
18M     UnitTests_svtkm_cont_testing.exe
13M     UnitTests_svtkm_cont_testing_mpi.exe
736K    UnitTests_svtkm_exec_arg_testing.exe
136K    UnitTests_svtkm_exec_internal_testing.exe
196K    UnitTests_svtkm_exec_serial_internal_testing.exe
196K    UnitTests_svtkm_exec_tbb_internal_testing.exe
2.0M    UnitTests_svtkm_exec_testing.exe
83M     UnitTests_svtkm_filter_testing.exe
476K    UnitTests_svtkm_internal_testing.exe
148K    UnitTests_svtkm_interop_internal_testing.exe
1.3M    UnitTests_svtkm_interop_testing.exe
2.9M    UnitTests_svtkm_io_reader_testing.exe
548K    UnitTests_svtkm_io_writer_testing.exe
792K    UnitTests_svtkm_rendering_testing.exe
3.7M    UnitTests_svtkm_testing.exe
320K    UnitTests_svtkm_worklet_internal_testing.exe
65M     UnitTests_svtkm_worklet_testing.exe
11M     svtkm_cont-1.3.dll
2.1M    svtkm_interop-1.3.dll
21M     svtkm_rendering-1.3.dll
3.9M    svtkm_worklet-1.3.dll
```

After making the singular change to the `Invocation` object to add the
`DeviceAdapterTag` as a template parameter (which should cause any extra
compile instances) the compile took **4 minuts and 5 seconds**. Here is the
new list of binaries.

```
kmorel2 0> du -sh *.exe *.dll
200K    BenchmarkArrayTransfer_SERIAL.exe
204K    BenchmarkArrayTransfer_TBB.exe
424K    BenchmarkAtomicArray_SERIAL.exe
424K    BenchmarkAtomicArray_TBB.exe
440K    BenchmarkCopySpeeds_SERIAL.exe
580K    BenchmarkCopySpeeds_TBB.exe
4.1M    BenchmarkDeviceAdapter_SERIAL.exe
5.3M    BenchmarkDeviceAdapter_TBB.exe
7.9M    BenchmarkFieldAlgorithms_SERIAL.exe
7.9M    BenchmarkFieldAlgorithms_TBB.exe
22M     BenchmarkFilters_SERIAL.exe
22M     BenchmarkFilters_TBB.exe
276K    BenchmarkRayTracing_SERIAL.exe
276K    BenchmarkRayTracing_TBB.exe
4.4M    BenchmarkTopologyAlgorithms_SERIAL.exe
4.4M    BenchmarkTopologyAlgorithms_TBB.exe
712K    Rendering_SERIAL.exe
712K    Rendering_TBB.exe
708K    UnitTests_svtkm_cont_arg_testing.exe
1.7M    UnitTests_svtkm_cont_internal_testing.exe
13M     UnitTests_svtkm_cont_serial_testing.exe
14M     UnitTests_svtkm_cont_tbb_testing.exe
19M     UnitTests_svtkm_cont_testing.exe
13M     UnitTests_svtkm_cont_testing_mpi.exe
736K    UnitTests_svtkm_exec_arg_testing.exe
136K    UnitTests_svtkm_exec_internal_testing.exe
196K    UnitTests_svtkm_exec_serial_internal_testing.exe
196K    UnitTests_svtkm_exec_tbb_internal_testing.exe
2.0M    UnitTests_svtkm_exec_testing.exe
86M     UnitTests_svtkm_filter_testing.exe
476K    UnitTests_svtkm_internal_testing.exe
148K    UnitTests_svtkm_interop_internal_testing.exe
1.3M    UnitTests_svtkm_interop_testing.exe
2.9M    UnitTests_svtkm_io_reader_testing.exe
548K    UnitTests_svtkm_io_writer_testing.exe
792K    UnitTests_svtkm_rendering_testing.exe
3.7M    UnitTests_svtkm_testing.exe
320K    UnitTests_svtkm_worklet_internal_testing.exe
68M     UnitTests_svtkm_worklet_testing.exe
11M     svtkm_cont-1.3.dll
2.1M    svtkm_interop-1.3.dll
21M     svtkm_rendering-1.3.dll
3.9M    svtkm_worklet-1.3.dll
```

So far the increase is quite negligible.

## Worklets now support an execution mask

There have recently been use cases where it would be helpful to mask out
some of the invocations of a worklet. The idea is that when invoking a
worklet with a mask array on the input domain, you might implement your
worklet more-or-less like the following.

```cpp
SVTKM_EXEC void operator()(bool mask, /* other parameters */)
{
  if (mask)
  {
    // Do interesting stuff
  }
}
```

This works, but what if your mask has mostly false values? In that case,
you are spending tons of time loading data to and from memory where fields
are stored for no reason.

You could potentially get around this problem by adding a scatter to the
worklet. However, that will compress the output arrays to only values that
are active in the mask. That is problematic if you want the masked output
in the appropriate place in the original arrays. You will have to do some
complex (and annoying and possibly expensive) permutations of the output
arrays.

Thus, we would like a new feature similar to scatter that instead masks out
invocations so that the worklet is simply not run on those outputs.

### New Interface

The new "Mask" feature that is similar (and orthogonal) to the existing
"Scatter" feature. Worklet objects now define a `MaskType` that provides on
object that manages the selections of which invocations are skipped. The
following Mask objects are defined.

  * `MaskNone` - This removes any mask of the output. All outputs are
    generated. This is the default if no `MaskType` is explicitly defined.
  * `MaskSelect` - The user to provides an array that specifies whether
    each output is created with a 1 to mean that the output should be
    created an 0 the mean that it should not.
  * `MaskIndices` - The user provides an array with a list of indices for
    all outputs that should be created.

It will be straightforward to implement other versions of masks. (For
example, you could make a mask class that selectes every Nth entry.) Those
could be made on an as-needed basis.

### Implementation

The implementation follows the same basic idea of how scatters are
implemented.

#### Mask Classes

The mask class is required to implement the following items.

  * `ThreadToOutputType` - A type for an array that maps a thread index (an
    index in the array) to an output index. A reasonable type for this
    could be `svtkm::cont::ArrayHandle<svtkm::Id>`.
  * `GetThreadToOutputMap` - Given the range for the output (e.g. the
    number of items in the output domain), returns an array of type
    `ThreadToOutputType` that is the actual map.
  * `GetThreadRange` - Given a range for the output (e.g. the number of
    items in the output domain), returns the range for the threads (e.g.
    the number of times the worklet will be invoked).

#### Dispatching

The `svtkm::worklet::internal::DispatcherBase` manages a mask class in
the same way it manages the scatter class. It gets the `MaskType` from
the worklet it is templated on. It requires a `MaskType` object during
its construction.

Previously the dispatcher (and downstream) had to manage the range and
indices of inputs and threads. They now have to also manage a separate
output range/index as now all three may be different.

The `svtkm::Invocation` is changed to hold the ThreadToOutputMap array from
the mask. It likewises has a templated `ChangeThreadToOutputMap` method
added (similar to those already existing for the arrays from a scatter).
This method is used in `DispatcherBase::InvokeTransportParameters` to add
the mask's array to the invocation before calling `InvokeSchedule`.

#### Thread Indices

With the addition of masks, the `ThreadIndices` classes are changed to
manage the actual output index. Previously, the output index was always the
same as the thread index. However, now these two can be different. The
`GetThreadIndices` methods of the worklet base classes have an argument
added that is the portal to the ThreadToOutputMap.

The worklet `GetThreadIndices` is called from the `Task` classes. These
classes are changed to pass in this additional argument. Since the `Task`
classes get an `Invocation` object from the dispatcher, which contains the
`ThreadToOutputMap`, this change is trivial.

### Interaction Between Mask and Scatter

Although it seems weird, it should work fine to mix scatters and masks. The
scatter will first be applied to the input to generate a (potential) list
of output elements. The mask will then be applied to these output elements.


## Redesign SVTK-m Runtime Device Tracking

The device tracking infrastructure in SVTK-m has been redesigned to
remove multiple redundant codes paths and to simplify reasoning
about around what an instance of RuntimeDeviceTracker will modify.


`svtkm::cont::RuntimeDeviceTracker` tracks runtime information on
a per-user thread basis. This is done to allow multiple calling
threads to use different svtk-m backends such as seen in this
example:

```cpp
  svtkm::cont::DeviceAdapterTagCuda cuda;
  svtkm::cont::DeviceAdapterTagOpenMP openmp;
  { // thread 1
    auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
    tracker->ForceDevice(cuda);
    svtkm::worklet::Invoker invoke;
    invoke(LightTask{}, input, output);
    svtkm::cont::Algorithm::Sort(output);
    invoke(HeavyTask{}, output);
  }

 { // thread 2
    auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
    tracker->ForceDevice(openmp);
    svtkm::worklet::Invoker invoke;
    invoke(LightTask{}, input, output);
    svtkm::cont::Algorithm::Sort(output);
    invoke(HeavyTask{}, output);
  }
```

Note: `GetGlobalRuntimeDeviceTracker` has ben refactored to be `GetRuntimeDeviceTracker`
as it always returned a unique instance for each control side thread. This design allows
for different threads to have different runtime device settings. By removing the term `Global`
from the name it becomes more clear what scope this class has.

While this address the ability for threads to specify what
device they should run on. It doesn't make it easy to toggle
the status of a device in a programmatic way, for example
the following block forces execution to only occur on
`cuda` and doesn't restore previous active devices after

```cpp
  {
  svtkm::cont::DeviceAdapterTagCuda cuda;
  auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
  tracker->ForceDevice(cuda);
  svtkm::worklet::Invoker invoke;
  invoke(LightTask{}, input, output);
  }
  //openmp/tbb/... still inactive
```

To resolve those issues we have `svtkm::cont::ScopedRuntimeDeviceTracker` which
has the same interface as `svtkm::cont::RuntimeDeviceTracker` but additionally
resets any per-user thread modifications when it goes out of scope. So by
switching over the previous example to use `ScopedRuntimeDeviceTracker` we
correctly restore the threads `RuntimeDeviceTracker` state when `tracker`
goes out of scope.
```cpp
  {
  svtkm::cont::DeviceAdapterTagCuda cuda;
  svtkm::cont::ScopedRuntimeDeviceTracker tracker(cuda);
  svtkm::worklet::Invoker invoke;
  invoke(LightTask{}, input, output);
  }
  //openmp/tbb/... are now again active
```

The  `svtkm::cont::ScopedRuntimeDeviceTracker` is not limited to forcing
execution to occur on a single device. When constructed it can either force
execution to a device, disable a device or enable a device. These options
also work with the `DeviceAdapterTagAny`.


```cpp
  {
  //enable all devices
  svtkm::cont::DeviceAdapterTagAny any;
  svtkm::cont::ScopedRuntimeDeviceTracker tracker(any,
                                                 svtkm::cont::RuntimeDeviceTrackerMode::Enable);
  ...
  }

  {
  //disable only cuda
  svtkm::cont::DeviceAdapterTagCuda cuda;
  svtkm::cont::ScopedRuntimeDeviceTracker tracker(cuda,
                                                 svtkm::cont::RuntimeDeviceTrackerMode::Disable);

  ...
  }
```


## `svtkm::cont::Initialize` added to make setting up SVTK-m runtime state easier

A new initialization function, `svtkm::cont::Initialize`, has been added.
Initialization is not required, but will configure the logging utilities (when
enabled) and allows forcing a device via a `-d` or `--device` command line
option.


Usage:

```cpp
#include <svtkm/cont/Initialize.h>

int main(int argc, char *argv[])
{
  auto config = svtkm::cont::Initialize(argc, argv);

  ...
}
```


# ArrayHandle

## Add `svtkm::cont::ArrayHandleVirtual`

Added a new class named `svtkm::cont::ArrayHandleVirtual` that allows you to type erase an
ArrayHandle storage type by using virtual calls. This simplification makes
storing `Fields` and `Coordinates` significantly easier as SVTK-m doesn't
need to deduce both the storage and value type when executing worklets.

To construct an `svtkm::cont::ArrayHandleVirtual` one can do the following:

```cpp
svtkm::cont::ArrayHandle<svtkm::Float32> pressure;
svtkm::cont::ArrayHandleConstant<svtkm::Float32> constant(42.0f);


// constrcut from an array handle
svtkm::cont::ArrayHandleVirtual<svtkm::Float32> v(pressure);

// or assign from an array handle
v = constant;

```

To help maintain performance `svtkm::cont::ArrayHandleVirtual` provides a collection of helper
functions/methods to query and cast back to the concrete storage and value type:
```cpp
svtkm::cont::ArrayHandleConstant<svtkm::Float32> constant(42.0f);
svtkm::cont::ArrayHandleVirtual<svtkm::Float32> v = constant;

const bool isConstant = svtkm::cont::IsType< decltype(constant) >(v);
if(isConstant)
  svtkm::cont::ArrayHandleConstant<svtkm::Float32> t = svtkm::cont::Cast< decltype(constant) >(v);

```

Lastly, a common operation of calling code using `ArrayHandleVirtual` is a desire to construct a new instance
of an existing virtual handle with the same storage type. This can be done by using the `NewInstance` method
as seen below
```cpp
svtkm::cont::ArrayHandle<svtkm::Float32> pressure;
svtkm::cont::ArrayHandleVirtual<svtkm::Float32> v = pressure;

svtkm::cont::ArrayHandleVirtual<svtkm::Float32> newArray = v->NewInstance();
bool isConstant = svtkm::cont::IsType< svtkm::cont::ArrayHandle<svtkm::Float32> >(newArray); //will be true
```


## `svtkm::cont::ArrayHandleZip` provides a consistent API even with non-writable handles

Previously `svtkm::cont::ArrayHandleZip` could not wrap an implicit handle and provide a consistent experience.
The primary issue was that if you tried to use the PortalType returned by `GetPortalControl()` you
would get a compile failure. This would occur as the PortalType returned would try to call `Set`
on an ImplicitPortal which doesn't have a set method.

Now with this change, the `ZipPortal` use SFINAE to determine if `Set` and `Get` should call the
underlying zipped portals.


## `svtkm::cont::VariantArrayHandle` replaces `svtkm::cont::DynamicArrayHandle`

`svtkm::cont::ArrayHandleVariant` replaces `svtkm::cont::DynamicArrayHandle` as the
primary method for holding onto a type erased `svtkm::cont::ArrayHandle`. The major
difference between the two implementations is how they handle the Storage component of
an array handle.

`svtkm::contDynamicArrayHandle` approach was to find the fully deduced type of the `ArrayHandle`
meaning it would check all value and storage types it knew about until it found a match.
This cross product of values and storages would cause significant compilation times when
a `DynamicArrayHandle` had multiple storage types.

`svtkm::cont::VariantArrayHandle` approach is to only deduce the value type of the `ArrayHandle`
and return a `svtkm::cont::ArrayHandleVirtual` which uses polymorpishm to hide the actual
storage type. This approach allows for better compile times, and for calling code
to always expect an `ArrayHandleVirtual` instead of the fully deduced type. This conversion
to `ArrayHandleVirtual` is usually done internally within SVTK-m when a  worklet or filter
is invoked.

In certain cases users of `VariantArrayHandle` want to be able to access the concrete
`ArrayHandle<T,S>` and not have it wrapped in a `ArrayHandleVirtual`. For those occurrences
`VariantArrayHandle` provides a collection of helper functions/methods to query and
cast back to the concrete storage and value type:

```cpp
svtkm::cont::ArrayHandleConstant<svtkm::Float32> constant(42.0f);
svtkm::cont::ArrayHandleVariant v(constant);

const bool isConstant = svtkm::cont::IsType< decltype(constant) >(v);
if(isConstant)
  svtkm::cont::ArrayHandleConstant<svtkm::Float32> t = svtkm::cont::Cast< decltype(constant) >(v);

```

Lastly, a common operation of calling code using `VariantArrayHandle` is a desire to construct a new instance
of an existing virtual handle with the same storage type. This can be done by using the `NewInstance` method
as seen below:

```cpp
svtkm::cont::ArrayHandle<svtkm::Float32> pressure;
svtkm::cont::ArrayHandleVariant v(pressure);

svtkm::cont::ArrayHandleVariant newArray = v->NewInstance();
const bool isConstant = svtkm::cont::IsType< decltype(pressure) >(newArray); //will be true
```


## `svtkm::cont::VariantArrayHandle` CastAndCall supports casting to concrete types

Previously, the `VariantArrayHandle::CastAndCall` (and indirect calls through
`svtkm::cont::CastAndCall`) attempted to cast to only
`svtkm::cont::ArrayHandleVirtual` with different value types. That worked, but
it meant that whatever was called had to operate through virtual functions.

Under most circumstances, it is worthwhile to also check for some common
storage types that, when encountered, can be accessed much faster. This
change provides the casting to concrete storage types and now uses
`svtkm::cont::ArrayHandleVirtual` as a fallback when no concrete storage
type is found.

By default, `CastAndCall` checks all the storage types in
`SVTKM_DEFAULT_STORAGE_LIST_TAG`, which typically contains only the basic
storage. The `ArrayHandleVirtual::CastAndCall` method also allows you to
override this behavior by specifying a different type list in the first
argument. If the first argument is a list type, `CastAndCall` assumes that
all the types in the list are storage tags. If you pass in
`svtkm::ListTagEmpty`, then `CastAndCall` will always cast to an
`ArrayHandleVirtual` (the previous behavior). Alternately, you can pass in
storage tags that might be likely under the current usage.

As an example, consider the following simple code.

``` cpp
svtkm::cont::VariantArrayHandle array;

// stuff happens

array.CastAndCall(myFunctor);
```

Previously, `myFunctor` would be called with
`svtkm::cont::ArrayHandleVirtual<T>` with different type `T`s. After this
change, `myFunctor` will be called with that and with
`svtkm::cont::ArrayHandle<T>` of the same type `T`s.

If you want to only call `myFunctor` with
`svtkm::cont::ArrayHandleVirtual<T>`, then replace the previous line with

``` cpp
array.CastAndCall(svtkm::ListTagEmpty(), myFunctor);
```

Let's say that additionally using `svtkm::cont::ArrayHandleIndex` was also
common. If you want to also specialize for that array, you can do so with
the following line.

``` cpp
array.CastAndCall(svtkm::ListTagBase<svtkm::cont::StorageBasic,
                                    svtkm::cont::ArrayHandleIndex::StorageTag>,
                  myFunctor);
```

Note that `myFunctor` will be called with
`svtkm::cont::ArrayHandle<T,svtkm::cont::ArrayHandleIndex::StorageTag>`, not
`svtkm::cont::ArrayHandleIndex`.


## `svtkm::cont::VariantArrayHandle::AsVirtual<T>()` performs casting

The `AsVirtual<T>` method of `svtkm::cont::VariantArrayHandle` now works for
any arithmetic type, not just the actual type of the underlying array. This
works by inserting an `ArrayHandleCast` between the underlying concrete array
and the new `ArrayHandleVirtual` when needed.


## `StorageBasic::StealArray()` now provides delete function to new owner

Memory that is stolen from SVTK-m has to be freed correctly. This is required
as the memory could have been allocated with `new`, `malloc` or even `cudaMallocManaged`.

Previously it was very easy to transfer ownership of memory out of SVTK-m and
either fail to capture the free function, or ask for it after the transfer
operation which would return a nullptr. Now stealing an array also
provides the free function reducing one source of memory leaks.

To properly steal memory from SVTK-m you do the following:
```cpp
  svtkm::cont::ArrayHandle<T> arrayHandle;

  ...

  auto* stolen = arrayHandle.StealArray();
  T* ptr = stolen.first;
  auto free_function = stolen.second;

  ...

  free_function(ptr);
```


# Control Environment

## `svtkm::cont::CellLocatorGeneral` has been added

`svtkm::cont::CellLocatorUniformBins` can work with all kinds of datasets, but there are cell
locators that are more efficient for specific data sets. Therefore, a new cell
locator - `svtkm::cont::CellLocatorGeneral` has been implemented that can be configured to use
specialized cell locators based on its input data. A "configurator" function object
can be specified using the `SetConfigurator()` function. The configurator should
have the following signature:

```cpp
void (std::unique_ptr<svtkm::cont::CellLocator>&,
     const svtkm::cont::DynamicCellSet&,
     const svtkm::cont::CoordinateSystem&);
```

The configurator is invoked whenever the `Update` method is called and the input
has changed. The current cell locator is passed in a `std::unique_ptr`. Based on
the types of the input cellset and coordinates, and possibly some heuristics on
their values, the current cell locator's parameters can be updated, or a different
cell-locator can be instantiated and transferred to the `unique_ptr`. The default
configurator configures a `svtkm::cont::CellLocatorUniformGrid` for uniform grid datasets,
a `svtkm::cont::CellLocatorRecitlinearGrid` for rectilinear datasets, and
`svtkm::cont::CellLocatorUniformBins` for all other dataset types.

The class `CellLocatorHelper` that implemented similar functionality to `CellLocatorGeneral`
 has been removed.

## `svtkm::cont::CellLocatorTwoLevelUniformGrid` has been renamed to `svtkm::cont::CellLocatorUniformBins`

`CellLocatorTwoLevelUniformGrid` has been renamed to `CellLocatorUniformBins`
for brevity. It has been modified to be a subclass of `svtkm::cont::CellLocator`
and can be used wherever a `CellLocator` is accepted.

## `svtkm::cont::Timer` now supports  asynchronous and device independent timers

`svtkm::cont::Timer` can now track execution time on a single device or across all
enabled devices as seen below:

```cpp
svtkm::cont::Timer tbb_timer{svtkm::cont::DeviceAdaptertagTBB()};
svtkm::cont::Timer all_timer;

all_timer.Start();
tbb_timer.Start();
// Run blocking algorithm on tbb
tbb_timer.Stop();
// Run async-algorithms cuda
all_timer.Stop();

// Do more work

//Now get time for all tbb work, and tbb_cuda work
auto tbb_time = tbb_timer.GetElapsedTime();
auto all_time = tbb_timer.GetElapsedTime();
```

When `Timer` is constructed without an explicit `svtkm::cont::DeviceAdapterId` it
will track all device adapters and return the maximum elapsed time over all devices
when `GetElapsedTime` is called.


## `svtkm::cont::DeviceAdapterId` construction from strings are now case-insensitive

You can now construct a `svtkm::cont::DeviceAdapterId` from a string no matter
the case of it. The following all will construct the same `svtkm::cont::DeviceAdapterId`.

```cpp
svtkm::cont::DeviceAdapterId id1 = svtkm::cont::make_DeviceAdapterId("cuda");
svtkm::cont::DeviceAdapterId id2 = svtkm::cont::make_DeviceAdapterId("CUDA");
svtkm::cont::DeviceAdapterId id3 = svtkm::cont::make_DeviceAdapterId("Cuda");

auto& tracker = svtkm::cont::GetRuntimeDeviceTracker();
svtkm::cont::DeviceAdapterId id4 = tracker.GetDeviceAdapterId("cuda");
svtkm::cont::DeviceAdapterId id5 = tracker.GetDeviceAdapterId("CUDA");
svtkm::cont::DeviceAdapterId id6 = tracker.GetDeviceAdapterId("Cuda");
```

## `svtkm::cont::Initialize` will only parse known arguments

When a library requires reading some command line arguments through a
function like Initialize, it is typical that it will parse through
arguments it supports and then remove those arguments from `argc` and
`argv` so that the remaining arguments can be parsed by the calling
program. Recent changes to the `svtkm::cont::Initialize` function support
that.

### Use Case

Say you are creating a simple benchmark where you want to provide a command
line option `--size` that allows you to adjust the size of the data that
you are working on. However, you also want to support flags like `--device`
and `-v` that are performed by `svtkm::cont::Initialize`. Rather than have
to re-implement all of `Initialize`'s parsing, you can now first call
`Initialize` to handle its arguments and then parse the remaining objects.

The following is a simple (and rather incomplete) example:

```cpp
int main(int argc, char** argv)
{
  svtkm::cont::InitializeResult initResult = svtkm::cont::Initialize(argc, argv);

  if ((argc > 1) && (strcmp(argv[1], "--size") == 0))
  {
    if (argc < 3)
	{
	  std::cerr << "--size option requires a numeric argument" << std::endl;
	  std::cerr << "USAGE: " << argv[0] << " [options]" << std::endl;
	  std::cerr << "Options are:" << std::endl;
	  std::cerr << "  --size <number>\tSpecify the size of the data." << std::endl;
	  std::cerr << initResult.Usage << std::endl;
	  exit(1);
	}

	g_size = atoi(argv[2]);
  }

  std::cout << "Using device: " << initResult.Device.GetName() << std::endl;
```

### Additional Initialize Options

Because `svtkm::cont::Initialize` no longer has the assumption that it is responsible
for parsing _all_ arguments, some options have been added to
`svtkm::cont::InitializeOptions` to manage these different use cases. The
following options are now supported.

  * `None` A placeholder for having all options off, which is the default.
    (Same as before this change.)
  * `RequireDevice` Issue an error if the device argument is not specified.
    (Same as before this change.)
  * `DefaultAnyDevice` If no device is specified, treat it as if the user
    gave --device=Any. This means that DeviceAdapterTagUndefined will never
    be return in the result.
  * `AddHelp` Add a help argument. If `-h` or `--help` is provided, prints
    a usage statement. Of course, the usage statement will only print out
    arguments processed by SVTK-m.
  * `ErrorOnBadOption` If an unknown option is encountered, the program
    terminates with an error and a usage statement is printed. If this
    option is not provided, any unknown options are returned in `argv`. If
    this option is used, it is a good idea to use `AddHelp` as well.
  * `ErrorOnBadArgument` If an extra argument is encountered, the program
    terminates with an error and a usage statement is printed. If this
    option is not provided, any unknown arguments are returned in `argv`.
  * `Strict` If supplied, Initialize treats its own arguments as the only
    ones supported by the application and provides an error if not followed
    exactly. This is a convenience option that is a combination of
    `ErrorOnBadOption`, `ErrorOnBadArgument`, and `AddHelp`.

### InitializeResult Changes

The changes in `Initialize` have also necessitated the changing of some of
the fields in the `InitializeResult` structure. The following fields are
now provided in the `InitializeResult` struct.

  * `Device` Returns the device selected in the command line arguments as a
    `DeviceAdapterId`. If no device was selected,
    `DeviceAdapterTagUndefined` is returned. (Same as before this change.)
  * `Usage` Returns a string containing the usage for the options
    recognized by `Initialize`. This can be used to build larger usage
    statements containing options for both `Initialize` and the calling
    program. See the example above.

Note that the `Arguments` field has been removed from `InitializeResult`.
This is because the unparsed arguments are now returned in the modified
`argc` and `argv`, which provides a more complete result than the
`Arguments` field did.

# Execution Environment

## SVTK-m logs details about each CUDA kernel launch

The SVTK-m logging infrastructure has been extended with a new log level
`KernelLaunches` which exists between `MemTransfer` and `Cast`.

This log level reports the number of blocks, threads per block, and the
PTX version of each CUDA kernel launched.

This logging level was primarily introduced to help developers that are
tracking down issues that occur when SVTK-m components have been built with
different `sm_XX` flags and help people looking to do kernel performance
tuning.


## SVTK-m CUDA allocations can have managed memory (cudaMallocManaged) enabled/disabled from C++

Previously it was impossible for calling code to explicitly disable cuda managed memory. This can be desirable for projects that know they don't need managed memory and are super performance critical.

```cpp
const bool usingManagedMemory = svtkm::cont::cuda::internal::CudaAllocator::UsingManagedMemory();
if(usingManagedMemory)
  {  //disable managed memory
  svtkm::cont::cuda::internal::CudaAllocator::ForceManagedMemoryOff();
  }
```


## SVTK-m CUDA kernel scheduling improved including better defaults, and user customization support

SVTK-m now offers a more GPU aware set of defaults for kernel scheduling.
When SVTK-m first launches a kernel we do system introspection and determine
what GPU's are on the machine and than match this information to a preset
table of values. The implementation is designed in a way that allows for
SVTK-m to offer both specific presets for a given GPU ( V100 ) or for
an entire generation of cards ( Pascal ).

Currently SVTK-m offers preset tables for the following GPU's:
- Tesla V100
- Tesla P100

If the hardware doesn't match a specific GPU card we than try to find the
nearest know hardware generation and use those defaults. Currently we offer
defaults for
- Older than Pascal Hardware
- Pascal Hardware
- Volta+ Hardware

Some users have workloads that don't align with the defaults provided by
SVTK-m. When that is the cause, it is possible to override the defaults
by binding a custom function to `svtkm::cont::cuda::InitScheduleParameters`.
As shown below:

```cpp
  ScheduleParameters CustomScheduleValues(char const* name,
                                          int major,
                                          int minor,
                                          int multiProcessorCount,
                                          int maxThreadsPerMultiProcessor,
                                          int maxThreadsPerBlock)
  {

    ScheduleParameters params  {
        64 * multiProcessorCount,  //1d blocks
        64,                        //1d threads per block
        64 * multiProcessorCount,  //2d blocks
        { 8, 8, 1 },               //2d threads per block
        64 * multiProcessorCount,  //3d blocks
        { 4, 4, 4 } };             //3d threads per block
    return params;
  }
  svtkm::cont::cuda::InitScheduleParameters(&CustomScheduleValues);
```


## SVTK-m Reduction algorithm now supports differing input and output types

It is common to want to perform a reduction where the input and output types
are of differing types. A basic example would be when the input is `svtkm::UInt8`
but the output is `svtkm::UInt64`. This has been supported since v1.2, as the input
type can be implicitly convertible to the output type.

What we now support is when the input type is not implicitly convertible to the output type,
such as when the output type is `svtkm::Pair< svtkm::UInt64, svtkm::UInt64>`. For this to work
we require that the custom binary operator implements also an `operator()` which handles
the unary transformation of input to output.

An example of a custom reduction operator for differing input and output types is:

```cxx

  struct CustomMinAndMax
  {
    using OutputType = svtkm::Pair<svtkm::Float64, svtkm::Float64>;

    SVTKM_EXEC_CONT
    OutputType operator()(svtkm::Float64 a) const
    {
    return OutputType(a, a);
    }

    SVTKM_EXEC_CONT
    OutputType operator()(svtkm::Float64 a, svtkm::Float64 b) const
    {
      return OutputType(svtkm::Min(a, b), svtkm::Max(a, b));
    }

    SVTKM_EXEC_CONT
    OutputType operator()(const OutputType& a, const OutputType& b) const
    {
      return OutputType(svtkm::Min(a.first, b.first), svtkm::Max(a.second, b.second));
    }

    SVTKM_EXEC_CONT
    OutputType operator()(svtkm::Float64 a, const OutputType& b) const
    {
      return OutputType(svtkm::Min(a, b.first), svtkm::Max(a, b.second));
    }

    SVTKM_EXEC_CONT
    OutputType operator()(const OutputType& a, svtkm::Float64 b) const
    {
      return OutputType(svtkm::Min(a.first, b), svtkm::Max(a.second, b));
    }
  };


```

## Added specialized operators for ArrayPortalValueReference

The `ArrayPortalValueReference` is supposed to behave just like the value it
encapsulates and does so by automatically converting to the base type when
necessary. However, when it is possible to convert that to something else,
it is possible to get errors about ambiguous overloads. To avoid these, add
specialized versions of the operators to specify which ones should be used.

Also consolidated the CUDA version of an `ArrayPortalValueReference` to the
standard one. The two implementations were equivalent and we would like
changes to apply to both.


# Worklets and Filters

## `svtkm::worklet::Invoker` now supports worklets which require a Scatter object

This change allows the `Invoker` class to support launching worklets that require
a custom scatter operation. This is done by providing the scatter as the second
argument when launch a worklet with the `()` operator.

The following example shows a scatter being provided with a worklet launch.

```cpp
struct CheckTopology : svtkm::worklet::WorkletMapPointToCell
{
  using ControlSignature = void(CellSetIn cellset, FieldOutCell);
  using ExecutionSignature = _2(FromIndices);
  using ScatterType = svtkm::worklet::ScatterPermutation<>;
  ...
};


svtkm::worklet::Ivoker invoke;
invoke( CheckTopology{}, svtkm::worklet::ScatterPermutation{}, cellset, result );
```


## `BitFields` are now a support field input/out type for SVTK-m worklets

`BitFields` are:
  - Stored in memory using a contiguous buffer of bits.
  - Accessible via portals, a la ArrayHandle.
  - Portals operate on individual bits or words.
  - Operations may be atomic for safe use from concurrent kernels.

The new `BitFieldToUnorderedSet` device algorithm produces an
ArrayHandle containing the indices of all set bits, in no particular
order.

The new AtomicInterface classes provide an abstraction into bitwise
atomic operations across control and execution environments and are
used to implement the BitPortals.

BitFields may be used as boolean-typed ArrayHandles using the
ArrayHandleBitField adapter. `svtkm::cont::ArrayHandleBitField` uses atomic operations to read
and write bits in the BitField, and is safe to use in concurrent code.

For example, a simple worklet that merges two arrays based on a boolean
condition:

```cpp
class ConditionalMergeWorklet : public svtkm::worklet::WorkletMapField
{
public:
using ControlSignature = void(FieldIn cond,
                              FieldIn trueVals,
                              FieldIn falseVals,
                              FieldOut result);
using ExecutionSignature = _4(_1, _2, _3);

template <typename T>
SVTKM_EXEC T operator()(bool cond, const T& trueVal, const T& falseVal) const
{
  return cond ? trueVal : falseVal;
}

};

BitField bits = ...;
auto condArray = svtkm::cont::make_ArrayHandleBitField(bits);
auto trueArray = svtkm::cont::make_ArrayHandleCounting<svtkm::Id>(20, 2, NUM_BITS);
auto falseArray = svtkm::cont::make_ArrayHandleCounting<svtkm::Id>(13, 2, NUM_BITS);
svtkm::cont::ArrayHandle<svtkm::Id> output;

svtkm::worklet::Invoker invoke( svtkm::cont::DeviceAdaptertagTBB{} );
invoke(ConditionalMergeWorklet{}, condArray, trueArray, falseArray, output);

```


## Added a Point Merging worklet

We have added `svtkm::worklet::PointMerge` which uses a virtual grid approach to
identify nearby points. The worklet works by creating a very fine but
sparsely represented locator grid. It then groups points by grid bins and
finds those within a specified radius.


## `svtkm::filter::CleanGrid` now can do point merging

The `CleanGrid` filter has been extended to use `svtkm::worklet::PointMerge` to
allow for point merging. The following flags have been added to `CleanGrid` to
modify the behavior of point merging.

  - `Set`/`GetMergePoints` - a flag to turn on/off the merging of
    duplicated coincident points. This extra operation will find points
    spatially located near each other and merge them together.
  - `Set`/`GetTolerance` - Defines the tolerance used when determining
    whether two points are considered coincident. If the
    `ToleranceIsAbsolute` flag is false (the default), then this tolerance
    is scaled by the diagonal of the points. This parameter is only used
    when merge points is on.
  - `Set`/`GetToleranceIsAbsolute` - When ToleranceIsAbsolute is false (the
     default) then the tolerance is scaled by the diagonal of the bounds of
     the dataset. If true, then the tolerance is taken as the actual
     distance to use. This parameter is only used when merge points is on.
  - `Set`/`GetFastMerge` - When FastMerge is true (the default), some
     corners are cut when computing coincident points. The point merge will
     go faster but the tolerance will not be strictly followed.


## Added a connected component worklets and filters

We have added the `svtkm::filter::ImageConnectivity` and `svtkm::filter::CellSetConnectivity` filters
to identify connected components in DataSets and the corresponding worklets. The `ImageConnectivity`
identify connected components in `svtkm::cont::CellSetStructured`, based on same field value of neighboring
cells. The `CellSetConnectivit`y identify connected components based on cell connectivity.

Currently Moore neighborhood (i.e. 8 neighboring pixels for 2D and 27 neighboring pixels
for 3D) is used for `ImageConnectivity`. For `CellSetConnectivity`, neighborhood is defined
as cells sharing a common edge.


# Build

## CMake 3.8+ now required to build SVTK-m

Historically SVTK-m has offered the ability to build a small
subset of device adapters with CMake 3.3. As both our primary
consumers have moved to CMake 3.8, and HPC machines continue
to provide new CMake versions we have decided to simplify
our CMake build system by requiring CMake 3.8 everywhere.


## SVTK-m now can verify that it installs itself correctly

It was a fairly common occurrence of SVTK-m to have a broken install
tree as it had no easy way to verify that all headers would be installed.

Now SVTK-m offers a testing infrastructure that creates a temporary installed
version and compile tests that build against the installed  SVTK-m version. Currently
we have tests that verify each header listed in SVTK-m is installed, users can
compile a custom `svtkm::filter` that uses diy, and users can call `svtkm::rendering`.

## SVTK-m now requires `CUDA` separable compilation to build

With the introduction of `svtkm::cont::ArrayHandleVirtual` and the related infrastructure, svtk-m now
requires that all CUDA code be compiled using separable compilation ( -rdc ).


## SVTK-m provides a `svtkm_filter` CMake target

SVTK-m now provides a `svtkm_filter` target that contains pre-built components
of filters for consuming projects.


## `svtkm::cont::CellLocatorBoundingIntervalHierarchy` is compiled into `svtkm_cont`

All of the methods in CellLocatorBoundingIntervalHierarchy were listed in
header files. This is sometimes problematic with virtual methods. Since
everything implemented in it can just be embedded in a library, move the
code into the svtkm_cont library.

These changes caused some warnings in clang to show up based on virtual
methods in other cell locators. Hence, the rest of the cell locators
have also had some of their code moved to svtkm_cont.


# Other

## LodePNG added as a thirdparty package

The lodepng library was brought is an thirdparty library.
This has allowed the SVTK-m rendering library to have a robust
png decode functionality.


## Optionparser added as a thirdparty package

Previously we just took the optionparser.h file and stuck it right in
our source code. That was problematic for a variety of reasons.

  - It incorrectly assigned our license to external code.
  - It made lots of unnecessary changes to the original source (like reformatting).
  - It made it near impossible to track patches we make and updates to the original software.

Now we use the third-party system to track changes to optionparser.h
in the https://gitlab.kitware.com/third-party/optionparser repository.


## Thirdparty diy now can coexist with external diy

Previously SVTK-m would leak macros that would cause an external diy
to be incorrectly mangled breaking consumers of SVTK-m that used diy.

Going forward to use `diy` from SVTK-m all calls must use the `svtkmdiy`
namespace instead of the `diy` namespace. This allows for SVTK-m to
properly forward calls to either the external or internal version correctly.


## Merge benchmark executables into a device dependent shared library

SVTK-m has been updated to replace old per device benchmark executables with a single
multi-device executable. Selection of the device adapter is done at runtime through
the `--device=` argument.


## Merge rendering testing executables to a shared library

SVTK-m has been updated to replace old per device rendering testing executables with a single
multi-device executable. Selection of the device adapter is done at runtime through
the `--device=` argument.


## Merge worklet testing executables into a device dependent shared library

SVTK-m has been updated to replace old per device working testing executables with a single
multi-device executable. Selection of the device adapter is done at runtime through
the `--device=` argument.

## SVTK-m runtime device detection properly handles busy CUDA devices

When an application that uses SVTK-m is first launched it will
do a check to see if CUDA is supported at runtime. If for
some reason that CUDA card is not allowing kernel execution
SVTK-m would report the hardware doesn't have CUDA support.

This was problematic as was over aggressive in disabling CUDA
support for hardware that could support kernel execution in
the future. With the fact that every SVTK-m worklet is executed
through a TryExecute it is no longer necessary to be so
aggressive in disabling CUDA support.

Now the behavior is that SVTK-m considers a machine to have
CUDA runtime support if it has 1+ GPU's of Kepler or
higher hardware (SM_30+).
