/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageAlgorithm.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageAlgorithm
 * @brief   Generic algorithm superclass for image algs
 *
 * svtkImageAlgorithm is a filter superclass that hides much of the
 * pipeline complexity. It handles breaking the pipeline execution
 * into smaller extents so that the svtkImageData limits are observed. It
 * also provides support for multithreading. If you don't need any of this
 * functionality, consider using svtkSimpleImageToImageFilter instead.
 * @sa
 * svtkSimpleImageToImageFilter
 */

#ifndef svtkImageAlgorithm_h
#define svtkImageAlgorithm_h

#include "svtkAlgorithm.h"
#include "svtkCommonExecutionModelModule.h" // For export macro

class svtkDataSet;
class svtkImageData;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkImageAlgorithm : public svtkAlgorithm
{
public:
  svtkTypeMacro(svtkImageAlgorithm, svtkAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Get the output data object for a port on this algorithm.
   */
  svtkImageData* GetOutput();
  svtkImageData* GetOutput(int);
  virtual void SetOutput(svtkDataObject* d);
  //@}

  /**
   * Process a request from the executive.  For svtkImageAlgorithm, the
   * request will be delegated to one of the following methods: RequestData,
   * RequestInformation, or RequestUpdateExtent.
   */
  svtkTypeBool ProcessRequest(
    svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection to
   * setup a pipeline connection.
   */
  void SetInputData(svtkDataObject*);
  void SetInputData(int, svtkDataObject*);
  //@}

  //@{
  /**
   * Get a data object for one of the input port connections.  The use
   * of this method is strongly discouraged, but some filters that were
   * written a long time ago still use this method.
   */
  svtkDataObject* GetInput(int port);
  svtkDataObject* GetInput() { return this->GetInput(0); }
  svtkImageData* GetImageDataInput(int port);
  //@}

  //@{
  /**
   * Assign a data object as input. Note that this method does not
   * establish a pipeline connection. Use SetInputConnection to
   * setup a pipeline connection.
   */
  virtual void AddInputData(svtkDataObject*);
  virtual void AddInputData(int, svtkDataObject*);
  //@}

protected:
  svtkImageAlgorithm();
  ~svtkImageAlgorithm() override;

  /**
   * Subclasses can reimplement this method to collect information
   * from their inputs and set information for their outputs.
   */
  virtual int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  /**
   * Subclasses can reimplement this method to translate the update
   * extent requests from each output port into update extent requests
   * for the input connections.
   */
  virtual int RequestUpdateExtent(svtkInformation*, svtkInformationVector**, svtkInformationVector*);

  /**
   * Convenience method to copy the scalar type and number of components
   * from the input data to the output data.  You will generally want to
   * call this from inside your RequestInformation method, unless you
   * want the output data to have a different scalar type or number of
   * components from the input.
   */
  virtual void CopyInputArrayAttributesToOutput(svtkInformation* request,
    svtkInformationVector** inputVector, svtkInformationVector* outputVector);

  /**
   * This is called in response to a REQUEST_DATA request from the
   * executive.  Subclasses should override either this method or the
   * ExecuteDataWithInformation method in order to generate data for
   * their outputs.  For images, the output arrays will already be
   * allocated, so all that is necessary is to fill in the voxel values.
   */
  virtual int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector);

  /**
   * This is a convenience method that is implemented in many subclasses
   * instead of RequestData.  It is called by RequestData.
   */
  virtual void ExecuteDataWithInformation(svtkDataObject* output, svtkInformation* outInfo);

  //@{
  /**
   * This method is the old style execute method, provided for the sake
   * of backwards compatibility with older filters and readers.
   */
  virtual void ExecuteData(svtkDataObject* output);
  virtual void Execute();
  //@}

  //@{
  /**
   * Allocate the output data.  This will be called before RequestData,
   * it is not necessary for subclasses to call this method themselves.
   */
  virtual void AllocateOutputData(svtkImageData* out, svtkInformation* outInfo, int* uExtent);
  virtual svtkImageData* AllocateOutputData(svtkDataObject* out, svtkInformation* outInfo);
  //@}

  /**
   * Copy the other point and cell data.  Subclasses will almost never
   * need to reimplement this method.
   */
  virtual void CopyAttributeData(
    svtkImageData* in, svtkImageData* out, svtkInformationVector** inputVector);

  //@{
  /**
   * These method should be reimplemented by subclasses that have
   * more than a single input or single output.
   * See svtkAlgorithm for more information.
   */
  int FillOutputPortInformation(int port, svtkInformation* info) override;
  int FillInputPortInformation(int port, svtkInformation* info) override;
  //@}

private:
  svtkImageAlgorithm(const svtkImageAlgorithm&) = delete;
  void operator=(const svtkImageAlgorithm&) = delete;
};

#endif
