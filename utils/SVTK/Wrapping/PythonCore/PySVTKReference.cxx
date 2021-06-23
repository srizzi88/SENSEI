/*=========================================================================

  Program:   Visualization Toolkit
  Module:    PySVTKReference.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-----------------------------------------------------------------------
  The PySVTKReference was created in Sep 2010 by David Gobbi.

  This class is a proxy for immutable python objects like int, float,
  and string.  It allows these objects to be passed to SVTK methods that
  require a reference.
-----------------------------------------------------------------------*/

#include "PySVTKReference.h"
#include "svtkPythonUtil.h"

// Silence warning like
// "dereferencing type-punned pointer will break strict-aliasing rules"
// it happens because this kind of expression: (long *)&ptr
#if defined(__GNUC__)
#pragma GCC diagnostic ignored "-Wstrict-aliasing"
#endif

//--------------------------------------------------------------------

static const char* PySVTKReference_Doc =
  "A simple container that acts as a reference to its contents.\n\n"
  "This wrapper class is needed when a SVTK method returns a value\n"
  "in an argument that has been passed by reference.  By calling\n"
  "\"m = svtk.reference(a)\" on a value, you can create a proxy to\n"
  "that value.  The value can be changed by calling \"m.set(b)\".\n";

//--------------------------------------------------------------------
// helper method: make sure than an object is usable
static PyObject* PySVTKReference_CompatibleObject(PyObject* self, PyObject* opn)
{
  if (PySVTKReference_Check(opn))
  {
    if (self == nullptr || Py_TYPE(opn) == Py_TYPE(self))
    {
      // correct type, so return it
      opn = ((PySVTKReference*)opn)->value;
      Py_INCREF(opn);
      return opn;
    }
    // get contents, do further compatibility checks
    opn = ((PySVTKReference*)opn)->value;
  }

  // check if it is a string
  if (self == nullptr || Py_TYPE(self) == &PySVTKStringReference_Type)
  {
    if (
#ifdef Py_USING_UNICODE
      PyUnicode_Check(opn) ||
#endif
      PyBytes_Check(opn))
    {
      Py_INCREF(opn);
      return opn;
    }
  }

  // check if it is a tuple or list
  if (self == nullptr || Py_TYPE(self) == &PySVTKTupleReference_Type)
  {
    if (PyTuple_Check(opn) || PyList_Check(opn))
    {
      Py_INCREF(opn);
      return opn;
    }
  }

  // check if it is a number
  if (self == nullptr || Py_TYPE(self) == &PySVTKNumberReference_Type)
  {
    if (PyFloat_Check(opn) ||
#ifndef SVTK_PY3K
      PyInt_Check(opn) ||
#endif
      PyLong_Check(opn))
    {
      Py_INCREF(opn);
      return opn;
    }

    // check if it has number protocol and suitable methods
    PyNumberMethods* nb = Py_TYPE(opn)->tp_as_number;
    if (nb)
    {
      if (nb->nb_index)
      {
        opn = nb->nb_index(opn);
        if (opn == nullptr ||
          (!PyLong_Check(opn)
#ifndef SVTK_PY3K
            && !PyInt_Check(opn)
#endif
              ))
        {
          PyErr_SetString(PyExc_TypeError, "nb_index should return integer object");
          return nullptr;
        }
        return opn;
      }
      else if (nb->nb_float)
      {
        opn = nb->nb_float(opn);
        if (opn == nullptr || !PyFloat_Check(opn))
        {
          PyErr_SetString(PyExc_TypeError, "nb_float should return float object");
          return nullptr;
        }
        return opn;
      }
    }
  }

  // set error message according to required type
  const char* errmsg = "bad type";
  if (self == nullptr)
  {
    errmsg = "a numeric, string, or tuple object is required";
  }
  else if (Py_TYPE(self) == &PySVTKStringReference_Type)
  {
    errmsg = "a string object is required";
  }
  else if (Py_TYPE(self) == &PySVTKTupleReference_Type)
  {
    errmsg = "a tuple object is required";
  }
  else if (Py_TYPE(self) == &PySVTKNumberReference_Type)
  {
    errmsg = "a numeric object is required";
  }

  PyErr_SetString(PyExc_TypeError, errmsg);
  return nullptr;
}

//--------------------------------------------------------------------
// methods from C

PyObject* PySVTKReference_GetValue(PyObject* self)
{
  if (PySVTKReference_Check(self))
  {
    return ((PySVTKReference*)self)->value;
  }
  else
  {
    PyErr_SetString(PyExc_TypeError, "a svtk.reference() object is required");
  }

  return nullptr;
}

int PySVTKReference_SetValue(PyObject* self, PyObject* val)
{
  if (PySVTKReference_Check(self))
  {
    PyObject** op = &((PySVTKReference*)self)->value;

    PyObject* result = PySVTKReference_CompatibleObject(self, val);
    Py_DECREF(val);
    if (result)
    {
      Py_DECREF(*op);
      *op = result;
      return 0;
    }
  }
  else
  {
    PyErr_SetString(PyExc_TypeError, "a svtk.reference() object is required");
  }

  return -1;
}

//--------------------------------------------------------------------
// methods from python

static PyObject* PySVTKReference_Get(PyObject* self, PyObject* args)
{
  if (PyArg_ParseTuple(args, ":get"))
  {
    PyObject* ob = PySVTKReference_GetValue(self);
    Py_INCREF(ob);
    return ob;
  }

  return nullptr;
}

static PyObject* PySVTKReference_Set(PyObject* self, PyObject* args)
{
  PyObject* opn;

  if (PyArg_ParseTuple(args, "O:set", &opn))
  {
    opn = PySVTKReference_CompatibleObject(self, opn);

    if (opn)
    {
      if (PySVTKReference_SetValue(self, opn) == 0)
      {
        Py_INCREF(Py_None);
        return Py_None;
      }
    }
  }

  return nullptr;
}

#ifdef SVTK_PY3K
static PyObject* PySVTKReference_Trunc(PyObject* self, PyObject* args)
{
  PyObject* opn;

  if (PyArg_ParseTuple(args, ":__trunc__", &opn))
  {
    PyObject* attr = PyUnicode_InternFromString("__trunc__");
    PyObject* ob = PySVTKReference_GetValue(self);
    PyObject* meth = _PyType_Lookup(Py_TYPE(ob), attr);
    if (meth == nullptr)
    {
      PyErr_Format(
        PyExc_TypeError, "type %.100s doesn't define __trunc__ method", Py_TYPE(ob)->tp_name);
      return nullptr;
    }
#if PY_VERSION_HEX >= 0x03040000
    return PyObject_CallFunction(meth, "O", ob);
#else
    return PyObject_CallFunction(meth, const_cast<char*>("O"), ob);
#endif
  }

  return nullptr;
}

static PyObject* PySVTKReference_Round(PyObject* self, PyObject* args)
{
  PyObject* opn;

  if (PyArg_ParseTuple(args, "|O:__round__", &opn))
  {
    PyObject* attr = PyUnicode_InternFromString("__round__");
    PyObject* ob = PySVTKReference_GetValue(self);
    PyObject* meth = _PyType_Lookup(Py_TYPE(ob), attr);
    if (meth == nullptr)
    {
      PyErr_Format(
        PyExc_TypeError, "type %.100s doesn't define __round__ method", Py_TYPE(ob)->tp_name);
      return nullptr;
    }
#if PY_VERSION_HEX >= 0x03040000
    if (opn)
    {
      return PyObject_CallFunction(meth, "OO", ob, opn);
    }
    return PyObject_CallFunction(meth, "O", ob);
#else
    if (opn)
    {
      return PyObject_CallFunction(meth, const_cast<char*>("OO"), ob, opn);
    }
    return PyObject_CallFunction(meth, const_cast<char*>("O"), ob);
#endif
  }

  return nullptr;
}
#endif

static PyMethodDef PySVTKReference_Methods[] = { { "get", PySVTKReference_Get, METH_VARARGS,
                                                  "Get the stored value." },
  { "set", PySVTKReference_Set, METH_VARARGS, "Set the stored value." },
#ifdef SVTK_PY3K
  { "__trunc__", PySVTKReference_Trunc, METH_VARARGS,
    "Returns the Integral closest to x between 0 and x." },
  { "__round__", PySVTKReference_Round, METH_VARARGS,
    "Returns the Integral closest to x, rounding half toward even.\n" },
#endif
  { nullptr, nullptr, 0, nullptr } };

//--------------------------------------------------------------------
// Macros used for defining protocol methods

#define REFOBJECT_INTFUNC(prot, op)                                                                \
  static int PySVTKReference_##op(PyObject* ob)                                                     \
  {                                                                                                \
    ob = ((PySVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob);                                                                    \
  }

#define REFOBJECT_INTFUNC2(prot, op)                                                               \
  static int PySVTKReference_##op(PyObject* ob, PyObject* o)                                        \
  {                                                                                                \
    ob = ((PySVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob, o);                                                                 \
  }

#define REFOBJECT_SIZEFUNC(prot, op)                                                               \
  static Py_ssize_t PySVTKReference_##op(PyObject* ob)                                              \
  {                                                                                                \
    ob = ((PySVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob);                                                                    \
  }

#define REFOBJECT_INDEXFUNC(prot, op)                                                              \
  static PyObject* PySVTKReference_##op(PyObject* ob, Py_ssize_t i)                                 \
  {                                                                                                \
    ob = ((PySVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob, i);                                                                 \
  }

#define REFOBJECT_INDEXSETFUNC(prot, op)                                                           \
  static int PySVTKReference_##op(PyObject* ob, Py_ssize_t i, PyObject* o)                          \
  {                                                                                                \
    ob = ((PySVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob, i, o);                                                              \
  }

#define REFOBJECT_SLICEFUNC(prot, op)                                                              \
  static PyObject* PySVTKReference_##op(PyObject* ob, Py_ssize_t i, Py_ssize_t j)                   \
  {                                                                                                \
    ob = ((PySVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob, i, j);                                                              \
  }

#define REFOBJECT_SLICESETFUNC(prot, op)                                                           \
  static int PySVTKReference_##op(PyObject* ob, Py_ssize_t i, Py_ssize_t j, PyObject* o)            \
  {                                                                                                \
    ob = ((PySVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob, i, j, o);                                                           \
  }

#define REFOBJECT_UNARYFUNC(prot, op)                                                              \
  static PyObject* PySVTKReference_##op(PyObject* ob)                                               \
  {                                                                                                \
    ob = ((PySVTKReference*)ob)->value;                                                             \
    return Py##prot##_##op(ob);                                                                    \
  }

#define REFOBJECT_BINARYFUNC(prot, op)                                                             \
  static PyObject* PySVTKReference_##op(PyObject* ob1, PyObject* ob2)                               \
  {                                                                                                \
    if (PySVTKReference_Check(ob1))                                                                 \
    {                                                                                              \
      ob1 = ((PySVTKReference*)ob1)->value;                                                         \
    }                                                                                              \
    if (PySVTKReference_Check(ob2))                                                                 \
    {                                                                                              \
      ob2 = ((PySVTKReference*)ob2)->value;                                                         \
    }                                                                                              \
    return Py##prot##_##op(ob1, ob2);                                                              \
  }

#define REFOBJECT_INPLACEFUNC(prot, op)                                                            \
  static PyObject* PySVTKReference_InPlace##op(PyObject* ob1, PyObject* ob2)                        \
  {                                                                                                \
    PySVTKReference* ob = (PySVTKReference*)ob1;                                                     \
    PyObject* obn;                                                                                 \
    ob1 = ob->value;                                                                               \
    if (PySVTKReference_Check(ob2))                                                                 \
    {                                                                                              \
      ob2 = ((PySVTKReference*)ob2)->value;                                                         \
    }                                                                                              \
    obn = Py##prot##_##op(ob1, ob2);                                                               \
    if (obn)                                                                                       \
    {                                                                                              \
      ob->value = obn;                                                                             \
      Py_DECREF(ob1);                                                                              \
      Py_INCREF(ob);                                                                               \
      return (PyObject*)ob;                                                                        \
    }                                                                                              \
    return 0;                                                                                      \
  }

#define REFOBJECT_INPLACEIFUNC(prot, op)                                                           \
  static PyObject* PySVTKReference_InPlace##op(PyObject* ob1, Py_ssize_t i)                         \
  {                                                                                                \
    PySVTKReference* ob = (PySVTKReference*)ob1;                                                     \
    PyObject* obn;                                                                                 \
    ob1 = ob->value;                                                                               \
    obn = Py##prot##_##op(ob1, i);                                                                 \
    if (obn)                                                                                       \
    {                                                                                              \
      ob->value = obn;                                                                             \
      Py_DECREF(ob1);                                                                              \
      Py_INCREF(ob);                                                                               \
      return (PyObject*)ob;                                                                        \
    }                                                                                              \
    return 0;                                                                                      \
  }

#define REFOBJECT_TERNARYFUNC(prot, op)                                                            \
  static PyObject* PySVTKReference_##op(PyObject* ob1, PyObject* ob2, PyObject* ob3)                \
  {                                                                                                \
    if (PySVTKReference_Check(ob1))                                                                 \
    {                                                                                              \
      ob1 = ((PySVTKReference*)ob1)->value;                                                         \
    }                                                                                              \
    if (PySVTKReference_Check(ob2))                                                                 \
    {                                                                                              \
      ob2 = ((PySVTKReference*)ob2)->value;                                                         \
    }                                                                                              \
    if (PySVTKReference_Check(ob2))                                                                 \
    {                                                                                              \
      ob3 = ((PySVTKReference*)ob3)->value;                                                         \
    }                                                                                              \
    return Py##prot##_##op(ob1, ob2, ob3);                                                         \
  }

#define REFOBJECT_INPLACETFUNC(prot, op)                                                           \
  static PyObject* PySVTKReference_InPlace##op(PyObject* ob1, PyObject* ob2, PyObject* ob3)         \
  {                                                                                                \
    PySVTKReference* ob = (PySVTKReference*)ob1;                                                     \
    PyObject* obn;                                                                                 \
    ob1 = ob->value;                                                                               \
    if (PySVTKReference_Check(ob2))                                                                 \
    {                                                                                              \
      ob2 = ((PySVTKReference*)ob2)->value;                                                         \
    }                                                                                              \
    if (PySVTKReference_Check(ob3))                                                                 \
    {                                                                                              \
      ob3 = ((PySVTKReference*)ob3)->value;                                                         \
    }                                                                                              \
    obn = Py##prot##_##op(ob1, ob2, ob3);                                                          \
    if (obn)                                                                                       \
    {                                                                                              \
      ob->value = obn;                                                                             \
      Py_DECREF(ob1);                                                                              \
      Py_INCREF(ob);                                                                               \
      return (PyObject*)ob;                                                                        \
    }                                                                                              \
    return 0;                                                                                      \
  }

//--------------------------------------------------------------------
// Number protocol

static int PySVTKReference_NonZero(PyObject* ob)
{
  ob = ((PySVTKReference*)ob)->value;
  return PyObject_IsTrue(ob);
}

#ifndef SVTK_PY3K
static int PySVTKReference_Coerce(PyObject** ob1, PyObject** ob2)
{
  *ob1 = ((PySVTKReference*)*ob1)->value;
  if (PySVTKReference_Check(*ob2))
  {
    *ob2 = ((PySVTKReference*)*ob2)->value;
  }
  return PyNumber_CoerceEx(ob1, ob2);
}

static PyObject* PySVTKReference_Hex(PyObject* ob)
{
  ob = ((PySVTKReference*)ob)->value;
#if PY_VERSION_HEX >= 0x02060000
  return PyNumber_ToBase(ob, 16);
#else
  if (Py_TYPE(ob)->tp_as_number && Py_TYPE(ob)->tp_as_number->nb_hex)
  {
    return Py_TYPE(ob)->tp_as_number->nb_hex(ob);
  }

  PyErr_SetString(PyExc_TypeError, "hex() argument can't be converted to hex");
  return nullptr;
#endif
}

static PyObject* PySVTKReference_Oct(PyObject* ob)
{
  ob = ((PySVTKReference*)ob)->value;
#if PY_VERSION_HEX >= 0x02060000
  return PyNumber_ToBase(ob, 8);
#else
  if (Py_TYPE(ob)->tp_as_number && Py_TYPE(ob)->tp_as_number->nb_oct)
  {
    return Py_TYPE(ob)->tp_as_number->nb_oct(ob);
  }

  PyErr_SetString(PyExc_TypeError, "oct() argument can't be converted to oct");
  return nullptr;
#endif
}
#endif

REFOBJECT_BINARYFUNC(Number, Add)
REFOBJECT_BINARYFUNC(Number, Subtract)
REFOBJECT_BINARYFUNC(Number, Multiply)
#ifndef SVTK_PY3K
REFOBJECT_BINARYFUNC(Number, Divide)
#endif
REFOBJECT_BINARYFUNC(Number, Remainder)
REFOBJECT_BINARYFUNC(Number, Divmod)
REFOBJECT_TERNARYFUNC(Number, Power)
REFOBJECT_UNARYFUNC(Number, Negative)
REFOBJECT_UNARYFUNC(Number, Positive)
REFOBJECT_UNARYFUNC(Number, Absolute)
// NonZero
REFOBJECT_UNARYFUNC(Number, Invert)
REFOBJECT_BINARYFUNC(Number, Lshift)
REFOBJECT_BINARYFUNC(Number, Rshift)
REFOBJECT_BINARYFUNC(Number, And)
REFOBJECT_BINARYFUNC(Number, Or)
REFOBJECT_BINARYFUNC(Number, Xor)
// Coerce
#ifndef SVTK_PY3K
REFOBJECT_UNARYFUNC(Number, Int)
#endif
REFOBJECT_UNARYFUNC(Number, Long)
REFOBJECT_UNARYFUNC(Number, Float)

REFOBJECT_INPLACEFUNC(Number, Add)
REFOBJECT_INPLACEFUNC(Number, Subtract)
REFOBJECT_INPLACEFUNC(Number, Multiply)
#ifndef SVTK_PY3K
REFOBJECT_INPLACEFUNC(Number, Divide)
#endif
REFOBJECT_INPLACEFUNC(Number, Remainder)
REFOBJECT_INPLACETFUNC(Number, Power)
REFOBJECT_INPLACEFUNC(Number, Lshift)
REFOBJECT_INPLACEFUNC(Number, Rshift)
REFOBJECT_INPLACEFUNC(Number, And)
REFOBJECT_INPLACEFUNC(Number, Or)
REFOBJECT_INPLACEFUNC(Number, Xor)

REFOBJECT_BINARYFUNC(Number, FloorDivide)
REFOBJECT_BINARYFUNC(Number, TrueDivide)
REFOBJECT_INPLACEFUNC(Number, FloorDivide)
REFOBJECT_INPLACEFUNC(Number, TrueDivide)

REFOBJECT_UNARYFUNC(Number, Index)

//--------------------------------------------------------------------
static PyNumberMethods PySVTKReference_AsNumber = {
  PySVTKReference_Add,      // nb_add
  PySVTKReference_Subtract, // nb_subtract
  PySVTKReference_Multiply, // nb_multiply
#ifndef SVTK_PY3K
  PySVTKReference_Divide, // nb_divide
#endif
  PySVTKReference_Remainder, // nb_remainder
  PySVTKReference_Divmod,    // nb_divmod
  PySVTKReference_Power,     // nb_power
  PySVTKReference_Negative,  // nb_negative
  PySVTKReference_Positive,  // nb_positive
  PySVTKReference_Absolute,  // nb_absolute
  PySVTKReference_NonZero,   // nb_nonzero
  PySVTKReference_Invert,    // nb_invert
  PySVTKReference_Lshift,    // nb_lshift
  PySVTKReference_Rshift,    // nb_rshift
  PySVTKReference_And,       // nb_and
  PySVTKReference_Xor,       // nb_xor
  PySVTKReference_Or,        // nb_or
#ifndef SVTK_PY3K
  PySVTKReference_Coerce, // nb_coerce
  PySVTKReference_Int,    // nb_int
  PySVTKReference_Long,   // nb_long
#else
  PySVTKReference_Long, // nb_int
  nullptr,             // nb_reserved
#endif
  PySVTKReference_Float, // nb_float
#ifndef SVTK_PY3K
  PySVTKReference_Oct, // nb_oct
  PySVTKReference_Hex, // nb_hex
#endif
  PySVTKReference_InPlaceAdd,      // nb_inplace_add
  PySVTKReference_InPlaceSubtract, // nb_inplace_subtract
  PySVTKReference_InPlaceMultiply, // nb_inplace_multiply
#ifndef SVTK_PY3K
  PySVTKReference_InPlaceDivide, // nb_inplace_divide
#endif
  PySVTKReference_InPlaceRemainder,   // nb_inplace_remainder
  PySVTKReference_InPlacePower,       // nb_inplace_power
  PySVTKReference_InPlaceLshift,      // nb_inplace_lshift
  PySVTKReference_InPlaceRshift,      // nb_inplace_rshift
  PySVTKReference_InPlaceAnd,         // nb_inplace_and
  PySVTKReference_InPlaceXor,         // nb_inplace_xor
  PySVTKReference_InPlaceOr,          // nb_inplace_or
  PySVTKReference_FloorDivide,        // nb_floor_divide
  PySVTKReference_TrueDivide,         // nb_true_divide
  PySVTKReference_InPlaceFloorDivide, // nb_inplace_floor_divide
  PySVTKReference_InPlaceTrueDivide,  // nb_inplace_true_divide
  PySVTKReference_Index,              // nb_index
#if PY_VERSION_HEX >= 0x03050000
  nullptr, // nb_matrix_multiply
  nullptr, // nb_inplace_matrix_multiply
#endif
};

//--------------------------------------------------------------------
static PyNumberMethods PySVTKStringReference_AsNumber = {
  nullptr, // nb_add
  nullptr, // nb_subtract
  nullptr, // nb_multiply
#ifndef SVTK_PY3K
  nullptr, // nb_divide
#endif
  PySVTKReference_Remainder, // nb_remainder
  nullptr,                  // nb_divmod
  nullptr,                  // nb_power
  nullptr,                  // nb_negative
  nullptr,                  // nb_positive
  nullptr,                  // nb_absolute
  nullptr,                  // nb_nonzero
  nullptr,                  // nb_invert
  nullptr,                  // nb_lshift
  nullptr,                  // nb_rshift
  nullptr,                  // nb_and
  nullptr,                  // nb_xor
  nullptr,                  // nb_or
#ifndef SVTK_PY3K
  nullptr, // nb_coerce
  nullptr, // nb_int
  nullptr, // nb_long
#else
  nullptr,             // nb_int
  nullptr,             // nb_reserved
#endif
  nullptr, // nb_float
#ifndef SVTK_PY3K
  nullptr, // nb_oct
  nullptr, // nb_hex
#endif
  nullptr, // nb_inplace_add
  nullptr, // nb_inplace_subtract
  nullptr, // nb_inplace_multiply
#ifndef SVTK_PY3K
  nullptr, // nb_inplace_divide
#endif
  nullptr, // nb_inplace_remainder
  nullptr, // nb_inplace_power
  nullptr, // nb_inplace_lshift
  nullptr, // nb_inplace_rshift
  nullptr, // nb_inplace_and
  nullptr, // nb_inplace_xor
  nullptr, // nb_inplace_or
  nullptr, // nb_floor_divide
  nullptr, // nb_true_divide
  nullptr, // nb_inplace_floor_divide
  nullptr, // nb_inplace_true_divide
  nullptr, // nb_index
#if PY_VERSION_HEX >= 0x03050000
  nullptr, // nb_matrix_multiply
  nullptr, // nb_inplace_matrix_multiply
#endif
};

//--------------------------------------------------------------------
// Sequence protocol

REFOBJECT_SIZEFUNC(Sequence, Size)
REFOBJECT_BINARYFUNC(Sequence, Concat)
REFOBJECT_INDEXFUNC(Sequence, Repeat)
REFOBJECT_INDEXFUNC(Sequence, GetItem)
#ifndef SVTK_PY3K
REFOBJECT_SLICEFUNC(Sequence, GetSlice)
#endif
REFOBJECT_INTFUNC2(Sequence, Contains)

//--------------------------------------------------------------------
static PySequenceMethods PySVTKReference_AsSequence = {
  PySVTKReference_Size,    // sq_length
  PySVTKReference_Concat,  // sq_concat
  PySVTKReference_Repeat,  // sq_repeat
  PySVTKReference_GetItem, // sq_item
#ifndef SVTK_PY3K
  PySVTKReference_GetSlice, // sq_slice
#else
  nullptr,             // sq_slice
#endif
  nullptr,                 // sq_ass_item
  nullptr,                 // sq_ass_slice
  PySVTKReference_Contains, // sq_contains
  nullptr,                 // sq_inplace_concat
  nullptr,                 // sq_inplace_repeat
};

//--------------------------------------------------------------------
// Mapping protocol

static PyObject* PySVTKReference_GetMapItem(PyObject* ob, PyObject* key)
{
  ob = ((PySVTKReference*)ob)->value;
  return PyObject_GetItem(ob, key);
}

//--------------------------------------------------------------------
static PyMappingMethods PySVTKReference_AsMapping = {
  PySVTKReference_Size,       // mp_length
  PySVTKReference_GetMapItem, // mp_subscript
  nullptr,                   // mp_ass_subscript
};

//--------------------------------------------------------------------
// Buffer protocol

#ifndef SVTK_PY3K
// old buffer protocol
static Py_ssize_t PySVTKReference_GetReadBuf(PyObject* op, Py_ssize_t segment, void** ptrptr)
{
  char text[80];
  PyBufferProcs* pb;
  op = ((PySVTKReference*)op)->value;
  pb = Py_TYPE(op)->tp_as_buffer;

  if (pb && pb->bf_getreadbuffer)
  {
    return Py_TYPE(op)->tp_as_buffer->bf_getreadbuffer(op, segment, ptrptr);
  }

  snprintf(text, sizeof(text), "type \'%.20s\' does not support readable buffer access",
    Py_TYPE(op)->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}

static Py_ssize_t PySVTKReference_GetWriteBuf(PyObject* op, Py_ssize_t segment, void** ptrptr)
{
  char text[80];
  PyBufferProcs* pb;
  op = ((PySVTKReference*)op)->value;
  pb = Py_TYPE(op)->tp_as_buffer;

  if (pb && pb->bf_getwritebuffer)
  {
    return Py_TYPE(op)->tp_as_buffer->bf_getwritebuffer(op, segment, ptrptr);
  }

  snprintf(text, sizeof(text), "type \'%.20s\' does not support writeable buffer access",
    Py_TYPE(op)->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}

static Py_ssize_t PySVTKReference_GetSegCount(PyObject* op, Py_ssize_t* lenp)
{
  char text[80];
  PyBufferProcs* pb;
  op = ((PySVTKReference*)op)->value;
  pb = Py_TYPE(op)->tp_as_buffer;

  if (pb && pb->bf_getsegcount)
  {
    return Py_TYPE(op)->tp_as_buffer->bf_getsegcount(op, lenp);
  }

  snprintf(
    text, sizeof(text), "type \'%.20s\' does not support buffer access", Py_TYPE(op)->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}

static Py_ssize_t PySVTKReference_GetCharBuf(PyObject* op, Py_ssize_t segment, char** ptrptr)
{
  char text[80];
  PyBufferProcs* pb;
  op = ((PySVTKReference*)op)->value;
  pb = Py_TYPE(op)->tp_as_buffer;

  if (pb && pb->bf_getcharbuffer)
  {
    return Py_TYPE(op)->tp_as_buffer->bf_getcharbuffer(op, segment, ptrptr);
  }

  snprintf(text, sizeof(text), "type \'%.20s\' does not support character buffer access",
    Py_TYPE(op)->tp_name);
  PyErr_SetString(PyExc_TypeError, text);

  return -1;
}
#endif

#if PY_VERSION_HEX >= 0x02060000
// new buffer protocol
static int PySVTKReference_GetBuffer(PyObject* self, Py_buffer* view, int flags)
{
  PyObject* obj = ((PySVTKReference*)self)->value;
  return PyObject_GetBuffer(obj, view, flags);
}

static void PySVTKReference_ReleaseBuffer(PyObject*, Py_buffer* view)
{
  PyBuffer_Release(view);
}
#endif

static PyBufferProcs PySVTKReference_AsBuffer = {
#ifndef SVTK_PY3K
  PySVTKReference_GetReadBuf,  // bf_getreadbuffer
  PySVTKReference_GetWriteBuf, // bf_getwritebuffer
  PySVTKReference_GetSegCount, // bf_getsegcount
  PySVTKReference_GetCharBuf,  // bf_getcharbuffer
#endif
#if PY_VERSION_HEX >= 0x02060000
  PySVTKReference_GetBuffer,    // bf_getbuffer
  PySVTKReference_ReleaseBuffer // bf_releasebuffer
#endif
};

//--------------------------------------------------------------------
// Object protocol

static void PySVTKReference_Delete(PyObject* ob)
{
  Py_DECREF(((PySVTKReference*)ob)->value);
  PyObject_Del(ob);
}

static PyObject* PySVTKReference_Repr(PyObject* ob)
{
  PyObject* r = nullptr;
  const char* name = Py_TYPE(ob)->tp_name;
  PyObject* s = PyObject_Repr(((PySVTKReference*)ob)->value);
  if (s)
  {
#ifdef SVTK_PY3K
    r = PyUnicode_FromFormat("%s(%U)", name, s);
#else
    const char* text = PyString_AsString(s);
    size_t n = strlen(name) + strlen(text) + 3; // for '(', ')', null
    if (n > 128)
    {
      char* cp = (char*)malloc(n);
      snprintf(cp, n, "%s(%s)", name, text);
      r = PyString_FromString(cp);
      free(cp);
    }
    else
    {
      char textspace[128];
      snprintf(textspace, sizeof(textspace), "%s(%s)", name, text);
      r = PyString_FromString(textspace);
    }
#endif
    Py_DECREF(s);
  }
  return r;
}

static PyObject* PySVTKReference_Str(PyObject* ob)
{
  return PyObject_Str(((PySVTKReference*)ob)->value);
}

static PyObject* PySVTKReference_RichCompare(PyObject* ob1, PyObject* ob2, int opid)
{
  if (PySVTKReference_Check(ob1))
  {
    ob1 = ((PySVTKReference*)ob1)->value;
  }
  if (PySVTKReference_Check(ob2))
  {
    ob2 = ((PySVTKReference*)ob2)->value;
  }
  return PyObject_RichCompare(ob1, ob2, opid);
}

static PyObject* PySVTKReference_GetIter(PyObject* ob)
{
  return PyObject_GetIter(((PySVTKReference*)ob)->value);
}

static PyObject* PySVTKReference_GetAttr(PyObject* self, PyObject* attr)
{
  PyObject* a = PyObject_GenericGetAttr(self, attr);
  if (a || !PyErr_ExceptionMatches(PyExc_AttributeError))
  {
    return a;
  }
  PyErr_Clear();

#ifndef SVTK_PY3K
  char* name = PyString_AsString(attr);
  int firstchar = name[0];
#elif PY_VERSION_HEX >= 0x03030000
  int firstchar = '\0';
  if (PyUnicode_GetLength(attr) > 0)
  {
    firstchar = PyUnicode_ReadChar(attr, 0);
  }
#else
  int firstchar = '\0';
  if (PyUnicode_Check(attr) && PyUnicode_GetSize(attr) > 0)
  {
    firstchar = PyUnicode_AS_UNICODE(attr)[0];
  }
#endif
  if (firstchar != '_')
  {
    a = PyObject_GetAttr(((PySVTKReference*)self)->value, attr);

    if (a || !PyErr_ExceptionMatches(PyExc_AttributeError))
    {
      return a;
    }
    PyErr_Clear();
  }

#ifdef SVTK_PY3K
  PyErr_Format(
    PyExc_AttributeError, "'%.50s' object has no attribute '%U'", Py_TYPE(self)->tp_name, attr);
#else
  PyErr_Format(
    PyExc_AttributeError, "'%.50s' object has no attribute '%.80s'", Py_TYPE(self)->tp_name, name);
#endif
  return nullptr;
}

static PyObject* PySVTKReference_New(PyTypeObject*, PyObject* args, PyObject* kwds)
{
  PyObject* o;

  if (kwds && PyDict_Size(kwds))
  {
    PyErr_SetString(PyExc_TypeError, "reference() does not take keyword arguments");
    return nullptr;
  }

  if (PyArg_ParseTuple(args, "O:reference", &o))
  {
    o = PySVTKReference_CompatibleObject(nullptr, o);

    if (o)
    {
      PySVTKReference* self;
      if (
#ifdef Py_USING_UNICODE
        PyUnicode_Check(o) ||
#endif
        PyBytes_Check(o))
      {
        self = PyObject_New(PySVTKReference, &PySVTKStringReference_Type);
      }
      else if (PyTuple_Check(o) || PyList_Check(o))
      {
        self = PyObject_New(PySVTKReference, &PySVTKTupleReference_Type);
      }
      else
      {
        self = PyObject_New(PySVTKReference, &PySVTKNumberReference_Type);
      }
      self->value = o;
      return (PyObject*)self;
    }
  }

  return nullptr;
}

// clang-format off
//--------------------------------------------------------------------
PyTypeObject PySVTKReference_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "svtkmodules.svtkCommonCore.reference", // tp_name
  sizeof(PySVTKReference), // tp_basicsize
  0,                      // tp_itemsize
  PySVTKReference_Delete,  // tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
  0,                      // tp_vectorcall_offset
#else
  nullptr,                // tp_print
#endif
  nullptr,                // tp_getattr
  nullptr,                // tp_setattr
  nullptr,                // tp_compare
  PySVTKReference_Repr,    // tp_repr
  nullptr,                // tp_as_number
  nullptr,                // tp_as_sequence
  nullptr,                // tp_as_mapping
#if PY_VERSION_HEX >= 0x02060000
  PyObject_HashNotImplemented, // tp_hash
#else
  nullptr,                // tp_hash
#endif
  nullptr,                // tp_call
  PySVTKReference_Str,     // tp_string
  PySVTKReference_GetAttr, // tp_getattro
  nullptr,                // tp_setattro
  nullptr,                // tp_as_buffer
#ifndef SVTK_PY3K
  Py_TPFLAGS_CHECKTYPES |
#endif
    Py_TPFLAGS_DEFAULT,       // tp_flags
  PySVTKReference_Doc,         // tp_doc
  nullptr,                    // tp_traverse
  nullptr,                    // tp_clear
  PySVTKReference_RichCompare, // tp_richcompare
  0,                          // tp_weaklistoffset
  nullptr,                    // tp_iter
  nullptr,                    // tp_iternext
  PySVTKReference_Methods,     // tp_methods
  nullptr,                    // tp_members
  nullptr,                    // tp_getset
  nullptr,                    // tp_base
  nullptr,                    // tp_dict
  nullptr,                    // tp_descr_get
  nullptr,                    // tp_descr_set
  0,                          // tp_dictoffset
  nullptr,                    // tp_init
  nullptr,                    // tp_alloc
  PySVTKReference_New,         // tp_new
  PyObject_Del,               // tp_free
  nullptr,                    // tp_is_gc
  nullptr,                    // tp_bases
  nullptr,                    // tp_mro
  nullptr,                    // tp_cache
  nullptr,                    // tp_subclasses
  nullptr,                    // tp_weaklist
  SVTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED };

//--------------------------------------------------------------------
PyTypeObject PySVTKNumberReference_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "svtkmodules.svtkCommonCore.number_reference", // tp_name
  sizeof(PySVTKReference),   // tp_basicsize
  0,                        // tp_itemsize
  PySVTKReference_Delete,    // tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
  0,                        // tp_vectorcall_offset
#else
  nullptr,                  // tp_print
#endif
  nullptr,                  // tp_getattr
  nullptr,                  // tp_setattr
  nullptr,                  // tp_compare
  PySVTKReference_Repr,      // tp_repr
  &PySVTKReference_AsNumber, // tp_as_number
  nullptr,                  // tp_as_sequence
  nullptr,                  // tp_as_mapping
#if PY_VERSION_HEX >= 0x02060000
  PyObject_HashNotImplemented, // tp_hash
#else
  nullptr,                  // tp_hash
#endif
  nullptr,                  // tp_call
  PySVTKReference_Str,       // tp_string
  PySVTKReference_GetAttr,   // tp_getattro
  nullptr,                  // tp_setattro
  nullptr,                  // tp_as_buffer
#ifndef SVTK_PY3K
  Py_TPFLAGS_CHECKTYPES |
#endif
    Py_TPFLAGS_DEFAULT,                // tp_flags
  PySVTKReference_Doc,                  // tp_doc
  nullptr,                             // tp_traverse
  nullptr,                             // tp_clear
  PySVTKReference_RichCompare,          // tp_richcompare
  0,                                   // tp_weaklistoffset
  nullptr,                             // tp_iter
  nullptr,                             // tp_iternext
  PySVTKReference_Methods,              // tp_methods
  nullptr,                             // tp_members
  nullptr,                             // tp_getset
  (PyTypeObject*)&PySVTKReference_Type, // tp_base
  nullptr,                             // tp_dict
  nullptr,                             // tp_descr_get
  nullptr,                             // tp_descr_set
  0,                                   // tp_dictoffset
  nullptr,                             // tp_init
  nullptr,                             // tp_alloc
  PySVTKReference_New,                  // tp_new
  PyObject_Del,                        // tp_free
  nullptr,                             // tp_is_gc
  nullptr,                             // tp_bases
  nullptr,                             // tp_mro
  nullptr,                             // tp_cache
  nullptr,                             // tp_subclasses
  nullptr,                             // tp_weaklist
  SVTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED };

//--------------------------------------------------------------------
PyTypeObject PySVTKStringReference_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "svtkmodules.svtkCommonCore.string_reference", // tp_name
  sizeof(PySVTKReference),         // tp_basicsize
  0,                              // tp_itemsize
  PySVTKReference_Delete,          // tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
  0,                              // tp_vectorcall_offset
#else
  nullptr,                        // tp_print
#endif
  nullptr,                        // tp_getattr
  nullptr,                        // tp_setattr
  nullptr,                        // tp_compare
  PySVTKReference_Repr,            // tp_repr
  &PySVTKStringReference_AsNumber, // tp_as_number
  &PySVTKReference_AsSequence,     // tp_as_sequence
  &PySVTKReference_AsMapping,      // tp_as_mapping
#if PY_VERSION_HEX >= 0x02060000
  PyObject_HashNotImplemented,    // tp_hash
#else
  nullptr,                        // tp_hash
#endif
  nullptr,                        // tp_call
  PySVTKReference_Str,             // tp_string
  PySVTKReference_GetAttr,         // tp_getattro
  nullptr,                        // tp_setattro
  &PySVTKReference_AsBuffer,       // tp_as_buffer
#ifndef SVTK_PY3K
  Py_TPFLAGS_CHECKTYPES |
#endif
    Py_TPFLAGS_DEFAULT,                // tp_flags
  PySVTKReference_Doc,                  // tp_doc
  nullptr,                             // tp_traverse
  nullptr,                             // tp_clear
  PySVTKReference_RichCompare,          // tp_richcompare
  0,                                   // tp_weaklistoffset
  PySVTKReference_GetIter,              // tp_iter
  nullptr,                             // tp_iternext
  PySVTKReference_Methods,              // tp_methods
  nullptr,                             // tp_members
  nullptr,                             // tp_getset
  (PyTypeObject*)&PySVTKReference_Type, // tp_base
  nullptr,                             // tp_dict
  nullptr,                             // tp_descr_get
  nullptr,                             // tp_descr_set
  0,                                   // tp_dictoffset
  nullptr,                             // tp_init
  nullptr,                             // tp_alloc
  PySVTKReference_New,                  // tp_new
  PyObject_Del,                        // tp_free
  nullptr,                             // tp_is_gc
  nullptr,                             // tp_bases
  nullptr,                             // tp_mro
  nullptr,                             // tp_cache
  nullptr,                             // tp_subclasses
  nullptr,                             // tp_weaklist
  SVTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED };

//--------------------------------------------------------------------
PyTypeObject PySVTKTupleReference_Type = {
  PyVarObject_HEAD_INIT(&PyType_Type, 0)
  "svtkmodules.svtkCommonCore.tuple_reference", // tp_name
  sizeof(PySVTKReference),     // tp_basicsize
  0,                          // tp_itemsize
  PySVTKReference_Delete,      // tp_dealloc
#if PY_VERSION_HEX >= 0x03080000
  0,                          // tp_vectorcall_offset
#else
  nullptr,                    // tp_print
#endif
  nullptr,                    // tp_getattr
  nullptr,                    // tp_setattr
  nullptr,                    // tp_compare
  PySVTKReference_Repr,        // tp_repr
  nullptr,                    // tp_as_number
  &PySVTKReference_AsSequence, // tp_as_sequence
  &PySVTKReference_AsMapping,  // tp_as_mapping
#if PY_VERSION_HEX >= 0x02060000
  PyObject_HashNotImplemented, // tp_hash
#else
  nullptr,                    // tp_hash
#endif
  nullptr,                    // tp_call
  PySVTKReference_Str,         // tp_string
  PySVTKReference_GetAttr,     // tp_getattro
  nullptr,                    // tp_setattro
  nullptr,                    // tp_as_buffer
#ifndef SVTK_PY3K
  Py_TPFLAGS_CHECKTYPES |
#endif
    Py_TPFLAGS_DEFAULT,                // tp_flags
  PySVTKReference_Doc,                  // tp_doc
  nullptr,                             // tp_traverse
  nullptr,                             // tp_clear
  PySVTKReference_RichCompare,          // tp_richcompare
  0,                                   // tp_weaklistoffset
  PySVTKReference_GetIter,              // tp_iter
  nullptr,                             // tp_iternext
  PySVTKReference_Methods,              // tp_methods
  nullptr,                             // tp_members
  nullptr,                             // tp_getset
  (PyTypeObject*)&PySVTKReference_Type, // tp_base
  nullptr,                             // tp_dict
  nullptr,                             // tp_descr_get
  nullptr,                             // tp_descr_set
  0,                                   // tp_dictoffset
  nullptr,                             // tp_init
  nullptr,                             // tp_alloc
  PySVTKReference_New,                  // tp_new
  PyObject_Del,                        // tp_free
  nullptr,                             // tp_is_gc
  nullptr,                             // tp_bases
  nullptr,                             // tp_mro
  nullptr,                             // tp_cache
  nullptr,                             // tp_subclasses
  nullptr,                             // tp_weaklist
  SVTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED };
// clang-format on
