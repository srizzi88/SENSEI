/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractDataArraysOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractDataArraysOverTime
 * @brief   extracts array from input dataset over time.
 *
 * svtkExtractDataArraysOverTime extracts array from input dataset over time.
 * The filter extracts attribute arrays, based on the chosen field association
 * (svtkExtractDataArraysOverTime::SetFieldAssociation).
 *
 * svtkExtractDataArraysOverTime::ReportStatisticsOnly determines if each element
 * is individually tracked or only summary statistics for each timestep are
 * tracked.
 *
 * If ReportStatisticsOnly is off, the filter tracks each element in the input
 * over time. It requires that it can identify matching elements from one
 * timestep to another. There are several ways of doing that.
 *
 * \li if svtkExtractDataArraysOverTime::UseGlobalIDs is true, then the filter
 *     will look for array marked as svtkDataSetAttributes::GLOBALIDS in the
 *     input and use that to track the element.
 * \li if svtkExtractDataArraysOverTime::UseGlobalIDs is false or there are no
 *     element ids present, then the filter will look for the array chosen for
 *     processing using `svtkAlgorithm::SetInputArrayToProcess` at index 0.
 * \li if earlier attempts fail, then simply the element id (i.e. index) is used.
 *
 * The output is a svtkMultiBlockDataSet with single level, where leaf nodes can
 * are svtkTable instances.
 *
 * The output is structured as follows:
 *
 * \li if svtkExtractDataArraysOverTime::ReportStatisticsOnly is true, then the
 *     stats are computed per input block (if input is a composite dataset) or on the whole
 *     input dataset and placed as blocks named as **stats block=<block id>**.
 *     For non-composite input, the single leaf block is output is named as
 *     **stats**.
 *
 * \li if svtkExtractDataArraysOverTime::ReportStatisticsOnly if off, then each
 *    tracked element is placed in a separate output block. The block name is of
 *    the form **id=<id> block=<block id>** where the **block=** suffix is
 *    dropped for non-composite input datasets. If global ids are being used for
 *    tracking then the name is simply **gid=<global id>**.
 *
 * @sa svtkPExtractDataArraysOverTime
 */

#ifndef svtkExtractDataArraysOverTime_h
#define svtkExtractDataArraysOverTime_h

#include "svtkDataObject.h"              // for svtkDataObject
#include "svtkFiltersExtractionModule.h" // For export macro
#include "svtkMultiBlockDataSetAlgorithm.h"
#include "svtkSmartPointer.h" // for svtkSmartPointer.

class svtkDataSet;
class svtkTable;
class svtkDataSetAttributes;
class svtkDescriptiveStatistics;
class svtkOrderStatistics;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractDataArraysOverTime
  : public svtkMultiBlockDataSetAlgorithm
{
public:
  static svtkExtractDataArraysOverTime* New();
  svtkTypeMacro(svtkExtractDataArraysOverTime, svtkMultiBlockDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the number of time steps
   */
  svtkGetMacro(NumberOfTimeSteps, int);
  //@}

  //@{
  /**
   * FieldAssociation indicates which attributes to extract over time. This filter
   * can extract only one type of attribute arrays. Currently, svtkDataObject::FIELD
   * and svtkDataObject::POINT_THEN_CELL are not supported.
   */
  svtkSetClampMacro(
    FieldAssociation, int, svtkDataObject::POINT, svtkDataObject::NUMBER_OF_ATTRIBUTE_TYPES - 1);
  svtkGetMacro(FieldAssociation, int);
  //@}

  //@{
  /**
   * Instead of breaking a data into a separate time-history
   * table for each (block,ID)-tuple, you may call
   * ReportStatisticsOnlyOn(). Then a single table per
   * block of the input dataset will report the minimum, maximum,
   * quartiles, and (for numerical arrays) the average and standard
   * deviation of the data over time.

   * The default is off to preserve backwards-compatibility.
   */
  svtkSetMacro(ReportStatisticsOnly, bool);
  svtkGetMacro(ReportStatisticsOnly, bool);
  svtkBooleanMacro(ReportStatisticsOnly, bool);
  //@}

  //@{
  /**
   * When ReportStatisticsOnly is false, if UseGlobalIDs is true, then the filter will track
   * elements using their global ids, if present. Default is true.
   */
  svtkSetMacro(UseGlobalIDs, bool);
  svtkGetMacro(UseGlobalIDs, bool);
  //@}

protected:
  svtkExtractDataArraysOverTime();
  ~svtkExtractDataArraysOverTime() override;

  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestUpdateExtent(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  virtual void PostExecute(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int CurrentTimeIndex;
  int NumberOfTimeSteps;
  int FieldAssociation;
  bool ReportStatisticsOnly;
  bool UseGlobalIDs;
  int Error;
  enum Errors
  {
    NoError,
    MoreThan1Indices
  };

  virtual svtkSmartPointer<svtkDescriptiveStatistics> NewDescriptiveStatistics();
  virtual svtkSmartPointer<svtkOrderStatistics> NewOrderStatistics();

private:
  svtkExtractDataArraysOverTime(const svtkExtractDataArraysOverTime&) = delete;
  void operator=(const svtkExtractDataArraysOverTime&) = delete;

  class svtkInternal;
  friend class svtkInternal;
  svtkInternal* Internal;
};
#endif
