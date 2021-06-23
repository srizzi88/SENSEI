/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeDataSetAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeDataSetAlgorithm
 * @brief   Superclass for algorithms that produce only svtkCompositeDataSet as output
 *
 * Algorithms that take any type of data object (including composite dataset)
 * and produce a svtkCompositeDataSet in the output can subclass from this
 * class.
 */

#ifndef svtkCompositeDataSetAlgorithm_h
#define svtkCompositeDataSetAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro

class svtkCompositeDataSet;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkCompositeDataSetAlgorithm : public svtkAlgorithm
{
public:
  static svtkCompositeDataSetAlgorithm* New();
  svtkTypeMacro(svtkCompositeDataSetAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkCompositeDataSet* GetOutput();
  svtkCompositeDataSet* GetOutput(int);
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
  svtkCompositeDataSetAlgorithm();
  ~svtkCompositeDataSetAlgorithm() override {}

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
  svtkCompositeDataSetAlgorithm(const svtkCompositeDataSetAlgorithm&) = delete;
  void operator=(const svtkCompositeDataSetAlgorithm&) = delete;
};

#endif
