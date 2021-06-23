/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDataEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkDataEncoder
 * @brief   class used to compress/encode images using threads.
 *
 * svtkDataEncoder is used to compress and encode images using threads.
 * Multiple images can be pushed into the encoder for compression and encoding.
 * We use a svtkTypeUInt32 as the key to identify different image pipes. The
 * images in each pipe will be processed in parallel threads. The latest
 * compressed and encoded image can be accessed using GetLatestOutput().
 *
 * svtkDataEncoder uses a thread-pool to do the compression and encoding in
 * parallel.  Note that images may not come out of the svtkDataEncoder in the
 * same order as they are pushed in, if an image pushed in at N-th location
 * takes longer to compress and encode than that pushed in at N+1-th location or
 * if it was pushed in before the N-th location was even taken up for encoding
 * by the a thread in the thread pool.
 */

#ifndef svtkDataEncoder_h
#define svtkDataEncoder_h

#include "svtkObject.h"
#include "svtkSmartPointer.h"  // needed for svtkSmartPointer
#include "svtkWebCoreModule.h" // needed for exports

class svtkUnsignedCharArray;
class svtkImageData;

class SVTKWEBCORE_EXPORT svtkDataEncoder : public svtkObject
{
public:
  static svtkDataEncoder* New();
  svtkTypeMacro(svtkDataEncoder, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Define the number of worker threads to use.
   * Initialize() needs to be called after changing the thread count.
   */
  void SetMaxThreads(svtkTypeUInt32);
  svtkGetMacro(MaxThreads, svtkTypeUInt32);

  /**
   * Re-initializes the encoder. This will abort any on going encoding threads
   * and clear internal data-structures.
   */
  void Initialize();

  /**
   * Push an image into the encoder. It is not safe to modify the image
   * after this point, including changing the reference counts for it.
   * You may run into thread safety issues. Typically,
   * the caller code will simply release reference to the data and stop using
   * it. svtkDataEncoder takes over the reference for the image and will call
   * svtkObject::UnRegister() on it when it's done.
   * encoding can be set to 0 to skip encoding.
   */
  void PushAndTakeReference(svtkTypeUInt32 key, svtkImageData*& data, int quality, int encoding = 1);

  /**
   * Get access to the most-recent fully encoded result corresponding to the
   * given key, if any. This methods returns true if the \c data obtained is the
   * result from the most recent Push() for the key, if any. If this method
   * returns false, it means that there's some image either being processed on
   * pending processing.
   */
  bool GetLatestOutput(svtkTypeUInt32 key, svtkSmartPointer<svtkUnsignedCharArray>& data);

  /**
   * Flushes the encoding pipe and blocks till the most recently pushed image
   * for the particular key has been processed. This call will block. Once this
   * method returns, caller can use GetLatestOutput(key) to access the processed
   * output.
   */
  void Flush(svtkTypeUInt32 key);

  /**
   * Take an image data and synchronously convert it to a base-64 encoded png.
   */
  const char* EncodeAsBase64Png(svtkImageData* img, int compressionLevel = 5);

  /**
   * Take an image data and synchronously convert it to a base-64 encoded jpg.
   */
  const char* EncodeAsBase64Jpg(svtkImageData* img, int quality = 50);

  /**
   * This method will wait for any running thread to terminate.
   */
  void Finalize();

protected:
  svtkDataEncoder();
  ~svtkDataEncoder() override;

  svtkTypeUInt32 MaxThreads;

private:
  svtkDataEncoder(const svtkDataEncoder&) = delete;
  void operator=(const svtkDataEncoder&) = delete;

  class svtkInternals;
  svtkInternals* Internals;
};

#endif
