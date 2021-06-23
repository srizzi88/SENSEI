/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTDxInteractorStyleSettings.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTDxInteractorStyleSettings
 * @brief   3DConnexion device settings
 *
 *
 * svtkTDxInteractorStyleSettings defines settings for 3DConnexion device such
 * as sensitivity, axis filters
 *
 * @sa
 * svtkInteractorStyle svtkRenderWindowInteractor
 * svtkTDxInteractorStyle
 */

#ifndef svtkTDxInteractorStyleSettings_h
#define svtkTDxInteractorStyleSettings_h

#include "svtkObject.h"
#include "svtkRenderingCoreModule.h" // For export macro

class SVTKRENDERINGCORE_EXPORT svtkTDxInteractorStyleSettings : public svtkObject
{
public:
  static svtkTDxInteractorStyleSettings* New();
  svtkTypeMacro(svtkTDxInteractorStyleSettings, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Sensitivity of the rotation angle. This can be any value:
   * positive, negative, null.
   * - x<-1.0: faster reversed
   * - x=-1.0: reversed neutral
   * - -1.0<x<0.0:  reversed slower
   * - x=0.0: no rotation
   * - 0.0<x<1.0: slower
   * - x=1.0: neutral
   * - x>1.0: faster
   */
  svtkSetMacro(AngleSensitivity, double);
  svtkGetMacro(AngleSensitivity, double);
  //@}

  //@{
  /**
   * Use or mask the rotation component around the X-axis. Initial value is
   * true.
   */
  svtkSetMacro(UseRotationX, bool);
  svtkGetMacro(UseRotationX, bool);
  //@}

  //@{
  /**
   * Use or mask the rotation component around the Y-axis. Initial value is
   * true.
   */
  svtkSetMacro(UseRotationY, bool);
  svtkGetMacro(UseRotationY, bool);
  //@}

  //@{
  /**
   * Use or mask the rotation component around the Z-axis. Initial value is
   * true.
   */
  svtkSetMacro(UseRotationZ, bool);
  svtkGetMacro(UseRotationZ, bool);
  //@}

  //@{
  /**
   * Sensitivity of the translation along the X-axis. This can be any value:
   * positive, negative, null.
   * - x<-1.0: faster reversed
   * - x=-1.0: reversed neutral
   * - -1.0<x<0.0:  reversed slower
   * - x=0.0: no translation
   * - 0.0<x<1.0: slower
   * - x=1.0: neutral
   * - x>1.0: faster
   * Initial value is 1.0
   */
  svtkSetMacro(TranslationXSensitivity, double);
  svtkGetMacro(TranslationXSensitivity, double);
  //@}

  //@{
  /**
   * Sensitivity of the translation along the Y-axis.
   * See comment of SetTranslationXSensitivity().
   */
  svtkSetMacro(TranslationYSensitivity, double);
  svtkGetMacro(TranslationYSensitivity, double);
  //@}

  //@{
  /**
   * Sensitivity of the translation along the Z-axis.
   * See comment of SetTranslationXSensitivity().
   */
  svtkSetMacro(TranslationZSensitivity, double);
  svtkGetMacro(TranslationZSensitivity, double);
  //@}

protected:
  svtkTDxInteractorStyleSettings();
  ~svtkTDxInteractorStyleSettings() override;

  double AngleSensitivity;
  bool UseRotationX;
  bool UseRotationY;
  bool UseRotationZ;

  double TranslationXSensitivity;
  double TranslationYSensitivity;
  double TranslationZSensitivity;

private:
  svtkTDxInteractorStyleSettings(const svtkTDxInteractorStyleSettings&) = delete;
  void operator=(const svtkTDxInteractorStyleSettings&) = delete;
};
#endif
