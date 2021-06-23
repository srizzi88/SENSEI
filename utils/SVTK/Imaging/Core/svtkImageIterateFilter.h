/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageIterateFilter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageIterateFilter
 * @brief   Multiple executes per update.
 *
 * svtkImageIterateFilter is a filter superclass that supports calling execute
 * multiple times per update.  The largest hack/open issue is that the input
 * and output caches are temporarily changed to "fool" the subclasses.  I
 * believe the correct solution is to pass the in and out cache to the
 * subclasses methods as arguments.  Now the data is passes.  Can the caches
 * be passed, and data retrieved from the cache?
 */

#ifndef svtkImageIterateFilter_h
#define svtkImageIterateFilter_h

#include "svtkImagingCoreModule.h" // For export macro
#include "svtkThreadedImageAlgorithm.h"

class SVTKIMAGINGCORE_EXPORT svtkImageIterateFilter : public svtkThreadedImageAlgorithm
{
public:
  svtkTypeMacro(svtkImageIterateFilter, svtkThreadedImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get which iteration is current being performed. Normally the
   * user will not access this method.
   */
  svtkGetMacro(Iteration, int);
  svtkGetMacro(NumberOfIterations, int);
  //@}

protected:
  svtkImageIterateFilter();
  ~svtkImageIterateFilter() override;

  // Implement standard requests by calling iterative versions the
  // specified number of times.
  int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  // Iterative versions of standard requests.  These are given the
  // pipeline information object for the in/out pair at each
  // iteration.
  virtual int IterativeRequestInformation(svtkInformation* in, svtkInformation* out);
  virtual int IterativeRequestUpdateExtent(svtkInformation* in, svtkInformation* out);
  virtual int IterativeRequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  virtual void SetNumberOfIterations(int num);

  // for filters that execute multiple times.
  int NumberOfIterations;
  int Iteration;
  // A list of intermediate caches that is created when
  // is called SetNumberOfIterations()
  svtkAlgorithm** IterationData;

  svtkInformationVector* InputVector;
  svtkInformationVector* OutputVector;

private:
  svtkImageIterateFilter(const svtkImageIterateFilter&) = delete;
  void operator=(const svtkImageIterateFilter&) = delete;
};

#endif
