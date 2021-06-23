/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkNIFTIImageReader.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkNIFTIImageReader
 * @brief   Read NIfTI-1 and NIfTI-2 medical image files
 *
 * This class reads NIFTI files, either in .nii format or as separate
 * .img and .hdr files.  If two files are used, then they can be passed
 * by using SetFileNames() instead of SetFileName().  Files ending in .gz
 * are decompressed on-the-fly while they are being read.  Files with
 * complex numbers or vector dimensions will be read as multi-component
 * images.  If a NIFTI file has a time dimension, then by default only the
 * first image in the time series will be read, but the TimeAsVector
 * flag can be set to read the time steps as vector components.  Files in
 * Analyze 7.5 format are also supported by this reader.
 * @par Thanks:
 * This class was contributed to SVTK by the Calgary Image Processing and
 * Analysis Centre (CIPAC).
 * @sa
 * svtkNIFTIImageWriter, svtkNIFTIImageHeader
 */

#ifndef svtkNIFTIImageReader_h
#define svtkNIFTIImageReader_h

#include "svtkIOImageModule.h" // For export macro
#include "svtkImageReader2.h"

class svtkNIFTIImageHeader;
class svtkMatrix4x4;

struct nifti_1_header;

//----------------------------------------------------------------------------
class SVTKIOIMAGE_EXPORT svtkNIFTIImageReader : public svtkImageReader2
{
public:
  //@{
  /**
   * Static method for construction.
   */
  static svtkNIFTIImageReader* New();
  svtkTypeMacro(svtkNIFTIImageReader, svtkImageReader2);
  //@}

  /**
   * Print information about this object.
   */
  void PrintSelf(ostream& os, svtkIndent indent) override;

  /**
   * Valid extensions for this file type.
   */
  const char* GetFileExtensions() override { return ".nii .nii.gz .img .img.gz .hdr .hdr.gz"; }

  /**
   * Return a descriptive name that might be useful in a GUI.
   */
  const char* GetDescriptiveName() override { return "NIfTI"; }

  /**
   * Return true if this reader can read the given file.
   */
  int CanReadFile(const char* filename) override;

  //@{
  /**
   * Read the time dimension as scalar components (default: Off).
   * If this is on, then each time point will be stored as a component in
   * the image data.  If the file has both a time dimension and a vector
   * dimension, then the number of components will be the product of these
   * two dimensions, i.e. the components will store a sequence of vectors.
   */
  svtkGetMacro(TimeAsVector, bool);
  svtkSetMacro(TimeAsVector, bool);
  svtkBooleanMacro(TimeAsVector, bool);
  //@}

  /**
   * Get the time dimension that was stored in the NIFTI header.
   */
  int GetTimeDimension() { return this->Dim[4]; }
  double GetTimeSpacing() { return this->PixDim[4]; }

  /**
   * Get the slope and intercept for rescaling the scalar values.
   * These values allow calibration of the data to real values.
   * Use the equation v = u*RescaleSlope + RescaleIntercept.
   * This directly returns the values stored in the scl_slope and
   * scl_inter fields in the NIFTI header.
   */
  double GetRescaleSlope() { return this->RescaleSlope; }
  double GetRescaleIntercept() { return this->RescaleIntercept; }

  //@{
  /**
   * Read planar RGB (separate R, G, and B planes), rather than packed RGB.
   * The NIFTI format should always use packed RGB.  The Analyze format,
   * however, was used to store both planar RGB and packed RGB depending
   * on the software, without any indication in the header about which
   * convention was being used.  Use this if you have a planar RGB file.
   */
  svtkGetMacro(PlanarRGB, bool);
  svtkSetMacro(PlanarRGB, bool);
  svtkBooleanMacro(PlanarRGB, bool);
  //@}

  /**
   * QFac gives the slice order in the NIFTI file versus the SVTK image.
   * If QFac is -1, then the SVTK slice index K is related to the NIFTI
   * slice index k by the equation K = (num_slices - k - 1).  SVTK requires
   * the slices to be ordered so that the voxel indices (I,J,K) provide a
   * right-handed coordinate system, whereas NIFTI does not.  Instead,
   * NIFTI stores a factor called "qfac" in the header to signal when the
   * (i,j,k) indices form a left-handed coordinate system.  QFac will only
   * ever have values of +1 or -1.
   */
  double GetQFac() { return this->QFac; }

  /**
   * Get a matrix that gives the "qform" orientation and offset for the data.
   * If no qform matrix was stored in the file, the return value is nullptr.
   * This matrix will transform SVTK data coordinates into the NIFTI oriented
   * data coordinates, where +X points right, +Y points anterior (toward the
   * front), and +Z points superior (toward the head). The qform matrix will
   * always have a positive determinant. The offset that is stored in the
   * matrix gives the position of the first pixel in the first slice of the
   * SVTK image data.  Note that if QFac is -1, then the first slice in the
   * SVTK image data is the last slice in the NIFTI file, and the Z offset
   * will automatically be adjusted to compensate for this.
   */
  svtkMatrix4x4* GetQFormMatrix() { return this->QFormMatrix; }

  /**
   * Get a matrix that gives the "sform" orientation and offset for the data.
   * If no sform matrix was stored in the file, the return value is nullptr.
   * Like the qform matrix, this matrix will transform SVTK data coordinates
   * into a NIFTI coordinate system.  Unlike the qform matrix, the sform
   * matrix can contain scaling information and can even (rarely) have
   * a negative determinant, i.e. a flip.  This matrix is modified slightly
   * as compared to the sform matrix stored in the NIFTI header: the pixdim
   * pixel spacing is factored out.  Also, if QFac is -1, then the SVTK slices
   * are in reverse order as compared to the NIFTI slices, hence as compared
   * to the sform matrix stored in the header, the third column of this matrix
   * is multiplied by -1 and the Z offset is shifted to compensate for the
   * fact that the last slice has become the first.
   */
  svtkMatrix4x4* GetSFormMatrix() { return this->SFormMatrix; }

  /**
   * Get the raw header information from the NIfTI file.
   */
  svtkNIFTIImageHeader* GetNIFTIHeader();

protected:
  svtkNIFTIImageReader();
  ~svtkNIFTIImageReader() override;

  /**
   * Read the header information.
   */
  int RequestInformation(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Read the voxel data.
   */
  int RequestData(svtkInformation* request, svtkInformationVector** inputVector,
    svtkInformationVector* outputVector) override;

  /**
   * Do a case-insensitive check for the given extension.
   * The check will succeed if the filename ends in ".gz", and if the
   * extension matches after removing the ".gz".
   */
  static bool CheckExtension(const char* fname, const char* ext);

  /**
   * Make a new filename by replacing extension "ext1" with "ext2".
   * The extensions must include a period, must be three characters
   * long, and must be lower case.  This method also verifies that
   * the file exists, and adds or subtracts a ".gz" as necessary
   * If the file exists, a new string is returned that must be
   * deleted by the caller.  Otherwise, the return value is nullptr.
   */
  static char* ReplaceExtension(const char* fname, const char* ext1, const char* ext2);

  /**
   * Check the version of the header.
   */
  static int CheckNIFTIVersion(const nifti_1_header* hdr);

  /**
   * Return true if an Analyze 7.5 header was found.
   */
  static bool CheckAnalyzeHeader(const nifti_1_header* hdr);

  /**
   * Read the time dimension as if it was a vector dimension.
   */
  bool TimeAsVector;

  //@{
  /**
   * Information for rescaling data to quantitative units.
   */
  double RescaleIntercept;
  double RescaleSlope;
  //@}

  /**
   * Is -1 if SVTK slice order is opposite to NIFTI slice order, +1 otherwise.
   */
  double QFac;

  //@{
  /**
   * The orientation matrices for the NIFTI file.
   */
  svtkMatrix4x4* QFormMatrix;
  svtkMatrix4x4* SFormMatrix;
  //@}

  /**
   * The dimensions of the NIFTI file.
   */
  int Dim[8];

  /**
   * The spacings in the NIFTI file.
   */
  double PixDim[8];

  /**
   * A copy of the header from the file that was most recently read.
   */
  svtkNIFTIImageHeader* NIFTIHeader;

  /**
   * Use planar RGB instead of the default (packed).
   */
  bool PlanarRGB;

private:
  svtkNIFTIImageReader(const svtkNIFTIImageReader&) = delete;
  void operator=(const svtkNIFTIImageReader&) = delete;
};

#endif // svtkNIFTIImageReader_h
