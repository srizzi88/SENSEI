/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSequencePass.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSequencePass
 * @brief   Execute render passes sequentially.
 *
 * svtkSequencePass executes a list of render passes sequentially.
 * This class allows to define a sequence of render passes at run time.
 * The other solution to write a sequence of render passes is to write an
 * effective subclass of svtkRenderPass.
 *
 * As svtkSequencePass is a svtkRenderPass itself, it is possible to have a
 * hierarchy of render passes built at runtime.
 * @sa
 * svtkRenderPass
 */

#ifndef svtkSequencePass_h
#define svtkSequencePass_h

#include "svtkRenderPass.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkRenderPassCollection;

class SVTKRENDERINGOPENGL2_EXPORT svtkSequencePass : public svtkRenderPass
{
public:
  static svtkSequencePass* New();
  svtkTypeMacro(svtkSequencePass, svtkRenderPass);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Perform rendering according to a render state \p s.
   * \pre s_exists: s!=0
   */
  void Render(const svtkRenderState* s) override;

  /**
   * Release graphics resources and ask components to release their own
   * resources.
   * \pre w_exists: w!=0
   */
  void ReleaseGraphicsResources(svtkWindow* w) override;

  //@{
  /**
   * The ordered list of render passes to execute sequentially.
   * If the pointer is NULL or the list is empty, it is silently ignored.
   * There is no warning.
   * Initial value is a NULL pointer.
   */
  svtkGetObjectMacro(Passes, svtkRenderPassCollection);
  virtual void SetPasses(svtkRenderPassCollection* passes);
  //@}

protected:
  svtkRenderPassCollection* Passes;

  svtkSequencePass();
  ~svtkSequencePass() override;

private:
  svtkSequencePass(const svtkSequencePass&) = delete;
  void operator=(const svtkSequencePass&) = delete;
};

#endif
