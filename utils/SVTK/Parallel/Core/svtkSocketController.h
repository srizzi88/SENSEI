/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSocketController.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSocketController
 * @brief   Process communication using Sockets
 *
 * This is a concrete implementation of svtkMultiProcessController.
 * It supports one-to-one communication using sockets. Note that
 * process 0 will always correspond to self and process 1 to the
 * remote process. This class is best used with ports.
 *
 * @bug
 * Note that because process 0 will always correspond to self, this class breaks
 * assumptions usually implied when using ad-hoc polymorphism.  That is, the
 * svtkSocketController will behave differently than other subclasses of
 * svtkMultiProcessController.  If you upcast svtkSocketController to
 * svtkMultiProcessController and send it to a method that does not know that the
 * object is actually a svtkSocketController, the object may not behave as
 * intended.  For example, if that oblivious class chose to identify a "root"
 * based on the local process id, then both sides of the controller will think
 * they are the root (and that will probably lead to deadlock).  If you plan to
 * upcast to svtkMultiProcessController, you should probably use the
 * CreateCompliantController instead.
 *
 * @sa
 * svtkMultiProcessController svtkSocketCommunicator svtkInputPort svtkOutputPort
 */

#ifndef svtkSocketController_h
#define svtkSocketController_h

#include "svtkMultiProcessController.h"
#include "svtkParallelCoreModule.h" // For export macro

class svtkSocketCommunicator;

class SVTKPARALLELCORE_EXPORT svtkSocketController : public svtkMultiProcessController
{
public:
  static svtkSocketController* New();
  svtkTypeMacro(svtkSocketController, svtkMultiProcessController);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This method is for initialiazing sockets.
   * One of these is REQUIRED for Windows.
   */
  void Initialize(int* argc, char*** argv, int) override { this->Initialize(argc, argv); }
  void Initialize(int* argc, char*** argv) override;
  virtual void Initialize() { this->Initialize(nullptr, nullptr); }

  /**
   * Does not apply to sockets. Does nothing.
   */
  void Finalize() override {}
  void Finalize(int) override {}

  /**
   * Does not apply to sockets. Does nothing.
   */
  void SingleMethodExecute() override {}

  /**
   * Does not apply to sockets.  Does nothing.
   */
  void MultipleMethodExecute() override {}

  /**
   * Does not apply to sockets. Does nothing.
   */
  void CreateOutputWindow() override {}

  /**
   * Wait for connection on a given port, forwarded
   * to the communicator
   */
  virtual int WaitForConnection(int port);

  /**
   * Close a connection, forwarded
   * to the communicator
   */
  virtual void CloseConnection();

  /**
   * Open a connection to a give machine, forwarded
   * to the communicator
   */
  virtual int ConnectTo(const char* hostName, int port);

  int GetSwapBytesInReceivedData();

  /**
   * Set the communicator used in normal and rmi communications.
   */
  void SetCommunicator(svtkSocketCommunicator* comm);

  /**
   * FOOLISH MORTALS!  Thou hast forsaken the sacred laws of ad-hoc polymorphism
   * when thou broke a critical assumption of the superclass (namely, each
   * process has thine own id).  The time frame to fix thy error has passed.
   * Too much code has come to rely on this abhorrent behavior.  Instead, we
   * offer this gift: a method for creating an equivalent communicator with
   * correct process id semantics.  The calling code is responsible for
   * deleting this controller.
   */
  svtkMultiProcessController* CreateCompliantController();

  enum Consts
  {
    ENDIAN_TAG = 1010580540,     // 0x3c3c3c3c
    IDTYPESIZE_TAG = 1027423549, // 0x3d3d3d3d
    VERSION_TAG = 1044266558,    // 0x3e3e3e3e
    HASH_TAG = 0x3f3f3f3f
  };

protected:
  svtkSocketController();
  ~svtkSocketController() override;

  // Initialize only once, finialize on destruction.
  static int Initialized;

private:
  svtkSocketController(const svtkSocketController&) = delete;
  void operator=(const svtkSocketController&) = delete;
};

#endif // svtkSocketController_h
