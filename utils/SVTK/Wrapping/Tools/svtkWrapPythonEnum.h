/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapPythonEnum.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkWrapPythonEnum_h
#define svtkWrapPythonEnum_h

#include "svtkParse.h"
#include "svtkParseData.h"
#include "svtkParseHierarchy.h"

/* check whether an enum type will be wrapped */
int svtkWrapPython_IsEnumWrapped(HierarchyInfo* hinfo, const char* enumname);

/* find and mark all enum parameters by setting IsEnum=1 */
void svtkWrapPython_MarkAllEnums(NamespaceInfo* contents, HierarchyInfo* hinfo);

/* write out an enum type wrapped in python */
void svtkWrapPython_GenerateEnumType(
  FILE* fp, const char* module, const char* classname, EnumInfo* data);

/* generate code that adds an enum type to a python dict */
void svtkWrapPython_AddEnumType(FILE* fp, const char* indent, const char* dictvar,
  const char* objvar, const char* scope, EnumInfo* cls);

/* generate code that adds all public enum types to a python dict */
void svtkWrapPython_AddPublicEnumTypes(
  FILE* fp, const char* indent, const char* dictvar, const char* objvar, NamespaceInfo* data);

#endif /* svtkWrapPythonEnum_h */
/* SVTK-HeaderTest-Exclude: svtkWrapPythonEnum.h */
