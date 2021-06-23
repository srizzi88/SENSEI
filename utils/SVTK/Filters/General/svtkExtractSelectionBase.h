/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectionBase.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractSelectionBase
 * @brief   abstract base class for all extract selection
 * filters.
 *
 * svtkExtractSelectionBase is an abstract base class for all extract selection
 * filters. It defines some properties common to all extract selection filters.
 */

#ifndef svtkExtractSelectionBase_h
#define svtkExtractSelectionBase_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersGeneralModule.h" // For export macro

class SVTKFILTERSGENERAL_EXPORT svtkExtractSelectionBase : public svtkDataObjectAlgorithm
{
public:
  svtkTypeMacro(svtkExtractSelectionBase, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Convenience method to specify the selection connection (2nd input
   * port)
   */
  void SetSelectionConnection(svtkAlgorithmOutput* algOutput)
  {
    this->SetInputConnection(1, algOutput);
  }

  //@{
  /**
   * This flag tells the extraction filter not to convert the selected
   * output into an unstructured grid, but instead to produce a svtkInsidedness
   * array and add it to the input dataset. Default value is false(0).
   */
  svtkSetMacro(PreserveTopology, svtkTypeBool);
  svtkGetMacro(PreserveTopology, svtkTypeBool);
  svtkBooleanMacro(PreserveTopology, svtkTypeBool);
  //@}

protected:
  svtkExtractSelectionBase();
  ~svtkExtractSelectionBase() override;

  /**
   * Sets up empty output dataset
   */
  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkTypeBool PreserveTopology;

private:
  svtkExtractSelectionBase(const svtkExtractSelectionBase&) = delete;
  void operator=(const svtkExtractSelectionBase&) = delete;
};

#endif
