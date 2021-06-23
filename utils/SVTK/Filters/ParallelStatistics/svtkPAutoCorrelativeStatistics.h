/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPAutoCorrelativeStatistics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkPAutoCorrelativeStatistics
 * @brief   A class for parallel auto-correlative statistics
 *
 * svtkPAutoCorrelativeStatistics is svtkAutoCorrelativeStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * @par Thanks:
 * This class was written by Philippe Pebay, Kitware SAS 2012.
 */

#ifndef svtkPAutoCorrelativeStatistics_h
#define svtkPAutoCorrelativeStatistics_h

#include "svtkAutoCorrelativeStatistics.h"
#include "svtkFiltersParallelStatisticsModule.h" // For export macro

class svtkMultiBlockDataSet;
class svtkMultiProcessController;

class SVTKFILTERSPARALLELSTATISTICS_EXPORT svtkPAutoCorrelativeStatistics
  : public svtkAutoCorrelativeStatistics
{
public:
  static svtkPAutoCorrelativeStatistics* New();
  svtkTypeMacro(svtkPAutoCorrelativeStatistics, svtkAutoCorrelativeStatistics);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  /**
   * Execute the parallel calculations required by the Learn option.
   */
  void Learn(svtkTable* inData, svtkTable* inParameters, svtkMultiBlockDataSet* outMeta) override;

  /**
   * Execute the calculations required by the Test option.
   * NB: Not implemented for more than 1 processor
   */
  void Test(svtkTable*, svtkMultiBlockDataSet*, svtkTable*) override;

protected:
  svtkPAutoCorrelativeStatistics();
  ~svtkPAutoCorrelativeStatistics() override;

  svtkMultiProcessController* Controller;

private:
  svtkPAutoCorrelativeStatistics(const svtkPAutoCorrelativeStatistics&) = delete;
  void operator=(const svtkPAutoCorrelativeStatistics&) = delete;
};

#endif
