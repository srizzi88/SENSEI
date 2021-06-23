/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExecutive.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExecutive
 * @brief   Superclass for all pipeline executives in SVTK.
 *
 * svtkExecutive is the superclass for all pipeline executives in SVTK.
 * A SVTK executive is responsible for controlling one instance of
 * svtkAlgorithm.  A pipeline consists of one or more executives that
 * control data flow.  Every reader, source, writer, or data
 * processing algorithm in the pipeline is implemented in an instance
 * of svtkAlgorithm.
 */

#ifndef svtkExecutive_h
#define svtkExecutive_h

#include "svtkCommonExecutionModelModule.h" // For export macro
#include "svtkObject.h"

class svtkAlgorithm;
class svtkAlgorithmOutput;
class svtkAlgorithmToExecutiveFriendship;
class svtkDataObject;
class svtkExecutiveInternals;
class svtkInformation;
class svtkInformationExecutivePortKey;
class svtkInformationExecutivePortVectorKey;
class svtkInformationIntegerKey;
class svtkInformationRequestKey;
class svtkInformationKeyVectorKey;
class svtkInformationVector;

class SVTKCOMMONEXECUTIONMODEL_EXPORT svtkExecutive : public svtkObject
{
public:
  svtkTypeMacro(svtkExecutive, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Get the algorithm to which this executive has been assigned.
   */
  svtkAlgorithm* GetAlgorithm();

  /**
   * Generalized interface for asking the executive to fulfill
   * pipeline requests.
   */
  virtual svtkTypeBool ProcessRequest(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo);

  /**
   * A special version of ProcessRequest meant specifically for the
   * pipeline modified time request.  This is an optimization since
   * the request is called so often and it travels the full length of
   * the pipeline.  We augment the signature with method arguments
   * containing the common information, specifically the output port
   * through which the request was made and the resulting modified
   * time.  Note that unlike ProcessRequest the request information
   * object may be nullptr for this method.  It also does not contain a
   * request identification key because the request is known from the
   * method name.
   */
  virtual int ComputePipelineMTime(svtkInformation* request, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec, int requestFromOutputPort, svtkMTimeType* mtime);

  /**
   * Bring the output information up to date.
   */
  virtual int UpdateInformation() { return 1; }

  //@{
  /**
   * Bring the algorithm's outputs up-to-date.  Returns 1 for success
   * and 0 for failure.
   */
  virtual svtkTypeBool Update();
  virtual svtkTypeBool Update(int port);
  //@}

  //@{
  /**
   * Get the number of input/output ports for the algorithm associated
   * with this executive.  Returns 0 if no algorithm is set.
   */
  int GetNumberOfInputPorts();
  int GetNumberOfOutputPorts();
  //@}

  /**
   * Get the number of input connections on the given port.
   */
  int GetNumberOfInputConnections(int port);

  /**
   * Get the pipeline information object for the given output port.
   */
  virtual svtkInformation* GetOutputInformation(int port);

  /**
   * Get the pipeline information object for all output ports.
   */
  svtkInformationVector* GetOutputInformation();

  /**
   * Get the pipeline information for the given input connection.
   */
  svtkInformation* GetInputInformation(int port, int connection);

  /**
   * Get the pipeline information vectors for the given input port.
   */
  svtkInformationVector* GetInputInformation(int port);

  /**
   * Get the pipeline information vectors for all inputs
   */
  svtkInformationVector** GetInputInformation();

  /**
   * Get the executive managing the given input connection.
   */
  svtkExecutive* GetInputExecutive(int port, int connection);

  //@{
  /**
   * Get/Set the data object for an output port of the algorithm.
   */
  virtual svtkDataObject* GetOutputData(int port);
  virtual void SetOutputData(int port, svtkDataObject*, svtkInformation* info);
  virtual void SetOutputData(int port, svtkDataObject*);
  //@}

  //@{
  /**
   * Get the data object for an input port of the algorithm.
   */
  virtual svtkDataObject* GetInputData(int port, int connection);
  virtual svtkDataObject* GetInputData(int port, int connection, svtkInformationVector** inInfoVec);
  //@}

  /**
   * Get the output port that produces the given data object.
   * Works only if the data was producer by this executive's
   * algorithm.
   * virtual svtkAlgorithmOutput* GetProducerPort(svtkDataObject*);
   */

  //@{
  /**
   * Set a pointer to an outside instance of input or output
   * information vectors.  No references are held to the given
   * vectors, and setting this does not change the executive object
   * modification time.  This is a preliminary interface to use in
   * implementing filters with internal pipelines, and may change
   * without notice when a future interface is created.
   */
  void SetSharedInputInformation(svtkInformationVector** inInfoVec);
  void SetSharedOutputInformation(svtkInformationVector* outInfoVec);
  //@}

  //@{
  /**
   * Participate in garbage collection.
   */
  void Register(svtkObjectBase* o) override;
  void UnRegister(svtkObjectBase* o) override;
  //@}

  /**
   * Information key to store the executive/port number producing an
   * information object.
   */
  static svtkInformationExecutivePortKey* PRODUCER();

  /**
   * Information key to store the executive/port number pairs
   * consuming an information object.
   */
  static svtkInformationExecutivePortVectorKey* CONSUMERS();

  /**
   * Information key to store the output port number from which a
   * request is made.
   */
  static svtkInformationIntegerKey* FROM_OUTPUT_PORT();

  //@{
  /**
   * Keys to program svtkExecutive::ProcessRequest with the default
   * behavior for unknown requests.
   */
  static svtkInformationIntegerKey* ALGORITHM_BEFORE_FORWARD();
  static svtkInformationIntegerKey* ALGORITHM_AFTER_FORWARD();
  static svtkInformationIntegerKey* ALGORITHM_DIRECTION();
  static svtkInformationIntegerKey* FORWARD_DIRECTION();
  static svtkInformationKeyVectorKey* KEYS_TO_COPY();
  //@}

  enum
  {
    RequestUpstream,
    RequestDownstream
  };
  enum
  {
    BeforeForward,
    AfterForward
  };

  /**
   * An API to CallAlgorithm that allows you to pass in the info objects to
   * be used
   */
  virtual int CallAlgorithm(svtkInformation* request, int direction, svtkInformationVector** inInfo,
    svtkInformationVector* outInfo);

protected:
  svtkExecutive();
  ~svtkExecutive() override;

  // Helper methods for subclasses.
  int InputPortIndexInRange(int port, const char* action);
  int OutputPortIndexInRange(int port, const char* action);

  // Called by methods to check for a recursive pipeline update.  A
  // request should be fulfilled without making another request.  This
  // is used to help enforce that behavior.  Returns 1 if no recursive
  // request is occurring, and 0 otherwise.  An error message is
  // produced automatically if 0 is returned.  The first argument is
  // the name of the calling method (the one that should not be
  // invoked recursively during an update).  The second argument is
  // the recursive request information object, if any.  It is used to
  // construct the error message.
  int CheckAlgorithm(const char* method, svtkInformation* request);

  virtual int ForwardDownstream(svtkInformation* request);
  virtual int ForwardUpstream(svtkInformation* request);
  virtual void CopyDefaultInformation(svtkInformation* request, int direction,
    svtkInformationVector** inInfo, svtkInformationVector* outInfo);

  // Reset the pipeline update values in the given output information object.
  virtual void ResetPipelineInformation(int port, svtkInformation*) = 0;

  // Bring the existence of output data objects up to date.
  virtual int UpdateDataObject() = 0;

  // Garbage collection support.
  void ReportReferences(svtkGarbageCollector*) override;

  virtual void SetAlgorithm(svtkAlgorithm* algorithm);

  // The algorithm managed by this executive.
  svtkAlgorithm* Algorithm;

  // Flag set when the algorithm is processing a request.
  int InAlgorithm;

  // Pointers to an outside instance of input or output information.
  // No references are held.  These are used to implement internal
  // pipelines.
  svtkInformationVector** SharedInputInformation;
  svtkInformationVector* SharedOutputInformation;

private:
  // Store an information object for each output port of the algorithm.
  svtkInformationVector* OutputInformation;

  // Internal implementation details.
  svtkExecutiveInternals* ExecutiveInternal;

  friend class svtkAlgorithmToExecutiveFriendship;

private:
  svtkExecutive(const svtkExecutive&) = delete;
  void operator=(const svtkExecutive&) = delete;
};

#endif
