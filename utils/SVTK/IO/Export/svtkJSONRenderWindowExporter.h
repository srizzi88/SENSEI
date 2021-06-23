/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkJSONRenderWindowExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkJSONRenderWindowExporter
 * @brief   Exports a render window for svtk-js
 *
 * svtkJSONRenderWindowExporter constructs a scene graph from an input render
 * window and generates an archive for svtk-js. The traversal of the scene graph
 * topology is handled by graph elements constructed by svtkVtkJSViewNodeFactory,
 * the translation from SVTK to svtk-js scene elements (renderers, actors,
 * mappers, etc.) is handled by svtkVtkJSSceneGraphSerializer, and the
 * transcription of data is handled by svtkArchiver. The latter two classes are
 * designed to be extensible via inheritance, and derived instances can be used
 * to modify the svtk-js file format and output mode.
 *
 *
 * @sa
 * svtkVtkJSSceneGraphSerializer svtkVtkJSViewNodeFactory
 */

#ifndef svtkJSONRenderWindowExporter_h
#define svtkJSONRenderWindowExporter_h

#include "svtkIOExportModule.h" // For export macro

#include "svtkExporter.h"
#include "svtkNew.h"             // For svtkNew
#include "svtkViewNodeFactory.h" // For svtkViewNodeFactory

class svtkArchiver;
class svtkVtkJSSceneGraphSerializer;
class svtkVtkJSViewNodeFactory;

class SVTKIOEXPORT_EXPORT svtkJSONRenderWindowExporter : public svtkExporter
{
public:
  static svtkJSONRenderWindowExporter* New();
  svtkTypeMacro(svtkJSONRenderWindowExporter, svtkExporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the Serializer object
   */
  virtual void SetSerializer(svtkVtkJSSceneGraphSerializer*);
  svtkGetObjectMacro(Serializer, svtkVtkJSSceneGraphSerializer);
  //@}

  //@{
  /**
   * Specify the Archiver object
   */
  virtual void SetArchiver(svtkArchiver*);
  svtkGetObjectMacro(Archiver, svtkArchiver);
  //@}

  //@{
  /**
   * Write scene data.
   */
  virtual void WriteData() override;
  //@}

  //@{
  /**
   * Write scene in compact form (default is true).
   */
  svtkSetMacro(CompactOutput, bool);
  svtkGetMacro(CompactOutput, bool);
  svtkBooleanMacro(CompactOutput, bool);
  //@}

protected:
  svtkJSONRenderWindowExporter();
  ~svtkJSONRenderWindowExporter() override;

private:
  svtkJSONRenderWindowExporter(const svtkJSONRenderWindowExporter&) = delete;
  void operator=(const svtkJSONRenderWindowExporter&) = delete;

  svtkArchiver* Archiver;
  svtkVtkJSSceneGraphSerializer* Serializer;
  svtkVtkJSViewNodeFactory* Factory;
  bool CompactOutput;
};

#endif
