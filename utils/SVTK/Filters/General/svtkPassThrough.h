/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPassThrough.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPassThrough
 * @brief   Shallow copies the input into the output
 *
 *
 * The output type is always the same as the input object type.
 */

#ifndef svtkPassThrough_h
#define svtkPassThrough_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class SVTKFILTERSGENERAL_EXPORT svtkPassThrough : public svtkPassInputTypeAlgorithm
{
public:
  static svtkPassThrough* New();
  svtkTypeMacro(svtkPassThrough, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Specify the first input port as optional
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  //@{
  /**
   * Whether or not to deep copy the input. This can be useful if you
   * want to create a copy of a data object. You can then disconnect
   * this filter's input connections and it will act like a source.
   * Defaults to OFF.
   */
  svtkSetMacro(DeepCopyInput, svtkTypeBool);
  svtkGetMacro(DeepCopyInput, svtkTypeBool);
  svtkBooleanMacro(DeepCopyInput, svtkTypeBool);
  //@}

  /**
   * Allow the filter to execute without error when no input connection is
   * specified. In this case, and empty svtkPolyData dataset will be created.
   * By default, this setting is false.
   * @{
   */
  svtkSetMacro(AllowNullInput, bool);
  svtkGetMacro(AllowNullInput, bool);
  svtkBooleanMacro(AllowNullInput, bool);
  /**@}*/

protected:
  svtkPassThrough();
  ~svtkPassThrough() override;

  int RequestDataObject(
    svtkInformation* request, svtkInformationVector** inVec, svtkInformationVector* outVec) override;
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool DeepCopyInput;
  bool AllowNullInput;

private:
  svtkPassThrough(const svtkPassThrough&) = delete;
  void operator=(const svtkPassThrough&) = delete;
};

#endif
