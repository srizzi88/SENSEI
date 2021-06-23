/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPExtractDataArraysOverTime.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class svtkPExtractDataArraysOverTime
 * @brief parallel version of svtkExtractDataArraysOverTime.
 *
 * svtkPExtractDataArraysOverTime adds distributed data support to
 * svtkExtractDataArraysOverTime.
 *
 * It combines results from all ranks and produce non-empty result only on rank 0.
 *
 * @caveats Caveats Caveats
 *
 * This filter's behavior when `ReportStatisticsOnly` is true is buggy and will
 * change in the future. When `ReportStatisticsOnly` currently, each rank
 * computes separate stats for local data. Consequently, this filter preserves
 * each processes results separately (by adding suffix **rank=<rank num>** to each
 * of the block names, as appropriate. In future, we plan to fix this to
 * correctly compute stats in parallel for each block.
 */

#ifndef svtkPExtractDataArraysOverTime_h
#define svtkPExtractDataArraysOverTime_h

#include "svtkExtractDataArraysOverTime.h"
#include "svtkFiltersParallelModule.h" // For export macro

class svtkMultiProcessController;

class SVTKFILTERSPARALLEL_EXPORT svtkPExtractDataArraysOverTime : public svtkExtractDataArraysOverTime
{
public:
  static svtkPExtractDataArraysOverTime* New();
  svtkTypeMacro(svtkPExtractDataArraysOverTime, svtkExtractDataArraysOverTime);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set and get the controller.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkPExtractDataArraysOverTime();
  ~svtkPExtractDataArraysOverTime() override;

  void PostExecute(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  svtkMultiProcessController* Controller;

private:
  svtkPExtractDataArraysOverTime(const svtkPExtractDataArraysOverTime&) = delete;
  void operator=(const svtkPExtractDataArraysOverTime&) = delete;
  void ReorganizeData(svtkMultiBlockDataSet* dataset);
};

#endif
