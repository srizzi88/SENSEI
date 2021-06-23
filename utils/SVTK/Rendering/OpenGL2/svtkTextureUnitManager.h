/*=========================================================================

  Program:   Visualization Toolkit
  Module:    svtkTextureUnitManager.h

  Copyright (c) Ken Martin, Will Schroeder, Bill Lorensen
  All rights reserved.
  See Copyright.txt or http://www.kitware.com/Copyright.htm for details.

     This software is distributed WITHOUT ANY WARRANTY; without even
     the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
     PURPOSE.  See the above copyright notice for more information.

=========================================================================*/
/**
 * @class   svtkTextureUnitManager
 * @brief   allocate/free texture units.
 *
 *
 * svtkTextureUnitManager is a central place used by shaders to reserve a
 * texture unit ( Allocate() ) or release it ( Free() ).
 *
 * Don't create a svtkTextureUnitManager, query it from the
 * svtkOpenGLRenderWindow
 *
 * @sa
 * svtkOpenGLRenderWindow
 */

#ifndef svtkTextureUnitManager_h
#define svtkTextureUnitManager_h

#include "svtkObject.h"
#include "svtkRenderingOpenGL2Module.h" // For export macro

class svtkOpenGLRenderWindow;

class SVTKRENDERINGOPENGL2_EXPORT svtkTextureUnitManager : public svtkObject
{
public:
  svtkTypeMacro(svtkTextureUnitManager, svtkObject);

  void PrintSelf(ostream& os, svtkIndent indent) override;

  static svtkTextureUnitManager* New();

  /**
   * Update the number of hardware texture units for the current context
   */
  void Initialize();

  /**
   * Number of texture units supported by the OpenGL context.
   */
  int GetNumberOfTextureUnits();

  /**
   * Reserve a texture unit. It returns its number.
   * It returns -1 if the allocation failed (because there are no more
   * texture units left).
   * \post valid_result: result==-1 || result>=0 && result<this->GetNumberOfTextureUnits())
   * \post allocated: result==-1 || this->IsAllocated(result)
   */
  virtual int Allocate();

  /**
   * Reserve a specific texture unit if not already in use.
   * This method should only be used when interacting with 3rd
   * party code that is allocating and using textures. It allows
   * someone to reserve a texture unit for that code and later release
   * it. SVTK will not use that texture unit until it is released.
   * It returns -1 if the allocation failed (because there are no more
   * texture units left).
   * \post valid_result: result==-1 || result>=0 && result<this->GetNumberOfTextureUnits())
   * \post allocated: result==-1 || this->IsAllocated(result)
   */
  virtual int Allocate(int unit);

  /**
   * Tell if texture unit `textureUnitId' is already allocated.
   * \pre valid_textureUnitId_range : textureUnitId>=0 &&
   * textureUnitId<this->GetNumberOfTextureUnits()
   */
  bool IsAllocated(int textureUnitId);

  /**
   * Release a texture unit.
   * \pre valid_textureUnitId: textureUnitId>=0 && textureUnitId<this->GetNumberOfTextureUnits()
   * \pre allocated_textureUnitId: this->IsAllocated(textureUnitId)
   */
  virtual void Free(int textureUnitId);

protected:
  /**
   * Default constructor.
   */
  svtkTextureUnitManager();

  /**
   * Destructor.
   */
  ~svtkTextureUnitManager() override;

  /**
   * Delete the allocation table and check if it is not called before
   * all the texture units have been released.
   */
  void DeleteTable();

  int NumberOfTextureUnits;
  bool* TextureUnits;

private:
  svtkTextureUnitManager(const svtkTextureUnitManager&) = delete;
  void operator=(const svtkTextureUnitManager&) = delete;
};

#endif
