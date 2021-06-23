/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDummyCommunicator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkDummyCommunicator
 * @brief   Dummy controller for single process applications.
 *
 *
 *
 * This is a dummy communicator, which can be used by applications that always
 * require a controller but are also compiled on systems without threads or MPI.
 * Because there is always only one process, no real communication takes place.
 *
 */

#ifndef svtkDummyCommunicator_h
#define svtkDummyCommunicator_h

#include "svtkCommunicator.h"
#include "svtkParallelCoreModule.h" // For export macro

class SVTKPARALLELCORE_EXPORT svtkDummyCommunicator : public svtkCommunicator
{
public:
  svtkTypeMacro(svtkDummyCommunicator, svtkCommunicator);
  static svtkDummyCommunicator* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Since there is no one to communicate with, these methods just report an
   * error.
   */
  int SendVoidArray(const void*, svtkIdType, int, int, int) override
  {
    svtkWarningMacro("There is no one to send to.");
    return 0;
  }
  int ReceiveVoidArray(void*, svtkIdType, int, int, int) override
  {
    svtkWarningMacro("There is no one to receive from.");
    return 0;
  }
  //@}

protected:
  svtkDummyCommunicator();
  ~svtkDummyCommunicator() override;

private:
  svtkDummyCommunicator(const svtkDummyCommunicator&) = delete;
  void operator=(const svtkDummyCommunicator&) = delete;
};

#endif // svtkDummyCommunicator_h
