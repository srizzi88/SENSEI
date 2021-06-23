/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkParametricFunction.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkParametricFunction
 * @brief   abstract interface for parametric functions
 *
 * svtkParametricFunction is an abstract interface for functions
 * defined by parametric mapping i.e. f(u,v,w)->(x,y,z) where
 * u_min <= u < u_max, v_min <= v < v_max, w_min <= w < w_max. (For
 * notational convenience, we will write f(u)->x and assume that
 * u means (u,v,w) and x means (x,y,z).)
 *
 * The interface contains the pure virtual function, Evaluate(), that
 * generates a point and the derivatives at that point which are then used to
 * construct the surface. A second pure virtual function, EvaluateScalar(),
 * can be used to generate a scalar for the surface. Finally, the
 * GetDimension() virtual function is used to differentiate 1D, 2D, and 3D
 * parametric functions. Since this abstract class defines a pure virtual
 * API, its subclasses must implement the pure virtual functions
 * GetDimension(), Evaluate() and EvaluateScalar().
 *
 * This class has also methods for defining a range of parametric values (u,v,w).
 *
 * @par Thanks:
 * Andrew Maclean andrew.amaclean@gmail.com for creating and contributing the
 * class.
 *
 * @sa
 * svtkParametricFunctionSource - tessellates a parametric function
 *
 * @sa
 * Implementations of derived classes implementing non-orentable surfaces:
 * svtkParametricBoy svtkParametricCrossCap svtkParametricFigure8Klein
 * svtkParametricKlein svtkParametricMobius svtkParametricRoman
 *
 * @sa
 * Implementations of derived classes implementing orientable surfaces:
 * svtkParametricConicSpiral svtkParametricDini svtkParametricEllipsoid
 * svtkParametricEnneper svtkParametricRandomHills svtkParametricSuperEllipsoid
 * svtkParametricSuperToroid svtkParametricTorus
 *
 */

#ifndef svtkParametricFunction_h
#define svtkParametricFunction_h

#include "svtkCommonComputationalGeometryModule.h" // For export macro
#include "svtkObject.h"

class SVTKCOMMONCOMPUTATIONALGEOMETRY_EXPORT svtkParametricFunction : public svtkObject
{
public:
  svtkTypeMacro(svtkParametricFunction, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Return the dimension of parametric space. Depending on the dimension,
   * then the (u,v,w) parameters and associated information (e.g., derivates)
   * have meaning. For example, if the dimension of the function is one, then
   * u[0] and Duvw[0...2] have meaning.
   * This is a pure virtual function that must be instantiated in
   * a derived class.
   */
  virtual int GetDimension() = 0;

  /**
   * Performs the mapping \$f(uvw)->(Pt,Duvw)\$f.
   * This is a pure virtual function that must be instantiated in
   * a derived class.

   * uvw are the parameters, with u corresponding to uvw[0],
   * v to uvw[1] and w to uvw[2] respectively. Pt is the returned Cartesian point,
   * Duvw are the derivatives of this point with respect to u, v and w.
   * Note that the first three values in Duvw are Du, the next three are Dv,
   * and the final three are Dw. Du Dv Dw are the partial derivatives of the
   * function at the point Pt with respect to u, v and w respectively.
   */
  virtual void Evaluate(double uvw[3], double Pt[3], double Duvw[9]) = 0;

  /**
   * Calculate a user defined scalar using one or all of uvw, Pt, Duvw.
   * This is a pure virtual function that must be instantiated in
   * a derived class.

   * uvw are the parameters with Pt being the cartesian point,
   * Duvw are the derivatives of this point with respect to u, v, and w.
   * Pt, Duvw are obtained from Evaluate().
   */
  virtual double EvaluateScalar(double uvw[3], double Pt[3], double Duvw[9]) = 0;

  //@{
  /**
   * Set/Get the minimum u-value.
   */
  svtkSetMacro(MinimumU, double);
  svtkGetMacro(MinimumU, double);
  //@}

  //@{
  /**
   * Set/Get the maximum u-value.
   */
  svtkSetMacro(MaximumU, double);
  svtkGetMacro(MaximumU, double);
  //@}

  //@{
  /**
   * Set/Get the minimum v-value.
   */
  svtkSetMacro(MinimumV, double);
  svtkGetMacro(MinimumV, double);
  //@}

  //@{
  /**
   * Set/Get the maximum v-value.
   */
  svtkSetMacro(MaximumV, double);
  svtkGetMacro(MaximumV, double);
  //@}

  //@{
  /**
   * Set/Get the minimum w-value.
   */
  svtkSetMacro(MinimumW, double);
  svtkGetMacro(MinimumW, double);
  //@}

  //@{
  /**
   * Set/Get the maximum w-value.
   */
  svtkSetMacro(MaximumW, double);
  svtkGetMacro(MaximumW, double);
  //@}

  //@{
  /**
   * Set/Get the flag which joins the first triangle strip to the last one.
   */
  svtkSetClampMacro(JoinU, svtkTypeBool, 0, 1);
  svtkGetMacro(JoinU, svtkTypeBool);
  svtkBooleanMacro(JoinU, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the flag which joins the ends of the triangle strips.
   */
  svtkSetClampMacro(JoinV, svtkTypeBool, 0, 1);
  svtkGetMacro(JoinV, svtkTypeBool);
  svtkBooleanMacro(JoinV, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the flag which joins the ends of the triangle strips.
   */
  svtkSetClampMacro(JoinW, svtkTypeBool, 0, 1);
  svtkGetMacro(JoinW, svtkTypeBool);
  svtkBooleanMacro(JoinW, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the flag which joins the first triangle strip to
   * the last one with a twist.
   * JoinU must also be set if this is set.
   * Used when building some non-orientable surfaces.
   */
  svtkSetClampMacro(TwistU, svtkTypeBool, 0, 1);
  svtkGetMacro(TwistU, svtkTypeBool);
  svtkBooleanMacro(TwistU, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the flag which joins the ends of the
   * triangle strips with a twist.
   * JoinV must also be set if this is set.
   * Used when building some non-orientable surfaces.
   */
  svtkSetClampMacro(TwistV, svtkTypeBool, 0, 1);
  svtkGetMacro(TwistV, svtkTypeBool);
  svtkBooleanMacro(TwistV, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the flag which joins the ends of the
   * triangle strips with a twist.
   * JoinW must also be set if this is set.
   * Used when building some non-orientable surfaces.
   */
  svtkSetClampMacro(TwistW, svtkTypeBool, 0, 1);
  svtkGetMacro(TwistW, svtkTypeBool);
  svtkBooleanMacro(TwistW, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the flag which determines the ordering of the
   * vertices forming the triangle strips. The ordering of the
   * points being inserted into the triangle strip is important
   * because it determines the direction of the normals for the
   * lighting. If set, the ordering is clockwise, otherwise the
   * ordering is anti-clockwise. Default is true (i.e. clockwise
   * ordering).
   */
  svtkSetClampMacro(ClockwiseOrdering, svtkTypeBool, 0, 1);
  svtkGetMacro(ClockwiseOrdering, svtkTypeBool);
  svtkBooleanMacro(ClockwiseOrdering, svtkTypeBool);
  //@}

  //@{
  /**
   * Set/Get the flag which determines whether derivatives are available
   * from the parametric function (i.e., whether the Evaluate() method
   * returns valid derivatives).
   */
  svtkSetClampMacro(DerivativesAvailable, svtkTypeBool, 0, 1);
  svtkGetMacro(DerivativesAvailable, svtkTypeBool);
  svtkBooleanMacro(DerivativesAvailable, svtkTypeBool);
  //@}

protected:
  svtkParametricFunction();
  ~svtkParametricFunction() override;

  // Variables
  double MinimumU;
  double MaximumU;
  double MinimumV;
  double MaximumV;
  double MinimumW;
  double MaximumW;

  svtkTypeBool JoinU;
  svtkTypeBool JoinV;
  svtkTypeBool JoinW;

  svtkTypeBool TwistU;
  svtkTypeBool TwistV;
  svtkTypeBool TwistW;

  svtkTypeBool ClockwiseOrdering;

  svtkTypeBool DerivativesAvailable;

private:
  svtkParametricFunction(const svtkParametricFunction&) = delete;
  void operator=(const svtkParametricFunction&) = delete;
};

#endif
