/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapJava.c

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
#include "svtkWrap.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

HierarchyInfo* hierarchyInfo = NULL;
StringCache* stringCache = NULL;
int numberOfWrappedFunctions = 0;
FunctionInfo* wrappedFunctions[1000];
FunctionInfo* currentFunction;
ClassInfo* CurrentData;

void output_proto_vars(FILE* fp, int i)
{
  unsigned int aType = (currentFunction->ArgTypes[i] & SVTK_PARSE_UNQUALIFIED_TYPE);

  /* ignore void */
  if (aType == SVTK_PARSE_VOID)
  {
    return;
  }

  if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
  {
    fprintf(fp, "jobject id0, jstring id1");
    return;
  }

  if ((aType == SVTK_PARSE_CHAR_PTR) || (aType == SVTK_PARSE_STRING) ||
    (aType == SVTK_PARSE_STRING_REF))
  {
    fprintf(fp, "jstring ");
    fprintf(fp, "id%i", i);
    return;
  }

  if ((aType == SVTK_PARSE_FLOAT_PTR) || (aType == SVTK_PARSE_DOUBLE_PTR))
  {
    fprintf(fp, "jdoubleArray ");
    fprintf(fp, "id%i", i);
    return;
  }

  if ((aType == SVTK_PARSE_INT_PTR) || (aType == SVTK_PARSE_SHORT_PTR) ||
    (aType == SVTK_PARSE_SIGNED_CHAR_PTR) || (aType == SVTK_PARSE_LONG_PTR) ||
    (aType == SVTK_PARSE_LONG_LONG_PTR) || (aType == SVTK_PARSE___INT64_PTR))
  {
    fprintf(fp, "jintArray ");
    fprintf(fp, "id%i", i);
    return;
  }

  switch ((aType & SVTK_PARSE_BASE_TYPE) & ~SVTK_PARSE_UNSIGNED)
  {
    case SVTK_PARSE_FLOAT:
      fprintf(fp, "jdouble ");
      break;
    case SVTK_PARSE_DOUBLE:
      fprintf(fp, "jdouble ");
      break;
    case SVTK_PARSE_INT:
      fprintf(fp, "jint ");
      break;
    case SVTK_PARSE_SHORT:
      fprintf(fp, "jint ");
      break;
    case SVTK_PARSE_LONG:
      fprintf(fp, "jint ");
      break;
    case SVTK_PARSE_LONG_LONG:
      fprintf(fp, "jint ");
      break;
    case SVTK_PARSE___INT64:
      fprintf(fp, "jint ");
      break;
    case SVTK_PARSE_SIGNED_CHAR:
      fprintf(fp, "jint ");
      break;
    case SVTK_PARSE_BOOL:
      fprintf(fp, "jboolean ");
      break;
    case SVTK_PARSE_VOID:
      fprintf(fp, "void ");
      break;
    case SVTK_PARSE_CHAR:
      fprintf(fp, "jchar ");
      break;
    case SVTK_PARSE_OBJECT:
      fprintf(fp, "jobject ");
      break;
    case SVTK_PARSE_UNKNOWN: /* enum */
      fprintf(fp, "jint ");
      break;
  }

  fprintf(fp, "id%i", i);
}

/* when the cpp file doesn't have enough info use the hint file */
void use_hints(FILE* fp)
{
  unsigned int rType = (currentFunction->ReturnType & SVTK_PARSE_UNQUALIFIED_TYPE);

  /* use the hint */
  switch (rType)
  {
    case SVTK_PARSE_UNSIGNED_CHAR_PTR:
      /* for svtkDataWriter we want to handle this case specially */
      if (strcmp(currentFunction->Name, "GetBinaryOutputString") ||
        strcmp(CurrentData->Name, "svtkDataWriter"))
      {
        fprintf(fp, "    return svtkJavaMakeJArrayOfByteFromUnsignedChar(env,temp%i,%i);\n",
          MAX_ARGS, currentFunction->HintSize);
      }
      else
      {
        fprintf(fp,
          "    return "
          "svtkJavaMakeJArrayOfByteFromUnsignedChar(env,temp%i,op->GetOutputStringLength());\n",
          MAX_ARGS);
      }
      break;

    case SVTK_PARSE_FLOAT_PTR:
      fprintf(fp, "    return svtkJavaMakeJArrayOfDoubleFromFloat(env,temp%i,%i);\n", MAX_ARGS,
        currentFunction->HintSize);
      break;

    case SVTK_PARSE_DOUBLE_PTR:
      fprintf(fp, "    return svtkJavaMakeJArrayOfDoubleFromDouble(env,temp%i,%i);\n", MAX_ARGS,
        currentFunction->HintSize);
      break;

    case SVTK_PARSE_INT_PTR:
      fprintf(fp, "    return svtkJavaMakeJArrayOfIntFromInt(env,temp%i,%i);\n", MAX_ARGS,
        currentFunction->HintSize);
      break;

    case SVTK_PARSE_LONG_LONG_PTR:
      fprintf(fp, "    return svtkJavaMakeJArrayOfIntFromLongLong(env,temp%i,%i);\n", MAX_ARGS,
        currentFunction->HintSize);
      break;

    case SVTK_PARSE_SIGNED_CHAR_PTR:
      fprintf(fp, "    return svtkJavaMakeJArrayOfIntFromSignedChar(env,temp%i,%i);\n", MAX_ARGS,
        currentFunction->HintSize);
      break;

    case SVTK_PARSE_BOOL_PTR:
      fprintf(fp, "    return svtkJavaMakeJArrayOfIntFromBool(env,temp%i,%i);\n", MAX_ARGS,
        currentFunction->HintSize);
      break;

    case SVTK_PARSE_SHORT_PTR:
      fprintf(fp, "    return svtkJavaMakeJArrayOfShortFromShort(env,temp%i,%i);\n", MAX_ARGS,
        currentFunction->HintSize);
      break;

    case SVTK_PARSE_LONG_PTR:
      fprintf(fp, "    return svtkJavaMakeJArrayOfLongFromLong(env,temp%i,%i);\n", MAX_ARGS,
        currentFunction->HintSize);
      break;

    case SVTK_PARSE_UNSIGNED_INT_PTR:
    case SVTK_PARSE_UNSIGNED_SHORT_PTR:
    case SVTK_PARSE_UNSIGNED_LONG_PTR:
    case SVTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case SVTK_PARSE_UNSIGNED___INT64_PTR:
      break;
  }
}

void return_result(FILE* fp)
{
  unsigned int rType = (currentFunction->ReturnType & SVTK_PARSE_UNQUALIFIED_TYPE);

  switch (rType)
  {
    case SVTK_PARSE_FLOAT:
      fprintf(fp, "jdouble ");
      break;
    case SVTK_PARSE_VOID:
      fprintf(fp, "void ");
      break;
    case SVTK_PARSE_CHAR:
      fprintf(fp, "jchar ");
      break;
    case SVTK_PARSE_DOUBLE:
      fprintf(fp, "jdouble ");
      break;
    case SVTK_PARSE_INT:
    case SVTK_PARSE_SHORT:
    case SVTK_PARSE_LONG:
    case SVTK_PARSE_LONG_LONG:
    case SVTK_PARSE___INT64:
    case SVTK_PARSE_SIGNED_CHAR:
    case SVTK_PARSE_UNSIGNED_CHAR:
    case SVTK_PARSE_UNSIGNED_INT:
    case SVTK_PARSE_UNSIGNED_SHORT:
    case SVTK_PARSE_UNSIGNED_LONG:
    case SVTK_PARSE_UNSIGNED_LONG_LONG:
    case SVTK_PARSE_UNSIGNED___INT64:
    case SVTK_PARSE_UNKNOWN: /* enum */
      fprintf(fp, "jint ");
      break;
    case SVTK_PARSE_BOOL:
      fprintf(fp, "jboolean ");
      break;
    case SVTK_PARSE_CHAR_PTR:
    case SVTK_PARSE_STRING:
    case SVTK_PARSE_STRING_REF:
      fprintf(fp, "jstring ");
      break;
    case SVTK_PARSE_OBJECT_PTR:
      fprintf(fp, "jlong ");
      break;
    case SVTK_PARSE_FLOAT_PTR:
    case SVTK_PARSE_DOUBLE_PTR:
    case SVTK_PARSE_UNSIGNED_CHAR_PTR:
    case SVTK_PARSE_INT_PTR:
    case SVTK_PARSE_SHORT_PTR:
    case SVTK_PARSE_LONG_PTR:
    case SVTK_PARSE_LONG_LONG_PTR:
    case SVTK_PARSE___INT64_PTR:
    case SVTK_PARSE_SIGNED_CHAR_PTR:
    case SVTK_PARSE_BOOL_PTR:
    case SVTK_PARSE_UNSIGNED_LONG_LONG_PTR:
    case SVTK_PARSE_UNSIGNED___INT64_PTR:
      fprintf(fp, "jarray ");
      break;
  }
}

void output_temp(FILE* fp, int i, unsigned int aType, const char* Id, int aCount)
{
  /* handle VAR FUNCTIONS */
  if (aType == SVTK_PARSE_FUNCTION)
  {
    fprintf(fp, "  svtkJavaVoidFuncArg *temp%i = new svtkJavaVoidFuncArg;\n", i);
    return;
  }

  /* ignore void */
  if ((aType & SVTK_PARSE_UNQUALIFIED_TYPE) == SVTK_PARSE_VOID)
  {
    return;
  }

  /* for const * return types prototype with const */
  if ((i == MAX_ARGS) && ((aType & SVTK_PARSE_INDIRECT) != 0) && ((aType & SVTK_PARSE_CONST) != 0))
  {
    fprintf(fp, "  const ");
  }
  else
  {
    fprintf(fp, "  ");
  }

  if ((aType & SVTK_PARSE_UNSIGNED) != 0)
  {
    fprintf(fp, " unsigned ");
  }

  switch ((aType & SVTK_PARSE_BASE_TYPE) & ~SVTK_PARSE_UNSIGNED)
  {
    case SVTK_PARSE_FLOAT:
      fprintf(fp, "float  ");
      break;
    case SVTK_PARSE_DOUBLE:
      fprintf(fp, "double ");
      break;
    case SVTK_PARSE_INT:
      fprintf(fp, "int    ");
      break;
    case SVTK_PARSE_SHORT:
      fprintf(fp, "short  ");
      break;
    case SVTK_PARSE_LONG:
      fprintf(fp, "long   ");
      break;
    case SVTK_PARSE_VOID:
      fprintf(fp, "void   ");
      break;
    case SVTK_PARSE_CHAR:
      fprintf(fp, "char   ");
      break;
    case SVTK_PARSE_LONG_LONG:
      fprintf(fp, "long long ");
      break;
    case SVTK_PARSE___INT64:
      fprintf(fp, "__int64 ");
      break;
    case SVTK_PARSE_SIGNED_CHAR:
      fprintf(fp, "signed char ");
      break;
    case SVTK_PARSE_BOOL:
      fprintf(fp, "bool ");
      break;
    case SVTK_PARSE_OBJECT:
      fprintf(fp, "%s ", Id);
      break;
    case SVTK_PARSE_STRING:
      fprintf(fp, "%s ", Id);
      break;
    case SVTK_PARSE_UNKNOWN:
      fprintf(fp, "%s ", Id);
      break;
  }

  switch (aType & SVTK_PARSE_INDIRECT)
  {
    case SVTK_PARSE_REF:
      if (i == MAX_ARGS)
      {
        fprintf(fp, " *"); /* act " &" */
      }
      break;
    case SVTK_PARSE_POINTER:
      if ((i == MAX_ARGS) || ((aType & SVTK_PARSE_UNQUALIFIED_TYPE) == SVTK_PARSE_OBJECT_PTR) ||
        ((aType & SVTK_PARSE_UNQUALIFIED_TYPE) == SVTK_PARSE_CHAR_PTR))
      {
        fprintf(fp, " *");
      }
      break;
    default:
      fprintf(fp, "  ");
      break;
  }
  fprintf(fp, "temp%i", i);

  /* handle arrays */
  if (((aType & SVTK_PARSE_INDIRECT) == SVTK_PARSE_POINTER) && (i != MAX_ARGS) &&
    ((aType & SVTK_PARSE_UNQUALIFIED_TYPE) != SVTK_PARSE_OBJECT_PTR) &&
    ((aType & SVTK_PARSE_UNQUALIFIED_TYPE) != SVTK_PARSE_CHAR_PTR))
  {
    fprintf(fp, "[%i]", aCount);
    fprintf(fp, ";\n  void *tempArray%i", i);
  }

  fprintf(fp, ";\n");
}

void get_args(FILE* fp, int i)
{
  unsigned int aType = (currentFunction->ArgTypes[i] & SVTK_PARSE_UNQUALIFIED_TYPE);
  int j;

  /* handle VAR FUNCTIONS */
  if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
  {
    fprintf(fp, "  env->GetJavaVM(&(temp%i->vm));\n", i);
    fprintf(fp, "  temp%i->uobj = env->NewGlobalRef(id0);\n", i);
    fprintf(fp, "  char *temp%i_str;\n", i);
    fprintf(fp, "  temp%i_str = svtkJavaUTFToChar(env,id1);\n", i);
    fprintf(
      fp, "  temp%i->mid = env->GetMethodID(env->GetObjectClass(id0),temp%i_str,\"()V\");\n", i, i);
    return;
  }

  /* ignore void */
  if (aType == SVTK_PARSE_VOID)
  {
    return;
  }

  switch (aType)
  {
    case SVTK_PARSE_CHAR:
      fprintf(fp, "  temp%i = (char)(0xff & id%i);\n", i, i);
      break;
    case SVTK_PARSE_BOOL:
      fprintf(fp, "  temp%i = (id%i != 0) ? true : false;\n", i, i);
      break;
    case SVTK_PARSE_CHAR_PTR:
      fprintf(fp, "  temp%i = svtkJavaUTFToChar(env,id%i);\n", i, i);
      break;
    case SVTK_PARSE_STRING:
    case SVTK_PARSE_STRING_REF:
      fprintf(fp, "  svtkJavaUTFToString(env,id%i,temp%i);\n", i, i);
      break;
    case SVTK_PARSE_OBJECT_PTR:
      fprintf(fp, "  temp%i = (%s *)(svtkJavaGetPointerFromObject(env,id%i));\n", i,
        currentFunction->ArgClasses[i], i);
      break;
    case SVTK_PARSE_FLOAT_PTR:
    case SVTK_PARSE_DOUBLE_PTR:
      fprintf(fp, "  tempArray%i = (void *)(env->GetDoubleArrayElements(id%i,nullptr));\n", i, i);
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
      {
        fprintf(fp, "  temp%i[%i] = ((jdouble *)tempArray%i)[%i];\n", i, j, i, j);
      }
      break;
    case SVTK_PARSE_INT_PTR:
    case SVTK_PARSE_SHORT_PTR:
    case SVTK_PARSE_LONG_PTR:
    case SVTK_PARSE_LONG_LONG_PTR:
    case SVTK_PARSE___INT64_PTR:
    case SVTK_PARSE_SIGNED_CHAR_PTR:
    case SVTK_PARSE_BOOL_PTR:
      fprintf(fp, "  tempArray%i = (void *)(env->GetIntArrayElements(id%i,nullptr));\n", i, i);
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
      {
        fprintf(fp, "  temp%i[%i] = ((jint *)tempArray%i)[%i];\n", i, j, i, j);
      }
      break;
    case SVTK_PARSE_UNKNOWN:
      fprintf(fp, "  temp%i = static_cast<%s>(id%i);\n", i, currentFunction->ArgClasses[i], i);
      break;
    case SVTK_PARSE_VOID:
    case SVTK_PARSE_OBJECT:
    case SVTK_PARSE_OBJECT_REF:
      break;
    default:
      fprintf(fp, "  temp%i = id%i;\n", i, i);
      break;
  }
}

void copy_and_release_args(FILE* fp, int i)
{
  unsigned int aType = (currentFunction->ArgTypes[i] & SVTK_PARSE_UNQUALIFIED_TYPE);
  int j;

  /* handle VAR FUNCTIONS */
  if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
  {
    fprintf(fp, "  delete[] temp%i_str;\n", i);
    return;
  }

  /* ignore void */
  if (aType == SVTK_PARSE_VOID)
  {
    return;
  }

  switch (aType)
  {
    case SVTK_PARSE_FLOAT_PTR:
    case SVTK_PARSE_DOUBLE_PTR:
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
      {
        fprintf(fp, "  ((jdouble *)tempArray%i)[%i] = temp%i[%i];\n", i, j, i, j);
      }
      fprintf(fp, "  env->ReleaseDoubleArrayElements(id%i,(jdouble *)tempArray%i,0);\n", i, i);
      break;
    case SVTK_PARSE_CHAR_PTR:
      fprintf(fp, "  delete[] temp%i;\n", i);
      break;
    case SVTK_PARSE_INT_PTR:
    case SVTK_PARSE_LONG_PTR:
    case SVTK_PARSE_SHORT_PTR:
    case SVTK_PARSE_LONG_LONG_PTR:
    case SVTK_PARSE___INT64_PTR:
    case SVTK_PARSE_SIGNED_CHAR_PTR:
    case SVTK_PARSE_BOOL_PTR:
      for (j = 0; j < currentFunction->ArgCounts[i]; j++)
      {
        fprintf(fp, "  ((jint *)tempArray%i)[%i] = temp%i[%i];\n", i, j, i, j);
      }
      fprintf(fp, "  env->ReleaseIntArrayElements(id%i,(jint *)tempArray%i,0);\n", i, i);
      break;
    default:
      break;
  }
}

void do_return(FILE* fp)
{
  unsigned int rType = (currentFunction->ReturnType & SVTK_PARSE_UNQUALIFIED_TYPE);

  /* ignore void */
  if (rType == SVTK_PARSE_VOID)
  {
    return;
  }

  switch (rType)
  {
    case SVTK_PARSE_CHAR_PTR:
    {
      fprintf(fp, "  return svtkJavaMakeJavaString(env,temp%i);\n", MAX_ARGS);
      break;
    }
    case SVTK_PARSE_STRING:
    {
      fprintf(fp, "  return svtkJavaMakeJavaString(env,temp%i.c_str());\n", MAX_ARGS);
      break;
    }
    case SVTK_PARSE_STRING_REF:
    {
      fprintf(fp, "  return svtkJavaMakeJavaString(env,temp%i->c_str());\n", MAX_ARGS);
      break;
    }
    case SVTK_PARSE_OBJECT_PTR:
    {
      fprintf(fp, "  return (jlong)(size_t)temp%i;", MAX_ARGS);
      break;
    }

    /* handle functions returning vectors */
    /* this is done by looking them up in a hint file */
    case SVTK_PARSE_FLOAT_PTR:
    case SVTK_PARSE_DOUBLE_PTR:
    case SVTK_PARSE_UNSIGNED_CHAR_PTR:
    case SVTK_PARSE_INT_PTR:
    case SVTK_PARSE_SHORT_PTR:
    case SVTK_PARSE_LONG_PTR:
    case SVTK_PARSE_LONG_LONG_PTR:
    case SVTK_PARSE___INT64_PTR:
    case SVTK_PARSE_SIGNED_CHAR_PTR:
    case SVTK_PARSE_BOOL_PTR:
      use_hints(fp);
      break;

    /* handle enums, they are the only 'UNKNOWN' these wrappers use */
    case SVTK_PARSE_UNKNOWN:
      fprintf(fp, "  return static_cast<jint>(temp%i);\n", MAX_ARGS);
      break;

    default:
      fprintf(fp, "  return temp%i;\n", MAX_ARGS);
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

void HandleDataReader(FILE* fp, ClassInfo* data)
{
  fprintf(fp, "\n");
  fprintf(fp, "extern \"C\" JNIEXPORT void");
  fprintf(fp, " JNICALL Java_svtk_%s_%s_1%i(JNIEnv *env, jobject obj, jbyteArray id0, jint id1)\n",
    data->Name, currentFunction->Name, numberOfWrappedFunctions);
  fprintf(fp, "{\n");
  fprintf(fp, "  %s *op;\n", data->Name);
  fprintf(fp, "  op = (%s *)svtkJavaGetPointerFromObject(env,obj);\n", data->Name);
  fprintf(fp, "  jboolean isCopy;\n");
  fprintf(fp, "  jbyte *data = env->GetByteArrayElements(id0,&isCopy);\n");
  fprintf(fp, "  op->SetBinaryInputString((const char *)data,id1);\n");
  fprintf(fp, "  env->ReleaseByteArrayElements(id0,data,JNI_ABORT);\n");
  fprintf(fp, "}\n");
}

void HandleDataArray(FILE* fp, ClassInfo* data)
{
  const char* type = 0;
  const char* jtype = 0;
  const char* fromtype = 0;
  const char* jfromtype = 0;

  if (!strcmp("svtkCharArray", data->Name))
  {
    type = "char";
    fromtype = "Char";
    jtype = "byte";
    jfromtype = "Byte";
  }
  else if (!strcmp("svtkDoubleArray", data->Name))
  {
    type = "double";
    fromtype = "Double";
    jtype = type;
    jfromtype = fromtype;
  }
  else if (!strcmp("svtkFloatArray", data->Name))
  {
    type = "float";
    fromtype = "Float";
    jtype = type;
    jfromtype = fromtype;
  }
  else if (!strcmp("svtkIntArray", data->Name))
  {
    type = "int";
    fromtype = "Int";
    jtype = type;
    jfromtype = fromtype;
  }
  else if (!strcmp("svtkLongArray", data->Name))
  {
    type = "long";
    fromtype = "Long";
    jtype = type;
    jfromtype = fromtype;
  }
  else if (!strcmp("svtkShortArray", data->Name))
  {
    type = "short";
    fromtype = "Short";
    jtype = type;
    jfromtype = fromtype;
  }
  else if (!strcmp("svtkUnsignedCharArray", data->Name))
  {
    type = "unsigned char";
    fromtype = "UnsignedChar";
    jtype = "byte";
    jfromtype = "Byte";
  }
  else if (!strcmp("svtkUnsignedIntArray", data->Name))
  {
    type = "unsigned int";
    fromtype = "UnsignedInt";
    jtype = "int";
    jfromtype = "Int";
  }
  else if (!strcmp("svtkUnsignedLongArray", data->Name))
  {
    type = "unsigned long";
    fromtype = "UnsignedLong";
    jtype = "long";
    jfromtype = "Long";
  }
  else if (!strcmp("svtkUnsignedShortArray", data->Name))
  {
    type = "unsigned short";
    fromtype = "UnsignedShort";
    jtype = "short";
    jfromtype = "Short";
  }
  else
  {
    return;
  }

  fprintf(fp, "// Array conversion routines\n");
  fprintf(fp,
    "extern \"C\" JNIEXPORT jarray JNICALL Java_svtk_%s_GetJavaArray_10("
    "JNIEnv *env, jobject obj)\n",
    data->Name);
  fprintf(fp, "{\n");
  fprintf(fp, "  %s *op;\n", data->Name);
  fprintf(fp, "  %s  *temp20;\n", type);
  fprintf(fp, "  svtkIdType size;\n");
  fprintf(fp, "\n");
  fprintf(fp, "  op = (%s *)svtkJavaGetPointerFromObject(env,obj);\n", data->Name);
  fprintf(fp, "  temp20 = static_cast<%s*>(op->GetVoidPointer(0));\n", type);
  fprintf(fp, "  size = op->GetMaxId()+1;\n");
  fprintf(fp, "  return svtkJavaMakeJArrayOf%sFrom%s(env,temp20,size);\n", fromtype, fromtype);
  fprintf(fp, "}\n");

  fprintf(fp,
    "extern \"C\" JNIEXPORT void  JNICALL Java_svtk_%s_SetJavaArray_10("
    "JNIEnv *env, jobject obj,j%sArray id0)\n",
    data->Name, jtype);
  fprintf(fp, "{\n");
  fprintf(fp, "  %s *op;\n", data->Name);
  fprintf(fp, "  %s *tempArray0;\n", type);
  fprintf(fp, "  int length;\n");
  fprintf(fp, "  tempArray0 = (%s *)(env->Get%sArrayElements(id0,nullptr));\n", type, jfromtype);
  fprintf(fp, "  length = env->GetArrayLength(id0);\n");
  fprintf(fp, "  op = (%s *)svtkJavaGetPointerFromObject(env,obj);\n", data->Name);
  fprintf(fp, "  op->SetNumberOfTuples(length/op->GetNumberOfComponents());\n");
  fprintf(fp, "  memcpy(op->GetVoidPointer(0), tempArray0, length*sizeof(%s));\n", type);
  fprintf(fp, "  env->Release%sArrayElements(id0,(j%s *)tempArray0,0);\n", jfromtype, jtype);
  fprintf(fp, "}\n");
}

static int isClassWrapped(const char* classname)
{
  HierarchyEntry* entry;

  if (hierarchyInfo)
  {
    entry = svtkParseHierarchy_FindEntry(hierarchyInfo, classname);

    if (entry == 0 || !svtkParseHierarchy_IsTypeOf(hierarchyInfo, entry, "svtkObjectBase"))
    {
      return 0;
    }
  }

  return 1;
}

int checkFunctionSignature(ClassInfo* data)
{
  static unsigned int supported_types[] = { SVTK_PARSE_VOID, SVTK_PARSE_BOOL, SVTK_PARSE_FLOAT,
    SVTK_PARSE_DOUBLE, SVTK_PARSE_CHAR, SVTK_PARSE_UNSIGNED_CHAR, SVTK_PARSE_SIGNED_CHAR, SVTK_PARSE_INT,
    SVTK_PARSE_UNSIGNED_INT, SVTK_PARSE_SHORT, SVTK_PARSE_UNSIGNED_SHORT, SVTK_PARSE_LONG,
    SVTK_PARSE_UNSIGNED_LONG, SVTK_PARSE_LONG_LONG, SVTK_PARSE_UNSIGNED_LONG_LONG, SVTK_PARSE___INT64,
    SVTK_PARSE_UNSIGNED___INT64, SVTK_PARSE_OBJECT, SVTK_PARSE_STRING, SVTK_PARSE_UNKNOWN, 0 };

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

    if (baseType == SVTK_PARSE_UNKNOWN)
    {
      const char* qualified_name = 0;
      if ((aType & SVTK_PARSE_INDIRECT) == 0)
      {
        qualified_name = svtkParseHierarchy_QualifiedEnumName(
          hierarchyInfo, data, stringCache, currentFunction->ArgClasses[i]);
      }
      if (qualified_name)
      {
        currentFunction->ArgClasses[i] = qualified_name;
      }
      else
      {
        args_ok = 0;
      }
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

  if (baseType == SVTK_PARSE_UNKNOWN)
  {
    const char* qualified_name = 0;
    if ((rType & SVTK_PARSE_INDIRECT) == 0)
    {
      qualified_name = svtkParseHierarchy_QualifiedEnumName(
        hierarchyInfo, data, stringCache, currentFunction->ReturnClass);
    }
    if (qualified_name)
    {
      currentFunction->ReturnClass = qualified_name;
    }
    else
    {
      args_ok = 0;
    }
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
  int i;
  int args_ok;
  unsigned int rType = (currentFunction->ReturnType & SVTK_PARSE_UNQUALIFIED_TYPE);
  const char* jniFunction = 0;
  char* jniFunctionNew = 0;
  char* jniFunctionOld = 0;
  size_t j;
  CurrentData = data;

  args_ok = checkFunctionSignature(data);

  /* handle DataReader SetBinaryInputString as a special case */
  if (!strcmp("SetBinaryInputString", currentFunction->Name) &&
    (!strcmp("svtkDataReader", data->Name) || !strcmp("svtkStructuredGridReader", data->Name) ||
      !strcmp("svtkRectilinearGridReader", data->Name) ||
      !strcmp("svtkUnstructuredGridReader", data->Name) ||
      !strcmp("svtkStructuredPointsReader", data->Name) || !strcmp("svtkPolyDataReader", data->Name)))
  {
    if (currentFunction->IsLegacy)
    {
      fprintf(fp, "#if !defined(SVTK_LEGACY_REMOVE)\n");
    }
    HandleDataReader(fp, data);
    if (currentFunction->IsLegacy)
    {
      fprintf(fp, "#endif\n");
    }
    wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
    numberOfWrappedFunctions++;
  }

  if (!currentFunction->IsExcluded && currentFunction->IsPublic && args_ok &&
    strcmp(data->Name, currentFunction->Name) && strcmp(data->Name, currentFunction->Name + 1))
  {
    /* make sure we haven't already done one of these */
    if (!DoneOne())
    {
      fprintf(fp, "\n");

      /* Underscores are escaped in method names, see
           http://java.sun.com/javase/6/docs/technotes/guides/jni/spec/design.html#wp133
         SVTK class names contain no underscore and do not need to be escaped.  */
      jniFunction = currentFunction->Name;
      jniFunctionOld = 0;
      j = 0;
      while (jniFunction[j] != '\0')
      {
        /* replace "_" with "_1" */
        if (jniFunction[j] == '_')
        {
          j++;
          jniFunctionNew = (char*)malloc(strlen(jniFunction) + 2);
          strncpy(jniFunctionNew, jniFunction, j);
          jniFunctionNew[j] = '1';
          strcpy(&jniFunctionNew[j + 1], &jniFunction[j]);
          free(jniFunctionOld);
          jniFunctionOld = jniFunctionNew;
          jniFunction = jniFunctionNew;
        }
        j++;
      }

      if (currentFunction->IsLegacy)
      {
        fprintf(fp, "#if !defined(SVTK_LEGACY_REMOVE)\n");
      }
      fprintf(fp, "extern \"C\" JNIEXPORT ");
      return_result(fp);
      fprintf(fp, " JNICALL Java_svtk_%s_%s_1%i(JNIEnv *env, jobject obj", data->Name, jniFunction,
        numberOfWrappedFunctions);

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        fprintf(fp, ",");
        output_proto_vars(fp, i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      fprintf(fp, ")\n{\n");

      /* get the object pointer */
      fprintf(fp, "  %s *op;\n", data->Name);

      /* process the args */
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        output_temp(fp, i, currentFunction->ArgTypes[i], currentFunction->ArgClasses[i],
          currentFunction->ArgCounts[i]);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      output_temp(fp, MAX_ARGS, currentFunction->ReturnType, currentFunction->ReturnClass, 0);

      /* now get the required args from the stack */
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        get_args(fp, i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
        {
          break;
        }
      }

      fprintf(fp, "\n  op = (%s *)svtkJavaGetPointerFromObject(env,obj);\n", data->Name);

      switch (rType)
      {
        case SVTK_PARSE_VOID:
          fprintf(fp, "  op->%s(", currentFunction->Name);
          break;
        default:
          if ((rType & SVTK_PARSE_INDIRECT) == SVTK_PARSE_REF)
          {
            fprintf(fp, "  temp%i = &(op)->%s(", MAX_ARGS, currentFunction->Name);
          }
          else
          {
            fprintf(fp, "  temp%i = (op)->%s(", MAX_ARGS, currentFunction->Name);
          }
          break;
      }

      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        if (i)
        {
          fprintf(fp, ",");
        }
        if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
        {
          fprintf(fp, "svtkJavaVoidFunc,(void *)temp%i", i);
          break;
        }
        else
        {
          fprintf(fp, "temp%i", i);
        }
      } /* for */

      fprintf(fp, ");\n");

      if (currentFunction->NumberOfArguments == 2 &&
        currentFunction->ArgTypes[0] == SVTK_PARSE_FUNCTION)
      {
        fprintf(fp, "  op->%sArgDelete(svtkJavaVoidFuncArgDelete);\n", jniFunction);
      }

      /* now copy and release any arrays */
      for (i = 0; i < currentFunction->NumberOfArguments; i++)
      {
        copy_and_release_args(fp, i);

        /* ignore args after function pointer */
        if (currentFunction->ArgTypes[i] == SVTK_PARSE_FUNCTION)
        {
          break;
        }
      }
      do_return(fp);
      fprintf(fp, "}\n");
      if (currentFunction->IsLegacy)
      {
        fprintf(fp, "#endif\n");
      }

      wrappedFunctions[numberOfWrappedFunctions] = currentFunction;
      numberOfWrappedFunctions++;
      if (jniFunctionNew)
      {
        free(jniFunctionNew);
        jniFunctionNew = 0;
      }
    } /* isDone() */
  }   /* isAbstract */
}

/* print the parsed structures */
int main(int argc, char* argv[])
{
  OptionInfo* options;
  FileInfo* file_info;
  ClassInfo* data;
  FILE* fp;
  int i;

  /* pre-define a macro to identify the language */
  svtkParse_DefineMacro("__SVTK_WRAP_JAVA__", 0);

  /* get command-line args and parse the header file */
  file_info = svtkParse_Main(argc, argv);

  /* some utility functions require the string cache */
  stringCache = file_info->Strings;

  /* get the command-line options */
  options = svtkParse_GetCommandLineOptions();

  /* get the hierarchy info for accurate typing */
  if (options->HierarchyFileNames)
  {
    hierarchyInfo =
      svtkParseHierarchy_ReadFiles(options->NumberOfHierarchyFileNames, options->HierarchyFileNames);
  }

  /* get the output file */
  fp = fopen(options->OutputFileName, "w");

  if (!fp)
  {
    fprintf(stderr, "Error opening output file %s\n", options->OutputFileName);
    exit(1);
  }

  /* get the main class */
  data = file_info->MainClass;
  if (data == NULL || data->IsExcluded)
  {
    fclose(fp);
    exit(0);
  }

  if (data->Template)
  {
    fclose(fp);
    exit(0);
  }

  for (i = 0; i < data->NumberOfSuperClasses; ++i)
  {
    if (strchr(data->SuperClasses[i], '<'))
    {
      fclose(fp);
      exit(0);
    }
  }

  if (hierarchyInfo)
  {
    if (!svtkWrap_IsTypeOf(hierarchyInfo, data->Name, "svtkObjectBase"))
    {
      fclose(fp);
      exit(0);
    }

    /* resolve using declarations within the header files */
    svtkWrap_ApplyUsingDeclarations(data, file_info, hierarchyInfo);

    /* expand typedefs */
    svtkWrap_ExpandTypedefs(data, file_info, hierarchyInfo);
  }

  fprintf(fp, "// java wrapper for %s object\n//\n", data->Name);
  fprintf(fp, "#define SVTK_WRAPPING_CXX\n");
  if (strcmp("svtkObjectBase", data->Name) != 0)
  {
    /* Block inclusion of full streams.  */
    fprintf(fp, "#define SVTK_STREAMS_FWD_ONLY\n");
  }
  fprintf(fp, "#include \"svtkSystemIncludes.h\"\n");
  fprintf(fp, "#include \"%s.h\"\n", data->Name);
  fprintf(fp, "#include \"svtkJavaUtil.h\"\n\n");
  fprintf(fp, "#include \"svtkStdString.h\"\n\n");
  fprintf(fp, "#include <sstream>\n");

  for (i = 0; i < data->NumberOfSuperClasses; i++)
  {
    char* safe_name = svtkWrap_SafeSuperclassName(data->SuperClasses[i]);
    const char* safe_superclass = safe_name ? safe_name : data->SuperClasses[i];

    /* if a template class is detected add a typedef */
    if (safe_name)
    {
      fprintf(fp, "typedef %s %s;\n", data->SuperClasses[i], safe_name);
    }

    fprintf(
      fp, "extern \"C\" JNIEXPORT void* %s_Typecast(void *op,char *dType);\n", safe_superclass);

    free(safe_name);
  }

  fprintf(fp, "\nextern \"C\" JNIEXPORT void* %s_Typecast(void *me,char *dType)\n{\n", data->Name);
  if (data->NumberOfSuperClasses > 0)
  {
    fprintf(fp, "  void* res;\n");
  }
  fprintf(fp, "  if (!strcmp(\"%s\",dType)) { return me; }\n", data->Name);
  /* check our superclasses */
  for (i = 0; i < data->NumberOfSuperClasses; i++)
  {
    char* safe_name = svtkWrap_SafeSuperclassName(data->SuperClasses[i]);
    const char* safe_superclass = safe_name ? safe_name : data->SuperClasses[i];

    fprintf(fp, "  if ((res= %s_Typecast(me,dType)) != nullptr)", safe_superclass);
    fprintf(fp, " { return res; }\n");

    free(safe_name);
  }
  fprintf(fp, "  return nullptr;\n");
  fprintf(fp, "}\n\n");

  HandleDataArray(fp, data);

  /* insert function handling code here */
  for (i = 0; i < data->NumberOfFunctions; i++)
  {
    currentFunction = data->Functions[i];
    outputFunction(fp, data);
  }

  if ((!data->NumberOfSuperClasses) && (data->HasDelete))
  {
    fprintf(fp,
      "\nextern \"C\" JNIEXPORT void JNICALL Java_svtk_%s_SVTKDeleteReference(JNIEnv *,jclass,jlong "
      "id)\n",
      data->Name);
    fprintf(fp, "{\n  %s *op;\n", data->Name);
    fprintf(fp, "  op = reinterpret_cast<%s*>(id);\n", data->Name);
    fprintf(fp, "  op->Delete();\n");
    fprintf(fp, "}\n");

    fprintf(fp,
      "\nextern \"C\" JNIEXPORT jstring JNICALL Java_svtk_%s_SVTKGetClassNameFromReference(JNIEnv "
      "*env,jclass,jlong id)\n",
      data->Name);
    fprintf(fp, "{\n");
    fprintf(fp, "  const char* name = \"\";\n");
    fprintf(fp, "  %s *op;\n", data->Name);
    fprintf(fp, "  if(id != 0)\n");
    fprintf(fp, "  {\n");
    fprintf(fp, "    op = reinterpret_cast<%s*>(id);\n", data->Name);
    // fprintf(fp,"    std::cout << \"cast pointer \" << id << std::endl;\n");
    fprintf(fp, "    name = op->GetClassName();\n");
    fprintf(fp, "  }\n");
    fprintf(fp, "  return svtkJavaMakeJavaString(env,name);\n");
    fprintf(fp, "}\n");

    fprintf(fp,
      "\nextern \"C\" JNIEXPORT void JNICALL Java_svtk_%s_SVTKDelete(JNIEnv *env,jobject obj)\n",
      data->Name);
    fprintf(fp, "{\n  %s *op;\n", data->Name);
    fprintf(fp, "  op = (%s *)svtkJavaGetPointerFromObject(env,obj);\n", data->Name);
    fprintf(fp, "  op->Delete();\n");
    fprintf(fp, "}\n");

    fprintf(fp,
      "\nextern \"C\" JNIEXPORT void JNICALL Java_svtk_%s_SVTKRegister(JNIEnv *env,jobject obj)\n",
      data->Name);
    fprintf(fp, "{\n  %s *op;\n", data->Name);
    fprintf(fp, "  op = (%s *)svtkJavaGetPointerFromObject(env,obj);\n", data->Name);
    fprintf(fp, "  op->Register(op);\n");
    fprintf(fp, "}\n");
  }
  if (!data->IsAbstract)
  {
    fprintf(fp, "\nextern \"C\" JNIEXPORT jlong JNICALL Java_svtk_%s_SVTKInit(JNIEnv *, jobject)",
      data->Name);
    fprintf(fp, "\n{");
    fprintf(fp, "\n  %s *aNewOne = %s::New();", data->Name, data->Name);
    fprintf(fp, "\n  return (jlong)(size_t)(void*)aNewOne;");
    fprintf(fp, "\n}\n");
  }

  /* for svtkRenderWindow we want to add a special method to support
   * native AWT rendering
   *
   * Including svtkJavaAwt.h provides inline implementations of
   * Java_svtk_svtkPanel_RenderCreate, Java_svtk_svtkPanel_Lock and
   * Java_svtk_svtkPanel_UnLock. */
  if (!strcmp("svtkRenderWindow", data->Name))
  {
    fprintf(fp, "\n#include \"svtkJavaAwt.h\"\n\n");
  }

  if (!strcmp("svtkObject", data->Name))
  {
    /* Add the Print method to svtkObjectBase. */
    fprintf(fp,
      "\nextern \"C\" JNIEXPORT jstring JNICALL Java_svtk_svtkObjectBase_Print(JNIEnv *env,jobject "
      "obj)\n");
    fprintf(fp, "{\n  svtkObjectBase *op;\n");
    fprintf(fp, "  jstring tmp;\n\n");
    fprintf(fp, "  op = (svtkObjectBase *)svtkJavaGetPointerFromObject(env,obj);\n");

    fprintf(fp, "  std::ostringstream svtkmsg_with_warning_C4701;\n");
    fprintf(fp, "  op->Print(svtkmsg_with_warning_C4701);\n");
    fprintf(fp, "  svtkmsg_with_warning_C4701.put('\\0');\n");
    fprintf(fp, "  tmp = svtkJavaMakeJavaString(env,svtkmsg_with_warning_C4701.str().c_str());\n");

    fprintf(fp, "  return tmp;\n");
    fprintf(fp, "}\n");

    fprintf(fp,
      "\nextern \"C\" JNIEXPORT jint JNICALL Java_svtk_svtkObject_AddObserver(JNIEnv *env,jobject "
      "obj, jstring id0, jobject id1, jstring id2)\n");
    fprintf(fp, "{\n  svtkObject *op;\n");

    fprintf(fp, "  svtkJavaCommand *cbc = svtkJavaCommand::New();\n");
    fprintf(fp, "  cbc->AssignJavaVM(env);\n");
    fprintf(fp, "  cbc->SetGlobalRef(env->NewGlobalRef(id1));\n");
    fprintf(fp, "  char    *temp2;\n");
    fprintf(fp, "  temp2 = svtkJavaUTFToChar(env,id2);\n");
    fprintf(fp, "  cbc->SetMethodID(env->GetMethodID(env->GetObjectClass(id1),temp2,\"()V\"));\n");
    fprintf(fp, "  char    *temp0;\n");
    fprintf(fp, "  temp0 = svtkJavaUTFToChar(env,id0);\n");
    fprintf(fp, "  op = (svtkObject *)svtkJavaGetPointerFromObject(env,obj);\n");
    fprintf(fp, "  unsigned long     temp20;\n");
    fprintf(fp, "  temp20 = op->AddObserver(temp0,cbc);\n");
    fprintf(fp, "  delete[] temp0;\n");
    fprintf(fp, "  delete[] temp2;\n");
    fprintf(fp, "  cbc->Delete();\n");
    fprintf(fp, "  return temp20;\n}\n");
  }

  svtkParse_Free(file_info);

  fclose(fp);

  return 0;
}
