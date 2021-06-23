/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFFMPEGVideoSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkFFMPEGVideoSource
 * @brief   Reader for ffmpeg supported formats
 *
 * Note this this class make use of multiple threads when decoding files. It has
 * a feed thread, a video drain thread, and an audio drain thread. The decoding
 * may use multiple threads as well as specified by DecodingThreads ivar.
 *
 * @sa
 * svtkVideoSource
 */

#ifndef svtkFFMPEGVideoSource_h
#define svtkFFMPEGVideoSource_h

#include "svtkIOFFMPEGModule.h" // For export macro
#include "svtkMultiThreader.h"  // for ivar
#include "svtkNew.h"            // for ivar
#include "svtkVideoSource.h"
#include <functional> // for audio callback

class svtkFFMPEGVideoSourceInternal;

class svtkConditionVariable;
class svtkMutexLock;
class svtkFFMPEGVideoSource;

// audio callback struct, outside the class so that we
// can forward ref it
struct svtkFFMPEGVideoSourceAudioCallbackData
{
  int NumberOfSamples;
  int BytesPerSample;
  int NumberOfChannels;
  int SampleRate;
  int DataType;
  bool Packed;
  unsigned char** Data;
  svtkFFMPEGVideoSource* Caller;
  void* ClientData;
};

// video callback struct, outside the class so that we
// can forward ref it
struct svtkFFMPEGVideoSourceVideoCallbackData
{
  int Height;
  int LineSize[8];
  unsigned char* Data[8]; // nullptr for empty planes
  svtkFFMPEGVideoSource* Caller;
  void* ClientData;
};

class SVTKIOFFMPEG_EXPORT svtkFFMPEGVideoSource : public svtkVideoSource
{
public:
  static svtkFFMPEGVideoSource* New();
  svtkTypeMacro(svtkFFMPEGVideoSource, svtkVideoSource);

  /**
   * Standard VCR functionality: Record incoming video.
   */
  void Record() override;

  /**
   * Standard VCR functionality: Play recorded video.
   */
  void Play() override;

  /**
   * Standard VCR functionality: Stop recording or playing.
   */
  void Stop() override;

  /**
   * Grab a single video frame.
   */
  void Grab() override;

  //@{
  /**
   * Request a particular frame size (set the third value to 1).
   */
  void SetFrameSize(int x, int y, int z) override;
  void SetFrameSize(int dim[3]) override { this->SetFrameSize(dim[0], dim[1], dim[2]); }
  //@}

  /**
   * Request a particular frame rate (default 30 frames per second).
   */
  void SetFrameRate(float rate) override;

  /**
   * Request a particular output format (default: SVTK_RGB).
   */
  void SetOutputFormat(int format) override;

  /**
   * Initialize the driver (this is called automatically when the
   * first grab is done).
   */
  void Initialize() override;

  /**
   * Free the driver (this is called automatically inside the
   * destructor).
   */
  void ReleaseSystemResources() override;

  //@{
  /**
   * Specify file name of the video
   */
  svtkSetStringMacro(FileName);
  svtkGetStringMacro(FileName);
  //@}

  /**
   * The internal function which actually does the grab.  You will
   * definitely want to override this if you develop a svtkVideoSource
   * subclass.
   */
  void InternalGrab() override;

  // is the video at the end of file?
  // Useful for while loops
  svtkGetMacro(EndOfFile, bool);

  // Is the video stream stereo 3d
  svtkGetMacro(Stereo3D, bool);

  // we do not use Invoke Observers here because this callback
  // will happen in a different thread that could conflict
  // with events from other threads. In this function you should
  // not block the thread (for example waiting for audio to play)
  // instead you should have enough buffering that you can consume
  // the provided data and return. Typically even 1 second of
  // buffer storage is enough to prevent blocking.
  typedef std::function<void(svtkFFMPEGVideoSourceAudioCallbackData const& data)> AudioCallbackType;
  void SetAudioCallback(AudioCallbackType cb, void* clientData)
  {
    this->AudioCallback = cb;
    this->AudioCallbackClientData = clientData;
  }

  // we do not use Invoke Observers here because this callback
  // will happen in a different thread that could conflict
  // with events from other threads. In this function you should
  // not block the thread (for example waiting for video to play)
  // instead you should have enough buffering that you can consume
  // the provided data and return.
  typedef std::function<void(svtkFFMPEGVideoSourceVideoCallbackData const& data)> VideoCallbackType;
  void SetVideoCallback(VideoCallbackType cb, void* clientData)
  {
    this->VideoCallback = cb;
    this->VideoCallbackClientData = clientData;
  }

  //@{
  /**
   * How many threads to use for the decoding codec
   * this will be in addition to the feed and drain threads.
   * the default value is 4.
   */
  svtkSetMacro(DecodingThreads, int);
  svtkGetMacro(DecodingThreads, int);
  //@}

protected:
  svtkFFMPEGVideoSource();
  ~svtkFFMPEGVideoSource() override;

  AudioCallbackType AudioCallback;
  void* AudioCallbackClientData;

  int DecodingThreads;

  static void* DrainAudioThread(svtkMultiThreader::ThreadInfo* data);
  void* DrainAudio(svtkMultiThreader::ThreadInfo* data);
  int DrainAudioThreadId;

  static void* DrainThread(svtkMultiThreader::ThreadInfo* data);
  void* Drain(svtkMultiThreader::ThreadInfo* data);
  int DrainThreadId;

  bool EndOfFile;

  svtkNew<svtkConditionVariable> FeedCondition;
  svtkNew<svtkMutexLock> FeedMutex;
  svtkNew<svtkConditionVariable> FeedAudioCondition;
  svtkNew<svtkMutexLock> FeedAudioMutex;
  static void* FeedThread(svtkMultiThreader::ThreadInfo* data);
  void* Feed(svtkMultiThreader::ThreadInfo* data);
  int FeedThreadId;

  char* FileName;

  svtkFFMPEGVideoSourceInternal* Internal;

  void ReadFrame();

  bool Stereo3D;

  VideoCallbackType VideoCallback;
  void* VideoCallbackClientData;

private:
  svtkFFMPEGVideoSource(const svtkFFMPEGVideoSource&) = delete;
  void operator=(const svtkFFMPEGVideoSource&) = delete;
};

#endif
