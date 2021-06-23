/*******************************************************************/
/*                               XDMF                              */
/*                   eXtensible Data Model and Format              */
/*                                                                 */
/*  Id : Id  */
/*                                                                 */
/*  Author:                                                        */
/*     Jerry A. Clarke                                             */
/*     clarke@arl.army.mil                                         */
/*     US Army Research Laboratory                                 */
/*     Aberdeen Proving Ground, MD                                 */
/*                                                                 */
/*     Copyright @ 2002 US Army Research Laboratory                */
/*     All Rights Reserved                                         */
/*     See Copyright.txt or http://www.arl.hpc.mil/ice for details */
/*                                                                 */
/*     This software is distributed WITHOUT ANY WARRANTY; without  */
/*     even the implied warranty of MERCHANTABILITY or FITNESS     */
/*     FOR A PARTICULAR PURPOSE.  See the above copyright notice   */
/*     for more information.                                       */
/*                                                                 */
/*******************************************************************/
#ifndef svtkXdmfDataArray_h
#define svtkXdmfDataArray_h

#include "svtkIOXdmf2Module.h" // For export macro
#include "svtkObject.h"

class svtkDataArray;
namespace xdmf2
{
class XdmfArray;
}

class SVTKIOXDMF2_EXPORT svtkXdmfDataArray : public svtkObject
{
public:
  static svtkXdmfDataArray* New();
  svtkTypeMacro(svtkXdmfDataArray, svtkObject);
  void PrintSelf(ostream& os, svtkIndent indent) override;

  svtkDataArray* FromArray(void);

  char* ToArray(void);

  svtkDataArray* FromXdmfArray(char* ArrayName = nullptr, int CopyShape = 1, int rank = 1,
    int Components = 1, int MakeCopy = 1);

  char* ToXdmfArray(svtkDataArray* DataArray = nullptr, int CopyShape = 1);

  void SetArray(char* TagName);

  char* GetArray(void);

  void SetVtkArray(svtkDataArray* array);

  svtkDataArray* GetVtkArray(void);

protected:
  svtkXdmfDataArray();

private:
  svtkDataArray* svtkArray;
  xdmf2::XdmfArray* Array;
  svtkXdmfDataArray(const svtkXdmfDataArray&) = delete;
  void operator=(const svtkXdmfDataArray&) = delete;
};

#endif /* svtkXdmfDataArray_h */
