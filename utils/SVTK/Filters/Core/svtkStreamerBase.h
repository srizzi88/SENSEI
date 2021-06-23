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

/**
 * @class   svtkStreamerBase
 * @brief   Superclass for filters that stream input pipeline
 *
 *
 * This class can be used as a superclass for filters that want to
 * stream their input pipeline by making multiple execution passes.
 * The subclass needs to set NumberOfPasses to > 1 before execution (
 * usual in the constructor or in RequestInformation) to initiate
 * streaming. svtkStreamerBase will handle streaming while calling
 * ExecutePass() during each pass. CurrentIndex can be used to obtain
 * the index for the current pass. Finally, PostExecute() is called
 * after the last pass and can be used to cleanup any internal data
 * structures and create the actual output.
 */

#ifndef svtkStreamerBase_h
#define svtkStreamerBase_h

#include "svtkAlgorithm.h"
#include "svtkFiltersCoreModule.h" // For export macro

class SVTKFILTERSCORE_EXPORT svtkStreamerBase : public svtkAlgorithm
{
public:
  svtkTypeMacro(svtkStreamerBase, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

protected:
  svtkStreamerBase();
  ~svtkStreamerBase() override;

  virtual int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) = 0;

  virtual int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  // This method is called during each execution pass. Subclasses
  // should implement this to do actual work.
  virtual int ExecutePass(
    svtkInformationVector** inputVector, svtkInformationVector* outputVector) = 0;

  // This method is called after streaming is completed. Subclasses
  // can override this method to perform cleanup.
  virtual int PostExecute(svtkInformationVector**, svtkInformationVector*) { return 1; }

  unsigned int NumberOfPasses;
  unsigned int CurrentIndex;

private:
  svtkStreamerBase(const svtkStreamerBase&) = delete;
  void operator=(const svtkStreamerBase&) = delete;
};

#endif //_svtkStreamerBase_h
