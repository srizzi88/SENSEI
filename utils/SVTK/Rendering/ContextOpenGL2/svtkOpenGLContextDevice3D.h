/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLContextDevice3D.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOpenGLContextDevice3D
 * @brief   OpenGL class drawing 3D primitives.
 *
 *
 * This defines the implementation of a 3D context device for drawing simple
 * primitives.
 */

#ifndef svtkOpenGLContextDevice3D_h
#define svtkOpenGLContextDevice3D_h

#include "svtkContextDevice3D.h"
#include "svtkNew.h"                           // For ivars.
#include "svtkRenderingContextOpenGL2Module.h" // For export macro
#include <vector>                             // STL Header

class svtkBrush;
class svtkOpenGLContextDevice2D;
class svtkOpenGLHelper;
class svtkOpenGLRenderWindow;
class svtkPen;
class svtkRenderer;
class svtkShaderProgram;
class svtkTransform;

class SVTKRENDERINGCONTEXTOPENGL2_EXPORT svtkOpenGLContextDevice3D : public svtkContextDevice3D
{
public:
  svtkTypeMacro(svtkOpenGLContextDevice3D, svtkContextDevice3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkOpenGLContextDevice3D* New();

  /**
   * Draw a polyline between the specified points.
   */
  void DrawPoly(const float* verts, int n, const unsigned char* colors, int nc) override;

  /**
   * Draw lines defined by specified pair of points.
   * \sa DrawPoly()
   */
  void DrawLines(const float* verts, int n, const unsigned char* colors, int nc) override;

  /**
   * Draw points at the vertex positions specified.
   */
  void DrawPoints(const float* verts, int n, const unsigned char* colors, int nc) override;

  /**
   * Draw triangles to generate the specified mesh.
   */
  void DrawTriangleMesh(const float* mesh, int n, const unsigned char* colors, int nc) override;

  /**
   * Apply the supplied pen which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the svtkPen
   * object in the svtkContext2D, it does not hold a pointer to the supplied object.
   */
  void ApplyPen(svtkPen* pen) override;

  /**
   * Apply the supplied brush which controls the outlines of shapes, as well as
   * lines, points and related primitives. This makes a deep copy of the svtkBrush
   * object in the svtkContext2D, it does not hold a pointer to the supplied object.
   */
  void ApplyBrush(svtkBrush* brush) override;

  /**
   * Set the model view matrix for the display
   */
  void SetMatrix(svtkMatrix4x4* m) override;

  /**
   * Set the model view matrix for the display
   */
  void GetMatrix(svtkMatrix4x4* m) override;

  /**
   * Multiply the current model view matrix by the supplied one
   */
  void MultiplyMatrix(svtkMatrix4x4* m) override;

  /**
   * Push the current matrix onto the stack.
   */
  void PushMatrix() override;

  /**
   * Pop the current matrix off of the stack.
   */
  void PopMatrix() override;

  /**
   * Supply a float array of length 4 with x1, y1, width, height specifying
   * clipping region for the device in pixels.
   */
  void SetClipping(const svtkRecti& rect) override;

  /**
   * Enable or disable the clipping of the scene.
   */
  void EnableClipping(bool enable) override;

  //@{
  /**
   * Enable/Disable the specified clipping plane.
   * i is the index of the clipping plane being enabled or disabled (0 - 5).
   * planeEquation points to the four coefficients of the equation for the
   * clipping plane: Ax + By + Cz + D = 0.  This is the equation format
   * expected by glClipPlane.
   */
  void EnableClippingPlane(int i, double* planeEquation) override;
  void DisableClippingPlane(int i) override;
  //@}

  /**
   * This must be set during initialization
   */
  void Initialize(svtkRenderer*, svtkOpenGLContextDevice2D*);

  /**
   * Begin drawing, pass in the viewport to set up the view.
   */
  virtual void Begin(svtkViewport* viewport);

protected:
  svtkOpenGLContextDevice3D();
  ~svtkOpenGLContextDevice3D() override;

  /**
   * Begin drawing, turn on the depth buffer.
   */
  virtual void EnableDepthBuffer();

  /**
   * End drawing, turn off the depth buffer.
   */
  virtual void DisableDepthBuffer();

  svtkOpenGLHelper* VCBO; // vertex + color
  void ReadyVCBOProgram();
  svtkOpenGLHelper* VBO; // vertex
  void ReadyVBOProgram();

  void SetMatrices(svtkShaderProgram* prog);
  void BuildVBO(svtkOpenGLHelper* cbo, const float* v, int nv, const unsigned char* coolors, int nc,
    float* tcoords);
  void CoreDrawTriangles(std::vector<float>& tverts);

  // do we have wide lines that require special handling
  virtual bool HaveWideLines();

  svtkTransform* ModelMatrix;

  /**
   * The OpenGL render window being used by the device
   */
  svtkOpenGLRenderWindow* RenderWindow;

  /**
   * We need to store a pointer to get the camera mats
   */
  svtkRenderer* Renderer;

  std::vector<bool> ClippingPlaneStates;
  std::vector<double> ClippingPlaneValues;

private:
  svtkOpenGLContextDevice3D(const svtkOpenGLContextDevice3D&) = delete;
  void operator=(const svtkOpenGLContextDevice3D&) = delete;

  //@{
  /**
   * Private data pointer of the class
   */
  class Private;
  Private* Storage;
  //@}

  // we need a pointer to this because only
  // the 2D device gets a Begin and sets up
  // the ortho matrix
  svtkOpenGLContextDevice2D* Device2D;

  svtkNew<svtkBrush> Brush;
  svtkNew<svtkPen> Pen;
};

#endif
