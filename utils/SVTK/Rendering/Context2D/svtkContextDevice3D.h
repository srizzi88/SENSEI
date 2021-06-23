/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkContextDevice3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkContextDevice3D
 * @brief   Abstract class for drawing 3D primitives.
 *
 *
 * This defines the interface for a svtkContextDevice3D. In this sense a
 * ContextDevice is a class used to paint 3D primitives onto a device, such as
 * an OpenGL context.
 *
 * This is private API, and should not be used outside of the svtkContext3D.
 */

#ifndef svtkContextDevice3D_h
#define svtkContextDevice3D_h

#include "svtkObject.h"
#include "svtkRect.h"                     // For the rectangles..
#include "svtkRenderingContext2DModule.h" // For export macro
#include "svtkVector.h"                   // For the vector coordinates.

class svtkMatrix4x4;
class svtkViewport;
class svtkPen;
class svtkBrush;

class SVTKRENDERINGCONTEXT2D_EXPORT svtkContextDevice3D : public svtkObject
{
public:
  svtkTypeMacro(svtkContextDevice3D, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkContextDevice3D* New();

  /**
   * Draw a polyline between the specified points.
   * \sa DrawLines()
   */
  virtual void DrawPoly(
    const float* verts, int n, const unsigned char* colors = nullptr, int nc = 0) = 0;

  /**
   * Draw lines defined by specified pair of points.
   * \sa DrawPoly()
   */
  virtual void DrawLines(
    const float* verts, int n, const unsigned char* colors = nullptr, int nc = 0) = 0;

  /**
   * Draw points at the vertex positions specified.
   */
  virtual void DrawPoints(
    const float* verts, int n, const unsigned char* colors = nullptr, int nc = 0) = 0;

  /**
   * Draw triangles to generate the specified mesh.
   */
  virtual void DrawTriangleMesh(const float* mesh, int n, const unsigned char* colors, int nc) = 0;

  /**
   * Apply the supplied pen which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the svtkPen
   * object in the svtkContext2D, it does not hold a pointer to the supplied object.
   */
  virtual void ApplyPen(svtkPen* pen) = 0;

  /**
   * Apply the supplied brush which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the svtkBrush
   * object in the svtkContext2D, it does not hold a pointer to the supplied object.
   */
  virtual void ApplyBrush(svtkBrush* brush) = 0;

  /**
   * Set the model view matrix for the display
   */
  virtual void SetMatrix(svtkMatrix4x4* m) = 0;

  /**
   * Set the model view matrix for the display
   */
  virtual void GetMatrix(svtkMatrix4x4* m) = 0;

  /**
   * Multiply the current model view matrix by the supplied one
   */
  virtual void MultiplyMatrix(svtkMatrix4x4* m) = 0;

  /**
   * Push the current matrix onto the stack.
   */
  virtual void PushMatrix() = 0;

  /**
   * Pop the current matrix off of the stack.
   */
  virtual void PopMatrix() = 0;

  /**
   * Supply a float array of length 4 with x1, y1, width, height specifying
   * clipping region for the device in pixels.
   */
  virtual void SetClipping(const svtkRecti& rect) = 0;

  /**
   * Disable clipping of the display.
   * Remove in a future release - retained for API compatibility.
   */
  virtual void DisableClipping() { this->EnableClipping(false); }

  /**
   * Enable or disable the clipping of the scene.
   */
  virtual void EnableClipping(bool enable) = 0;

  //@{
  /**
   * Enable/Disable the specified clipping plane.
   */
  virtual void EnableClippingPlane(int i, double* planeEquation) = 0;
  virtual void DisableClippingPlane(int i) = 0;
  //@}

protected:
  svtkContextDevice3D();
  ~svtkContextDevice3D() override;

private:
  svtkContextDevice3D(const svtkContextDevice3D&) = delete;
  void operator=(const svtkContextDevice3D&) = delete;
};

#endif
