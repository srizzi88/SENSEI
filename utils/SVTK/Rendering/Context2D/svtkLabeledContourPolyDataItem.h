/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLabeledContourPolyDataItem.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLabeledContourPolyDataItem
 * @brief   Filter that translate a svtkPolyData 2D mesh into svtkContextItems.
 *
 * @warning
 * The input svtkPolyData should be a 2D mesh.
 *
 */

#ifndef svtkLabeledContourPolyDataItem_h
#define svtkLabeledContourPolyDataItem_h

#include "svtkPolyDataItem.h"
#include "svtkRect.h"                     // For svtkRect/svtkVector/svtkTuple
#include "svtkRenderingContext2DModule.h" // For export macro
#include "svtkSmartPointer.h"             // For svtkSmartPointer

class svtkActor;
class svtkContext2D;
class svtkDoubleArray;
class svtkRenderer;
class svtkTextActor3D;
class svtkTextProperty;
class svtkTextPropertyCollection;
struct PDILabelHelper;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkLabeledContourPolyDataItem : public svtkPolyDataItem
{
public:
  svtkTypeMacro(svtkLabeledContourPolyDataItem, svtkPolyDataItem);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkLabeledContourPolyDataItem* New();

  /**
   * Paint event for the item.
   */
  bool Paint(svtkContext2D* painter) override;

  /**
   * The text property used to label the lines. Note that both vertical and
   * horizontal justifications will be reset to "Centered" prior to rendering.
   * @note This is a convenience method that clears TextProperties and inserts
   * the argument as the only property in the collection.
   * @sa SetTextProperties
   */
  virtual void SetTextProperty(svtkTextProperty* tprop);

  //@{
  /**
   * The text properties used to label the lines. Note that both vertical and
   * horizontal justifications will be reset to "Centered" prior to rendering.

   * If the TextPropertyMapping array exists, then it is used to identify which
   * text property to use for each label as follows: If the scalar value of a
   * line is found in the mapping, the index of the value in mapping is used to
   * lookup the text property in the collection. If there are more mapping
   * values than properties, the properties are looped through until the
   * mapping is exhausted.

   * Lines with scalar values missing from the mapping are assigned text
   * properties in a round-robin fashion starting from the beginning of the
   * collection, repeating from the start of the collection as necessary.
   * @sa SetTextProperty
   * @sa SetTextPropertyMapping
   */
  virtual void SetTextProperties(svtkTextPropertyCollection* coll);
  virtual svtkTextPropertyCollection* GetTextProperties();
  //@}

  //@{
  /**
   * Values in this array correspond to svtkTextProperty objects in the
   * TextProperties collection. If a contour line's scalar value exists in
   * this array, the corresponding text property is used for the label.
   * See SetTextProperties for more information.
   */
  virtual svtkDoubleArray* GetTextPropertyMapping();
  virtual void SetTextPropertyMapping(svtkDoubleArray* mapping);
  //@}

  //@{
  /**
   * If true, labels will be placed and drawn during rendering. Otherwise,
   * only the mapper returned by GetPolyDataMapper() will be rendered.
   * The default is to draw labels.
   */
  svtkSetMacro(LabelVisibility, bool);
  svtkGetMacro(LabelVisibility, bool);
  svtkBooleanMacro(LabelVisibility, bool);
  //@}

  //@{
  /**
   * Ensure that there are at least SkipDistance pixels between labels. This
   * is only enforced on labels along the same line. The default is 0.
   */
  svtkSetMacro(SkipDistance, double);
  svtkGetMacro(SkipDistance, double);
  //@}

protected:
  svtkLabeledContourPolyDataItem();
  ~svtkLabeledContourPolyDataItem() override;

  virtual void ComputeBounds();

  void Reset();

  bool CheckInputs();
  bool CheckRebuild();
  bool PrepareRender();
  bool PlaceLabels();
  bool ResolveLabels();
  virtual bool CreateLabels();
  bool RenderLabels(svtkContext2D* painter);

  bool AllocateTextActors(svtkIdType num);
  bool FreeTextActors();

  double SkipDistance;

  bool LabelVisibility;
  svtkIdType NumberOfTextActors;
  svtkIdType NumberOfUsedTextActors;
  svtkTextActor3D** TextActors;

  PDILabelHelper** LabelHelpers;

  svtkSmartPointer<svtkTextPropertyCollection> TextProperties;
  svtkSmartPointer<svtkDoubleArray> TextPropertyMapping;

  svtkTimeStamp LabelBuildTime;

private:
  svtkLabeledContourPolyDataItem(const svtkLabeledContourPolyDataItem&) = delete;
  void operator=(const svtkLabeledContourPolyDataItem&) = delete;

  struct Private;
  Private* Internal;
};

#endif
