/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPMultiCorrelativeStatistics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2011 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   svtkPMultiCorrelativeStatistics
 * @brief   A class for parallel bivariate correlative statistics
 *
 * svtkPMultiCorrelativeStatistics is svtkMultiCorrelativeStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay and David Thompson from Sandia National Laboratories for implementing
 * this class.
 */

#ifndef svtkPMultiCorrelativeStatistics_h
#define svtkPMultiCorrelativeStatistics_h

#include "svtkFiltersParallelStatisticsModule.h" // For export macro
#include "svtkMultiCorrelativeStatistics.h"

class svtkMultiProcessController;

class SVTKFILTERSPARALLELSTATISTICS_EXPORT svtkPMultiCorrelativeStatistics
  : public svtkMultiCorrelativeStatistics
{
public:
  static svtkPMultiCorrelativeStatistics* New();
  svtkTypeMacro(svtkPMultiCorrelativeStatistics, svtkMultiCorrelativeStatistics);
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
   * Performs Reduction
   */
  static void GatherStatistics(svtkMultiProcessController* curController, svtkTable* sparseCov);

protected:
  svtkPMultiCorrelativeStatistics();
  ~svtkPMultiCorrelativeStatistics() override;

  svtkMultiProcessController* Controller;

  // Execute the parallel calculations required by the Learn option.
  void Learn(svtkTable* inData, svtkTable* inParameters, svtkMultiBlockDataSet* outMeta) override;

  svtkOrderStatistics* CreateOrderStatisticsInstance() override;

private:
  svtkPMultiCorrelativeStatistics(const svtkPMultiCorrelativeStatistics&) = delete;
  void operator=(const svtkPMultiCorrelativeStatistics&) = delete;
};

#endif
