/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSocketCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSocketCommunicator
 * @brief   Process communication using Sockets
 *
 * This is a concrete implementation of svtkCommunicator which supports
 * interprocess communication using BSD style sockets.
 * It supports byte swapping for the communication of machines
 * with different endianness.
 *
 * @warning
 * Communication between 32 bit and 64 bit systems is not fully
 * supported. If a type does not have the same length on both
 * systems, this communicator can not be used to transfer data
 * of that type.
 *
 * @sa
 * svtkCommunicator svtkSocketController
 */

#ifndef svtkSocketCommunicator_h
#define svtkSocketCommunicator_h

#include "svtkCommunicator.h"
#include "svtkParallelCoreModule.h" // For export macro

#include "svtkByteSwap.h" // Needed for svtkSwap macros

#ifdef SVTK_WORDS_BIGENDIAN
#define svtkSwap4 svtkByteSwap::Swap4LE
#define svtkSwap4Range svtkByteSwap::Swap4LERange
#define svtkSwap8 svtkByteSwap::Swap8LE
#define svtkSwap8Range svtkByteSwap::Swap8LERange
#else
#define svtkSwap4 svtkByteSwap::Swap4BE
#define svtkSwap4Range svtkByteSwap::Swap4BERange
#define svtkSwap8 svtkByteSwap::Swap8BE
#define svtkSwap8Range svtkByteSwap::Swap8BERange
#endif

class svtkClientSocket;
class svtkServerSocket;

class SVTKPARALLELCORE_EXPORT svtkSocketCommunicator : public svtkCommunicator
{
public:
  static svtkSocketCommunicator* New();
  svtkTypeMacro(svtkSocketCommunicator, svtkCommunicator);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Wait for connection on a given port.
   * These methods return 1 on success, 0 on error.
   */
  virtual int WaitForConnection(int port);
  virtual int WaitForConnection(svtkServerSocket* socket, unsigned long msec = 0);
  //@}

  /**
   * Close a connection.
   */
  virtual void CloseConnection();

  /**
   * Open a connection to host.
   */
  virtual int ConnectTo(const char* hostName, int port);

  //@{
  /**
   * Returns 1 if bytes must be swapped in received ints, floats, etc
   */
  svtkGetMacro(SwapBytesInReceivedData, int);
  //@}

  /**
   * Is the communicator connected?.
   */
  int GetIsConnected();

  /**
   * Set the number of processes you will be using.
   */
  void SetNumberOfProcesses(int num) override;

  //------------------ Communication --------------------

  //@{
  /**
   * Performs the actual communication.  You will usually use the convenience
   * Send functions defined in the superclass.
   */
  int SendVoidArray(
    const void* data, svtkIdType length, int type, int remoteHandle, int tag) override;
  int ReceiveVoidArray(void* data, svtkIdType length, int type, int remoteHandle, int tag) override;
  //@}

  /**
   * This class foolishly breaks the conventions of the superclass, so this
   * overload fixes the method.
   */
  void Barrier() override;

  //@{
  /**
   * This class foolishly breaks the conventions of the superclass, so the
   * default implementations of these methods do not work.  These just give
   * errors instead.
   */
  int BroadcastVoidArray(void* data, svtkIdType length, int type, int srcProcessId) override;
  int GatherVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType length, int type,
    int destProcessId) override;
  int GatherVVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int type, int destProcessId) override;
  int ScatterVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType length, int type,
    int srcProcessId) override;
  int ScatterVVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType* sendLengths,
    svtkIdType* offsets, svtkIdType recvLength, int type, int srcProcessId) override;
  int AllGatherVoidArray(
    const void* sendBuffer, void* recvBuffer, svtkIdType length, int type) override;
  int AllGatherVVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType sendLength,
    svtkIdType* recvLengths, svtkIdType* offsets, int type) override;
  int ReduceVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType length, int type,
    int operation, int destProcessId) override;
  int ReduceVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType length, int type,
    Operation* operation, int destProcessId) override;
  int AllReduceVoidArray(
    const void* sendBuffer, void* recvBuffer, svtkIdType length, int type, int operation) override;
  int AllReduceVoidArray(const void* sendBuffer, void* recvBuffer, svtkIdType length, int type,
    Operation* operation) override;
  //@}

  //@{
  /**
   * Set or get the PerformHandshake ivar. If it is on, the communicator
   * will try to perform a handshake when connected.
   * It is on by default.
   */
  svtkSetClampMacro(PerformHandshake, svtkTypeBool, 0, 1);
  svtkBooleanMacro(PerformHandshake, svtkTypeBool);
  svtkGetMacro(PerformHandshake, svtkTypeBool);
  //@}

  //@{
  /**
   * Get/Set the output stream to which communications should be
   * logged.  This is intended as a debugging feature.
   */
  virtual void SetLogStream(ostream* stream);
  virtual ostream* GetLogStream();
  //@}

  //@{
  /**
   * Log messages to the given file.  The file is truncated unless the
   * second argument is non-zero (default is to truncate).  If the
   * file name is empty or nullptr, logging is disabled.  Returns 0 if
   * the file failed to open, and 1 otherwise.
   */
  virtual int LogToFile(const char* name);
  virtual int LogToFile(const char* name, int append);
  //@}

  //@{
  /**
   * If ReportErrors if false, all svtkErrorMacros are suppressed.
   */
  svtkSetMacro(ReportErrors, int);
  svtkGetMacro(ReportErrors, int);
  //@}

  //@{
  /**
   * Get/Set the actual socket used for communication.
   */
  svtkGetObjectMacro(Socket, svtkClientSocket);
  void SetSocket(svtkClientSocket*);
  //@}

  /**
   * Performs handshake. This uses svtkClientSocket::ConnectingSide to decide
   * whether to perform ServerSideHandshake or ClientSideHandshake.
   */
  int Handshake();

  /**
   * Performs ServerSide handshake.
   * One should preferably use Handshake() which calls ServerSideHandshake or
   * ClientSideHandshake as required.
   */
  int ServerSideHandshake();

  /**
   * Performs ClientSide handshake.
   * One should preferably use Handshake() which calls ServerSideHandshake or
   * ClientSideHandshake as required.
   */
  int ClientSideHandshake();

  //@{
  /**
   * Returns true if this side of the socket is the server.  The result
   * is invalid if the socket is not connected.
   */
  svtkGetMacro(IsServer, int);
  //@}

  /**
   * Uniquely identifies the version of this class.  If the versions match,
   * then the socket communicators should be compatible.
   */
  static int GetVersion();

  /**
   * This flag is cleared before svtkCommand::WrongTagEvent is fired when ever a
   * message with mismatched tag is received. If the handler wants the message
   * to be buffered for later use, it should set this flag to true. In which
   * case the svtkSocketCommunicator will buffer the message and it will be
   * automatically processed the next time one does a ReceiveTagged() with a
   * matching tag.
   */
  void BufferCurrentMessage() { this->BufferMessage = true; }

  /**
   * Returns true if there are any messages in the receive buffer.
   */
  bool HasBufferredMessages();

protected:
  svtkClientSocket* Socket;
  int SwapBytesInReceivedData;
  int RemoteHas64BitIds;
  svtkTypeBool PerformHandshake;
  int IsServer;

  int ReportErrors;

  ostream* LogFile;
  ostream* LogStream;

  svtkSocketCommunicator();
  ~svtkSocketCommunicator() override;

  // Wrappers around send/recv calls to implement loops.  Return 1 for
  // success, and 0 for failure.
  int SendTagged(const void* data, int wordSize, int numWords, int tag, const char* logName);
  int ReceiveTagged(void* data, int wordSize, int numWords, int tag, const char* logName);
  int ReceivePartialTagged(void* data, int wordSize, int numWords, int tag, const char* logName);

  int ReceivedTaggedFromBuffer(
    void* data, int wordSize, int numWords, int tag, const char* logName);

  /**
   * Fix byte order for received data.
   */
  void FixByteOrder(void* data, int wordSize, int numWords);

  // Internal utility methods.
  void LogTagged(
    const char* name, const void* data, int wordSize, int numWords, int tag, const char* logName);
  int CheckForErrorInternal(int id);
  bool BufferMessage;

private:
  svtkSocketCommunicator(const svtkSocketCommunicator&) = delete;
  void operator=(const svtkSocketCommunicator&) = delete;

  int SelectSocket(int socket, unsigned long msec);

  // SwapBytesInReceiveData needs an invalid / not set.
  // This avoids checking length of endian handshake.
  enum ErrorIds
  {
    SwapOff = 0,
    SwapOn,
    SwapNotSet
  };

  // One may be tempted to change this to a svtkIdType, but really an int is
  // enough since we split messages > SVTK_INT_MAX.
  int TagMessageLength;

  //  Buffer to save messages received with different tag than requested.
  class svtkMessageBuffer;
  svtkMessageBuffer* ReceivedMessageBuffer;
};

#endif
