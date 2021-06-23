/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkHierarchicalBoxDataSetAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkHierarchicalBoxDataSetAlgorithm
 * @brief   superclass for algorithms that
 * produce svtkHierarchicalBoxDataSet as output.
 *
 * Algorithms that take any type of data object (including composite dataset)
 * and produce a svtkHierarchicalBoxDataSet in the output can subclass from this
 * class.
 */

#ifndef svtkHierarchicalBoxDataSetAlgorithm_h
#define svtkHierarchicalBoxDataSetAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro

class svtkHierarchicalBoxDataSet;
class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkHierarchicalBoxDataSetAlgorithm : public svtkAlgorithm
{
public:
  static svtkHierarchicalBoxDataSetAlgorithm* New();
  svtkTypeMacro(svtkHierarchicalBoxDataSetAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkHierarchicalBoxDataSet* GetOutput();
  svtkHierarchicalBoxDataSet* GetOutput(int);
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
  svtkHierarchicalBoxDataSetAlgorithm();
  ~svtkHierarchicalBoxDataSetAlgorithm() override;

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
  svtkHierarchicalBoxDataSetAlgorithm(const svtkHierarchicalBoxDataSetAlgorithm&) = delete;
  void operator=(const svtkHierarchicalBoxDataSetAlgorithm&) = delete;
};

#endif
