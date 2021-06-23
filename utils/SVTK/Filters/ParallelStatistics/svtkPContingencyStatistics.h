/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkPContingencyStatistics.h

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
 * @class   svtkPContingencyStatistics
 * @brief   A class for parallel bivariate contingency statistics
 *
 * svtkPContingencyStatistics is svtkContingencyStatistics subclass for parallel datasets.
 * It learns and derives the global statistical model on each node, but assesses each
 * individual data points on the node that owns it.
 *
 * .NOTE: It is assumed that the keys in the contingency table be contained in the set {0,...,n-1}
 * of successive integers, where n is the number of rows of the summary table.
 * If this requirement is not fulfilled, then the outcome of the parallel update of contingency
 * tables is unpredictable but will most likely be a crash.
 * Note that this requirement is consistent with the way contingency tables are constructed
 * by the (serial) superclass and thus, if you are using this class as it is intended to be ran,
 * then you do not have to worry about this requirement.
 *
 * @par Thanks:
 * Thanks to Philippe Pebay from Sandia National Laboratories for implementing this class.
 */

#ifndef svtkPContingencyStatistics_h
#define svtkPContingencyStatistics_h

#include "svtkContingencyStatistics.h"
#include "svtkFiltersParallelStatisticsModule.h" // For export macro

#include <vector> // STL Header

class svtkMultiBlockDataSet;
class svtkMultiProcessController;

class SVTKFILTERSPARALLELSTATISTICS_EXPORT svtkPContingencyStatistics
  : public svtkContingencyStatistics
{
public:
  static svtkPContingencyStatistics* New();
  svtkTypeMacro(svtkPContingencyStatistics, svtkContingencyStatistics);
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
  void Learn(svtkTable*, svtkTable*, svtkMultiBlockDataSet*) override;

protected:
  svtkPContingencyStatistics();
  ~svtkPContingencyStatistics() override;

  /**
   * Reduce the collection of local contingency tables to the global one
   */
  bool Reduce(svtkIdType&, char*, svtkStdString&, svtkIdType&, svtkIdType*, std::vector<svtkIdType>&);

  /**
   * Broadcast reduced contingency table to all processes
   */
  bool Broadcast(svtkIdType, svtkStdString&, std::vector<svtkStdString>&, svtkIdType,
    std::vector<svtkIdType>&, svtkIdType);

  svtkMultiProcessController* Controller;

private:
  svtkPContingencyStatistics(const svtkPContingencyStatistics&) = delete;
  void operator=(const svtkPContingencyStatistics&) = delete;
};

#endif
