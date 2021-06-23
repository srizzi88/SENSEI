/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkSphericalDirectionEncoder.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkSphericalDirectionEncoder
 * @brief   A direction encoder based on spherical coordinates
 *
 * svtkSphericalDirectionEncoder is a direction encoder which uses spherical
 * coordinates for mapping (nx, ny, nz) into an azimuth, elevation pair.
 *
 * @sa
 * svtkDirectionEncoder
 */

#ifndef svtkSphericalDirectionEncoder_h
#define svtkSphericalDirectionEncoder_h

#include "svtkDirectionEncoder.h"
#include "svtkRenderingVolumeModule.h" // For export macro

class SVTKRENDERINGVOLUME_EXPORT svtkSphericalDirectionEncoder : public svtkDirectionEncoder
{
public:
  svtkTypeMacro(svtkSphericalDirectionEncoder, svtkDirectionEncoder);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Construct the object. Initialize the index table which will be
   * used to map the normal into a patch on the recursively subdivided
   * sphere.
   */
  static svtkSphericalDirectionEncoder* New();

  /**
   * Given a normal vector n, return the encoded direction
   */
  int GetEncodedDirection(float n[3]) override;

  /**
   * / Given an encoded value, return a pointer to the normal vector
   */
  float* GetDecodedGradient(int value) SVTK_SIZEHINT(3) override;

  /**
   * Return the number of encoded directions
   */
  int GetNumberOfEncodedDirections(void) override { return 65536; }

  /**
   * Get the decoded gradient table. There are
   * this->GetNumberOfEncodedDirections() entries in the table, each
   * containing a normal (direction) vector. This is a flat structure -
   * 3 times the number of directions floats in an array.
   */
  float* GetDecodedGradientTable(void) override { return &(this->DecodedGradientTable[0]); }

protected:
  svtkSphericalDirectionEncoder();
  ~svtkSphericalDirectionEncoder() override;

  static float DecodedGradientTable[65536 * 3];

  //@{
  /**
   * Initialize the table at startup
   */
  static void InitializeDecodedGradientTable();
  static int DecodedGradientTableInitialized;
  //@}

private:
  svtkSphericalDirectionEncoder(const svtkSphericalDirectionEncoder&) = delete;
  void operator=(const svtkSphericalDirectionEncoder&) = delete;
};

#endif
