/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLabeledTreeMapDataMapper.h

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
 * @class   svtkLabeledTreeMapDataMapper
 * @brief   draw text labels on a tree map
 *
 *
 * svtkLabeledTreeMapDataMapper is a mapper that renders text on a tree map.
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
 * Thanks to Patricia Crossno, Ken Moreland, Andrew Wilson and Brian Wylie from
 * Sandia National Laboratories for their help in developing this class.
 */

#ifndef svtkLabeledTreeMapDataMapper_h
#define svtkLabeledTreeMapDataMapper_h

#include "svtkLabeledDataMapper.h"
#include "svtkRenderingLabelModule.h" // For export macro

class svtkTree;
class svtkPoints;
class svtkCoordinate;
class svtkFloatArray;
class svtkStringArray;
class svtkIdList;

class SVTKRENDERINGLABEL_EXPORT svtkLabeledTreeMapDataMapper : public svtkLabeledDataMapper
{
public:
  static svtkLabeledTreeMapDataMapper* New();
  svtkTypeMacro(svtkLabeledTreeMapDataMapper, svtkLabeledDataMapper);
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
  virtual void SetRectanglesArrayName(const char* name);

  //@{
  /**
   * Indicates if the label can be displayed clipped by the Window
   * mode = 0 - ok to clip labels
   * 1 - auto center labels w/r to the area of the vertex's clipped region
   */
  svtkGetMacro(ClipTextMode, int);
  svtkSetMacro(ClipTextMode, int);
  //@}

  //@{
  /**
   * Indicates if the label can be moved by its ancestors
   */
  svtkGetMacro(ChildMotion, int);
  svtkSetMacro(ChildMotion, int);
  //@}

  //@{
  /**
   * Indicates at which level labeling should be dynamic
   */
  svtkGetMacro(DynamicLevel, int);
  svtkSetMacro(DynamicLevel, int);
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  //@{
  /**
   * The range of font sizes to use when rendering the labels.
   */
  void SetFontSizeRange(int maxSize, int minSize, int delta = 4);
  void GetFontSizeRange(int range[3]);
  //@}

  //@{
  /**
   * The range of levels to attempt to label.
   * The level of a vertex is the length of the path to the root
   * (the root has level 0).
   */
  void SetLevelRange(int startLevel, int endLevel);
  void GetLevelRange(int range[2]);
  //@}

protected:
  svtkLabeledTreeMapDataMapper();
  ~svtkLabeledTreeMapDataMapper() override;
  void LabelTree(svtkTree* tree, svtkFloatArray* boxInfo, svtkDataArray* numericData,
    svtkStringArray* stringData, int activeComp, int numComps);
  void GetVertexLabel(svtkIdType vertex, svtkDataArray* numericData, svtkStringArray* stringData,
    int activeComp, int numComps, char* string, size_t stringSize);
  void UpdateFontSizes();
  int UpdateWindowInfo(svtkViewport* viewport);
  int GetStringSize(char* string, int level);
  // Returns 1 if the transformed box is off screen
  int ConvertToDC(float* origBoxInfo, float* newBoxInfo);
  // Returns 1 if the label will not fit in box - 2 if the text could
  // not be placed due to other labels
  int AnalyseLabel(
    char* string, int level, float* blimitsDC, float* textPosWC, svtkTextProperty** tprop);
  int ApplyMasks(int level, float flimits[4], float blimits[4]);
  svtkViewport* CurrentViewPort;
  int* FontHeights;
  int** FontWidths;
  int MaxFontLevel;
  int* ChildrenCount;
  int MaxTreeLevels;
  double BoxTrans[2][2];
  double WindowLimits[2][2];

  float (*LabelMasks)[4];

  svtkIdList* VertexList;
  svtkPoints* TextPoints;
  svtkCoordinate* VCoord;
  int ClipTextMode;
  int ChildMotion;
  int StartLevel;
  int EndLevel;
  int DynamicLevel;
  svtkTextProperty* VerticalLabelProperty;
  svtkTextProperty** HLabelProperties;

private:
  svtkLabeledTreeMapDataMapper(const svtkLabeledTreeMapDataMapper&) = delete;
  void operator=(const svtkLabeledTreeMapDataMapper&) = delete;
};

#endif
