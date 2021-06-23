/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkGPUInfoList.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkGPUInfoList
 * @brief   Stores the list of GPUs VRAM information.
 *
 * svtkGPUInfoList stores a list of svtkGPUInfo. An host can have
 * several GPUs. It creates and sets the list by probing the host with system
 * calls. This an abstract class. Concrete classes are OS specific.
 * @sa
 * svtkGPUInfo svtkDirectXGPUInfoList svtkCoreGraphicsGPUInfoList
 */

#ifndef svtkGPUInfoList_h
#define svtkGPUInfoList_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkGPUInfoListArray; // STL Pimpl
class svtkGPUInfo;

class SVTKRENDERINGCORE_EXPORT svtkGPUInfoList : public svtkObject
{
public:
  static svtkGPUInfoList* New();
  svtkTypeMacro(svtkGPUInfoList, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Build the list of svtkInfoGPU if not done yet.
   * Default implementation created an empty list. Useful if there is no
   * implementation available for a given architecture yet.
   * \post probed: IsProbed()
   */
  virtual void Probe() = 0;

  /**
   * Tells if the operating system has been probed. Initial value is false.
   */
  virtual bool IsProbed();

  /**
   * Return the number of GPUs.
   * \pre probed: IsProbed()
   */
  virtual int GetNumberOfGPUs();

  /**
   * Return information about GPU i.
   * \pre probed: IsProbed()
   * \pre valid_index: i>=0 && i<GetNumberOfGPUs()
   * \post result_exists: result!=0
   */
  virtual svtkGPUInfo* GetGPUInfo(int i);

protected:
  //@{
  /**
   * Default constructor. Set Probed to false. Set Array to NULL.
   */
  svtkGPUInfoList();
  ~svtkGPUInfoList() override;
  //@}

  bool Probed;
  svtkGPUInfoListArray* Array;

private:
  svtkGPUInfoList(const svtkGPUInfoList&) = delete;
  void operator=(const svtkGPUInfoList&) = delete;
};

#endif
