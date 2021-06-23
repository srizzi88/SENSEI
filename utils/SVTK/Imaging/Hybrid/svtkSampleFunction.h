/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSampleFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSampleFunction
 * @brief   sample an implicit function over a structured point set
 *
 * svtkSampleFunction is a source object that evaluates an implicit function
 * and normals at each point in a svtkStructuredPoints. The user can specify
 * the sample dimensions and location in space to perform the sampling. To
 * create closed surfaces (in conjunction with the svtkContourFilter), capping
 * can be turned on to set a particular value on the boundaries of the sample
 * space.
 *
 * @sa
 * svtkImplicitModeller
 */

#ifndef svtkSampleFunction_h
#define svtkSampleFunction_h

#include "svtkImageAlgorithm.h"
#include "svtkImagingHybridModule.h" // For export macro

class svtkImplicitFunction;
class svtkDataArray;

class SVTKIMAGINGHYBRID_EXPORT svtkSampleFunction : public svtkImageAlgorithm
{
public:
  svtkTypeMacro(svtkSampleFunction, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct with ModelBounds=(-1,1,-1,1,-1,1), SampleDimensions=(50,50,50),
   * Capping turned off, and normal generation on.
   */
  static svtkSampleFunction* New();

  //@{
  /**
   * Specify the implicit function to use to generate data.
   */
  virtual void SetImplicitFunction(svtkImplicitFunction*);
  svtkGetObjectMacro(ImplicitFunction, svtkImplicitFunction);
  //@}

  //@{
  /**
   * Set what type of scalar data this source should generate.
   */
  svtkSetMacro(OutputScalarType, int);
  svtkGetMacro(OutputScalarType, int);
  void SetOutputScalarTypeToDouble() { this->SetOutputScalarType(SVTK_DOUBLE); }
  void SetOutputScalarTypeToFloat() { this->SetOutputScalarType(SVTK_FLOAT); }
  void SetOutputScalarTypeToLong() { this->SetOutputScalarType(SVTK_LONG); }
  void SetOutputScalarTypeToUnsignedLong() { this->SetOutputScalarType(SVTK_UNSIGNED_LONG); }
  void SetOutputScalarTypeToInt() { this->SetOutputScalarType(SVTK_INT); }
  void SetOutputScalarTypeToUnsignedInt() { this->SetOutputScalarType(SVTK_UNSIGNED_INT); }
  void SetOutputScalarTypeToShort() { this->SetOutputScalarType(SVTK_SHORT); }
  void SetOutputScalarTypeToUnsignedShort() { this->SetOutputScalarType(SVTK_UNSIGNED_SHORT); }
  void SetOutputScalarTypeToChar() { this->SetOutputScalarType(SVTK_CHAR); }
  void SetOutputScalarTypeToUnsignedChar() { this->SetOutputScalarType(SVTK_UNSIGNED_CHAR); }
  //@}

  /**
   * Specify the dimensions of the data on which to sample.
   */
  void SetSampleDimensions(int i, int j, int k);

  //@{
  /**
   * Specify the dimensions of the data on which to sample.
   */
  void SetSampleDimensions(int dim[3]);
  svtkGetVectorMacro(SampleDimensions, int, 3);
  //@}

  //@{
  /**
   * Specify the region in space over which the sampling occurs. The
   * bounds is specified as (xMin,xMax, yMin,yMax, zMin,zMax).
   */
  void SetModelBounds(const double bounds[6]);
  void SetModelBounds(double xMin, double xMax, double yMin, double yMax, double zMin, double zMax);
  svtkGetVectorMacro(ModelBounds, double, 6);
  //@}

  //@{
  /**
   * Turn on/off capping. If capping is on, then the outer boundaries of the
   * structured point set are set to cap value. This can be used to insure
   * surfaces are closed.
   */
  svtkSetMacro(Capping, svtkTypeBool);
  svtkGetMacro(Capping, svtkTypeBool);
  svtkBooleanMacro(Capping, svtkTypeBool);
  //@}

  //@{
  /**
   * Set the cap value.
   */
  svtkSetMacro(CapValue, double);
  svtkGetMacro(CapValue, double);
  //@}

  //@{
  /**
   * Turn on/off the computation of normals (normals are float values).
   */
  svtkSetMacro(ComputeNormals, svtkTypeBool);
  svtkGetMacro(ComputeNormals, svtkTypeBool);
  svtkBooleanMacro(ComputeNormals, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/get the scalar array name for this data set. Initial value is
   * "scalars".
   */
  svtkSetStringMacro(ScalarArrayName);
  svtkGetStringMacro(ScalarArrayName);
  //@}

  //@{
  /**
   * Set/get the normal array name for this data set. Initial value is
   * "normals".
   */
  svtkSetStringMacro(NormalArrayName);
  svtkGetStringMacro(NormalArrayName);
  //@}

  /**
   * Return the MTime also considering the implicit function.
   */
  svtkMTimeType GetMTime() override;

protected:
  /**
   * Default constructor.
   * Construct with ModelBounds=(-1,1,-1,1,-1,1), SampleDimensions=(50,50,50),
   * Capping turned off, CapValue=SVTK_DOUBLE_MAX, normal generation on,
   * OutputScalarType set to SVTK_DOUBLE, ImplicitFunction set to nullptr,
   * ScalarArrayName is "" and NormalArrayName is "".
   */
  svtkSampleFunction();

  ~svtkSampleFunction() override;

  void ReportReferences(svtkGarbageCollector*) override;

  void ExecuteDataWithInformation(svtkDataObject*, svtkInformation*) override;
  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;
  void Cap(svtkDataArray* s);

  int OutputScalarType;
  int SampleDimensions[3];
  double ModelBounds[6];
  svtkTypeBool Capping;
  double CapValue;
  svtkImplicitFunction* ImplicitFunction;
  svtkTypeBool ComputeNormals;
  char* ScalarArrayName;
  char* NormalArrayName;

private:
  svtkSampleFunction(const svtkSampleFunction&) = delete;
  void operator=(const svtkSampleFunction&) = delete;
};

#endif
