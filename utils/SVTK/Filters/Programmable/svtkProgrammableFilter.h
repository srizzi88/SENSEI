/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkProgrammableFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkProgrammableFilter
 * @brief   a user-programmable filter
 *
 * svtkProgrammableFilter is a filter that can be programmed by the user.  To
 * use the filter you define a function that retrieves input of the correct
 * type, creates data, and then manipulates the output of the filter.  Using
 * this filter avoids the need for subclassing - and the function can be
 * defined in an interpreter wrapper language such as Java.
 *
 * The trickiest part of using this filter is that the input and output
 * methods are unusual and cannot be compile-time type checked. Instead, as a
 * user of this filter it is your responsibility to set and get the correct
 * input and output types.
 *
 * @warning
 * The filter correctly manages modified time and network execution in most
 * cases. However, if you change the definition of the filter function,
 * you'll want to send a manual Modified() method to the filter to force it
 * to reexecute.
 *
 * @sa
 * svtkProgrammablePointDataFilter svtkProgrammableSource
 */

#ifndef svtkProgrammableFilter_h
#define svtkProgrammableFilter_h

#include "svtkFiltersProgrammableModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class SVTKFILTERSPROGRAMMABLE_EXPORT svtkProgrammableFilter : public svtkPassInputTypeAlgorithm
{
public:
  static svtkProgrammableFilter* New();
  svtkTypeMacro(svtkProgrammableFilter, svtkPassInputTypeAlgorithm);
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
   * Specify the function to use to operate on the point attribute data. Note
   * that the function takes a single (void *) argument.
   */
  void SetExecuteMethod(void (*f)(void*), void* arg);

  /**
   * Set the arg delete method. This is used to free user memory.
   */
  void SetExecuteMethodArgDelete(void (*f)(void*));

  //@{
  /**
   * Get the input as a concrete type. This method is typically used by the
   * writer of the filter function to get the input as a particular type (i.e.,
   * it essentially does type casting). It is the users responsibility to know
   * the correct type of the input data.
   */
  svtkPolyData* GetPolyDataInput();
  svtkStructuredPoints* GetStructuredPointsInput();
  svtkStructuredGrid* GetStructuredGridInput();
  svtkUnstructuredGrid* GetUnstructuredGridInput();
  svtkRectilinearGrid* GetRectilinearGridInput();
  svtkGraph* GetGraphInput();
  svtkMolecule* GetMoleculeInput();
  svtkTable* GetTableInput();
  //@}

  //@{
  /**
   * When CopyArrays is true, all arrays are copied to the output
   * iff input and output are of the same type. False by default.
   */
  svtkSetMacro(CopyArrays, bool);
  svtkGetMacro(CopyArrays, bool);
  svtkBooleanMacro(CopyArrays, bool);
  //@}

protected:
  svtkProgrammableFilter();
  ~svtkProgrammableFilter() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  ProgrammableMethodCallbackType ExecuteMethod; // function to invoke
  ProgrammableMethodCallbackType ExecuteMethodArgDelete;
  void* ExecuteMethodArg;

  bool CopyArrays;

private:
  svtkProgrammableFilter(const svtkProgrammableFilter&) = delete;
  void operator=(const svtkProgrammableFilter&) = delete;
};

#endif

// SVTK-HeaderTest-Exclude: svtkProgrammableFilter.h
