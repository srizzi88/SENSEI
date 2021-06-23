/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkViewNodeFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkViewNodeFactory
 * @brief   factory that chooses svtkViewNodes to create
 *
 * Class tells SVTK which specific svtkViewNode subclass to make when it is
 * asked to make a svtkViewNode for a particular renderable. modules for
 * different rendering backends are expected to use this to customize the
 * set of instances for their own purposes
 */

#ifndef svtkViewNodeFactory_h
#define svtkViewNodeFactory_h

#include "svtkObject.h"
#include "svtkRenderingSceneGraphModule.h" // For export macro

class svtkViewNode;

class SVTKRENDERINGSCENEGRAPH_EXPORT svtkViewNodeFactory : public svtkObject
{
public:
  static svtkViewNodeFactory* New();
  svtkTypeMacro(svtkViewNodeFactory, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Give a function pointer to a class that will manufacture a
   * svtkViewNode when given a class name string.
   */
  void RegisterOverride(const char* name, svtkViewNode* (*func)());

  /**
   * Creates and returns a svtkViewNode for the provided renderable.
   */
  svtkViewNode* CreateNode(svtkObject*);

  /**
   * @deprecated As of 9.0, No longer equivalent to CreateNode(svtkObject*).
   * Unused in 8.2.
   */
  SVTK_LEGACY(svtkViewNode* CreateNode(const char*));

protected:
  svtkViewNodeFactory();
  ~svtkViewNodeFactory() override;

private:
  svtkViewNodeFactory(const svtkViewNodeFactory&) = delete;
  void operator=(const svtkViewNodeFactory&) = delete;

  class svtkInternals;
  svtkInternals* Internals;
};

#endif
