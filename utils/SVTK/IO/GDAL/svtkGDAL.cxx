/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkGDAL.cxx

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkGDAL.h"
#include "svtkInformationIntegerVectorKey.h"
#include "svtkInformationKey.h"
#include "svtkInformationStringKey.h"

svtkInformationKeyMacro(svtkGDAL, MAP_PROJECTION, String);
svtkInformationKeyRestrictedMacro(svtkGDAL, FLIP_AXIS, IntegerVector, 3);
