/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPPCAStatistics.h

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
 * @class   svtkPPCAStatistics
 * @brief   A class for parallel principal component analysis
 *
 * svtkPPCAStatistics is svtkPCAStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay, David Thompson and Janine Bennett from
 * Sandia National Laboratories for implementing this class.
 */

#ifndef svtkPPCAStatistics_h
#define svtkPPCAStatistics_h

#include "svtkFiltersParallelStatisticsModule.h" // For export macro
#include "svtkPCAStatistics.h"

class svtkMultiProcessController;

class SVTKFILTERSPARALLELSTATISTICS_EXPORT svtkPPCAStatistics : public svtkPCAStatistics
{
public:
  svtkTypeMacro(svtkPPCAStatistics, svtkPCAStatistics);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkPPCAStatistics* New();

  //@{
  /**
   * Get/Set the multiprocess controller. If no controller is set,
   * single process is assumed.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

protected:
  svtkPPCAStatistics();
  ~svtkPPCAStatistics() override;

  svtkMultiProcessController* Controller;

  // Execute the parallel calculations required by the Learn option.
  void Learn(svtkTable* inData, svtkTable* inParameters, svtkMultiBlockDataSet* outMeta) override;

  /**
   * Execute the calculations required by the Test option.
   * NB: Not implemented for more than 1 processor
   */
  void Test(svtkTable*, svtkMultiBlockDataSet*, svtkTable*) override;

  svtkOrderStatistics* CreateOrderStatisticsInstance() override;

private:
  svtkPPCAStatistics(const svtkPPCAStatistics&) = delete;
  void operator=(const svtkPPCAStatistics&) = delete;
};

#endif
