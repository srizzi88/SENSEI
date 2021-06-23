/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContext3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkContext3D
 * @brief   Class for drawing 3D primitives to a graphical context.
 *
 *
 * This defines the interface for drawing onto a 3D context. The context must
 * be set up with a svtkContextDevice3D derived class that provides the functions
 * to facilitate the low level calls to the context. Currently only an OpenGL
 * based device is provided.
 */

#ifndef svtkContext3D_h
#define svtkContext3D_h

#include "svtkObject.h"
#include "svtkRenderingContext2DModule.h" // For export macro
#include "svtkSmartPointer.h"             // For SP ivars.
#include "svtkVector.h"                   // For the vector coordinates.

class svtkContextDevice3D;
class svtkPen;
class svtkBrush;
class svtkTransform;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkContext3D : public svtkObject
{
public:
  svtkTypeMacro(svtkContext3D, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Creates a 3D context object.
   */
  static svtkContext3D* New();

  /**
   * Begin painting on a svtkContextDevice3D, no painting can occur before this
   * call has been made. Only one painter is allowed at a time on any given
   * paint device. Returns true if successful, otherwise false.
   */
  bool Begin(svtkContextDevice3D* device);

  /**
   * Get access to the underlying 3D context.
   */
  svtkContextDevice3D* GetDevice();

  /**
   * Ends painting on the device, you would not usually need to call this as it
   * should be called by the destructor. Returns true if the painter is no
   * longer active, otherwise false.
   */
  bool End();

  /**
   * Draw a line between the specified points.
   */
  void DrawLine(const svtkVector3f& start, const svtkVector3f& end);

  /**
   * Draw a poly line between the specified points.
   */
  void DrawPoly(const float* points, int n);

  /**
   * Draw a point at the point in 3D space.
   */
  void DrawPoint(const svtkVector3f& point);

  /**
   * Draw a sequence of points at the specified locations.
   */
  void DrawPoints(const float* points, int n);

  /**
   * Draw a sequence of points at the specified locations.  The points will be
   * colored by the colors array, which must have nc_comps components
   * (defining a single color).
   */
  void DrawPoints(const float* points, int n, unsigned char* colors, int nc_comps);

  /**
   * Draw triangles to generate the specified mesh.
   */
  void DrawTriangleMesh(const float* mesh, int n, const unsigned char* colors, int nc);

  /**
   * Apply the supplied pen which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the svtkPen
   * object in the svtkContext2D, it does not hold a pointer to the supplied object.
   */
  void ApplyPen(svtkPen* pen);

  /**
   * Apply the supplied brush which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the svtkBrush
   * object in the svtkContext2D, it does not hold a pointer to the supplied object.
   */
  void ApplyBrush(svtkBrush* brush);

  /**
   * Set the transform for the context, the underlying device will use the
   * matrix of the transform. Note, this is set immediately, later changes to
   * the matrix will have no effect until it is set again.
   */
  void SetTransform(svtkTransform* transform);

  /**
   * Compute the current transform applied to the context.
   */
  svtkTransform* GetTransform();

  /**
   * Append the transform for the context, the underlying device will use the
   * matrix of the transform. Note, this is set immediately, later changes to
   * the matrix will have no effect until it is set again. The matrix of the
   * transform will multiply the current context transform.
   */
  void AppendTransform(svtkTransform* transform);

  //@{
  /**
   * Push/pop the transformation matrix for the painter (sets the underlying
   * matrix for the device when available).
   */
  void PushMatrix();
  void PopMatrix();
  //@}

  //@{
  /**
   * Enable/Disable the specified clipping plane.
   * i is the index of the clipping plane being enabled or disabled (0 - 5).
   * planeEquation points to the four coefficients of the equation for the
   * clipping plane: Ax + By + Cz + D = 0.  This is the equation format
   * expected by glClipPlane.
   */
  void EnableClippingPlane(int i, double* planeEquation);
  void DisableClippingPlane(int i);
  //@}

protected:
  svtkContext3D();
  ~svtkContext3D() override;

  svtkSmartPointer<svtkContextDevice3D> Device; // The underlying device
  svtkSmartPointer<svtkTransform> Transform;    // Current transform

private:
  svtkContext3D(const svtkContext3D&) = delete;
  void operator=(const svtkContext3D&) = delete;
};

#endif // SVTKCONTEXT3D_H
