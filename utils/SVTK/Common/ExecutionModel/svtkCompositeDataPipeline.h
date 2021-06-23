/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositeDataPipeline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositeDataPipeline
 * @brief   Executive supporting composite datasets.
 *
 * svtkCompositeDataPipeline is an executive that supports the processing of
 * composite dataset. It supports algorithms that are aware of composite
 * dataset as well as those that are not. Type checking is performed at run
 * time. Algorithms that are not composite dataset-aware have to support
 * all dataset types contained in the composite dataset. The pipeline
 * execution can be summarized as follows:
 *
 * * REQUEST_INFORMATION: The producers have to provide information about
 * the contents of the composite dataset in this pass.
 * Sources that can produce more than one piece (note that a piece is
 * different than a block; each piece consistes of 0 or more blocks) should
 * set CAN_HANDLE_PIECE_REQUEST.
 *
 * * REQUEST_UPDATE_EXTENT: This pass is identical to the one implemented
 * in svtkStreamingDemandDrivenPipeline
 *
 * * REQUEST_DATA: This is where the algorithms execute. If the
 * svtkCompositeDataPipeline is assigned to a simple filter,
 * it will invoke the  svtkStreamingDemandDrivenPipeline passes in a loop,
 * passing a different block each time and will collect the results in a
 * composite dataset.
 * @sa
 *  svtkCompositeDataSet
 */

#ifndef svtkCompositeDataPipeline_h
#define svtkCompositeDataPipeline_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkStreamingDemandDrivenPipeline.h"
#include <svtkSmartPointer.h> // smart pointer

#include <vector> // for vector in return type

class svtkCompositeDataSet;
class svtkCompositeDataIterator;
class svtkInformationDoubleKey;
class svtkInformationIntegerVectorKey;
class svtkInformationObjectBaseKey;
class svtkInformationStringKey;
class svtkInformationDataObjectKey;
class svtkInformationIntegerKey;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkCompositeDataPipeline
  : public svtkStreamingDemandDrivenPipeline
{
public:
  static svtkCompositeDataPipeline* New();
  svtkTypeMacro(svtkCompositeDataPipeline, svtkStreamingDemandDrivenPipeline);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Returns the data object stored with the DATA_OBJECT() in the
   * output port
   */
  svtkDataObject* GetCompositeOutputData(int port);

  /**
   * Returns the data object stored with the DATA_OBJECT() in the
   * input port
   */
  svtkDataObject* GetCompositeInputData(int port, int index, svtkInformationVector** inInfoVec);

  /**
   * An integer key that indicates to the source to load all requested
   * blocks specified in UPDATE_COMPOSITE_INDICES.
   */
  static svtkInformationIntegerKey* LOAD_REQUESTED_BLOCKS();

  /**
   * COMPOSITE_DATA_META_DATA is a key placed in the output-port information by
   * readers/sources producing composite datasets. This meta-data provides
   * information about the structure of the composite dataset and things like
   * data-bounds etc.
   * *** THIS IS AN EXPERIMENTAL FEATURE. IT MAY CHANGE WITHOUT NOTICE ***
   */
  static svtkInformationObjectBaseKey* COMPOSITE_DATA_META_DATA();

  /**
   * UPDATE_COMPOSITE_INDICES is a key placed in the request to request a set of
   * composite indices from a reader/source producing composite dataset.
   * Typically, the reader publishes its structure using
   * COMPOSITE_DATA_META_DATA() and then the sink requests blocks of interest
   * using UPDATE_COMPOSITE_INDICES().
   * Note that UPDATE_COMPOSITE_INDICES has to be sorted vector with increasing
   * indices.
   * *** THIS IS AN EXPERIMENTAL FEATURE. IT MAY CHANGE WITHOUT NOTICE ***
   */
  static svtkInformationIntegerVectorKey* UPDATE_COMPOSITE_INDICES();

  /**
   * BLOCK_AMOUNT_OF_DETAIL is a key placed in the information about a multi-block
   * dataset that indicates how complex the block is.  It is intended to work with
   * multi-resolution streaming code.  For example in a multi-resolution dataset of
   * points, this key might store the number of points.
   * *** THIS IS AN EXPERIMENTAL FEATURE. IT MAY CHANGE WITHOUT NOTICE ***
   */
  static svtkInformationDoubleKey* BLOCK_AMOUNT_OF_DETAIL();

protected:
  svtkCompositeDataPipeline();
  ~svtkCompositeDataPipeline() override;

  int ForwardUpstream(svtkInformation* request) override;
  virtual int ForwardUpstream(int i, int j, svtkInformation* request);

  // Copy information for the given request.
  void CopyDefaultInformation(svtkInformation* request, int direction,
    svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec) override;

  virtual void PushInformation(svtkInformation*);
  virtual void PopInformation(svtkInformation*);

  int ExecuteDataObject(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

  int ExecuteData(svtkInformation* request, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec) override;

  void ExecuteDataStart(svtkInformation* request, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec) override;

  // Override this check to account for update extent.
  int NeedToExecuteData(
    int outputPort, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec) override;

  // Check whether the data object in the pipeline information exists
  // and has a valid type.
  virtual int CheckCompositeData(
    svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec);

  // True when the pipeline is iterating over the current (simple) filter
  // to produce composite output. In this case, ExecuteDataStart() should
  // NOT Initialize() the composite output.
  int InLocalLoop;

  virtual void ExecuteSimpleAlgorithm(svtkInformation* request, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec, int compositePort);

  virtual void ExecuteEach(svtkCompositeDataIterator* iter, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec, int compositePort, int connection, svtkInformation* request,
    std::vector<svtkSmartPointer<svtkCompositeDataSet> >& compositeOutput);

  std::vector<svtkDataObject*> ExecuteSimpleAlgorithmForBlock(svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec, svtkInformation* inInfo, svtkInformation* request,
    svtkDataObject* dobj);

  bool ShouldIterateOverInput(svtkInformationVector** inInfoVec, int& compositePort);

  int InputTypeIsValid(int port, int index, svtkInformationVector** inInfoVec) override;

  svtkInformation* InformationCache;

  svtkInformation* GenericRequest;
  svtkInformation* InformationRequest;

  void ResetPipelineInformation(int port, svtkInformation*) override;

  /**
   * Tries to create the best possible composite data output for the given input
   * and non-composite algorithm output. Returns a new instance on success.
   * It's main purpose is
   * to determine if svtkHierarchicalBoxDataSet can be propagated as
   * svtkHierarchicalBoxDataSet in the output (if the algorithm can produce
   * svtkUniformGrid given svtkUniformGrid inputs) or if it should be downgraded
   * to a svtkMultiBlockDataSet.
   */
  std::vector<svtkSmartPointer<svtkDataObject> > CreateOutputCompositeDataSet(
    svtkCompositeDataSet* input, int compositePort, int numOutputPorts);

  // Override this to handle UPDATE_COMPOSITE_INDICES().
  void MarkOutputsGenerated(svtkInformation* request, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec) override;

  int NeedToExecuteBasedOnCompositeIndices(svtkInformation* outInfo);

  // Because we sometimes have to swap between "simple" data types and composite
  // data types, we sometimes want to skip resetting the pipeline information.
  static svtkInformationIntegerKey* SUPPRESS_RESET_PI();

  /**
   * COMPOSITE_INDICES() is put in the output information by the executive if
   * the request has UPDATE_COMPOSITE_INDICES() using the generated composite
   * dataset's structure.
   * Note that COMPOSITE_INDICES has to be sorted vector with increasing
   * indices.
   * *** THIS IS AN EXPERIMENTAL FEATURE. IT MAY CHANGE WITHOUT NOTICE ***
   */
  static svtkInformationIntegerVectorKey* DATA_COMPOSITE_INDICES();

private:
  svtkCompositeDataPipeline(const svtkCompositeDataPipeline&) = delete;
  void operator=(const svtkCompositeDataPipeline&) = delete;
};

#endif
