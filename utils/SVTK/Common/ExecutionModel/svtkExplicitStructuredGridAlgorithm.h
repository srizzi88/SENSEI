/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExplicitStructuredGridAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExplicitStructuredGridAlgorithm
 * @brief   Superclass for algorithms that produce only
 * explicit structured grid as output.
 */

#ifndef svtkExplicitStructuredGridAlgorithm_h
#define svtkExplicitStructuredGridAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro

class svtkDataSet;
class svtkExplicitStructuredGrid;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkExplicitStructuredGridAlgorithm : public svtkAlgorithm
{
public:
  static svtkExplicitStructuredGridAlgorithm* New();
  svtkTypeMacro(svtkExplicitStructuredGridAlgorithm, svtkAlgorithm);

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkExplicitStructuredGrid* GetOutput();
  svtkExplicitStructuredGrid* GetOutput(int);
  virtual void SetOutput(svtkDataObject* d);
  //@}

  /**
   * see svtkAlgorithm for details
   */
  virtual svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  // this method is not recommended for use, but lots of old style filters
  // use it
  svtkDataObject* GetInput();
  svtkDataObject* GetInput(int port);
  svtkExplicitStructuredGrid* GetExplicitStructuredGridInput(int port);

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
  svtkExplicitStructuredGridAlgorithm();
  ~svtkExplicitStructuredGridAlgorithm() override = default;

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
  virtual int FillOutputPortInformation(int port, svtkInformation* info) override;
  virtual int FillInputPortInformation(int port, svtkInformation* info) override;

private:
  svtkExplicitStructuredGridAlgorithm(const svtkExplicitStructuredGridAlgorithm&) = delete;
  void operator=(const svtkExplicitStructuredGridAlgorithm&) = delete;
};

#endif
