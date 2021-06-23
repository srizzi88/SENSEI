/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractSelectedArraysOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractSelectedArraysOverTime
 * @brief   extracts a selection over time.
 *
 * svtkExtractSelectedArraysOverTime extracts a selection over time.
 * This is combination of two filters, an svtkExtractSelection filter followed by
 * svtkExtractDataArraysOverTime, to do its work.
 *
 * The filter has two inputs - 0th input is the temporal data to extracted,
 * while the second input is the selection (svtkSelection) to extract. Based on
 * the type of the selection, this filter setups up properties on the internal
 * svtkExtractDataArraysOverTime instance to produce a reasonable extract.
 *
 * The output is a svtkMultiBlockDataSet. See svtkExtractDataArraysOverTime for
 * details on how the output is structured.
 */

#ifndef svtkExtractSelectedArraysOverTime_h
#define svtkExtractSelectedArraysOverTime_h

#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"
#include "svtkSmartPointer.h" // for svtkSmartPointer.

class svtkDataSet;
class svtkDataSetAttributes;
class svtkExtractDataArraysOverTime;
class svtkExtractSelection;
class svtkSelection;
class svtkTable;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractSelectedArraysOverTime
  : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkExtractSelectedArraysOverTime* New();
  svtkTypeMacro(svtkExtractSelectedArraysOverTime, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the number of time steps
   */
  svtkGetMacro(NumberOfTimeSteps, int);
  //@}

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
   * Set/get the svtkExtractSelection instance used to obtain
   * array values at each time step. By default, svtkExtractSelection is used.
   */
  virtual void SetSelectionExtractor(svtkExtractSelection*);
  svtkExtractSelection* GetSelectionExtractor();
  //@}

  //@{
  /**
   * Instead of breaking a selection into a separate time-history
   * table for each (block,ID)-tuple, you may call
   * ReportStatisticsOnlyOn(). Then a single table per
   * block of the input dataset will report the minimum, maximum,
   * quartiles, and (for numerical arrays) the average and standard
   * deviation of the selection over time.

   * The default is off to preserve backwards-compatibility.
   */
  svtkSetMacro(ReportStatisticsOnly, bool);
  svtkGetMacro(ReportStatisticsOnly, bool);
  svtkBooleanMacro(ReportStatisticsOnly, bool);
  //@}

protected:
  svtkExtractSelectedArraysOverTime();
  ~svtkExtractSelectedArraysOverTime() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestUpdateExtent(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  virtual void PostExecute(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  /**
   * Determines the FieldType and ContentType for the selection. If the
   * selection is a svtkSelection::SELECTIONS selection, then this method ensures
   * that all child nodes have the same field type and content type otherwise,
   * it returns 0.
   */
  int DetermineSelectionType(svtkSelection*);

  int NumberOfTimeSteps;
  int FieldType;
  int ContentType;
  bool ReportStatisticsOnly;
  int Error;

  enum Errors
  {
    NoError,
    MoreThan1Indices
  };

  svtkSmartPointer<svtkExtractSelection> SelectionExtractor;
  svtkSmartPointer<svtkExtractDataArraysOverTime> ArraysExtractor;

private:
  svtkExtractSelectedArraysOverTime(const svtkExtractSelectedArraysOverTime&) = delete;
  void operator=(const svtkExtractSelectedArraysOverTime&) = delete;

  /**
   * Applies the `SelectionExtractor` to extract the dataset to track
   * and return it. This should be called for each time iteration.
   */
  svtkSmartPointer<svtkDataObject> Extract(svtkInformationVector** inputV, svtkInformation* outInfo);

  bool IsExecuting;
};

#endif
