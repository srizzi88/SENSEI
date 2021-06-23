/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkQtTreeRingLabelMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/*-------------------------------------------------------------------------
  Copyright 2008 Sandia Corporation.
  Under the terms of Contract DE-AC04-94AL85000 with Sandia Corporation,
  the U.S. Government retains certain rights in this software.
-------------------------------------------------------------------------*/
/**
 * @class   svtkQtTreeRingLabelMapper
 * @brief   draw text labels on a tree map
 *
 *
 * svtkQtTreeRingLabelMapper is a mapper that renders text on a tree map.
 * A tree map is a svtkTree with an associated 4-tuple array
 * used for storing the boundary rectangle for each vertex in the tree.
 * The user must specify the array name used for storing the rectangles.
 *
 * The mapper iterates through the tree and attempts and renders a label
 * inside the vertex's rectangle as long as the following conditions hold:
 * 1. The vertex level is within the range of levels specified for labeling.
 * 2. The label can fully fit inside its box.
 * 3. The label does not overlap an ancestor's label.
 *
 * @sa
 * svtkLabeledDataMapper
 *
 * @par Thanks:
 * Thanks to Jason Shepherd from
 * Sandia National Laboratories for help in developing this class.
 */

#ifndef svtkQtTreeRingLabelMapper_h
#define svtkQtTreeRingLabelMapper_h

#include "svtkLabeledDataMapper.h"
#include "svtkRenderingQtModule.h" // For export macro

class QImage;

class svtkQImageToImageSource;
class svtkCoordinate;
class svtkDoubleArray;
class svtkPlaneSource;
class svtkPolyDataMapper2D;
class svtkRenderer;
class svtkStringArray;
class svtkTexture;
class svtkTextureMapToPlane;
class svtkTree;
class svtkUnicodeStringArray;

class SVTKRENDERINGQT_EXPORT svtkQtTreeRingLabelMapper : public svtkLabeledDataMapper
{
public:
  static svtkQtTreeRingLabelMapper* New();
  svtkTypeMacro(svtkQtTreeRingLabelMapper, svtkLabeledDataMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Draw the text to the screen at each input point.
   */
  void RenderOpaqueGeometry(svtkViewport* viewport, svtkActor2D* actor) override;
  void RenderOverlay(svtkViewport* viewport, svtkActor2D* actor) override;
  //@}

  /**
   * The input to this filter.
   */
  virtual svtkTree* GetInputTree();

  /**
   * The name of the 4-tuple array used for
   */
  virtual void SetSectorsArrayName(const char* name);

  //@{
  /**
   * Set/Get the text property. Note that multiple type text properties
   * (set with a second integer parameter) are not currently supported,
   * but are provided to avoid compiler warnings.
   */
  void SetLabelTextProperty(svtkTextProperty* p) override;
  svtkTextProperty* GetLabelTextProperty() override { return this->LabelTextProperty; }
  void SetLabelTextProperty(svtkTextProperty* p, int type) override
  {
    this->Superclass::SetLabelTextProperty(p, type);
  }
  svtkTextProperty* GetLabelTextProperty(int type) override
  {
    return this->Superclass::GetLabelTextProperty(type);
  }
  //@}

  //@{
  /**
   * Set/Get the name of the text rotation array.
   */
  svtkSetStringMacro(TextRotationArrayName);
  svtkGetStringMacro(TextRotationArrayName);
  //@}

  /**
   * Return the object's MTime. This is overridden to include
   * the timestamp of its internal class.
   */
  svtkMTimeType GetMTime() override;

  void SetRenderer(svtkRenderer* ren)
  {
    if (this->Renderer != ren)
    {
      this->Renderer = ren;
      this->Modified();
    }
  }
  svtkRenderer* GetRenderer() { return this->Renderer; }

protected:
  svtkQtTreeRingLabelMapper();
  ~svtkQtTreeRingLabelMapper() override;
  void LabelTree(svtkTree* tree, svtkDataArray* sectorInfo, svtkDataArray* numericData,
    svtkStringArray* stringData, svtkUnicodeStringArray* uStringData, int activeComp, int numComps,
    svtkViewport* viewport);
  void GetVertexLabel(svtkIdType vertex, svtkDataArray* numericData, svtkStringArray* stringData,
    svtkUnicodeStringArray* uStringData, int activeComp, int numComps, char* string,
    size_t stringSize);

  // Returns true if the center of the sector is in the window
  // along with the pixel dimensions (width, height)  of the sector
  bool PointInWindow(double* sinfo, double* newDim, double* textPosDC, svtkViewport* viewport);

  svtkViewport* CurrentViewPort;
  svtkCoordinate* VCoord;
  svtkQImageToImageSource* QtImageSource;
  svtkPlaneSource* PlaneSource;
  svtkRenderer* Renderer;
  svtkTextProperty* LabelTextProperty;
  svtkTexture* LabelTexture;
  svtkTextureMapToPlane* TextureMapToPlane;
  char* TextRotationArrayName;
  svtkPolyDataMapper2D* polyDataMapper;
  QImage* QtImage;
  int WindowSize[2];

private:
  svtkQtTreeRingLabelMapper(const svtkQtTreeRingLabelMapper&) = delete;
  void operator=(const svtkQtTreeRingLabelMapper&) = delete;
};

#endif
