/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkExtractTensorComponents.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkExtractTensorComponents
 * @brief   extract parts of tensor and create a scalar, vector, normal, or texture coordinates.
 *
 * svtkExtractTensorComponents is a filter that extracts components of
 * a tensor to create a scalar, vector, normal, or texture coords. For
 * example, if the tensor contains components of stress, then you
 * could extract the normal stress in the x-direction as a scalar
 * (i.e., tensor component (0,0).
 *
 * To use this filter, you must set some boolean flags to control
 * which data is extracted from the tensors, and whether you want to
 * pass the tensor data through to the output. Also, you must specify
 * the tensor component(s) for each type of data you want to
 * extract. The tensor component(s) is(are) specified using matrix notation
 * into a 3x3 matrix. That is, use the (row,column) address to specify
 * a particular tensor component; and if the data you are extracting
 * requires more than one component, use a list of addresses. (Note
 * that the addresses are 0-offset -> (0,0) specifies upper left
 * corner of the tensor.)
 *
 * There are two optional methods to extract scalar data. You can
 * extract the determinant of the tensor, or you can extract the
 * effective stress of the tensor. These require that the ivar
 * ExtractScalars is on, and the appropriate scalar extraction mode is
 * set.
 */

#ifndef svtkExtractTensorComponents_h
#define svtkExtractTensorComponents_h

#include "svtkDataSetAlgorithm.h"
#include "svtkFiltersExtractionModule.h" // For export macro

#define SVTK_EXTRACT_COMPONENT 0
#define SVTK_EXTRACT_EFFECTIVE_STRESS 1
#define SVTK_EXTRACT_DETERMINANT 2

class SVTKFILTERSEXTRACTION_EXPORT svtkExtractTensorComponents : public svtkDataSetAlgorithm
{
public:
  svtkTypeMacro(svtkExtractTensorComponents, svtkDataSetAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct object to extract nothing and to not pass tensor data
   * through the pipeline.
   */
  static svtkExtractTensorComponents* New();

  //@{
  /**
   * Boolean controls whether tensor data is passed through to output.
   */
  svtkSetMacro(PassTensorsToOutput, svtkTypeBool);
  svtkGetMacro(PassTensorsToOutput, svtkTypeBool);
  svtkBooleanMacro(PassTensorsToOutput, svtkTypeBool);
  //@}

  //@{
  /**
   * Boolean controls whether scalar data is extracted from tensor.
   */
  svtkSetMacro(ExtractScalars, svtkTypeBool);
  svtkGetMacro(ExtractScalars, svtkTypeBool);
  svtkBooleanMacro(ExtractScalars, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the (row,column) tensor component to extract as a scalar.
   */
  svtkSetVector2Macro(ScalarComponents, int);
  svtkGetVectorMacro(ScalarComponents, int, 2);
  //@}

  //@{
  /**
   * Specify how to extract the scalar. You can extract it as one of
   * the components of the tensor, as effective stress, or as the
   * determinant of the tensor. If you extract a component make sure
   * that you set the ScalarComponents ivar.
   */
  svtkSetMacro(ScalarMode, int);
  svtkGetMacro(ScalarMode, int);
  void SetScalarModeToComponent() { this->SetScalarMode(SVTK_EXTRACT_COMPONENT); }
  void SetScalarModeToEffectiveStress() { this->SetScalarMode(SVTK_EXTRACT_EFFECTIVE_STRESS); }
  void SetScalarModeToDeterminant() { this->SetScalarMode(SVTK_EXTRACT_DETERMINANT); }
  void ScalarIsComponent() { this->SetScalarMode(SVTK_EXTRACT_COMPONENT); }
  void ScalarIsEffectiveStress() { this->SetScalarMode(SVTK_EXTRACT_EFFECTIVE_STRESS); }
  void ScalarIsDeterminant() { this->SetScalarMode(SVTK_EXTRACT_DETERMINANT); }
  //@}

  //@{
  /**
   * Boolean controls whether vector data is extracted from tensor.
   */
  svtkSetMacro(ExtractVectors, svtkTypeBool);
  svtkGetMacro(ExtractVectors, svtkTypeBool);
  svtkBooleanMacro(ExtractVectors, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the ((row,column)0,(row,column)1,(row,column)2) tensor
   * components to extract as a vector.
   */
  svtkSetVector6Macro(VectorComponents, int);
  svtkGetVectorMacro(VectorComponents, int, 6);
  //@}

  //@{
  /**
   * Boolean controls whether normal data is extracted from tensor.
   */
  svtkSetMacro(ExtractNormals, svtkTypeBool);
  svtkGetMacro(ExtractNormals, svtkTypeBool);
  svtkBooleanMacro(ExtractNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Boolean controls whether normal vector is converted to unit normal
   * after extraction.
   */
  svtkSetMacro(NormalizeNormals, svtkTypeBool);
  svtkGetMacro(NormalizeNormals, svtkTypeBool);
  svtkBooleanMacro(NormalizeNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Specify the ((row,column)0,(row,column)1,(row,column)2) tensor
   * components to extract as a vector.
   */
  svtkSetVector6Macro(NormalComponents, int);
  svtkGetVectorMacro(NormalComponents, int, 6);
  //@}

  //@{
  /**
   * Boolean controls whether texture coordinates are extracted from tensor.
   */
  svtkSetMacro(ExtractTCoords, svtkTypeBool);
  svtkGetMacro(ExtractTCoords, svtkTypeBool);
  svtkBooleanMacro(ExtractTCoords, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the dimension of the texture coordinates to extract.
   */
  svtkSetClampMacro(NumberOfTCoords, int, 1, 3);
  svtkGetMacro(NumberOfTCoords, int);
  //@}

  //@{
  /**
   * Specify the ((row,column)0,(row,column)1,(row,column)2) tensor
   * components to extract as a vector. Up to NumberOfTCoords
   * components are extracted.
   */
  svtkSetVector6Macro(TCoordComponents, int);
  svtkGetVectorMacro(TCoordComponents, int, 6);
  //@}

protected:
  svtkExtractTensorComponents();
  ~svtkExtractTensorComponents() override {}

  int RequestData(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  svtkTypeBool PassTensorsToOutput;

  svtkTypeBool ExtractScalars;
  svtkTypeBool ExtractVectors;
  svtkTypeBool ExtractNormals;
  svtkTypeBool ExtractTCoords;

  int ScalarMode;
  int ScalarComponents[2];

  int VectorComponents[6];

  svtkTypeBool NormalizeNormals;
  int NormalComponents[6];

  int NumberOfTCoords;
  int TCoordComponents[6];

private:
  svtkExtractTensorComponents(const svtkExtractTensorComponents&) = delete;
  void operator=(const svtkExtractTensorComponents&) = delete;
};

#endif
