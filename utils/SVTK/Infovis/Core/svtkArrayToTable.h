/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkArrayToTable.h

-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkArrayToTable
 * @brief   Converts one- and two-dimensional svtkArrayData
 * objects to svtkTable
 *
 * @par Thanks:
 * Developed by Timothy M. Shead (tshead@sandia.gov) at Sandia National Laboratories.
 */

#ifndef svtkArrayToTable_h
#define svtkArrayToTable_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class SVTKINFOVISCORE_EXPORT svtkArrayToTable : public svtkTableAlgorithm
{
public:
  static svtkArrayToTable* New();
  svtkTypeMacro(svtkArrayToTable, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkArrayToTable();
  ~svtkArrayToTable() override;

  int FillInputPortInformation(int, svtkInformation*) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkArrayToTable(const svtkArrayToTable&) = delete;
  void operator=(const svtkArrayToTable&) = delete;
};

#endif
