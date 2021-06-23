/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThreadedCompositeDataPipeline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

  This software is distributed WITHOUT ANY WARRANTY; without even
  the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
  PURPOSE.  See the above copyright notice for more information.

  =========================================================================*/
/**
 * @class   svtkThreadedCompositeDataPipeline
 * @brief   Executive that works in parallel
 *
 * svtkThreadedCompositeDataPipeline processes a composite data object in
 * parallel using the SMP framework. It does this by creating a vector of
 * data objects (the pieces of the composite data) and processing them
 * using svtkSMPTools::For. Note that this requires that the
 * algorithm implement all pipeline passes in a re-entrant way. It should
 * store/retrieve all state changes using input and output information
 * objects, which are unique to each thread.
 */

#ifndef svtkThreadedCompositeDataPipeline_h
#define svtkThreadedCompositeDataPipeline_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkCompositeDataPipeline.h"

class svtkInformationVector;
class svtkInformation;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkThreadedCompositeDataPipeline
  : public svtkCompositeDataPipeline
{
public:
  static svtkThreadedCompositeDataPipeline* New();
  svtkTypeMacro(svtkThreadedCompositeDataPipeline, svtkCompositeDataPipeline);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * An API to CallAlgorithm that allows you to pass in the info objects to
   * be used
   */
  int CallAlgorithm(svtkInformation* request, int direction, svtkInformationVector** inInfo,
    svtkInformationVector* outInfo) override;

protected:
  svtkThreadedCompositeDataPipeline();
  ~svtkThreadedCompositeDataPipeline() override;
  void ExecuteEach(svtkCompositeDataIterator* iter, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec, int compositePort, int connection, svtkInformation* request,
    std::vector<svtkSmartPointer<svtkCompositeDataSet> >& compositeOutput) override;

private:
  svtkThreadedCompositeDataPipeline(const svtkThreadedCompositeDataPipeline&) = delete;
  void operator=(const svtkThreadedCompositeDataPipeline&) = delete;
  friend class ProcessBlock;
};

#endif
