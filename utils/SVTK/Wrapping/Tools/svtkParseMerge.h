/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParseMerge.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright (c) 2010,2015 David Gobbi

  Contributed to the VisualizationToolkit by the author in March 2015
  under the terms of the Visualization Toolkit 2015 copyright.
-------------------------------------------------------------------------*/

/**
 This file contains utility functions for merging together the
 methods for a class with those inherited from all superclasses.
*/

#ifndef svtkParseMerge_h
#define svtkParseMerge_h

#include "svtkParseData.h"
#include "svtkParseHierarchy.h"
#include "svtkWrappingToolsModule.h"

/**
 * This struct is meant to supplement ClassInfo, it gives information
 * about which class (or classes) each method was inherited from
 */
typedef struct _MergeInfo
{
  int NumberOfClasses;     /* number of classes in genealogy */
  const char** ClassNames; /* class name */
  int NumberOfFunctions;   /* must match FunctionInfo */
  int* NumberOfOverrides;  /* n classes that define this function */
  int** OverrideClasses;   /* class for the override */
} MergeInfo;

#ifdef __cplusplus
extern "C"
{
#endif

  /**
   * Merge all inherited methods into the ClassInfo.
   * This will find and parse the header files for all the superclasses,
   * and recursively add all inherited superclass methods into one ClassInfo.
   * The returned MergeInfo object provides information about which class
   * each inherited method was inherited from.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  MergeInfo* svtkParseMerge_MergeSuperClasses(
    FileInfo* finfo, NamespaceInfo* data, ClassInfo* classInfo);

  /**
   * Create an initialized MergeInfo object.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  MergeInfo* svtkParseMerge_CreateMergeInfo(ClassInfo* classInfo);

  /**
   * Free the MergeInfo object.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParseMerge_FreeMergeInfo(MergeInfo* info);

  /**
   * Recursive suproutine to inherit methods from "classname".
   * The class named by "classname" should be a superclass of the ClassInfo.
   * If the MergeInfo is not provided (if it is NULL), then the only methods
   * that are brought into the class are ones that are explicitly named
   * by using declarations.  The hintfile can also be NULL, if there is
   * no hints file.
   */
  SVTKWRAPPINGTOOLS_EXPORT
  void svtkParseMerge_MergeHelper(FileInfo* finfo, const NamespaceInfo* data,
    const HierarchyInfo* hinfo, const char* classname, int nhintfiles, char** hintfiles,
    MergeInfo* info, ClassInfo* merge);

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif
/* SVTK-HeaderTest-Exclude: svtkParseMerge.h */
