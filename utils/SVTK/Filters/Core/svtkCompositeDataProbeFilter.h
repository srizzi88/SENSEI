/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeDataProbeFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeDataProbeFilter
 * @brief   subclass of svtkProbeFilter which supports
 * composite datasets in the input.
 *
 * svtkCompositeDataProbeFilter supports probing into multi-group datasets.
 * It sequentially probes through each concrete dataset within the composite
 * probing at only those locations at which there were no hits when probing
 * earlier datasets. For Hierarchical datasets, this traversal through leaf
 * datasets is done in reverse order of levels i.e. highest level first.
 *
 * When dealing with composite datasets, partial arrays are common i.e.
 * data-arrays that are not available in all of the blocks. By default, this
 * filter only passes those point and cell data-arrays that are available in all
 * the blocks i.e. partial array are removed.
 * When PassPartialArrays is turned on, this behavior is changed to take a
 * union of all arrays present thus partial arrays are passed as well. However,
 * for composite dataset input, this filter still produces a non-composite
 * output. For all those locations in a block of where a particular data array
 * is missing, this filter uses svtkMath::Nan() for double and float arrays,
 * while 0 for all other types of arrays i.e int, char etc.
 */

#ifndef svtkCompositeDataProbeFilter_h
#define svtkCompositeDataProbeFilter_h

#include "svtkFiltersCoreModule.h" // For export macro
#include "svtkProbeFilter.h"

class svtkCompositeDataSet;
class SVTKFILTERSCORE_EXPORT svtkCompositeDataProbeFilter : public svtkProbeFilter
{
public:
  static svtkCompositeDataProbeFilter* New();
  svtkTypeMacro(svtkCompositeDataProbeFilter, svtkProbeFilter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * When dealing with composite datasets, partial arrays are common i.e.
   * data-arrays that are not available in all of the blocks. By default, this
   * filter only passes those point and cell data-arrays that are available in
   * all the blocks i.e. partial array are removed.  When PassPartialArrays is
   * turned on, this behavior is changed to take a union of all arrays present
   * thus partial arrays are passed as well. However, for composite dataset
   * input, this filter still produces a non-composite output. For all those
   * locations in a block of where a particular data array is missing, this
   * filter uses svtkMath::Nan() for double and float arrays, while 0 for all
   * other types of arrays i.e int, char etc.
   */
  svtkSetMacro(PassPartialArrays, bool);
  svtkGetMacro(PassPartialArrays, bool);
  svtkBooleanMacro(PassPartialArrays, bool);
  //@}

protected:
  svtkCompositeDataProbeFilter();
  ~svtkCompositeDataProbeFilter() override;

  /**
   * Change input information to accept composite datasets as the input which
   * is probed into.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  /**
   * Builds the field list using the composite dataset source.
   */
  int BuildFieldList(svtkCompositeDataSet* source);

  /**
   * Initializes output and various arrays which keep track for probing status.
   */
  void InitializeOutputArrays(svtkPointData* outPD, svtkIdType numPts) override;

  /**
   * Handle composite input.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Create a default executive.
   */
  svtkExecutive* CreateDefaultExecutive() override;

  bool PassPartialArrays;

private:
  svtkCompositeDataProbeFilter(const svtkCompositeDataProbeFilter&) = delete;
  void operator=(const svtkCompositeDataProbeFilter&) = delete;
};

#endif
