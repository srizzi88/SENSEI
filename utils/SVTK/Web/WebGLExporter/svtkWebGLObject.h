/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkWebGLObject.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkWebGLObject
 *
 * svtkWebGLObject represent and manipulate an WebGL object and its data.
 */

#ifndef svtkWebGLObject_h
#define svtkWebGLObject_h

#include "svtkObject.h"
#include "svtkWebGLExporterModule.h" // needed for export macro

#include <string> // needed for ID and md5 storing

class svtkMatrix4x4;
class svtkUnsignedCharArray;

enum WebGLObjectTypes
{
  wPOINTS = 0,
  wLINES = 1,
  wTRIANGLES = 2
};

class SVTKWEBGLEXPORTER_EXPORT svtkWebGLObject : public svtkObject
{
public:
  static svtkWebGLObject* New();
  svtkTypeMacro(svtkWebGLObject, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  virtual void GenerateBinaryData();
  virtual unsigned char* GetBinaryData(int part);
  virtual int GetBinarySize(int part);
  virtual int GetNumberOfParts();

  /**
   * This is a wrapper friendly method for access the binary data.
   * The binary data for the requested part will be copied into the
   * given svtkUnsignedCharArray.
   */
  void GetBinaryData(int part, svtkUnsignedCharArray* buffer);

  void SetLayer(int l);
  void SetRendererId(size_t i);
  void SetId(const std::string& i);
  void SetWireframeMode(bool wireframe);
  void SetVisibility(bool vis);
  void SetTransformationMatrix(svtkMatrix4x4* m);
  void SetIsWidget(bool w);
  void SetHasTransparency(bool t);
  void SetInteractAtServer(bool i);
  void SetType(WebGLObjectTypes t);
  bool isWireframeMode();
  bool isVisible();
  bool HasChanged();
  bool isWidget();
  bool HasTransparency();
  bool InteractAtServer();

  std::string GetMD5();
  std::string GetId();

  size_t GetRendererId();
  int GetLayer();

protected:
  svtkWebGLObject();
  ~svtkWebGLObject() override;

  float Matrix[16];
  size_t rendererId;
  int layer;      // Renderer Layer
  std::string id; // Id of the object
  std::string MD5;
  bool hasChanged;
  bool iswireframeMode;
  bool isvisible;
  WebGLObjectTypes webGlType;
  bool hasTransparency;
  bool iswidget;
  bool interactAtServer;

private:
  svtkWebGLObject(const svtkWebGLObject&) = delete;
  void operator=(const svtkWebGLObject&) = delete;
};

#endif
