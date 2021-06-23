/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkUndirectedGraphAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkUndirectedGraphAlgorithm
 * @brief   Superclass for algorithms that produce undirected graph as output
 *
 *
 * svtkUndirectedGraphAlgorithm is a convenience class to make writing algorithms
 * easier. It is also designed to help transition old algorithms to the new
 * pipeline edgehitecture. There are some assumptions and defaults made by this
 * class you should be aware of. This class defaults such that your filter
 * will have one input port and one output port. If that is not the case
 * simply change it with SetNumberOfInputPorts etc. See this class
 * constructor for the default. This class also provides a FillInputPortInfo
 * method that by default says that all inputs will be Graph. If that
 * isn't the case then please override this method in your subclass.
 *
 * @par Thanks:
 * Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
 * Sandia National Laboratories for their help in developing this class.
 */

#ifndef svtkUndirectedGraphAlgorithm_h
#define svtkUndirectedGraphAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkUndirectedGraph.h"            // makes things a bit easier

class svtkDataSet;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkUndirectedGraphAlgorithm : public svtkAlgorithm
{
public:
  static svtkUndirectedGraphAlgorithm* New();
  svtkTypeMacro(svtkUndirectedGraphAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * see svtkAlgorithm for details
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkUndirectedGraph* GetOutput() { return this->GetOutput(0); }
  svtkUndirectedGraph* GetOutput(int index);

  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection() to
   * setup a pipeline connection.
   */
  void SetInputData(svtkDataObject* obj) { this->SetInputData(0, obj); }
  void SetInputData(int index, svtkDataObject* obj);

protected:
  svtkUndirectedGraphAlgorithm();
  ~svtkUndirectedGraphAlgorithm() override;

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
  svtkUndirectedGraphAlgorithm(const svtkUndirectedGraphAlgorithm&) = delete;
  void operator=(const svtkUndirectedGraphAlgorithm&) = delete;
};

#endif
