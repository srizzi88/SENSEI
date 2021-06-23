/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLogLookupTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLogLookupTable
 * @brief   map scalars into colors using log (base 10) scale
 *
 * This class is an empty shell.  Use svtkLookupTable with SetScaleToLog10()
 * instead.
 *
 * @sa
 * svtkLookupTable
 */

#ifndef svtkLogLookupTable_h
#define svtkLogLookupTable_h

#include "svtkLookupTable.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkLogLookupTable : public svtkLookupTable
{
public:
  static svtkLogLookupTable* New();

  svtkTypeMacro(svtkLogLookupTable, svtkLookupTable);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkLogLookupTable(int sze = 256, int ext = 256);
  ~svtkLogLookupTable() override {}

private:
  svtkLogLookupTable(const svtkLogLookupTable&) = delete;
  void operator=(const svtkLogLookupTable&) = delete;
};

#endif
