// -*- c++ -*-
/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTemporalStatistics.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/*
 * Copyright 2008 Sandia Corporation.
 * Under the terms of Contract DE-AC04-94AL85000, there is a non-exclusive
 * license for use of this work by or on behalf of the
 * U.S. Government. Redistribution and use in source and binary forms, with
 * or without modification, are permitted provided that this Notice and any
 * statement of authorship are reproduced on all copies.
 */

/**
 * @class   svtkTemporalStatistics
 * @brief   Compute statistics of point or cell data as it changes over time
 *
 *
 *
 * Given an input that changes over time, svtkTemporalStatistics looks at the
 * data for each time step and computes some statistical information of how a
 * point or cell variable changes over time.  For example, svtkTemporalStatistics
 * can compute the average value of "pressure" over time of each point.
 *
 * Note that this filter will require the upstream filter to be run on every
 * time step that it reports that it can compute.  This may be a time consuming
 * operation.
 *
 * svtkTemporalStatistics ignores the temporal spacing.  Each timestep will be
 * weighted the same regardless of how long of an interval it is to the next
 * timestep.  Thus, the average statistic may be quite different from an
 * integration of the variable if the time spacing varies.
 *
 * @par Thanks:
 * This class was originally written by Kenneth Moreland (kmorel@sandia.gov)
 * from Sandia National Laboratories.
 *
 */

#ifndef svtkTemporalStatistics_h
#define svtkTemporalStatistics_h

#include "svtkFiltersGeneralModule.h" // For export macro
#include "svtkPassInputTypeAlgorithm.h"

class svtkCompositeDataSet;
class svtkDataSet;
class svtkFieldData;
class svtkGraph;

class SVTKFILTERSGENERAL_EXPORT svtkTemporalStatistics : public svtkPassInputTypeAlgorithm
{
public:
  //@{
  /**
   * Standard methods for instantiation, type information, and printing.
   */
  svtkTypeMacro(svtkTemporalStatistics, svtkPassInputTypeAlgorithm);
  static svtkTemporalStatistics* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  //@}

  //@{
  /**
   * Turn on/off the computation of the average values over time.  On by
   * default.  The resulting array names have "_average" appended to them.
   */
  svtkGetMacro(ComputeAverage, svtkTypeBool);
  svtkSetMacro(ComputeAverage, svtkTypeBool);
  svtkBooleanMacro(ComputeAverage, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the computation of the minimum values over time.  On by
   * default.  The resulting array names have "_minimum" appended to them.
   */
  svtkGetMacro(ComputeMinimum, svtkTypeBool);
  svtkSetMacro(ComputeMinimum, svtkTypeBool);
  svtkBooleanMacro(ComputeMinimum, svtkTypeBool);
  //@}

  //@{
  /**
   * Turn on/off the computation of the maximum values over time.  On by
   * default.  The resulting array names have "_maximum" appended to them.
   */
  svtkGetMacro(ComputeMaximum, svtkTypeBool);
  svtkSetMacro(ComputeMaximum, svtkTypeBool);
  svtkBooleanMacro(ComputeMaximum, svtkTypeBool);
  //@}

  // Definition:
  // Turn on/off the computation of the standard deviation of the values over
  // time.  On by default.  The resulting array names have "_stddev" appended to
  // them.
  svtkGetMacro(ComputeStandardDeviation, svtkTypeBool);
  svtkSetMacro(ComputeStandardDeviation, svtkTypeBool);
  svtkBooleanMacro(ComputeStandardDeviation, svtkTypeBool);

protected:
  svtkTemporalStatistics();
  ~svtkTemporalStatistics() override;

  svtkTypeBool ComputeAverage;
  svtkTypeBool ComputeMaximum;
  svtkTypeBool ComputeMinimum;
  svtkTypeBool ComputeStandardDeviation;

  // Used when iterating the pipeline to keep track of which timestep we are on.
  int CurrentTimeIndex;

  int FillInputPortInformation(int port, svtkInformation* info) override;

  int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestUpdateExtent(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  virtual void InitializeStatistics(svtkDataObject* input, svtkDataObject* output);
  virtual void InitializeStatistics(svtkDataSet* input, svtkDataSet* output);
  virtual void InitializeStatistics(svtkGraph* input, svtkGraph* output);
  virtual void InitializeStatistics(svtkCompositeDataSet* input, svtkCompositeDataSet* output);
  virtual void InitializeArrays(svtkFieldData* inFd, svtkFieldData* outFd);
  virtual void InitializeArray(svtkDataArray* array, svtkFieldData* outFd);

  virtual void AccumulateStatistics(svtkDataObject* input, svtkDataObject* output);
  virtual void AccumulateStatistics(svtkDataSet* input, svtkDataSet* output);
  virtual void AccumulateStatistics(svtkGraph* input, svtkGraph* output);
  virtual void AccumulateStatistics(svtkCompositeDataSet* input, svtkCompositeDataSet* output);
  virtual void AccumulateArrays(svtkFieldData* inFd, svtkFieldData* outFd);

  virtual void PostExecute(svtkDataObject* input, svtkDataObject* output);
  virtual void PostExecute(svtkDataSet* input, svtkDataSet* output);
  virtual void PostExecute(svtkGraph* input, svtkGraph* output);
  virtual void PostExecute(svtkCompositeDataSet* input, svtkCompositeDataSet* output);
  virtual void FinishArrays(svtkFieldData* inFd, svtkFieldData* outFd);

  virtual svtkDataArray* GetArray(
    svtkFieldData* fieldData, svtkDataArray* inArray, const char* nameSuffix);

private:
  svtkTemporalStatistics(const svtkTemporalStatistics&) = delete;
  void operator=(const svtkTemporalStatistics&) = delete;

  //@{
  /**
   * Used to avoid multiple warnings for the same filter when
   * the number of points or cells in the data set is changing
   * between time steps.
   */
  bool GeneratedChangingTopologyWarning;
  //@}
};

#endif //_svtkTemporalStatistics_h
