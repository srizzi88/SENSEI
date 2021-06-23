/*=========================================================================

 Program:   Visualization Toolkit
 Module:    svtkUniformGridAMRAlgorithm.h

 Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
 All rights reserved.
 See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

 This software is distributed WITHOUT ANY WARRANTY; without even
 the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 PURPOSE.  See the above copyright notice for more information.

 =========================================================================*/
/**
 * @class   svtkUniformGridAMRAlgorithm
 *  svtkUniformGridAMR as output.
 *
 *
 *  A base class for all algorithms that take as input any type of data object
 *  including composite datasets and produce svtkUniformGridAMR in the output.
 */

#ifndef svtkUniformGridAMRAlgorithm_h
#define svtkUniformGridAMRAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro

class svtkUniformGridAMR;
class svtkInformation;
class svtkInformationVector;
class svtkExecutive;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkUniformGridAMRAlgorithm : public svtkAlgorithm
{
public:
  static svtkUniformGridAMRAlgorithm* New();
  svtkTypeMacro(svtkUniformGridAMRAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm
   */
  svtkUniformGridAMR* GetOutput();
  svtkUniformGridAMR* GetOutput(int);
  //@}

  //@{
  /**
   * Set an input of this algorithm.
   */
  void SetInputData(svtkDataObject*);
  void SetInputData(int, svtkDataObject*);
  //@}

  /**
   * See svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

protected:
  svtkUniformGridAMRAlgorithm();
  ~svtkUniformGridAMRAlgorithm() override;

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

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*)
  {
    return 1;
  }

  /**
   * Create a default executive
   */
  svtkExecutive* CreateDefaultExecutive() override;

  //@{
  /**
   * See algorithm for more info.
   */
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  //@}

  svtkDataObject* GetInput(int port);

private:
  svtkUniformGridAMRAlgorithm(const svtkUniformGridAMRAlgorithm&) = delete;
  void operator=(const svtkUniformGridAMRAlgorithm&) = delete;
};

#endif /* SVTKUNIFORMGRIDAMRALGORITHM_H_ */
