/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkLabelSizeCalculator.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkLabelSizeCalculator
 *
 * This filter takes an input dataset, an array to process
 * (which must be a string array), and a text property.
 * It creates a new output array (named "LabelSize" by default) with
 * 4 components per tuple that contain the width, height, horizontal
 * offset, and descender height (in that order) of each string in
 * the array.
 *
 * Use the inherited SelectInputArrayToProcess to indicate a string array.
 * In no input array is specified, the first of the following that
 * is a string array is used: point scalars, cell scalars, field scalars.
 *
 * The second input array to process is an array specifying the type of
 * each label. Different label types may have different font properties.
 * This array must be a svtkIntArray.
 * Any type that does not map to a font property that was set will
 * be set to the type 0's type property.
 */

#ifndef svtkLabelSizeCalculator_h
#define svtkLabelSizeCalculator_h

#include "svtkPassInputTypeAlgorithm.h"
#include "svtkRenderingLabelModule.h" // For export macro

class svtkIntArray;
class svtkTextRenderer;
class svtkStringArray;
class svtkTextProperty;

class SVTKRENDERINGLABEL_EXPORT svtkLabelSizeCalculator : public svtkPassInputTypeAlgorithm
{
public:
  static svtkLabelSizeCalculator* New();
  void PrintSelf(ostream& os, svtkIndent indent) override;
  svtkTypeMacro(svtkLabelSizeCalculator, svtkPassInputTypeAlgorithm);

  //@{
  /**
   * Get/Set the font used compute label sizes.
   * This defaults to "Arial" at 12 points.
   * If type is provided, it refers to the type of the text label provided
   * in the optional label type array. The default type is type 0.
   */
  virtual void SetFontProperty(svtkTextProperty* fontProp, int type = 0);
  virtual svtkTextProperty* GetFontProperty(int type = 0);
  //@}

  //@{
  /**
   * The name of the output array containing text label sizes
   * This defaults to "LabelSize"
   */
  svtkSetStringMacro(LabelSizeArrayName);
  svtkGetStringMacro(LabelSizeArrayName);
  //@}

  //@{
  /**
   * Get/Set the DPI at which the labels are to be rendered. Defaults to 72.
   * @sa svtkWindow::GetDPI()
   */
  svtkSetMacro(DPI, int);
  svtkGetMacro(DPI, int);
  //@}

protected:
  svtkLabelSizeCalculator();
  ~svtkLabelSizeCalculator() override;

  int FillInputPortInformation(int port, svtkInformation* info) override;
  int RequestData(
    svtkInformation* request, svtkInformationVector** inInfo, svtkInformationVector* outInfo) override;

  virtual svtkIntArray* LabelSizesForArray(svtkAbstractArray* labels, svtkIntArray* types);

  virtual void SetFontUtil(svtkTextRenderer* fontProp);
  svtkGetObjectMacro(FontUtil, svtkTextRenderer);

  svtkTextRenderer* FontUtil;
  char* LabelSizeArrayName;

  int DPI;

  class Internals;
  Internals* Implementation;

private:
  svtkLabelSizeCalculator(const svtkLabelSizeCalculator&) = delete;
  void operator=(const svtkLabelSizeCalculator&) = delete;
};

#endif // svtkLabelSizeCalculator_h
