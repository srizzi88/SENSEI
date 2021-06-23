/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkRIBExporter.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkRIBExporter
 * @brief   export a scene into RenderMan RIB format.
 *
 * svtkRIBExporter is a concrete subclass of svtkExporter that writes a
 * Renderman .RIB files. The input specifies a svtkRenderWindow. All
 * visible actors and lights will be included in the rib file. The
 * following file naming conventions apply:
 *   rib file - FilePrefix.rib
 *   image file created by RenderMan - FilePrefix.tif
 *   texture files - TexturePrefix_0xADDR_MTIME.tif
 * This object does NOT generate an image file. The user must run either
 * RenderMan or a RenderMan emulator like Blue Moon Ray Tracer (BMRT).
 * svtk properties are convert to Renderman shaders as follows:
 *   Normal property, no texture map - plastic.sl
 *   Normal property with texture map - txtplastic.sl
 * These two shaders must be compiled by the rendering package being
 * used.  svtkRIBExporter also supports custom shaders. The shaders are
 * written using the Renderman Shading Language. See "The Renderman
 * Companion", ISBN 0-201-50868, 1989 for details on writing shaders.
 * svtkRIBProperty specifies the declarations and parameter settings for
 * custom shaders.
 *
 * @sa
 * svtkExporter svtkRIBProperty svtkRIBLight
 */

#ifndef svtkRIBExporter_h
#define svtkRIBExporter_h

#include "svtkExporter.h"
#include "svtkIOExportModule.h" // For export macro

class svtkActor;
class svtkCamera;
class svtkLight;
class svtkPolyData;
class svtkProperty;
class svtkRenderer;
class svtkTexture;
class svtkUnsignedCharArray;

class SVTKIOEXPORT_EXPORT svtkRIBExporter : public svtkExporter
{
public:
  static svtkRIBExporter* New();
  svtkTypeMacro(svtkRIBExporter, svtkExporter);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Specify the size of the image for RenderMan. If none is specified, the
   * size of the render window will be used.
   */
  svtkSetVector2Macro(Size, int);
  svtkGetVectorMacro(Size, int, 2);
  //@}

  //@{
  /**
   * Specify the sampling rate for the rendering. Default is 2 2.
   */
  svtkSetVector2Macro(PixelSamples, int);
  svtkGetVectorMacro(PixelSamples, int, 2);
  //@}

  //@{
  /**
   * Specify the prefix of the files to write out. The resulting file names
   * will have .rib appended to them.
   */
  svtkSetStringMacro(FilePrefix);
  svtkGetStringMacro(FilePrefix);
  //@}

  //@{
  /**
   * Specify the prefix of any generated texture files.
   */
  svtkSetStringMacro(TexturePrefix);
  svtkGetStringMacro(TexturePrefix);
  //@}

  //@{
  /**
   * Set/Get the background flag. Default is 0 (off).
   * If set, the rib file will contain an
   * image shader that will use the renderer window's background
   * color. Normally, RenderMan does generate backgrounds. Backgrounds are
   * composited into the scene with the tiffcomp program that comes with
   * Pixar's RenderMan Toolkit.  In fact, Pixar's Renderman will accept an
   * image shader but only sets the alpha of the background. Images created
   * this way will still have a black background but contain an alpha of 1
   * at all pixels and CANNOT be subsequently composited with other images
   * using tiffcomp.  However, other RenderMan compliant renderers like
   * Blue Moon Ray Tracing (BMRT) do allow image shaders and properly set
   * the background color. If this sounds too confusing, use the following
   * rules: If you are using Pixar's Renderman, leave the Background
   * off. Otherwise, try setting BackGroundOn and see if you get the
   * desired results.
   */
  svtkSetMacro(Background, svtkTypeBool);
  svtkGetMacro(Background, svtkTypeBool);
  svtkBooleanMacro(Background, svtkTypeBool);
  //@}

  //@{
  /**
   * Set or get the ExportArrays. If ExportArrays is set, then
   * all point data, field data, and cell data arrays will get
   * exported together with polygons. Default is Off (0).
   */
  svtkSetClampMacro(ExportArrays, svtkTypeBool, 0, 1);
  svtkBooleanMacro(ExportArrays, svtkTypeBool);
  svtkGetMacro(ExportArrays, svtkTypeBool);
  //@}

protected:
  svtkRIBExporter();
  ~svtkRIBExporter() override;

  svtkTypeBool Background;
  int Size[2];
  int PixelSamples[2];

  /**
   * This variable defines whether the arrays are exported or not.
   */
  svtkTypeBool ExportArrays;

  //@{
  /**
   * Write the RIB header.
   */
  void WriteHeader(svtkRenderer* aRen);
  void WriteTrailer();
  void WriteTexture(svtkTexture* aTexture);
  void WriteViewport(svtkRenderer* aRenderer, int size[2]);
  void WriteCamera(svtkCamera* aCamera);
  void WriteLight(svtkLight* aLight, int count);
  void WriteAmbientLight(int count);
  void WriteProperty(svtkProperty* aProperty, svtkTexture* aTexture);
  void WritePolygons(svtkPolyData* pd, svtkUnsignedCharArray* colors, svtkProperty* aProperty);
  void WriteStrips(svtkPolyData* pd, svtkUnsignedCharArray* colors, svtkProperty* aProperty);
  //@}

  void WriteData() override;
  void WriteActor(svtkActor* anActor);

  /**
   * Since additional variables are sent to the shader as
   * variables, and their names are used in the shader, these
   * names have to follow C naming convention. This method
   * modifies array name so that you can use it in shader.
   */
  void ModifyArrayName(char* newname, const char* name);

  char* GetTextureName(svtkTexture* aTexture);
  char* GetTIFFName(svtkTexture* aTexture);
  char* FilePrefix;
  FILE* FilePtr;
  char* TexturePrefix;

private:
  svtkRIBExporter(const svtkRIBExporter&) = delete;
  void operator=(const svtkRIBExporter&) = delete;
};

#endif
