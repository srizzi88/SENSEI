/*=========================================================================

Program:   Visualization Toolkit
Module:    svtkStreamingStatistics.h

Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
All rights reserved.
See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

This software is distributed WITHOUT ANY WARRANTY; without even
the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2010 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
  -------------------------------------------------------------------------*/
/**
 * @class   svtkStreamingStatistics
 * @brief   A class for using the statistics filters
 * in a streaming mode.
 *
 *
 * A class for using the statistics filters in a streaming mode or perhaps
 * an "online, incremental, push" mode.
 *
 * @par Thanks:
 * Thanks to the Universe for unfolding in a way that allowed this class
 * to be implemented, also Godzilla for not crushing my computer.
 */

#ifndef svtkStreamingStatistics_h
#define svtkStreamingStatistics_h

#include "svtkFiltersStatisticsModule.h" // For export macro
#include "svtkTableAlgorithm.h"

class svtkDataObjectCollection;
class svtkMultiBlockDataSet;
class svtkStatisticsAlgorithm;
class svtkTable;

class SVTKFILTERSSTATISTICS_EXPORT svtkStreamingStatistics : public svtkTableAlgorithm
{
public:
  svtkTypeMacro(svtkStreamingStatistics, svtkTableAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;
  static svtkStreamingStatistics* New();

  /**
   * enumeration values to specify input port types
   */
  enum InputPorts
  {
    INPUT_DATA = 0,       //!< Port 0 is for learn data
    LEARN_PARAMETERS = 1, //!< Port 1 is for learn parameters (initial guesses, etc.)
    INPUT_MODEL = 2       //!< Port 2 is for a priori models
  };

  /**
   * enumeration values to specify output port types
   */
  enum OutputIndices
  {
    OUTPUT_DATA = 0,  //!< Output 0 mirrors the input data, plus optional assessment columns
    OUTPUT_MODEL = 1, //!< Output 1 contains any generated model
    OUTPUT_TEST = 2   //!< Output 2 contains result of statistical test(s)
  };

  virtual void SetStatisticsAlgorithm(svtkStatisticsAlgorithm*);

protected:
  svtkStreamingStatistics();
  ~svtkStreamingStatistics() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int FillOutputPortInformation(int port, svtkInformation* info) override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

private:
  svtkStreamingStatistics(const svtkStreamingStatistics&) = delete;
  void operator=(const svtkStreamingStatistics&) = delete;

  // Internal statistics algorithm to care for and feed
  svtkStatisticsAlgorithm* StatisticsAlgorithm;

  // Internal model that gets aggregated
  svtkMultiBlockDataSet* InternalModel;
};

#endif
