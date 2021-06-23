/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCommunicator
 * @brief   Used to send/receive messages in a multiprocess environment.
 *
 * This is an abstract class which contains functionality for sending
 * and receiving inter-process messages. It contains methods for marshaling
 * an object into a string (currently used by the MPI communicator but
 * not the shared memory communicator).
 *
 * @warning
 * Communication between systems with different svtkIdTypes is not
 * supported. All machines have to have the same svtkIdType.
 *
 * @sa
 * svtkMPICommunicator
 */

#ifndef svtkCommunicator_h
#define svtkCommunicator_h

#include "svtkObject.h"
#include "svtkParallelCoreModule.h" // For export macro
#include "svtkSmartPointer.h"       // needed for svtkSmartPointer.
#include <vector>                  // needed for std::vector

class svtkBoundingBox;
class svtkCharArray;
class svtkDataArray;
class svtkDataObject;
class svtkDataSet;
class svtkIdTypeArray;
class svtkImageData;
class svtkMultiBlockDataSet;
class svtkMultiProcessStream;

class SVTKPARALLELCORE_EXPORT svtkCommunicator : public svtkObject
{

public:
  svtkTypeMacro(svtkCommunicator, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the number of processes you will be using.  This defaults
   * to the maximum number available.  If you set this to a value
   * higher than the default, you will get an error.
   */
  virtual void SetNumberOfProcesses(int num);
  svtkGetMacro(NumberOfProcesses, int);
  //@}

  //@{
  /**
   * Tells you which process [0, NumProcess) you are in.
   */
  svtkGetMacro(LocalProcessId, int);
  //@}

  enum Tags
  {
    BROADCAST_TAG = 10,
    GATHER_TAG = 11,
    GATHERV_TAG = 12,
    SCATTER_TAG = 13,
    SCATTERV_TAG = 14,
    REDUCE_TAG = 15,
    BARRIER_TAG = 16
  };

  enum StandardOperations
  {
    MAX_OP,
    MIN_OP,
    SUM_OP,
    PRODUCT_OP,
    LOGICAL_AND_OP,
    BITWISE_AND_OP,
    LOGICAL_OR_OP,
    BITWISE_OR_OP,
    LOGICAL_XOR_OP,
    BITWISE_XOR_OP
  };

  /**
   * A custom operation to use in a reduce command.  Subclass this object to
   * provide your own operations.
   */
  class Operation
  {
  public:
    /**
     * Subclasses must overload this method, which performs the actual
     * operations.  The methods should first do a reinterpret cast of the arrays
     * to the type suggested by \c datatype (which will be one of the SVTK type
     * identifiers like SVTK_INT, etc.).  Both arrays are considered top be
     * length entries.  The method should perform the operation A*B (where * is
     * a placeholder for whatever operation is actually performed) and store the
     * result in B.  The operation is assumed to be associative.  Commutativity
     * is specified by the Commutative method.
     */
    virtual void Function(const void* A, void* B, svtkIdType length, int datatype) = 0;

    /**
     * Subclasses override this method to specify whether their operation
     * is commutative.  It should return 1 if commutative or 0 if not.
     */
    virtual int Commutative() = 0;

    virtual ~Operation() {}
  };

  /**
   * This method sends a data object to a destination.
   * Tag eliminates ambiguity
   * and is used to match sends to receives.
   */
  int Send(svtkDataObject* data, int remoteHandle, int tag);

  /**
   * This method sends a data array to a destination.
   * Tag eliminates ambiguity
   * and is used to match sends to receives.
   */
  int Send(svtkDataArray* data, int remoteHandle, int tag);

  /**
   * Subclasses have to supply this method to send various arrays of data.
   * The \c type arg is one of the SVTK type constants recognized by the
   * svtkTemplateMacro (SVTK_FLOAT, SVTK_INT, etc.).  \c length is measured
   * in number of values (as opposed to number of bytes).
   */
  virtual int SendVoidArray(
    const void* data, svtkIdType length, int type, int remoteHandle, int tag) = 0;

  //@{
  /**
   * Convenience methods for sending data arrays.
   */
  int Send(const int* data, svtkIdType length, int remoteHandle, int tag)
  {
    return this->SendVoidArray(data, length, SVTK_INT, remoteHandle, tag);
  }
  int Send(const unsigned int* data, svtkIdType length, int remoteHandle, int tag)
  {
    return this->SendVoidArray(data, length, SVTK_INT, remoteHandle, tag);
  }
  int Send(const short* data, svtkIdType length, int remoteHandle, int tag)
  {
    return this->SendVoidArray(data, length, SVTK_SHORT, remoteHandle, tag);
  }
  int Send(const unsigned short* data, svtkIdType length, int remoteHandle, int tag)
  {
    return this->SendVoidArray(data, length, SVTK_UNSIGNED_SHORT, remoteHandle, tag);
  }
  int Send(const long* data, svtkIdType length, int remoteHandle, int tag)
  {
    return this->SendVoidArray(data, length, SVTK_LONG, remoteHandle, tag);
  }
  int Send(const unsigned long* data, svtkIdType length, int remoteHandle, int tag)
  {
    return this->SendVoidArray(data, length, SVTK_UNSIGNED_LONG, remoteHandle, tag);
  }
  int Send(const unsigned char* data, svtkIdType length, int remoteHandle, int tag)
  {
    return this->SendVoidArray(data, length, SVTK_UNSIGNED_CHAR, remoteHandle, tag);
  }
  int Send(const char* data, svtkIdType length, int remoteHandle, int tag)
  {
    return this->SendVoidArray(data, length, SVTK_CHAR, remoteHandle, tag);
  }
  int Send(const signed char* data, svtkIdType length, int remoteHandle, int tag)
  {
    return this->SendVoidArray(data, length, SVTK_SIGNED_CHAR, remoteHandle, tag);
  }
  int Send(const float* data, svtkIdType length, int remoteHandle, int tag)
  {
    return this->SendVoidArray(data, length, SVTK_FLOAT, remoteHandle, tag);
  }
  int Send(const double* data, svtkIdType length, int remoteHandle, int tag)
  {
    return this->SendVoidArray(data, length, SVTK_DOUBLE, remoteHandle, tag);
  }
  int Send(const long long* data, svtkIdType length, int remoteHandle, int tag)
  {
    return this->SendVoidArray(data, length, SVTK_LONG_LONG, remoteHandle, tag);
  }
  int Send(const unsigned long long* data, svtkIdType length, int remoteHandle, int tag)
  {
    return this->SendVoidArray(data, length, SVTK_UNSIGNED_LONG_LONG, remoteHandle, tag);
  }
  //@}

  int Send(const svtkMultiProcessStream& stream, int remoteId, int tag);

  /**
   * This method receives a data object from a corresponding send. It blocks
   * until the receive is finished.
   */
  int Receive(svtkDataObject* data, int remoteHandle, int tag);

  /**
   * The caller does not have to know the data type before this call is made.
   * It returns the newly created object.
   */
  svtkDataObject* ReceiveDataObject(int remoteHandle, int tag);

  /**
   * This method receives a data array from a corresponding send. It blocks
   * until the receive is finished.
   */
  int Receive(svtkDataArray* data, int remoteHandle, int tag);

  /**
   * Subclasses have to supply this method to receive various arrays of data.
   * The \c type arg is one of the SVTK type constants recognized by the
   * svtkTemplateMacro (SVTK_FLOAT, SVTK_INT, etc.).  \c maxlength is measured
   * in number of values (as opposed to number of bytes) and is the maxmum
   * length of the data to receive.  If the maxlength is less than the length
   * of the message sent by the sender, an error will be flagged. Once a
   * message is received, use the GetCount() method to determine the actual
   * size of the data received.
   */
  virtual int ReceiveVoidArray(
    void* data, svtkIdType maxlength, int type, int remoteHandle, int tag) = 0;

  //@{
  /**
   * Convenience methods for receiving data arrays.
   */
  int Receive(int* data, svtkIdType maxlength, int remoteHandle, int tag)
  {
    return this->ReceiveVoidArray(data, maxlength, SVTK_INT, remoteHandle, tag);
  }
  int Receive(unsigned int* data, svtkIdType maxlength, int remoteHandle, int tag)
  {
    return this->ReceiveVoidArray(data, maxlength, SVTK_INT, remoteHandle, tag);
  }
  int Receive(short* data, svtkIdType maxlength, int remoteHandle, int tag)
  {
    return this->ReceiveVoidArray(data, maxlength, SVTK_SHORT, remoteHandle, tag);
  }
  int Receive(unsigned short* data, svtkIdType maxlength, int remoteHandle, int tag)
  {
    return this->ReceiveVoidArray(data, maxlength, SVTK_UNSIGNED_SHORT, remoteHandle, tag);
  }
  int Receive(long* data, svtkIdType maxlength, int remoteHandle, int tag)
  {
    return this->ReceiveVoidArray(data, maxlength, SVTK_LONG, remoteHandle, tag);
  }
  int Receive(unsigned long* data, svtkIdType maxlength, int remoteHandle, int tag)
  {
    return this->ReceiveVoidArray(data, maxlength, SVTK_UNSIGNED_LONG, remoteHandle, tag);
  }
  int Receive(unsigned char* data, svtkIdType maxlength, int remoteHandle, int tag)
  {
    return this->ReceiveVoidArray(data, maxlength, SVTK_UNSIGNED_CHAR, remoteHandle, tag);
  }
  int Receive(char* data, svtkIdType maxlength, int remoteHandle, int tag)
  {
    return this->ReceiveVoidArray(data, maxlength, SVTK_CHAR, remoteHandle, tag);
  }
  int Receive(signed char* data, svtkIdType maxlength, int remoteHandle, int tag)
  {
    return this->ReceiveVoidArray(data, maxlength, SVTK_SIGNED_CHAR, remoteHandle, tag);
  }
  int Receive(float* data, svtkIdType maxlength, int remoteHandle, int tag)
  {
    return this->ReceiveVoidArray(data, maxlength, SVTK_FLOAT, remoteHandle, tag);
  }
  int Receive(double* data, svtkIdType maxlength, int remoteHandle, int tag)
  {
    return this->ReceiveVoidArray(data, maxlength, SVTK_DOUBLE, remoteHandle, tag);
  }
  int Receive(long long* data, svtkIdType maxlength, int remoteHandle, int tag)
  {
    return this->ReceiveVoidArray(data, maxlength, SVTK_LONG_LONG, remoteHandle, tag);
  }
  int Receive(unsigned long long* data, svtkIdType maxlength, int remoteHandle, int tag)
  {
    return this->ReceiveVoidArray(data, maxlength, SVTK_UNSIGNED_LONG_LONG, remoteHandle, tag);
  }
  //@}

  int Receive(svtkMultiProcessStream& stream, int remoteId, int tag);

  //@{
  /**
   * Returns the number of words received by the most recent Receive().
   * Note that this is not the number of bytes received, but the number of items
   * of the data-type received by the most recent Receive() eg. if
   * Receive(int*,..) was used, then this returns the number of ints received;
   * if Receive(double*,..) was used, then this returns the number of doubles
   * received etc. The return value is valid only after a successful Receive().
   */
  svtkGetMacro(Count, svtkIdType);
  //@}

  //---------------------- Collective Operations ----------------------

  /**
   * Will block the processes until all other processes reach the Barrier
   * function.
   */
  virtual void Barrier();

  //@{
  /**
   * Broadcast sends the array in the process with id \c srcProcessId to all of
   * the other processes.  All processes must call these method with the same
   * arguments in order for it to complete.
   */
  int Broadcast(int* data, svtkIdType length, int srcProcessId)
  {
    return this->BroadcastVoidArray(data, length, SVTK_INT, srcProcessId);
  }
  int Broadcast(unsigned int* data, svtkIdType length, int srcProcessId)
  {
    return this->BroadcastVoidArray(data, length, SVTK_UNSIGNED_INT, srcProcessId);
  }
  int Broadcast(short* data, svtkIdType length, int srcProcessId)
  {
    return this->BroadcastVoidArray(data, length, SVTK_SHORT, srcProcessId);
  }
  int Broadcast(unsigned short* data, svtkIdType length, int srcProcessId)
  {
    return this->BroadcastVoidArray(data, length, SVTK_UNSIGNED_SHORT, srcProcessId);
  }
  int Broadcast(long* data, svtkIdType length, int srcProcessId)
  {
    return this->BroadcastVoidArray(data, length, SVTK_LONG, srcProcessId);
  }
  int Broadcast(unsigned long* data, svtkIdType length, int srcProcessId)
  {
    return this->BroadcastVoidArray(data, length, SVTK_UNSIGNED_LONG, srcProcessId);
  }
  int Broadcast(unsigned char* data, svtkIdType length, int srcProcessId)
  {
    return this->BroadcastVoidArray(data, length, SVTK_UNSIGNED_CHAR, srcProcessId);
  }
  int Broadcast(char* data, svtkIdType length, int srcProcessId)
  {
    return this->BroadcastVoidArray(data, length, SVTK_CHAR, srcProcessId);
  }
  int Broadcast(signed char* data, svtkIdType length, int srcProcessId)
  {
    return this->BroadcastVoidArray(data, length, SVTK_SIGNED_CHAR, srcProcessId);
  }
  int Broadcast(float* data, svtkIdType length, int srcProcessId)
  {
    return this->BroadcastVoidArray(data, length, SVTK_FLOAT, srcProcessId);
  }
  int Broadcast(double* data, svtkIdType length, int srcProcessId)
  {
    return this->BroadcastVoidArray(data, length, SVTK_DOUBLE, srcProcessId);
  }
  int Broadcast(long long* data, svtkIdType length, int srcProcessId)
  {
    return this->BroadcastVoidArray(data, length, SVTK_LONG_LONG, srcProcessId);
  }
  int Broadcast(unsigned long long* data, svtkIdType length, int srcProcessId)
  {
    return this->BroadcastVoidArray(data, length, SVTK_UNSIGNED_LONG_LONG, srcProcessId);
  }
  int Broadcast(svtkDataObject* data, int srcProcessId);
  int Broadcast(svtkDataArray* data, int srcProcessId);
  //@}

  int Broadcast(svtkMultiProcessStream& stream, int srcProcessId);

  //@{
  /**
   * Gather collects arrays in the process with id \c destProcessId.  Each
   * process (including the destination) sends the contents of its send buffer
   * to the destination process.  The destination process receives the
   * messages and stores them in rank order.  The \c length argument
   * (which must be the same on all processes) is the length of the
   * sendBuffers.  The \c recvBuffer (on the destination process) must be of
   * length length*numProcesses.  Gather is the inverse operation of Scatter.
   */
  int Gather(const int* sendBuffer, int* recvBuffer, svtkIdType length, int destProcessId)
  {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length, SVTK_INT, destProcessId);
  }
  int Gather(
    const unsigned int* sendBuffer, unsigned int* recvBuffer, svtkIdType length, int destProcessId)
  {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_INT, destProcessId);
  }
  int Gather(const short* sendBuffer, short* recvBuffer, svtkIdType length, int destProcessId)
  {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length, SVTK_SHORT, destProcessId);
  }
  int Gather(const unsigned short* sendBuffer, unsigned short* recvBuffer, svtkIdType length,
    int destProcessId)
  {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_SHORT, destProcessId);
  }
  int Gather(const long* sendBuffer, long* recvBuffer, svtkIdType length, int destProcessId)
  {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length, SVTK_LONG, destProcessId);
  }
  int Gather(
    const unsigned long* sendBuffer, unsigned long* recvBuffer, svtkIdType length, int destProcessId)
  {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG, destProcessId);
  }
  int Gather(
    const unsigned char* sendBuffer, unsigned char* recvBuffer, svtkIdType length, int destProcessId)
  {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_CHAR, destProcessId);
  }
  int Gather(const char* sendBuffer, char* recvBuffer, svtkIdType length, int destProcessId)
  {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length, SVTK_CHAR, destProcessId);
  }
  int Gather(
    const signed char* sendBuffer, signed char* recvBuffer, svtkIdType length, int destProcessId)
  {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length, SVTK_SIGNED_CHAR, destProcessId);
  }
  int Gather(const float* sendBuffer, float* recvBuffer, svtkIdType length, int destProcessId)
  {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length, SVTK_FLOAT, destProcessId);
  }
  int Gather(const double* sendBuffer, double* recvBuffer, svtkIdType length, int destProcessId)
  {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length, SVTK_DOUBLE, destProcessId);
  }
  int Gather(
    const long long* sendBuffer, long long* recvBuffer, svtkIdType length, int destProcessId)
  {
    return this->GatherVoidArray(sendBuffer, recvBuffer, length, SVTK_LONG_LONG, destProcessId);
  }
  int Gather(const unsigned long long* sendBuffer, unsigned long long* recvBuffer, svtkIdType length,
    int destProcessId)
  {
    return this->GatherVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG_LONG, destProcessId);
  }
  int Gather(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, int destProcessId);
  //@}

  /**
   * Gathers svtkDataObject (\c sendBuffer) from all ranks to the \c destProcessId.
   * @param[in] sendBuffer - data object to send from local process. Can be null if
   * not sending any data from the current process.
   * @param[out] recvBuffer - vector of data objects to receive data on the receiving
   * rank (identified by \c destProcessId). This may be
   * empty or filled with data object instances. If empty,
   * data objects will be created as needed. If not empty,
   * existing data object will be used.
   * @param[in] destProcessId - process id to gather on.
   * @return - 1 on success, 0 on failure.
   */
  int Gather(svtkDataObject* sendBuffer, std::vector<svtkSmartPointer<svtkDataObject> >& recvBuffer,
    int destProcessId);

  /**
   * Gathers svtkMultiProcessStream (\c sendBuffer) from all ranks to the \c
   * destProcessId.
   * @param[in]  sendBuffer - svtkMultiProcessStream to send from local process.
   * @param[out] recvBuffer - vector of svtkMultiProcessStream instances recevied
   *             on the receiving rank (identified by \c destProcessId).
   * @param[in]  destProcessId - process id to gather on.
   * @return     1 on success, 0 on failure.
   */
  int Gather(const svtkMultiProcessStream& sendBuffer,
    std::vector<svtkMultiProcessStream>& recvBuffer, int destProcessId);

  //@{
  /**
   * GatherV is the vector variant of Gather.  It extends the functionality of
   * Gather by allowing a varying count of data from each process.
   * GatherV collects arrays in the process with id \c destProcessId.  Each
   * process (including the destination) sends the contents of its send buffer
   * to the destination process.  The destination process receives the
   * messages and stores them in rank order.  The \c sendLength argument
   * defines how much the local process sends to \c destProcessId and
   * \c recvLengths is an array containing the amount \c destProcessId
   * receives from each process, in rank order.
   */
  int GatherV(const int* sendBuffer, int* recvBuffer, svtkIdType sendLength, svtkIdType* recvLengths,
    svtkIdType* offsets, int destProcessId)
  {
    return this->GatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_INT, destProcessId);
  }
  int GatherV(const unsigned int* sendBuffer, unsigned int* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int destProcessId)
  {
    return this->GatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_UNSIGNED_INT, destProcessId);
  }
  int GatherV(const short* sendBuffer, short* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int destProcessId)
  {
    return this->GatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_SHORT, destProcessId);
  }
  int GatherV(const unsigned short* sendBuffer, unsigned short* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int destProcessId)
  {
    return this->GatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_UNSIGNED_SHORT, destProcessId);
  }
  int GatherV(const long* sendBuffer, long* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int destProcessId)
  {
    return this->GatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_LONG, destProcessId);
  }
  int GatherV(const unsigned long* sendBuffer, unsigned long* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int destProcessId)
  {
    return this->GatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_UNSIGNED_LONG, destProcessId);
  }
  int GatherV(const unsigned char* sendBuffer, unsigned char* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int destProcessId)
  {
    return this->GatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_UNSIGNED_CHAR, destProcessId);
  }
  int GatherV(const char* sendBuffer, char* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int destProcessId)
  {
    return this->GatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_CHAR, destProcessId);
  }
  int GatherV(const signed char* sendBuffer, signed char* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int destProcessId)
  {
    return this->GatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_SIGNED_CHAR, destProcessId);
  }
  int GatherV(const float* sendBuffer, float* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int destProcessId)
  {
    return this->GatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_FLOAT, destProcessId);
  }
  int GatherV(const double* sendBuffer, double* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int destProcessId)
  {
    return this->GatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_DOUBLE, destProcessId);
  }
  int GatherV(const long long* sendBuffer, long long* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int destProcessId)
  {
    return this->GatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_LONG_LONG, destProcessId);
  }
  int GatherV(const unsigned long long* sendBuffer, unsigned long long* recvBuffer,
    svtkIdType sendLength, svtkIdType* recvLengths, svtkIdType* offsets, int destProcessId)
  {
    return this->GatherVVoidArray(sendBuffer, recvBuffer, sendLength, recvLengths, offsets,
      SVTK_UNSIGNED_LONG_LONG, destProcessId);
  }
  //@}
  //@{
  /**
   * For the first GatherV variant, \c recvLengths and \c offsets known on
   * \c destProcessId and are passed in as parameters
   * For the second GatherV variant, \c recvLengths and \c offsets are not known
   * on \c destProcessId.  The \c recvLengths is set using a gather operation
   * and \c offsets is computed from \c recvLengths. recvLengths has
   * \c NumberOfProcesses elements and \offsets has NumberOfProcesses + 1 elements.
   * The third variant is the same as the second variant but it does not expose
   * \c recvLength and \c offsets
   */
  int GatherV(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, svtkIdType* recvLengths,
    svtkIdType* offsets, int destProcessId);
  int GatherV(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, svtkIdTypeArray* recvLengths,
    svtkIdTypeArray* offsets, int destProcessId);
  int GatherV(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, int destProcessId);
  //@}
  /**
   * Collects data objects in the process with id \c
   * destProcessId.  Each process (including the destination) marshals
   * and then sends the data object to the destination process.  The
   * destination process unmarshals and then stores the data objects
   * in rank order. The \c recvData (on the destination process) must
   * be of length numProcesses.
   */
  int GatherV(svtkDataObject* sendData, svtkSmartPointer<svtkDataObject>* recvData, int destProcessId);

  //@{
  /**
   * Scatter takes an array in the process with id \c srcProcessId and
   * distributes it.  Each process (including the source) receives a portion of
   * the send buffer.  Process 0 receives the first \c length values, process 1
   * receives the second \c length values, and so on.  Scatter is the inverse
   * operation of Gather.
   */
  int Scatter(const int* sendBuffer, int* recvBuffer, svtkIdType length, int srcProcessId)
  {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length, SVTK_INT, srcProcessId);
  }
  int Scatter(
    const unsigned int* sendBuffer, unsigned int* recvBuffer, svtkIdType length, int srcProcessId)
  {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_INT, srcProcessId);
  }
  int Scatter(const short* sendBuffer, short* recvBuffer, svtkIdType length, int srcProcessId)
  {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length, SVTK_SHORT, srcProcessId);
  }
  int Scatter(const unsigned short* sendBuffer, unsigned short* recvBuffer, svtkIdType length,
    int srcProcessId)
  {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_SHORT, srcProcessId);
  }
  int Scatter(const long* sendBuffer, long* recvBuffer, svtkIdType length, int srcProcessId)
  {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length, SVTK_INT, srcProcessId);
  }
  int Scatter(
    const unsigned long* sendBuffer, unsigned long* recvBuffer, svtkIdType length, int srcProcessId)
  {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG, srcProcessId);
  }
  int Scatter(
    const unsigned char* sendBuffer, unsigned char* recvBuffer, svtkIdType length, int srcProcessId)
  {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_CHAR, srcProcessId);
  }
  int Scatter(const char* sendBuffer, char* recvBuffer, svtkIdType length, int srcProcessId)
  {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length, SVTK_CHAR, srcProcessId);
  }
  int Scatter(
    const signed char* sendBuffer, signed char* recvBuffer, svtkIdType length, int srcProcessId)
  {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length, SVTK_SIGNED_CHAR, srcProcessId);
  }
  int Scatter(const float* sendBuffer, float* recvBuffer, svtkIdType length, int srcProcessId)
  {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length, SVTK_FLOAT, srcProcessId);
  }
  int Scatter(const double* sendBuffer, double* recvBuffer, svtkIdType length, int srcProcessId)
  {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length, SVTK_DOUBLE, srcProcessId);
  }
  int Scatter(
    const long long* sendBuffer, long long* recvBuffer, svtkIdType length, int srcProcessId)
  {
    return this->ScatterVoidArray(sendBuffer, recvBuffer, length, SVTK_LONG_LONG, srcProcessId);
  }
  int Scatter(const unsigned long long* sendBuffer, unsigned long long* recvBuffer,
    svtkIdType length, int srcProcessId)
  {
    return this->ScatterVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG_LONG, srcProcessId);
  }
  int Scatter(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, int srcProcessId);
  //@}

  //@{
  /**
   * ScatterV is the vector variant of Scatter.  It extends the functionality of
   * Scatter by allowing a varying count of data to each process.
   * ScatterV takes an array in the process with id \c srcProcessId and
   * distributes it.  Each process (including the source) receives a portion of
   * the send buffer defined by the \c sendLengths and \c offsets arrays.
   */
  int ScatterV(const int* sendBuffer, int* recvBuffer, svtkIdType* sendLengths, svtkIdType* offsets,
    svtkIdType recvLength, int srcProcessId)
  {
    return this->ScatterVVoidArray(
      sendBuffer, recvBuffer, sendLengths, offsets, recvLength, SVTK_INT, srcProcessId);
  }
  int ScatterV(const unsigned int* sendBuffer, unsigned int* recvBuffer, svtkIdType* sendLengths,
    svtkIdType* offsets, svtkIdType recvLength, int srcProcessId)
  {
    return this->ScatterVVoidArray(
      sendBuffer, recvBuffer, sendLengths, offsets, recvLength, SVTK_UNSIGNED_INT, srcProcessId);
  }
  int ScatterV(const short* sendBuffer, short* recvBuffer, svtkIdType* sendLengths,
    svtkIdType* offsets, svtkIdType recvLength, int srcProcessId)
  {
    return this->ScatterVVoidArray(
      sendBuffer, recvBuffer, sendLengths, offsets, recvLength, SVTK_SHORT, srcProcessId);
  }
  int ScatterV(const unsigned short* sendBuffer, unsigned short* recvBuffer, svtkIdType* sendLengths,
    svtkIdType* offsets, svtkIdType recvLength, int srcProcessId)
  {
    return this->ScatterVVoidArray(
      sendBuffer, recvBuffer, sendLengths, offsets, recvLength, SVTK_UNSIGNED_SHORT, srcProcessId);
  }
  int ScatterV(const long* sendBuffer, long* recvBuffer, svtkIdType* sendLengths, svtkIdType* offsets,
    svtkIdType recvLength, int srcProcessId)
  {
    return this->ScatterVVoidArray(
      sendBuffer, recvBuffer, sendLengths, offsets, recvLength, SVTK_LONG, srcProcessId);
  }
  int ScatterV(const unsigned long* sendBuffer, unsigned long* recvBuffer, svtkIdType* sendLengths,
    svtkIdType* offsets, svtkIdType recvLength, int srcProcessId)
  {
    return this->ScatterVVoidArray(
      sendBuffer, recvBuffer, sendLengths, offsets, recvLength, SVTK_UNSIGNED_LONG, srcProcessId);
  }
  int ScatterV(const unsigned char* sendBuffer, unsigned char* recvBuffer, svtkIdType* sendLengths,
    svtkIdType* offsets, svtkIdType recvLength, int srcProcessId)
  {
    return this->ScatterVVoidArray(
      sendBuffer, recvBuffer, sendLengths, offsets, recvLength, SVTK_UNSIGNED_CHAR, srcProcessId);
  }
  int ScatterV(const char* sendBuffer, char* recvBuffer, svtkIdType* sendLengths, svtkIdType* offsets,
    svtkIdType recvLength, int srcProcessId)
  {
    return this->ScatterVVoidArray(
      sendBuffer, recvBuffer, sendLengths, offsets, recvLength, SVTK_CHAR, srcProcessId);
  }
  int ScatterV(const signed char* sendBuffer, signed char* recvBuffer, svtkIdType* sendLengths,
    svtkIdType* offsets, svtkIdType recvLength, int srcProcessId)
  {
    return this->ScatterVVoidArray(
      sendBuffer, recvBuffer, sendLengths, offsets, recvLength, SVTK_SIGNED_CHAR, srcProcessId);
  }
  int ScatterV(const float* sendBuffer, float* recvBuffer, svtkIdType* sendLengths,
    svtkIdType* offsets, svtkIdType recvLength, int srcProcessId)
  {
    return this->ScatterVVoidArray(
      sendBuffer, recvBuffer, sendLengths, offsets, recvLength, SVTK_FLOAT, srcProcessId);
  }
  int ScatterV(const double* sendBuffer, double* recvBuffer, svtkIdType* sendLengths,
    svtkIdType* offsets, svtkIdType recvLength, int srcProcessId)
  {
    return this->ScatterVVoidArray(
      sendBuffer, recvBuffer, sendLengths, offsets, recvLength, SVTK_DOUBLE, srcProcessId);
  }
  int ScatterV(const long long* sendBuffer, long long* recvBuffer, svtkIdType* sendLengths,
    svtkIdType* offsets, svtkIdType recvLength, int srcProcessId)
  {
    return this->ScatterVVoidArray(
      sendBuffer, recvBuffer, sendLengths, offsets, recvLength, SVTK_LONG_LONG, srcProcessId);
  }
  int ScatterV(const unsigned long long* sendBuffer, unsigned long long* recvBuffer,
    svtkIdType* sendLengths, svtkIdType* offsets, svtkIdType recvLength, int srcProcessId)
  {
    return this->ScatterVVoidArray(sendBuffer, recvBuffer, sendLengths, offsets, recvLength,
      SVTK_UNSIGNED_LONG_LONG, srcProcessId);
  }
  //@}

  //@{
  /**
   * Same as gather except that the result ends up on all processes.
   */
  int AllGather(const int* sendBuffer, int* recvBuffer, svtkIdType length)
  {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, SVTK_INT);
  }
  int AllGather(const unsigned int* sendBuffer, unsigned int* recvBuffer, svtkIdType length)
  {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_INT);
  }
  int AllGather(const short* sendBuffer, short* recvBuffer, svtkIdType length)
  {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, SVTK_SHORT);
  }
  int AllGather(const unsigned short* sendBuffer, unsigned short* recvBuffer, svtkIdType length)
  {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_SHORT);
  }
  int AllGather(const long* sendBuffer, long* recvBuffer, svtkIdType length)
  {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, SVTK_LONG);
  }
  int AllGather(const unsigned long* sendBuffer, unsigned long* recvBuffer, svtkIdType length)
  {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG);
  }
  int AllGather(const unsigned char* sendBuffer, unsigned char* recvBuffer, svtkIdType length)
  {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_CHAR);
  }
  int AllGather(const char* sendBuffer, char* recvBuffer, svtkIdType length)
  {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, SVTK_CHAR);
  }
  int AllGather(const signed char* sendBuffer, signed char* recvBuffer, svtkIdType length)
  {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, SVTK_SIGNED_CHAR);
  }
  int AllGather(const float* sendBuffer, float* recvBuffer, svtkIdType length)
  {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, SVTK_FLOAT);
  }
  int AllGather(const double* sendBuffer, double* recvBuffer, svtkIdType length)
  {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, SVTK_DOUBLE);
  }
  int AllGather(const long long* sendBuffer, long long* recvBuffer, svtkIdType length)
  {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, SVTK_LONG_LONG);
  }
  int AllGather(
    const unsigned long long* sendBuffer, unsigned long long* recvBuffer, svtkIdType length)
  {
    return this->AllGatherVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG_LONG);
  }
  int AllGather(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer);
  //@}

  //@{
  /**
   * Same as GatherV except that the result is placed in all processes.
   */
  int AllGatherV(const int* sendBuffer, int* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets)
  {
    return this->AllGatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_INT);
  }
  int AllGatherV(const unsigned int* sendBuffer, unsigned int* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets)
  {
    return this->AllGatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_UNSIGNED_INT);
  }
  int AllGatherV(const short* sendBuffer, short* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets)
  {
    return this->AllGatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_SHORT);
  }
  int AllGatherV(const unsigned short* sendBuffer, unsigned short* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets)
  {
    return this->AllGatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_UNSIGNED_SHORT);
  }
  int AllGatherV(const long* sendBuffer, long* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets)
  {
    return this->AllGatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_LONG);
  }
  int AllGatherV(const unsigned long* sendBuffer, unsigned long* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets)
  {
    return this->AllGatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_UNSIGNED_LONG);
  }
  int AllGatherV(const unsigned char* sendBuffer, unsigned char* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets)
  {
    return this->AllGatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_UNSIGNED_CHAR);
  }
  int AllGatherV(const char* sendBuffer, char* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets)
  {
    return this->AllGatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_CHAR);
  }
  int AllGatherV(const signed char* sendBuffer, signed char* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets)
  {
    return this->AllGatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_UNSIGNED_CHAR);
  }
  int AllGatherV(const float* sendBuffer, float* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets)
  {
    return this->AllGatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_FLOAT);
  }
  int AllGatherV(const double* sendBuffer, double* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets)
  {
    return this->AllGatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_DOUBLE);
  }
  int AllGatherV(const long long* sendBuffer, long long* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets)
  {
    return this->AllGatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_LONG_LONG);
  }
  int AllGatherV(const unsigned long long* sendBuffer, unsigned long long* recvBuffer,
    svtkIdType sendLength, svtkIdType* recvLengths, svtkIdType* offsets)
  {
    return this->AllGatherVVoidArray(
      sendBuffer, recvBuffer, sendLength, recvLengths, offsets, SVTK_UNSIGNED_LONG_LONG);
  }
  int AllGatherV(
    svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, svtkIdType* recvLengths, svtkIdType* offsets);
  int AllGatherV(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer);
  //@}

  //@{
  /**
   * Reduce an array to the given destination process.  This version of Reduce
   * takes an identifier defined in the
   * svtkCommunicator::StandardOperations enum to define the operation.
   */
  int Reduce(
    const int* sendBuffer, int* recvBuffer, svtkIdType length, int operation, int destProcessId)
  {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_INT, operation, destProcessId);
  }
  int Reduce(const unsigned int* sendBuffer, unsigned int* recvBuffer, svtkIdType length,
    int operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_INT, operation, destProcessId);
  }
  int Reduce(
    const short* sendBuffer, short* recvBuffer, svtkIdType length, int operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_SHORT, operation, destProcessId);
  }
  int Reduce(const unsigned short* sendBuffer, unsigned short* recvBuffer, svtkIdType length,
    int operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_SHORT, operation, destProcessId);
  }
  int Reduce(
    const long* sendBuffer, long* recvBuffer, svtkIdType length, int operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_LONG, operation, destProcessId);
  }
  int Reduce(const unsigned long* sendBuffer, unsigned long* recvBuffer, svtkIdType length,
    int operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG, operation, destProcessId);
  }
  int Reduce(const unsigned char* sendBuffer, unsigned char* recvBuffer, svtkIdType length,
    int operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_CHAR, operation, destProcessId);
  }
  int Reduce(
    const char* sendBuffer, char* recvBuffer, svtkIdType length, int operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_CHAR, operation, destProcessId);
  }
  int Reduce(const signed char* sendBuffer, signed char* recvBuffer, svtkIdType length,
    int operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_SIGNED_CHAR, operation, destProcessId);
  }
  int Reduce(
    const float* sendBuffer, float* recvBuffer, svtkIdType length, int operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_FLOAT, operation, destProcessId);
  }
  int Reduce(const double* sendBuffer, double* recvBuffer, svtkIdType length, int operation,
    int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_DOUBLE, operation, destProcessId);
  }
  int Reduce(const long long* sendBuffer, long long* recvBuffer, svtkIdType length, int operation,
    int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_LONG_LONG, operation, destProcessId);
  }
  int Reduce(const unsigned long long* sendBuffer, unsigned long long* recvBuffer, svtkIdType length,
    int operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG_LONG, operation, destProcessId);
  }
  int Reduce(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, int operation, int destProcessId);
  //@}

  //@{
  /**
   * Reduce an array to the given destination process.  This version of Reduce
   * takes a custom operation as a subclass of svtkCommunicator::Operation.
   */
  int Reduce(const int* sendBuffer, int* recvBuffer, svtkIdType length, Operation* operation,
    int destProcessId)
  {
    return this->ReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_INT, operation, destProcessId);
  }
  int Reduce(const unsigned int* sendBuffer, unsigned int* recvBuffer, svtkIdType length,
    Operation* operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_INT, operation, destProcessId);
  }
  int Reduce(const short* sendBuffer, short* recvBuffer, svtkIdType length, Operation* operation,
    int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_SHORT, operation, destProcessId);
  }
  int Reduce(const unsigned short* sendBuffer, unsigned short* recvBuffer, svtkIdType length,
    Operation* operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_SHORT, operation, destProcessId);
  }
  int Reduce(const long* sendBuffer, long* recvBuffer, svtkIdType length, Operation* operation,
    int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_LONG, operation, destProcessId);
  }
  int Reduce(const unsigned long* sendBuffer, unsigned long* recvBuffer, svtkIdType length,
    Operation* operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG, operation, destProcessId);
  }
  int Reduce(const unsigned char* sendBuffer, unsigned char* recvBuffer, svtkIdType length,
    Operation* operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_CHAR, operation, destProcessId);
  }
  int Reduce(const char* sendBuffer, char* recvBuffer, svtkIdType length, Operation* operation,
    int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_CHAR, operation, destProcessId);
  }
  int Reduce(const signed char* sendBuffer, signed char* recvBuffer, svtkIdType length,
    Operation* operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_SIGNED_CHAR, operation, destProcessId);
  }
  int Reduce(const float* sendBuffer, float* recvBuffer, svtkIdType length, Operation* operation,
    int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_FLOAT, operation, destProcessId);
  }
  int Reduce(const double* sendBuffer, double* recvBuffer, svtkIdType length, Operation* operation,
    int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_DOUBLE, operation, destProcessId);
  }
  int Reduce(const long long* sendBuffer, long long* recvBuffer, svtkIdType length,
    Operation* operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_LONG_LONG, operation, destProcessId);
  }
  int Reduce(const unsigned long long* sendBuffer, unsigned long long* recvBuffer, svtkIdType length,
    Operation* operation, int destProcessId)
  {
    return this->ReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG_LONG, operation, destProcessId);
  }
  int Reduce(
    svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, Operation* operation, int destProcessId);
  //@}

  //@{
  /**
   * Same as Reduce except that the result is placed in all of the processes.
   */
  int AllReduce(const int* sendBuffer, int* recvBuffer, svtkIdType length, int operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_INT, operation);
  }
  int AllReduce(
    const unsigned int* sendBuffer, unsigned int* recvBuffer, svtkIdType length, int operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_INT, operation);
  }
  int AllReduce(const short* sendBuffer, short* recvBuffer, svtkIdType length, int operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_SHORT, operation);
  }
  int AllReduce(
    const unsigned short* sendBuffer, unsigned short* recvBuffer, svtkIdType length, int operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_SHORT, operation);
  }
  int AllReduce(const long* sendBuffer, long* recvBuffer, svtkIdType length, int operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_LONG, operation);
  }
  int AllReduce(
    const unsigned long* sendBuffer, unsigned long* recvBuffer, svtkIdType length, int operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG, operation);
  }
  int AllReduce(
    const unsigned char* sendBuffer, unsigned char* recvBuffer, svtkIdType length, int operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_CHAR, operation);
  }
  int AllReduce(const char* sendBuffer, char* recvBuffer, svtkIdType length, int operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_CHAR, operation);
  }
  int AllReduce(
    const signed char* sendBuffer, signed char* recvBuffer, svtkIdType length, int operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_SIGNED_CHAR, operation);
  }
  int AllReduce(const float* sendBuffer, float* recvBuffer, svtkIdType length, int operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_FLOAT, operation);
  }
  int AllReduce(const double* sendBuffer, double* recvBuffer, svtkIdType length, int operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_DOUBLE, operation);
  }
  int AllReduce(const long long* sendBuffer, long long* recvBuffer, svtkIdType length, int operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_LONG_LONG, operation);
  }
  int AllReduce(const unsigned long long* sendBuffer, unsigned long long* recvBuffer,
    svtkIdType length, int operation)
  {
    return this->AllReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG_LONG, operation);
  }
  int AllReduce(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, int operation);
  int AllReduce(const int* sendBuffer, int* recvBuffer, svtkIdType length, Operation* operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_INT, operation);
  }
  int AllReduce(const unsigned int* sendBuffer, unsigned int* recvBuffer, svtkIdType length,
    Operation* operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_INT, operation);
  }
  int AllReduce(const short* sendBuffer, short* recvBuffer, svtkIdType length, Operation* operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_SHORT, operation);
  }
  int AllReduce(const unsigned short* sendBuffer, unsigned short* recvBuffer, svtkIdType length,
    Operation* operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_SHORT, operation);
  }
  int AllReduce(const long* sendBuffer, long* recvBuffer, svtkIdType length, Operation* operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_LONG, operation);
  }
  int AllReduce(const unsigned long* sendBuffer, unsigned long* recvBuffer, svtkIdType length,
    Operation* operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG, operation);
  }
  int AllReduce(const unsigned char* sendBuffer, unsigned char* recvBuffer, svtkIdType length,
    Operation* operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_UNSIGNED_CHAR, operation);
  }
  int AllReduce(const char* sendBuffer, char* recvBuffer, svtkIdType length, Operation* operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_CHAR, operation);
  }
  int AllReduce(
    const signed char* sendBuffer, signed char* recvBuffer, svtkIdType length, Operation* operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_SIGNED_CHAR, operation);
  }
  int AllReduce(const float* sendBuffer, float* recvBuffer, svtkIdType length, Operation* operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_FLOAT, operation);
  }
  int AllReduce(
    const double* sendBuffer, double* recvBuffer, svtkIdType length, Operation* operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_DOUBLE, operation);
  }
  int AllReduce(
    const long long* sendBuffer, long long* recvBuffer, svtkIdType length, Operation* operation)
  {
    return this->AllReduceVoidArray(sendBuffer, recvBuffer, length, SVTK_LONG_LONG, operation);
  }
  int AllReduce(const unsigned long long* sendBuffer, unsigned long long* recvBuffer,
    svtkIdType length, Operation* operation)
  {
    return this->AllReduceVoidArray(
      sendBuffer, recvBuffer, length, SVTK_UNSIGNED_LONG_LONG, operation);
  }
  int AllReduce(svtkDataArray* sendBuffer, svtkDataArray* recvBuffer, Operation* operation);
  //@}

  //@{
  /**
   * Subclasses should reimplement these if they have a more efficient
   * implementation.
   */
  virtual int BroadcastVoidArray(void* data, svtkIdType length, int type, int srcProcessId);
  virtual int GatherVoidArray(
    const void* sendBuffer, void* recvBuffer, svtkIdType length, int type, int destProcessId);
  virtual int GatherVVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int type, int destProcessId);
  virtual int ScatterVoidArray(
    const void* sendBuffer, void* recvBuffer, svtkIdType length, int type, int srcProcessId);
  virtual int ScatterVVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType* sendLengths,
    svtkIdType* offsets, svtkIdType recvLength, int type, int srcProcessId);
  virtual int AllGatherVoidArray(
    const void* sendBuffer, void* recvBuffer, svtkIdType length, int type);
  virtual int AllGatherVVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int type);
  virtual int ReduceVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType length, int type,
    int operation, int destProcessId);
  virtual int ReduceVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType length, int type,
    Operation* operation, int destProcessId);
  virtual int AllReduceVoidArray(
    const void* sendBuffer, void* recvBuffer, svtkIdType length, int type, int operation);
  virtual int AllReduceVoidArray(
    const void* sendBuffer, void* recvBuffer, svtkIdType length, int type, Operation* operation);
  //@}

  static void SetUseCopy(int useCopy);

  /**
   * Determine the global bounds for a set of processes.  BBox is
   * initially set (outside of the call to the local bounds of the process
   * and will be modified to be the global bounds - this default implementation
   * views the processors as a heap tree with the root being processId = 0
   * If either rightHasBounds or leftHasBounds is not 0 then the
   * corresponding int will be set to 1 if the right/left processor has
   * bounds else it will be set to 0
   * The last three arguments are the tags to be used when performing
   * the operation
   */
  virtual int ComputeGlobalBounds(int processorId, int numProcesses, svtkBoundingBox* bounds,
    int* rightHasBounds = nullptr, int* leftHasBounds = nullptr, int hasBoundsTag = 288402,
    int localBoundsTag = 288403, int globalBoundsTag = 288404);

  //@{
  /**
   * Some helper functions when dealing with heap tree - based
   * algorithms - we don't need a function for getting the right
   * processor since it is 1 + theLeftProcessor
   */
  static int GetParentProcessor(int pid);
  static int GetLeftChildProcessor(int pid);
  //@}

  //@{
  /**
   * Convert a data object into a string that can be transmitted and vice versa.
   * Returns 1 for success and 0 for failure.
   * WARNING: This will only work for types that have a svtkDataWriter class.
   */
  static int MarshalDataObject(svtkDataObject* object, svtkCharArray* buffer);
  static int UnMarshalDataObject(svtkCharArray* buffer, svtkDataObject* object);
  //@}

  /**
   * Same as UnMarshalDataObject(svtkCharArray*, svtkDataObject*) except that this
   * method doesn't need to know the type of the data object a priori. It can
   * deduce that from the contents of the \c buffer. May return nullptr data object
   * if \c buffer is nullptr or empty.
   */
  static svtkSmartPointer<svtkDataObject> UnMarshalDataObject(svtkCharArray* buffer);

protected:
  int WriteDataArray(svtkDataArray* object);
  int ReadDataArray(svtkDataArray* object);

  svtkCommunicator();
  ~svtkCommunicator() override;

  // Internal methods called by Send/Receive(svtkDataObject *... ) above.
  int SendElementalDataObject(svtkDataObject* data, int remoteHandle, int tag);
  //@{
  /**
   * GatherV collects arrays in the process with id \c destProcessId.
   * Each process (including the destination) sends its sendArray to
   * the destination process.  The destination process receives the
   * arrays and stores them in rank order in recvArrays.  The \c recvArrays is an
   * array containing  \c NumberOfProcesses elements. The \c recvArray allocates
   * and manages memory for \c recvArrays.
   */
  int GatherV(svtkDataArray* sendArray, svtkDataArray* recvArray,
    svtkSmartPointer<svtkDataArray>* recvArrays, int destProcessId);
  int GatherVElementalDataObject(
    svtkDataObject* sendData, svtkSmartPointer<svtkDataObject>* receiveData, int destProcessId);
  //@}

  int ReceiveDataObject(svtkDataObject* data, int remoteHandle, int tag, int type = -1);
  int ReceiveElementalDataObject(svtkDataObject* data, int remoteHandle, int tag);
  int ReceiveMultiBlockDataSet(svtkMultiBlockDataSet* data, int remoteHandle, int tag);

  int MaximumNumberOfProcesses;
  int NumberOfProcesses;

  int LocalProcessId;

  static int UseCopy;

  svtkIdType Count;

private:
  svtkCommunicator(const svtkCommunicator&) = delete;
  void operator=(const svtkCommunicator&) = delete;
};

#endif // svtkCommunicator_h
// SVTK-HeaderTest-Exclude: svtkCommunicator.h
