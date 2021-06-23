/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAppendSelection.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*----------------------------------------------------------------------------
 Copyright (c) Sandia Corporation
 See Copyright.txt or http://www.paraview.org/HTML/Copyright.html for details.
----------------------------------------------------------------------------*/
/**
 * @class   svtkAppendSelection
 * @brief   appends one or more selections together
 *
 *
 * svtkAppendSelection is a filter that appends one of more selections into
 * a single selection.  All selections must have the same content type unless
 * AppendByUnion is false.
 */

#ifndef svtkAppendSelection_h
#define svtkAppendSelection_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkSelectionAlgorithm.h"

class svtkSelection;

class SVTKFILTERSCORE_EXPORT svtkAppendSelection : public svtkSelectionAlgorithm
{
public:
  static svtkAppendSelection* New();

  svtkTypeMacro(svtkAppendSelection, svtkSelectionAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * UserManagedInputs allows the user to set inputs by number instead of
   * using the AddInput/RemoveInput functions. Calls to
   * SetNumberOfInputs/SetInputByNumber should not be mixed with calls
   * to AddInput/RemoveInput. By default, UserManagedInputs is false.
   */
  svtkSetMacro(UserManagedInputs, svtkTypeBool);
  svtkGetMacro(UserManagedInputs, svtkTypeBool);
  svtkBooleanMacro(UserManagedInputs, svtkTypeBool);
  //@}

  /**
   * Add a dataset to the list of data to append. Should not be
   * used when UserManagedInputs is true, use SetInputByNumber instead.
   */
  void AddInputData(svtkSelection*);

  /**
   * Remove a dataset from the list of data to append. Should not be
   * used when UserManagedInputs is true, use SetInputByNumber (nullptr) instead.
   */
  void RemoveInputData(svtkSelection*);

  //@{
  /**
   * Get any input of this filter.
   */
  svtkSelection* GetInput(int idx);
  svtkSelection* GetInput() { return this->GetInput(0); }
  //@}

  /**
   * Directly set(allocate) number of inputs, should only be used
   * when UserManagedInputs is true.
   */
  void SetNumberOfInputs(int num);

  // Set Nth input, should only be used when UserManagedInputs is true.
  void SetInputConnectionByNumber(int num, svtkAlgorithmOutput* input);

  //@{
  /**
   * When set to true, all the selections are combined together to form a single
   * svtkSelection output.
   * When set to false, the output is a composite selection with
   * input selections as the children of the composite selection. This allows
   * for selections with different content types and properties. Default is
   * true.
   */
  svtkSetMacro(AppendByUnion, svtkTypeBool);
  svtkGetMacro(AppendByUnion, svtkTypeBool);
  svtkBooleanMacro(AppendByUnion, svtkTypeBool);
  //@}

protected:
  svtkAppendSelection();
  ~svtkAppendSelection() override;

  // Usual data generation method
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int FillInputPortInformation(int, svtkInformation*) override;

private:
  // hide the superclass' AddInput() from the user and the compiler
  void AddInputData(svtkDataObject*)
  {
    svtkErrorMacro(<< "AddInput() must be called with a svtkSelection not a svtkDataObject.");
  }

  svtkTypeBool UserManagedInputs;
  svtkTypeBool AppendByUnion;

private:
  svtkAppendSelection(const svtkAppendSelection&) = delete;
  void operator=(const svtkAppendSelection&) = delete;
};

#endif
