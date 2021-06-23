/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkMILVideoSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkMILVideoSource
 * @brief   Matrox Imaging Library frame grabbers
 *
 * svtkMILVideoSource provides an interface to Matrox Meteor, MeteorII
 * and Corona video digitizers through the Matrox Imaging Library
 * interface.  In order to use this class, you must link SVTK with mil.lib,
 * MIL version 5.0 or higher is required.
 * @warning
 * With some capture cards, if this class is leaked and ReleaseSystemResources
 * is not called, you may have to reboot before you can capture again.
 * svtkVideoSource used to keep a global list and delete the video sources
 * if your program leaked, due to exit crashes that was removed.
 * @sa
 * svtkWin32VideoSource svtkVideoSource
 */

#ifndef svtkMILVideoSource_h
#define svtkMILVideoSource_h

#include "svtkIOVideoModule.h" // For export macro
#include "svtkVideoSource.h"

// digitizer hardware
#define SVTK_MIL_DEFAULT 0
#define SVTK_MIL_METEOR "M_SYSTEM_METEOR"
#define SVTK_MIL_METEOR_II "M_SYSTEM_METEOR_II"
#define SVTK_MIL_METEOR_II_DIG "M_SYSTEM_METEOR_II_DIG"
#define SVTK_MIL_METEOR_II_CL "M_SYSTEM_METEOR_II_CL"
#define SVTK_MIL_METEOR_II_1394 "M_SYSTEM_METEOR_II_1394"
#define SVTK_MIL_CORONA "M_SYSTEM_CORONA"
#define SVTK_MIL_CORONA_II "M_SYSTEM_CORONA_II"
#define SVTK_MIL_PULSAR "M_SYSTEM_PULSAR"
#define SVTK_MIL_GENESIS "M_SYSTEM_GENESIS"
#define SVTK_MIL_GENESIS_PLUS "M_SYSTEM_GENESIS_PLUS"
#define SVTK_MIL_ORION "M_SYSTEM_ORION"
#define SVTK_MIL_CRONOS "M_SYSTEM_CRONOS"
#define SVTK_MIL_ODYSSEY "M_SYSTEM_ODYSSEY"

// video inputs:
#define SVTK_MIL_MONO 0
#define SVTK_MIL_COMPOSITE 1
#define SVTK_MIL_YC 2
#define SVTK_MIL_RGB 3
#define SVTK_MIL_DIGITAL 4

// video formats:
#define SVTK_MIL_RS170 0
#define SVTK_MIL_NTSC 1
#define SVTK_MIL_CCIR 2
#define SVTK_MIL_PAL 3
#define SVTK_MIL_SECAM 4
#define SVTK_MIL_NONSTANDARD 5

class SVTKIOVIDEO_EXPORT svtkMILVideoSource : public svtkVideoSource
{
public:
  static svtkMILVideoSource* New();
  svtkTypeMacro(svtkMILVideoSource, svtkVideoSource);
  void PrintSelf(ostream& os, svtkIndent indent);

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

  /**
   * Request a particular frame size (set the third value to 1).
   */
  void SetFrameSize(int x, int y, int z) override;

  /**
   * Request a particular output format (default: SVTK_RGB).
   */
  void SetOutputFormat(int format) override;

  //@{
  /**
   * Set/Get the video channel
   */
  virtual void SetVideoChannel(int channel);
  svtkGetMacro(VideoChannel, int);
  //@}

  //@{
  /**
   * Set/Get the video format
   */
  virtual void SetVideoFormat(int format);
  void SetVideoFormatToNTSC() { this->SetVideoFormat(SVTK_MIL_NTSC); }
  void SetVideoFormatToPAL() { this->SetVideoFormat(SVTK_MIL_PAL); }
  void SetVideoFormatToSECAM() { this->SetVideoFormat(SVTK_MIL_SECAM); }
  void SetVideoFormatToRS170() { this->SetVideoFormat(SVTK_MIL_RS170); }
  void SetVideoFormatToCCIR() { this->SetVideoFormat(SVTK_MIL_CCIR); }
  void SetVideoFormatToNonStandard() { this->SetVideoFormat(SVTK_MIL_NONSTANDARD); }
  svtkGetMacro(VideoFormat, int);
  //@}

  //@{
  /**
   * Set/Get the video input
   */
  virtual void SetVideoInput(int input);
  void SetVideoInputToMono() { this->SetVideoInput(SVTK_MIL_MONO); }
  void SetVideoInputToComposite() { this->SetVideoInput(SVTK_MIL_COMPOSITE); }
  void SetVideoInputToYC() { this->SetVideoInput(SVTK_MIL_YC); }
  void SetVideoInputToRGB() { this->SetVideoInput(SVTK_MIL_RGB); }
  void SetVideoInputToDigital() { this->SetVideoInput(SVTK_MIL_DIGITAL); }
  svtkGetMacro(VideoInput, int);
  //@}

  //@{
  /**
   * Set/Get the video levels for composite/SVideo: the valid ranges are:
   * Contrast [0.0,2.0]
   * Brightness [0.0,255.0]
   * Hue [-0.5,0.5]
   * Saturation [0.0,2.0]
   */
  virtual void SetContrastLevel(float contrast);
  svtkGetMacro(ContrastLevel, float);
  virtual void SetBrightnessLevel(float brightness);
  svtkGetMacro(BrightnessLevel, float);
  virtual void SetHueLevel(float hue);
  svtkGetMacro(HueLevel, float);
  virtual void SetSaturationLevel(float saturation);
  svtkGetMacro(SaturationLevel, float);
  //@}

  //@{
  /**
   * Set/Get the video levels for monochrome/RGB: valid values are
   * between 0.0 and 255.0.
   */
  virtual void SetBlackLevel(float value);
  virtual float GetBlackLevel() { return this->BlackLevel; }
  virtual void SetWhiteLevel(float value);
  virtual float GetWhiteLevel() { return this->WhiteLevel; }
  //@}

  //@{
  /**
   * Set the system which you want use.  If you don't specify a system,
   * then your primary digitizer will be autodetected.
   */
  svtkSetStringMacro(MILSystemType);
  svtkGetStringMacro(MILSystemType);
  void SetMILSystemTypeToMeteor() { this->SetMILSystemType(SVTK_MIL_METEOR); }
  void SetMILSystemTypeToMeteorII() { this->SetMILSystemType(SVTK_MIL_METEOR_II); }
  void SetMILSystemTypeToMeteorIIDig() { this->SetMILSystemType(SVTK_MIL_METEOR_II_DIG); }
  void SetMILSystemTypeToMeteorIICL() { this->SetMILSystemType(SVTK_MIL_METEOR_II_CL); }
  void SetMILSystemTypeToMeteorII1394() { this->SetMILSystemType(SVTK_MIL_METEOR_II_1394); }
  void SetMILSystemTypeToCorona() { this->SetMILSystemType(SVTK_MIL_CORONA); }
  void SetMILSystemTypeToCoronaII() { this->SetMILSystemType(SVTK_MIL_CORONA_II); }
  void SetMILSystemTypeToPulsar() { this->SetMILSystemType(SVTK_MIL_PULSAR); }
  void SetMILSystemTypeToGenesis() { this->SetMILSystemType(SVTK_MIL_GENESIS); }
  void SetMILSystemTypeToGenesisPlus() { this->SetMILSystemType(SVTK_MIL_GENESIS_PLUS); }
  void SetMILSystemTypeToOrion() { this->SetMILSystemType(SVTK_MIL_ORION); }
  void SetMILSystemTypeToCronos() { this->SetMILSystemType(SVTK_MIL_CRONOS); }
  void SetMILSystemTypeToOdyssey() { this->SetMILSystemType(SVTK_MIL_ODYSSEY); }
  //@}
  //@{
  /**
   * Set the system number if you have multiple systems of the same type
   */
  svtkSetMacro(MILSystemNumber, int);
  svtkGetMacro(MILSystemNumber, int);
  //@}

  //@{
  /**
   * Set the DCF filename for non-standard video formats
   */
  svtkSetStringMacro(MILDigitizerDCF);
  svtkGetStringMacro(MILDigitizerDCF);
  //@}

  //@{
  /**
   * Set the digitizer number for systems with multiple digitizers
   */
  svtkSetMacro(MILDigitizerNumber, int);
  svtkGetMacro(MILDigitizerNumber, int);
  //@}

  //@{
  /**
   * Set whether to display MIL error messages (default on)
   */
  virtual void SetMILErrorMessages(int yesno);
  svtkBooleanMacro(MILErrorMessages, int);
  svtkGetMacro(MILErrorMessages, int);
  //@}

  //@{
  /**
   * Allows fine-grained control
   */
  svtkSetMacro(MILAppID, long);
  svtkGetMacro(MILAppID, long);
  svtkSetMacro(MILSysID, long);
  svtkGetMacro(MILSysID, long);
  svtkGetMacro(MILDigID, long);
  svtkGetMacro(MILBufID, long);
  //@}

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
   * For internal use only
   */
  void* OldHookFunction;
  void* OldUserDataPtr;
  int FrameCounter;
  int ForceGrab;
  void InternalGrab() override;
  //@}

protected:
  svtkMILVideoSource();
  ~svtkMILVideoSource() override;

  virtual void AllocateMILDigitizer();
  virtual void AllocateMILBuffer();

  virtual char* MILInterpreterForSystem(const char* system);
  char* MILInterpreterDLL;

  int VideoChannel;
  int VideoInput;
  int VideoInputForColor;
  int VideoFormat;

  float ContrastLevel;
  float BrightnessLevel;
  float HueLevel;
  float SaturationLevel;

  float BlackLevel;
  float WhiteLevel;

  int FrameMaxSize[2];

  long MILAppID;
  long MILSysID;
  long MILDigID;
  long MILBufID;
  // long MILDispBufID;
  // long MILDispID;

  char* MILSystemType;
  int MILSystemNumber;

  int MILDigitizerNumber;
  char* MILDigitizerDCF;

  int MILErrorMessages;

  int MILAppInternallyAllocated;
  int MILSysInternallyAllocated;

  int FatalMILError;

  /**
   * Method for updating the virtual clock that accurately times the
   * arrival of each frame, more accurately than is possible with
   * the system clock alone because the virtual clock averages out the
   * jitter.
   */
  double CreateTimeStampForFrame(unsigned long frame);

  double LastTimeStamp;
  unsigned long LastFrameCount;
  double EstimatedFramePeriod;
  double NextFramePeriod;

private:
  svtkMILVideoSource(const svtkMILVideoSource&) = delete;
  void operator=(const svtkMILVideoSource&) = delete;
};

#endif
