/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiTimeStepAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMultiTimeStepAlgorithm
 * @brief   Superclass for algorithms that would like to
 *  make multiple time requests
 *
 * This class can be inherited by any algorithm that wishes to make multiple
 * time requests upstream.
 * The child class uses UPDATE_TIME_STEPS to make the time requests and
 * use set of time-stamped data objects are stored in time order
 * in a svtkMultiBlockDataSet object.
 */

#ifndef svtkMultiTimeStepAlgorithm_h
#define svtkMultiTimeStepAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkSmartPointer.h"               //needed for a private variable

#include "svtkDataObject.h" // needed for the smart pointer
#include <vector>          //needed for a private variable

class svtkInformationDoubleVectorKey;
class svtkMultiBlockDataSet;
class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkMultiTimeStepAlgorithm : public svtkAlgorithm
{
public:
  static svtkMultiTimeStepAlgorithm* New();
  svtkTypeMacro(svtkMultiTimeStepAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkMultiTimeStepAlgorithm();

  ~svtkMultiTimeStepAlgorithm() override {}

  /**
   * This is filled by the child class to request multiple time steps
   */
  static svtkInformationDoubleVectorKey* UPDATE_TIME_STEPS();

  //@{
  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }
  //@}

  //@{
  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }
  //@}

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }

  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  bool CacheData;
  unsigned int NumberOfCacheEntries;

private:
  svtkMultiTimeStepAlgorithm(const svtkMultiTimeStepAlgorithm&) = delete;
  void operator=(const svtkMultiTimeStepAlgorithm&) = delete;

  svtkSmartPointer<svtkMultiBlockDataSet> MDataSet; // stores all the temporal data sets
  int RequestUpdateIndex;                         // keep track of the time looping index
  std::vector<double> UpdateTimeSteps;            // store the requested time steps

  bool IsInCache(double time, size_t& idx);

  struct TimeCache
  {
    TimeCache(double time, svtkDataObject* data)
      : TimeValue(time)
      , Data(data)
    {
    }
    double TimeValue;
    svtkSmartPointer<svtkDataObject> Data;
  };

  std::vector<TimeCache> Cache;
};

#endif
