/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapPythonTemplate.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkWrapPythonTemplate.h"
#include "svtkWrapPythonClass.h"
#include "svtkWrapPythonMethod.h"
#include "svtkWrapPythonType.h"

#include "svtkParseExtras.h"
#include "svtkWrapText.h"

#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* -------------------------------------------------------------------- */
/* convert a C++ templated type to pythonic dict form */
size_t svtkWrapPython_PyTemplateName(const char* name, char* pname)
{
  unsigned int ctype = 0;
  const char* ptype = NULL;
  size_t i, j, n, m;

  /* skip const, volatile qualifiers */
  for (;;)
  {
    if (strncmp(name, "const ", 6) == 0)
    {
      name += 6;
    }
    else if (strncmp(name, "volatile ", 9) == 0)
    {
      name += 9;
    }
    else
    {
      break;
    }
  }

  /* convert basic types to their SVTK_PARSE constants */
  n = svtkParse_BasicTypeFromString(name, &ctype, NULL, NULL);

  /* convert to pythonic equivalents (borrowed from numpy) */
  switch (ctype & SVTK_PARSE_BASE_TYPE)
  {
    case SVTK_PARSE_BOOL:
      ptype = "bool";
      break;
    case SVTK_PARSE_CHAR:
      ptype = "char";
      break;
    case SVTK_PARSE_SIGNED_CHAR:
      ptype = "int8";
      break;
    case SVTK_PARSE_UNSIGNED_CHAR:
      ptype = "uint8";
      break;
    case SVTK_PARSE_SHORT:
      ptype = "int16";
      break;
    case SVTK_PARSE_UNSIGNED_SHORT:
      ptype = "uint16";
      break;
    case SVTK_PARSE_INT:
      ptype = "int32";
      break;
    case SVTK_PARSE_UNSIGNED_INT:
      ptype = "uint32";
      break;
    case SVTK_PARSE_LONG:
      ptype = "int"; /* python int is C long */
      break;
    case SVTK_PARSE_UNSIGNED_LONG:
      ptype = "uint";
      break;
    case SVTK_PARSE_LONG_LONG:
    case SVTK_PARSE___INT64:
      ptype = "int64";
      break;
    case SVTK_PARSE_UNSIGNED_LONG_LONG:
    case SVTK_PARSE_UNSIGNED___INT64:
      ptype = "uint64";
      break;
    case SVTK_PARSE_FLOAT:
      ptype = "float32";
      break;
    case SVTK_PARSE_DOUBLE:
      ptype = "float64";
      break;
  }

  /* if type was a simple type, then we're done */
  if (ptype)
  {
    strcpy(pname, ptype);
    return n;
  }

  /* look for SVTK types that become common python types */
  if ((n == 12 && strncmp(name, "svtkStdString", n) == 0) ||
    (n == 11 && strncmp(name, "std::string", n) == 0))
  {
    strcpy(pname, "str");
    return n;
  }
  else if (n == 16 && strncmp(name, "svtkUnicodeString", n) == 0)
  {
    strcpy(pname, "unicode");
    return n;
  }

  /* check whether name is templated */
  for (i = 0; i < n; i++)
  {
    if (name[i] == '<')
    {
      break;
    }
  }

  strncpy(pname, name, i);

  if (name[i] != '<')
  {
    pname[i] = '\0';
    return i;
  }

  /* if templated, subst '[' for '<' */
  pname[i++] = '[';
  j = i;

  m = 1;
  while (i < n && m != 0 && name[i] != '>')
  {
    if (name[i] >= '0' && name[i] <= '9')
    {
      /* incomplete: only does decimal integers */
      do
      {
        pname[j++] = name[i++];
      } while (name[i] >= '0' && name[i] <= '9');
      while (name[i] == 'u' || name[i] == 'l' || name[i] == 'U' || name[i] == 'L')
      {
        i++;
      }
    }
    else
    {
      m = svtkWrapPython_PyTemplateName(&name[i], &pname[j]);
      i += m;
      j = strlen(pname);
    }
    while (name[i] == ' ' || name[i] == '\t')
    {
      i++;
    }
    if (name[i] == ',')
    {
      pname[j++] = name[i++];
    }
    while (name[i] == ' ' || name[i] == '\t')
    {
      i++;
    }
  }

  if (name[i] == '>')
  {
    i++;
    pname[j++] = ']';
  }

  pname[j] = '\0';

  return i;
}

/* -------------------------------------------------------------------- */
/* Wrap a templated class */
int svtkWrapPython_WrapTemplatedClass(
  FILE* fp, ClassInfo* data, FileInfo* file_info, HierarchyInfo* hinfo)
{
  char classname[1024];
  const char* instantiations[1024];
  int ninstantiations = 0;
  int i, j, k, nargs;
  ClassInfo* sdata;
  ValueInfo* tdef;
  HierarchyEntry* entry;
  const char* name;
  char* cp;
  const char** args;
  const char** defaults;
  const char* modulename;
  const char* name_with_args;
  int is_svtkobject = 0;
  const char** types;

  /* do not directly wrap svtkTypeTemplate */
  if (strcmp(data->Name, "svtkTypeTemplate") == 0)
  {
    return 0;
  }

  if (hinfo == 0)
  {
    return 0;
  }
  entry = svtkParseHierarchy_FindEntry(hinfo, data->Name);
  if (entry == 0)
  {
    return 0;
  }
  modulename = entry->Module;
  defaults = entry->TemplateDefaults;

  /* find all instantiations from derived classes */
  for (j = 0; j < hinfo->NumberOfEntries; j++)
  {
    entry = &hinfo->Entries[j];
    classname[0] = '\0';

    /* skip enum entries */
    if (entry->IsEnum)
    {
      continue;
    }
    /* look for typedefs of template instantiations */
    if (entry->IsTypedef)
    {
      tdef = entry->Typedef;
      if ((tdef->Type & SVTK_PARSE_BASE_TYPE) == SVTK_PARSE_OBJECT &&
        entry->NumberOfTemplateParameters == 0)
      {
        if (tdef->Class && tdef->Class[0] != '\0' && tdef->Class[strlen(tdef->Class) - 1] == '>')
        {
          strcpy(classname, tdef->Class);
          entry = svtkParseHierarchy_FindEntry(hinfo, classname);
        }
      }
      if (!entry || entry->IsTypedef || entry->IsEnum)
      {
        continue;
      }
    }

    nargs = entry->NumberOfTemplateParameters;
    args = entry->TemplateParameters;
    if (strcmp(entry->Module, modulename) == 0 &&
      (entry->NumberOfSuperClasses == 1 || strcmp(entry->Name, data->Name) == 0))
    {
      types = NULL;

      /* only do these classes directly */
      if (strcmp(entry->Name, "svtkArrayIteratorTemplate") == 0 ||
        strcmp(entry->Name, "svtkDenseArray") == 0 || strcmp(entry->Name, "svtkSparseArray") == 0)
      {
        types = svtkParse_GetArrayTypes();
      }
      else if (strcmp(entry->Name, "svtkSOADataArrayTemplate") == 0 ||
        strcmp(entry->Name, "svtkScaledSOADataArrayTemplate") == 0)
      {
        types = svtkParse_GetTemplateMacroTypes();
      }
      else if (strcmp(entry->Name, "svtkTuple") == 0)
      {
        static const char* tuple_types[13] = { "unsigned char, 2", "unsigned char, 3",
          "unsigned char, 4", "int, 2", "int, 3", "int, 4", "float, 2", "float, 3", "float, 4",
          "double, 2", "double, 3", "double, 4", NULL };
        types = tuple_types;
      }
      /* do all other templated classes indirectly */
      else if (nargs > 0)
      {
        continue;
      }

      for (i = 0; i == 0 || (types && types[i] != NULL); i++)
      {
        /* make the classname, with template args if present */
        if (classname[0] == '\0')
        {
          if (nargs == 0)
          {
            sprintf(classname, "%s", entry->Name);
          }
          else
          {
            sprintf(classname, "%s<%s>", entry->Name, types[i]);
          }
        }

        name_with_args = NULL;
        if (strcmp(data->Name, entry->Name) == 0)
        {
          /* entry is the same as data */
          cp = (char*)malloc(strlen(classname) + 1);
          strcpy(cp, classname);
          name_with_args = cp;
        }
        else
        {
          /* entry is not data, see if it is a subclass, and if so,
           * what template args of 'data' it corresponds to */
          svtkParseHierarchy_IsTypeOfTemplated(hinfo, entry, classname, data->Name, &name_with_args);
        }

        if (name_with_args)
        {
          /* append to the list of instantiations if not present yet */
          for (k = 0; k < ninstantiations; k++)
          {
            if (strcmp(name_with_args, instantiations[k]) == 0)
            {
              break;
            }
          }
          if (k == ninstantiations)
          {
            instantiations[ninstantiations++] = name_with_args;
          }
          else
          {
            free((char*)name_with_args);
          }
        }

        classname[0] = '\0';
      }
    }
  }

  if (ninstantiations)
  {
    for (k = 0; k < ninstantiations; k++)
    {
      entry = svtkParseHierarchy_FindEntry(hinfo, instantiations[k]);
      is_svtkobject =
        svtkParseHierarchy_IsTypeOfTemplated(hinfo, entry, instantiations[k], "svtkObjectBase", NULL);

      nargs = data->Template->NumberOfParameters;
      svtkParse_DecomposeTemplatedType(instantiations[k], &name, nargs, &args, defaults);

      sdata = (ClassInfo*)malloc(sizeof(ClassInfo));
      svtkParse_CopyClass(sdata, data);
      svtkParse_InstantiateClassTemplate(sdata, file_info->Strings, nargs, args);
      svtkWrapText_PythonName(instantiations[k], classname);

      svtkWrapPython_WrapOneClass(fp, modulename, classname, sdata, file_info, hinfo, is_svtkobject);

      svtkParse_FreeClass(sdata);
      svtkParse_FreeTemplateDecomposition(name, nargs, args);
    }

    /* the docstring for the templated class */
    fprintf(fp, "static const char *Py%s_Doc =\n", data->Name);

    svtkWrapPython_ClassDoc(fp, file_info, data, hinfo, is_svtkobject);

    fprintf(fp, "\n  \"\\nProvided Types:\\n\\n\"");

    for (k = 0; k < ninstantiations; k++)
    {
      svtkWrapPython_PyTemplateName(instantiations[k], classname);
      fprintf(fp, "\n  \"  %s => %s\\n\"\n", classname, instantiations[k]);
    }

    fprintf(fp, ";\n\n");

    fprintf(fp,
      "PyObject *Py%s_TemplateNew()\n"
      "{\n"
      "  PyObject *o;\n"
      "\n"
      "  PyObject *temp = PySVTKTemplate_New(\"%sPython.%s\",\n"
      "                                     Py%s_Doc);\n"
      "\n",
      data->Name, modulename, data->Name, data->Name);

    for (k = 0; k < ninstantiations; k++)
    {
      svtkWrapText_PythonName(instantiations[k], classname);

      entry = svtkParseHierarchy_FindEntry(hinfo, instantiations[k]);
      if (svtkParseHierarchy_IsTypeOfTemplated(
            hinfo, entry, instantiations[k], "svtkObjectBase", NULL))
      {
        fprintf(fp, "  o = Py%s_ClassNew();\n", classname);
      }
      else
      {
        fprintf(fp, "  o = Py%s_TypeNew();\n", classname);
      }

      fprintf(fp,
        "  if (o && PySVTKTemplate_AddItem(temp, o) != 0)\n"
        "  {\n"
        "    Py_DECREF(o);\n"
        "  }\n"
        "\n");

      free((char*)instantiations[k]);
    }

    fprintf(fp,
      "  return temp;\n"
      "}\n"
      "\n");

    return 1;
  }

  return 0;
}
