/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTreeCompositer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

// This software and ancillary information known as svtk_ext (and
// herein called "SOFTWARE") is made available under the terms
// described below.  The SOFTWARE has been approved for release with
// associated LA_CC Number 99-44, granted by Los Alamos National
// Laboratory in July 1999.
//
// Unless otherwise indicated, this SOFTWARE has been authored by an
// employee or employees of the University of California, operator of
// the Los Alamos National Laboratory under Contract No. W-7405-ENG-36
// with the United States Department of Energy.
//
// The United States Government has rights to use, reproduce, and
// distribute this SOFTWARE.  The public may copy, distribute, prepare
// derivative works and publicly display this SOFTWARE without charge,
// provided that this Notice and any statement of authorship are
// reproduced on all copies.
//
// Neither the U. S. Government, the University of California, nor the
// Advanced Computing Laboratory makes any warranty, either express or
// implied, nor assumes any liability or responsibility for the use of
// this SOFTWARE.
//
// If SOFTWARE is modified to produce derivative works, such modified
// SOFTWARE should be clearly marked, so as not to confuse it with the
// version available from Los Alamos National Laboratory.

/**
 * @class   svtkTreeCompositer
 * @brief   Implements tree based compositing.
 *
 *
 * svtkTreeCompositer operates in multiple processes.  Each compositer has
 * a render window.  They use a svtkMultiProcessController to communicate
 * the color and depth buffer to process 0's render window.
 * It will not handle transparency well.
 *
 * @sa
 * svtkCompositeManager
 */

#ifndef svtkTreeCompositer_h
#define svtkTreeCompositer_h

#include "svtkCompositer.h"
#include "svtkRenderingParallelModule.h" // For export macro

class SVTKRENDERINGPARALLEL_EXPORT svtkTreeCompositer : public svtkCompositer
{
public:
  static svtkTreeCompositer* New();
  svtkTypeMacro(svtkTreeCompositer, svtkCompositer);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void CompositeBuffer(
    svtkDataArray* pBuf, svtkFloatArray* zBuf, svtkDataArray* pTmp, svtkFloatArray* zTmp) override;

protected:
  svtkTreeCompositer();
  ~svtkTreeCompositer() override;

private:
  svtkTreeCompositer(const svtkTreeCompositer&) = delete;
  void operator=(const svtkTreeCompositer&) = delete;
};

#endif
