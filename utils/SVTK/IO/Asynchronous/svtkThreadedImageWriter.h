/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkThreadedImageWriter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class    svtkThreadedImageWriter
 * @brief    class used to compress/write images using threads to prevent
 *           locking while encoding data.
 *
 * @details  This writer allow to encode an image data based on its file
 *           extension: tif, tiff, bpm, png, jpg, jpeg, vti, Z, ppm, raw
 *
 * @author   Patricia Kroll Fasel @ LANL
 */

#ifndef svtkThreadedImageWriter_h
#define svtkThreadedImageWriter_h

#include "svtkIOAsynchronousModule.h" // For export macro
#include "svtkObject.h"

class svtkImageData;

class SVTKIOASYNCHRONOUS_EXPORT svtkThreadedImageWriter : public svtkObject
{
public:
  static svtkThreadedImageWriter* New();
  svtkTypeMacro(svtkThreadedImageWriter, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Need to be called at least once before using the class.
   * Then it should be called again after any change on the
   * thread count or if Finalize() was called.
   *
   * This method will wait for any running thread to terminate and start
   * a new pool with the given number of threads.
   */
  void Initialize();

  /**
   * Push an image into the threaded writer. It is not safe to modify the image
   * after this point.
   * You may run into thread safety issues. Typically, the caller code will
   * simply release reference to the data and stop using it.
   */
  void EncodeAndWrite(svtkImageData* image, const char* fileName);

  /**
   * Define the number of worker thread to use.
   * Initialize() need to be called after any thread count change.
   */
  void SetMaxThreads(svtkTypeUInt32);
  svtkGetMacro(MaxThreads, svtkTypeUInt32);

  /**
   * This method will wait for any running thread to terminate.
   */
  void Finalize();

protected:
  svtkThreadedImageWriter();
  ~svtkThreadedImageWriter() override;

private:
  svtkThreadedImageWriter(const svtkThreadedImageWriter&) = delete;
  void operator=(const svtkThreadedImageWriter&) = delete;

  class svtkInternals;
  svtkInternals* Internals;
  svtkTypeUInt32 MaxThreads;
};

#endif
