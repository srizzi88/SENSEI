/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompressCompositer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompressCompositer
 * @brief   Implements compressed tree based compositing.
 *
 *
 * svtkCompressCompositer operates in multiple processes.  Each compositer has
 * a render window.  They use svtkMultiProcessController to communicate
 * the color and depth buffer to process 0's render window.
 * It will not handle transparency.  Compositing is run length encoding
 * of background pixels.
 *
 * SECTION See Also
 * svtkCompositeManager.
 */

#ifndef svtkCompressCompositer_h
#define svtkCompressCompositer_h

#include "svtkCompositer.h"
#include "svtkRenderingParallelModule.h" // For export macro

class svtkTimerLog;
class svtkDataArray;
class svtkFloatArray;

class SVTKRENDERINGPARALLEL_EXPORT svtkCompressCompositer : public svtkCompositer
{
public:
  static svtkCompressCompositer* New();
  svtkTypeMacro(svtkCompressCompositer, svtkCompositer);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void CompositeBuffer(
    svtkDataArray* pBuf, svtkFloatArray* zBuf, svtkDataArray* pTmp, svtkFloatArray* zTmp) override;

  /**
   * I am granting access to these methods and making them static
   * So I can create a TileDisplayCompositer which uses compression.
   */
  static void Compress(
    svtkFloatArray* zIn, svtkDataArray* pIn, svtkFloatArray* zOut, svtkDataArray* pOut);

  static void Uncompress(svtkFloatArray* zIn, svtkDataArray* pIn, svtkFloatArray* zOut,
    svtkDataArray* pOut, int finalLength);

  static void CompositeImagePair(svtkFloatArray* localZ, svtkDataArray* localP,
    svtkFloatArray* remoteZ, svtkDataArray* remoteP, svtkFloatArray* outZ, svtkDataArray* outP);

protected:
  svtkCompressCompositer();
  ~svtkCompressCompositer() override;

  svtkDataArray* InternalPData;
  svtkFloatArray* InternalZData;

  svtkTimerLog* Timer;

private:
  svtkCompressCompositer(const svtkCompressCompositer&) = delete;
  void operator=(const svtkCompressCompositer&) = delete;
};

#endif
