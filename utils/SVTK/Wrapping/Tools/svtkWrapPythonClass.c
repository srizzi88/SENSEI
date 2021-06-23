/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapPythonClass.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkWrapPythonClass.h"
#include "svtkWrapPythonConstant.h"
#include "svtkWrapPythonEnum.h"
#include "svtkWrapPythonMethodDef.h"
#include "svtkWrapPythonTemplate.h"
#include "svtkWrapPythonType.h"

#include "svtkParseExtras.h"
#include "svtkWrap.h"
#include "svtkWrapText.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/* prototypes for the methods used by the python wrappers */

/* declare the exports and imports for a SVTK/Python class */
static void svtkWrapPython_ExportSVTKClass(FILE* fp, ClassInfo* data, HierarchyInfo* hinfo);

/* generate the New method for a svtkObjectBase object */
static void svtkWrapPython_GenerateObjectNew(
  FILE* fp, const char* classname, ClassInfo* data, HierarchyInfo* hinfo, int class_has_new);

/* -------------------------------------------------------------------- */
/* get the true superclass */
const char* svtkWrapPython_GetSuperClass(ClassInfo* data, HierarchyInfo* hinfo)
{
  int i;
  const char* supername = NULL;
  const char* name;
  const char** args;
  const char* defaults[2] = { NULL, NULL };
  char* cp;

  for (i = 0; i < data->NumberOfSuperClasses; i++)
  {
    supername = data->SuperClasses[i];

    if (strncmp(supername, "svtkTypeTemplate<", 16) == 0)
    {
      svtkParse_DecomposeTemplatedType(supername, &name, 2, &args, defaults);
      cp = (char*)malloc(strlen(args[1]) + 1);
      strcpy(cp, args[1]);
      svtkParse_FreeTemplateDecomposition(name, 2, args);
      supername = cp;
    }

    /* Add QSVTKInteractor as the sole exception: It is derived
     * from svtkObject but does not start with "svtk".  Given its
     * name, it would be expected to be derived from QObject. */
    if (svtkWrap_IsSVTKObjectBaseType(hinfo, data->Name) || strcmp(data->Name, "QSVTKInteractor") == 0)
    {
      if (svtkWrap_IsClassWrapped(hinfo, supername) && svtkWrap_IsSVTKObjectBaseType(hinfo, supername))
      {
        return supername;
      }
    }
    else if (svtkWrapPython_HasWrappedSuperClass(hinfo, data->Name, NULL))
    {
      return supername;
    }
  }

  return NULL;
}

/* -------------------------------------------------------------------- */
/* check whether the superclass of the specified class is wrapped */
const char* svtkWrapPython_HasWrappedSuperClass(
  HierarchyInfo* hinfo, const char* classname, int* is_external)
{
  HierarchyEntry* entry;
  const char* module;
  const char* name;
  const char* supername;
  const char* result = NULL;
  int depth = 0;

  if (is_external)
  {
    *is_external = 0;
  }

  if (!hinfo)
  {
    return result;
  }

  name = classname;
  entry = svtkParseHierarchy_FindEntry(hinfo, name);
  if (!entry)
  {
    return result;
  }

  module = entry->Module;
  while (entry->NumberOfSuperClasses == 1)
  {
    supername = svtkParseHierarchy_TemplatedSuperClass(entry, name, 0);
    if (name != classname)
    {
      free((char*)name);
    }
    name = supername;
    entry = svtkParseHierarchy_FindEntry(hinfo, name);
    if (!entry)
    {
      break;
    }

    /* check if superclass is in a different module */
    if (is_external && depth == 0 && strcmp(entry->Module, module) != 0)
    {
      *is_external = 1;
    }
    depth++;

    /* the order of these conditions is important */
    if (entry->IsTypedef)
    {
      break;
    }
    else if (strncmp(entry->Name, "svtk", 3) != 0)
    {
      break;
    }
    else
    {
      result = entry->Module;
      break;
    }
  }

  if (name != classname)
  {
    free((char*)name);
  }

  return result;
}

/* -------------------------------------------------------------------- */
/* Create the docstring for a class, and print it to fp */
void svtkWrapPython_ClassDoc(
  FILE* fp, FileInfo* file_info, ClassInfo* data, HierarchyInfo* hinfo, int is_svtkobject)
{
  char pythonname[1024];
  const char* supername;
  char* cp;
  const char* ccp = NULL;
  size_t i, n;
  size_t briefmax = 255;
  int j;
  char temp[500];
  char* comment;

  if (data == file_info->MainClass && file_info->NameComment)
  {
    /* use the old SVTK-style class description */
    fprintf(fp, "  \"%s\\n\"\n",
      svtkWrapText_QuoteString(svtkWrapText_FormatComment(file_info->NameComment, 70), 500));
  }
  else if (data->Comment)
  {
    strncpy(temp, data->Name, briefmax);
    temp[briefmax] = '\0';
    i = strlen(temp);
    temp[i++] = ' ';
    temp[i++] = '-';
    if (data->Comment[0] != ' ')
    {
      temp[i++] = ' ';
    }

    /* extract the brief comment, if present */
    ccp = data->Comment;
    while (i < briefmax && *ccp != '\0')
    {
      /* a blank line ends the brief comment */
      if (ccp[0] == '\n' && ccp[1] == '\n')
      {
        break;
      }
      /* fuzzy: capital letter or a new command on next line ends brief */
      if (ccp[0] == '\n' && ccp[1] == ' ' &&
        ((ccp[2] >= 'A' && ccp[2] <= 'Z') || ccp[2] == '@' || ccp[2] == '\\'))
      {
        break;
      }
      temp[i] = *ccp;
      /* a sentence-ending period ends the brief comment */
      if (ccp[0] == '.' && (ccp[1] == ' ' || ccp[1] == '\n'))
      {
        i++;
        ccp++;
        while (*ccp == ' ')
        {
          ccp++;
        }
        break;
      }
      ccp++;
      i++;
    }
    /* skip all blank lines */
    while (*ccp == '\n')
    {
      ccp++;
    }
    if (*ccp == '\0')
    {
      ccp = NULL;
    }

    temp[i] = '\0';
    fprintf(fp, "  \"%s\\n\"\n", svtkWrapText_QuoteString(svtkWrapText_FormatComment(temp, 70), 500));
  }
  else
  {
    fprintf(
      fp, "  \"%s - no description provided.\\n\\n\"\n", svtkWrapText_QuoteString(data->Name, 500));
  }

  /* only consider superclasses that are wrapped */
  supername = svtkWrapPython_GetSuperClass(data, hinfo);
  if (supername)
  {
    svtkWrapPython_PyTemplateName(supername, pythonname);
    fprintf(fp, "  \"Superclass: %s\\n\\n\"\n", svtkWrapText_QuoteString(pythonname, 500));
  }

  if (data == file_info->MainClass &&
    (file_info->Description || file_info->Caveats || file_info->SeeAlso))
  {
    n = 100;
    if (file_info->Description)
    {
      n += strlen(file_info->Description);
    }

    if (file_info->Caveats)
    {
      n += strlen(file_info->Caveats);
    }

    if (file_info->SeeAlso)
    {
      n += strlen(file_info->SeeAlso);
    }

    comment = (char*)malloc(n);
    cp = comment;
    *cp = '\0';

    if (file_info->Description)
    {
      strcpy(cp, file_info->Description);
      cp += strlen(cp);
      *cp++ = '\n';
      *cp++ = '\n';
      *cp = '\0';
    }

    if (file_info->Caveats)
    {
      sprintf(cp, ".SECTION Caveats\n\n");
      cp += strlen(cp);
      strcpy(cp, file_info->Caveats);
      cp += strlen(cp);
      *cp++ = '\n';
      *cp++ = '\n';
      *cp = '\0';
    }

    if (file_info->SeeAlso)
    {
      sprintf(cp, ".SECTION See Also\n\n");
      cp += strlen(cp);
      strcpy(cp, file_info->SeeAlso);
      cp += strlen(cp);
      *cp = '\0';
    }

    ccp = svtkWrapText_FormatComment(comment, 70);
    free(comment);
  }
  else if (ccp)
  {
    ccp = svtkWrapText_FormatComment(ccp, 70);
  }

  if (ccp)
  {
    i = 0;
    while (ccp[i] != '\0')
    {
      n = i;
      /* skip forward until newline */
      while (ccp[i] != '\0' && ccp[i] != '\n' && i - n < 400)
      {
        i++;
      }
      /* skip over consecutive newlines */
      while (ccp[i] == '\n' && i - n < 400)
      {
        i++;
      }

      strncpy(temp, &ccp[n], i - n);
      temp[i - n] = '\0';
      fprintf(
        fp, "  \"%s%s", svtkWrapText_QuoteString(temp, 500), ccp[i] == '\0' ? "\\n\"" : "\"\n");
    }
  }

  /* for special objects, add constructor signatures to the doc */
  if (!is_svtkobject && !data->Template && !data->IsAbstract)
  {
    for (j = 0; j < data->NumberOfFunctions; j++)
    {
      if (svtkWrapPython_MethodCheck(data, data->Functions[j], hinfo) &&
        svtkWrap_IsConstructor(data, data->Functions[j]))
      {
        fprintf(fp, "\n  \"%s\\n\"",
          svtkWrapText_FormatSignature(data->Functions[j]->Signature, 70, 2000));
      }
    }
  }
}

/* -------------------------------------------------------------------- */
/* Declare the exports and imports for a SVTK/Python class */
static void svtkWrapPython_ExportSVTKClass(FILE* fp, ClassInfo* data, HierarchyInfo* hinfo)
{
  char classname[1024];
  const char* supername;

  /* mangle the classname if necessary */
  svtkWrapText_PythonName(data->Name, classname);

  /* for svtkObjectBase objects: export New method for use by subclasses */
  fprintf(fp, "extern \"C\" { PyObject *Py%s_ClassNew(); }\n\n", classname);

  /* declare the New methods for all the superclasses */
  supername = svtkWrapPython_GetSuperClass(data, hinfo);
  if (supername)
  {
    svtkWrapText_PythonName(supername, classname);
    fprintf(fp,
      "#ifndef DECLARED_Py%s_ClassNew\n"
      "extern \"C\" { PyObject *Py%s_ClassNew(); }\n"
      "#define DECLARED_Py%s_ClassNew\n"
      "#endif\n",
      classname, classname, classname);
  }
}

/* -------------------------------------------------------------------- */
/* generate the New method for a svtkObjectBase object */
static void svtkWrapPython_GenerateObjectNew(
  FILE* fp, const char* classname, ClassInfo* data, HierarchyInfo* hinfo, int class_has_new)
{
  char superclassname[1024];
  const char* name;
  int is_external;
  int has_constants = 0;
  int has_enums = 0;
  int i;

  if (class_has_new)
  {
    fprintf(fp,
      "static svtkObjectBase *Py%s_StaticNew()\n"
      "{\n"
      "  return %s::New();\n"
      "}\n"
      "\n",
      classname, data->Name);
  }

  fprintf(fp,
    "PyObject *Py%s_ClassNew()\n"
    "{\n"
    "  PyTypeObject *pytype = PySVTKClass_Add(\n"
    "    &Py%s_Type, Py%s_Methods,\n",
    classname, classname, classname);

  if (strcmp(data->Name, classname) == 0)
  {
    fprintf(fp, "    \"%s\",\n", classname);
  }
  else
  {
    /* use of typeid() matches svtkTypeTemplate */
    fprintf(fp, "    typeid(%s).name(),\n", data->Name);
  }

  if (class_has_new)
  {
    fprintf(fp, " &Py%s_StaticNew);\n\n", classname);
  }
  else
  {
    fprintf(fp, " nullptr);\n\n");
  }

  /* if type is already ready, then return */
  fprintf(fp,
    "  if ((pytype->tp_flags & Py_TPFLAGS_READY) != 0)\n"
    "  {\n"
    "    return (PyObject *)pytype;\n"
    "  }\n\n");

  /* add any flags specific to this type */
  fprintf(fp,
    "#if !defined(SVTK_PY3K) && PY_VERSION_HEX >= 0x02060000\n"
    "  pytype->tp_flags |= Py_TPFLAGS_HAVE_NEWBUFFER;\n"
    "#endif\n\n");

  /* find the first superclass that is a SVTK class, create it first */
  name = svtkWrapPython_GetSuperClass(data, hinfo);
  if (name)
  {
    svtkWrapText_PythonName(name, superclassname);
    svtkWrapPython_HasWrappedSuperClass(hinfo, data->Name, &is_external);
    if (!is_external) /* superclass is in the same module */
    {
      fprintf(fp, "  pytype->tp_base = (PyTypeObject *)Py%s_ClassNew();\n\n", superclassname);
    }
    else /* superclass is in a different module */
    {
      fprintf(
        fp, "  pytype->tp_base = svtkPythonUtil::FindClassTypeObject(\"%s\");\n\n", superclassname);
    }
  }

  /* check if any constants need to be added to the class dict */
  for (i = 0; i < data->NumberOfConstants; i++)
  {
    if (data->Constants[i]->Access == SVTK_ACCESS_PUBLIC)
    {
      has_constants = 1;
      break;
    }
  }

  /* check if any enums need to be added to the class dict */
  for (i = 0; i < data->NumberOfEnums; i++)
  {
    if (data->Enums[i]->Access == SVTK_ACCESS_PUBLIC)
    {
      has_enums = 1;
      break;
    }
  }

  if (has_constants || has_enums)
  {
    fprintf(fp,
      "  PyObject *d = pytype->tp_dict;\n"
      "  PyObject *o;\n"
      "\n");
  }

  if (has_enums)
  {
    /* add any enum types defined in the class to its dict */
    svtkWrapPython_AddPublicEnumTypes(fp, "  ", "d", "o", data);
  }

  if (has_constants)
  {
    /* add any constants defined in the class to its dict */
    svtkWrapPython_AddPublicConstants(fp, "  ", "d", "o", data);
  }

  fprintf(fp,
    "  PyType_Ready(pytype);\n"
    "  return (PyObject *)pytype;\n"
    "}\n\n");
}

/* -------------------------------------------------------------------- */
/* write out the type object */
void svtkWrapPython_GenerateObjectType(FILE* fp, const char* module, const char* classname)
{
  /* Generate the TypeObject */
  fprintf(fp,
    "static PyTypeObject Py%s_Type = {\n"
    "  PyVarObject_HEAD_INIT(&PyType_Type, 0)\n"
    "  PYTHON_PACKAGE_SCOPE \"%s.%s\", // tp_name\n"
    "  sizeof(PySVTKObject), // tp_basicsize\n"
    "  0, // tp_itemsize\n"
    "  PySVTKObject_Delete, // tp_dealloc\n"
    "#if PY_VERSION_HEX >= 0x03080000\n"
    "  0, // tp_vectorcall_offset\n"
    "#else\n"
    "  nullptr, // tp_print\n"
    "#endif\n"
    "  nullptr, // tp_getattr\n"
    "  nullptr, // tp_setattr\n"
    "  nullptr, // tp_compare\n"
    "  PySVTKObject_Repr, // tp_repr\n",
    classname, module, classname);

  fprintf(fp,
    "  nullptr, // tp_as_number\n"
    "  nullptr, // tp_as_sequence\n"
    "  nullptr, // tp_as_mapping\n"
    "  nullptr, // tp_hash\n"
    "  nullptr, // tp_call\n"
    "  PySVTKObject_String, // tp_str\n");

  fprintf(fp,
    "  PyObject_GenericGetAttr, // tp_getattro\n"
    "  PyObject_GenericSetAttr, // tp_setattro\n"
    "  &PySVTKObject_AsBuffer, // tp_as_buffer\n"
    "  Py_TPFLAGS_DEFAULT|Py_TPFLAGS_HAVE_GC|Py_TPFLAGS_BASETYPE,"
    " // tp_flags\n"
    "  Py%s_Doc, // tp_doc\n"
    "  PySVTKObject_Traverse, // tp_traverse\n"
    "  nullptr, // tp_clear\n"
    "  nullptr, // tp_richcompare\n"
    "  offsetof(PySVTKObject, svtk_weakreflist), // tp_weaklistoffset\n",
    classname);

  if (strcmp(classname, "svtkCollection") == 0)
  {
    fprintf(fp,
      "  PysvtkCollection_Iter, // tp_iter\n"
      "  nullptr, // tp_iternext\n");
  }
  else
  {
    if (strcmp(classname, "svtkCollectionIterator") == 0)
    {
      fprintf(fp,
        "  PysvtkCollectionIterator_Iter, // tp_iter\n"
        "  PysvtkCollectionIterator_Next, // tp_iternext\n");
    }
    else
    {
      fprintf(fp,
        "  nullptr, // tp_iter\n"
        "  nullptr, // tp_iternext\n");
    }
  }
  fprintf(fp,
    "  nullptr, // tp_methods\n"
    "  nullptr, // tp_members\n"
    "  PySVTKObject_GetSet, // tp_getset\n"
    "  nullptr, // tp_base\n"
    "  nullptr, // tp_dict\n"
    "  nullptr, // tp_descr_get\n"
    "  nullptr, // tp_descr_set\n"
    "  offsetof(PySVTKObject, svtk_dict), // tp_dictoffset\n"
    "  nullptr, // tp_init\n"
    "  nullptr, // tp_alloc\n"
    "  PySVTKObject_New, // tp_new\n"
    "  PyObject_GC_Del, // tp_free\n"
    "  nullptr, // tp_is_gc\n");

  /* fields set by python itself */
  fprintf(fp,
    "  nullptr, // tp_bases\n"
    "  nullptr, // tp_mro\n"
    "  nullptr, // tp_cache\n"
    "  nullptr, // tp_subclasses\n"
    "  nullptr, // tp_weaklist\n");

  /* internal struct members */
  fprintf(fp,
    "  SVTK_WRAP_PYTHON_SUPPRESS_UNINITIALIZED\n"
    "};\n"
    "\n");
}

/* -------------------------------------------------------------------- */
/* Wrap one class */
int svtkWrapPython_WrapOneClass(FILE* fp, const char* module, const char* classname, ClassInfo* data,
  FileInfo* finfo, HierarchyInfo* hinfo, int is_svtkobject)
{
  int class_has_new = 0;
  int i;

  /* recursive handling of templated classes */
  if (data->Template)
  {
    return svtkWrapPython_WrapTemplatedClass(fp, data, finfo, hinfo);
  }

  /* verify wrappability */
  if (!is_svtkobject && !svtkWrapPython_IsSpecialTypeWrappable(data))
  {
    return 0;
  }

  /* declare items to be exported or imported */
  if (is_svtkobject)
  {
    svtkWrapPython_ExportSVTKClass(fp, data, hinfo);
  }

  /* the docstring for the class, as a static var ending in "Doc" */
  fprintf(fp, "\nstatic const char *Py%s_Doc =\n", classname);

  svtkWrapPython_ClassDoc(fp, finfo, data, hinfo, is_svtkobject);

  fprintf(fp, ";\n\n");

  /* check for New() function */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    FunctionInfo* func = data->Functions[i];

    if (func->Name && !func->IsExcluded && func->Access == SVTK_ACCESS_PUBLIC &&
      strcmp("New", func->Name) == 0 && func->NumberOfParameters == 0 &&
      !svtkWrap_IsInheritedMethod(data, func))
    {
      class_has_new = 1;
    }
  }

  /* create any enum types defined in the class */
  for (i = 0; i < data->NumberOfEnums; i++)
  {
    if (!data->Enums[i]->IsExcluded && data->Enums[i]->Access == SVTK_ACCESS_PUBLIC)
    {
      svtkWrapPython_GenerateEnumType(fp, module, classname, data->Enums[i]);
    }
  }

  /* now output all the methods are wrappable */
  svtkWrapPython_GenerateMethods(fp, classname, data, finfo, hinfo, is_svtkobject, 0);

  /* output the class initialization function for SVTK objects */
  if (is_svtkobject)
  {
    svtkWrapPython_GenerateObjectType(fp, module, classname);
    svtkWrapPython_GenerateObjectNew(fp, classname, data, hinfo, class_has_new);
  }

  /* output the class initialization function for special objects */
  else
  {
    svtkWrapPython_GenerateSpecialType(fp, module, classname, data, finfo, hinfo);
  }

  return 1;
}
