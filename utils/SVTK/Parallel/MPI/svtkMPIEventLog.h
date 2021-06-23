/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMPIEventLog.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMPIEventLog
 * @brief   Class for logging and timing.
 *
 *
 * This class is wrapper around MPE event logging functions
 * (available from Argonne National Lab/Missippi State
 * University). It allows users to create events with names
 * and log them. Different log file formats can be generated
 * by changing MPE's configuration. Some of these formats are
 * binary (for examples SLOG and CLOG) and can be analyzed with
 * viewers from ANL. ALOG is particularly useful since it is
 * text based and can be processed with simple scripts.
 *
 * @sa
 * svtkTimerLog svtkMPIController svtkMPICommunicator
 */

#ifndef svtkMPIEventLog_h
#define svtkMPIEventLog_h

#include "svtkObject.h"
#include "svtkParallelMPIModule.h" // For export macro

class SVTKPARALLELMPI_EXPORT svtkMPIEventLog : public svtkObject
{
public:
  svtkTypeMacro(svtkMPIEventLog, svtkObject);

  /**
   * Construct a svtkMPIEventLog with the following initial state:
   * Processes = 0, MaximumNumberOfProcesses = 0.
   */
  static svtkMPIEventLog* New();

  /**
   * Used to initialize the underlying mpe event.
   * HAS TO BE CALLED BY ALL PROCESSES before any event
   * logging is done.
   * It takes a name and a description for the graphical
   * representation, for example, "red:vlines3". See
   * mpe documentation for details.
   * Returns 0 on MPI failure (or aborts depending on
   * MPI error handlers)
   */
  int SetDescription(const char* name, const char* desc);

  //@{
  /**
   * These methods have to be called once on all processors
   * before and after invoking any logging events.
   * The name of the logfile is given by fileName.
   * See mpe documentation for file formats.
   */
  static void InitializeLogging();
  static void FinalizeLogging(const char* fileName);
  //@}

  //@{
  /**
   * Issue start and stop events for this log entry.
   */
  void StartLogging();
  void StopLogging();
  //@}

  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkMPIEventLog();
  ~svtkMPIEventLog() override;

  static int LastEventId;
  int Active;
  int BeginId;
  int EndId;

private:
  svtkMPIEventLog(const svtkMPIEventLog&) = delete;
  void operator=(const svtkMPIEventLog&) = delete;
};

#endif
