/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkImageImport.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkImageImport
 * @brief   Import data from a C array.
 *
 * svtkImageImport provides methods needed to import image data from a source
 * independent of SVTK, such as a simple C array or a third-party pipeline.
 * Note that the SVTK convention is for the image voxel index (0,0,0) to be
 * the lower-left corner of the image, while most 2D image formats use
 * the upper-left corner.  You can use svtkImageFlip to correct the
 * orientation after the image has been loaded into SVTK.
 * Note that is also possible to import the raw data from a Python string
 * instead of from a C array. The array applies on scalar point data only, not
 * on cell data.
 * @sa
 * svtkImageExport
 */

#ifndef svtkImageImport_h
#define svtkImageImport_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageAlgorithm.h"

class SVTKIOIMAGE_EXPORT svtkImageImport : public svtkImageAlgorithm
{
public:
  static svtkImageImport* New();
  svtkTypeMacro(svtkImageImport, svtkImageAlgorithm);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Import data and make an internal copy of it.  If you do not want
   * SVTK to copy the data, then use SetImportVoidPointer instead (do
   * not use both).  Give the size of the data array in bytes.
   */
  void CopyImportVoidPointer(void* ptr, svtkIdType size);

  //@{
  /**
   * Set the pointer from which the image data is imported.  SVTK will
   * not make its own copy of the data, it will access the data directly
   * from the supplied array.  SVTK will not attempt to delete the data
   * nor modify the data.
   */
  void SetImportVoidPointer(void* ptr);
  void* GetImportVoidPointer() { return this->ImportVoidPointer; }
  //@}

  /**
   * Set the pointer from which the image data is imported.  Set save to 1
   * (the default) unless you want SVTK to delete the array via C++ delete
   * when the svtkImageImport object is deallocated.  SVTK will not make its
   * own copy of the data, it will access the data directly from the
   * supplied array.
   */
  void SetImportVoidPointer(void* ptr, int save);

  //@{
  /**
   * Set/Get the data type of pixels in the imported data.  This is used
   * as the scalar type of the Output.  Default: Short.
   */
  svtkSetMacro(DataScalarType, int);
  void SetDataScalarTypeToDouble() { this->SetDataScalarType(SVTK_DOUBLE); }
  void SetDataScalarTypeToFloat() { this->SetDataScalarType(SVTK_FLOAT); }
  void SetDataScalarTypeToInt() { this->SetDataScalarType(SVTK_INT); }
  void SetDataScalarTypeToShort() { this->SetDataScalarType(SVTK_SHORT); }
  void SetDataScalarTypeToUnsignedShort() { this->SetDataScalarType(SVTK_UNSIGNED_SHORT); }
  void SetDataScalarTypeToUnsignedChar() { this->SetDataScalarType(SVTK_UNSIGNED_CHAR); }
  svtkGetMacro(DataScalarType, int);
  const char* GetDataScalarTypeAsString()
  {
    return svtkImageScalarTypeNameMacro(this->DataScalarType);
  }
  //@}

  //@{
  /**
   * Set/Get the number of scalar components, for RGB images this must be 3.
   * Default: 1.
   */
  svtkSetMacro(NumberOfScalarComponents, int);
  svtkGetMacro(NumberOfScalarComponents, int);
  //@}

  //@{
  /**
   * Get/Set the extent of the data buffer.  The dimensions of your data
   * must be equal to (extent[1]-extent[0]+1) * (extent[3]-extent[2]+1) *
   * (extent[5]-DataExtent[4]+1).  For example, for a 2D image use
   * (0,width-1, 0,height-1, 0,0).
   */
  svtkSetVector6Macro(DataExtent, int);
  svtkGetVector6Macro(DataExtent, int);
  void SetDataExtentToWholeExtent() { this->SetDataExtent(this->GetWholeExtent()); }
  //@}

  //@{
  /**
   * Set/Get the spacing (typically in mm) between image voxels.
   * Default: (1.0, 1.0, 1.0).
   */
  svtkSetVector3Macro(DataSpacing, double);
  svtkGetVector3Macro(DataSpacing, double);
  //@}

  //@{
  /**
   * Set/Get the origin of the data, i.e. the coordinates (usually in mm)
   * of voxel (0,0,0).  Default: (0.0, 0.0, 0.0).
   */
  svtkSetVector3Macro(DataOrigin, double);
  svtkGetVector3Macro(DataOrigin, double);
  //@}

  //@{
  /**
   * Set/Get the direction of the data, i.e. the 3x3 matrix to rotate
   * the coordinates from index space (ijk) to physical space (xyz).
   * Default: Identity Matrix (1.0, 0.0, 0.0, 0.0, 1.0, 0.0, 0.0, 0.0, 1.0)
   */
  svtkSetVectorMacro(DataDirection, double, 9);
  svtkGetVectorMacro(DataDirection, double, 9);
  //@}

  //@{
  /**
   * Get/Set the whole extent of the image.  This is the largest possible
   * extent.  Set the DataExtent to the extent of the image in the buffer
   * pointed to by the ImportVoidPointer.
   */
  svtkSetVector6Macro(WholeExtent, int);
  svtkGetVector6Macro(WholeExtent, int);
  //@}

  /**
   * Propagates the update extent through the callback if it is set.
   */
  int RequestUpdateExtent(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;
  /**
   * Override svtkAlgorithm
   */
  int ComputePipelineMTime(svtkInformation* request, svtkInformationVector** inInfoVec,
    svtkInformationVector* outInfoVec, int requestFromOutputPort, svtkMTimeType* mtime) override;

  //@{
  /**
   * Set/get the scalar array name for this data set. Initial value is
   * "scalars".
   */
  svtkSetStringMacro(ScalarArrayName);
  svtkGetStringMacro(ScalarArrayName);
  //@}

  //@{
  /**
   * These are function pointer types for the pipeline connection
   * callbacks.  See further documentation on each individual callback.
   */
  typedef void (*UpdateInformationCallbackType)(void*);
  typedef int (*PipelineModifiedCallbackType)(void*);
  typedef int* (*WholeExtentCallbackType)(void*);
  typedef double* (*SpacingCallbackType)(void*);
  typedef double* (*OriginCallbackType)(void*);
  typedef double* (*DirectionCallbackType)(void*);
  typedef const char* (*ScalarTypeCallbackType)(void*);
  typedef int (*NumberOfComponentsCallbackType)(void*);
  typedef void (*PropagateUpdateExtentCallbackType)(void*, int*);
  typedef void (*UpdateDataCallbackType)(void*);
  typedef int* (*DataExtentCallbackType)(void*);
  typedef void* (*BufferPointerCallbackType)(void*);
  //@}

  //@{
  /**
   * Set/Get the callback for propagating UpdateInformation calls to a
   * third-party pipeline.  The callback should make sure that the
   * third-party pipeline information is up to date.
   */
  svtkSetMacro(UpdateInformationCallback, UpdateInformationCallbackType);
  svtkGetMacro(UpdateInformationCallback, UpdateInformationCallbackType);
  //@}

  //@{
  /**
   * Set/Get the callback for checking whether the third-party
   * pipeline has been modified since the last invocation of the
   * callback.  The callback should return 1 for modified, and 0 for
   * not modified.  The first call should always return modified.
   */
  svtkSetMacro(PipelineModifiedCallback, PipelineModifiedCallbackType);
  svtkGetMacro(PipelineModifiedCallback, PipelineModifiedCallbackType);
  //@}

  //@{
  /**
   * Set/Get the callback for getting the whole extent of the input
   * image from a third-party pipeline.  The callback should return a
   * vector of six integers describing the extent of the whole image
   * (x1 x2 y1 y2 z1 z2).
   */
  svtkSetMacro(WholeExtentCallback, WholeExtentCallbackType);
  svtkGetMacro(WholeExtentCallback, WholeExtentCallbackType);
  //@}

  //@{
  /**
   * Set/Get the callback for getting the spacing of the input image
   * from a third-party pipeline.  The callback should return a vector
   * of three double values describing the spacing (dx dy dz).
   */
  svtkSetMacro(SpacingCallback, SpacingCallbackType);
  svtkGetMacro(SpacingCallback, SpacingCallbackType);
  //@}

  //@{
  /**
   * Set/Get the callback for getting the origin of the input image
   * from a third-party pipeline.  The callback should return a vector
   * of three double values describing the origin (x0 y0 z0).
   */
  svtkSetMacro(OriginCallback, OriginCallbackType);
  svtkGetMacro(OriginCallback, OriginCallbackType);
  //@}

  //@{
  /**
   * Set/Get the callback for getting the direction of the input image
   * from a third-party pipeline.  The callback should return a vector
   * of nine double values describing the rotation from ijk to xyz.
   */
  svtkSetMacro(DirectionCallback, DirectionCallbackType);
  svtkGetMacro(DirectionCallback, DirectionCallbackType);
  //@}

  //@{
  /**
   * Set/Get the callback for getting the scalar value type of the
   * input image from a third-party pipeline.  The callback should
   * return a string with the name of the type.
   */
  svtkSetMacro(ScalarTypeCallback, ScalarTypeCallbackType);
  svtkGetMacro(ScalarTypeCallback, ScalarTypeCallbackType);
  //@}

  //@{
  /**
   * Set/Get the callback for getting the number of components of the
   * input image from a third-party pipeline.  The callback should
   * return an integer with the number of components.
   */
  svtkSetMacro(NumberOfComponentsCallback, NumberOfComponentsCallbackType);
  svtkGetMacro(NumberOfComponentsCallback, NumberOfComponentsCallbackType);
  //@}

  //@{
  /**
   * Set/Get the callback for propagating the pipeline update extent
   * to a third-party pipeline.  The callback should take a vector of
   * six integers describing the extent.  This should cause the
   * third-party pipeline to provide data which contains at least this
   * extent after the next UpdateData callback.
   */
  svtkSetMacro(PropagateUpdateExtentCallback, PropagateUpdateExtentCallbackType);
  svtkGetMacro(PropagateUpdateExtentCallback, PropagateUpdateExtentCallbackType);
  //@}

  //@{
  /**
   * Set/Get the callback for propagating UpdateData calls to a
   * third-party pipeline.  The callback should make sure the
   * third-party pipeline is up to date.
   */
  svtkSetMacro(UpdateDataCallback, UpdateDataCallbackType);
  svtkGetMacro(UpdateDataCallback, UpdateDataCallbackType);
  //@}

  //@{
  /**
   * Set/Get the callback for getting the data extent of the input
   * image from a third-party pipeline.  The callback should return a
   * vector of six integers describing the extent of the buffered
   * portion of the image (x1 x2 y1 y2 z1 z2).  The buffer location
   * should be set with the BufferPointerCallback.
   */
  svtkSetMacro(DataExtentCallback, DataExtentCallbackType);
  svtkGetMacro(DataExtentCallback, DataExtentCallbackType);
  //@}

  //@{
  /**
   * Set/Get the callback for getting a pointer to the data buffer of
   * an image from a third-party pipeline.  The callback should return
   * a pointer to the beginning of the buffer.  The extent of the
   * buffer should be set with the DataExtentCallback.
   */
  svtkSetMacro(BufferPointerCallback, BufferPointerCallbackType);
  svtkGetMacro(BufferPointerCallback, BufferPointerCallbackType);
  //@}

  //@{
  /**
   * Set/Get the user data which will be passed as the first argument
   * to all of the third-party pipeline callbacks.
   */
  svtkSetMacro(CallbackUserData, void*);
  svtkGetMacro(CallbackUserData, void*);
  //@}

  //@{
  /**
   * Invoke the appropriate callbacks
   */
  int InvokePipelineModifiedCallbacks();
  void InvokeUpdateInformationCallbacks();
  void InvokeExecuteInformationCallbacks();
  void InvokeExecuteDataCallbacks();
  void LegacyCheckWholeExtent();
  //@}

protected:
  svtkImageImport();
  ~svtkImageImport() override;

  int RequestInformation(svtkInformation*, svtkInformationVector**, svtkInformationVector*) override;

  void* ImportVoidPointer;
  int SaveUserArray;

  int NumberOfScalarComponents;
  int DataScalarType;

  int WholeExtent[6];
  int DataExtent[6];
  double DataSpacing[3];
  double DataOrigin[3];
  double DataDirection[9];

  char* ScalarArrayName;
  void* CallbackUserData;

  UpdateInformationCallbackType UpdateInformationCallback;
  PipelineModifiedCallbackType PipelineModifiedCallback;
  WholeExtentCallbackType WholeExtentCallback;
  SpacingCallbackType SpacingCallback;
  OriginCallbackType OriginCallback;
  DirectionCallbackType DirectionCallback;
  ScalarTypeCallbackType ScalarTypeCallback;
  NumberOfComponentsCallbackType NumberOfComponentsCallback;
  PropagateUpdateExtentCallbackType PropagateUpdateExtentCallback;
  UpdateDataCallbackType UpdateDataCallback;
  DataExtentCallbackType DataExtentCallback;
  BufferPointerCallbackType BufferPointerCallback;

  void ExecuteDataWithInformation(svtkDataObject* d, svtkInformation* outInfo) override;

private:
  svtkImageImport(const svtkImageImport&) = delete;
  void operator=(const svtkImageImport&) = delete;
};

#endif
