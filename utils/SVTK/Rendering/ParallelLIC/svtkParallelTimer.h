/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParallelTimer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
//
/**
 * @class   svtkParallelTimer
 *
 *
 *  Provides distributed log functionality. When the file is
 *  written each process data is collected by rank 0 who
 *  writes the data to a single file in rank order.
 *
 *  The log works as an event stack. EventStart pushes the
 *  event identifier and its start time onto the stack. EventEnd
 *  pops the most recent event time and identifier computes the
 *  elapsed time and adds an entry to the log recording the
 *  event, it's start and end times, and its elapsed time.
 *  EndEventSynch includes a barrier before the measurement.
 *
 *  The log class implements the singleton pattern so that it
 *  may be shared across class boundaries. If the log instance
 *  doesn't exist then one is created. It will be automatically
 *  destroyed at exit by the signleton destructor. It can be
 *  destroyed explicitly by calling DeleteGlobalInstance.
 */

#ifndef svtkParallelTimer_h
#define svtkParallelTimer_h

#define svtkParallelTimerDEBUG -1

#include "svtkObject.h"
#include "svtkRenderingParallelLICModule.h" // for export

#include <sstream> // for sstream
#include <string>  // for string
#include <vector>  // for vector
#if svtkParallelTimerDEBUG > 0
#include <iostream> // for cerr
#endif

class svtkParallelTimerBuffer;

class SVTKRENDERINGPARALLELLIC_EXPORT svtkParallelTimer : public svtkObject
{
public:
  static svtkParallelTimer* New();
  svtkTypeMacro(svtkParallelTimer, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Type used to direct an output stream into the log's header. The header
   * is a buffer used only by the root rank.
   */
  class LogHeaderType
  {
  public:
    template <typename T>
    LogHeaderType& operator<<(const T& s);
  };
  //@}

  //@{
  /**
   * Type used to direct an output stream into the log's body. The body is a
   * buffer that all ranks write to.
   */
  class LogBodyType
  {
  public:
    template <typename T>
    LogBodyType& operator<<(const T& s);
  };
  //@}

  //@{
  /**
   * Set the rank who writes.
   */
  svtkSetMacro(WriterRank, int);
  svtkGetMacro(WriterRank, int);
  //@}

  //@{
  /**
   * Set the filename that is used during write when the object
   * is used as a singleton. If nothing is set the default is
   * ROOT_RANKS_PID.log
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  void SetFileName(const std::string& fileName) { this->SetFileName(fileName.c_str()); }

  //@{
  /**
   * The log works as an event stack. EventStart pushes the
   * event identifier and its start time onto the stack. EventEnd
   * pops the most recent event time and identifier computes the
   * elapsed time and adds an entry to the log recording the
   * event, it's start and end times, and its elapsed time.
   * EndEventSynch includes a barrier before the measurement.
   */
  void StartEvent(const char* event);
  void StartEvent(int rank, const char* event);
  void EndEvent(const char* event);
  void EndEvent(int rank, const char* event);
  void EndEventSynch(const char* event);
  void EndEventSynch(int rank, const char* event);
  //@}

  /**
   * Insert text into the log header on the writer rank.
   */
  template <typename T>
  svtkParallelTimer& operator<<(const T& s);

  /**
   * stream output to the log's header(root rank only).
   */
  svtkParallelTimer::LogHeaderType GetHeader() { return svtkParallelTimer::LogHeaderType(); }

  /**
   * stream output to log body(all ranks).
   */
  svtkParallelTimer::LogBodyType GetBody() { return svtkParallelTimer::LogBodyType(); }

  /**
   * Clear the log.
   */
  void Clear();

  /**
   * When an object is finished writing data to the log
   * object it must call Update to send the data to the writer
   * rank.
   * This ensures that all data is transferred to the root before
   * MPI_Finalize is called while allowing the write to occur
   * after Mpi_finalize. Note: This is a collective call.
   */
  void Update();

  /**
   * Write the log contents to a file.
   */
  int Write();

  /**
   * The log class implements the singleton pattern so that it
   * may be shared across class boundaries. If the log instance
   * doesn't exist then one is created. It will be automatically
   * destroyed at exit by the signleton destructor. It can be
   * destroyed explicitly by calling DeleteGlobalInstance.
   */
  static svtkParallelTimer* GetGlobalInstance();

  /**
   * Explicitly delete the singleton.
   */
  static void DeleteGlobalInstance();

  //@{
  /**
   * If enabled and used as a singleton the log will write
   * it's contents to disk during program termination.
   */
  svtkSetMacro(WriteOnClose, int);
  svtkGetMacro(WriteOnClose, int);
  //@}

  //@{
  /**
   * Set/Get the global log level. Applications can set this to the
   * desired level so that all pipeline objects will log data.
   */
  svtkSetMacro(GlobalLevel, int);
  svtkGetMacro(GlobalLevel, int);
  //@}

protected:
  svtkParallelTimer();
  virtual ~svtkParallelTimer();

private:
  svtkParallelTimer(const svtkParallelTimer&) = delete;
  void operator=(const svtkParallelTimer&) = delete;

  /**
   * A class responsible for delete'ing the global instance of the log.
   */
  class SVTKRENDERINGPARALLELLIC_EXPORT svtkParallelTimerDestructor
  {
  public:
    svtkParallelTimerDestructor()
      : Log(0)
    {
    }
    ~svtkParallelTimerDestructor();

    void SetLog(svtkParallelTimer* log) { this->Log = log; }

  private:
    svtkParallelTimer* Log;
  };

private:
  int GlobalLevel;
  int Initialized;
  int WorldRank;
  int WriterRank;
  char* FileName;
  int WriteOnClose;
  std::vector<double> StartTime;
#if svtkParallelTimerDEBUG < 0
  std::vector<std::string> EventId;
#endif

  svtkParallelTimerBuffer* Log;

  static svtkParallelTimer* GlobalInstance;
  static svtkParallelTimerDestructor GlobalInstanceDestructor;

  std::ostringstream HeaderBuffer;

  friend class LogHeaderType;
  friend class LogBodyType;
};

//-----------------------------------------------------------------------------
template <typename T>
svtkParallelTimer& svtkParallelTimer::operator<<(const T& s)
{
  if (this->WorldRank == this->WriterRank)
  {
    this->HeaderBuffer << s;
#if svtkParallelTimerDEBUG > 0
    std::cerr << s;
#endif
  }
  return *this;
}

//-----------------------------------------------------------------------------
template <typename T>
svtkParallelTimer::LogHeaderType& svtkParallelTimer::LogHeaderType::operator<<(const T& s)
{
  svtkParallelTimer* log = svtkParallelTimer::GetGlobalInstance();

  if (log->WorldRank == log->WriterRank)
  {
    log->HeaderBuffer << s;
#if svtkParallelTimerDEBUG > 0
    std::cerr << s;
#endif
  }

  return *this;
}

//-----------------------------------------------------------------------------
template <typename T>
svtkParallelTimer::LogBodyType& svtkParallelTimer::LogBodyType::operator<<(const T& s)
{
  svtkParallelTimer* log = svtkParallelTimer::GetGlobalInstance();

  *(log->Log) << s;
#if svtkParallelTimerDEBUG > 0
  std::cerr << s;
#endif

  return *this;
}

#endif
