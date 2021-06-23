/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDemandDrivenPipeline.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDemandDrivenPipeline
 * @brief   Executive supporting on-demand execution.
 *
 * svtkDemandDrivenPipeline is an executive that will execute an
 * algorithm only when its outputs are out-of-date with respect to its
 * inputs.
 */

#ifndef svtkDemandDrivenPipeline_h
#define svtkDemandDrivenPipeline_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkExecutive.h"

class svtkAbstractArray;
class svtkDataArray;
class svtkDataSetAttributes;
class svtkDemandDrivenPipelineInternals;
class svtkFieldData;
class svtkInformation;
class svtkInformationIntegerKey;
class svtkInformationVector;
class svtkInformationKeyVectorKey;
class svtkInformationUnsignedLongKey;

///\defgroup InformationKeys Information Keys
/// The SVTK pipeline relies on algorithms providing information about their
/// input and output and responding to requests.  The information objects used
/// to perform these actions map known keys to values.  This is a list of keys
/// that information objects use and what each key should be used for.
///

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkDemandDrivenPipeline : public svtkExecutive
{
public:
  static svtkDemandDrivenPipeline* New();
  svtkTypeMacro(svtkDemandDrivenPipeline, svtkExecutive);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Generalized interface for asking the executive to fulfill update
   * requests.
   */
  svtkTypeBool ProcessRequest(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

  /**
   * Implement the pipeline modified time request.
   */
  int ComputePipelineMTime(svtkInformation* request, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec, int requestFromOutputPort, svtkMTimeType* mtime) override;

  //@{
  /**
   * Bring the algorithm's outputs up-to-date.  Returns 1 for success
   * and 0 for failure.
   */
  svtkTypeBool Update() override;
  svtkTypeBool Update(int port) override;
  //@}

  //@{
  /**
   * Get the PipelineMTime for this exective.
   */
  svtkGetMacro(PipelineMTime, svtkMTimeType);
  //@}

  /**
   * Set whether the given output port releases data when it is
   * consumed.  Returns 1 if the value changes and 0 otherwise.
   */
  virtual int SetReleaseDataFlag(int port, int n);

  /**
   * Get whether the given output port releases data when it is consumed.
   */
  virtual int GetReleaseDataFlag(int port);

  /**
   * Bring the PipelineMTime up to date.
   */
  virtual int UpdatePipelineMTime();

  /**
   * Bring the output data object's existence up to date.  This does
   * not actually produce data, but does create the data object that
   * will store data produced during the UpdateData step.
   */
  int UpdateDataObject() override;

  /**
   * Bring the output information up to date.
   */
  int UpdateInformation() override;

  /**
   * Bring the output data up to date.  This should be called only
   * when information is up to date.  Use the Update method if it is
   * not known that the information is up to date.
   */
  virtual int UpdateData(int outputPort);

  /**
   * Key defining a request to make sure the output data objects exist.
   * @ingroup InformationKeys
   */
  static svtkInformationRequestKey* REQUEST_DATA_OBJECT();

  /**
   * Key defining a request to make sure the output information is up to date.
   * @ingroup InformationKeys
   */
  static svtkInformationRequestKey* REQUEST_INFORMATION();

  /**
   * Key defining a request to make sure the output data are up to date.
   * @ingroup InformationKeys
   */
  static svtkInformationRequestKey* REQUEST_DATA();

  /**
   * Key defining a request to mark outputs that will NOT be generated
   * during a REQUEST_DATA.
   * @ingroup InformationKeys
   */
  static svtkInformationRequestKey* REQUEST_DATA_NOT_GENERATED();

  /**
   * Key to specify in pipeline information the request that data be
   * released after it is used.
   * @ingroup InformationKeys
   */
  static svtkInformationIntegerKey* RELEASE_DATA();

  /**
   * Key to store a mark for an output that will not be generated.
   * Algorithms use this to tell the executive that they will not
   * generate certain outputs for a REQUEST_DATA.
   * @ingroup InformationKeys
   */
  static svtkInformationIntegerKey* DATA_NOT_GENERATED();

  /**
   * Create (New) and return a data object of the given type.
   * This is here for backwards compatibility. Use
   * svtkDataObjectTypes::NewDataObject() instead.
   */
  static svtkDataObject* NewDataObject(const char* type);

protected:
  svtkDemandDrivenPipeline();
  ~svtkDemandDrivenPipeline() override;

  // Helper methods to send requests to the algorithm.
  virtual int ExecuteDataObject(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo);
  virtual int ExecuteInformation(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo);
  virtual int ExecuteData(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo);

  // Reset the pipeline update values in the given output information object.
  void ResetPipelineInformation(int, svtkInformation*) override;

  // Check whether the data object in the pipeline information for an
  // output port exists and has a valid type.
  virtual int CheckDataObject(int port, svtkInformationVector* outInfo);

  // Input connection validity checkers.
  int InputCountIsValid(svtkInformationVector**);
  int InputCountIsValid(int port, svtkInformationVector**);
  int InputTypeIsValid(svtkInformationVector**);
  int InputTypeIsValid(int port, svtkInformationVector**);
  virtual int InputTypeIsValid(int port, int index, svtkInformationVector**);
  int InputFieldsAreValid(svtkInformationVector**);
  int InputFieldsAreValid(int port, svtkInformationVector**);
  virtual int InputFieldsAreValid(int port, int index, svtkInformationVector**);

  // Field existence checkers.
  int DataSetAttributeExists(svtkDataSetAttributes* dsa, svtkInformation* field);
  int FieldArrayExists(svtkFieldData* data, svtkInformation* field);
  int ArrayIsValid(svtkAbstractArray* array, svtkInformation* field);

  // Input port information checkers.
  int InputIsOptional(int port);
  int InputIsRepeatable(int port);

  // Decide whether the output data need to be generated.
  virtual int NeedToExecuteData(
    int outputPort, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec);

  // Handle before/after operations for ExecuteData method.
  virtual void ExecuteDataStart(
    svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec);
  virtual void ExecuteDataEnd(
    svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec);
  virtual void MarkOutputsGenerated(
    svtkInformation* request, svtkInformationVector** inInfoVec, svtkInformationVector* outInfoVec);

  // Largest MTime of any algorithm on this executive or preceding
  // executives.
  svtkMTimeType PipelineMTime;

  // Time when information or data were last generated.
  svtkTimeStamp DataObjectTime;
  svtkTimeStamp InformationTime;
  svtkTimeStamp DataTime;

  friend class svtkCompositeDataPipeline;

  svtkInformation* InfoRequest;
  svtkInformation* DataObjectRequest;
  svtkInformation* DataRequest;

private:
  svtkDemandDrivenPipeline(const svtkDemandDrivenPipeline&) = delete;
  void operator=(const svtkDemandDrivenPipeline&) = delete;
};

#endif
