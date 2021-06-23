/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVtkJSViewNodeFactory.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkVtkJSViewNodeFactory
 * @brief   Constructs view nodes for traversing a scene for svtk-js
 *
 * svtkVtkJSViewNodeFactory constructs view nodes that are subsequently executed
 * as a scene graph is traversed. The generated view nodes inherit from
 * svtkViewNode and augment the synchronize and render traversal steps to
 * construct Json representations of the scene elements and to update the
 * pipelines associated with the datasets to render, respectively.
 *
 *
 * @sa
 * svtkVtkJSSceneGraphSerializer
 */

#ifndef svtkVtkJSViewNodeFactory_h
#define svtkVtkJSViewNodeFactory_h

#include "svtkRenderingVtkJSModule.h" // For export macro

#include "svtkViewNodeFactory.h"

class svtkVtkJSSceneGraphSerializer;

class SVTKRENDERINGSVTKJS_EXPORT svtkVtkJSViewNodeFactory : public svtkViewNodeFactory
{
public:
  static svtkVtkJSViewNodeFactory* New();
  svtkTypeMacro(svtkVtkJSViewNodeFactory, svtkViewNodeFactory);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the Serializer object
   */
  void SetSerializer(svtkVtkJSSceneGraphSerializer*);
  svtkGetObjectMacro(Serializer, svtkVtkJSSceneGraphSerializer);
  //@}

protected:
  svtkVtkJSViewNodeFactory();
  ~svtkVtkJSViewNodeFactory() override;

  svtkVtkJSSceneGraphSerializer* Serializer;

private:
  svtkVtkJSViewNodeFactory(const svtkVtkJSViewNodeFactory&) = delete;
  void operator=(const svtkVtkJSViewNodeFactory&) = delete;
};

#endif
