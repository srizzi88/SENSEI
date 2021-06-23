/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParseExtras.c

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2011 David Gobbi.

  Contributed to the VisualizationToolkit by the author in May 2011
  under the terms of the Visualization Toolkit 2008 copyright.
-------------------------------------------------------------------------*/

#include "svtkParseExtras.h"
#include "svtkParseString.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* skip over an expression in brackets */
static size_t svtkparse_bracket_len(const char* text)
{
  size_t i = 0;
  size_t j = 1;
  char bc = text[0];
  char tc = 0;
  char semi = ';';
  char c;

  if (bc == '(')
  {
    tc = ')';
  }
  else if (bc == '[')
  {
    tc = ']';
  }
  else if (bc == '{')
  {
    tc = '}';
    semi = '\0';
  }
  else if (bc == '<')
  {
    tc = '>';
  }
  else
  {
    return 0;
  }

  do
  {
    i += j;
    j = 1;
    c = text[i];
    if (svtkParse_CharType(c, CPRE_QUOTE))
    {
      j = svtkParse_SkipQuotes(&text[i]);
    }
    else if (c == bc || c == '(' || c == '[' || c == '{')
    {
      j = svtkparse_bracket_len(&text[i]);
    }
  } while (
    c != tc && c != ')' && c != ']' && c != '}' && c != '\0' && c != '\n' && c != semi && j != 0);

  if (c == tc)
  {
    i++;
  }

  return i;
}

/* skip over a name that is neither scoped or templated, return the
 * total number of characters in the name */
size_t svtkParse_IdentifierLength(const char* text)
{
  return svtkParse_SkipId(text);
}

/* skip over a name that might be templated, return the
 * total number of characters in the name */
size_t svtkParse_UnscopedNameLength(const char* text)
{
  size_t i = 0;

  i += svtkParse_SkipId(text);
  if (text[i] == '<')
  {
    i += svtkparse_bracket_len(&text[i]);
    if (text[i - 1] != '>')
    {
      fprintf(stderr, "Bad template args %*.*s\n", (int)i, (int)i, text);
      assert(text[i - 1] == '>');
      return 0;
    }
  }

  return i;
}

/* skip over a name that might be scoped or templated, return the
 * total number of characters in the name */
size_t svtkParse_NameLength(const char* text)
{
  size_t i = 0;
  do
  {
    if (text[i] == ':' && text[i + 1] == ':')
    {
      i += 2;
    }
    i += svtkParse_UnscopedNameLength(&text[i]);
  } while (text[i] == ':' && text[i + 1] == ':');
  return i;
}

/* Search and replace, return the initial string if no replacements
 * occurred, otherwise return a new string. */
static const char* svtkparse_string_replace(
  StringCache* cache, const char* str1, int n, const char* name[], const char* val[])
{
  const char* cp = str1;
  char result_store[1024];
  size_t resultMaxLen = 1024;
  char *result, *tmp;
  int k;
  size_t i, j, l, m;
  size_t lastPos, nameBegin, nameEnd;
  int replaced = 0;
  int any_replaced = 0;

  result = result_store;

  if (n == 0)
  {
    return str1;
  }

  i = 0;
  j = 0;
  result[j] = '\0';

  while (cp[i] != '\0')
  {
    lastPos = i;

    /* skip all chars that aren't part of a name */
    while (!svtkParse_CharType(cp[i], CPRE_ID) && cp[i] != '\0')
    {
      if (svtkParse_CharType(cp[i], CPRE_QUOTE))
      {
        i += svtkParse_SkipQuotes(&cp[i]);
      }
      else if (svtkParse_CharType(cp[i], CPRE_QUOTE))
      {
        i += svtkParse_SkipNumber(&cp[i]);
      }
      else
      {
        i++;
      }
    }
    nameBegin = i;

    /* skip all chars that are part of a name */
    i += svtkParse_SkipId(&cp[i]);
    nameEnd = i;

    /* search through the list of names to replace */
    replaced = 0;
    m = nameEnd - nameBegin;
    for (k = 0; k < n; k++)
    {
      l = strlen(name[k]);
      if (l > 0 && l == m && strncmp(&cp[nameBegin], name[k], l) == 0)
      {
        m = strlen(val[k]);
        replaced = 1;
        any_replaced = 1;
        break;
      }
    }

    /* expand the storage space if needed */
    if (j + m + (nameBegin - lastPos) + 1 >= resultMaxLen)
    {
      resultMaxLen *= 2;
      tmp = (char*)malloc(resultMaxLen);
      strcpy(tmp, result);
      if (result != result_store)
      {
        free(result);
      }
      result = tmp;
    }

    /* copy the old bits */
    if (nameBegin > lastPos)
    {
      strncpy(&result[j], &cp[lastPos], nameBegin - lastPos);
      j += (nameBegin - lastPos);
    }

    /* do the replacement */
    if (replaced)
    {
      strncpy(&result[j], val[k], m);
      j += m;
      /* guard against creating double ">>" */
      if (val[k][m - 1] == '>' && cp[nameEnd] == '>')
      {
        result[j++] = ' ';
      }
    }
    else if (nameEnd > nameBegin)
    {
      strncpy(&result[j], &cp[nameBegin], nameEnd - nameBegin);
      j += (nameEnd - nameBegin);
    }

    result[j] = '\0';
  }

  if (cache)
  {
    if (any_replaced)
    {
      /* use the efficient CacheString method */
      cp = svtkParse_CacheString(cache, result, j);
      if (result != result_store)
      {
        free(result);
      }
    }
  }
  else
  {
    if (any_replaced)
    {
      /* return a string that was allocated with malloc */
      if (result == result_store)
      {
        tmp = (char*)malloc(strlen(result) + 1);
        strcpy(tmp, result);
        result = tmp;
      }
      cp = result;
    }
  }

  return cp;
}

/* Wherever one of the specified names exists inside a Value or inside
 * a Dimension size, replace it with the corresponding val string. */
void svtkParse_ExpandValues(
  ValueInfo* valinfo, StringCache* cache, int n, const char* name[], const char* val[])
{
  int j, m, dim, count;
  const char* cp;

  if (valinfo->Value)
  {
    valinfo->Value = svtkparse_string_replace(cache, valinfo->Value, n, name, val);
  }

  m = valinfo->NumberOfDimensions;
  if (m)
  {
    count = 1;
    for (j = 0; j < m; j++)
    {
      cp = valinfo->Dimensions[j];
      if (cp)
      {
        cp = svtkparse_string_replace(cache, cp, n, name, val);
        valinfo->Dimensions[j] = cp;

        /* check whether dimension has become an integer literal */
        if (cp[0] == '0' && (cp[1] == 'x' || cp[1] == 'X'))
        {
          cp += 2;
        }
        while (*cp >= '0' && *cp <= '9')
        {
          cp++;
        }
        while (*cp == 'u' || *cp == 'l' || *cp == 'U' || *cp == 'L')
        {
          cp++;
        }
        dim = 0;
        if (*cp == '\0')
        {
          dim = (int)strtol(valinfo->Dimensions[j], NULL, 0);
        }
        count *= dim;
      }
    }

    /* update count if all values are integer literals */
    if (count)
    {
      valinfo->Count = count;
    }
  }
}

/* Expand a typedef within a type declaration. */
void svtkParse_ExpandTypedef(ValueInfo* valinfo, ValueInfo* typedefinfo)
{
  const char* classname;
  unsigned int baseType;
  unsigned int pointers;
  unsigned int refbit;
  unsigned int qualifiers;
  unsigned int attributes;
  unsigned int tmp1, tmp2;
  int i;

  classname = typedefinfo->Class;
  baseType = (typedefinfo->Type & SVTK_PARSE_BASE_TYPE);
  pointers = (typedefinfo->Type & SVTK_PARSE_POINTER_MASK);
  refbit = (valinfo->Type & SVTK_PARSE_REF);
  qualifiers = (typedefinfo->Type & SVTK_PARSE_CONST);
  attributes = (valinfo->Type & SVTK_PARSE_ATTRIBUTES);

  /* handle const */
  if ((valinfo->Type & SVTK_PARSE_CONST) != 0)
  {
    if ((pointers & SVTK_PARSE_POINTER_LOWMASK) != 0)
    {
      if ((pointers & SVTK_PARSE_POINTER_LOWMASK) != SVTK_PARSE_ARRAY)
      {
        /* const turns into const pointer */
        pointers = (pointers & ~SVTK_PARSE_POINTER_LOWMASK);
        pointers = (pointers | SVTK_PARSE_CONST_POINTER);
      }
    }
    else
    {
      /* const remains as const value */
      qualifiers = (qualifiers | SVTK_PARSE_CONST);
    }
  }

  /* make a reversed copy of the pointer bitfield */
  tmp1 = (valinfo->Type & SVTK_PARSE_POINTER_MASK);
  tmp2 = 0;
  while (tmp1)
  {
    tmp2 = ((tmp2 << 2) | (tmp1 & SVTK_PARSE_POINTER_LOWMASK));
    tmp1 = ((tmp1 >> 2) & SVTK_PARSE_POINTER_MASK);
  }

  /* turn pointers into zero-element arrays where necessary */
  if ((pointers & SVTK_PARSE_POINTER_LOWMASK) == SVTK_PARSE_ARRAY)
  {
    tmp2 = ((tmp2 >> 2) & SVTK_PARSE_POINTER_MASK);
    while (tmp2)
    {
      svtkParse_AddStringToArray(&valinfo->Dimensions, &valinfo->NumberOfDimensions, "");
      tmp2 = ((tmp2 >> 2) & SVTK_PARSE_POINTER_MASK);
    }
  }
  else
  {
    /* combine the pointers */
    while (tmp2)
    {
      pointers = ((pointers << 2) | (tmp2 & SVTK_PARSE_POINTER_LOWMASK));
      tmp2 = ((tmp2 >> 2) & SVTK_PARSE_POINTER_MASK);
    }
  }

  /* combine the arrays */
  for (i = 0; i < typedefinfo->NumberOfDimensions; i++)
  {
    svtkParse_AddStringToArray(
      &valinfo->Dimensions, &valinfo->NumberOfDimensions, typedefinfo->Dimensions[i]);
  }
  if (valinfo->NumberOfDimensions > 1)
  {
    pointers = ((pointers & ~SVTK_PARSE_POINTER_LOWMASK) | SVTK_PARSE_ARRAY);
  }

  /* put everything together */
  valinfo->Type = (baseType | pointers | refbit | qualifiers | attributes);
  valinfo->Class = classname;
  valinfo->Function = typedefinfo->Function;
}

/* Expand any unrecognized types within a variable, parameter, or typedef
 * that match any of the supplied typedefs. The expansion is done in-place. */
void svtkParse_ExpandTypedefs(ValueInfo* valinfo, StringCache* cache, int n, const char* name[],
  const char* val[], ValueInfo* typedefinfo[])
{
  int i;

  if (((valinfo->Type & SVTK_PARSE_BASE_TYPE) == SVTK_PARSE_OBJECT ||
        (valinfo->Type & SVTK_PARSE_BASE_TYPE) == SVTK_PARSE_UNKNOWN) &&
    valinfo->Class != 0)
  {
    for (i = 0; i < n; i++)
    {
      if (typedefinfo[i] && strcmp(valinfo->Class, typedefinfo[i]->Name) == 0)
      {
        svtkParse_ExpandTypedef(valinfo, typedefinfo[i]);
        break;
      }
    }
    if (i == n)
    {
      /* in case type appears as a template arg of another type */
      valinfo->Class = svtkparse_string_replace(cache, valinfo->Class, n, name, val);
    }
  }
}

/* Helper struct for SVTK-specific types */
struct svtk_type_struct
{
  size_t len;
  const char* name;
  unsigned int type;
};

/* Get a type from a type name, and return the number of characters used.
 * If the "classname" argument is not NULL, then it is used to return
 * the short name for the type, e.g. "long int" becomes "long", while
 * typedef names and class names are returned unchanged.  If "const"
 * appears in the type name, then the const bit flag is set for the
 * type, but "const" will not appear in the returned classname. */
size_t svtkParse_BasicTypeFromString(
  const char* text, unsigned int* type_ptr, const char** classname_ptr, size_t* len_ptr)
{
  /* The various typedefs and types specific to SVTK */
  static struct svtk_type_struct svtktypes[] = { { 12, "svtkStdString", SVTK_PARSE_STRING },
    { 16, "svtkUnicodeString", SVTK_PARSE_UNICODE_STRING }, { 0, 0, 0 } };

  /* Other typedefs and types */
  static struct svtk_type_struct stdtypes[] = { { 6, "size_t", SVTK_PARSE_SIZE_T },
    { 7, "ssize_t", SVTK_PARSE_SSIZE_T }, { 7, "ostream", SVTK_PARSE_OSTREAM },
    { 7, "istream", SVTK_PARSE_ISTREAM }, { 6, "string", SVTK_PARSE_STRING }, { 0, 0, 0 } };

  const char* cp = text;
  const char* tmpcp;
  size_t k, n, m;
  int i;
  unsigned int const_bits = 0;
  unsigned int static_bits = 0;
  unsigned int unsigned_bits = 0;
  unsigned int base_bits = 0;
  const char* classname = NULL;
  size_t len = 0;

  while (svtkParse_CharType(*cp, CPRE_HSPACE))
  {
    cp++;
  }

  while (svtkParse_CharType(*cp, CPRE_ID) || (cp[0] == ':' && cp[1] == ':'))
  {
    /* skip all chars that are part of a name */
    n = svtkParse_NameLength(cp);

    if ((n == 6 && strncmp("static", cp, n) == 0) || (n == 4 && strncmp("auto", cp, n) == 0) ||
      (n == 8 && strncmp("register", cp, n) == 0) || (n == 8 && strncmp("volatile", cp, n) == 0))
    {
      if (strncmp("static", cp, n) == 0)
      {
        static_bits = SVTK_PARSE_STATIC;
      }
    }
    else if (n == 5 && strncmp(cp, "const", n) == 0)
    {
      const_bits |= SVTK_PARSE_CONST;
    }
    else if (n == 8 && strncmp(cp, "unsigned", n) == 0)
    {
      unsigned_bits |= SVTK_PARSE_UNSIGNED;
      if (base_bits == 0)
      {
        classname = "int";
        base_bits = SVTK_PARSE_INT;
      }
    }
    else if (n == 6 && strncmp(cp, "signed", n) == 0)
    {
      if (base_bits == SVTK_PARSE_CHAR)
      {
        classname = "signed char";
        base_bits = SVTK_PARSE_SIGNED_CHAR;
      }
      else
      {
        classname = "int";
        base_bits = SVTK_PARSE_INT;
      }
    }
    else if (n == 3 && strncmp(cp, "int", n) == 0)
    {
      if (base_bits == 0)
      {
        classname = "int";
        base_bits = SVTK_PARSE_INT;
      }
    }
    else if (n == 4 && strncmp(cp, "long", n) == 0)
    {
      if (base_bits == SVTK_PARSE_DOUBLE)
      {
        classname = "long double";
        base_bits = SVTK_PARSE_LONG_DOUBLE;
      }
      else if (base_bits == SVTK_PARSE_LONG)
      {
        classname = "long long";
        base_bits = SVTK_PARSE_LONG_LONG;
      }
      else
      {
        classname = "long";
        base_bits = SVTK_PARSE_LONG;
      }
    }
    else if (n == 5 && strncmp(cp, "short", n) == 0)
    {
      classname = "short";
      base_bits = SVTK_PARSE_SHORT;
    }
    else if (n == 4 && strncmp(cp, "char", n) == 0)
    {
      if (base_bits == SVTK_PARSE_INT && unsigned_bits != SVTK_PARSE_UNSIGNED)
      {
        classname = "signed char";
        base_bits = SVTK_PARSE_SIGNED_CHAR;
      }
      else
      {
        classname = "char";
        base_bits = SVTK_PARSE_CHAR;
      }
    }
    else if (n == 5 && strncmp(cp, "float", n) == 0)
    {
      classname = "float";
      base_bits = SVTK_PARSE_FLOAT;
    }
    else if (n == 6 && strncmp(cp, "double", n) == 0)
    {
      if (base_bits == SVTK_PARSE_LONG)
      {
        classname = "long double";
        base_bits = SVTK_PARSE_LONG_DOUBLE;
      }
      else
      {
        classname = "double";
        base_bits = SVTK_PARSE_DOUBLE;
      }
    }
    else if (n == 4 && strncmp(cp, "bool", n) == 0)
    {
      classname = "bool";
      base_bits = SVTK_PARSE_BOOL;
    }
    else if (n == 4 && strncmp(cp, "void", n) == 0)
    {
      classname = "void";
      base_bits = SVTK_PARSE_VOID;
    }
    else if (n == 7 && strncmp(cp, "__int64", n) == 0)
    {
      classname = "__int64";
      base_bits = SVTK_PARSE___INT64;
    }
    else
    {
      /* if type already found, break */
      if (base_bits != 0)
      {
        break;
      }

      /* check svtk typedefs */
      if (strncmp(cp, "svtk", 3) == 0)
      {
        for (i = 0; svtktypes[i].len != 0; i++)
        {
          if (n == svtktypes[i].len && strncmp(cp, svtktypes[i].name, n) == 0)
          {
            classname = svtktypes[i].name;
            base_bits = svtktypes[i].type;
          }
        }
      }

      /* check standard typedefs */
      if (base_bits == 0)
      {
        m = 0;
        if (strncmp(cp, "::", 2) == 0)
        {
          m = 2;
        }
        else if (strncmp(cp, "std::", 5) == 0)
        {
          m = 5;
        }

        /* advance past the namespace */
        tmpcp = cp + m;

        for (i = 0; stdtypes[i].len != 0; i++)
        {
          if (n == m + stdtypes[i].len && strncmp(tmpcp, stdtypes[i].name, n - m) == 0)
          {
            classname = stdtypes[i].name;
            base_bits = stdtypes[i].type;
          }
        }

        /* include the namespace if present */
        if (base_bits != 0 && m > 0)
        {
          classname = cp;
          len = n;
        }
      }

      /* anything else is assumed to be a class, enum, or who knows */
      if (base_bits == 0)
      {
        base_bits = SVTK_PARSE_UNKNOWN;
        classname = cp;
        len = n;

        /* SVTK classes all start with svtk */
        if (strncmp(classname, "svtk", 3) == 0)
        {
          base_bits = SVTK_PARSE_OBJECT;
          /* make sure the "svtk" isn't just part of the namespace */
          for (k = 0; k < n; k++)
          {
            if (cp[k] == ':')
            {
              base_bits = SVTK_PARSE_UNKNOWN;
              break;
            }
          }
        }
        /* Qt objects and enums */
        else if (classname[0] == 'Q' &&
          ((classname[1] >= 'A' && classname[2] <= 'Z') || strncmp(classname, "Qt::", 4) == 0))
        {
          base_bits = SVTK_PARSE_QOBJECT;
        }
      }
    }

    cp += n;
    while (svtkParse_CharType(*cp, CPRE_HSPACE))
    {
      cp++;
    }
  }

  if ((unsigned_bits & SVTK_PARSE_UNSIGNED) != 0)
  {
    switch (base_bits)
    {
      case SVTK_PARSE_CHAR:
        classname = "unsigned char";
        break;
      case SVTK_PARSE_SHORT:
        classname = "unsigned short";
        break;
      case SVTK_PARSE_INT:
        classname = "unsigned int";
        break;
      case SVTK_PARSE_LONG:
        classname = "unsigned long";
        break;
      case SVTK_PARSE_LONG_LONG:
        classname = "unsigned long long";
        break;
      case SVTK_PARSE___INT64:
        classname = "unsigned __int64";
        break;
    }
  }

  *type_ptr = (static_bits | const_bits | unsigned_bits | base_bits);

  if (classname_ptr)
  {
    *classname_ptr = classname;
    if (classname && len == 0)
    {
      len = strlen(classname);
    }
    *len_ptr = len;
  }

  return (size_t)(cp - text);
}

/* Parse a type description in "text" and generate a typedef named "name" */
size_t svtkParse_ValueInfoFromString(ValueInfo* data, StringCache* cache, const char* text)
{
  const char* cp = text;
  size_t n;
  int m, count;
  unsigned int base_bits = 0;
  unsigned int pointer_bits = 0;
  unsigned int ref_bits = 0;
  const char* classname = NULL;

  /* get the basic type with qualifiers */
  cp += svtkParse_BasicTypeFromString(cp, &base_bits, &classname, &n);

  data->Class = svtkParse_CacheString(cache, classname, n);

  if ((base_bits & SVTK_PARSE_STATIC) != 0)
  {
    data->IsStatic = 1;
  }

  /* look for pointers (and const pointers) */
  while (*cp == '*')
  {
    cp++;
    pointer_bits = (pointer_bits << 2);
    while (svtkParse_CharType(*cp, CPRE_HSPACE))
    {
      cp++;
    }
    if (strncmp(cp, "const", 5) == 0 && !svtkParse_CharType(cp[5], CPRE_XID))
    {
      cp += 5;
      while (svtkParse_CharType(*cp, CPRE_HSPACE))
      {
        cp++;
      }
      pointer_bits = (pointer_bits | SVTK_PARSE_CONST_POINTER);
    }
    else
    {
      pointer_bits = (pointer_bits | SVTK_PARSE_POINTER);
    }
    pointer_bits = (pointer_bits & SVTK_PARSE_POINTER_MASK);
  }

  /* look for ref */
  if (*cp == '&')
  {
    cp++;
    while (svtkParse_CharType(*cp, CPRE_HSPACE))
    {
      cp++;
    }
    ref_bits = SVTK_PARSE_REF;
  }

  /* look for the variable name */
  if (svtkParse_CharType(*cp, CPRE_ID))
  {
    /* skip all chars that are part of a name */
    n = svtkParse_SkipId(cp);
    data->Name = svtkParse_CacheString(cache, cp, n);
    cp += n;
    while (svtkParse_CharType(*cp, CPRE_HSPACE))
    {
      cp++;
    }
  }

  /* look for array brackets */
  /* (should also look for parenthesized parameter list, for func types) */
  if (*cp == '[')
  {
    count = 1;

    while (*cp == '[')
    {
      n = svtkparse_bracket_len(cp);
      if (n > 1)
      {
        cp++;
        n -= 2;
      }
      while (svtkParse_CharType(*cp, CPRE_HSPACE))
      {
        cp++;
        n--;
      }
      while (n > 0 && svtkParse_CharType(cp[n - 1], CPRE_HSPACE))
      {
        n--;
      }
      svtkParse_AddStringToArray(
        &data->Dimensions, &data->NumberOfDimensions, svtkParse_CacheString(cache, cp, n));
      m = 0;
      if (svtkParse_CharType(*cp, CPRE_DIGIT) && svtkParse_SkipNumber(cp) == n)
      {
        m = (int)strtol(cp, NULL, 0);
      }
      count *= m;

      cp += n;
      while (svtkParse_CharType(*cp, CPRE_HSPACE))
      {
        cp++;
      }
      if (*cp == ']')
      {
        cp++;
      }
      while (svtkParse_CharType(*cp, CPRE_HSPACE))
      {
        cp++;
      }
    }
  }

  /* add pointer indirection to correspond to first array dimension */
  if (data->NumberOfDimensions > 1)
  {
    pointer_bits = ((pointer_bits << 2) | SVTK_PARSE_ARRAY);
  }
  else if (data->NumberOfDimensions == 1)
  {
    pointer_bits = ((pointer_bits << 2) | SVTK_PARSE_POINTER);
  }
  pointer_bits = (pointer_bits & SVTK_PARSE_POINTER_MASK);

  /* (Add code here to look for "=" followed by a value ) */

  data->Type = (pointer_bits | ref_bits | base_bits);

  return (cp - text);
}

/* Generate a C++ declaration string from a ValueInfo struct */
size_t svtkParse_ValueInfoToString(ValueInfo* data, char* text, unsigned int flags)
{
  unsigned int pointer_bits = (data->Type & SVTK_PARSE_POINTER_MASK);
  unsigned int ref_bits = (data->Type & (SVTK_PARSE_REF | SVTK_PARSE_RVALUE));
  unsigned int qualifier_bits = (data->Type & SVTK_PARSE_CONST);
  unsigned int reverse_bits = 0;
  unsigned int pointer_type = 0;
  const char* tpname = data->Class;
  int dimensions = data->NumberOfDimensions;
  int pointer_dimensions = 0;
  size_t i = 0;
  int j = 0;

  /* check if this is a type parameter for a template */
  if (!tpname)
  {
    tpname = "class";
  }

  /* don't shown anything that isn't in the flags */
  ref_bits &= flags;
  qualifier_bits &= flags;

  /* if this is to be a return value, [] becomes * */
  if ((flags & SVTK_PARSE_ARRAY) == 0 && pointer_bits == SVTK_PARSE_POINTER)
  {
    if (dimensions == 1)
    {
      dimensions = 0;
    }
  }

  if (!data->Function && (qualifier_bits & SVTK_PARSE_CONST) != 0)
  {
    if (text)
    {
      strcpy(&text[i], "const ");
    }
    i += 6;
  }

  if (data->Function)
  {
    if (text)
    {
      i += svtkParse_FunctionInfoToString(data->Function, &text[i], SVTK_PARSE_RETURN_VALUE);
      text[i++] = '(';
      if (data->Function->Class)
      {
        strcpy(&text[i], data->Function->Class);
        i += strlen(data->Function->Class);
        text[i++] = ':';
        text[i++] = ':';
      }
    }
    else
    {
      i += svtkParse_FunctionInfoToString(data->Function, NULL, SVTK_PARSE_RETURN_VALUE);
      i += 1;
      if (data->Function->Class)
      {
        i += strlen(data->Function->Class);
        i += 2;
      }
    }
  }
  else
  {
    if (text)
    {
      strcpy(&text[i], tpname);
    }
    i += strlen(tpname);
    if (text)
    {
      text[i] = ' ';
    }
    i++;
  }

  while (pointer_bits != 0)
  {
    reverse_bits <<= 2;
    reverse_bits |= (pointer_bits & SVTK_PARSE_POINTER_LOWMASK);
    pointer_bits = ((pointer_bits >> 2) & SVTK_PARSE_POINTER_MASK);
  }

  while (reverse_bits != 0)
  {
    pointer_type = (reverse_bits & SVTK_PARSE_POINTER_LOWMASK);
    if (pointer_type == SVTK_PARSE_ARRAY || (reverse_bits == SVTK_PARSE_POINTER && dimensions > 0))
    {
      if ((flags & SVTK_PARSE_ARRAY) == 0)
      {
        pointer_dimensions = 1;
        if (text)
        {
          text[i] = '(';
          text[i + 1] = '*';
        }
        i += 2;
      }
      break;
    }
    else if (pointer_type == SVTK_PARSE_POINTER)
    {
      if (text)
      {
        text[i] = '*';
      }
      i++;
    }
    else if (pointer_type == SVTK_PARSE_CONST_POINTER)
    {
      if (text)
      {
        strcpy(&text[i], "*const ");
      }
      i += 7;
    }

    reverse_bits = ((reverse_bits >> 2) & SVTK_PARSE_POINTER_MASK);
  }

  if ((ref_bits & SVTK_PARSE_REF) != 0)
  {
    if ((ref_bits & SVTK_PARSE_RVALUE) != 0)
    {
      if (text)
      {
        text[i] = '&';
      }
      i++;
    }
    if (text)
    {
      text[i] = '&';
    }
    i++;
  }

  if (data->Name && (flags & SVTK_PARSE_NAMES) != 0)
  {
    if (text)
    {
      strcpy(&text[i], data->Name);
    }
    i += strlen(data->Name);
    if (data->Value && (flags & SVTK_PARSE_VALUES) != 0)
    {
      if (text)
      {
        text[i] = '=';
      }
      i++;
      if (text)
      {
        strcpy(&text[i], data->Value);
      }
      i += strlen(data->Value);
    }
  }

  for (j = 0; j < pointer_dimensions; j++)
  {
    if (text)
    {
      text[i] = ')';
    }
    i++;
  }

  for (j = pointer_dimensions; j < dimensions; j++)
  {
    if (text)
    {
      text[i] = '[';
    }
    i++;
    if (data->Dimensions[j])
    {
      if (text)
      {
        strcpy(&text[i], data->Dimensions[j]);
      }
      i += strlen(data->Dimensions[j]);
    }
    if (text)
    {
      text[i] = ']';
    }
    i++;
  }

  if (data->Function)
  {
    if (text)
    {
      text[i++] = ')';
      i += svtkParse_FunctionInfoToString(
        data->Function, &text[i], SVTK_PARSE_CONST | SVTK_PARSE_PARAMETER_LIST);
    }
    else
    {
      i++;
      i += svtkParse_FunctionInfoToString(
        data->Function, NULL, SVTK_PARSE_CONST | SVTK_PARSE_PARAMETER_LIST);
    }
  }

  if (text)
  {
    text[i] = '\0';
  }

  return i;
}

/* Generate a template declaration string */
size_t svtkParse_TemplateInfoToString(TemplateInfo* data, char* text, unsigned int flags)
{
  int i;
  size_t k = 0;

  if (text)
  {
    strcpy(&text[k], "template<");
  }
  k += 9;
  for (i = 0; i < data->NumberOfParameters; i++)
  {
    if (i != 0)
    {
      if (text)
      {
        text[k] = ',';
        text[k + 1] = ' ';
      }
      k += 2;
    }
    if (text)
    {
      k += svtkParse_ValueInfoToString(data->Parameters[i], &text[k], flags);
      while (k > 0 && text[k - 1] == ' ')
      {
        k--;
      }
    }
    else
    {
      k += svtkParse_ValueInfoToString(data->Parameters[i], NULL, flags);
    }
  }
  if (text)
  {
    text[k] = '>';
    text[k + 1] = '\0';
  }
  k++;

  return k;
}

/* generate a function signature for a FunctionInfo struct */
size_t svtkParse_FunctionInfoToString(FunctionInfo* func, char* text, unsigned int flags)
{
  int i;
  size_t k = 0;

  if (func->Template && (flags & SVTK_PARSE_TEMPLATES) != 0)
  {
    if (text)
    {
      k += svtkParse_TemplateInfoToString(func->Template, &text[k], flags);
      text[k++] = ' ';
    }
    else
    {
      k += svtkParse_TemplateInfoToString(func->Template, NULL, flags);
      k++;
    }
  }

  if (func->IsStatic && (flags & SVTK_PARSE_STATIC) != 0)
  {
    if (text)
    {
      strcpy(&text[k], "static ");
    }
    k += 7;
  }
  if (func->IsVirtual && (flags & SVTK_PARSE_VIRTUAL) != 0)
  {
    if (text)
    {
      strcpy(&text[k], "virtual ");
    }
    k += 8;
  }
  if (func->IsExplicit && (flags & SVTK_PARSE_EXPLICIT) != 0)
  {
    if (text)
    {
      strcpy(&text[k], "explicit ");
    }
    k += 9;
  }

  if (func->ReturnValue && (flags & SVTK_PARSE_RETURN_VALUE) != 0)
  {
    if (text)
    {
      k += svtkParse_ValueInfoToString(
        func->ReturnValue, &text[k], SVTK_PARSE_EVERYTHING ^ (SVTK_PARSE_ARRAY | SVTK_PARSE_NAMES));
    }
    else
    {
      k += svtkParse_ValueInfoToString(
        func->ReturnValue, NULL, SVTK_PARSE_EVERYTHING ^ (SVTK_PARSE_ARRAY | SVTK_PARSE_NAMES));
    }
  }

  if ((flags & SVTK_PARSE_RETURN_VALUE) != 0 && (flags & SVTK_PARSE_PARAMETER_LIST) != 0)
  {
    if (func->Name)
    {
      if (text)
      {
        strcpy(&text[k], func->Name);
      }
      k += strlen(func->Name);
    }
    else
    {
      if (text)
      {
        text[k++] = '(';
        if (func->Class)
        {
          strcpy(&text[k], func->Class);
          k += strlen(func->Class);
          text[k++] = ':';
          text[k++] = ':';
        }
        text[k++] = '*';
        text[k++] = ')';
      }
      else
      {
        k++;
        if (func->Class)
        {
          k += strlen(func->Class);
          k += 2;
        }
        k += 2;
      }
    }
  }
  if ((flags & SVTK_PARSE_PARAMETER_LIST) != 0)
  {
    if (text)
    {
      text[k] = '(';
    }
    k++;
    for (i = 0; i < func->NumberOfParameters; i++)
    {
      if (i != 0)
      {
        if (text)
        {
          text[k] = ',';
          text[k + 1] = ' ';
        }
        k += 2;
      }
      if (text)
      {
        k += svtkParse_ValueInfoToString(func->Parameters[i], &text[k],
          (SVTK_PARSE_EVERYTHING ^ (SVTK_PARSE_NAMES | SVTK_PARSE_VALUES)) |
            (flags & (SVTK_PARSE_NAMES | SVTK_PARSE_VALUES)));
        while (k > 0 && text[k - 1] == ' ')
        {
          k--;
        }
      }
      else
      {
        k += svtkParse_ValueInfoToString(func->Parameters[i], NULL,
          (SVTK_PARSE_EVERYTHING ^ (SVTK_PARSE_NAMES | SVTK_PARSE_VALUES)) |
            (flags & (SVTK_PARSE_NAMES | SVTK_PARSE_VALUES)));
      }
    }
    if (text)
    {
      text[k] = ')';
    }
    k++;
  }
  if (func->IsConst && (flags & SVTK_PARSE_CONST) != 0)
  {
    if (text)
    {
      strcpy(&text[k], " const");
    }
    k += 6;
  }
  if (func->IsFinal && (flags & SVTK_PARSE_TRAILERS) != 0)
  {
    if (text)
    {
      strcpy(&text[k], " final");
    }
    k += 6;
  }
  if (func->IsPureVirtual && (flags & SVTK_PARSE_TRAILERS) != 0)
  {
    if (text)
    {
      strcpy(&text[k], " = 0");
    }
    k += 4;
  }
  if (text)
  {
    text[k] = '\0';
  }

  return k;
}

/* compare two types to see if they are equivalent */
static int override_compatible(unsigned int t1, unsigned int t2)
{
  /* const and virtual qualifiers are part of the type for the
     sake of method resolution, but only if the type is a pointer
     or reference */
  unsigned int typebits =
    (SVTK_PARSE_UNQUALIFIED_TYPE | SVTK_PARSE_CONST | SVTK_PARSE_VOLATILE | SVTK_PARSE_RVALUE);
  unsigned int diff = (t1 ^ t2) & typebits;
  return (
    diff == 0 || ((t1 & SVTK_PARSE_INDIRECT) == 0 && (diff & SVTK_PARSE_UNQUALIFIED_TYPE) == 0));
}

/* Compare two functions */
int svtkParse_CompareFunctionSignature(const FunctionInfo* func1, const FunctionInfo* func2)
{
  ValueInfo* p1;
  ValueInfo* p2;
  int j;
  int k;
  int match = 0;

  /* uninstantiated templates cannot be compared */
  if (func1->Template || func2->Template)
  {
    return 0;
  }

  /* check the parameters */
  if (func2->NumberOfParameters == func1->NumberOfParameters)
  {
    for (k = 0; k < func2->NumberOfParameters; k++)
    {
      p1 = func1->Parameters[k];
      p2 = func2->Parameters[k];
      if (!override_compatible(p2->Type, p1->Type) || strcmp(p2->Class, p1->Class) != 0)
      {
        break;
      }
      if (p1->Function && p2->Function)
      {
        if (svtkParse_CompareFunctionSignature(p1->Function, p2->Function) < 7)
        {
          break;
        }
      }
      if (p1->NumberOfDimensions > 1 || p2->NumberOfDimensions > 1)
      {
        if (p1->NumberOfDimensions != p2->NumberOfDimensions)
        {
          break;
        }
        for (j = 1; j < p1->NumberOfDimensions; j++)
        {
          if (strcmp(p1->Dimensions[j], p2->Dimensions[j]) != 0)
          {
            break;
          }
        }
      }
    }
    if (k == func2->NumberOfParameters)
    {
      match = 1;
    }
  }

  /* check the return value */
  if (match && func1->ReturnValue && func2->ReturnValue)
  {
    p1 = func1->ReturnValue;
    p2 = func2->ReturnValue;
    if (override_compatible(p2->Type, p1->Type) && strcmp(p2->Class, p1->Class) == 0)
    {
      if (p1->Function && p2->Function)
      {
        if (svtkParse_CompareFunctionSignature(p1->Function, p2->Function) < 7)
        {
          match |= 2;
        }
      }
      else
      {
        match |= 2;
      }
    }
  }

  /* check the class */
  if (match && func1->Class && func2->Class && strcmp(func1->Class, func2->Class) == 0)
  {
    if (func1->IsConst == func2->IsConst)
    {
      match |= 4;
    }
  }

  return match;
}

/* Search and replace, return the initial string if no replacements
 * occurred, otherwise return a new string allocated with malloc. */
const char* svtkParse_StringReplace(const char* str1, int n, const char* name[], const char* val[])
{
  return svtkparse_string_replace(NULL, str1, n, name, val);
}

/* substitute generic types and values with actual types and values */
static void func_substitution(FunctionInfo* data, StringCache* cache, int m,
  const char* arg_names[], const char* arg_values[], ValueInfo* arg_types[]);

static void value_substitution(ValueInfo* data, StringCache* cache, int m, const char* arg_names[],
  const char* arg_values[], ValueInfo* arg_types[])
{
  svtkParse_ExpandTypedefs(data, cache, m, arg_names, arg_values, arg_types);
  svtkParse_ExpandValues(data, cache, m, arg_names, arg_values);

  if (data->Function)
  {
    func_substitution(data->Function, cache, m, arg_names, arg_values, arg_types);
  }
}

static void func_substitution(FunctionInfo* data, StringCache* cache, int m,
  const char* arg_names[], const char* arg_values[], ValueInfo* arg_types[])
{
  int i, n;

  n = data->NumberOfParameters;
  for (i = 0; i < n; i++)
  {
    value_substitution(data->Parameters[i], cache, m, arg_names, arg_values, arg_types);
  }

  if (data->ReturnValue)
  {
    value_substitution(data->ReturnValue, cache, m, arg_names, arg_values, arg_types);
  }

  if (data->Signature)
  {
    data->Signature = svtkparse_string_replace(cache, data->Signature, m, arg_names, arg_values);
  }

  /* legacy information for old wrappers */
#ifndef SVTK_PARSE_LEGACY_REMOVE
  n = data->NumberOfArguments;
  for (i = 0; i < n; i++)
  {
    data->ArgTypes[i] = data->Parameters[i]->Type;
    data->ArgClasses[i] = data->Parameters[i]->Class;
    if (data->Parameters[i]->NumberOfDimensions == 1 && data->Parameters[i]->Count > 0)
    {
      data->ArgCounts[i] = data->Parameters[i]->Count;
    }
  }

  if (data->ReturnValue)
  {
    data->ReturnType = data->ReturnValue->Type;
    data->ReturnClass = data->ReturnValue->Class;
    if (data->ReturnValue->NumberOfDimensions == 1 && data->ReturnValue->Count > 0)
    {
      data->HintSize = data->ReturnValue->Count;
      data->HaveHint = 1;
    }
  }
#endif /* SVTK_PARSE_LEGACY_REMOVE */
}

static void class_substitution(ClassInfo* data, StringCache* cache, int m, const char* arg_names[],
  const char* arg_values[], ValueInfo* arg_types[])
{
  int i, n;

  /* superclasses may be templated */
  n = data->NumberOfSuperClasses;
  for (i = 0; i < n; i++)
  {
    data->SuperClasses[i] =
      svtkparse_string_replace(cache, data->SuperClasses[i], m, arg_names, arg_values);
  }

  n = data->NumberOfClasses;
  for (i = 0; i < n; i++)
  {
    class_substitution(data->Classes[i], cache, m, arg_names, arg_values, arg_types);
  }

  n = data->NumberOfFunctions;
  for (i = 0; i < n; i++)
  {
    func_substitution(data->Functions[i], cache, m, arg_names, arg_values, arg_types);
  }

  n = data->NumberOfConstants;
  for (i = 0; i < n; i++)
  {
    value_substitution(data->Constants[i], cache, m, arg_names, arg_values, arg_types);
  }

  n = data->NumberOfVariables;
  for (i = 0; i < n; i++)
  {
    value_substitution(data->Variables[i], cache, m, arg_names, arg_values, arg_types);
  }

  n = data->NumberOfTypedefs;
  for (i = 0; i < n; i++)
  {
    value_substitution(data->Typedefs[i], cache, m, arg_names, arg_values, arg_types);
  }
}

/* Extract template args from a comma-separated list enclosed
 * in angle brackets.  Returns zero if no angle brackets found. */
size_t svtkParse_DecomposeTemplatedType(
  const char* text, const char** classname, int nargs, const char*** argp, const char* defaults[])
{
  size_t i, j, k, n;
  const char* arg;
  char* new_text;
  const char** template_args = NULL;
  int template_arg_count = 0;

  n = svtkParse_NameLength(text);

  /* is the class templated? */
  for (i = 0; i < n; i++)
  {
    if (text[i] == '<')
    {
      break;
    }
  }

  if (classname)
  {
    new_text = (char*)malloc(i + 1);
    strncpy(new_text, text, i);
    new_text[i] = '\0';
    *classname = new_text;
  }

  if (text[i] == '<')
  {
    i++;
    /* extract the template arguments */
    for (;;)
    {
      while (svtkParse_CharType(text[i], CPRE_HSPACE))
      {
        i++;
      }
      j = i;
      while (text[j] != ',' && text[j] != '>' && text[j] != '\n' && text[j] != '\0')
      {
        if (text[j] == '<' || text[j] == '(' || text[j] == '[' || text[j] == '{')
        {
          j += svtkparse_bracket_len(&text[j]);
        }
        else if (svtkParse_CharType(text[j], CPRE_QUOTE))
        {
          j += svtkParse_SkipQuotes(&text[j]);
        }
        else
        {
          j++;
        }
      }

      k = j;
      while (svtkParse_CharType(text[k - 1], CPRE_HSPACE))
      {
        --k;
      }

      new_text = (char*)malloc(k - i + 1);
      strncpy(new_text, &text[i], k - i);
      new_text[k - i] = '\0';
      svtkParse_AddStringToArray(&template_args, &template_arg_count, new_text);

      assert(template_arg_count <= nargs);

      i = j + 1;

      if (text[j] != ',')
      {
        break;
      }
    }
  }

  while (template_arg_count < nargs)
  {
    assert(defaults != NULL);
    arg = defaults[template_arg_count];
    assert(arg != NULL);
    new_text = (char*)malloc(strlen(arg) + 1);
    strcpy(new_text, arg);
    svtkParse_AddStringToArray(&template_args, &template_arg_count, new_text);
  }

  *argp = template_args;

  return i;
}

/* Free the list of strings returned by ExtractTemplateArgs.  */
void svtkParse_FreeTemplateDecomposition(const char* name, int n, const char** args)
{
  int i;

  if (name)
  {
    free((char*)name);
  }

  if (n > 0)
  {
    for (i = 0; i < n; i++)
    {
      free((char*)args[i]);
    }

    free((char**)args);
  }
}

/* Instantiate a class template by substituting the provided arguments. */
void svtkParse_InstantiateClassTemplate(
  ClassInfo* data, StringCache* cache, int n, const char* args[])
{
  TemplateInfo* t = data->Template;
  const char** new_args = NULL;
  const char** arg_names = NULL;
  ValueInfo** arg_types = NULL;
  int i, m;
  char* new_name;
  size_t k;

  if (t == NULL)
  {
    fprintf(stderr,
      "svtkParse_InstantiateClassTemplate: "
      "this class is not templated.\n");
    return;
  }

  m = t->NumberOfParameters;
  if (n > m)
  {
    fprintf(stderr,
      "svtkParse_InstantiateClassTemplate: "
      "too many template args.\n");
    return;
  }

  for (i = n; i < m; i++)
  {
    if (t->Parameters[i]->Value == NULL || t->Parameters[i]->Value[0] == '\0')
    {
      fprintf(stderr,
        "svtkParse_InstantiateClassTemplate: "
        "too few template args.\n");
      return;
    }
  }

  new_args = (const char**)malloc(m * sizeof(char**));
  for (i = 0; i < n; i++)
  {
    new_args[i] = args[i];
  }
  for (i = n; i < m; i++)
  {
    new_args[i] = t->Parameters[i]->Value;
  }
  args = new_args;

  arg_names = (const char**)malloc(m * sizeof(char**));
  arg_types = (ValueInfo**)malloc(m * sizeof(ValueInfo*));
  for (i = 0; i < m; i++)
  {
    arg_names[i] = t->Parameters[i]->Name;
    arg_types[i] = NULL;
    if (t->Parameters[i]->Type == 0)
    {
      arg_types[i] = (ValueInfo*)malloc(sizeof(ValueInfo));
      svtkParse_InitValue(arg_types[i]);
      svtkParse_ValueInfoFromString(arg_types[i], cache, args[i]);
      arg_types[i]->ItemType = SVTK_TYPEDEF_INFO;
      arg_types[i]->Name = arg_names[i];
    }
  }

  /* no longer a template (has been instantiated) */
  if (data->Template)
  {
    svtkParse_FreeTemplate(data->Template);
  }
  data->Template = NULL;

  /* append template args to class name */
  k = strlen(data->Name) + 2;
  for (i = 0; i < m; i++)
  {
    k += strlen(args[i]) + 2;
  }
  new_name = (char*)malloc(k);
  strcpy(new_name, data->Name);
  k = strlen(new_name);
  new_name[k++] = '<';
  for (i = 0; i < m; i++)
  {
    strcpy(&new_name[k], args[i]);
    k += strlen(args[i]);
    if (i + 1 < m)
    {
      new_name[k++] = ',';
      new_name[k++] = ' ';
    }
  }
  if (new_name[k - 1] == '>')
  {
    new_name[k++] = ' ';
  }
  new_name[k++] = '>';
  new_name[k] = '\0';

  data->Name = svtkParse_CacheString(cache, new_name, k);
  free(new_name);

  /* do the template arg substitution */
  class_substitution(data, cache, m, arg_names, args, arg_types);

  /* free all allocated arrays */
  free((char**)new_args);
  free((char**)arg_names);

  for (i = 0; i < m; i++)
  {
    if (arg_types[i])
    {
      svtkParse_FreeValue(arg_types[i]);
    }
  }
  free(arg_types);
}

/* Get a zero-terminated array of the types in svtkTemplateMacro. */
const char** svtkParse_GetTemplateMacroTypes(void)
{
  static const char* types[] = { "char", "signed char", "unsigned char", "short", "unsigned short",
    "int", "unsigned int", "long", "unsigned long", "long long", "unsigned long long", "float",
    "double", NULL };

  return types;
}

/* Get a zero-terminated array of the types in svtkArray. */
const char** svtkParse_GetArrayTypes(void)
{
  static const char* types[] = { "char", "signed char", "unsigned char", "short", "unsigned short",
    "int", "unsigned int", "long", "unsigned long", "long long", "unsigned long long", "float",
    "double", "svtkStdString", "svtkUnicodeString", "svtkVariant", NULL };

  return types;
}
