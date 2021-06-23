/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLabeledContourMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLabeledContourMapper
 * @brief   Draw labeled isolines.
 *
 * Draw isolines with 3D inline labels.
 *
 * The lines in the input polydata will be drawn with labels displaying the
 * scalar value.
 *
 * For this mapper to function properly, stenciling must be enabled in the
 * render window (it is disabled by default). Otherwise the lines will be
 * drawn through the labels.
 */

#ifndef svtkLabeledContourMapper_h
#define svtkLabeledContourMapper_h

#include "svtkRenderingCoreModule.h" // For export macro

#include "svtkMapper.h"
#include "svtkNew.h"          // For svtkNew
#include "svtkSmartPointer.h" // For svtkSmartPointer

class svtkDoubleArray;
class svtkTextActor3D;
class svtkTextProperty;
class svtkTextPropertyCollection;
class svtkPolyData;
class svtkPolyDataMapper;

class SVTKRENDERINGCORE_EXPORT svtkLabeledContourMapper : public svtkMapper
{
public:
  static svtkLabeledContourMapper* New();
  svtkTypeMacro(svtkLabeledContourMapper, svtkMapper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void Render(svtkRenderer* ren, svtkActor* act) override;

  //@{
  /**
   * Specify the input data to map.
   */
  void SetInputData(svtkPolyData* in);
  svtkPolyData* GetInput();
  //@}

  //@{
  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double* GetBounds() override;
  void GetBounds(double bounds[6]) override;
  //@}

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

  //@{
  /**
   * The polydata mapper used to render the contours.
   */
  svtkGetNewMacro(PolyDataMapper, svtkPolyDataMapper);
  //@}

  void ReleaseGraphicsResources(svtkWindow*) override;

protected:
  svtkLabeledContourMapper();
  ~svtkLabeledContourMapper() override;

  virtual void ComputeBounds();

  int FillInputPortInformation(int, svtkInformation*) override;

  void Reset();

  bool CheckInputs(svtkRenderer* ren);
  bool CheckRebuild(svtkRenderer* ren, svtkActor* act);
  bool PrepareRender(svtkRenderer* ren, svtkActor* act);
  bool PlaceLabels();
  bool ResolveLabels();
  virtual bool CreateLabels(svtkActor* actor);
  bool BuildStencilQuads();
  virtual bool ApplyStencil(svtkRenderer* ren, svtkActor* act);
  bool RenderPolyData(svtkRenderer* ren, svtkActor* act);
  virtual bool RemoveStencil(svtkRenderer* ren);
  bool RenderLabels(svtkRenderer* ren, svtkActor* act);

  bool AllocateTextActors(svtkIdType num);
  bool FreeTextActors();

  double SkipDistance;

  bool LabelVisibility;
  svtkIdType NumberOfTextActors;
  svtkIdType NumberOfUsedTextActors;
  svtkTextActor3D** TextActors;

  svtkNew<svtkPolyDataMapper> PolyDataMapper;
  svtkSmartPointer<svtkTextPropertyCollection> TextProperties;
  svtkSmartPointer<svtkDoubleArray> TextPropertyMapping;

  float* StencilQuads;
  svtkIdType StencilQuadsSize;
  unsigned int* StencilQuadIndices;
  svtkIdType StencilQuadIndicesSize;
  void FreeStencilQuads();

  svtkTimeStamp LabelBuildTime;

private:
  svtkLabeledContourMapper(const svtkLabeledContourMapper&) = delete;
  void operator=(const svtkLabeledContourMapper&) = delete;

  struct Private;
  Private* Internal;
};

#endif
