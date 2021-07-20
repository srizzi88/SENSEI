%module svtk

%{
#include "svtkObjectBase.h"
#include "svtkObject.h"

#include "svtkType.h"
#include "svtkCommonCoreModule.h"
#include "svtkCommonDataModelModule.h"
#include "svtkConfigure.h"
#include "svtk_kwiml.h"
#include "kwiml/int.h"
#include "kwiml/abi.h"
#include "svtkType.h"
#include "svtkSetGet.h"
#include "svtkWrappingHints.h"

#include "svtkAbstractArray.h"
#include "svtkAOSDataArrayTemplate.h"
#include "svtkArrayCoordinates.h"
#include "svtkArrayDispatch.h"
#include "svtkArrayExtents.h"
#include "svtkArrayExtentsList.h"
#include "svtkArray.h"
#include "svtkArrayInterpolate.h"
#include "svtkArrayIterator.h"
#include "svtkArrayIteratorIncludes.h"
#include "svtkArrayIteratorTemplate.h"
#include "svtkArrayPrint.h"
#include "svtkArrayRange.h"
#include "svtkArraySort.h"
#include "svtkArrayWeights.h"
#include "svtkBitArray.h"
#include "svtkBitArrayIterator.h"
#include "svtkCharArray.h"
#include "svtkDataArrayAccessor.h"
#include "svtkDataArrayCollection.h"
#include "svtkDataArrayCollectionIterator.h"
#include "svtkDataArray.h"
#include "svtkDataArrayIteratorMacro.h"
#include "svtkDataArrayMeta.h"
#include "svtkDataArrayRange.h"
#include "svtkDataArraySelection.h"
#include "svtkDataArrayTemplate.h"
#include "svtkDataArrayTupleRange_AOS.h"
#include "svtkDataArrayTupleRange_Generic.h"
#include "svtkDataArrayValueRange_AOS.h"
#include "svtkDataArrayValueRange_Generic.h"
#include "svtkDenseArray.h"
#include "svtkDoubleArray.h"
#include "svtkFloatArray.h"
#include "svtkGenericDataArray.h"
#include "svtkGenericDataArrayLookupHelper.h"
#include "svtkIdTypeArray.h"
#include "svtkIntArray.h"
#include "svtkLongArray.h"
#include "svtkLongLongArray.h"
#include "svtkMappedDataArray.h"
#include "svtkScaledSOADataArrayTemplate.h"
#include "svtkShortArray.h"
#include "svtkSignedCharArray.h"
#include "svtkSOADataArrayTemplate.h"
#include "svtkSortDataArray.h"
#include "svtkSparseArray.h"
#include "svtkStringArray.h"
#include "svtkTestDataArray.h"
#include "svtkTypedArray.h"
#include "svtkTypedDataArray.h"
#include "svtkTypedDataArrayIterator.h"
#include "svtkUnicodeStringArray.h"
#include "svtkUnsignedCharArray.h"
#include "svtkUnsignedIntArray.h"
#include "svtkUnsignedLongArray.h"
#include "svtkUnsignedLongLongArray.h"
#include "svtkUnsignedShortArray.h"
#include "svtkVariantArray.h"
#include "svtkVoidArray.h"
%}

#define __GNUC__
#define __x86_64
#define KWIML_INT_NO_VERIFY
#define KWIML_ABI_NO_VERIFY

/*---------------------------------------------------------------------------
macro: SVTK_DERIVED(derived_t)

arguments:
  derived_t - name of a class that derives from vtkObjectBase.

The macro causes SWIG to wrap the class and defines memory management hooks
that prevent memory leaks when SWIG creates the objects. Use this to wrap
SVTK classes defined in your project.
---------------------------------------------------------------------------*/
%define SVTK_DERIVED(derived_t)
%{
#include <derived_t##.h>
%}
%feature("ref") derived_t "$this->Register(nullptr);"
%feature("unref") derived_t "$this->UnRegister(nullptr);"
%newobject derived_t##::New();
%delobject derived_t##::Delete();
%typemap(newfree) derived_t* "$1->UnRegister(nullptr);"
%include <derived_t##.h>
%enddef



%include "svtkCommonCoreModule.h"
%include "svtkCommonDataModelModule.h"

%include "svtkConfigure.h"
%include "svtk_kwiml.h"
%include "kwiml/int.h"
%include "kwiml/abi.h"
%include "svtkType.h"
%include "svtkSetGet.h"
%include "svtkWrappingHints.h"

SVTK_DERIVED(svtkObjectBase)
SVTK_DERIVED(svtkObject)

SVTK_DERIVED(svtkAbstractArray)
SVTK_DERIVED(svtkDataArray)

SVTK_DERIVED(svtkGenericDataArray)
%template(svtkGenericDataArray_char) svtkGenericDataArray< svtkAOSDataArrayTemplate< char >, char >;
%template(svtkGenericDataArray_double) svtkGenericDataArray< svtkAOSDataArrayTemplate< double >, double  >;
%template(svtkGenericDataArray_float) svtkGenericDataArray< svtkAOSDataArrayTemplate< float >, float >;
/*%template(svtkGenericDataArray_svtkIdType) svtkGenericDataArray< svtkAOSDataArrayTemplate< svtkIdType > >;*/
%template(svtkGenericDataArray_int) svtkGenericDataArray< svtkAOSDataArrayTemplate< int >, int >;
%template(svtkGenericDataArray_long) svtkGenericDataArray< svtkAOSDataArrayTemplate< long >, long >;
%template(svtkGenericDataArray_long_long) svtkGenericDataArray< svtkAOSDataArrayTemplate< long long >, long long >;
%template(svtkGenericDataArray_short) svtkGenericDataArray< svtkAOSDataArrayTemplate< short >, short >;
%template(svtkGenericDataArray_signed_char) svtkGenericDataArray< svtkAOSDataArrayTemplate< signed char >, signed char >;
%template(svtkGenericDataArray_unsigned_char) svtkGenericDataArray< svtkAOSDataArrayTemplate< unsigned char >, unsigned char >;
%template(svtkGenericDataArray_unsigned_int) svtkGenericDataArray< svtkAOSDataArrayTemplate< unsigned int >, unsigned int >;
%template(svtkGenericDataArray_unsigned_long) svtkGenericDataArray< svtkAOSDataArrayTemplate< unsigned long >, unsigned long >;
%template(svtkGenericDataArray_unsigned_long_long) svtkGenericDataArray< svtkAOSDataArrayTemplate< unsigned long long >, unsigned long long >;
%template(svtkGenericDataArray_unsigned_short) svtkGenericDataArray< svtkAOSDataArrayTemplate< unsigned short >, unsigned short >;

SVTK_DERIVED(svtkAOSDataArrayTemplate)
%template(svtkAOSDataArrayTemplate_char) svtkAOSDataArrayTemplate< char >;
%template(svtkAOSDataArrayTemplate_double) svtkAOSDataArrayTemplate< double >;
%template(svtkAOSDataArrayTemplate_float) svtkAOSDataArrayTemplate< float >;
/*%template(svtkAOSDataArrayTemplate_svtkIdType) svtkAOSDataArrayTemplate< svtkIdType >;*/
%template(svtkAOSDataArrayTemplate_int) svtkAOSDataArrayTemplate< int >;
%template(svtkAOSDataArrayTemplate_long) svtkAOSDataArrayTemplate< long >;
%template(svtkAOSDataArrayTemplate_long_long) svtkAOSDataArrayTemplate< long long >;
%template(svtkAOSDataArrayTemplate_short) svtkAOSDataArrayTemplate< short >;
%template(svtkAOSDataArrayTemplate_signed_char) svtkAOSDataArrayTemplate< signed char >;
%template(svtkAOSDataArrayTemplate_unsigned_char) svtkAOSDataArrayTemplate< unsigned char >;
%template(svtkAOSDataArrayTemplate_unsigned_int) svtkAOSDataArrayTemplate< unsigned int >;
%template(svtkAOSDataArrayTemplate_unsigned_long) svtkAOSDataArrayTemplate< unsigned long >;
%template(svtkAOSDataArrayTemplate_unsigned_long_long) svtkAOSDataArrayTemplate< unsigned long long >;
%template(svtkAOSDataArrayTemplate_unsigned_short) svtkAOSDataArrayTemplate< unsigned short >;



/*SVTK_DERIVED(svtkArrayCoordinates)*/
SVTK_DERIVED(svtkArrayDispatch)
/*SVTK_DERIVED(svtkArrayExtents)
SVTK_DERIVED(svtkArrayExtentsList)*/
SVTK_DERIVED(svtkArray)
SVTK_DERIVED(svtkArrayInterpolate)
SVTK_DERIVED(svtkArrayIterator)
SVTK_DERIVED(svtkArrayIteratorIncludes)
SVTK_DERIVED(svtkArrayIteratorTemplate)
SVTK_DERIVED(svtkArrayPrint)
/*SVTK_DERIVED(svtkArrayRange)
SVTK_DERIVED(svtkArraySort)
SVTK_DERIVED(svtkArrayWeights)*/
SVTK_DERIVED(svtkBitArray)
SVTK_DERIVED(svtkBitArrayIterator)
SVTK_DERIVED(svtkCharArray)
SVTK_DERIVED(svtkDataArrayAccessor)
SVTK_DERIVED(svtkDataArrayCollection)
SVTK_DERIVED(svtkDataArrayCollectionIterator)
SVTK_DERIVED(svtkDataArrayIteratorMacro)
/*SVTK_DERIVED(svtkDataArrayMeta)
SVTK_DERIVED(svtkDataArrayRange)
SVTK_DERIVED(svtkDataArraySelection)*/
SVTK_DERIVED(svtkDataArrayTemplate)
/*SVTK_DERIVED(svtkDataArrayTupleRange_AOS)
SVTK_DERIVED(svtkDataArrayTupleRange_Generic)
SVTK_DERIVED(svtkDataArrayValueRange_AOS)
SVTK_DERIVED(svtkDataArrayValueRange_Generic)
*/
/*
SVTK_DERIVED(svtkDenseArray)
*/
SVTK_DERIVED(svtkDoubleArray)
SVTK_DERIVED(svtkFloatArray)
/*SVTK_DERIVED(svtkGenericDataArrayLookupHelper)
*/
SVTK_DERIVED(svtkIdTypeArray)
SVTK_DERIVED(svtkIntArray)
SVTK_DERIVED(svtkLongArray)
SVTK_DERIVED(svtkLongLongArray)
/*SVTK_DERIVED(svtkMappedDataArray)
SVTK_DERIVED(svtkScaledSOADataArrayTemplate)
*/
SVTK_DERIVED(svtkShortArray)
SVTK_DERIVED(svtkSignedCharArray)
SVTK_DERIVED(svtkSOADataArrayTemplate)
/*SVTK_DERIVED(svtkSortDataArray)
SVTK_DERIVED(svtkSparseArray)
SVTK_DERIVED(svtkStringArray)
SVTK_DERIVED(svtkTestDataArray)
*/
SVTK_DERIVED(svtkTypedArray)
SVTK_DERIVED(svtkTypedDataArray)
SVTK_DERIVED(svtkTypedDataArrayIterator)
SVTK_DERIVED(svtkUnicodeStringArray)
SVTK_DERIVED(svtkUnsignedCharArray)
SVTK_DERIVED(svtkUnsignedIntArray)
SVTK_DERIVED(svtkUnsignedLongArray)
SVTK_DERIVED(svtkUnsignedLongLongArray)
SVTK_DERIVED(svtkUnsignedShortArray)
SVTK_DERIVED(svtkVariantArray)
SVTK_DERIVED(svtkVoidArray)






