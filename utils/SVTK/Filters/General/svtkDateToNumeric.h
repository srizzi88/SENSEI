/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDateToNumeric.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDateToNumeric
 * @brief   Converts string dates to numeric values
 *
 *
 * This filter preserves all the topology of the input. All string arrays are
 * examined to see if their value is a date. If so an array is added with the
 * numeric value of that date. The new array is of type double and its name
 * is the source arrays name with _numeric appended.
 *
 * default date formats parsed include
 *
 *   "%Y-%m-%d %H:%M:%S"
 *   "%d/%m/%Y %H:%M:%S"
 */

#ifndef svtkDateToNumeric_h
#define svtkDateToNumeric_h

#include "svtkDataObject.h"           // for svtkDataObject::FieldAssociations
#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"
#include "svtkSmartPointer.h" // for ivar

class SVTKFILTERSGENERAL_EXPORT svtkDateToNumeric : public svtkPassInputTypeAlgorithm
{
public:
  static svtkDateToNumeric* New();
  svtkTypeMacro(svtkDateToNumeric, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * You can specify your own format to parse dates by. This string
   * follows the formatting conventions of std::get_time
   */
  svtkGetStringMacro(DateFormat);
  svtkSetStringMacro(DateFormat);
  //@}

protected:
  svtkDateToNumeric();
  ~svtkDateToNumeric() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  char* DateFormat;

private:
  svtkDateToNumeric(const svtkDateToNumeric&) = delete;
  void operator=(const svtkDateToNumeric&) = delete;
};

#endif
