/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkEncodedGradientShader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

/**
 * @class   svtkEncodedGradientShader
 * @brief   Compute shading tables for encoded normals.
 *
 *
 * svtkEncodedGradientShader computes shading tables for encoded normals
 * that indicates the amount of diffuse and specular illumination that is
 * received from all light sources at a surface location with that normal.
 * For diffuse illumination this is accurate, but for specular illumination
 * it is approximate for perspective projections since the center view
 * direction is always used as the view direction. Since the shading table is
 * dependent on the volume (for the transformation that must be applied to
 * the normals to put them into world coordinates) there is a shading table
 * per volume. This is necessary because multiple volumes can share a
 * volume mapper.
 */

#ifndef svtkEncodedGradientShader_h
#define svtkEncodedGradientShader_h

#include "svtkObject.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class svtkVolume;
class svtkRenderer;
class svtkEncodedGradientEstimator;

#define SVTK_MAX_SHADING_TABLES 100

class SVTKRENDERINGVOLUME_EXPORT svtkEncodedGradientShader : public svtkObject
{
public:
  static svtkEncodedGradientShader* New();
  svtkTypeMacro(svtkEncodedGradientShader, svtkObject);

  /**
   * Print the svtkEncodedGradientShader
   */
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set / Get the intensity diffuse / specular light used for the
   * zero normals.
   */
  svtkSetClampMacro(ZeroNormalDiffuseIntensity, float, 0.0f, 1.0f);
  svtkGetMacro(ZeroNormalDiffuseIntensity, float);
  svtkSetClampMacro(ZeroNormalSpecularIntensity, float, 0.0f, 1.0f);
  svtkGetMacro(ZeroNormalSpecularIntensity, float);
  //@}

  /**
   * Cause the shading table to be updated
   */
  void UpdateShadingTable(svtkRenderer* ren, svtkVolume* vol, svtkEncodedGradientEstimator* gradest);

  //@{
  /**
   * Get the red/green/blue shading table.
   */
  float* GetRedDiffuseShadingTable(svtkVolume* vol);
  float* GetGreenDiffuseShadingTable(svtkVolume* vol);
  float* GetBlueDiffuseShadingTable(svtkVolume* vol);
  float* GetRedSpecularShadingTable(svtkVolume* vol);
  float* GetGreenSpecularShadingTable(svtkVolume* vol);
  float* GetBlueSpecularShadingTable(svtkVolume* vol);
  //@}

  //@{
  /**
   * Set the active component for shading. This component's
   * ambient / diffuse / specular / specular power values will
   * be used to create the shading table. The default is 1.0
   */
  svtkSetClampMacro(ActiveComponent, int, 0, 3);
  svtkGetMacro(ActiveComponent, int);
  //@}

protected:
  svtkEncodedGradientShader();
  ~svtkEncodedGradientShader() override;

  /**
   * Build a shading table for a light with the specified direction,
   * and color for an object of the specified material properties.
   * material[0] = ambient, material[1] = diffuse, material[2] = specular
   * and material[3] = specular exponent.  If the ambient flag is 1,
   * then ambient illumination is added. If not, then this means we
   * are calculating the "other side" of two sided lighting, so no
   * ambient intensity is added in. If the update flag is 0,
   * the shading table is overwritten with these new shading values.
   * If the updateFlag is 1, then the computed light contribution is
   * added to the current shading table values. There is one shading
   * table per volume, and the index value indicated which index table
   * should be used. It is computed in the UpdateShadingTable method.
   */
  void BuildShadingTable(int index, double lightDirection[3], double lightAmbientColor[3],
    double lightDiffuseColor[3], double lightSpecularColor[3], double lightIntensity,
    double viewDirection[3], double material[4], int twoSided, svtkEncodedGradientEstimator* gradest,
    int updateFlag);

  // The six shading tables (r diffuse ,g diffuse ,b diffuse,
  // r specular, g specular, b specular ) - with an entry for each
  // encoded normal plus one entry at the end for the zero normal
  // There is one shading table per volume listed in the ShadingTableVolume
  // array. A null entry indicates an available slot.
  float* ShadingTable[SVTK_MAX_SHADING_TABLES][6];
  svtkVolume* ShadingTableVolume[SVTK_MAX_SHADING_TABLES];
  int ShadingTableSize[SVTK_MAX_SHADING_TABLES];

  int ActiveComponent;

  // The intensity of light used for the zero normals, since it
  // can not be computed from the normal angles. Defaults to 0.0.
  float ZeroNormalDiffuseIntensity;
  float ZeroNormalSpecularIntensity;

private:
  svtkEncodedGradientShader(const svtkEncodedGradientShader&) = delete;
  void operator=(const svtkEncodedGradientShader&) = delete;
};

#endif
