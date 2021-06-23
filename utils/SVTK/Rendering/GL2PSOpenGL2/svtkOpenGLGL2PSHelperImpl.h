/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkOpenGLGL2PSHelperImpl.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkOpenGLGL2PSHelperImpl
 * @brief   svtkOpenGLGL2PSHelper override
 * implementation.
 */

#ifndef svtkOpenGLGL2PSHelperImpl_h
#define svtkOpenGLGL2PSHelperImpl_h

#include "svtkOpenGLGL2PSHelper.h"
#include "svtkRenderingGL2PSOpenGL2Module.h" // For export macro

class svtkMatrix4x4;
class svtkPoints;

class SVTKRENDERINGGL2PSOPENGL2_EXPORT svtkOpenGLGL2PSHelperImpl : public svtkOpenGLGL2PSHelper
{
public:
  static svtkOpenGLGL2PSHelperImpl* New();
  svtkTypeMacro(svtkOpenGLGL2PSHelperImpl, svtkOpenGLGL2PSHelper);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  void ProcessTransformFeedback(
    svtkTransformFeedback* tfc, svtkRenderer* ren, svtkActor* act) override;
  void ProcessTransformFeedback(
    svtkTransformFeedback* tfc, svtkRenderer* ren, unsigned char col[4]) override;
  void ProcessTransformFeedback(svtkTransformFeedback* tfc, svtkRenderer* ren, float col[4]) override;

  void DrawString(const std::string& str, svtkTextProperty* tprop, double pos[3],
    double backgroundDepth, svtkRenderer* ren) override;

  void DrawPath(svtkPath* path, double rasterPos[3], double windowPos[2], unsigned char rgba[4],
    double scale[2] = nullptr, double rotateAngle = 0.0, float strokeWidth = -1,
    const char* label = nullptr) override;

  void Draw3DPath(svtkPath* path, svtkMatrix4x4* actorMatrix, double rasterPos[3],
    unsigned char actorColor[4], svtkRenderer* ren, const char* label = nullptr) override;

  void DrawImage(svtkImageData* image, double pos[3]) override;

protected:
  svtkOpenGLGL2PSHelperImpl();
  ~svtkOpenGLGL2PSHelperImpl() override;

  /**
   * Translate the tprop's fontname into a Postscript font name.
   */
  static const char* TextPropertyToPSFontName(svtkTextProperty* tprop);

  /**
   * Convert the alignment hint in tprop to a GL2PS text alignment constant.
   */
  static int TextPropertyToGL2PSAlignment(svtkTextProperty* tprop);

  /**
   * Extracts the information needed for transforming and projecting points
   * from a renderer.
   */
  static void GetTransformParameters(svtkRenderer* ren, svtkMatrix4x4* actorMatrix,
    svtkMatrix4x4* xform, double vpOrigin[2], double halfSize[2], double zfact[2]);

  //@{
  /**
   * Project the point from world coordinates into device coordinates.
   */
  static void ProjectPoint(double point[3], svtkRenderer* ren, svtkMatrix4x4* actorMatrix = nullptr);
  static void ProjectPoint(double point[4], svtkMatrix4x4* transformMatrix, double viewportOrigin[2],
    double halfWidth, double halfHeight, double zfact1, double zfact2);
  static void ProjectPoints(
    svtkPoints* points, svtkRenderer* ren, svtkMatrix4x4* actorMatrix = nullptr);
  //@}

  //@{
  /**
   * Unproject the point from device coordinates into world coordinates.
   * Input Z coordinate should be in NDC space.
   */
  static void UnprojectPoint(double point[4], svtkMatrix4x4* invTransformMatrix,
    double viewportOrigin[2], double halfWidth, double halfHeight, double zfact1, double zfact2);
  static void UnprojectPoints(
    double* points3D, svtkIdType numPoints, svtkRenderer* ren, svtkMatrix4x4* actorMatrix = nullptr);
  //@}

  void DrawPathPS(svtkPath* path, double rasterPos[3], double windowPos[2], unsigned char rgba[4],
    double scale[2], double rotateAngle, float strokeWidth, const std::string& label);
  void DrawPathPDF(svtkPath* path, double rasterPos[3], double windowPos[2], unsigned char rgba[4],
    double scale[2], double rotateAngle, float strokeWidth, const std::string& label);
  void DrawPathSVG(svtkPath* path, double rasterPos[3], double windowPos[2], unsigned char rgba[4],
    double scale[2], double rotateAngle, float strokeWidth, const std::string& label);

private:
  svtkOpenGLGL2PSHelperImpl(const svtkOpenGLGL2PSHelperImpl&) = delete;
  void operator=(const svtkOpenGLGL2PSHelperImpl&) = delete;
};

#endif // svtkOpenGLGL2PSHelperImpl_h
