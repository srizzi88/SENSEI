/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParseJavaBeans.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#include "svtkParse.h"
#include "svtkParseHierarchy.h"
#include "svtkParseMain.h"
#include <stdio.h>
#include <string.h>

HierarchyInfo* hierarchyInfo = NULL;
int numberOfWrappedFunctions = 0;
FunctionInfo* wrappedFunctions[1000];
extern FunctionInfo* currentFunction;

void output_temp(FILE* fp, int i)
{
  unsigned int aType = (currentFunction->ArgTypes[i] & SVTK_PARSE_UNQUALIFIED_TYPE);

  /* ignore void */
  if (aType == SVTK_PARSE_VOID)
  {
    return;
  }

  if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
  {
    fprintf(fp, "Object id0, String id1");
    return;
  }

  if ((aType == SVTK_PARSE_CHAR_PTR) || (aType == SVTK_PARSE_STRING) ||
    (aType == SVTK_PARSE_STRING_REF))
  {
    fprintf(fp, "String ");
  }
  else
  {
    switch ((aType & SVTK_PARSE_BASE_TYPE) & ~SVTK_PARSE_UNSIGNED)
    {
      case SVTK_PARSE_FLOAT:
        fprintf(fp, "double ");
        break;
      case SVTK_PARSE_DOUBLE:
        fprintf(fp, "double ");
        break;
      case SVTK_PARSE_INT:
        fprintf(fp, "int ");
        break;
      case SVTK_PARSE_SHORT:
        fprintf(fp, "int ");
        break;
      case SVTK_PARSE_LONG:
        fprintf(fp, "int ");
        break;
      case SVTK_PARSE_LONG_LONG:
        fprintf(fp, "int ");
        break;
      case SVTK_PARSE___INT64:
        fprintf(fp, "int ");
        break;
      case SVTK_PARSE_VOID:
        fprintf(fp, "void ");
        break;
      case SVTK_PARSE_SIGNED_CHAR:
        fprintf(fp, "char ");
        break;
      case SVTK_PARSE_CHAR:
        fprintf(fp, "char ");
        break;
      case SVTK_PARSE_OBJECT:
        fprintf(fp, "%s ", currentFunction->ArgClasses[i]);
        break;
      case SVTK_PARSE_UNKNOWN:
        return;
    }
  }

  fprintf(fp, "id%i", i);
  if (((aType & SVTK_PARSE_INDIRECT) == SVTK_PARSE_POINTER) && (aType != SVTK_PARSE_CHAR_PTR) &&
    (aType != SVTK_PARSE_OBJECT_PTR))
  {
    fprintf(fp, "[]");
  }
}

void return_result(FILE* fp)
{
  switch (currentFunction->ReturnType & SVTK_PARSE_UNQUALIFIED_TYPE)
  {
    case SVTK_PARSE_FLOAT:
      fprintf(fp, "double ");
      break;
    case SVTK_PARSE_VOID:
      fprintf(fp, "void ");
      break;
    case SVTK_PARSE_CHAR:
      fprintf(fp, "char ");
      break;
    case SVTK_PARSE_DOUBLE:
      fprintf(fp, "double ");
      break;
    case SVTK_PARSE_INT:
    case SVTK_PARSE_SHORT:
    case SVTK_PARSE_LONG:
    case SVTK_PARSE_LONG_LONG:
    case SVTK_PARSE___INT64:
    case SVTK_PARSE_UNSIGNED_CHAR:
    case SVTK_PARSE_UNSIGNED_INT:
    case SVTK_PARSE_UNSIGNED_SHORT:
    case SVTK_PARSE_UNSIGNED_LONG:
    case SVTK_PARSE_UNSIGNED_LONG_LONG:
    case SVTK_PARSE_UNSIGNED___INT64:
      fprintf(fp, "int ");
      break;
    case SVTK_PARSE_CHAR_PTR:
    case SVTK_PARSE_STRING:
    case SVTK_PARSE_STRING_REF:
      fprintf(fp, "String ");
      break;
    case SVTK_PARSE_OBJECT_PTR:
      fprintf(fp, "%s ", currentFunction->ReturnClass);
      break;

      /* handle functions returning vectors */
      /* this is done by looking them up in a hint file */
    case SVTK_PARSE_FLOAT_PTR:
    case SVTK_PARSE_DOUBLE_PTR:
      fprintf(fp, "double[] ");
      break;
    case SVTK_PARSE_INT_PTR:
    case SVTK_PARSE_SHORT_PTR:
    case SVTK_PARSE_LONG_PTR:
    case SVTK_PARSE_LONG_LONG_PTR:
    case SVTK_PARSE___INT64_PTR:
    case SVTK_PARSE_SIGNED_CHAR_PTR:
    case SVTK_PARSE_UNSIGNED_CHAR_PTR:
    case SVTK_PARSE_UNSIGNED_INT_PTR:
    case SVTK_PARSE_UNSIGNED_SHORT_PTR:
    case SVTK_PARSE_UNSIGNED_LONG_PTR:
    case SVTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case SVTK_PARSE_UNSIGNED___INT64_PTR:
      fprintf(fp, "int[]  ");
      break;
  }
}

/* Check to see if two types will map to the same Java type,
 * return 1 if type1 should take precedence,
 * return 2 if type2 should take precedence,
 * return 0 if the types do not map to the same type */
static int CheckMatch(unsigned int type1, unsigned int type2, const char* c1, const char* c2)
{
  static unsigned int floatTypes[] = { SVTK_PARSE_DOUBLE, SVTK_PARSE_FLOAT, 0 };

  static unsigned int intTypes[] = { SVTK_PARSE_UNSIGNED_LONG_LONG, SVTK_PARSE_UNSIGNED___INT64,
    SVTK_PARSE_LONG_LONG, SVTK_PARSE___INT64, SVTK_PARSE_UNSIGNED_LONG, SVTK_PARSE_LONG,
    SVTK_PARSE_UNSIGNED_INT, SVTK_PARSE_INT, SVTK_PARSE_UNSIGNED_SHORT, SVTK_PARSE_SHORT,
    SVTK_PARSE_UNSIGNED_CHAR, SVTK_PARSE_SIGNED_CHAR, 0 };

  static unsigned int stringTypes[] = { SVTK_PARSE_CHAR_PTR, SVTK_PARSE_STRING_REF, SVTK_PARSE_STRING,
    0 };

  static unsigned int* numericTypes[] = { floatTypes, intTypes, 0 };

  int i, j;
  int hit1, hit2;

  if ((type1 & SVTK_PARSE_UNQUALIFIED_TYPE) == (type2 & SVTK_PARSE_UNQUALIFIED_TYPE))
  {
    if ((type1 & SVTK_PARSE_BASE_TYPE) == SVTK_PARSE_OBJECT)
    {
      if (strcmp(c1, c2) == 0)
      {
        return 1;
      }
      return 0;
    }
    else
    {
      return 1;
    }
  }

  for (i = 0; numericTypes[i]; i++)
  {
    hit1 = 0;
    hit2 = 0;
    for (j = 0; numericTypes[i][j]; j++)
    {
      if ((type1 & SVTK_PARSE_BASE_TYPE) == numericTypes[i][j])
      {
        hit1 = j + 1;
      }
      if ((type2 & SVTK_PARSE_BASE_TYPE) == numericTypes[i][j])
      {
        hit2 = j + 1;
      }
    }
    if (hit1 && hit2 && (type1 & SVTK_PARSE_INDIRECT) == (type2 & SVTK_PARSE_INDIRECT))
    {
      if (hit1 < hit2)
      {
        return 1;
      }
      else
      {
        return 2;
      }
    }
  }

  hit1 = 0;
  hit2 = 0;
  for (j = 0; stringTypes[j]; j++)
  {
    if ((type1 & SVTK_PARSE_UNQUALIFIED_TYPE) == stringTypes[j])
    {
      hit1 = j + 1;
    }
    if ((type2 & SVTK_PARSE_UNQUALIFIED_TYPE) == stringTypes[j])
    {
      hit2 = j + 1;
    }
  }
  if (hit1 && hit2)
  {
    if (hit1 < hit2)
    {
      return 1;
    }
    else
    {
      return 2;
    }
  }

  return 0;
}

/* have we done one of these yet */
int DoneOne()
{
  int i, j;
  int match;
  FunctionInfo* fi;

  for (i = 0; i < numberOfWrappedFunctions; i++)
  {
    fi = wrappedFunctions[i];

    if ((!strcmp(fi->Name, currentFunction->Name)) &&
      (fi->NumberOfArguments == currentFunction->NumberOfArguments))
    {
      match = 1;
      for (j = 0; j < fi->NumberOfArguments; j++)
      {
        if (!CheckMatch(currentFunction->ArgTypes[j], fi->ArgTypes[j],
              currentFunction->ArgClasses[j], fi->ArgClasses[j]))
        {
          match = 0;
        }
      }
      if (!CheckMatch(currentFunction->ReturnType, fi->ReturnType, currentFunction->ReturnClass,
            fi->ReturnClass))
      {
        match = 0;
      }
      if (match)
        return 1;
    }
  }
  return 0;
}

static int isClassWrapped(const char* classname)
{
  HierarchyEntry* entry;

  if (hierarchyInfo)
  {
    entry = svtkParseHierarchy_FindEntry(hierarchyInfo, classname);

    if (entry == 0 || svtkParseHierarchy_GetProperty(entry, "WRAPEXCLUDE") ||
      !svtkParseHierarchy_IsTypeOf(hierarchyInfo, entry, "svtkObjectBase"))
    {
      return 0;
    }

    /* Only the primary class in the header is wrapped in Java */
    return svtkParseHierarchy_IsPrimary(entry);
  }

  return 1;
}

int checkFunctionSignature(ClassInfo* data)
{
  static unsigned int supported_types[] = { SVTK_PARSE_VOID, SVTK_PARSE_BOOL, SVTK_PARSE_FLOAT,
    SVTK_PARSE_DOUBLE, SVTK_PARSE_CHAR, SVTK_PARSE_UNSIGNED_CHAR, SVTK_PARSE_SIGNED_CHAR, SVTK_PARSE_INT,
    SVTK_PARSE_UNSIGNED_INT, SVTK_PARSE_SHORT, SVTK_PARSE_UNSIGNED_SHORT, SVTK_PARSE_LONG,
    SVTK_PARSE_UNSIGNED_LONG, SVTK_PARSE_LONG_LONG, SVTK_PARSE_UNSIGNED_LONG_LONG, SVTK_PARSE___INT64,
    SVTK_PARSE_UNSIGNED___INT64, SVTK_PARSE_OBJECT, SVTK_PARSE_STRING, 0 };

  int i, j;
  int args_ok = 1;
  unsigned int rType = (currentFunction->ReturnType & SVTK_PARSE_UNQUALIFIED_TYPE);
  unsigned int aType = 0;
  unsigned int baseType = 0;

  /* some functions will not get wrapped no matter what else */
  if (currentFunction->IsOperator || currentFunction->ArrayFailure || currentFunction->IsExcluded ||
    currentFunction->IsDeleted || !currentFunction->IsPublic || !currentFunction->Name)
  {
    return 0;
  }

  /* NewInstance and SafeDownCast can not be wrapped because it is a
     (non-virtual) method which returns a pointer of the same type as
     the current pointer. Since all methods are virtual in Java, this
     looks like polymorphic return type.  */
  if (!strcmp("NewInstance", currentFunction->Name))
  {
    return 0;
  }

  if (!strcmp("SafeDownCast", currentFunction->Name))
  {
    return 0;
  }

  /* The GetInput() in svtkMapper cannot be overridden with a
   * different return type, Java doesn't allow this */
  if (strcmp(data->Name, "svtkMapper") == 0 && strcmp(currentFunction->Name, "GetInput") == 0)
  {
    return 0;
  }

  /* function pointer arguments for callbacks */
  if (currentFunction->NumberOfArguments == 2 &&
    currentFunction->ArgTypes[0] == SVTK_PARSE_FUNCTION &&
    currentFunction->ArgTypes[1] == SVTK_PARSE_VOID_PTR && rType == SVTK_PARSE_VOID)
  {
    return 1;
  }

  /* check to see if we can handle the args */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
  {
    aType = (currentFunction->ArgTypes[i] & SVTK_PARSE_UNQUALIFIED_TYPE);
    baseType = (aType & SVTK_PARSE_BASE_TYPE);

    for (j = 0; supported_types[j] != 0; j++)
    {
      if (baseType == supported_types[j])
      {
        break;
      }
    }
    if (supported_types[j] == 0)
    {
      args_ok = 0;
    }

    if (baseType == SVTK_PARSE_OBJECT)
    {
      if ((aType & SVTK_PARSE_INDIRECT) != SVTK_PARSE_POINTER)
      {
        args_ok = 0;
      }
      else if (!isClassWrapped(currentFunction->ArgClasses[i]))
      {
        args_ok = 0;
      }
    }

    if (aType == SVTK_PARSE_OBJECT)
      args_ok = 0;
    if (((aType & SVTK_PARSE_INDIRECT) != SVTK_PARSE_POINTER) &&
      ((aType & SVTK_PARSE_INDIRECT) != 0) && (aType != SVTK_PARSE_STRING_REF))
      args_ok = 0;
    if (aType == SVTK_PARSE_STRING_PTR)
      args_ok = 0;
    if (aType == SVTK_PARSE_UNSIGNED_CHAR_PTR)
      args_ok = 0;
    if (aType == SVTK_PARSE_UNSIGNED_INT_PTR)
      args_ok = 0;
    if (aType == SVTK_PARSE_UNSIGNED_SHORT_PTR)
      args_ok = 0;
    if (aType == SVTK_PARSE_UNSIGNED_LONG_PTR)
      args_ok = 0;
    if (aType == SVTK_PARSE_UNSIGNED_LONG_LONG_PTR)
      args_ok = 0;
    if (aType == SVTK_PARSE_UNSIGNED___INT64_PTR)
      args_ok = 0;
  }

  baseType = (rType & SVTK_PARSE_BASE_TYPE);

  for (j = 0; supported_types[j] != 0; j++)
  {
    if (baseType == supported_types[j])
    {
      break;
    }
  }
  if (supported_types[j] == 0)
  {
    args_ok = 0;
  }

  if (baseType == SVTK_PARSE_OBJECT)
  {
    if ((rType & SVTK_PARSE_INDIRECT) != SVTK_PARSE_POINTER)
    {
      args_ok = 0;
    }
    else if (!isClassWrapped(currentFunction->ReturnClass))
    {
      args_ok = 0;
    }
  }

  if (((rType & SVTK_PARSE_INDIRECT) != SVTK_PARSE_POINTER) && ((rType & SVTK_PARSE_INDIRECT) != 0) &&
    (rType != SVTK_PARSE_STRING_REF))
    args_ok = 0;
  if (rType == SVTK_PARSE_STRING_PTR)
    args_ok = 0;

  /* eliminate unsigned char * and unsigned short * */
  if (rType == SVTK_PARSE_UNSIGNED_INT_PTR)
    args_ok = 0;
  if (rType == SVTK_PARSE_UNSIGNED_SHORT_PTR)
    args_ok = 0;
  if (rType == SVTK_PARSE_UNSIGNED_LONG_PTR)
    args_ok = 0;
  if (rType == SVTK_PARSE_UNSIGNED_LONG_LONG_PTR)
    args_ok = 0;
  if (rType == SVTK_PARSE_UNSIGNED___INT64_PTR)
    args_ok = 0;

  /* make sure we have all the info we need for array arguments in */
  for (i = 0; i < currentFunction->NumberOfArguments; i++)
  {
    aType = (currentFunction->ArgTypes[i] & SVTK_PARSE_UNQUALIFIED_TYPE);

    if (((aType & SVTK_PARSE_INDIRECT) == SVTK_PARSE_POINTER) &&
      (currentFunction->ArgCounts[i] <= 0) && (aType != SVTK_PARSE_OBJECT_PTR) &&
      (aType != SVTK_PARSE_CHAR_PTR))
      args_ok = 0;
  }

  /* if we need a return type hint make sure we have one */
  switch (rType)
  {
    case SVTK_PARSE_FLOAT_PTR:
    case SVTK_PARSE_VOID_PTR:
    case SVTK_PARSE_DOUBLE_PTR:
    case SVTK_PARSE_INT_PTR:
    case SVTK_PARSE_SHORT_PTR:
    case SVTK_PARSE_LONG_PTR:
    case SVTK_PARSE_LONG_LONG_PTR:
    case SVTK_PARSE___INT64_PTR:
    case SVTK_PARSE_SIGNED_CHAR_PTR:
    case SVTK_PARSE_BOOL_PTR:
    case SVTK_PARSE_UNSIGNED_CHAR_PTR:
      args_ok = currentFunction->HaveHint;
      break;
  }

  /* make sure there isn't a Java-specific override */
  if (!strcmp("svtkObject", data->Name))
  {
    /* remove the original svtkCommand observer methods */
    if (!strcmp(currentFunction->Name, "AddObserver") ||
      !strcmp(currentFunction->Name, "GetCommand") ||
      (!strcmp(currentFunction->Name, "RemoveObserver") &&
        (currentFunction->ArgTypes[0] != SVTK_PARSE_UNSIGNED_LONG)) ||
      ((!strcmp(currentFunction->Name, "RemoveObservers") ||
         !strcmp(currentFunction->Name, "HasObserver")) &&
        (((currentFunction->ArgTypes[0] != SVTK_PARSE_UNSIGNED_LONG) &&
           (currentFunction->ArgTypes[0] != (SVTK_PARSE_CHAR_PTR | SVTK_PARSE_CONST))) ||
          (currentFunction->NumberOfArguments > 1))) ||
      (!strcmp(currentFunction->Name, "RemoveAllObservers") &&
        (currentFunction->NumberOfArguments > 0)))
    {
      args_ok = 0;
    }
  }
  else if (!strcmp("svtkObjectBase", data->Name))
  {
    /* remove the special svtkObjectBase methods */
    if (!strcmp(currentFunction->Name, "Print"))
    {
      args_ok = 0;
    }
  }

  /* make sure it isn't a Delete or New function */
  if (!strcmp("Delete", currentFunction->Name) || !strcmp("New", currentFunction->Name))
  {
    args_ok = 0;
  }

  return args_ok;
}

void outputFunction(FILE* fp, ClassInfo* data)
{
  unsigned int rType = (currentFunction->ReturnType & SVTK_PARSE_UNQUALIFIED_TYPE);
  unsigned int aType = 0;
  int i;
  /* beans */
  char* beanfunc;

  /* make the first letter lowercase for set get methods */
  beanfunc = strdup(currentFunction->Name);
  if (isupper(beanfunc[0]))
    beanfunc[0] = beanfunc[0] + 32;

  args_ok = checkFunctionSignature(data);

  if (!currentFunction->IsExcluded && currentFunction->IsPublic && args_ok &&
    strcmp(data->Name, currentFunction->Name) && strcmp(data->Name, currentFunction->Name + 1))
  {
    /* make sure we haven't already done one of these */
    if (!DoneOne())
    {
      fprintf(fp, "\n  private native ");
      return_result(fp);
      fprintf(fp, "%s_%i(", currentFunction->Name, numberOfWrappedFunctions);

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        if (i)
        {
          fprintf(fp, ",");
        }
        output_temp(fp, i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      fprintf(fp, ");\n");
      fprintf(fp, "  public ");
      return_result(fp);
      fprintf(fp, "%s(", beanfunc);

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        if (i)
        {
          fprintf(fp, ",");
        }
        output_temp(fp, i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      /* if not void then need return otherwise none */
      if (rType == SVTK_PARSE_VOID)
      {
        fprintf(fp, ")\n    { %s_%i(", currentFunction->Name, numberOfWrappedFunctions);
      }
      else
      {
        fprintf(fp, ")\n    { return %s_%i(", currentFunction->Name, numberOfWrappedFunctions);
      }
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        if (i)
        {
          fprintf(fp, ",");
        }
        fprintf(fp, "id%i", i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      if ((currentFunction->NumberOfArguments == 1) &&
        (currentFunction->ArgTypes[0] == SVTK_PARSE_FUNCTION))
        fprintf(fp, ",id1");

      /* stick in secret beanie code for set methods */
      if (rType == SVTK_PARSE_VOID)
      {
        aType = (currentFunction->ArgTypes[0] & SVTK_PARSE_UNQUALIFIED_TYPE);

        /* only care about set methods and On/Off methods */
        if (!strncmp(beanfunc, "set", 3) && currentFunction->NumberOfArguments == 1 &&
          (((aType & SVTK_PARSE_INDIRECT) == 0 && (aType & SVTK_PARSE_UNSIGNED) == 0) ||
            aType == SVTK_PARSE_CHAR_PTR || (aType & SVTK_PARSE_BASE_TYPE) == SVTK_PARSE_OBJECT))
        {
          char prop[256];

          strncpy(prop, beanfunc + 3, strlen(beanfunc) - 3);
          prop[strlen(beanfunc) - 3] = '\0';
          if (isupper(prop[0]))
            prop[0] = prop[0] + 32;
          fprintf(fp, ");\n      changes.firePropertyChange(\"%s\",null,", prop);

          /* handle basic types */
          if ((aType == SVTK_PARSE_CHAR_PTR) || (aType == SVTK_PARSE_STRING) ||
            (aType == SVTK_PARSE_STRING_REF))
          {
            fprintf(fp, " id0");
          }
          else
          {
            switch ((aType & SVTK_PARSE_BASE_TYPE) & ~SVTK_PARSE_UNSIGNED)
            {
              case SVTK_PARSE_FLOAT:
              case SVTK_PARSE_DOUBLE:
                fprintf(fp, " new Double(id0)");
                break;
              case SVTK_PARSE_INT:
              case SVTK_PARSE_SHORT:
              case SVTK_PARSE_LONG:
                fprintf(fp, " new Integer(id0)");
                break;
              case SVTK_PARSE_OBJECT:
                fprintf(fp, " id0");
                break;
              case SVTK_PARSE_CHAR: /* not implemented yet */
              default:
                fprintf(fp, " null");
            }
          }
        }
        /* not a set method is it an On/Off method ? */
        else
        {
          if (!strncmp(beanfunc + strlen(beanfunc) - 2, "On", 2))
          {
            /* OK we think this is a Boolean method so need to fire a change */
            char prop[256];
            strncpy(prop, beanfunc, strlen(beanfunc) - 2);
            prop[strlen(beanfunc) - 2] = '\0';
            fprintf(fp, ");\n      changes.firePropertyChange(\"%s\",null,new Integer(1)", prop);
          }
          if (!strncmp(beanfunc + strlen(beanfunc) - 3, "Off", 3))
          {
            /* OK we think this is a Boolean method so need to fire a change */
            char prop[256];
            strncpy(prop, beanfunc, strlen(beanfunc) - 3);
            prop[strlen(beanfunc) - 3] = '\0';
            fprintf(fp, ");\n      changes.firePropertyChange(\"%s\",null,new Integer(0)", prop);
          }
        }
      }
      fprintf(fp, "); }\n");

      wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
      numberOfWrappedFunctions++;
    }
  }
  free(beanfunc);
}

/* print the parsed structures */
void svtkParseOutput(FILE* fp, FileInfo* file_info)
{
  OptionInfo* options;
  ClassInfo* data;
  int i;

  /* get the main class */
  data = file_info->MainClass;
  if (data == NULL || data->IsExcluded)
  {
    return;
  }

  /* get the command-line options */
  options = svtkParse_GetCommandLineOptions();

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileNames)
  {
    hierarchyInfo =
      svtkParseHierarchy_ReadFiles(options->NumberOfHierarchyFileNames, options->HierarchyFileNames);
  }

  fprintf(fp, "// java wrapper for %s object\n//\n", data->Name);
  fprintf(fp, "\npackage svtk;\n");

  /* beans */
  if (!data->NumberOfSuperClasses)
  {
    fprintf(fp, "import java.beans.*;\n");
  }

  if (strcmp("svtkObject", data->Name))
  {
    fprintf(fp, "import svtk.*;\n");
  }
  fprintf(fp, "\npublic class %s", data->Name);
  if (strcmp("svtkObject", data->Name))
  {
    if (data->NumberOfSuperClasses)
      fprintf(fp, " extends %s", data->SuperClasses[0]);
  }
  fprintf(fp, "\n{\n");

  fprintf(fp, "  public %s getThis%s() { return this;}\n\n", data->Name, data->Name + 3);

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    currentFunction = data->Functions[i];
    if (!currentFunction->IsExcluded)
    {
      outputFunction(fp, data);
    }
  }

  if (!data->NumberOfSuperClasses)
  {
    fprintf(fp, "\n  public %s() { this.SVTKInit();}\n", data->Name);
    fprintf(fp, "  protected int svtkId = 0;\n");

    /* beans */
    fprintf(fp, "  public void addPropertyChangeListener(PropertyChangeListener l)\n  {\n");
    fprintf(fp, "    changes.addPropertyChangeListener(l);\n  }\n");
    fprintf(fp, "  public void removePropertyChangeListener(PropertyChangeListener l)\n  {\n");
    fprintf(fp, "    changes.removePropertyChangeListener(l);\n  }\n");
    fprintf(fp, "  protected PropertyChangeSupport changes = new PropertyChangeSupport(this);\n\n");

    /* if we are a base class and have a delete method */
    if (data->HasDelete)
    {
      fprintf(fp, "\n  public native void SVTKDelete();\n");
      fprintf(fp, "  protected void finalize() { this.SVTKDelete();}\n");
    }
  }
  if ((!data->IsAbstract) && strcmp(data->Name, "svtkDataWriter") &&
    strcmp(data->Name, "svtkPointSet") && strcmp(data->Name, "svtkDataSetSource"))
  {
    fprintf(fp, "  public native void   SVTKInit();\n");
  }
  if (!strcmp("svtkObject", data->Name))
  {
    fprintf(fp, "  public native String Print();\n");
  }
  fprintf(fp, "}\n");
}
