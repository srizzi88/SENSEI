/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLabeledDataMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkLabeledDataMapper
 * @brief   draw text labels at dataset points
 *
 * svtkLabeledDataMapper is a mapper that renders text at dataset
 * points. Various items can be labeled including point ids, scalars,
 * vectors, normals, texture coordinates, tensors, and field data components.
 *
 * The format with which the label is drawn is specified using a
 * printf style format string. The font attributes of the text can
 * be set through the svtkTextProperty associated to this mapper.
 *
 * By default, all the components of multi-component data such as
 * vectors, normals, texture coordinates, tensors, and multi-component
 * scalars are labeled. However, you can specify a single component if
 * you prefer. (Note: the label format specifies the format to use for
 * a single component. The label is creating by looping over all components
 * and using the label format to render each component.)
 * The character separator between components can be set. By default,
 * it is set to a single whitespace.
 *
 * @warning
 * Use this filter in combination with svtkSelectVisiblePoints if you want
 * to label only points that are visible. If you want to label cells rather
 * than points, use the filter svtkCellCenters to generate points at the
 * center of the cells. Also, you can use the class svtkIdFilter to
 * generate ids as scalars or field data, which can then be labeled.
 *
 * @sa
 * svtkMapper2D svtkActor2D svtkTextMapper svtkTextProperty svtkSelectVisiblePoints
 * svtkIdFilter svtkCellCenters
 */

#ifndef svtkLabeledDataMapper_h
#define svtkLabeledDataMapper_h

#include "svtkMapper2D.h"
#include "svtkRenderingLabelModule.h" // For export macro

#include <cassert> // For assert macro

class svtkDataObject;
class svtkDataSet;
class svtkTextMapper;
class svtkTextProperty;
class svtkTransform;

#define SVTK_LABEL_IDS 0
#define SVTK_LABEL_SCALARS 1
#define SVTK_LABEL_VECTORS 2
#define SVTK_LABEL_NORMALS 3
#define SVTK_LABEL_TCOORDS 4
#define SVTK_LABEL_TENSORS 5
#define SVTK_LABEL_FIELD_DATA 6

class SVTKRENDERINGLABEL_EXPORT svtkLabeledDataMapper : public svtkMapper2D
{
public:
  /**
   * Instantiate object with %%-#6.3g label format. By default, point ids
   * are labeled.
   */
  static svtkLabeledDataMapper* New();

  svtkTypeMacro(svtkLabeledDataMapper, svtkMapper2D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the format with which to print the labels.  This should
   * be a printf-style format string.

   * By default, the mapper will try to print each component of the
   * tuple using a sane format: %d for integers, %f for floats, %g for
   * doubles, %ld for longs, et cetera.  If you need a different
   * format, set it here.  You can do things like limit the number of
   * significant digits, add prefixes/suffixes, basically anything
   * that printf can do.  If you only want to print one component of a
   * vector, see the ivar LabeledComponent.
   */
  svtkSetStringMacro(LabelFormat);
  svtkGetStringMacro(LabelFormat);
  //@}

  //@{
  /**
   * Set/Get the component number to label if the data to print has
   * more than one component. For example, all the components of
   * scalars, vectors, normals, etc. are labeled by default
   * (LabeledComponent=(-1)). However, if this ivar is nonnegative,
   * then only the one component specified is labeled.
   */
  svtkSetMacro(LabeledComponent, int);
  svtkGetMacro(LabeledComponent, int);
  //@}

  //@{
  /**
   * Set/Get the separator between components.
   */
  svtkSetMacro(ComponentSeparator, char);
  svtkGetMacro(ComponentSeparator, char);
  //@}

  //@{
  /**
   * Set/Get the field data array to label. This instance variable is
   * only applicable if field data is labeled.  This will clear
   * FieldDataName when set.
   */
  void SetFieldDataArray(int arrayIndex);
  svtkGetMacro(FieldDataArray, int);
  //@}

  //@{
  /**
   * Set/Get the name of the field data array to label.  This instance
   * variable is only applicable if field data is labeled.  This will
   * override FieldDataArray when set.
   */
  void SetFieldDataName(const char* arrayName);
  svtkGetStringMacro(FieldDataName);
  //@}

  /**
   * Set the input dataset to the mapper. This mapper handles any type of data.
   */
  virtual void SetInputData(svtkDataObject*);

  /**
   * Use GetInputDataObject() to get the input data object for composite
   * datasets.
   */
  svtkDataSet* GetInput();

  //@{
  /**
   * Specify which data to plot: IDs, scalars, vectors, normals, texture coords,
   * tensors, or field data. If the data has more than one component, use
   * the method SetLabeledComponent to control which components to plot.
   * The default is SVTK_LABEL_IDS.
   */
  svtkSetMacro(LabelMode, int);
  svtkGetMacro(LabelMode, int);
  void SetLabelModeToLabelIds() { this->SetLabelMode(SVTK_LABEL_IDS); }
  void SetLabelModeToLabelScalars() { this->SetLabelMode(SVTK_LABEL_SCALARS); }
  void SetLabelModeToLabelVectors() { this->SetLabelMode(SVTK_LABEL_VECTORS); }
  void SetLabelModeToLabelNormals() { this->SetLabelMode(SVTK_LABEL_NORMALS); }
  void SetLabelModeToLabelTCoords() { this->SetLabelMode(SVTK_LABEL_TCOORDS); }
  void SetLabelModeToLabelTensors() { this->SetLabelMode(SVTK_LABEL_TENSORS); }
  void SetLabelModeToLabelFieldData() { this->SetLabelMode(SVTK_LABEL_FIELD_DATA); }
  //@}

  //@{
  /**
   * Set/Get the text property.
   * If an integer argument is provided, you may provide different text
   * properties for different label types. The type is determined by an
   * optional type input array.
   */
  virtual void SetLabelTextProperty(svtkTextProperty* p) { this->SetLabelTextProperty(p, 0); }
  virtual svtkTextProperty* GetLabelTextProperty() { return this->GetLabelTextProperty(0); }
  virtual void SetLabelTextProperty(svtkTextProperty* p, int type);
  virtual svtkTextProperty* GetLabelTextProperty(int type);
  //@}

  //@{
  /**
   * Draw the text to the screen at each input point.
   */
  void RenderOpaqueGeometry(svtkViewport* viewport, svtkActor2D* actor) override;
  void RenderOverlay(svtkViewport* viewport, svtkActor2D* actor) override;
  //@}

  /**
   * Release any graphics resources that are being consumed by this actor.
   */
  void ReleaseGraphicsResources(svtkWindow*) override;

  //@{
  /**
   * The transform to apply to the labels before mapping to 2D.
   */
  svtkGetObjectMacro(Transform, svtkTransform);
  void SetTransform(svtkTransform* t);
  //@}

  /// Coordinate systems that output dataset may use.
  enum Coordinates
  {
    WORLD = 0,  //!< Output 3-D world-space coordinates for each label anchor.
    DISPLAY = 1 //!< Output 2-D display coordinates for each label anchor (3 components but only 2
                //!< are significant).
  };

  //@{
  /**
   * Set/get the coordinate system used for output labels.
   * The output datasets may have point coordinates reported in the world space or display space.
   */
  svtkGetMacro(CoordinateSystem, int);
  svtkSetClampMacro(CoordinateSystem, int, WORLD, DISPLAY);
  void CoordinateSystemWorld() { this->SetCoordinateSystem(svtkLabeledDataMapper::WORLD); }
  void CoordinateSystemDisplay() { this->SetCoordinateSystem(svtkLabeledDataMapper::DISPLAY); }
  //@}

  /**
   * Return the modified time for this object.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Return the number of labels rendered by the mapper.
   */
  svtkGetMacro(NumberOfLabels, int);
  //@}

  //@{
  /**
   * Return the position of the requested label.
   */
  void GetLabelPosition(int label, double pos[3])
  {
    assert("label index range" && label >= 0 && label < this->NumberOfLabels);
    pos[0] = this->LabelPositions[3 * label];
    pos[1] = this->LabelPositions[3 * label + 1];
    pos[2] = this->LabelPositions[3 * label + 2];
  }
  //@}

  /**
   * Return the text for the requested label.
   */
  const char* GetLabelText(int label);

protected:
  svtkLabeledDataMapper();
  ~svtkLabeledDataMapper() override;

  svtkDataSet* Input;

  char* LabelFormat;
  int LabelMode;
  int LabeledComponent;
  int FieldDataArray;
  char* FieldDataName;
  int CoordinateSystem;

  char ComponentSeparator;

  svtkTimeStamp BuildTime;

  int NumberOfLabels;
  int NumberOfLabelsAllocated;
  svtkTextMapper** TextMappers;
  double* LabelPositions;
  svtkTransform* Transform;

  int FillInputPortInformation(int, svtkInformation*) override;

  void AllocateLabels(int numLabels);
  void BuildLabels();
  void BuildLabelsInternal(svtkDataSet*);

  class Internals;
  Internals* Implementation;

private:
  svtkLabeledDataMapper(const svtkLabeledDataMapper&) = delete;
  void operator=(const svtkLabeledDataMapper&) = delete;
};

#endif
