/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTextMapper
 * @brief   2D text annotation
 *
 * svtkTextMapper provides 2D text annotation support for SVTK.  It is a
 * svtkMapper2D that can be associated with a svtkActor2D and placed into a
 * svtkRenderer.
 *
 * To use svtkTextMapper, specify an input text string.
 *
 * @sa
 * svtkActor2D svtkTextActor svtkTextActor3D svtkTextProperty svtkTextRenderer
 */

#ifndef svtkTextMapper_h
#define svtkTextMapper_h

#include "svtkMapper2D.h"
#include "svtkRenderingCoreModule.h" // For export macro

#include "svtkNew.h" // For svtkNew

class svtkActor2D;
class svtkImageData;
class svtkPoints;
class svtkPolyData;
class svtkPolyDataMapper2D;
class svtkTextProperty;
class svtkTexture;
class svtkTimeStamp;
class svtkViewport;

class SVTKRENDERINGCORE_EXPORT svtkTextMapper : public svtkMapper2D
{
public:
  svtkTypeMacro(svtkTextMapper, svtkMapper2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a new text mapper.
   */
  static svtkTextMapper* New();

  //@{
  /**
   * Return the size[2]/width/height of the rectangle required to draw this
   * mapper (in pixels).
   */
  virtual void GetSize(svtkViewport*, int size[2]);
  virtual int GetWidth(svtkViewport* v);
  virtual int GetHeight(svtkViewport* v);
  //@}

  //@{
  /**
   * The input text string to the mapper.
   */
  svtkSetStringMacro(Input);
  svtkGetStringMacro(Input);
  //@}

  //@{
  /**
   * Set/Get the text property.
   */
  virtual void SetTextProperty(svtkTextProperty* p);
  svtkGetObjectMacro(TextProperty, svtkTextProperty);
  //@}

  /**
   * Shallow copy of an actor.
   */
  void ShallowCopy(svtkAbstractMapper* m) override;

  //@{
  /**
   * Set and return the font size (in points) required to make this mapper fit
   * in a given
   * target rectangle (width x height, in pixels). A static version of the method
   * is also available for convenience to other classes (e.g., widgets).
   */
  virtual int SetConstrainedFontSize(svtkViewport*, int targetWidth, int targetHeight);
  static int SetConstrainedFontSize(
    svtkTextMapper*, svtkViewport*, int targetWidth, int targetHeight);
  //@}

  /**
   * Set and return the font size (in points) required to make each element of
   * an array
   * of mappers fit in a given rectangle (width x height, in pixels).  This
   * font size is the smallest size that was required to fit the largest
   * mapper in this constraint.
   */
  static int SetMultipleConstrainedFontSize(svtkViewport*, int targetWidth, int targetHeight,
    svtkTextMapper** mappers, int nbOfMappers, int* maxResultingSize);

  //@{
  /**
   * Use these methods when setting font size relative to the renderer's size. These
   * methods are static so that external classes (e.g., widgets) can easily use them.
   */
  static int SetRelativeFontSize(
    svtkTextMapper*, svtkViewport*, const int* winSize, int* stringSize, float sizeFactor = 0.0);
  static int SetMultipleRelativeFontSize(svtkViewport* viewport, svtkTextMapper** textMappers,
    int nbOfMappers, int* winSize, int* stringSize, float sizeFactor);
  //@}

  void RenderOverlay(svtkViewport*, svtkActor2D*) override;
  void ReleaseGraphicsResources(svtkWindow*) override;
  svtkMTimeType GetMTime() override;

protected:
  svtkTextMapper();
  ~svtkTextMapper() override;

  char* Input;
  svtkTextProperty* TextProperty;

private:
  svtkTextMapper(const svtkTextMapper&) = delete;
  void operator=(const svtkTextMapper&) = delete;

  void UpdateQuad(svtkActor2D* actor, int dpi);
  void UpdateImage(int dpi);

  int TextDims[2];

  int RenderedDPI;
  svtkTimeStamp CoordsTime;
  svtkTimeStamp TCoordsTime;
  svtkNew<svtkImageData> Image;
  svtkNew<svtkPoints> Points;
  svtkNew<svtkPolyData> PolyData;
  svtkNew<svtkPolyDataMapper2D> Mapper;
  svtkNew<svtkTexture> Texture;
};

#endif
