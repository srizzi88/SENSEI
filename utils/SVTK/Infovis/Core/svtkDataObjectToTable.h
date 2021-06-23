/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataObjectToTable.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkDataObjectToTable
 * @brief   extract field data as a table
 *
 *
 * This filter is used to extract either the field, cell or point data of
 * any data object as a table.
 */

#ifndef svtkDataObjectToTable_h
#define svtkDataObjectToTable_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class SVTKINFOVISCORE_EXPORT svtkDataObjectToTable : public svtkTableAlgorithm
{
public:
  static svtkDataObjectToTable* New();
  svtkTypeMacro(svtkDataObjectToTable, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum
  {
    FIELD_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4
  };

  //@{
  /**
   * The field type to copy into the output table.
   * Should be one of FIELD_DATA, POINT_DATA, CELL_DATA, VERTEX_DATA, EDGE_DATA.
   */
  svtkGetMacro(FieldType, int);
  svtkSetClampMacro(FieldType, int, 0, 4);
  //@}

protected:
  svtkDataObjectToTable();
  ~svtkDataObjectToTable() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FieldType;

private:
  svtkDataObjectToTable(const svtkDataObjectToTable&) = delete;
  void operator=(const svtkDataObjectToTable&) = delete;
};

#endif
