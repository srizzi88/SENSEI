/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgrammableDataObjectSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProgrammableDataObjectSource
 * @brief   generate source data object via a user-specified function
 *
 * svtkProgrammableDataObjectSource is a source object that is programmable by
 * the user. The output of the filter is a data object (svtkDataObject) which
 * represents data via an instance of field data. To use this object, you
 * must specify a function that creates the output.
 *
 * Example use of this filter includes reading tabular data and encoding it
 * as svtkFieldData. You can then use filters like svtkDataObjectToDataSetFilter
 * to convert the data object to a dataset and then visualize it.  Another
 * important use of this class is that it allows users of interpreters (e.g.,
 * Java) the ability to write source objects without having to
 * recompile C++ code or generate new libraries.
 *
 * @sa
 * svtkProgrammableFilter svtkProgrammableAttributeDataFilter
 * svtkProgrammableSource svtkDataObjectToDataSetFilter
 */

#ifndef svtkProgrammableDataObjectSource_h
#define svtkProgrammableDataObjectSource_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersSourcesModule.h" // For export macro

class SVTKFILTERSSOURCES_EXPORT svtkProgrammableDataObjectSource : public svtkDataObjectAlgorithm
{
public:
  static svtkProgrammableDataObjectSource* New();
  svtkTypeMacro(svtkProgrammableDataObjectSource, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Signature definition for programmable method callbacks. Methods passed to
   * SetExecuteMethod or SetExecuteMethodArgDelete must conform to this
   * signature.
   * The presence of this typedef is useful for reference and for external
   * analysis tools, but it cannot be used in the method signatures in these
   * header files themselves because it prevents the internal SVTK wrapper
   * generators from wrapping these methods.
   */
  typedef void (*ProgrammableMethodCallbackType)(void* arg);

  /**
   * Specify the function to use to generate the output data object. Note
   * that the function takes a single (void *) argument.
   */
  void SetExecuteMethod(void (*f)(void*), void* arg);

  /**
   * Set the arg delete method. This is used to free user memory.
   */
  void SetExecuteMethodArgDelete(void (*f)(void*));

protected:
  svtkProgrammableDataObjectSource();
  ~svtkProgrammableDataObjectSource() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  ProgrammableMethodCallbackType ExecuteMethod; // function to invoke
  ProgrammableMethodCallbackType ExecuteMethodArgDelete;
  void* ExecuteMethodArg;

private:
  svtkProgrammableDataObjectSource(const svtkProgrammableDataObjectSource&) = delete;
  void operator=(const svtkProgrammableDataObjectSource&) = delete;
};

#endif
