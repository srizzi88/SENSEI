/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractVectorComponents.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractVectorComponents
 * @brief   extract components of vector as separate scalars
 *
 * svtkExtractVectorComponents is a filter that extracts vector components as
 * separate scalars. This is accomplished by creating three different outputs.
 * Each output is the same as the input, except that the scalar values will be
 * one of the three components of the vector. These can be found in the
 * VxComponent, VyComponent, and VzComponent.
 * Alternatively, if the ExtractToFieldData flag is set, the filter will
 * put all the components in the field data. The first component will be
 * the scalar and the others will be non-attribute arrays.
 *
 * @warning
 * This filter is unusual in that it creates multiple outputs.
 * If you use the GetOutput() method, you will be retrieving the x vector
 * component.
 */

#ifndef svtkExtractVectorComponents_h
#define svtkExtractVectorComponents_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersExtractionModule.h" // For export macro

class svtkDataSet;

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractVectorComponents : public svtkDataSetAlgorithm
{
public:
  static svtkExtractVectorComponents* New();
  svtkTypeMacro(svtkExtractVectorComponents, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Specify the input data or filter.
   */
  virtual void SetInputData(svtkDataSet* input);

  /**
   * Get the output dataset representing velocity x-component. If output is
   * nullptr then input hasn't been set, which is necessary for abstract
   * objects. (Note: this method returns the same information as the
   * GetOutput() method with an index of 0.)
   */
  svtkDataSet* GetVxComponent();

  /**
   * Get the output dataset representing velocity y-component. If output is
   * nullptr then input hasn't been set, which is necessary for abstract
   * objects. (Note: this method returns the same information as the
   * GetOutput() method with an index of 1.)
   * Note that if ExtractToFieldData is true, this output will be empty.
   */
  svtkDataSet* GetVyComponent();

  /**
   * Get the output dataset representing velocity z-component. If output is
   * nullptr then input hasn't been set, which is necessary for abstract
   * objects. (Note: this method returns the same information as the
   * GetOutput() method with an index of 2.)
   * Note that if ExtractToFieldData is true, this output will be empty.
   */
  svtkDataSet* GetVzComponent();

  //@{
  /**
   * Determines whether the vector components will be put
   * in separate outputs or in the first output's field data
   */
  svtkSetMacro(ExtractToFieldData, svtkTypeBool);
  svtkGetMacro(ExtractToFieldData, svtkTypeBool);
  svtkBooleanMacro(ExtractToFieldData, svtkTypeBool);
  //@}

protected:
  svtkExtractVectorComponents();
  ~svtkExtractVectorComponents() override;

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  svtkTypeBool ExtractToFieldData;
  int OutputsInitialized;

private:
  svtkExtractVectorComponents(const svtkExtractVectorComponents&) = delete;
  void operator=(const svtkExtractVectorComponents&) = delete;
};

#endif
