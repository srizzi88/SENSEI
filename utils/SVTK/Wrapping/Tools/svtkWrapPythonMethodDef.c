/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapPythonMethodDef.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkWrapPythonMethodDef.h"
#include "svtkWrapPythonClass.h"
#include "svtkWrapPythonMethod.h"
#include "svtkWrapPythonOverload.h"

#include "svtkParseExtras.h"
#include "svtkWrap.h"
#include "svtkWrapText.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/* prototypes for the methods used by the python wrappers */

/* output the MethodDef table for this class */
static void svtkWrapPython_ClassMethodDef(FILE* fp, const char* classname, ClassInfo* data,
  FunctionInfo** wrappedFunctions, int numberOfWrappedFunctions, int fnum);

/* print out any custom methods */
static void svtkWrapPython_CustomMethods(
  FILE* fp, const char* classname, ClassInfo* data, int do_constructors);

/* -------------------------------------------------------------------- */
/* prototypes for utility methods */

/* check for wrappability, flags may be SVTK_WRAP_ARG or SVTK_WRAP_RETURN */
static int svtkWrapPython_IsValueWrappable(
  ClassInfo* data, ValueInfo* val, HierarchyInfo* hinfo, int flags);

/* weed out methods that will never be called */
static void svtkWrapPython_RemovePrecededMethods(
  FunctionInfo* wrappedFunctions[], int numberWrapped, int fnum);

/* -------------------------------------------------------------------- */
/* Check for type precedence. Some method signatures will just never
 * be called because of the way python types map to C++ types.  If
 * we don't remove such methods, they can lead to ambiguities later.
 *
 * The precedence rule is the following:
 * The type closest to the native Python type wins.
 */

static void svtkWrapPython_RemovePrecededMethods(
  FunctionInfo* wrappedFunctions[], int numberOfWrappedFunctions, int fnum)
{
  FunctionInfo* theFunc = wrappedFunctions[fnum];
  const char* name = theFunc->Name;
  FunctionInfo* sig1;
  FunctionInfo* sig2;
  ValueInfo* val1;
  ValueInfo* val2;
  int dim1, dim2;
  int vote1 = 0;
  int vote2 = 0;
  int occ1, occ2;
  unsigned int baseType1, baseType2;
  unsigned int unsigned1, unsigned2;
  unsigned int indirect1, indirect2;
  int i, nargs1, nargs2;
  int argmatch, allmatch;

  if (!name)
  {
    return;
  }

  for (occ1 = fnum; occ1 < numberOfWrappedFunctions; occ1++)
  {
    sig1 = wrappedFunctions[occ1];
    nargs1 = svtkWrap_CountWrappedParameters(sig1);

    if (sig1->Name && strcmp(sig1->Name, name) == 0)
    {
      for (occ2 = occ1 + 1; occ2 < numberOfWrappedFunctions; occ2++)
      {
        sig2 = wrappedFunctions[occ2];
        nargs2 = svtkWrap_CountWrappedParameters(sig2);
        vote1 = 0;
        vote2 = 0;

        if (nargs2 == nargs1 && sig2->Name && strcmp(sig2->Name, name) == 0)
        {
          allmatch = 1;
          for (i = 0; i < nargs1; i++)
          {
            argmatch = 0;
            val1 = sig1->Parameters[i];
            val2 = sig2->Parameters[i];
            dim1 = (val1->NumberOfDimensions > 0
                ? val1->NumberOfDimensions
                : (svtkWrap_IsPODPointer(val1) || svtkWrap_IsArray(val1)));
            dim2 = (val2->NumberOfDimensions > 0
                ? val2->NumberOfDimensions
                : (svtkWrap_IsPODPointer(val2) || svtkWrap_IsArray(val2)));
            if (dim1 != dim2)
            {
              vote1 = 0;
              vote2 = 0;
              allmatch = 0;
              break;
            }
            else
            {
              baseType1 = (val1->Type & SVTK_PARSE_BASE_TYPE);
              baseType2 = (val2->Type & SVTK_PARSE_BASE_TYPE);

              unsigned1 = (baseType1 & SVTK_PARSE_UNSIGNED);
              unsigned2 = (baseType2 & SVTK_PARSE_UNSIGNED);

              baseType1 = (baseType1 & ~SVTK_PARSE_UNSIGNED);
              baseType2 = (baseType2 & ~SVTK_PARSE_UNSIGNED);

              indirect1 = (val1->Type & SVTK_PARSE_INDIRECT);
              indirect2 = (val2->Type & SVTK_PARSE_INDIRECT);

              if ((indirect1 == indirect2) && (unsigned1 == unsigned2) &&
                (baseType1 == baseType2) &&
                ((val1->Type & SVTK_PARSE_CONST) == (val2->Type & SVTK_PARSE_CONST)))
              {
                argmatch = 1;
              }
              /* double precedes float */
              else if ((indirect1 == indirect2) && (baseType1 == SVTK_PARSE_DOUBLE) &&
                (baseType2 == SVTK_PARSE_FLOAT))
              {
                if (!vote2)
                {
                  vote1 = 1;
                }
              }
              else if ((indirect1 == indirect2) && (baseType1 == SVTK_PARSE_FLOAT) &&
                (baseType2 == SVTK_PARSE_DOUBLE))
              {
                if (!vote1)
                {
                  vote2 = 1;
                }
              }
              /* unsigned char precedes signed char */
              else if ((indirect1 == indirect2) && ((baseType1 == SVTK_PARSE_CHAR) && unsigned1) &&
                (baseType2 == SVTK_PARSE_SIGNED_CHAR))
              {
                if (!vote2)
                {
                  vote1 = 1;
                }
              }
              else if ((indirect1 == indirect2) && (baseType1 == SVTK_PARSE_SIGNED_CHAR) &&
                ((baseType2 == SVTK_PARSE_CHAR) && unsigned2))
              {
                if (!vote1)
                {
                  vote2 = 1;
                }
              }
              /* signed precedes unsigned for everything but char */
              else if ((indirect1 == indirect2) && (baseType1 != SVTK_PARSE_CHAR) &&
                (baseType2 != SVTK_PARSE_CHAR) && (baseType1 == baseType2) &&
                (unsigned1 != unsigned2))
              {
                if (unsigned2 && !vote2)
                {
                  vote1 = 1;
                }
                else if (unsigned1 && !vote1)
                {
                  vote2 = 1;
                }
              }
              /* integer promotion precedence */
              else if ((indirect1 == indirect2) && (baseType1 == SVTK_PARSE_INT) &&
                ((baseType2 == SVTK_PARSE_SHORT) || (baseType2 == SVTK_PARSE_SIGNED_CHAR) ||
                  ((baseType2 == SVTK_PARSE_CHAR) && unsigned2)))
              {
                if (!vote2)
                {
                  vote1 = 1;
                }
              }
              else if ((indirect1 == indirect2) && (baseType2 == SVTK_PARSE_INT) &&
                ((baseType1 == SVTK_PARSE_SHORT) || (baseType1 == SVTK_PARSE_SIGNED_CHAR) ||
                  ((baseType1 == SVTK_PARSE_CHAR) && unsigned1)))
              {
                if (!vote1)
                {
                  vote2 = 1;
                }
              }
              /* a string method precedes a "char *" method */
              else if ((baseType2 == SVTK_PARSE_CHAR) && (indirect2 == SVTK_PARSE_POINTER) &&
                (baseType1 == SVTK_PARSE_STRING) &&
                ((indirect1 == SVTK_PARSE_REF) || (indirect1 == 0)))
              {
                if (!vote2)
                {
                  vote1 = 1;
                }
              }
              else if ((baseType1 == SVTK_PARSE_CHAR) && (indirect1 == SVTK_PARSE_POINTER) &&
                (baseType2 == SVTK_PARSE_STRING) &&
                ((indirect2 == SVTK_PARSE_REF) || (indirect2 == 0)))
              {
                if (!vote1)
                {
                  vote2 = 1;
                }
              }
              /* mismatch: both methods are allowed to live */
              else if ((baseType1 != baseType2) || (unsigned1 != unsigned2) ||
                (indirect1 != indirect2))
              {
                vote1 = 0;
                vote2 = 0;
                allmatch = 0;
                break;
              }
            }

            if (argmatch == 0)
            {
              allmatch = 0;
            }
          }

          /* if all args match, prefer the non-const method */
          if (allmatch)
          {
            if (sig1->IsConst)
            {
              vote2 = 1;
            }
            else if (sig2->IsConst)
            {
              vote1 = 1;
            }
          }
        }

        if (vote1)
        {
          sig2->Name = 0;
        }
        else if (vote2)
        {
          sig1->Name = 0;
          break;
        }

      } /* for (occ2 ... */
    }   /* if (sig1->Name ... */
  }     /* for (occ1 ... */
}

/* -------------------------------------------------------------------- */
/* Print out all the python methods that call the C++ class methods.
 * After they're all printed, a Py_MethodDef array that has function
 * pointers and documentation for each method is printed.  In other
 * words, this poorly named function is "the big one". */

void svtkWrapPython_GenerateMethods(FILE* fp, const char* classname, ClassInfo* data,
  FileInfo* finfo, HierarchyInfo* hinfo, int is_svtkobject, int do_constructors)
{
  int i;
  int fnum;
  int numberOfWrappedFunctions = 0;
  FunctionInfo** wrappedFunctions;
  FunctionInfo* theFunc;
  char* cp;
  const char* ccp;

  wrappedFunctions = (FunctionInfo**)malloc(data->NumberOfFunctions * sizeof(FunctionInfo*));

  /* output any custom methods */
  svtkWrapPython_CustomMethods(fp, classname, data, do_constructors);

  /* modify the arg count for svtkDataArray methods */
  svtkWrap_FindCountHints(data, finfo, hinfo);

  /* identify methods that create new instances of objects */
  svtkWrap_FindNewInstanceMethods(data, hinfo);

  /* go through all functions and see which are wrappable */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    theFunc = data->Functions[i];

    /* check for wrappability */
    if (svtkWrapPython_MethodCheck(data, theFunc, hinfo) && !theFunc->IsOperator &&
      !theFunc->Template && !svtkWrap_IsDestructor(data, theFunc) &&
      (!svtkWrap_IsConstructor(data, theFunc) == !do_constructors))
    {
      ccp = svtkWrapText_PythonSignature(theFunc);
      cp = (char*)malloc(strlen(ccp) + 1);
      strcpy(cp, ccp);
      theFunc->Signature = cp;
      wrappedFunctions[numberOfWrappedFunctions++] = theFunc;
    }
  }

  /* write out the wrapper for each function in the array */
  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
  {
    theFunc = wrappedFunctions[fnum];

    /* check for type precedence, don't need a "float" method if a
       "double" method exists */
    svtkWrapPython_RemovePrecededMethods(wrappedFunctions, numberOfWrappedFunctions, fnum);

    /* if theFunc wasn't removed, process all its signatures */
    if (theFunc->Name)
    {
      fprintf(fp, "\n");

      svtkWrapPython_GenerateOneMethod(fp, classname, data, hinfo, wrappedFunctions,
        numberOfWrappedFunctions, fnum, is_svtkobject, do_constructors);

    } /* is this method non NULL */
  }   /* loop over all methods */

  /* the method table for constructors is produced elsewhere */
  if (!do_constructors)
  {
    svtkWrapPython_ClassMethodDef(
      fp, classname, data, wrappedFunctions, numberOfWrappedFunctions, fnum);
  }

  free(wrappedFunctions);
}

/* -------------------------------------------------------------------- */
/* output the MethodDef table for this class */
static void svtkWrapPython_ClassMethodDef(FILE* fp, const char* classname, ClassInfo* data,
  FunctionInfo** wrappedFunctions, int numberOfWrappedFunctions, int fnum)
{
  /* output the method table, with pointers to each function defined above */
  fprintf(fp, "static PyMethodDef Py%s_Methods[] = {\n", classname);

  for (fnum = 0; fnum < numberOfWrappedFunctions; fnum++)
  {
    if (wrappedFunctions[fnum]->IsLegacy)
    {
      fprintf(fp, "#if !defined(SVTK_LEGACY_REMOVE)\n");
    }
    if (wrappedFunctions[fnum]->Name)
    {
      /* string literals must be under 2048 chars */
      size_t maxlen = 2040;
      const char* comment;
      const char* signatures;

      /* format the comment nicely to a 66 char width */
      signatures = svtkWrapText_FormatSignature(wrappedFunctions[fnum]->Signature, 66, maxlen - 32);
      comment = svtkWrapText_FormatComment(wrappedFunctions[fnum]->Comment, 66);
      comment = svtkWrapText_QuoteString(comment, maxlen - strlen(signatures));

      fprintf(fp, "  {\"%s\", Py%s_%s, METH_VARARGS,\n", wrappedFunctions[fnum]->Name, classname,
        wrappedFunctions[fnum]->Name);

      fprintf(fp, "   \"%s\\n\\n%s\"},\n", signatures, comment);
    }
    if (wrappedFunctions[fnum]->IsLegacy)
    {
      fprintf(fp, "#endif\n");
    }
  }

  /* svtkObject needs a special entry for AddObserver */
  if (strcmp("svtkObject", data->Name) == 0)
  {
    fprintf(fp,
      "  {\"AddObserver\",  Py%s_AddObserver, 1,\n"
      "   \"V.AddObserver(int, function) -> int\\nC++: unsigned long AddObserver(const char "
      "*event,\\n    svtkCommand *command, float priority=0.0f)\\n\\nAdd an event callback "
      "function(svtkObject, int) for an event type.\\nReturns a handle that can be used with "
      "RemoveEvent(int).\"},\n",
      classname);

    /* svtkObject needs a special entry for InvokeEvent */
    fprintf(fp,
      "{\"InvokeEvent\", PysvtkObject_InvokeEvent, METH_VARARGS,\n"
      "   \"V.InvokeEvent(int, void) -> int\\nC++: int InvokeEvent(unsigned long event, void "
      "*callData)\\nV.InvokeEvent(string, void) -> int\\nC++: int InvokeEvent(const char *event, "
      "void *callData)\\nV.InvokeEvent(int) -> int\\nC++: int InvokeEvent(unsigned long "
      "event)\\nV.InvokeEvent(string) -> int\\nC++: int InvokeEvent(const char *event)\\n\\nThis "
      "method invokes an event and return whether the event was\\naborted or not. If the event was "
      "aborted, the return value is 1,\\notherwise it is 0.\"\n},\n");
  }

  /* svtkObjectBase needs GetAddressAsString, UnRegister */
  else if (strcmp("svtkObjectBase", data->Name) == 0)
  {
    fprintf(fp,
      "  {\"GetAddressAsString\",  Py%s_GetAddressAsString, 1,\n"
      "   \"V.GetAddressAsString(string) -> string\\nC++: const char "
      "*GetAddressAsString()\\n\\nGet address of C++ object in format 'Addr=%%p' after casting "
      "to\\nthe specified type.  You can get the same information from o.__this__.\"},\n",
      classname);
    fprintf(fp,
      "  {\"Register\", Py%s_Register, 1,\n"
      "   \"V.Register(svtkObjectBase)\\nC++: virtual void Register(svtkObjectBase *o)\\n\\nIncrease "
      "the reference count by 1.\\n\"},\n"
      "  {\"UnRegister\", Py%s_UnRegister, 1,\n"
      "   \"V.UnRegister(svtkObjectBase)\\nC++: virtual void UnRegister(svtkObjectBase "
      "*o)\\n\\nDecrease the reference count (release by another object). This\\nhas the same "
      "effect as invoking Delete() (i.e., it reduces the\\nreference count by 1).\\n\"},\n",
      classname, classname);
  }

  /* python expects the method table to end with a "nullptr" entry */
  fprintf(fp,
    "  {nullptr, nullptr, 0, nullptr}\n"
    "};\n"
    "\n");
}

/* -------------------------------------------------------------------- */
/* Check an arg to see if it is wrappable */

static int svtkWrapPython_IsValueWrappable(
  ClassInfo* data, ValueInfo* val, HierarchyInfo* hinfo, int flags)
{
  static unsigned int wrappableTypes[] = { SVTK_PARSE_VOID, SVTK_PARSE_BOOL, SVTK_PARSE_FLOAT,
    SVTK_PARSE_DOUBLE, SVTK_PARSE_CHAR, SVTK_PARSE_UNSIGNED_CHAR, SVTK_PARSE_SIGNED_CHAR, SVTK_PARSE_INT,
    SVTK_PARSE_UNSIGNED_INT, SVTK_PARSE_SHORT, SVTK_PARSE_UNSIGNED_SHORT, SVTK_PARSE_LONG,
    SVTK_PARSE_UNSIGNED_LONG, SVTK_PARSE_SSIZE_T, SVTK_PARSE_SIZE_T, SVTK_PARSE_UNKNOWN,
    SVTK_PARSE_LONG_LONG, SVTK_PARSE_UNSIGNED_LONG_LONG, SVTK_PARSE_OBJECT, SVTK_PARSE_QOBJECT,
    SVTK_PARSE_STRING,
#ifndef SVTK_PYTHON_NO_UNICODE
    SVTK_PARSE_UNICODE_STRING,
#endif
    0 };

  const char* aClass;
  unsigned int baseType;
  int j;

  if ((flags & SVTK_WRAP_RETURN) != 0)
  {
    if (svtkWrap_IsVoid(val))
    {
      return 1;
    }

    if (svtkWrap_IsNArray(val))
    {
      return 0;
    }
  }

  /* wrap std::vector<T> (IsScalar means "not pointer or array") */
  if (svtkWrap_IsStdVector(val) && svtkWrap_IsScalar(val))
  {
    size_t l, n;
    const char* tname;
    const char** args;
    const char* defaults[2] = { NULL, "" };
    int wrappable = 0;
    svtkParse_DecomposeTemplatedType(val->Class, &tname, 2, &args, defaults);
    l = svtkParse_BasicTypeFromString(args[0], &baseType, &aClass, &n);
    /* check that type has no following '*', '[]', or '<>' decorators */
    if (args[0][l] == '\0')
    {
      if (baseType != SVTK_PARSE_UNKNOWN && baseType != SVTK_PARSE_OBJECT &&
        baseType != SVTK_PARSE_QOBJECT && baseType != SVTK_PARSE_CHAR)
      {
        for (j = 0; wrappableTypes[j] != 0; j++)
        {
          if (baseType == wrappableTypes[j])
          {
            wrappable = 1;
            break;
          }
        }
      }
    }
    svtkParse_FreeTemplateDecomposition(tname, 2, args);
    return wrappable;
  }

  aClass = val->Class;
  baseType = (val->Type & SVTK_PARSE_BASE_TYPE);

  /* go through all types that are indicated as wrappable */
  for (j = 0; wrappableTypes[j] != 0; j++)
  {
    if (baseType == wrappableTypes[j])
    {
      break;
    }
  }
  if (wrappableTypes[j] == 0)
  {
    return 0;
  }

  if (svtkWrap_IsRef(val) && !svtkWrap_IsScalar(val) && !svtkWrap_IsArray(val) &&
    !svtkWrap_IsPODPointer(val))
  {
    return 0;
  }

  if (svtkWrap_IsScalar(val))
  {
    if (svtkWrap_IsNumeric(val) || val->IsEnum || /* marked as enum in ImportExportEnumTypes */
      svtkWrap_IsEnumMember(data, val) || svtkWrap_IsString(val))
    {
      return 1;
    }
    if (svtkWrap_IsObject(val))
    {
      if (svtkWrap_IsSpecialType(hinfo, aClass) ||
        svtkWrapPython_HasWrappedSuperClass(hinfo, aClass, NULL))
      {
        return 1;
      }
    }
  }
  else if (svtkWrap_IsArray(val) || svtkWrap_IsNArray(val))
  {
    if (svtkWrap_IsNumeric(val))
    {
      return 1;
    }
  }
  else if (svtkWrap_IsPointer(val))
  {
    if (svtkWrap_IsCharPointer(val) || svtkWrap_IsVoidPointer(val) ||
      svtkWrap_IsZeroCopyPointer(val) || svtkWrap_IsPODPointer(val))
    {
      return 1;
    }
    if (svtkWrap_IsPythonObject(val))
    {
      return 1;
    }
    else if (svtkWrap_IsObject(val))
    {
      if (svtkWrap_IsSVTKObjectBaseType(hinfo, aClass))
      {
        return 1;
      }
    }
  }

  return 0;
}

/* -------------------------------------------------------------------- */
/* Check a method to see if it is wrappable in python */

int svtkWrapPython_MethodCheck(ClassInfo* data, FunctionInfo* currentFunction, HierarchyInfo* hinfo)
{
  int i, n;

  /* some functions will not get wrapped no matter what */
  if (currentFunction->IsExcluded || currentFunction->IsDeleted ||
    currentFunction->Access != SVTK_ACCESS_PUBLIC ||
    svtkWrap_IsInheritedMethod(data, currentFunction))
  {
    return 0;
  }

  /* new and delete are meaningless in wrapped languages */
  if (currentFunction->Name == 0 || strcmp("Register", currentFunction->Name) == 0 ||
    strcmp("UnRegister", currentFunction->Name) == 0 ||
    strcmp("Delete", currentFunction->Name) == 0 || strcmp("New", currentFunction->Name) == 0)
  {
    return 0;
  }

  /* function pointer arguments for callbacks */
  if (currentFunction->NumberOfParameters == 2 &&
    svtkWrap_IsVoidFunction(currentFunction->Parameters[0]) &&
    svtkWrap_IsVoidPointer(currentFunction->Parameters[1]) &&
    !svtkWrap_IsConst(currentFunction->Parameters[1]) &&
    svtkWrap_IsVoid(currentFunction->ReturnValue))
  {
    return 1;
  }

  n = svtkWrap_CountWrappedParameters(currentFunction);

  /* check to see if we can handle all the args */
  for (i = 0; i < n; i++)
  {
    if (!svtkWrapPython_IsValueWrappable(data, currentFunction->Parameters[i], hinfo, SVTK_WRAP_ARG))
    {
      return 0;
    }
  }

  /* check the return value */
  if (!svtkWrapPython_IsValueWrappable(data, currentFunction->ReturnValue, hinfo, SVTK_WRAP_RETURN))
  {
    return 0;
  }

  return 1;
}

/* -------------------------------------------------------------------- */
/* generate code for custom methods for some classes */
static void svtkWrapPython_CustomMethods(
  FILE* fp, const char* classname, ClassInfo* data, int do_constructors)
{
  int i;
  FunctionInfo* theFunc;

  /* the python svtkObject needs special hooks for observers */
  if (strcmp("svtkObject", data->Name) == 0 && do_constructors == 0)
  {
    /* Remove the original AddObserver method */
    for (i = 0; i < data->NumberOfFunctions; i++)
    {
      theFunc = data->Functions[i];

      if (strcmp(theFunc->Name, "AddObserver") == 0)
      {
        data->Functions[i]->Name = NULL;
      }
    }

    /* Add the AddObserver method to svtkObject. */
    fprintf(fp,
      "static PyObject *\n"
      "Py%s_AddObserver(PyObject *self, PyObject *args)\n"
      "{\n"
      "  svtkPythonArgs ap(self, args, \"AddObserver\");\n"
      "  svtkObjectBase *vp = ap.GetSelfPointer(self, args);\n"
      "  %s *op = static_cast<%s *>(vp);\n"
      "\n"
      "  const char *temp0s = nullptr;\n"
      "  int temp0i = 0;\n"
      "  PyObject *temp1 = nullptr;\n"
      "  float temp2 = 0.0f;\n"
      "  unsigned long tempr;\n"
      "  PyObject *result = nullptr;\n"
      "  int argtype = 0;\n"
      "\n",
      classname, data->Name, data->Name);

    fprintf(fp,
      "  if (op)\n"
      "  {\n"
      "    if (ap.CheckArgCount(2,3) &&\n"
      "        ap.GetValue(temp0i) &&\n"
      "        ap.GetFunction(temp1) &&\n"
      "        (ap.NoArgsLeft() || ap.GetValue(temp2)))\n"
      "    {\n"
      "      argtype = 1;\n"
      "    }\n"
      "  }\n"
      "\n"
      "  if (op && !argtype)\n"
      "  {\n"
      "    PyErr_Clear();\n"
      "    ap.Reset();\n"
      "\n"
      "    if (ap.CheckArgCount(2,3) &&\n"
      "        ap.GetValue(temp0s) &&\n"
      "        ap.GetFunction(temp1) &&\n"
      "        (ap.NoArgsLeft() || ap.GetValue(temp2)))\n"
      "    {\n"
      "      argtype = 2;\n"
      "    }\n"
      "  }\n"
      "\n");

    fprintf(fp,
      "  if (argtype)\n"
      "  {\n"
      "    svtkPythonCommand *cbc = svtkPythonCommand::New();\n"
      "    cbc->SetObject(temp1);\n"
      "    cbc->SetThreadState(PyThreadState_Get());\n"
      "\n"
      "    if (argtype == 1)\n"
      "    {\n"
      "      if (ap.IsBound())\n"
      "      {\n"
      "        tempr = op->AddObserver(temp0i, cbc, temp2);\n"
      "      }\n"
      "      else\n"
      "      {\n"
      "        tempr = op->%s::AddObserver(temp0i, cbc, temp2);\n"
      "      }\n"
      "    }\n"
      "    else\n"
      "    {\n"
      "      if (ap.IsBound())\n"
      "      {\n"
      "        tempr = op->AddObserver(temp0s, cbc, temp2);\n"
      "      }\n"
      "      else\n"
      "      {\n"
      "        tempr = op->%s::AddObserver(temp0s, cbc, temp2);\n"
      "      }\n"
      "    }\n"
      "    PySVTKObject_AddObserver(self, tempr);\n"
      "\n",
      data->Name, data->Name);

    fprintf(fp,
      "    cbc->Delete();\n"
      "\n"
      "    if (!ap.ErrorOccurred())\n"
      "    {\n"
      "      result = ap.BuildValue(tempr);\n"
      "    }\n"
      "  }\n"
      "\n"
      "  return result;\n"
      "}\n"
      "\n");

    /* the python svtkObject needs a special InvokeEvent to turn any
       calldata into an appropriately unwrapped void pointer */

    /* different types of callback data */

    int numCallBackTypes = 5;

    static const char* callBackTypeString[] = { "z", "", "i", "d", "V" };

    static const char* fullCallBackTypeString[] = { "z", "", "i", "d", "V *svtkObjectBase" };

    static const char* callBackTypeDecl[] = { "  const char *calldata = nullptr;\n", "",
      "  long calldata;\n", "  double calldata;\n", "  svtkObjectBase *calldata = nullptr;\n" };

    static const char* callBackReadArg[] = { " &&\n      ap.GetValue(calldata)", "",
      " &&\n      ap.GetValue(calldata)", " &&\n      ap.GetValue(calldata)",
      " &&\n      ap.GetSVTKObject(calldata, \"svtkObject\")" };

    static const char* methodCallSecondHalf[] = { ", const_cast<char *>(calldata)", "",
      ", &calldata", ", &calldata", ", calldata" };

    /* two ways to refer to an event */
    static const char* eventTypeString[] = { "L", "z" };
    static const char* eventTypeDecl[] = { "  unsigned long event;\n",
      "  const char *event = nullptr;\n" };

    int callBackIdx, eventIdx;

    /* Remove the original InvokeEvent method */
    for (i = 0; i < data->NumberOfFunctions; i++)
    {
      theFunc = data->Functions[i];

      if (theFunc->Name && strcmp(theFunc->Name, "InvokeEvent") == 0)
      {
        data->Functions[i]->Name = NULL;
      }
    }

    /* Add the InvokeEvent method to svtkObject. */
    fprintf(fp,
      "// This collection of methods that handle InvokeEvent are\n"
      "// generated by a special case in svtkWrapPythonMethodDef.c\n"
      "// The last characters of the method name indicate the type signature\n"
      "// of the overload they handle: for example, \"_zd\" indicates that\n"
      "// the event type is specified by string and the calldata is a double\n");

    for (callBackIdx = 0; callBackIdx < numCallBackTypes; callBackIdx++)
    {
      for (eventIdx = 0; eventIdx < 2; eventIdx++)
      {
        fprintf(fp,
          "static PyObject *\n"
          "PysvtkObject_InvokeEvent_%s%s(PyObject *self, PyObject *args)\n"
          "{\n"
          "  svtkPythonArgs ap(self, args, \"InvokeEvent\");\n"
          "  svtkObjectBase *vp = ap.GetSelfPointer(self, args);\n"
          "  svtkObject *op = static_cast<svtkObject *>(vp);\n"
          "\n"
          "%s%s"
          "  PyObject *result = nullptr;\n"
          "\n"
          "  if (op && ap.CheckArgCount(%d) &&\n"
          "      ap.GetValue(event)%s)\n"
          "  {\n"
          "    int tempr = op->InvokeEvent(event%s);\n"
          "\n"
          "    if (!ap.ErrorOccurred())\n"
          "    {\n"
          "      result = ap.BuildValue(tempr);\n"
          "    }\n"
          "  }\n"
          "  return result;\n"
          "}\n"
          "\n",
          eventTypeString[eventIdx], callBackTypeString[callBackIdx], eventTypeDecl[eventIdx],
          callBackTypeDecl[callBackIdx], 1 + (callBackReadArg[callBackIdx][0] != '\0'),
          callBackReadArg[callBackIdx], methodCallSecondHalf[callBackIdx]);
      }
    }
    fprintf(fp, "static PyMethodDef PysvtkObject_InvokeEvent_Methods[] = {\n");
    for (callBackIdx = 0; callBackIdx < numCallBackTypes; callBackIdx++)
    {
      for (eventIdx = 0; eventIdx < 2; eventIdx++)
      {
        fprintf(fp,
          "  {nullptr, PysvtkObject_InvokeEvent_%s%s, METH_VARARGS,\n"
          "   \"@%s%s\"},\n",
          eventTypeString[eventIdx], callBackTypeString[callBackIdx], eventTypeString[eventIdx],
          fullCallBackTypeString[callBackIdx]);
      }
    }

    fprintf(fp,
      "  {nullptr, nullptr, 0, nullptr}\n"
      "};\n"
      "\n"
      "static PyObject *\n"
      "PysvtkObject_InvokeEvent(PyObject *self, PyObject *args)\n"
      "{\n"
      "  PyMethodDef *methods = PysvtkObject_InvokeEvent_Methods;\n"
      "  int nargs = svtkPythonArgs::GetArgCount(self, args);\n"
      "\n"
      "  switch(nargs)\n"
      "  {\n"
      "    case 1:\n"
      "    case 2:\n"
      "      return svtkPythonOverload::CallMethod(methods, self, args);\n"
      "  }\n"
      "\n"
      "  svtkPythonArgs::ArgCountError(nargs, \"InvokeEvent\");\n"
      "  return nullptr;\n"
      "}\n");
  }

  /* the python svtkObjectBase needs a couple extra functions */
  if (strcmp("svtkObjectBase", data->Name) == 0 && do_constructors == 0)
  {
    /* remove the original methods, if they exist */
    for (i = 0; i < data->NumberOfFunctions; i++)
    {
      theFunc = data->Functions[i];

      if ((strcmp(theFunc->Name, "GetAddressAsString") == 0) ||
        (strcmp(theFunc->Name, "Register") == 0) || (strcmp(theFunc->Name, "UnRegister") == 0))
      {
        theFunc->Name = NULL;
      }
    }

    /* add the GetAddressAsString method to svtkObjectBase */
    fprintf(fp,
      "static PyObject *\n"
      "Py%s_GetAddressAsString(PyObject *self, PyObject *args)\n"
      "{\n"
      "  svtkPythonArgs ap(self, args, \"GetAddressAsString\");\n"
      "  svtkObjectBase *vp = ap.GetSelfPointer(self, args);\n"
      "  %s *op = static_cast<%s *>(vp);\n"
      "\n"
      "  const char *temp0;\n"
      "  char tempr[256];\n"
      "  PyObject *result = nullptr;\n"
      "\n"
      "  if (op && ap.CheckArgCount(1) &&\n"
      "      ap.GetValue(temp0))\n"
      "  {\n"
      "    sprintf(tempr, \"Addr=%%p\", static_cast<void*>(op));\n"
      "\n"
      "    result = ap.BuildValue(tempr);\n"
      "  }\n"
      "\n"
      "  return result;\n"
      "}\n"
      "\n",
      classname, data->Name, data->Name);

    /* Override the Register method to check whether to ignore Register */
    fprintf(fp,
      "static PyObject *\n"
      "Py%s_Register(PyObject *self, PyObject *args)\n"
      "{\n"
      "  svtkPythonArgs ap(self, args, \"Register\");\n"
      "  svtkObjectBase *vp = ap.GetSelfPointer(self, args);\n"
      "  %s *op = static_cast<%s *>(vp);\n"
      "\n"
      "  svtkObjectBase *temp0 = nullptr;\n"
      "  PyObject *result = nullptr;\n"
      "\n"
      "  if (op && ap.CheckArgCount(1) &&\n"
      "      ap.GetSVTKObject(temp0, \"svtkObjectBase\"))\n"
      "  {\n"
      "    if (!PySVTKObject_Check(self) ||\n"
      "        (PySVTKObject_GetFlags(self) & SVTK_PYTHON_IGNORE_UNREGISTER) == 0)\n"
      "    {\n"
      "      if (ap.IsBound())\n"
      "      {\n"
      "        op->Register(temp0);\n"
      "      }\n"
      "      else\n"
      "      {\n"
      "        op->%s::Register(temp0);\n"
      "      }\n"
      "    }\n"
      "\n"
      "    if (!ap.ErrorOccurred())\n"
      "    {\n"
      "      result = ap.BuildNone();\n"
      "    }\n"
      "  }\n"
      "\n"
      "  return result;\n"
      "}\n"
      "\n",
      classname, data->Name, data->Name, data->Name);

    /* Override the UnRegister method to check whether to ignore UnRegister */
    fprintf(fp,
      "static PyObject *\n"
      "Py%s_UnRegister(PyObject *self, PyObject *args)\n"
      "{\n"
      "  svtkPythonArgs ap(self, args, \"UnRegister\");\n"
      "  svtkObjectBase *vp = ap.GetSelfPointer(self, args);\n"
      "  %s *op = static_cast<%s *>(vp);\n"
      "\n"
      "  svtkObjectBase *temp0 = nullptr;\n"
      "  PyObject *result = nullptr;\n"
      "\n"
      "  if (op && ap.CheckArgCount(1) &&\n"
      "      ap.GetSVTKObject(temp0, \"svtkObjectBase\"))\n"
      "  {\n"
      "    if (!PySVTKObject_Check(self) ||\n"
      "        (PySVTKObject_GetFlags(self) & SVTK_PYTHON_IGNORE_UNREGISTER) == 0)\n"
      "    {\n"
      "      if (ap.IsBound())\n"
      "      {\n"
      "        op->UnRegister(temp0);\n"
      "      }\n"
      "      else\n"
      "      {\n"
      "        op->%s::UnRegister(temp0);\n"
      "      }\n"
      "    }\n"
      "\n"
      "    if (!ap.ErrorOccurred())\n"
      "    {\n"
      "      result = ap.BuildNone();\n"
      "    }\n"
      "  }\n"
      "\n"
      "  return result;\n"
      "}\n"
      "\n",
      classname, data->Name, data->Name, data->Name);
  }

  if (strcmp("svtkCollection", data->Name) == 0 && do_constructors == 0)
  {
    fprintf(fp,
      "static PyObject *\n"
      "PysvtkCollection_Iter(PyObject *self)\n"
      "{\n"
      "  PySVTKObject *vp = (PySVTKObject *)self;\n"
      "  svtkCollection *op = static_cast<svtkCollection *>(vp->svtk_ptr);\n"
      "\n"
      "  PyObject *result = nullptr;\n"
      "\n"
      "  if (op)\n"
      "  {\n"
      "    svtkCollectionIterator *tempr = op->NewIterator();\n"
      "    if (tempr != nullptr)\n"
      "    {\n"
      "      result = svtkPythonArgs::BuildSVTKObject(tempr);\n"
      "      PySVTKObject_GetObject(result)->UnRegister(nullptr);\n"
      "    }\n"
      "  }\n"
      "\n"
      "  return result;\n"
      "}\n");
  }

  if (strcmp("svtkCollectionIterator", data->Name) == 0 && do_constructors == 0)
  {
    fprintf(fp,
      "static PyObject *\n"
      "PysvtkCollectionIterator_Next(PyObject *self)\n"
      "{\n"
      "  PySVTKObject *vp = (PySVTKObject *)self;\n"
      "  svtkCollectionIterator *op = static_cast<svtkCollectionIterator*>(vp->svtk_ptr);\n"
      "\n"
      "  PyObject *result = nullptr;\n"
      "\n"
      "  if (op)\n"
      "  {\n"
      "    svtkObject *tempr = op->GetCurrentObject();\n"
      "    op->GoToNextItem();\n"
      "    if (tempr != nullptr)\n"
      "    {\n"
      "      result = svtkPythonArgs::BuildSVTKObject(tempr);\n"
      "    }\n"
      "  }\n"
      "\n"
      "  return result;\n"
      "}\n"
      "\n"
      "static PyObject *\n"
      "PysvtkCollectionIterator_Iter(PyObject *self)\n"
      "{\n"
      "  Py_INCREF(self);\n"
      "  return self;\n"
      "}\n");
  }
}
