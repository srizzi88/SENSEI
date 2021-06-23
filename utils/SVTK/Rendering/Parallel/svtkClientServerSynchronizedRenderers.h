/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkClientServerSynchronizedRenderers.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkClientServerSynchronizedRenderers
 *
 * svtkClientServerSynchronizedRenderers is a svtkSynchronizedRenderers subclass
 * designed to be used in 2 processes, client-server mode.
 */

#ifndef svtkClientServerSynchronizedRenderers_h
#define svtkClientServerSynchronizedRenderers_h

#include "svtkRenderingParallelModule.h" // For export macro
#include "svtkSynchronizedRenderers.h"

class SVTKRENDERINGPARALLEL_EXPORT svtkClientServerSynchronizedRenderers
  : public svtkSynchronizedRenderers
{
public:
  static svtkClientServerSynchronizedRenderers* New();
  svtkTypeMacro(svtkClientServerSynchronizedRenderers, svtkSynchronizedRenderers);
  void PrintSelf(ostream& os, svtkIndent indent) override;

protected:
  svtkClientServerSynchronizedRenderers();
  ~svtkClientServerSynchronizedRenderers() override;

  void MasterEndRender() override;
  void SlaveEndRender() override;

private:
  svtkClientServerSynchronizedRenderers(const svtkClientServerSynchronizedRenderers&) = delete;
  void operator=(const svtkClientServerSynchronizedRenderers&) = delete;
};

#endif
