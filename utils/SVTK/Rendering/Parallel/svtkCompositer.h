/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkCompositer.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkCompositer
 * @brief   Super class for composite algorthms.
 *
 *
 * svtkCompositer operates in multiple processes.  Each compositer has
 * a render window.  They use svtkMultiProcessControllers to communicate
 * the color and depth buffer to process 0's render window.
 * It will not handle transparency well.
 *
 * @sa
 * svtkCompositeManager.
 */

#ifndef svtkCompositer_h
#define svtkCompositer_h

#include "svtkObject.h"
#include "svtkRenderingParallelModule.h" // For export macro

class svtkMultiProcessController;
class svtkCompositer;
class svtkDataArray;
class svtkFloatArray;
class svtkUnsignedCharArray;

class SVTKRENDERINGPARALLEL_EXPORT svtkCompositer : public svtkObject
{
public:
  static svtkCompositer* New();
  svtkTypeMacro(svtkCompositer, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * This method gets called on every process.  The final image gets
   * put into pBuf and zBuf.
   */
  virtual void CompositeBuffer(
    svtkDataArray* pBuf, svtkFloatArray* zBuf, svtkDataArray* pTmp, svtkFloatArray* zTmp);

  //@{
  /**
   * Access to the controller.
   */
  virtual void SetController(svtkMultiProcessController*);
  svtkGetObjectMacro(Controller, svtkMultiProcessController);
  //@}

  //@{
  /**
   * A hack to get a sub world until I can get communicators working.
   */
  svtkSetMacro(NumberOfProcesses, int);
  svtkGetMacro(NumberOfProcesses, int);
  //@}

  //@{
  /**
   * Methods that allocate and delete memory with special MPIPro calls.
   */
  static void DeleteArray(svtkDataArray* da);
  static void ResizeFloatArray(svtkFloatArray* fa, int numComp, svtkIdType size);
  static void ResizeUnsignedCharArray(svtkUnsignedCharArray* uca, int numComp, svtkIdType size);
  //@}

protected:
  svtkCompositer();
  ~svtkCompositer() override;

  svtkMultiProcessController* Controller;
  int NumberOfProcesses;

private:
  svtkCompositer(const svtkCompositer&) = delete;
  void operator=(const svtkCompositer&) = delete;
};

#endif
