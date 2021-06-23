Notes on the Python Wrappers for SVTK
====================================

First version by David Gobbi: Dec 19, 2002

Last update was on Feb 12, 2016

Abstract:
=========

This document provides a detailed description of how SVTK objects are
accessed and modified through Python.


Overview:
=========

Nearly all the power of SVTK objects are available through Python
    (with a few exceptions as noted below).  The C++ semantics are
translated as directly as possible to Python semantics.  Currently,
full support is provided for Python 2.6, 2.7, and 3.2 through 3.5.


The Basics:
===========

If SVTK has been properly built and installed with the python wrappers,
then SVTK can be accessed by importing the 'svtk' module:

    import svtk

or

    from svtk import *

For the most part, using SVTK from Python is very similar to using SVTK
from C++ except for changes in syntax, e.g.

    svtkObject *o = svtkObject::New()

becomes

    o = svtkObject()

and

    o->Method()

becomes

    o.Method()

Some other important differences are that you can pass a Python tuple,
list or array to a method that requires a C++ array e.g.:

    >>> a = svtkActor()
    >>> p = (100.0, 200.0, 100.0)
    >>> a.SetPosition(p)

If the C++ array parameter is used to return information, you must
pass a Python list or array that has the correct number of slots to
accept the returned values:

    >>> z = [0.0, 0.0, 0.0]
    >>> svtkMath.Cross((1,0,0),(0,1,0),z)
    >>> print z
    [0.0, 0.0, 1.0]

For multi-dimensional arrays, it is your choice whether to use
a nested list, or whether to use a numpy array with the correct
size and number of dimensions.

If the C++ method returns a pointer to an array, then that method is
only wrapped if there is a hint (usually in SVTK/Wrapping/hints)
that gives the size of the array.  Hinted pointers are returned as
tuples:

    >>> a = svtkActor()
    >>> print a.GetPosition()
    (0.0, 0.0, 0.0)

Finally, the python 'None' is treated the same as C++ 'NULL':

    >>> a = svtkActor()
    >>> a.SetMapper(None)
    >>> print a.GetMapper()
    None

And perhaps one of the most pleasant features of Python is that all
type-checking is performed at run time, so the type casts that are
often necessary in SVTK-C++ are never needed in SVTK-Python.


Strings (Python 3 vs. Python 2)
---------------------

The mapping of Python string types to C++ string types is different
in Python 3 as compared to Python 2.  In Python 2, the basic 'str()'
type was for 8-bit strings, and a separate 'unicode()' type was used
for unicode strings.

For Python 3, the basic 'str()' type is unicode, and when you pass a
str() as a SVTK 'std::string' or 'const char \*' parameter, SVTK will
receive the string in utf-8 encoding.  To use a different encoding,
you must encode the string yourself and pass it to SVTK as a bytes()
object, e.g. writer.SetFileName('myfile'.encode('latin1')).

When SVTK returns a 'const char \*' or 'std::string' in Python 3, the
wrappers will test to see of the string is utf-8 (or ascii), and if
so they will pass the string to python as a unicode str() object.
If the SVTK string is not valid utf-8, then the wrappers will pass
the string to python as a bytes() object.

This means that using encodings other than utf-8 with SVTK is risky.
In rare cases, these other encodings might produce 8-bit data that
the wrappers will detect as utf-8, causing them to produce a string
with incorrect characters.


STL Containers
-----------

SVTK provides conversion between 'std::vector' and Python sequences
such as 'tuple' and 'list'.  If the C++ method returns a vector,
the Python method will return a tuple:

    C++: const std::vector<std::string>& GetPaths()
    C++: std::vector<std::string> GetPaths()
    Python: GetPaths() -> Tuple[str]

If the C++ method accepts a vector, then the Python method can be
passed any sequence with compatible values:

    C++: void SetPaths(const std::vector<std::string>& paths)
    C++: void SetPaths(std::vector<std::string> paths)
    Python: SetPaths(paths: Sequence[str]) -> None

Furthermore, if the C++ method accepts a non-const vector reference,
then the Python method can be passed a mutable sequence (e.g. list):

    C++: void GetPaths(std::vector<std::string>& paths)
    Python: GetPaths(paths: MutableSequence[str]) -> None

The value type of the std::vector must be std::string or a
fundamental numeric type such as 'double' or 'int' (including
'signed char' and 'unsigned char' but excluding 'char').


Constants
---------

Most SVTK constants are available in Python, usually in the 'svtk'
module but sometimes as class attributes:

    >>> svtk.SVTK_DOUBLE_MAX
    1.0000000000000001e+299
    >>> svtk.svtkCommand.ErrorEvent
    39

Each named enum type is wrapped as a new Python type, and members
of the enum are instances of that type.  This allows type checking
for enum types:

    >>> # works if given a constant of the correct enum type
    >>> o.SetIntegrationMode(o.Continuous)
    >>> # does not work, because 'int' is not of the correct type
    >>> o.SetIntegrationMode(10)
    TypeError: SetIntegrationMode arg 1: expected enum EnumType, got int

Note that members of anonymous enums do not have a special type, and
are simply wrapped as python ints.


Namespaces
----------

Namespaces are currently wrapped in a very limited manner: the only
namespace members that are wrapped are constants and enum types.
There is no wrapping of namespaced classes or functions, or of nested
namespaces.  This is likely to be expanded upon when (or if) SVTK begins
to make greater use of namespaces.


Unavailable methods
-------------------

A method is not wrapped if:

1. its parameter list contains a pointer to anything other than
   a svtkObjectBase-derived object or a fundamental C++ type (void,
   char, int, unsigned short, double, etc.)
2. it returns a pointer to anything other than a svtkObjectBase-derived
   object, unless the method returns a pointer to a fundamental C++
   type and has an entry in the wrapping hints file, or the method is
   a svtkDataArray Get() method, or the returned type is 'char \*' or 'void \*'
3. it is an operator method (though many exceptions exist)


Unavailable classes
-------------------

Some classes are meant to be used only by other SVTK classes and are
not wrapped.  These are labelled as WRAP\_EXCLUDE\_PYTHON in the
CMakeLists.txt files.


Printing SVTK objects
====================

Printing a svtk object will provide the same information as provided
by the svtkObject::Print() method (i.e. the values of all instance variables)
The same information is provided by calling str(obj).  A more compact
representation of the object is available by calling repr(obj)

    repr(obj)  ->  '(svtkFloatArray)0x100c8d48'


Built-in documentation
======================

All of the documentation that is available in the SVTK header files and
in the html man pages is also available through python.

If you want to see what methods are available for a class, use e.g.

    >>> dir(svtk.svtkActor)

or, equivalently,

    >>> a = svtk.svtkActor()
    >>> dir(a)


You can also retrieve documentation about SVTK classes and methods
from the built-in 'docstrings':

    >>> help(svtk.svtkActor)
    >>> help(svtk.svtkActor.SetUserTransform)
    [ lots of info printed, try it yourself ]

For the method documentation, all the different 'signatures' for the
method are given in Python format and the original C++ format:

    >>> help(svtkActor.SetPosition)
    SetPosition(...)
    V.SetPosition(float, float, float)
    C++: virtual void SetPosition(double _arg1, double _arg2, double _arg3)
    V.SetPosition([float, float, float])
    C++: virtual void SetPosition(double _arg[3])

    Set/Get/Add the position of the Prop3D in world coordinates.


Peculiarities and special features
==================================

Deleting a svtkObject
---------------------

There is no direct equivalent of SVTK's Delete() since Python provides a
mechanism for automatic garbage collection.  The object will be
deleted once there are no remaining references to it either from
inside Python or from other SVTK objects.  It is possible to get rid of
the local reference to the object by using the python 'del' command,
i.e. 'del o', and this will result in a call to Delete() if the
local reference to the object was the last remaining reference to the
object from within Python.


Templated classes
-----------------

Templated classes are rare in SVTK.  Where they occur, they can be
instantiated much like they can be in C++, except that \[ \] brackets
are used for the template arguments instead of < > brackets:

    >>> v = svtkVector['float64',3]([1.0, 2.0, 3.0])
    >>> a = svtkDenseArray[str]()

Only a limited range of template args can be used, usually dictated by
by which args are used by typedefs and by other classes in the C++ code.
A list of the allowed argument combinations available for a particular
template can be found by calling help() on the template:

    >>> help(svtkVector)

The types are usually given as strings, in the form 'int32', 'uint16',
'float32', 'float64', 'bool', 'char', 'str', 'svtkVariant'.
Python type objects are acceptable, too, if the name of the type is
the same as one of the accepted type strings:

    >>> a = svtkDenseArray[int]()

Note that python 'int' is the same size as a C++ 'long', and python
'float' is the same size as C++ 'double'.  For compatibility with the
python array module, single-character typecodes are allowed, taken from
this list: '?', 'c', 'b', 'B', 'h', 'H', 'i', 'I', 'l', 'L', 'q', 'Q',
'f', 'd'.  The python array documentation explains what these mean.


Operator methods
----------------

Some useful operators are wrapped in python: the \[ \] operator is
wrapped for indexing and item assignment, but because it relies on
hints to guess which indices are out-of-bounds, it is only wrapped
for svtkVector and a few other classes.

The comparison operators '<' '<=' '==' '>=' '>' are wrapped for all
classes that have these operators in C++.

The '<<' operator for printing is wrapped and is used by the python
'print()' and 'str()' commands.


Pass-by-reference
-----------------

Pass-by-reference of values that are mutable in C++ but not in
Python (such as string, int, and float) is only possible by
using svtk.reference(), which is in the svtkCommonCore Python module:

    >>> plane = svtk.svtkPlane()
    >>> t = svtk.reference(0.0)
    >>> x = [0.0, 0.0, 0.0]
    >>> plane.InsersectWithLine([0, 0, -1], [0, 0, 1], t, x)
    >>> print t
    0.5


Observer, Event and CallData
----------------------------

*Simple callback*

Similarly to what can be done in C++, a python function can be called
each time a SVTK event is invoked on a given object:

    >>> def onObjectModified(object, event):
    >>>     print('object: %s - event: %s' % (object.GetClassName(), event))
    >>>
    >>> o = svtkObject()
    >>> o.AddObserver(svtkCommand.ModifiedEvent, onObjectModified)
    1
    >>> o.Modified()
    object: svtkObject - event: ModifiedEvent


*Callback with CallData*

In case there is a 'CallData' value associated with an event, in C++, you
have to cast it from void\* to the expected type using reinterpret\_cast.
For example, see http://www.svtk.org/Wiki/SVTK/Examples/Cxx/Interaction/CallData

The equivalent in python is to set a CallDataType attribute on the
associated python callback. The supported CallDataType are svtk.SVTK\_STRING,
svtk.SVTK\_OBJECT, svtk.SVTK\_INT, svtk.SVTK\_LONG, svtk.SVTK\_DOUBLE, svtk.SVTK\_FLOAT

For example:

    >>> def onError(object, event, calldata):
    >>>     print('object: %s - event: %s - msg: %s' % (object.GetClassName(),
                                                        event, calldata))
    >>>
    >>> onError.CallDataType = svtk.SVTK_INT
    >>>
    >>> lt = svtkLookupTable()
    >>> lt.AddObserver(svtkCommand.ErrorEvent, onError)
    1
    >>> lt.SetTableRange(2,1)
    object: svtkLookupTable - event: ErrorEvent - msg: ERROR:
    In /home/jchris/Projects/SVTK6/Common/Core/svtkLookupTable.cxx, line 122
    svtkLookupTable (0x6b40b30): Bad table range: [2, 1]


For convenience, the CallDataType can also be specified where the function
is first declared with the help of the 'calldata\_type' decorator:

    >>> @calldata_type(svtk.SVTK_INT)
    >>> def onError(object, event, calldata):
    >>>     print('object: %s - event: %s - msg: %s' % (object.GetClassName(),
                                                        event, calldata))


Subclassing a SVTK class
-----------------------

It is possible to subclass a SVTK class from within Python, but it
is NOT possible to properly override the virtual methods of the class.
The python-level code will be invisible to the SVTK C++ code, so when
the virtual method is called from C++, the method that you defined from
within Python will not be called.  The method will only be executed if
you call it from within Python.

It is therefore not reasonable, for instance, to subclass the
svtkInteractorStyle to provide custom python interaction.  Instead,
you have to do this by adding Observers to the svtkInteractor object.


Class methods
-------------

In C++, if you want to call a method from a superclass you can do the
following:

    svtkActor *a = svtkActor::New();
    a->svtkProp3D::SetPosition(10,20,50);

The equivalent in python is

    >>> a = svtkActor()
    >>> svtkProp3D.SetPosition(a,10,20,50)


Void pointers
-------------

As a special feature, a C++ method that requires a 'void \*' can
be passed any python object that supports the 'buffer' protocol,
which include string objects, numpy arrays and even SVTK arrays.
Extreme caution should be applied when using this feature.

Methods that return a 'void \*' in C++ will, in Python, return a
string with a hexadecimal number that gives the memory address.


Transmitting data from Python to SVTK
------------------------------------

If you have a large block of data in Python (for example a Numeric
array) that you want to access from SVTK, then you can do so using
the svtkDataArray.SetVoidArray() method.


Creating a Python object from just the address of a SVTK object
--------------------------------------------------------------

When you instantiate a class, you can provide a hexadecimal string
containing the address of an existing svtk object, e.g.

    t = svtkTransform('_1010e068_svtkTransform_p')

The string follows SWIG mangling conventions.  If a wrapper for the
specified object already exists, then that wrapper will be used rather
than a new wrapper being created.  If you want to use this feature
of svtkpython, please think twice.


SVTK C++ methods with 'pythonic' equivalents
-------------------------------------------

    SafeDownCast(): Unnecessary, Python already knows the real type
    IsA():          Python provides a built-in isinstance() method.
    IsTypeOf():     Python provides a built-in issubclass() method.
    GetClassName(): This info is given by o.__class__.__name__


Special SVTK types
===================

In addition to SVTK objects that are derived from svtkObjectBase, there
are many lightweight types in SVTK such as svtkTimeStamp or svtkVariant.
These can usually be distinguished from svtkObjects because they do
not have a C++ '::New()' method for construction.

These types are wrapped in a slightly different way from svtkObject-derived
classes.  The details of memory management are different because Python
actually keeps a copy of the object within its 'wrapper', whereas for
svtkObjects it just keeps a pointer.

An incomplete list of these types is as follows:
svtkVariant, svtkTimeStamp, svtkArrayCoordinates, svtkArrayExtents,
svtkArrayExtentsList, svtkArrayRange


Automatic conversion
--------------------

These special types can have several constructors, and the constructors
can be used for automatic type conversion for SVTK methods.  For
example, svtkVariantArray has a method InsertNextItem(svtkVariant v), and
svtkVariant has a constructor svtkVariant(int x).  So, you can do this:

    >>> variantArray.InsertNextItem(1)

The wrappers will automatically construct a svtkVariant from '1', and
will then pass it as a parameter to InsertNextItem.


Comparison and mapping
----------------------

Some special types can be sorted by value, and some can be used as
dict keys.  Sorting requires the existence of comparison operators
such as '<' '<=' '==' '>=' '>' and these are not automatically wrapped.
The use of an object as a dict key requires the computation of a
hash.  Comparison and hashing are supported by svtkVariant and
svtkTimeStamp, and will be supported by other types on a case-by-case
basis.

The reason that all svtkObjects can be easily hashed, while svtk
special types are hard to hash, is that svtkObjects are hashed
by memory address.  This cannot be done for special types, since
they must be hashed by value, not by address.  I.e. svtkVariant(1)
must hash equal to every other svtkVariant(1), even though the
various instances will lie and different memory addresses.


Special attributes available from SVTK-Python
============================================

Special svtk object attributes:
-----------------------------

    o.__class__   the class that this object is an instance of
    o.__doc__     a description of the class (obtained from C++ header file)
    o.__this__    a string containing the address of the SVTK object


Special method attributes:
--------------------------

    m.__doc__     a description of the method (obtained from C++ header file)


Special svtk class attributes:
----------------------------

    c.__bases__   a tuple of base classes for this class (empty for svtkObject)
    c.__doc__     a description of the class (obtained from C++ header file)
    c.__name__    the name of the class, same as returned by GetClassName()
