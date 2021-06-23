/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWebApplication.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWebApplication
 * @brief   defines ParaViewWeb application interface.
 *
 * svtkWebApplication defines the core interface for a ParaViewWeb application.
 * This exposes methods that make it easier to manage views and rendered images
 * from views.
 */

#ifndef svtkWebApplication_h
#define svtkWebApplication_h

#include "svtkObject.h"
#include "svtkWebCoreModule.h" // needed for exports
#include <string>             // needed for std::string

class svtkObjectIdMap;
class svtkRenderWindow;
class svtkUnsignedCharArray;
class svtkWebInteractionEvent;

class SVTKWEBCORE_EXPORT svtkWebApplication : public svtkObject
{
public:
  static svtkWebApplication* New();
  svtkTypeMacro(svtkWebApplication, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set the encoding to be used for rendered images.
   */
  enum
  {
    ENCODING_NONE = 0,
    ENCODING_BASE64 = 1
  };
  svtkSetClampMacro(ImageEncoding, int, ENCODING_NONE, ENCODING_BASE64);
  svtkGetMacro(ImageEncoding, int);
  //@}

  //@{
  /**
   * Set the compression to be used for rendered images.
   */
  enum
  {
    COMPRESSION_NONE = 0,
    COMPRESSION_PNG = 1,
    COMPRESSION_JPEG = 2
  };
  svtkSetClampMacro(ImageCompression, int, COMPRESSION_NONE, COMPRESSION_JPEG);
  svtkGetMacro(ImageCompression, int);
  //@}

  //@{
  /**
   * Set the number of worker threads to use for image encoding.  Calling this
   * method with a number greater than 32 or less than zero will have no effect.
   */
  void SetNumberOfEncoderThreads(svtkTypeUInt32);
  svtkTypeUInt32 GetNumberOfEncoderThreads();
  //@}

  //@{
  /**
   * Render a view and obtain the rendered image.
   */
  svtkUnsignedCharArray* StillRender(svtkRenderWindow* view, int quality = 100);
  svtkUnsignedCharArray* InteractiveRender(svtkRenderWindow* view, int quality = 50);
  const char* StillRenderToString(svtkRenderWindow* view, svtkMTimeType time = 0, int quality = 100);
  svtkUnsignedCharArray* StillRenderToBuffer(
    svtkRenderWindow* view, svtkMTimeType time = 0, int quality = 100);
  //@}

  /**
   * StillRenderToString() need not necessary returns the most recently rendered
   * image. Use this method to get whether there are any pending images being
   * processed concurrently.
   */
  bool GetHasImagesBeingProcessed(svtkRenderWindow*);

  /**
   * Communicate mouse interaction to a view.
   * Returns true if the interaction changed the view state, otherwise returns false.
   */
  bool HandleInteractionEvent(svtkRenderWindow* view, svtkWebInteractionEvent* event);

  /**
   * Invalidate view cache
   */
  void InvalidateCache(svtkRenderWindow* view);

  //@{
  /**
   * Return the MTime of the last array exported by StillRenderToString.
   */
  svtkGetMacro(LastStillRenderToMTime, svtkMTimeType);
  //@}

  /**
   * Return the Meta data description of the input scene in JSON format.
   * This is using the svtkWebGLExporter to parse the scene.
   * NOTE: This should be called before getting the webGL binary data.
   */
  const char* GetWebGLSceneMetaData(svtkRenderWindow* view);

  /**
   * Return the binary data given the part index
   * and the webGL object piece id in the scene.
   */
  const char* GetWebGLBinaryData(svtkRenderWindow* view, const char* id, int partIndex);

  svtkObjectIdMap* GetObjectIdMap();

  /**
   * Return a hexadecimal formatted string of the SVTK object's memory address,
   * useful for uniquely identifying the object when exporting data.
   *
   * e.g. 0x8f05a90
   */
  static std::string GetObjectId(svtkObject* obj);

protected:
  svtkWebApplication();
  ~svtkWebApplication() override;

  int ImageEncoding;
  int ImageCompression;
  svtkMTimeType LastStillRenderToMTime;

private:
  svtkWebApplication(const svtkWebApplication&) = delete;
  void operator=(const svtkWebApplication&) = delete;

  class svtkInternals;
  svtkInternals* Internals;
};

#endif
