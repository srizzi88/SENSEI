/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAddMembershipArray.h

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
 * @class   svtkAddMembershipArray
 * @brief   Add an array to the output indicating
 * membership within an input selection.
 *
 *
 * This filter takes an input selection, svtkDataSetAttribute
 * information, and data object and adds a bit array to the output
 * svtkDataSetAttributes indicating whether each index was selected or not.
 */

#ifndef svtkAddMembershipArray_h
#define svtkAddMembershipArray_h

#include "svtkInfovisCoreModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class svtkAbstractArray;

class SVTKINFOVISCORE_EXPORT svtkAddMembershipArray : public svtkPassInputTypeAlgorithm
{
public:
  static svtkAddMembershipArray* New();
  svtkTypeMacro(svtkAddMembershipArray, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  enum
  {
    FIELD_DATA = 0,
    POINT_DATA = 1,
    CELL_DATA = 2,
    VERTEX_DATA = 3,
    EDGE_DATA = 4,
    ROW_DATA = 5
  };

  //@{
  /**
   * The field type to add the membership array to.
   */
  svtkGetMacro(FieldType, int);
  svtkSetClampMacro(FieldType, int, 0, 5);
  //@}

  //@{
  /**
   * The name of the array added to the output svtkDataSetAttributes
   * indicating membership. Defaults to "membership".
   */
  svtkSetStringMacro(OutputArrayName);
  svtkGetStringMacro(OutputArrayName);
  //@}

  svtkSetStringMacro(InputArrayName);
  svtkGetStringMacro(InputArrayName);

  void SetInputValues(svtkAbstractArray*);
  svtkGetObjectMacro(InputValues, svtkAbstractArray);

protected:
  svtkAddMembershipArray();
  ~svtkAddMembershipArray() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FieldType;
  char* OutputArrayName;
  char* InputArrayName;

  svtkAbstractArray* InputValues;

private:
  svtkAddMembershipArray(const svtkAddMembershipArray&) = delete;
  void operator=(const svtkAddMembershipArray&) = delete;
};

#endif
