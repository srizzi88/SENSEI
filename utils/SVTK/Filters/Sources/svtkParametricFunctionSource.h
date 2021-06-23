/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricFunctionSource.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParametricFunctionSource
 * @brief   tessellate parametric functions
 *
 * This class tessellates parametric functions. The user must specify how
 * many points in the parametric coordinate directions are required (i.e.,
 * the resolution), and the mode to use to generate scalars.
 *
 * @par Thanks:
 * Andrew Maclean andrew.amaclean@gmail.com for creating and contributing
 * the class.
 *
 * @sa
 * svtkParametricFunction
 *
 * @sa
 * Implementation of parametrics for 1D lines:
 * svtkParametricSpline
 *
 * @sa
 * Subclasses of svtkParametricFunction implementing non-orentable surfaces:
 * svtkParametricBoy svtkParametricCrossCap svtkParametricFigure8Klein
 * svtkParametricKlein svtkParametricMobius svtkParametricRoman
 *
 * @sa
 * Subclasses of svtkParametricFunction implementing orientable surfaces:
 * svtkParametricConicSpiral svtkParametricDini svtkParametricEllipsoid
 * svtkParametricEnneper svtkParametricRandomHills svtkParametricSuperEllipsoid
 * svtkParametricSuperToroid svtkParametricTorus
 *
 */

#ifndef svtkParametricFunctionSource_h
#define svtkParametricFunctionSource_h

#include "svtkFiltersSourcesModule.h" // For export macro
#include "svtkPolyDataAlgorithm.h"

class svtkCellArray;
class svtkParametricFunction;

class SVTKFILTERSSOURCES_EXPORT svtkParametricFunctionSource : public svtkPolyDataAlgorithm
{
public:
  svtkTypeMacro(svtkParametricFunctionSource, svtkPolyDataAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Create a new instance with (50,50,50) points in the (u-v-w) directions.
   */
  static svtkParametricFunctionSource* New();

  //@{
  /**
   * Specify the parametric function to use to generate the tessellation.
   */
  virtual void SetParametricFunction(svtkParametricFunction*);
  svtkGetObjectMacro(ParametricFunction, svtkParametricFunction);
  //@}

  //@{
  /**
   * Set/Get the number of subdivisions / tessellations in the u parametric
   * direction. Note that the number of tessellant points in the u
   * direction is the UResolution + 1.
   */
  svtkSetClampMacro(UResolution, int, 2, SVTK_INT_MAX);
  svtkGetMacro(UResolution, int);
  //@}

  //@{
  /**
   * Set/Get the number of subdivisions / tessellations in the v parametric
   * direction. Note that the number of tessellant points in the v
   * direction is the VResolution + 1.
   */
  svtkSetClampMacro(VResolution, int, 2, SVTK_INT_MAX);
  svtkGetMacro(VResolution, int);
  //@}

  //@{
  /**
   * Set/Get the number of subdivisions / tessellations in the w parametric
   * direction. Note that the number of tessellant points in the w
   * direction is the WResolution + 1.
   */
  svtkSetClampMacro(WResolution, int, 2, SVTK_INT_MAX);
  svtkGetMacro(WResolution, int);
  //@}

  //@{
  /**
   * Set/Get the generation of texture coordinates. This is off by
   * default.
   * Note that this is only applicable to parametric surfaces
   * whose parametric dimension is 2.
   * Note that texturing may fail in some cases.
   */
  svtkBooleanMacro(GenerateTextureCoordinates, svtkTypeBool);
  svtkSetClampMacro(GenerateTextureCoordinates, svtkTypeBool, 0, 1);
  svtkGetMacro(GenerateTextureCoordinates, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the generation of normals. This is on by
   * default.
   * Note that this is only applicable to parametric surfaces
   * whose parametric dimension is 2.
   */
  svtkBooleanMacro(GenerateNormals, svtkTypeBool);
  svtkSetClampMacro(GenerateNormals, svtkTypeBool, 0, 1);
  svtkGetMacro(GenerateNormals, svtkTypeBool);
  //@}

  /**
   * Enumerate the supported scalar generation modes.<br>
   * SCALAR_NONE - Scalars are not generated (default).<br>
   * SCALAR_U - The scalar is set to the u-value.<br>
   * SCALAR_V - The scalar is set to the v-value.<br>
   * SCALAR_U0 - The scalar is set to 1 if
   * u = (u_max - u_min)/2 = u_avg, 0 otherwise.<br>
   * SCALAR_V0 - The scalar is set to 1 if
   * v = (v_max - v_min)/2 = v_avg, 0 otherwise.<br>
   * SCALAR_U0V0 - The scalar is
   * set to 1 if u == u_avg, 2 if v == v_avg,
   * 3 if u = u_avg && v = v_avg, 0 otherwise.<br>
   * SCALAR_MODULUS - The scalar is set to (sqrt(u*u+v*v)),
   * this is measured relative to (u_avg,v_avg).<br>
   * SCALAR_PHASE - The scalar is set to (atan2(v,u))
   * (in degrees, 0 to 360),
   * this is measured relative to (u_avg,v_avg).<br>
   * SCALAR_QUADRANT - The scalar is set to 1, 2, 3 or 4.
   * depending upon the quadrant of the point (u,v).<br>
   * SCALAR_X - The scalar is set to the x-value.<br>
   * SCALAR_Y - The scalar is set to the y-value.<br>
   * SCALAR_Z - The scalar is set to the z-value.<br>
   * SCALAR_DISTANCE - The scalar is set to (sqrt(x*x+y*y+z*z)).
   * I.e. distance from the origin.<br>
   * SCALAR_USER_DEFINED - The scalar is set to the value
   * returned from EvaluateScalar().<br>
   */
  enum SCALAR_MODE
  {
    SCALAR_NONE = 0,
    SCALAR_U,
    SCALAR_V,
    SCALAR_U0,
    SCALAR_V0,
    SCALAR_U0V0,
    SCALAR_MODULUS,
    SCALAR_PHASE,
    SCALAR_QUADRANT,
    SCALAR_X,
    SCALAR_Y,
    SCALAR_Z,
    SCALAR_DISTANCE,
    SCALAR_FUNCTION_DEFINED
  };

  //@{
  /**
   * Get/Set the mode used for the scalar data.
   * See SCALAR_MODE for a description of the types of scalars generated.
   */
  svtkSetClampMacro(ScalarMode, int, SCALAR_NONE, SCALAR_FUNCTION_DEFINED);
  svtkGetMacro(ScalarMode, int);
  void SetScalarModeToNone(void) { this->SetScalarMode(SCALAR_NONE); }
  void SetScalarModeToU(void) { this->SetScalarMode(SCALAR_U); }
  void SetScalarModeToV(void) { this->SetScalarMode(SCALAR_V); }
  void SetScalarModeToU0(void) { this->SetScalarMode(SCALAR_U0); }
  void SetScalarModeToV0(void) { this->SetScalarMode(SCALAR_V0); }
  void SetScalarModeToU0V0(void) { this->SetScalarMode(SCALAR_U0V0); }
  void SetScalarModeToModulus(void) { this->SetScalarMode(SCALAR_MODULUS); }
  void SetScalarModeToPhase(void) { this->SetScalarMode(SCALAR_PHASE); }
  void SetScalarModeToQuadrant(void) { this->SetScalarMode(SCALAR_QUADRANT); }
  void SetScalarModeToX(void) { this->SetScalarMode(SCALAR_X); }
  void SetScalarModeToY(void) { this->SetScalarMode(SCALAR_Y); }
  void SetScalarModeToZ(void) { this->SetScalarMode(SCALAR_Z); }
  void SetScalarModeToDistance(void) { this->SetScalarMode(SCALAR_DISTANCE); }
  void SetScalarModeToFunctionDefined(void) { this->SetScalarMode(SCALAR_FUNCTION_DEFINED); }
  //@}

  /**
   * Return the MTime also considering the parametric function.
   */
  svtkMTimeType GetMTime() override;

  //@{
  /**
   * Set/get the desired precision for the output points.
   * See the documentation for the svtkAlgorithm::Precision enum for an
   * explanation of the available precision settings.
   */
  svtkSetMacro(OutputPointsPrecision, int);
  svtkGetMacro(OutputPointsPrecision, int);
  //@}

protected:
  svtkParametricFunctionSource();
  ~svtkParametricFunctionSource() override;

  // Usual data generation method
  int RequestData(
    svtkInformation* info, svtkInformationVector** input, svtkInformationVector* output) override;

  // Variables
  svtkParametricFunction* ParametricFunction;

  int UResolution;
  int VResolution;
  int WResolution;
  svtkTypeBool GenerateTextureCoordinates;
  svtkTypeBool GenerateNormals;
  int ScalarMode;
  int OutputPointsPrecision;

private:
  // Create output depending on function dimension
  void Produce1DOutput(svtkInformationVector* output);
  void Produce2DOutput(svtkInformationVector* output);

  /**
   * Generate triangles from an ordered set of points.

   * Given a parametrization f(u,v)->(x,y,z), this function generates
   * a svtkCellAarray of point IDs over the range MinimumU <= u < MaximumU
   * and MinimumV <= v < MaximumV.

   * Before using this function, ensure that: UResolution,
   * VResolution, MinimumU, MaximumU, MinimumV, MaximumV, JoinU, JoinV,
   * TwistU, TwistV, ordering are set appropriately for the parametric function.
   */
  void MakeTriangles(svtkCellArray* strips, int PtsU, int PtsV);

  svtkParametricFunctionSource(const svtkParametricFunctionSource&) = delete;
  void operator=(const svtkParametricFunctionSource&) = delete;
};

#endif
