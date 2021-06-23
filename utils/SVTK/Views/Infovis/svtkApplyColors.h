/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkApplyColors.h

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
 * @class   svtkApplyColors
 * @brief   apply colors to a data set.
 *
 *
 * svtkApplyColors performs a coloring of the dataset using default colors,
 * lookup tables, annotations, and/or a selection. The output is a
 * four-component svtkUnsignedCharArray containing RGBA tuples for each
 * element in the dataset. The first input is the dataset to be colored, which
 * may be a svtkTable, svtkGraph subclass, or svtkDataSet subclass. The API
 * of this algorithm refers to "points" and "cells". For svtkGraph, the
 * "points" refer to the graph vertices and "cells" refer to graph edges.
 * For svtkTable, "points" refer to table rows. For svtkDataSet subclasses, the
 * meaning is obvious.
 *
 * The second (optional) input is a svtkAnnotationLayers object, which stores
 * a list of annotation layers, with each layer holding a list of
 * svtkAnnotation objects. The annotation specifies a subset of data along with
 * other properties, including color. For annotations with color properties,
 * this algorithm will use the color to color elements, using a "top one wins"
 * strategy.
 *
 * The third (optional) input is a svtkSelection object, meant for specifying
 * the current selection. You can control the color of the selection.
 *
 * The algorithm takes two input arrays, specified with
 * SetInputArrayToProcess(0, 0, 0, svtkDataObject::FIELD_ASSOCIATION_POINTS, name)
 * and
 * SetInputArrayToProcess(1, 0, 0, svtkDataObject::FIELD_ASSOCIATION_CELLS, name).
 * These set the point and cell data arrays to use to color the data with
 * the associated lookup table. For svtkGraph, svtkTable inputs, you would use
 * FIELD_ASSOCIATION_VERTICES, FIELD_ASSOCIATION_EDGES, or
 * FIELD_ASSOCIATION_ROWS as appropriate.
 *
 * To use the color array generated here, you should do the following:
 *
 *  mapper->SetScalarModeToUseCellFieldData();
 *  mapper->SelectColorArray("svtkApplyColors color");
 *  mapper->SetScalarVisibility(true);
 *
 * Colors are assigned with the following priorities:
 * <ol>
 * <li> If an item is part of the selection, it is colored with that color.
 * <li> Otherwise, if the item is part of an annotation, it is colored
 *      with the color of the final (top) annotation in the set of layers.
 * <li> Otherwise, if the lookup table is used, it is colored using the
 *      lookup table color for the data value of the element.
 * <li> Otherwise it will be colored with the default color.
 * </ol>
 *
 * Note: The opacity of an unselected item is defined by the multiplication
 * of default opacity, lookup table opacity, and annotation opacity, where
 * opacity is taken as a number from 0 to 1. So items will never be more opaque
 * than any of these three opacities. Selected items are always given the
 * selection opacity directly.
 */

#ifndef svtkApplyColors_h
#define svtkApplyColors_h

#include "svtkPassInputTypeAlgorithm.h"
#include "svtkViewsInfovisModule.h" // For export macro

class svtkScalarsToColors;
class svtkUnsignedCharArray;

class SVTKVIEWSINFOVIS_EXPORT svtkApplyColors : public svtkPassInputTypeAlgorithm
{
public:
  static svtkApplyColors* New();
  svtkTypeMacro(svtkApplyColors, svtkPassInputTypeAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * The lookup table to use for point colors. This is only used if
   * input array 0 is set and UsePointLookupTable is on.
   */
  virtual void SetPointLookupTable(svtkScalarsToColors* lut);
  svtkGetObjectMacro(PointLookupTable, svtkScalarsToColors);
  //@}

  //@{
  /**
   * If on, uses the point lookup table to set the colors of unannotated,
   * unselected elements of the data.
   */
  svtkSetMacro(UsePointLookupTable, bool);
  svtkGetMacro(UsePointLookupTable, bool);
  svtkBooleanMacro(UsePointLookupTable, bool);
  //@}

  //@{
  /**
   * If on, uses the range of the data to scale the lookup table range.
   * Otherwise, uses the range defined in the lookup table.
   */
  svtkSetMacro(ScalePointLookupTable, bool);
  svtkGetMacro(ScalePointLookupTable, bool);
  svtkBooleanMacro(ScalePointLookupTable, bool);
  //@}

  //@{
  /**
   * The default point color for all unannotated, unselected elements
   * of the data. This is used if UsePointLookupTable is off.
   */
  svtkSetVector3Macro(DefaultPointColor, double);
  svtkGetVector3Macro(DefaultPointColor, double);
  //@}

  //@{
  /**
   * The default point opacity for all unannotated, unselected elements
   * of the data. This is used if UsePointLookupTable is off.
   */
  svtkSetMacro(DefaultPointOpacity, double);
  svtkGetMacro(DefaultPointOpacity, double);
  //@}

  //@{
  /**
   * The point color for all selected elements of the data.
   * This is used if the selection input is available.
   */
  svtkSetVector3Macro(SelectedPointColor, double);
  svtkGetVector3Macro(SelectedPointColor, double);
  //@}

  //@{
  /**
   * The point opacity for all selected elements of the data.
   * This is used if the selection input is available.
   */
  svtkSetMacro(SelectedPointOpacity, double);
  svtkGetMacro(SelectedPointOpacity, double);
  //@}

  //@{
  /**
   * The output array name for the point color RGBA array.
   * Default is "svtkApplyColors color".
   */
  svtkSetStringMacro(PointColorOutputArrayName);
  svtkGetStringMacro(PointColorOutputArrayName);
  //@}

  //@{
  /**
   * The lookup table to use for cell colors. This is only used if
   * input array 1 is set and UseCellLookupTable is on.
   */
  virtual void SetCellLookupTable(svtkScalarsToColors* lut);
  svtkGetObjectMacro(CellLookupTable, svtkScalarsToColors);
  //@}

  //@{
  /**
   * If on, uses the cell lookup table to set the colors of unannotated,
   * unselected elements of the data.
   */
  svtkSetMacro(UseCellLookupTable, bool);
  svtkGetMacro(UseCellLookupTable, bool);
  svtkBooleanMacro(UseCellLookupTable, bool);
  //@}

  //@{
  /**
   * If on, uses the range of the data to scale the lookup table range.
   * Otherwise, uses the range defined in the lookup table.
   */
  svtkSetMacro(ScaleCellLookupTable, bool);
  svtkGetMacro(ScaleCellLookupTable, bool);
  svtkBooleanMacro(ScaleCellLookupTable, bool);
  //@}

  //@{
  /**
   * The default cell color for all unannotated, unselected elements
   * of the data. This is used if UseCellLookupTable is off.
   */
  svtkSetVector3Macro(DefaultCellColor, double);
  svtkGetVector3Macro(DefaultCellColor, double);
  //@}

  //@{
  /**
   * The default cell opacity for all unannotated, unselected elements
   * of the data. This is used if UseCellLookupTable is off.
   */
  svtkSetMacro(DefaultCellOpacity, double);
  svtkGetMacro(DefaultCellOpacity, double);
  //@}

  //@{
  /**
   * The cell color for all selected elements of the data.
   * This is used if the selection input is available.
   */
  svtkSetVector3Macro(SelectedCellColor, double);
  svtkGetVector3Macro(SelectedCellColor, double);
  //@}

  //@{
  /**
   * The cell opacity for all selected elements of the data.
   * This is used if the selection input is available.
   */
  svtkSetMacro(SelectedCellOpacity, double);
  svtkGetMacro(SelectedCellOpacity, double);
  //@}

  //@{
  /**
   * The output array name for the cell color RGBA array.
   * Default is "svtkApplyColors color".
   */
  svtkSetStringMacro(CellColorOutputArrayName);
  svtkGetStringMacro(CellColorOutputArrayName);
  //@}

  //@{
  /**
   * Use the annotation to color the current annotation
   * (i.e. the current selection). Otherwise use the selection
   * color attributes of this filter.
   */
  svtkSetMacro(UseCurrentAnnotationColor, bool);
  svtkGetMacro(UseCurrentAnnotationColor, bool);
  svtkBooleanMacro(UseCurrentAnnotationColor, bool);
  //@}

  /**
   * Retrieve the modified time for this filter.
   */
  svtkMTimeType GetMTime() override;

protected:
  svtkApplyColors();
  ~svtkApplyColors() override;

  /**
   * Convert the svtkGraph into svtkPolyData.
   */
  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  /**
   * Set the input type of the algorithm to svtkGraph.
   */
  int FillInputPortInformation(int port, svtkInformation* info) override;

  void ProcessColorArray(svtkUnsignedCharArray* colorArr, svtkScalarsToColors* lut,
    svtkAbstractArray* arr, unsigned char color[4], bool scale);

  svtkScalarsToColors* PointLookupTable;
  svtkScalarsToColors* CellLookupTable;
  double DefaultPointColor[3];
  double DefaultPointOpacity;
  double DefaultCellColor[3];
  double DefaultCellOpacity;
  double SelectedPointColor[3];
  double SelectedPointOpacity;
  double SelectedCellColor[3];
  double SelectedCellOpacity;
  bool ScalePointLookupTable;
  bool ScaleCellLookupTable;
  bool UsePointLookupTable;
  bool UseCellLookupTable;
  char* PointColorOutputArrayName;
  char* CellColorOutputArrayName;
  bool UseCurrentAnnotationColor;

private:
  svtkApplyColors(const svtkApplyColors&) = delete;
  void operator=(const svtkApplyColors&) = delete;
};

#endif
