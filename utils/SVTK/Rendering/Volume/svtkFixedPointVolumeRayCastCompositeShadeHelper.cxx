/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkFixedPointVolumeRayCastCompositeShadeHelper.cxx
  Language:  C++

  Copyright (c) 1993-2002 Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
#include "svtkFixedPointVolumeRayCastCompositeShadeHelper.h"

#include "svtkCommand.h"
#include "svtkDataArray.h"
#include "svtkFixedPointRayCastImage.h"
#include "svtkFixedPointVolumeRayCastMapper.h"
#include "svtkImageData.h"
#include "svtkObjectFactory.h"
#include "svtkRenderWindow.h"
#include "svtkVolume.h"
#include "svtkVolumeProperty.h"

#include <cmath>

svtkStandardNewMacro(svtkFixedPointVolumeRayCastCompositeShadeHelper);

// Construct a new svtkFixedPointVolumeRayCastCompositeShadeHelper with default values
svtkFixedPointVolumeRayCastCompositeShadeHelper::svtkFixedPointVolumeRayCastCompositeShadeHelper() =
  default;

// Destruct a svtkFixedPointVolumeRayCastCompositeShadeHelper - clean up any memory used
svtkFixedPointVolumeRayCastCompositeShadeHelper::~svtkFixedPointVolumeRayCastCompositeShadeHelper() =
  default;

// This method is used when the interpolation type is nearest neighbor and
// the data has one component and scale == 1.0 and shift == 0.0. In the inner
// loop we get the data value as an unsigned short, and use this index to
// lookup a color and opacity for this sample. We then composite this into
// the color computed so far along the ray, and check if we can terminate at
// this point (if the accumulated opacity is higher than some threshold).
// Finally we move on to the next sample along the ray.
template <class T>
void svtkFixedPointCompositeShadeHelperGenerateImageOneSimpleNN(
  T* data, int threadID, int threadCount, svtkFixedPointVolumeRayCastMapper* mapper, svtkVolume* vol)
{
  SVTKKWRCHelper_InitializationAndLoopStartShadeNN();
  SVTKKWRCHelper_InitializeCompositeOneNN();
  SVTKKWRCHelper_InitializeCompositeShadeNN();
  SVTKKWRCHelper_SpaceLeapSetup();

  for (k = 0; k < numSteps; k++)
  {
    if (k)
    {
      SVTKKWRCHelper_MoveToNextSampleShadeNN();
    }

    SVTKKWRCHelper_SpaceLeapCheck();
    SVTKKWRCHelper_CroppingCheckNN(pos);

    unsigned short val = static_cast<unsigned short>(((*dptr)));
    SVTKKWRCHelper_LookupColorUS(colorTable[0], scalarOpacityTable[0], val, tmp);
    if (tmp[3])
    {
      unsigned short normal = *dirPtr;
      SVTKKWRCHelper_LookupShading(diffuseShadingTable[0], specularShadingTable[0], normal, tmp);
      SVTKKWRCHelper_CompositeColorAndCheckEarlyTermination(color, tmp, remainingOpacity);
    }
  }

  SVTKKWRCHelper_SetPixelColor(imagePtr, color, remainingOpacity);
  SVTKKWRCHelper_IncrementAndLoopEnd();
}

// This method is used when the interpolation type is nearest neighbor and
// the data has one component. In the inner loop we get the data value as
// an unsigned short using the scale/shift, and use this index to lookup
// a color and opacity for this sample. We then composite this into the
// color computed so far along the ray, and check if we can terminate at
// this point (if the accumulated opacity is higher than some threshold).
// Finally we move on to the next sample along the ray.
template <class T>
void svtkFixedPointCompositeShadeHelperGenerateImageOneNN(
  T* data, int threadID, int threadCount, svtkFixedPointVolumeRayCastMapper* mapper, svtkVolume* vol)
{
  SVTKKWRCHelper_InitializationAndLoopStartShadeNN();
  SVTKKWRCHelper_InitializeCompositeOneNN();
  SVTKKWRCHelper_InitializeCompositeShadeNN();
  SVTKKWRCHelper_SpaceLeapSetup();

  for (k = 0; k < numSteps; k++)
  {
    if (k)
    {
      SVTKKWRCHelper_MoveToNextSampleShadeNN();
    }

    SVTKKWRCHelper_SpaceLeapCheck();
    SVTKKWRCHelper_CroppingCheckNN(pos);

    unsigned short val = static_cast<unsigned short>(((*dptr) + shift[0]) * scale[0]);
    SVTKKWRCHelper_LookupColorUS(colorTable[0], scalarOpacityTable[0], val, tmp);
    if (tmp[3])
    {
      unsigned short normal = *dirPtr;
      SVTKKWRCHelper_LookupShading(diffuseShadingTable[0], specularShadingTable[0], normal, tmp);
      SVTKKWRCHelper_CompositeColorAndCheckEarlyTermination(color, tmp, remainingOpacity);
    }
  }

  SVTKKWRCHelper_SetPixelColor(imagePtr, color, remainingOpacity);
  SVTKKWRCHelper_IncrementAndLoopEnd();
}

// This method is used when the interpolation type is nearest neighbor and
// the data has two components which are not considered independent. In the
// inner loop we compute the two unsigned short index values from the data
// values (using the scale/shift). We use the first index to lookup a color,
// and we use the second index to look up the opacity. We then composite
// the color into the color computed so far along this ray, and check to
// see if we can terminate here (if the opacity accumulated exceed some
// threshold). Finally we move to the next sample along the ray.
template <class T>
void svtkFixedPointCompositeShadeHelperGenerateImageTwoDependentNN(
  T* data, int threadID, int threadCount, svtkFixedPointVolumeRayCastMapper* mapper, svtkVolume* vol)
{
  SVTKKWRCHelper_InitializationAndLoopStartShadeNN();
  SVTKKWRCHelper_InitializeCompositeOneNN();
  SVTKKWRCHelper_InitializeCompositeShadeNN();
  SVTKKWRCHelper_SpaceLeapSetup();

  for (k = 0; k < numSteps; k++)
  {
    if (k)
    {
      SVTKKWRCHelper_MoveToNextSampleShadeNN();
    }

    SVTKKWRCHelper_SpaceLeapCheck();
    SVTKKWRCHelper_CroppingCheckNN(pos);

    unsigned short val[2];
    val[0] = static_cast<unsigned short>(((*(dptr)) + shift[0]) * scale[0]);
    val[1] = static_cast<unsigned short>(((*(dptr + 1)) + shift[1]) * scale[1]);

    tmp[3] = scalarOpacityTable[0][val[1]];
    if (tmp[3])
    {
      tmp[0] = static_cast<unsigned short>(
        (colorTable[0][3 * val[0]] * tmp[3] + 0x7fff) >> (SVTKKW_FP_SHIFT));
      tmp[1] = static_cast<unsigned short>(
        (colorTable[0][3 * val[0] + 1] * tmp[3] + 0x7fff) >> (SVTKKW_FP_SHIFT));
      tmp[2] = static_cast<unsigned short>(
        (colorTable[0][3 * val[0] + 2] * tmp[3] + 0x7fff) >> (SVTKKW_FP_SHIFT));

      unsigned short normal = *dirPtr;
      SVTKKWRCHelper_LookupShading(diffuseShadingTable[0], specularShadingTable[0], normal, tmp);
      SVTKKWRCHelper_CompositeColorAndCheckEarlyTermination(color, tmp, remainingOpacity);
    }
  }

  SVTKKWRCHelper_SetPixelColor(imagePtr, color, remainingOpacity);
  SVTKKWRCHelper_IncrementAndLoopEnd();
}

// This method is used when the interpolation type is nearest neighbor and
// the data has four components which are not considered independent . This
// means that the first three components directly represent color, and this
// data must be of unsigned char type. In the inner loop we directly access
// the four data values (no scale/shift is needed). The first three are the
// color of this sample and the fourth is used to look up an opacity in the
// scalar opacity transfer function. We then composite this color into the
// color we have accumulated so far along the ray, and check if we can
// terminate here (if our accumulated opacity has exceed some threshold).
// Finally we move onto the next sample along the ray.
template <class T>
void svtkFixedPointCompositeShadeHelperGenerateImageFourDependentNN(
  T* data, int threadID, int threadCount, svtkFixedPointVolumeRayCastMapper* mapper, svtkVolume* vol)
{
  SVTKKWRCHelper_InitializationAndLoopStartShadeNN();
  SVTKKWRCHelper_InitializeCompositeOneNN();
  SVTKKWRCHelper_InitializeCompositeShadeNN();
  SVTKKWRCHelper_SpaceLeapSetup();

  for (k = 0; k < numSteps; k++)
  {
    if (k)
    {
      SVTKKWRCHelper_MoveToNextSampleShadeNN();
    }

    SVTKKWRCHelper_SpaceLeapCheck();
    SVTKKWRCHelper_CroppingCheckNN(pos);

    unsigned short val[4];
    val[0] = *(dptr);
    val[1] = *(dptr + 1);
    val[2] = *(dptr + 2);
    val[3] = static_cast<unsigned short>(((*(dptr + 3)) + shift[3]) * scale[3]);

    tmp[3] = scalarOpacityTable[0][val[3]];
    if (tmp[3])
    {
      tmp[0] = (val[0] * tmp[3] + 0x7f) >> (8);
      tmp[1] = (val[1] * tmp[3] + 0x7f) >> (8);
      tmp[2] = (val[2] * tmp[3] + 0x7f) >> (8);

      unsigned short normal = *dirPtr;
      SVTKKWRCHelper_LookupShading(diffuseShadingTable[0], specularShadingTable[0], normal, tmp);
      SVTKKWRCHelper_CompositeColorAndCheckEarlyTermination(color, tmp, remainingOpacity);
    }
  }

  SVTKKWRCHelper_SetPixelColor(imagePtr, color, remainingOpacity);
  SVTKKWRCHelper_IncrementAndLoopEnd();
}

// This method is used when the interpolation type is nearest neighbor and
// the data has more than one component and the components are considered to
// be independent. In the inner loop we access each component value, using
// the scale/shift to turn the data value into an unsigned short index. We
// then lookup the color/opacity for each component and combine them according
// to the weighting value for each component. We composite this resulting
// color into the color already accumulated for this ray, and we check
// whether we can terminate here (if the accumulated opacity exceeds some
// threshold). Finally we increment to the next sample on the ray.
//
// TODO: short circuit calculations when opacity is 0
template <class T>
void svtkFixedPointCompositeShadeHelperGenerateImageIndependentNN(
  T* data, int threadID, int threadCount, svtkFixedPointVolumeRayCastMapper* mapper, svtkVolume* vol)
{
  SVTKKWRCHelper_InitializeWeights();
  SVTKKWRCHelper_InitializationAndLoopStartShadeNN();
  SVTKKWRCHelper_InitializeCompositeMultiNN();
  SVTKKWRCHelper_InitializeCompositeShadeNN();

  for (k = 0; k < numSteps; k++)
  {
    if (k)
    {
      SVTKKWRCHelper_MoveToNextSampleShadeNN();
    }

    SVTKKWRCHelper_CroppingCheckNN(pos);

    unsigned short normal[4];
    for (c = 0; c < components; c++)
    {
      val[c] = static_cast<unsigned short>(((*(dptr + c)) + shift[c]) * scale[c]);
      normal[c] = *(dirPtr + c);
    }

    SVTKKWRCHelper_LookupAndCombineIndependentColorsShadeUS(colorTable, scalarOpacityTable,
      diffuseShadingTable, specularShadingTable, val, normal, weights, components, tmp);

    if (tmp[3])
    {
      SVTKKWRCHelper_CompositeColorAndCheckEarlyTermination(color, tmp, remainingOpacity);
    }
  }

  SVTKKWRCHelper_SetPixelColor(imagePtr, color, remainingOpacity);
  SVTKKWRCHelper_IncrementAndLoopEnd();
}

// This method is used when the interpolation type is linear and the data
// has one component and scale = 1.0 and shift = 0.0. In the inner loop we
// get the data value for the eight cell corners (if we have changed cells)
// as an unsigned short (the range must be right and we don't need the
// scale/shift). We compute our weights within the cell according to our
// fractional position within the cell, apply trilinear interpolation to
// compute the index, and use this index to lookup a color and opacity for
// this sample. We then composite this into the color computed so far along
// the ray, and check if we can terminate at this point (if the accumulated
// opacity is higher than some threshold). Finally we move on to the next
// sample along the ray.
template <class T>
void svtkFixedPointCompositeShadeHelperGenerateImageOneSimpleTrilin(
  T* data, int threadID, int threadCount, svtkFixedPointVolumeRayCastMapper* mapper, svtkVolume* vol)
{
  SVTKKWRCHelper_InitializationAndLoopStartShadeTrilin();
  SVTKKWRCHelper_InitializeCompositeOneTrilin();
  SVTKKWRCHelper_InitializeCompositeOneShadeTrilin();
  SVTKKWRCHelper_SpaceLeapSetup();

  int needToSampleDirection = 0;
  for (k = 0; k < numSteps; k++)
  {
    if (k)
    {
      mapper->FixedPointIncrement(pos, dir);
    }

    SVTKKWRCHelper_SpaceLeapCheck();
    SVTKKWRCHelper_CroppingCheckTrilin(pos);

    mapper->ShiftVectorDown(pos, spos);
    if (spos[0] != oldSPos[0] || spos[1] != oldSPos[1] || spos[2] != oldSPos[2])
    {
      oldSPos[0] = spos[0];
      oldSPos[1] = spos[1];
      oldSPos[2] = spos[2];

      dptr = data + spos[0] * inc[0] + spos[1] * inc[1] + spos[2] * inc[2];
      SVTKKWRCHelper_GetCellScalarValuesSimple(dptr);
      dirPtrABCD = gradientDir[spos[2]] + spos[0] * dInc[0] + spos[1] * dInc[1];
      dirPtrEFGH = gradientDir[spos[2] + 1] + spos[0] * dInc[0] + spos[1] * dInc[1];
      needToSampleDirection = 1;
    }

    SVTKKWRCHelper_ComputeWeights(pos);
    SVTKKWRCHelper_InterpolateScalar(val);

    SVTKKWRCHelper_LookupColorUS(colorTable[0], scalarOpacityTable[0], val, tmp);
    if (needToSampleDirection)
    {
      SVTKKWRCHelper_GetCellDirectionValues(dirPtrABCD, dirPtrEFGH);
      needToSampleDirection = 0;
    }

    SVTKKWRCHelper_InterpolateShading(diffuseShadingTable[0], specularShadingTable[0], tmp);
    SVTKKWRCHelper_CompositeColorAndCheckEarlyTermination(color, tmp, remainingOpacity);
  }

  SVTKKWRCHelper_SetPixelColor(imagePtr, color, remainingOpacity);
  SVTKKWRCHelper_IncrementAndLoopEnd();
}

// This method is used when the interpolation type is linear and the data
// has one component and scale != 1.0 or shift != 0.0. In the inner loop we
// get the data value for the eight cell corners (if we have changed cells)
// as an unsigned short (we use the scale/shift to ensure the correct range).
// We compute our weights within the cell according to our fractional position
// within the cell, apply trilinear interpolation to compute the index, and use
// this index to lookup a color and opacity for this sample. We then composite
// this into the color computed so far along the ray, and check if we can
// terminate at this point (if the accumulated opacity is higher than some
// threshold). Finally we move on to the next sample along the ray.
template <class T>
void svtkFixedPointCompositeShadeHelperGenerateImageOneTrilin(
  T* data, int threadID, int threadCount, svtkFixedPointVolumeRayCastMapper* mapper, svtkVolume* vol)
{
  SVTKKWRCHelper_InitializationAndLoopStartShadeTrilin();
  SVTKKWRCHelper_InitializeCompositeOneTrilin();
  SVTKKWRCHelper_InitializeCompositeOneShadeTrilin();
  SVTKKWRCHelper_SpaceLeapSetup();

  int needToSampleDirection = 0;
  for (k = 0; k < numSteps; k++)
  {
    if (k)
    {
      mapper->FixedPointIncrement(pos, dir);
    }

    SVTKKWRCHelper_SpaceLeapCheck();
    SVTKKWRCHelper_CroppingCheckTrilin(pos);

    mapper->ShiftVectorDown(pos, spos);
    if (spos[0] != oldSPos[0] || spos[1] != oldSPos[1] || spos[2] != oldSPos[2])
    {
      oldSPos[0] = spos[0];
      oldSPos[1] = spos[1];
      oldSPos[2] = spos[2];

      dptr = data + spos[0] * inc[0] + spos[1] * inc[1] + spos[2] * inc[2];
      SVTKKWRCHelper_GetCellScalarValues(dptr, scale[0], shift[0]);
      dirPtrABCD = gradientDir[spos[2]] + spos[0] * dInc[0] + spos[1] * dInc[1];
      dirPtrEFGH = gradientDir[spos[2] + 1] + spos[0] * dInc[0] + spos[1] * dInc[1];
      needToSampleDirection = 1;
    }

    SVTKKWRCHelper_ComputeWeights(pos);
    SVTKKWRCHelper_InterpolateScalar(val);

    SVTKKWRCHelper_LookupColorUS(colorTable[0], scalarOpacityTable[0], val, tmp);
    if (needToSampleDirection)
    {
      SVTKKWRCHelper_GetCellDirectionValues(dirPtrABCD, dirPtrEFGH);
      needToSampleDirection = 0;
    }
    SVTKKWRCHelper_InterpolateShading(diffuseShadingTable[0], specularShadingTable[0], tmp);
    SVTKKWRCHelper_CompositeColorAndCheckEarlyTermination(color, tmp, remainingOpacity);
  }

  SVTKKWRCHelper_SetPixelColor(imagePtr, color, remainingOpacity);
  SVTKKWRCHelper_IncrementAndLoopEnd();
}

// This method is used when the interpolation type is linear, the data has
// two components and the components are not considered independent. In the
// inner loop we get the data value for the eight cell corners (if we have
// changed cells) for both components as an unsigned shorts (we use the
// scale/shift to ensure the correct range). We compute our weights within
// the cell according to our fractional position within the cell, and apply
// trilinear interpolation to compute the two index value. We use the first
// index to lookup a color and the second to look up an opacity for this sample.
// We then composite this into the color computed so far along the ray, and
// check if we can terminate at this point (if the accumulated opacity is
// higher than some threshold). Finally we move on to the next sample along
// the ray.
template <class T>
void svtkFixedPointCompositeShadeHelperGenerateImageTwoDependentTrilin(
  T* data, int threadID, int threadCount, svtkFixedPointVolumeRayCastMapper* mapper, svtkVolume* vol)
{
  SVTKKWRCHelper_InitializationAndLoopStartShadeTrilin();
  SVTKKWRCHelper_InitializeCompositeMultiTrilin();
  SVTKKWRCHelper_InitializeCompositeOneShadeTrilin();
  SVTKKWRCHelper_SpaceLeapSetup();

  int needToSampleDirection = 0;
  for (k = 0; k < numSteps; k++)
  {
    if (k)
    {
      mapper->FixedPointIncrement(pos, dir);
    }

    SVTKKWRCHelper_SpaceLeapCheck();
    SVTKKWRCHelper_CroppingCheckTrilin(pos);

    mapper->ShiftVectorDown(pos, spos);
    if (spos[0] != oldSPos[0] || spos[1] != oldSPos[1] || spos[2] != oldSPos[2])
    {
      oldSPos[0] = spos[0];
      oldSPos[1] = spos[1];
      oldSPos[2] = spos[2];

      dptr = data + spos[0] * inc[0] + spos[1] * inc[1] + spos[2] * inc[2];
      SVTKKWRCHelper_GetCellComponentScalarValues(dptr, 0, scale[0], shift[0]);

      dptr++;
      SVTKKWRCHelper_GetCellComponentScalarValues(dptr, 1, scale[1], shift[1]);

      dirPtrABCD = gradientDir[spos[2]] + spos[0] * dInc[0] + spos[1] * dInc[1];
      dirPtrEFGH = gradientDir[spos[2] + 1] + spos[0] * dInc[0] + spos[1] * dInc[1];
      needToSampleDirection = 1;
    }

    SVTKKWRCHelper_ComputeWeights(pos);
    SVTKKWRCHelper_InterpolateScalarComponent(val, c, 2);

    tmp[3] = scalarOpacityTable[0][val[1]];
    if (!tmp[3])
    {
      continue;
    }

    if (needToSampleDirection)
    {
      SVTKKWRCHelper_GetCellDirectionValues(dirPtrABCD, dirPtrEFGH);
      needToSampleDirection = 0;
    }

    tmp[0] = static_cast<unsigned short>(
      (colorTable[0][3 * val[0]] * tmp[3] + 0x7fff) >> (SVTKKW_FP_SHIFT));
    tmp[1] = static_cast<unsigned short>(
      (colorTable[0][3 * val[0] + 1] * tmp[3] + 0x7fff) >> (SVTKKW_FP_SHIFT));
    tmp[2] = static_cast<unsigned short>(
      (colorTable[0][3 * val[0] + 2] * tmp[3] + 0x7fff) >> (SVTKKW_FP_SHIFT));

    SVTKKWRCHelper_InterpolateShading(diffuseShadingTable[0], specularShadingTable[0], tmp);
    SVTKKWRCHelper_CompositeColorAndCheckEarlyTermination(color, tmp, remainingOpacity);
  }

  SVTKKWRCHelper_SetPixelColor(imagePtr, color, remainingOpacity);
  SVTKKWRCHelper_IncrementAndLoopEnd();
}

// This method is used when the interpolation type is linear, the data has
// four components and the components are not considered independent. In the
// inner loop we get the data value for the eight cell corners (if we have
// changed cells) for all components as an unsigned shorts (we don't have to
// use the scale/shift because only unsigned char data is supported for four
// component data when the components are not independent). We compute our
// weights within the cell according to our fractional position within the cell,
// and apply trilinear interpolation to compute a value for each component. We
// use the first three directly as the color of the sample, and the fourth is
// used to look up an opacity for this sample. We then composite this into the
// color computed so far along the ray, and check if we can terminate at this
// point (if the accumulated opacity is higher than some threshold). Finally we
// move on to the next sample along the ray.
template <class T>
void svtkFixedPointCompositeShadeHelperGenerateImageFourDependentTrilin(
  T* data, int threadID, int threadCount, svtkFixedPointVolumeRayCastMapper* mapper, svtkVolume* vol)
{
  SVTKKWRCHelper_InitializationAndLoopStartShadeTrilin();
  SVTKKWRCHelper_InitializeCompositeMultiTrilin();
  SVTKKWRCHelper_InitializeCompositeOneShadeTrilin();
  SVTKKWRCHelper_SpaceLeapSetup();

  int needToSampleDirection = 0;
  for (k = 0; k < numSteps; k++)
  {
    if (k)
    {
      mapper->FixedPointIncrement(pos, dir);
    }

    SVTKKWRCHelper_SpaceLeapCheck();
    SVTKKWRCHelper_CroppingCheckTrilin(pos);

    mapper->ShiftVectorDown(pos, spos);
    if (spos[0] != oldSPos[0] || spos[1] != oldSPos[1] || spos[2] != oldSPos[2])
    {
      oldSPos[0] = spos[0];
      oldSPos[1] = spos[1];
      oldSPos[2] = spos[2];

      dptr = data + spos[0] * inc[0] + spos[1] * inc[1] + spos[2] * inc[2];
      SVTKKWRCHelper_GetCellComponentRawScalarValues(dptr, 0);

      dptr++;
      SVTKKWRCHelper_GetCellComponentRawScalarValues(dptr, 1);

      dptr++;
      SVTKKWRCHelper_GetCellComponentRawScalarValues(dptr, 2);

      dptr++;
      SVTKKWRCHelper_GetCellComponentScalarValues(dptr, 3, scale[3], shift[3]);

      dirPtrABCD = gradientDir[spos[2]] + spos[0] * dInc[0] + spos[1] * dInc[1];
      dirPtrEFGH = gradientDir[spos[2] + 1] + spos[0] * dInc[0] + spos[1] * dInc[1];
      needToSampleDirection = 1;
    }

    SVTKKWRCHelper_ComputeWeights(pos);
    SVTKKWRCHelper_InterpolateScalarComponent(val, c, 4);

    tmp[3] = scalarOpacityTable[0][val[3]];
    if (!tmp[3])
    {
      continue;
    }

    if (needToSampleDirection)
    {
      SVTKKWRCHelper_GetCellDirectionValues(dirPtrABCD, dirPtrEFGH);
      needToSampleDirection = 0;
    }

    tmp[0] = (val[0] * tmp[3] + 0x7f) >> 8;
    tmp[1] = (val[1] * tmp[3] + 0x7f) >> 8;
    tmp[2] = (val[2] * tmp[3] + 0x7f) >> 8;

    SVTKKWRCHelper_InterpolateShading(diffuseShadingTable[0], specularShadingTable[0], tmp);
    SVTKKWRCHelper_CompositeColorAndCheckEarlyTermination(color, tmp, remainingOpacity);
  }

  SVTKKWRCHelper_SetPixelColor(imagePtr, color, remainingOpacity);
  SVTKKWRCHelper_IncrementAndLoopEnd();
}

// This method is used when the interpolation type is linear, the data has
// more than one component and the components are considered independent. In
// the inner loop we get the data value for the eight cell corners (if we have
// changed cells) for all components as an unsigned shorts (we have to use the
// scale/shift to ensure that we obtained unsigned short indices) We compute our
// weights within the cell according to our fractional position within the cell,
// and apply trilinear interpolation to compute a value for each component. We
// look up a color/opacity for each component and blend them according to the
// component weights. We then composite this resulting color into the
// color computed so far along the ray, and check if we can terminate at this
// point (if the accumulated opacity is higher than some threshold). Finally we
// move on to the next sample along the ray.
template <class T>
void svtkFixedPointCompositeShadeHelperGenerateImageIndependentTrilin(
  T* data, int threadID, int threadCount, svtkFixedPointVolumeRayCastMapper* mapper, svtkVolume* vol)
{
  SVTKKWRCHelper_InitializeWeights();
  SVTKKWRCHelper_InitializationAndLoopStartShadeTrilin();
  SVTKKWRCHelper_InitializeCompositeMultiTrilin();
  SVTKKWRCHelper_InitializeCompositeMultiShadeTrilin();

  for (k = 0; k < numSteps; k++)
  {
    if (k)
    {
      mapper->FixedPointIncrement(pos, dir);
    }

    SVTKKWRCHelper_CroppingCheckTrilin(pos);

    mapper->ShiftVectorDown(pos, spos);
    if (spos[0] != oldSPos[0] || spos[1] != oldSPos[1] || spos[2] != oldSPos[2])
    {
      oldSPos[0] = spos[0];
      oldSPos[1] = spos[1];
      oldSPos[2] = spos[2];

      dptr = data + spos[0] * inc[0] + spos[1] * inc[1] + spos[2] * inc[2];
      SVTKKWRCHelper_GetCellComponentScalarValues(dptr, 0, scale[0], shift[0]);

      dptr++;
      SVTKKWRCHelper_GetCellComponentScalarValues(dptr, 1, scale[1], shift[1]);

      if (components > 2)
      {
        dptr++;
        SVTKKWRCHelper_GetCellComponentScalarValues(dptr, 2, scale[2], shift[2]);
        if (components > 3)
        {
          dptr++;
          SVTKKWRCHelper_GetCellComponentScalarValues(dptr, 3, scale[3], shift[3]);
        }
      }

      dirPtrABCD = gradientDir[spos[2]] + spos[0] * dInc[0] + spos[1] * dInc[1];
      dirPtrEFGH = gradientDir[spos[2] + 1] + spos[0] * dInc[0] + spos[1] * dInc[1];
      SVTKKWRCHelper_GetCellComponentDirectionValues(dirPtrABCD, dirPtrEFGH, 0);

      dirPtrABCD++;
      dirPtrEFGH++;
      SVTKKWRCHelper_GetCellComponentDirectionValues(dirPtrABCD, dirPtrEFGH, 1);

      if (components > 2)
      {
        dirPtrABCD++;
        dirPtrEFGH++;
        SVTKKWRCHelper_GetCellComponentDirectionValues(dirPtrABCD, dirPtrEFGH, 2);
        if (components > 3)
        {
          dirPtrABCD++;
          dirPtrEFGH++;
          SVTKKWRCHelper_GetCellComponentDirectionValues(dirPtrABCD, dirPtrEFGH, 3);
        }
      }
    }

    SVTKKWRCHelper_ComputeWeights(pos);
    SVTKKWRCHelper_InterpolateScalarComponent(val, c, components);

    SVTKKWRCHelper_LookupAndCombineIndependentColorsInterpolateShadeUS(colorTable,
      scalarOpacityTable, diffuseShadingTable, specularShadingTable, val, weights, components, tmp);

    SVTKKWRCHelper_CompositeColorAndCheckEarlyTermination(color, tmp, remainingOpacity);
  }

  SVTKKWRCHelper_SetPixelColor(imagePtr, color, remainingOpacity);
  SVTKKWRCHelper_IncrementAndLoopEnd();
}

void svtkFixedPointVolumeRayCastCompositeShadeHelper::GenerateImage(
  int threadID, int threadCount, svtkVolume* vol, svtkFixedPointVolumeRayCastMapper* mapper)
{
  void* data = mapper->GetCurrentScalars()->GetVoidPointer(0);
  int scalarType = mapper->GetCurrentScalars()->GetDataType();

  // Nearest Neighbor interpolate
  if (mapper->ShouldUseNearestNeighborInterpolation(vol))
  {
    // One component data
    if (mapper->GetCurrentScalars()->GetNumberOfComponents() == 1)
    {
      // Scale == 1.0 and shift == 0.0 - simple case (faster)
      if (mapper->GetTableScale()[0] == 1.0 && mapper->GetTableShift()[0] == 0.0)
      {
        switch (scalarType)
        {
          svtkTemplateMacro(svtkFixedPointCompositeShadeHelperGenerateImageOneSimpleNN(
            static_cast<SVTK_TT*>(data), threadID, threadCount, mapper, vol));
        }
      }
      else
      {
        switch (scalarType)
        {
          svtkTemplateMacro(svtkFixedPointCompositeShadeHelperGenerateImageOneNN(
            static_cast<SVTK_TT*>(data), threadID, threadCount, mapper, vol));
        }
      }
    }
    // More that one independent components
    else if (vol->GetProperty()->GetIndependentComponents())
    {
      switch (scalarType)
      {
        svtkTemplateMacro(svtkFixedPointCompositeShadeHelperGenerateImageIndependentNN(
          static_cast<SVTK_TT*>(data), threadID, threadCount, mapper, vol));
      }
    }
    // Dependent (color) components
    else
    {
      // Two components - the first specifies color (through a lookup table)
      // and the second specified opacity (through a lookup table)
      if (mapper->GetCurrentScalars()->GetNumberOfComponents() == 2)
      {
        switch (scalarType)
        {
          svtkTemplateMacro(svtkFixedPointCompositeShadeHelperGenerateImageTwoDependentNN(
            static_cast<SVTK_TT*>(data), threadID, threadCount, mapper, vol));
        }
      }
      // Four components - they must be unsigned char, the first three directly
      // specify color and the fourth specifies opacity (through a lookup
      // table)
      else
      {
        if (scalarType == SVTK_UNSIGNED_CHAR)
        {
          svtkFixedPointCompositeShadeHelperGenerateImageFourDependentNN(
            static_cast<unsigned char*>(data), threadID, threadCount, mapper, vol);
        }
        else
        {
          svtkErrorMacro("Four component dependent data must be unsigned char");
        }
      }
    }
  }
  // Trilinear Interpolation
  else
  {
    // One component
    if (mapper->GetCurrentScalars()->GetNumberOfComponents() == 1)
    {
      // Scale == 1.0 and shift == 0.0 - simple case (faster)
      if (mapper->GetTableScale()[0] == 1.0 && mapper->GetTableShift()[0] == 0.0)
      {
        switch (scalarType)
        {
          svtkTemplateMacro(svtkFixedPointCompositeShadeHelperGenerateImageOneSimpleTrilin(
            static_cast<SVTK_TT*>(data), threadID, threadCount, mapper, vol));
        }
      }
      // Scale != 1.0 or shift != 0.0 - must apply scale/shift in inner loop
      else
      {
        switch (scalarType)
        {
          svtkTemplateMacro(svtkFixedPointCompositeShadeHelperGenerateImageOneTrilin(
            static_cast<SVTK_TT*>(data), threadID, threadCount, mapper, vol));
        }
      }
    }
    // Independent components (more than one)
    else if (vol->GetProperty()->GetIndependentComponents())
    {
      switch (scalarType)
      {
        svtkTemplateMacro(svtkFixedPointCompositeShadeHelperGenerateImageIndependentTrilin(
          static_cast<SVTK_TT*>(data), threadID, threadCount, mapper, vol));
      }
    }
    // Dependent components
    else
    {
      // Two components - the first specifies color (through a lookup table)
      // and the second specified opacity (through a lookup table)
      if (mapper->GetCurrentScalars()->GetNumberOfComponents() == 2)
      {
        switch (scalarType)
        {
          svtkTemplateMacro(svtkFixedPointCompositeShadeHelperGenerateImageTwoDependentTrilin(
            static_cast<SVTK_TT*>(data), threadID, threadCount, mapper, vol));
        }
      }
      // Four components - they must be unsigned char, the first three directly
      // specify color and the fourth specifies opacity (through a lookup
      // table)
      else
      {
        if (scalarType == SVTK_UNSIGNED_CHAR)
        {
          svtkFixedPointCompositeShadeHelperGenerateImageFourDependentTrilin(
            static_cast<unsigned char*>(data), threadID, threadCount, mapper, vol);
        }
        else
        {
          svtkErrorMacro("Four component dependent data must be unsigned char");
        }
      }
    }
  }
}

// Print method for svtkFixedPointVolumeRayCastCompositeShadeHelper
void svtkFixedPointVolumeRayCastCompositeShadeHelper::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
