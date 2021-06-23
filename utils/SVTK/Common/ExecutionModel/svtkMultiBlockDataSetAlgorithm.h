/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMultiBlockDataSetAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMultiBlockDataSetAlgorithm
 * @brief   Superclass for algorithms that produce only svtkMultiBlockDataSet as output
 *
 * Algorithms that take any type of data object (including composite dataset)
 * and produce a svtkMultiBlockDataSet in the output can subclass from this
 * class.
 */

#ifndef svtkMultiBlockDataSetAlgorithm_h
#define svtkMultiBlockDataSetAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro

class svtkMultiBlockDataSet;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkMultiBlockDataSetAlgorithm : public svtkAlgorithm
{
public:
  static svtkMultiBlockDataSetAlgorithm* New();
  svtkTypeMacro(svtkMultiBlockDataSetAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkMultiBlockDataSet* GetOutput();
  svtkMultiBlockDataSet* GetOutput(int);
  //@}

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(svtkDataObject*);
  void SetInputData(int, svtkDataObject*);
  //@}

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

protected:
  svtkMultiBlockDataSetAlgorithm();
  ~svtkMultiBlockDataSetAlgorithm() override {}

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestDataObject(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }

  //@{
  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }
  //@}

  // Create a default executive.
  svtkExecutive* CreateDefaultExecutive() override;

  // see algorithm for more info
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

  svtkDataObject* GetInput(int port);

private:
  svtkMultiBlockDataSetAlgorithm(const svtkMultiBlockDataSetAlgorithm&) = delete;
  void operator=(const svtkMultiBlockDataSetAlgorithm&) = delete;
};

#endif
