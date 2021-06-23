/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkAbstractVolumeMapper.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkAbstractVolumeMapper
 * @brief   Abstract class for a volume mapper
 *
 *
 * svtkAbstractVolumeMapper is the abstract definition of a volume mapper.
 * Specific subclasses deal with different specific types of data input
 *
 * @sa
 * svtkVolumeMapper svtkUnstructuredGridVolumeMapper
 */

#ifndef svtkAbstractVolumeMapper_h
#define svtkAbstractVolumeMapper_h

#include "svtkAbstractMapper3D.h"
#include "svtkRenderingCoreModule.h" // For export macro

class svtkRenderer;
class svtkVolume;
class svtkWindow;
class svtkDataSet;

class SVTKRENDERINGCORE_EXPORT svtkAbstractVolumeMapper : public svtkAbstractMapper3D
{
public:
  svtkTypeMacro(svtkAbstractVolumeMapper, svtkAbstractMapper3D);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  //@{
  /**
   * Set/Get the input data
   */
  virtual svtkDataSet* GetDataSetInput();
  virtual svtkDataObject* GetDataObjectInput();
  //@}

  //@{
  /**
   * Return bounding box (array of six doubles) of data expressed as
   * (xmin,xmax, ymin,ymax, zmin,zmax).
   */
  double* GetBounds() SVTK_SIZEHINT(6) override;
  void GetBounds(double bounds[6]) override { this->svtkAbstractMapper3D::GetBounds(bounds); }
  //@}

  //@{
  /**
   * Control how the mapper works with scalar point data and cell attribute
   * data.  By default (ScalarModeToDefault), the mapper will use point data,
   * and if no point data is available, then cell data is used. Alternatively
   * you can explicitly set the mapper to use point data
   * (ScalarModeToUsePointData) or cell data (ScalarModeToUseCellData).
   * You can also choose to get the scalars from an array in point field
   * data (ScalarModeToUsePointFieldData) or cell field data
   * (ScalarModeToUseCellFieldData).  If scalars are coming from a field
   * data array, you must call SelectScalarArray.
   */
  svtkSetMacro(ScalarMode, int);
  svtkGetMacro(ScalarMode, int);
  svtkSetMacro(ArrayAccessMode, int);
  void SetScalarModeToDefault() { this->SetScalarMode(SVTK_SCALAR_MODE_DEFAULT); }
  void SetScalarModeToUsePointData() { this->SetScalarMode(SVTK_SCALAR_MODE_USE_POINT_DATA); }
  void SetScalarModeToUseCellData() { this->SetScalarMode(SVTK_SCALAR_MODE_USE_CELL_DATA); }
  void SetScalarModeToUsePointFieldData()
  {
    this->SetScalarMode(SVTK_SCALAR_MODE_USE_POINT_FIELD_DATA);
  }
  void SetScalarModeToUseCellFieldData()
  {
    this->SetScalarMode(SVTK_SCALAR_MODE_USE_CELL_FIELD_DATA);
  }
  //@}

  //@{
  /**
   * When ScalarMode is set to UsePointFieldData or UseCellFieldData,
   * you can specify which scalar array to use during rendering.
   * The transfer function in the svtkVolumeProperty (attached to the calling
   * svtkVolume) will decide how to convert vectors to colors.
   */
  virtual void SelectScalarArray(int arrayNum);
  virtual void SelectScalarArray(const char* arrayName);
  //@}

  /**
   * Get the array name or number and component to use for rendering.
   */
  virtual char* GetArrayName() { return this->ArrayName; }
  virtual int GetArrayId() { return this->ArrayId; }
  virtual int GetArrayAccessMode() { return this->ArrayAccessMode; }

  /**
   * Return the method for obtaining scalar data.
   */
  const char* GetScalarModeAsString();

  //@{
  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   */
  virtual float GetGradientMagnitudeScale() { return 1.0f; }
  virtual float GetGradientMagnitudeBias() { return 0.0f; }
  virtual float GetGradientMagnitudeScale(int) { return 1.0f; }
  virtual float GetGradientMagnitudeBias(int) { return 0.0f; }
  //@}

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * DO NOT USE THIS METHOD OUTSIDE OF THE RENDERING PROCESS
   * Render the volume
   */
  virtual void Render(svtkRenderer* ren, svtkVolume* vol) = 0;

  /**
   * WARNING: INTERNAL METHOD - NOT INTENDED FOR GENERAL USE
   * Release any graphics resources that are being consumed by this mapper.
   * The parameter window could be used to determine which graphic
   * resources to release.
   */
  void ReleaseGraphicsResources(svtkWindow*) override {}

protected:
  svtkAbstractVolumeMapper();
  ~svtkAbstractVolumeMapper() override;

  // see algorithm for more info
  int FillInputPortInformation(int port, svtkInformation* info) override;

  int ScalarMode;
  char* ArrayName;
  int ArrayId;
  int ArrayAccessMode;

private:
  svtkAbstractVolumeMapper(const svtkAbstractVolumeMapper&) = delete;
  void operator=(const svtkAbstractVolumeMapper&) = delete;
};

#endif
