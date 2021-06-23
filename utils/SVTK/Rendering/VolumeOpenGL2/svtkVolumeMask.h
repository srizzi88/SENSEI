/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkVolumeMask.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/

#ifndef svtkVolumeMask_h
#define svtkVolumeMask_h

#include <svtkDataArray.h>
#include <svtkImageData.h>
#include <svtkOpenGLRenderWindow.h>
#include <svtkRenderWindow.h>
#include <svtkRenderer.h>
#include <svtkTextureObject.h>

#include <map> // STL required

//----------------------------------------------------------------------------
class svtkVolumeMask
{
public:
  //--------------------------------------------------------------------------
  svtkVolumeMask()
  {
    this->Texture = nullptr;
    this->Loaded = false;
    this->LoadedExtent[0] = SVTK_INT_MAX;
    this->LoadedExtent[1] = SVTK_INT_MIN;
    this->LoadedExtent[2] = SVTK_INT_MAX;
    this->LoadedExtent[3] = SVTK_INT_MIN;
    this->LoadedExtent[4] = SVTK_INT_MAX;
    this->LoadedExtent[5] = SVTK_INT_MIN;
  }

  //--------------------------------------------------------------------------
  ~svtkVolumeMask()
  {
    if (this->Texture)
    {
      this->Texture->Delete();
      this->Texture = nullptr;
    }
  }

  //--------------------------------------------------------------------------
  svtkTimeStamp GetBuildTime() { return this->BuildTime; }

  //--------------------------------------------------------------------------
  void Activate() { this->Texture->Activate(); }

  //--------------------------------------------------------------------------
  void Deactivate() { this->Texture->Deactivate(); }

  //--------------------------------------------------------------------------
  void Update(svtkRenderer* ren, svtkImageData* input, int cellFlag, int textureExtent[6],
    int scalarMode, int arrayAccessMode, int arrayId, const char* arrayName,
    svtkIdType maxMemoryInBytes)
  {
    bool needUpdate = false;
    bool modified = false;

    if (!this->Texture)
    {
      this->Texture = svtkTextureObject::New();
      needUpdate = true;
    }

    this->Texture->SetContext(svtkOpenGLRenderWindow::SafeDownCast(ren->GetRenderWindow()));

    if (!this->Texture->GetHandle())
    {
      needUpdate = true;
    }

    int obsolete = needUpdate || !this->Loaded || input->GetMTime() > this->BuildTime;
    if (!obsolete)
    {
      obsolete = cellFlag != this->LoadedCellFlag;
      int i = 0;
      while (!obsolete && i < 6)
      {
        obsolete = obsolete || this->LoadedExtent[i] > textureExtent[i];
        ++i;
        obsolete = obsolete || this->LoadedExtent[i] < textureExtent[i];
        ++i;
      }
    }

    if (obsolete)
    {
      this->Loaded = false;
      int dim[3];
      input->GetDimensions(dim);

      svtkDataArray* scalars = svtkAbstractMapper::GetScalars(
        input, scalarMode, arrayAccessMode, arrayId, arrayName, this->LoadedCellFlag);

      // DON'T USE GetScalarType() or GetNumberOfScalarComponents() on
      // ImageData as it deals only with point data...
      int scalarType = scalars->GetDataType();
      if (scalarType != SVTK_UNSIGNED_CHAR)
      {
        cout << "Mask should be SVTK_UNSIGNED_CHAR." << endl;
      }
      if (scalars->GetNumberOfComponents() != 1)
      {
        cout << "Mask should be a one-component scalar field." << endl;
      }

      GLint internalFormat = GL_R8;
      GLenum format = GL_RED;
      GLenum type = GL_UNSIGNED_BYTE;

      // Enough memory?
      int textureSize[3];
      int i = 0;
      while (i < 3)
      {
        textureSize[i] = textureExtent[2 * i + 1] - textureExtent[2 * i] + 1;
        ++i;
      }

      GLint width;
      glGetIntegerv(GL_MAX_3D_TEXTURE_SIZE, &width);
      this->Loaded = textureSize[0] <= width && textureSize[1] <= width && textureSize[2] <= width;
      if (this->Loaded)
      {
        // so far, so good but some cards don't report allocation error
        this->Loaded = textureSize[0] * textureSize[1] * textureSize[2] *
            svtkAbstractArray::GetDataTypeSize(scalarType) * scalars->GetNumberOfComponents() <=
          maxMemoryInBytes;
        if (this->Loaded)
        {
          glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

          if (!(textureExtent[1] - textureExtent[0] + cellFlag == dim[0]))
          {
            glPixelStorei(GL_UNPACK_ROW_LENGTH, dim[0] - cellFlag);
          }
          if (!(textureExtent[3] - textureExtent[2] + cellFlag == dim[1]))
          {
            glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, dim[1] - cellFlag);
          }
          void* dataPtr = scalars->GetVoidPointer(
            ((textureExtent[4] * (dim[1] - cellFlag) + textureExtent[2]) * (dim[0] - cellFlag) +
              textureExtent[0]) *
            scalars->GetNumberOfComponents());

          this->Texture->SetDataType(type);
          this->Texture->SetFormat(format);
          this->Texture->SetInternalFormat(internalFormat);
          this->Texture->Create3DFromRaw(
            textureSize[0], textureSize[1], textureSize[2], 1, scalarType, dataPtr);
          this->Texture->SetWrapS(svtkTextureObject::ClampToEdge);
          this->Texture->SetWrapT(svtkTextureObject::ClampToEdge);
          this->Texture->SetWrapR(svtkTextureObject::ClampToEdge);
          this->Texture->SetMagnificationFilter(svtkTextureObject::Nearest);
          this->Texture->SetMinificationFilter(svtkTextureObject::Nearest);
          this->Texture->SetBorderColor(0.0f, 0.0f, 0.0f, 0.0f);

          // Restore the default values.
          glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);
          glPixelStorei(GL_UNPACK_IMAGE_HEIGHT, 0);

          this->LoadedCellFlag = cellFlag;
          i = 0;
          while (i < 6)
          {
            this->LoadedExtent[i] = textureExtent[i];
            ++i;
          }

          double spacing[3];
          double origin[3];
          input->GetSpacing(spacing);
          input->GetOrigin(origin);
          int swapBounds[3];
          swapBounds[0] = (spacing[0] < 0);
          swapBounds[1] = (spacing[1] < 0);
          swapBounds[2] = (spacing[2] < 0);

          if (!this->LoadedCellFlag) // loaded extents represent points
          {
            // slabsPoints[i]=(slabsDataSet[i] - origin[i/2]) / spacing[i/2];
            // in general, x=o+i*spacing.
            // if spacing is positive min extent match the min of the
            // bounding box
            // and the max extent match the max of the bounding box
            // if spacing is negative min extent match the max of the
            // bounding box
            // and the max extent match the min of the bounding box

            // if spacing is negative, we may have to rethink the equation
            // between real point and texture coordinate...
            this->LoadedBounds[0] =
              origin[0] + static_cast<double>(this->LoadedExtent[0 + swapBounds[0]]) * spacing[0];
            this->LoadedBounds[2] =
              origin[1] + static_cast<double>(this->LoadedExtent[2 + swapBounds[1]]) * spacing[1];
            this->LoadedBounds[4] =
              origin[2] + static_cast<double>(this->LoadedExtent[4 + swapBounds[2]]) * spacing[2];
            this->LoadedBounds[1] =
              origin[0] + static_cast<double>(this->LoadedExtent[1 - swapBounds[0]]) * spacing[0];
            this->LoadedBounds[3] =
              origin[1] + static_cast<double>(this->LoadedExtent[3 - swapBounds[1]]) * spacing[1];
            this->LoadedBounds[5] =
              origin[2] + static_cast<double>(this->LoadedExtent[5 - swapBounds[2]]) * spacing[2];
          }
          else // loaded extents represent cells
          {
            int wholeTextureExtent[6];
            input->GetExtent(wholeTextureExtent);
            i = 1;
            while (i < 6)
            {
              wholeTextureExtent[i]--;
              i += 2;
            }

            i = 0;
            while (i < 3)
            {
              if (this->LoadedExtent[2 * i] == wholeTextureExtent[2 * i])
              {
                this->LoadedBounds[2 * i + swapBounds[i]] = origin[i];
              }
              else
              {
                this->LoadedBounds[2 * i + swapBounds[i]] =
                  origin[i] + (static_cast<double>(this->LoadedExtent[2 * i]) + 0.5) * spacing[i];
              }

              if (this->LoadedExtent[2 * i + 1] == wholeTextureExtent[2 * i + 1])
              {
                this->LoadedBounds[2 * i + 1 - swapBounds[i]] = origin[i] +
                  (static_cast<double>(this->LoadedExtent[2 * i + 1]) + 1.0) * spacing[i];
              }
              else
              {
                this->LoadedBounds[2 * i + 1 - swapBounds[i]] = origin[i] +
                  (static_cast<double>(this->LoadedExtent[2 * i + 1]) + 0.5) * spacing[i];
              }
              ++i;
            }
          }
          modified = true;
        }
      }
    }

    if (modified)
    {
      this->BuildTime.Modified();
    }
  }

  //--------------------------------------------------------------------------
  double* GetLoadedBounds() { return this->LoadedBounds; }

  //--------------------------------------------------------------------------
  svtkIdType* GetLoadedExtent() { return this->LoadedExtent; }

  //--------------------------------------------------------------------------
  int GetLoadedCellFlag() { return this->LoadedCellFlag; }

  //--------------------------------------------------------------------------
  bool IsLoaded() { return this->Loaded; }

  // Get the texture unit
  //--------------------------------------------------------------------------
  int GetTextureUnit(void)
  {
    if (!this->Texture)
    {
      return -1;
    }
    return this->Texture->GetTextureUnit();
  }

  //--------------------------------------------------------------------------
  void ReleaseGraphicsResources(svtkWindow* window)
  {
    if (this->Texture)
    {
      this->Texture->ReleaseGraphicsResources(window);
      this->Texture->Delete();
      this->Texture = nullptr;
    }
  }

protected:
  svtkTextureObject* Texture;
  svtkTimeStamp BuildTime;

  double LoadedBounds[6];
  svtkIdType LoadedExtent[6];

  int LoadedCellFlag;
  bool Loaded;
};

//----------------------------------------------------------------------------
class svtkMapMaskTextureId
{
public:
  std::map<svtkImageData*, svtkVolumeMask*> Map;
  svtkMapMaskTextureId() {}

private:
  svtkMapMaskTextureId(const svtkMapMaskTextureId& other);
  svtkMapMaskTextureId& operator=(const svtkMapMaskTextureId& other);
};

#endif // svtkVolumeMask_h
// SVTK-HeaderTest-Exclude: svtkVolumeMask.h
