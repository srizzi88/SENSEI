/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWrapPythonNamespace.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkWrapPythonNamespace_h
#define svtkWrapPythonNamespace_h

#include "svtkParse.h"
#include "svtkParseData.h"
#include "svtkParseHierarchy.h"

/* Wrap one class, returns zero if not wrappable */
int svtkWrapPython_WrapNamespace(FILE* fp, const char* module, NamespaceInfo* data);

#endif /* svtkWrapPythonNamespace_h */
/* SVTK-HeaderTest-Exclude: svtkWrapPythonNamespace.h */
