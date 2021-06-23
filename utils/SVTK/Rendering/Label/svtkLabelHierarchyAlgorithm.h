/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLabelHierarchyAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLabelHierarchyAlgorithm
 * @brief   Superclass for algorithms that produce only label hierarchies as output
 *
 *
 * svtkLabelHierarchyAlgorithm is a convenience class to make writing algorithms
 * easier. It is also designed to help transition old algorithms to the new
 * pipeline architecture. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this class
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be DataObjects. If that
 * isn't the case then please override this method in your subclass. This
 * class breaks out the downstream requests into separate functions such as
 * RequestData and RequestInformation.  You should
 * implement RequestData( request, inputVec, outputVec) in subclasses.
 */

#ifndef svtkLabelHierarchyAlgorithm_h
#define svtkLabelHierarchyAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkRenderingLabelModule.h" // For export macro

class svtkDataSet;
class svtkLabelHierarchy;

class SVTKRENDERINGLABEL_EXPORT svtkLabelHierarchyAlgorithm : public svtkAlgorithm
{
public:
  static svtkLabelHierarchyAlgorithm* New();
  svtkTypeMacro(svtkLabelHierarchyAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkLabelHierarchy* GetOutput();
  svtkLabelHierarchy* GetOutput(int);
  virtual void SetOutput(svtkDataObject* d);
  //@}

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // this method is not recommended for use, but lots of old style filters use it
  svtkDataObject* GetInput();
  svtkDataObject* GetInput(int port);
  svtkLabelHierarchy* GetLabelHierarchyInput(int port);

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(svtkDataObject*);
  void SetInputData(int, svtkDataObject*);
  //@}

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use AddInputConnection() to
   * setup a pipeline connection.
   */
  void AddInputData(svtkDataObject*);
  void AddInputData(int, svtkDataObject*);
  //@}

protected:
  svtkLabelHierarchyAlgorithm();
  ~svtkLabelHierarchyAlgorithm() override;

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestDataObject(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  // convenience method
  virtual int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  /**
   * This is called by the superclass.
   * This is the method you should override.
   */
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  // see algorithm for more info
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkLabelHierarchyAlgorithm(const svtkLabelHierarchyAlgorithm&) = delete;
  void operator=(const svtkLabelHierarchyAlgorithm&) = delete;
};

#endif
