/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractTemporalFieldData.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractTemporalFieldData
 * @brief   Extract temporal arrays from input field data
 *
 * @deprecated in SVTK 9.0. Use svtkExtractExodusGlobalTemporalVariables instead.
 * The global temporal variable concept is a very Exodus specific thing and
 * hence the filter is now maybe to work closely with the exodus reader and
 * hence can better support other exodus use-cases like restart files.
 *
 */

#ifndef svtkExtractTemporalFieldData_h
#define svtkExtractTemporalFieldData_h

#include "svtkDataObjectAlgorithm.h"
#include "svtkFiltersExtractionModule.h" // For export macro

#if !defined(SVTK_LEGACY_REMOVE)
class svtkDataSet;
class svtkTable;
class svtkDataSetAttributes;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractTemporalFieldData : public svtkDataObjectAlgorithm
{
public:
  static svtkExtractTemporalFieldData* New();
  svtkTypeMacro(svtkExtractTemporalFieldData, svtkDataObjectAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the number of time steps
   */
  int GetNumberOfTimeSteps();

  //@{
  /**
   * When set to true (default), if the input is a svtkCompositeDataSet, then
   * each block in the input dataset in processed separately. If false, then the first
   * non-empty FieldData is considered.
   */
  svtkSetMacro(HandleCompositeDataBlocksIndividually, bool);
  svtkGetMacro(HandleCompositeDataBlocksIndividually, bool);
  svtkBooleanMacro(HandleCompositeDataBlocksIndividually, bool);
  //@}

protected:
  svtkExtractTemporalFieldData();
  ~svtkExtractTemporalFieldData() override;

  int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * This looks at the arrays in the svtkFieldData of input and copies them
   * to the output point data.
   * Returns true if the input had an "appropriate" field data.
   */
  bool CopyDataToOutput(svtkDataSet* input, svtkTable* output);

  bool HandleCompositeDataBlocksIndividually;

private:
  svtkExtractTemporalFieldData(const svtkExtractTemporalFieldData&) = delete;
  void operator=(const svtkExtractTemporalFieldData&) = delete;

  class svtkInternals;
  svtkInternals* Internals;
};

#endif // !defined(SVTK_LEGACY_REMOVE)
#endif
