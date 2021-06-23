/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkDummyGPUInfoList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkDummyGPUInfoList
 * @brief   Do thing during Probe()
 *
 * svtkDummyGPUInfoList implements Probe() by just setting the count of
 * GPUs to be zero. Useful when an OS specific implementation is not available.
 * @sa
 * svtkGPUInfo svtkGPUInfoList
 */

#ifndef svtkDummyGPUInfoList_h
#define svtkDummyGPUInfoList_h

#include "svtkGPUInfoList.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class SVTKRENDERINGOPENGL2_EXPORT svtkDummyGPUInfoList : public svtkGPUInfoList
{
public:
  static svtkDummyGPUInfoList* New();
  svtkTypeMacro(svtkDummyGPUInfoList, svtkGPUInfoList);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Build the list of svtkInfoGPU if not done yet.
   * \post probed: IsProbed()
   */
  void Probe() override;

protected:
  //@{
  /**
   * Default constructor.
   */
  svtkDummyGPUInfoList();
  ~svtkDummyGPUInfoList() override;
  //@}

private:
  svtkDummyGPUInfoList(const svtkDummyGPUInfoList&) = delete;
  void operator=(const svtkDummyGPUInfoList&) = delete;
};

#endif
