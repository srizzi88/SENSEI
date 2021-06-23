/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSocketCommunicator.cxx

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkSocketCommunicator.h"

#include "svtkClientSocket.h"
#include "svtkCommand.h"
#include "svtkObjectFactory.h"
#include "svtkServerSocket.h"
#include "svtkSocketController.h"
#include "svtkStdString.h"
#include "svtkTypeTraits.h"
#include "svtksys/Encoding.hxx"
#include "svtksys/FStream.hxx"
#include <cassert>

#include <algorithm>
#include <list>
#include <map>
#include <vector>

// Uncomment the following line to help with debugging. When
// ENABLE_SYNCHRONIZED_COMMUNICATION is defined, every Send() blocks until the
// receive is successful.
//#define ENABLE_SYNCHRONIZED_COMMUNICATION

class svtkSocketCommunicator::svtkMessageBuffer
{
public:
  typedef std::vector<char> MessageType;
  typedef std::list<MessageType> QueueType;
  typedef std::map<int, QueueType> BufferType;
  BufferType Buffer; // key --> tag, value-->queue of messages.

  bool HasBufferredMessages() { return !this->Buffer.empty(); }

  bool HasMessage(int tag)
  {
    BufferType::iterator iter = this->Buffer.find(tag);
    if (iter == this->Buffer.end())
    {
      return false;
    }
    return (!iter->second.empty());
  }

  void Push(int tag, int numchars, char* data)
  {
    this->Buffer[tag].push_back(MessageType());
    MessageType& msg = this->Buffer[tag].back();
    msg.insert(msg.end(), data, (data + numchars));
  }

  void Pop(int tag)
  {
    this->Buffer[tag].pop_front();
    if (this->Buffer[tag].empty())
    {
      this->Buffer.erase(tag);
    }
  }

  MessageType& Head(int tag) { return this->Buffer[tag].front(); }
};

#define svtkSocketCommunicatorErrorMacro(msg)                                                       \
  if (this->ReportErrors)                                                                          \
  {                                                                                                \
    svtkErrorMacro(msg);                                                                            \
  }

// The handshake checks that the client and server are using the same
// version of this source file.  It first compares a fixed integer
// hash identifier to make sure the hash algorithms match.  Then it
// compares hash strings.  Note that the integer id exchange used to
// represent the CVS revision number of this file, so the value must
// be larger than the last revision which used that strategy.
#define svtkSocketCommunicatorHashId 100 /* MD5 */
#include "svtkSocketCommunicatorHash.h"

svtkStandardNewMacro(svtkSocketCommunicator);
svtkCxxSetObjectMacro(svtkSocketCommunicator, Socket, svtkClientSocket);
//----------------------------------------------------------------------------
svtkSocketCommunicator::svtkSocketCommunicator()
{
  this->Socket = nullptr;
  this->NumberOfProcesses = 2;
  this->SwapBytesInReceivedData = svtkSocketCommunicator::SwapNotSet;
  this->RemoteHas64BitIds = -1; // Invalid until handshake.
  this->PerformHandshake = 1;
  this->IsServer = 0;
  this->LogStream = nullptr;
  this->LogFile = nullptr;
  this->TagMessageLength = 0;
  this->BufferMessage = false;

  this->ReportErrors = 1;
  this->ReceivedMessageBuffer = new svtkSocketCommunicator::svtkMessageBuffer();
}

//----------------------------------------------------------------------------
svtkSocketCommunicator::~svtkSocketCommunicator()
{
  this->SetSocket(nullptr);
  this->SetLogStream(nullptr);
  delete this->ReceivedMessageBuffer;
  this->ReceivedMessageBuffer = nullptr;
}

//----------------------------------------------------------------------------
void svtkSocketCommunicator::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);

  os << indent << "SwapBytesInReceivedData: ";
  if (this->SwapBytesInReceivedData == SwapOff)
  {
    os << "Off\n";
  }
  if (this->SwapBytesInReceivedData == SwapOn)
  {
    os << "On\n";
  }
  if (this->SwapBytesInReceivedData == SwapNotSet)
  {
    os << "NotSet\n";
  }
  os << indent << "IsServer: " << (this->IsServer ? "yes" : "no") << endl;
  os << indent << "RemoteHas64BitIds: " << (this->RemoteHas64BitIds ? "yes" : "no") << endl;
  os << indent << "Socket: ";
  if (this->Socket)
  {
    os << endl;
    this->Socket->PrintSelf(os, indent.GetNextIndent());
  }
  else
  {
    os << "(none)" << endl;
  }

  os << indent << "Perform a handshake: " << (this->PerformHandshake ? "Yes" : "No") << endl;

  os << indent << "ReportErrors: " << this->ReportErrors << endl;
}

//----------------------------------------------------------------------------
void svtkSocketCommunicator::SetLogStream(ostream* stream)
{
  if (this->LogStream != stream)
  {
    // If the log stream is our own log file, close the file.
    if (this->LogFile && this->LogFile == this->LogStream)
    {
      delete this->LogFile;
      this->LogFile = nullptr;
    }

    // Use the given log stream.
    this->LogStream = stream;
  }
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::GetIsConnected()
{
  if (this->Socket)
  {
    return this->Socket->GetConnected();
  }
  return 0;
}

//----------------------------------------------------------------------------
void svtkSocketCommunicator::SetNumberOfProcesses(int svtkNotUsed(num))
{
  svtkErrorMacro("Can not change the number of processes.");
}

//----------------------------------------------------------------------------
ostream* svtkSocketCommunicator::GetLogStream()
{
  return this->LogStream;
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::LogToFile(const char* name)
{
  return this->LogToFile(name, 0);
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::LogToFile(const char* name, int append)
{
  // Close old logging file.
  delete this->LogFile;
  this->LogFile = nullptr;
  this->LogStream = nullptr;

  // Log to given file, if any.
  if (name && name[0])
  {
    this->LogFile = new svtksys::ofstream(name, (ios::out | (append ? ios::ate : ios::trunc)));
    if (!this->LogFile)
    {
      return 0;
    }
    if (!*this->LogFile)
    {
      delete this->LogFile;
      this->LogFile = nullptr;
      return 0;
    }
    this->LogStream = this->LogFile;
  }
  return 1;
}

//-----------------------------------------------------------------------------
int svtkSocketCommunicator::SendVoidArray(
  const void* data, svtkIdType length, int type, int remoteProcessId, int tag)
{
  if (this->CheckForErrorInternal(remoteProcessId))
  {
    return 0;
  }

#ifdef SVTK_USE_64BIT_IDS
  // Special case for type ids.  If the remote does not have 64 bit ids, we
  // need to convert them before sending them.
  if ((type == SVTK_ID_TYPE) && !this->RemoteHas64BitIds)
  {
    std::vector<int> newData;
    newData.resize(length);
    std::copy(reinterpret_cast<const svtkIdType*>(data),
      reinterpret_cast<const svtkIdType*>(data) + length, newData.begin());
    return this->SendVoidArray(&newData[0], length, SVTK_INT, remoteProcessId, tag);
  }
#endif

  int typeSize;
  svtkStdString typeName;
  switch (type)
  {
    svtkTemplateMacro(typeSize = sizeof(SVTK_TT); typeName = svtkTypeTraits<SVTK_TT>().SizedName());
    default:
      svtkWarningMacro(<< "Invalid data type " << type);
      typeSize = 1;
      typeName = "???";
      break;
  }
  // Special case for logging.
  if (type == SVTK_CHAR)
  {
    typeName = "char";
  }

  const char* byteData = reinterpret_cast<const char*>(data);
  int maxSend = SVTK_INT_MAX / typeSize;
  // If sending an array longer than the maximum number that can be held
  // in an integer, break up the array into pieces.
  while (length >= maxSend)
  {
    if (!this->SendTagged(byteData, typeSize, maxSend, tag, typeName))
    {
      return 0;
    }
    byteData += maxSend * typeSize;
    length -= maxSend;
  }
  if (!this->SendTagged(byteData, typeSize, length, tag, typeName))
  {
    return 0;
  }
#ifdef ENABLE_SYNCHRONIZED_COMMUNICATION
  int status[3] = { 0, 0, 0 };
  this->ReceiveTagged(status, sizeof(int), 3, 9876543, "ENABLE_SYNCHRONIZED_COMMUNICATION#1");
  assert(status[0] == 9876543 && status[2] == 9876544 && (status[1] == 1 || status[1] == 2));
  this->SendTagged(status, sizeof(int), 3, 9876544, "ENABLE_SYNCHRONIZED_COMMUNICATION#2");
#endif
  return 1;
}

//-----------------------------------------------------------------------------
inline svtkIdType svtkSocketCommunicatorMin(svtkIdType a, svtkIdType b)
{
  return (a < b) ? a : b;
}

//-----------------------------------------------------------------------------
int svtkSocketCommunicator::ReceiveVoidArray(
  void* data, svtkIdType length, int type, int remoteProcessId, int tag)
{
  this->Count = 0;
  if (this->CheckForErrorInternal(remoteProcessId))
  {
    return 0;
  }

#ifdef SVTK_USE_64BIT_IDS
  // Special case for type ids.  If the remote does not have 64 bit ids, we
  // need to convert them before sending them.
  if ((type == SVTK_ID_TYPE) && !this->RemoteHas64BitIds)
  {
    std::vector<int> newData;
    newData.resize(length);
    int retval = this->ReceiveVoidArray(&newData[0], length, SVTK_INT, remoteProcessId, tag);
    std::copy(newData.begin(), newData.end(), reinterpret_cast<svtkIdType*>(data));
    return retval;
  }
#endif

  int typeSize;
  svtkStdString typeName;
  switch (type)
  {
    svtkTemplateMacro(typeSize = sizeof(SVTK_TT); typeName = svtkTypeTraits<SVTK_TT>().SizedName());
    default:
      svtkWarningMacro(<< "Invalid data type " << type);
      typeSize = 1;
      typeName = "???";
      break;
  }
  // Special case for logging.
  if (type == SVTK_CHAR)
  {
    typeName = "char";
  }

  char* byteData = reinterpret_cast<char*>(data);
  int maxReceive = SVTK_INT_MAX / typeSize;
  // If receiving an array longer than the maximum number that can be held
  // in an integer, break up the array into pieces.
  int ret = 0;
  while (this->ReceiveTagged(
    byteData, typeSize, svtkSocketCommunicatorMin(maxReceive, length), tag, typeName))
  {
    this->Count += this->TagMessageLength;
    byteData += this->TagMessageLength * typeSize;
    length -= this->TagMessageLength;
    if (this->TagMessageLength < maxReceive)
    {
      // words_received in this packet is exactly equal to maxReceive, then it
      // means that the sender is sending at least one more packet for this
      // message. Otherwise, we have received all the packets for this message
      // and we no longer need to iterate.
      ret = 1;
      break;
    }
  }

  // Some crazy special crud for RMIs that may one day screw someone up in
  // a weird way.  No, I did not write this, but I'm sure there is code that
  // relies on it.
  // (This is setting the process id for the sender in the message).
  if (ret && (tag == svtkMultiProcessController::RMI_TAG))
  {
    int* idata = reinterpret_cast<int*>(data);
    idata[2] = 1;
    svtkByteSwap::SwapLE(&idata[2]);
  }

#ifdef ENABLE_SYNCHRONIZED_COMMUNICATION
  int status[3] = { 9876543, 1, 9876544 };
  int other_status[3] = { -1, -1, -1 };
  assert(this->SendTagged(status, sizeof(int), 3, 9876543, "ENABLE_SYNCHRONIZED_COMMUNICATION#1"));
  assert(this->ReceiveTagged(
    other_status, sizeof(int), 3, 9876544, "ENABLE_SYNCHRONIZED_COMMUNICATION#2"));
  assert(
    other_status[0] == status[0] && other_status[1] == status[1] && other_status[2] == status[2]);
#endif

  return ret;
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::Handshake()
{
  if (!this->Socket)
  {
    svtkErrorMacro("No socket set. Cannot perform handshake.");
    return 0;
  }

  if (this->Socket->GetConnectingSide())
  {
    return this->ClientSideHandshake();
  }
  else
  {
    return this->ServerSideHandshake();
  }
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::ServerSideHandshake()
{
  this->IsServer = 1;
  if (this->PerformHandshake)
  {
    // Handshake to determine if the client machine has the same endianness
    char clientIsBE;
    if (!this->ReceiveTagged(
          &clientIsBE, static_cast<int>(sizeof(char)), 1, svtkSocketController::ENDIAN_TAG, nullptr))
    {
      svtkSocketCommunicatorErrorMacro("Endian handshake failed.");
      return 0;
    }
    svtkDebugMacro(<< "Client is " << (clientIsBE ? "big" : "little") << "-endian");

#ifdef SVTK_WORDS_BIGENDIAN
    char IAmBE = 1;
#else
    char IAmBE = 0;
#endif
    svtkDebugMacro(<< "I am " << (IAmBE ? "big" : "little") << "-endian");
    if (!this->SendTagged(
          &IAmBE, static_cast<int>(sizeof(char)), 1, svtkSocketController::ENDIAN_TAG, nullptr))
    {
      svtkSocketCommunicatorErrorMacro("Endian handshake failed.");
      return 0;
    }

    if (clientIsBE != IAmBE)
    {
      this->SwapBytesInReceivedData = svtkSocketCommunicator::SwapOn;
    }
    else
    {
      this->SwapBytesInReceivedData = svtkSocketCommunicator::SwapOff;
    }

    // Check to make sure the client and server have the same version of the
    // socket communicator.
    int myVersion = svtkSocketCommunicator::GetVersion();
    int clientVersion;
    if (!this->ReceiveTagged(&clientVersion, static_cast<int>(sizeof(int)), 1,
          svtkSocketController::VERSION_TAG, nullptr))
    {
      svtkSocketCommunicatorErrorMacro("Version handshake failed.  "
                                      "Perhaps there is a client/server version mismatch.");
      return 0;
    }
    if (!this->SendTagged(
          &myVersion, static_cast<int>(sizeof(int)), 1, svtkSocketController::VERSION_TAG, nullptr))
    {
      svtkSocketCommunicatorErrorMacro("Version handshake failed.  "
                                      "Perhaps there is a client/server version mismatch.");
      return 0;
    }
    if (myVersion != clientVersion)
    {
      svtkSocketCommunicatorErrorMacro("Client/server version mismatch.");
      return 0;
    }

    // Compare hashes of this source file from each side.
    const char myHash[] = svtkSocketCommunicatorHash;
    char clientHash[sizeof(myHash)];
    if (!this->ReceiveTagged(&clientHash, 1, static_cast<int>(sizeof(clientHash)),
          svtkSocketController::HASH_TAG, nullptr) ||
      !this->SendTagged(
        &myHash, 1, static_cast<int>(sizeof(myHash)), svtkSocketController::HASH_TAG, nullptr))
    {
      svtkSocketCommunicatorErrorMacro("Version hash handshake failed.  "
                                      "Perhaps there is a client/server version mismatch.");
      return 0;
    }
    if (strncmp(myHash, clientHash, sizeof(myHash) - 1) != 0)
    {
      svtkSocketCommunicatorErrorMacro("Client/server version hash mismatch.");
      return 0;
    }

    // Handshake to determine if remote has 64 bit ids.
#ifdef SVTK_USE_64BIT_IDS
    int IHave64BitIds = 1;
#else
    int IHave64BitIds = 0;
#endif
    if (!this->ReceiveTagged(&(this->RemoteHas64BitIds), static_cast<int>(sizeof(int)), 1,
          svtkSocketController::IDTYPESIZE_TAG, nullptr))
    {
      svtkSocketCommunicatorErrorMacro("Id Type Size handshake failed.");
      return 0;
    }
    svtkDebugMacro(<< "Remote has 64 bit ids: " << this->RemoteHas64BitIds);
    if (!this->SendTagged(&IHave64BitIds, static_cast<int>(sizeof(int)), 1,
          svtkSocketController::IDTYPESIZE_TAG, nullptr))
    {
      svtkSocketCommunicatorErrorMacro("Id Type Size handshake failed.");
      return 0;
    }
  }
  return 1;
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::ClientSideHandshake()
{
  this->IsServer = 0;
  if (!this->PerformHandshake)
  {
    return 1;
  }

  // Handshake to determine if the server machine has the same endianness
#ifdef SVTK_WORDS_BIGENDIAN
  char IAmBE = 1;
#else
  char IAmBE = 0;
#endif
  svtkDebugMacro(<< "I am " << (IAmBE ? "big" : "little") << "-endian");
  if (!this->SendTagged(
        &IAmBE, static_cast<int>(sizeof(char)), 1, svtkSocketController::ENDIAN_TAG, nullptr))
  {
    svtkSocketCommunicatorErrorMacro("Endian handshake failed.");
    return 0;
  }

  char serverIsBE;
  if (!this->ReceiveTagged(
        &serverIsBE, static_cast<int>(sizeof(char)), 1, svtkSocketController::ENDIAN_TAG, nullptr))
  {
    svtkSocketCommunicatorErrorMacro("Endian handshake failed.");
    return 0;
  }
  svtkDebugMacro(<< "Server is " << (serverIsBE ? "big" : "little") << "-endian");

  if (serverIsBE != IAmBE)
  {
    this->SwapBytesInReceivedData = svtkSocketCommunicator::SwapOn;
  }
  else
  {
    this->SwapBytesInReceivedData = svtkSocketCommunicator::SwapOff;
  }

  // Check to make sure the client and server have the same version of the
  // socket communicator.
  int myVersion = svtkSocketCommunicator::GetVersion();
  int serverVersion;
  if (!this->SendTagged(
        &myVersion, static_cast<int>(sizeof(int)), 1, svtkSocketController::VERSION_TAG, nullptr))
  {
    svtkSocketCommunicatorErrorMacro("Version handshake failed.  "
                                    "Perhaps there is a client/server version mismatch.");
    return 0;
  }
  if (!this->ReceiveTagged(&serverVersion, static_cast<int>(sizeof(int)), 1,
        svtkSocketController::VERSION_TAG, nullptr))
  {
    svtkSocketCommunicatorErrorMacro("Version handshake failed.  "
                                    "Perhaps there is a client/server version mismatch.");
    return 0;
  }
  if (myVersion != serverVersion)
  {
    svtkSocketCommunicatorErrorMacro("Client/server version mismatch.");
    return 0;
  }

  // Compare hashes of this source file from each side.
  const char myHash[] = svtkSocketCommunicatorHash;
  char serverHash[sizeof(myHash)];
  if (!this->SendTagged(
        &myHash, 1, static_cast<int>(sizeof(myHash)), svtkSocketController::HASH_TAG, nullptr) ||
    !this->ReceiveTagged(
      &serverHash, 1, static_cast<int>(sizeof(serverHash)), svtkSocketController::HASH_TAG, nullptr))
  {
    svtkSocketCommunicatorErrorMacro("Version hash handshake failed.  "
                                    "Perhaps there is a client/server version mismatch.");
    return 0;
  }
  if (strncmp(myHash, serverHash, sizeof(myHash) - 1) != 0)
  {
    svtkSocketCommunicatorErrorMacro("Client/server version hash mismatch.");
    return 0;
  }

  // Handshake to determine if remote has 64 bit ids.
#ifdef SVTK_USE_64BIT_IDS
  int IHave64BitIds = 1;
#else
  int IHave64BitIds = 0;
#endif
  if (!this->SendTagged(&IHave64BitIds, static_cast<int>(sizeof(int)), 1,
        svtkSocketController::IDTYPESIZE_TAG, nullptr))
  {
    svtkSocketCommunicatorErrorMacro("Id Type Size handshake failed.");
    return 0;
  }
  if (!this->ReceiveTagged(&(this->RemoteHas64BitIds), static_cast<int>(sizeof(int)), 1,
        svtkSocketController::IDTYPESIZE_TAG, nullptr))
  {
    svtkSocketCommunicatorErrorMacro("Id Type Size handshake failed.");
    return 0;
  }
  svtkDebugMacro(<< "Remote has 64 bit ids: " << this->RemoteHas64BitIds);

  return 1;
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::WaitForConnection(int port)
{
  if (this->GetIsConnected())
  {
    svtkSocketCommunicatorErrorMacro("Communicator port " << 1 << " is occupied.");
    return 0;
  }
  svtkServerSocket* soc = svtkServerSocket::New();
  if (soc->CreateServer(port) != 0)
  {
    soc->Delete();
    return 0;
  }
  int ret = this->WaitForConnection(soc);
  soc->Delete();

  return ret;
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::WaitForConnection(svtkServerSocket* socket, unsigned long msec /*=0*/)
{
  if (this->GetIsConnected())
  {
    svtkSocketCommunicatorErrorMacro("Communicator port " << 1 << " is occupied.");
    return 0;
  }

  if (!socket)
  {
    return 0;
  }

  svtkClientSocket* cs = socket->WaitForConnection(msec);
  if (cs)
  {
    this->SetSocket(cs);
    cs->Delete();
  }

  if (!this->Socket)
  {
    return 0;
  }
  return this->ServerSideHandshake();
}

//----------------------------------------------------------------------------
void svtkSocketCommunicator::CloseConnection()
{
  if (this->Socket)
  {
    this->Socket->CloseSocket();
    this->Socket->Delete();
    this->Socket = nullptr;
  }
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::ConnectTo(const char* hostName, int port)
{

  if (this->GetIsConnected())
  {
    svtkSocketCommunicatorErrorMacro("Communicator port " << 1 << " is occupied.");
    return 0;
  }

  svtkClientSocket* tmp = svtkClientSocket::New();

  if (tmp->ConnectToServer(hostName, port))
  {
    svtkSocketCommunicatorErrorMacro("Can not connect to " << hostName << " on port " << port);
    tmp->Delete();
    return 0;
  }
  this->SetSocket(tmp);
  tmp->Delete();

  svtkDebugMacro("Connected to " << hostName << " on port " << port);
  return this->ClientSideHandshake();
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::SendTagged(
  const void* data, int wordSize, int numWords, int tag, const char* logName)
{
  if (!this->Socket->Send(&tag, static_cast<int>(sizeof(int))))
  {
    svtkSocketCommunicatorErrorMacro("Could not send tag.");
    return 0;
  }
  int length = wordSize * numWords;
  if (!this->Socket->Send(&length, static_cast<int>(sizeof(int))))
  {
    svtkSocketCommunicatorErrorMacro("Could not send length.");
    return 0;
  }
  // Only do the actual send if there is some data in the message.
  if (length > 0)
  {
    if (!this->Socket->Send(data, length))
    {
      svtkSocketCommunicatorErrorMacro("Could not send message.");
      return 0;
    }
  }

  // Log this event.
  this->LogTagged("Sent", data, wordSize, numWords, tag, logName);

  return 1;
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::ReceivedTaggedFromBuffer(
  void* data, int wordSize, int numWords, int tag, const char* logName)
{
  this->TagMessageLength = 0;
  svtkMessageBuffer::MessageType& message = this->ReceivedMessageBuffer->Head(tag);
  if (static_cast<unsigned int>(numWords * wordSize) < message.size())
  {
    svtkSocketCommunicatorErrorMacro("Message truncated."
                                    "Receive buffer size ("
      << (wordSize * numWords)
      << ") is less than "
         "message length ("
      << message.size() << ")");
    return 0;
  }

  // The static_cast is OK since we split messages > SVTK_INT_MAX.
  this->TagMessageLength = static_cast<int>(message.size()) / wordSize;
  memcpy(data, &message[0], message.size());
  this->ReceivedMessageBuffer->Pop(tag);

  this->FixByteOrder(data, wordSize, numWords);

  // Log this event.
  this->LogTagged("Receive(from Buffer)", data, wordSize, numWords, tag, logName);

  return 1;
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::ReceiveTagged(
  void* data, int wordSize, int numWords, int tag, const char* logName)
{
  if (this->ReceivedMessageBuffer->HasMessage(tag))
  {
    // If a message for the given tag was already received, it will be in the
    // queue, so simply return that.
    return this->ReceivedTaggedFromBuffer(data, wordSize, numWords, tag, logName);
  }

  // Since the message queue for the \c tag is empty, try to receive the message
  // over the socket.
  this->TagMessageLength = 0;
  int success = 0;
  int length = -1;
  while (!success)
  {
    int recvTag = -1;
    length = -1;
    if (!this->Socket->Receive(&recvTag, static_cast<int>(sizeof(int))))
    {
      svtkSocketCommunicatorErrorMacro("Could not receive tag. " << tag);
      return 0;
    }
    if (this->SwapBytesInReceivedData == svtkSocketCommunicator::SwapOn)
    {
      svtkSwap4(reinterpret_cast<char*>(&recvTag));
    }
    if (!this->Socket->Receive(&length, static_cast<int>(sizeof(int))))
    {
      svtkSocketCommunicatorErrorMacro("Could not receive length.");
      return 0;
    }
    if (this->SwapBytesInReceivedData == svtkSocketCommunicator::SwapOn)
    {
      svtkSwap4(reinterpret_cast<char*>(&length));
    }
    else if (this->SwapBytesInReceivedData == svtkSocketCommunicator::SwapNotSet)
    {
      // Clearly, we still haven't determined our endianness. In that case, the
      // only legal communication should be svtkSocketController::ENDIAN_TAG.
      // However, I am not flagging an error since applications may used socket
      // communicator without the handshake part (where it's assumed that the
      // application takes over the handshaking). So if the message is for
      // endianness check, then we simply adjust the length.
      if (tag == svtkSocketController::ENDIAN_TAG)
      {
        // ignore the length the we received, just set it to what we want.
        length = numWords * wordSize;
      }
    }
    if (recvTag != tag)
    {
      // There's a tag mismatch, call the error handler. If the error handler
      // tells us that the mismatch is non-fatal, we keep on receiving,
      // otherwise we quit with an error.
      char* idata = new char[length + sizeof(recvTag) + sizeof(length)];
      char* ptr = idata;
      memcpy(ptr, (void*)&recvTag, sizeof(recvTag));
      ptr += sizeof(recvTag);
      memcpy(ptr, (void*)&length, sizeof(length));
      ptr += sizeof(length);
      this->BufferMessage = false;
      this->ReceivePartialTagged(ptr, 1, length, tag, "Wrong tag");
      int res = this->InvokeEvent(svtkCommand::WrongTagEvent, idata);
      // if res == 1, then it implies that the observer has processed the
      // message.
      // if res == 0 and this->BufferMessage is true, it implies that the
      // observer wants us to buffer this message for later use.
      if (this->BufferMessage)
      {
        // TODO: we may want to optimize this to avoid multiple copying of the
        // data.
        if (this->LogStream)
        {
          *this->LogStream << "Bufferring last message (" << recvTag << ")" << endl;
        }
        this->ReceivedMessageBuffer->Push(recvTag, length, ptr);
      }
      delete[] idata;

      if (res || this->BufferMessage)
      {
        continue;
      }

      svtkSocketCommunicatorErrorMacro(
        "Tag mismatch: got " << recvTag << ", expecting " << tag << ".");
      return 0;
    }
    else
    {
      success = 1;
    }
  }

  if ((numWords * wordSize) < length)
  {
    svtkSocketCommunicatorErrorMacro("Message truncated."
                                    "Receive buffer size ("
      << (wordSize * numWords)
      << ") is less than "
         "message length ("
      << length << ")");
    return 0;
  }

  this->TagMessageLength = length / wordSize;
  return this->ReceivePartialTagged(data, wordSize, length / wordSize, tag, logName);
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::ReceivePartialTagged(
  void* data, int wordSize, int numWords, int tag, const char* logName)
{
  // Only do the actual receive if there is some data to receive
  if (wordSize * numWords > 0)
  {
    if (!this->Socket->Receive(data, wordSize * numWords))
    {
      svtkSocketCommunicatorErrorMacro("Could not receive message.");
      return 0;
    }
  }

  this->FixByteOrder(data, wordSize, numWords);

  // Log this event.
  this->LogTagged("Received", data, wordSize, numWords, tag, logName);
  return 1;
}

//----------------------------------------------------------------------------
void svtkSocketCommunicator::FixByteOrder(void* data, int wordSize, int numWords)
{
  // Unless we're dealing with chars, then check byte ordering.
  // This is really bad and should probably use some enum for types
  if (this->SwapBytesInReceivedData == svtkSocketCommunicator::SwapOn)
  {
    if (wordSize == 4)
    {
      svtkDebugMacro(<< " swapping 4 range, size = " << wordSize << " length = " << numWords);
      svtkSwap4Range(reinterpret_cast<char*>(data), numWords);
    }
    else if (wordSize == 8)
    {
      svtkDebugMacro(<< " swapping 8 range, size = " << wordSize << " length = " << numWords);
      svtkSwap8Range(reinterpret_cast<char*>(data), numWords);
    }
  }
}

//----------------------------------------------------------------------------
bool svtkSocketCommunicator::HasBufferredMessages()
{
  return this->ReceivedMessageBuffer->HasBufferredMessages();
}

//----------------------------------------------------------------------------
template <class T, class OutType>
void svtkSocketCommunicatorLogArray(ostream& os, T* array, int length, int max, OutType*)
{
  if (length > 0)
  {
    int num = (length <= max) ? length : max;
    os << " data={" << static_cast<OutType>(array[0]);
    for (int i = 1; i < num; ++i)
    {
      os << " " << static_cast<OutType>(array[i]);
    }
    if (length > max)
    {
      os << " ...";
    }
    os << "}";
  }
}

//----------------------------------------------------------------------------
void svtkSocketCommunicator::LogTagged(
  const char* name, const void* data, int wordSize, int numWords, int tag, const char* logName)
{
  if (this->LogStream)
  {
    // Log the general event information.
    *this->LogStream << name;
    if (logName)
    {
      *this->LogStream << " " << logName;
    }
    *this->LogStream << " data: tag=" << tag << " wordSize=" << wordSize
                     << " numWords=" << numWords;

    // If this is a string, log the first 70 characters.  If this is
    // an array of data values, log the first few.
    if (wordSize == static_cast<int>(sizeof(char)) && logName && (strcmp(logName, "char") == 0))
    {
      const char* chars = reinterpret_cast<const char*>(data);
      if ((chars[numWords - 1]) == 0 && (static_cast<int>(strlen(chars)) == numWords - 1))
      {
        // String data.  Display the first 70 characters.
        *this->LogStream << " data={";
        if (numWords <= 71)
        {
          *this->LogStream << chars;
        }
        else
        {
          this->LogStream->write(reinterpret_cast<const char*>(data), 70);
          *this->LogStream << " ...";
        }
        *this->LogStream << "}";
      }
      else
      {
        // Not string data.  Display the characters as integer values.
        svtkSocketCommunicatorLogArray(*this->LogStream, reinterpret_cast<const char*>(data),
          numWords, 6, static_cast<int*>(nullptr));
      }
    }
    else if ((wordSize == 1) && logName && (strcmp(logName, "Int8") == 0))
    {
      svtkSocketCommunicatorLogArray(*this->LogStream, reinterpret_cast<const svtkTypeInt8*>(data),
        numWords, 6, static_cast<svtkTypeInt16*>(nullptr));
    }
    else if ((wordSize == 1) && logName && (strcmp(logName, "UInt8") == 0))
    {
      svtkSocketCommunicatorLogArray(*this->LogStream, reinterpret_cast<const svtkTypeUInt8*>(data),
        numWords, 6, static_cast<svtkTypeUInt16*>(nullptr));
    }
    else if ((wordSize == 2) && logName && (strcmp(logName, "Int16") == 0))
    {
      svtkSocketCommunicatorLogArray(*this->LogStream, reinterpret_cast<const svtkTypeInt16*>(data),
        numWords, 6, static_cast<svtkTypeInt16*>(nullptr));
    }
    else if ((wordSize == 2) && logName && (strcmp(logName, "UInt16") == 0))
    {
      svtkSocketCommunicatorLogArray(*this->LogStream, reinterpret_cast<const svtkTypeUInt16*>(data),
        numWords, 6, static_cast<svtkTypeUInt16*>(nullptr));
    }
    else if ((wordSize == 4) && logName && (strcmp(logName, "Int32") == 0))
    {
      svtkSocketCommunicatorLogArray(*this->LogStream, reinterpret_cast<const svtkTypeInt32*>(data),
        numWords, 6, static_cast<svtkTypeInt32*>(nullptr));
    }
    else if ((wordSize == 4) && logName && (strcmp(logName, "UInt32") == 0))
    {
      svtkSocketCommunicatorLogArray(*this->LogStream, reinterpret_cast<const svtkTypeUInt32*>(data),
        numWords, 6, static_cast<svtkTypeUInt32*>(nullptr));
    }
    else if ((wordSize == 8) && logName && (strcmp(logName, "Int64") == 0))
    {
      svtkSocketCommunicatorLogArray(*this->LogStream, reinterpret_cast<const svtkTypeInt64*>(data),
        numWords, 6, static_cast<svtkTypeInt64*>(nullptr));
    }
    else if ((wordSize == 8) && logName && (strcmp(logName, "UInt64") == 0))
    {
      svtkSocketCommunicatorLogArray(*this->LogStream, reinterpret_cast<const svtkTypeUInt64*>(data),
        numWords, 6, static_cast<svtkTypeUInt64*>(nullptr));
    }
    else if ((wordSize == 4) && logName && (strcmp(logName, "Float32") == 0))
    {
      svtkSocketCommunicatorLogArray(*this->LogStream, reinterpret_cast<const svtkTypeFloat32*>(data),
        numWords, 6, static_cast<svtkTypeFloat32*>(nullptr));
    }
    else if ((wordSize == 8) && logName && (strcmp(logName, "Float64") == 0))
    {
      svtkSocketCommunicatorLogArray(*this->LogStream, reinterpret_cast<const svtkTypeFloat64*>(data),
        numWords, 6, static_cast<svtkTypeFloat64*>(nullptr));
    }
    *this->LogStream << endl;
  }
}

//----------------------------------------------------------------------------
int svtkSocketCommunicator::CheckForErrorInternal(int id)
{
  if (id == 0)
  {
    svtkSocketCommunicatorErrorMacro("Can not connect to myself!");
    return 1;
  }
  else if (id >= this->NumberOfProcesses)
  {
    svtkSocketCommunicatorErrorMacro("No port for process " << id << " exists.");
    return 1;
  }
  else if (!this->Socket)
  {
    svtkSocketCommunicatorErrorMacro("Socket does not exist.");
    return 1;
  }
  return 0;
}

//-----------------------------------------------------------------------------
void svtkSocketCommunicator::Barrier()
{
  int junk = 0;
  if (this->IsServer)
  {
    this->Send(&junk, 1, 1, BARRIER_TAG);
    this->Receive(&junk, 1, 1, BARRIER_TAG);
  }
  else
  {
    this->Receive(&junk, 1, 1, BARRIER_TAG);
    this->Send(&junk, 1, 1, BARRIER_TAG);
  }
}

//-----------------------------------------------------------------------------
int svtkSocketCommunicator::BroadcastVoidArray(void* data, svtkIdType length, int type, int root)
{
  return this->Superclass::BroadcastVoidArray(data, length, type, root);
}

//-----------------------------------------------------------------------------
int svtkSocketCommunicator::GatherVoidArray(const void*, void*, svtkIdType, int, int)
{
  svtkErrorMacro("Collective operations not supported on sockets.");
  return 0;
}
int svtkSocketCommunicator::GatherVVoidArray(
  const void*, void*, svtkIdType, svtkIdType*, svtkIdType*, int, int)
{
  svtkErrorMacro("Collective operations not supported on sockets.");
  return 0;
}
int svtkSocketCommunicator::ScatterVoidArray(const void*, void*, svtkIdType, int, int)
{
  svtkErrorMacro("Collective operations not supported on sockets.");
  return 0;
}
int svtkSocketCommunicator::ScatterVVoidArray(
  const void*, void*, svtkIdType*, svtkIdType*, svtkIdType, int, int)
{
  svtkErrorMacro("Collective operations not supported on sockets.");
  return 0;
}
int svtkSocketCommunicator::AllGatherVoidArray(const void*, void*, svtkIdType, int)
{
  svtkErrorMacro("Collective operations not supported on sockets.");
  return 0;
}
int svtkSocketCommunicator::AllGatherVVoidArray(
  const void*, void*, svtkIdType, svtkIdType*, svtkIdType*, int)
{
  svtkErrorMacro("Collective operations not supported on sockets.");
  return 0;
}
int svtkSocketCommunicator::ReduceVoidArray(const void*, void*, svtkIdType, int, int, int)
{
  svtkErrorMacro("Collective operations not supported on sockets.");
  return 0;
}
int svtkSocketCommunicator::ReduceVoidArray(const void*, void*, svtkIdType, int, Operation*, int)
{
  svtkErrorMacro("Collective operations not supported on sockets.");
  return 0;
}
int svtkSocketCommunicator::AllReduceVoidArray(const void*, void*, svtkIdType, int, int)
{
  svtkErrorMacro("Collective operations not supported on sockets.");
  return 0;
}
int svtkSocketCommunicator::AllReduceVoidArray(const void*, void*, svtkIdType, int, Operation*)
{
  svtkErrorMacro("Collective operations not supported on sockets.");
  return 0;
}

//-----------------------------------------------------------------------------
int svtkSocketCommunicator::GetVersion()
{
  return svtkSocketCommunicatorHashId;
}
