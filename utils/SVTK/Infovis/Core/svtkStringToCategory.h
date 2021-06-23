/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkStringToCategory.h

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
 * @class   svtkStringToCategory
 * @brief   Creates a category array from a string array
 *
 *
 * svtkStringToCategory creates an integer array named "category" based on the
 * values in a string array.  You may use this filter to create an array that
 * you may use to color points/cells by the values in a string array.  Currently
 * there is not support to color by a string array directly.
 * The category values will range from zero to N-1,
 * where N is the number of distinct strings in the string array.  Set the string
 * array to process with SetInputArrayToProcess(0,0,0,...).  The array may be in
 * the point, cell, or field data of the data object.
 *
 * The list of unique strings, in the order they are mapped, can also be
 * retrieved from output port 1. They are in a svtkTable, stored in the "Strings"
 * column as a svtkStringArray.
 */

#ifndef svtkStringToCategory_h
#define svtkStringToCategory_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkInfovisCoreModule.h" // For export macro

class SVTKINFOVISCORE_EXPORT svtkStringToCategory : public svtkDataObjectAlgorithm
{
public:
  static svtkStringToCategory* New();
  svtkTypeMacro(svtkStringToCategory, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The name to give to the output svtkIntArray of category values.
   */
  svtkSetStringMacro(CategoryArrayName);
  svtkGetStringMacro(CategoryArrayName);
  //@}

  /**
   * This is required to capture REQUEST_DATA_OBJECT requests.
   */
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

protected:
  svtkStringToCategory();
  ~svtkStringToCategory() override;

  /**
   * Creates the same output type as the input type.
   */
  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  int FillOutputPortInformation(int port, svtkInformation* info) override;

  char* CategoryArrayName;

private:
  svtkStringToCategory(const svtkStringToCategory&) = delete;
  void operator=(const svtkStringToCategory&) = delete;
};

#endif
