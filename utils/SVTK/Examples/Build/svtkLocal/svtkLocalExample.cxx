/*=========================================================================
This source has no copyright.  It is intended to be copied by users
wishing to create their own SVTK classes locally.
=========================================================================*/
#include "svtkLocalExample.h"

#include "svtkObjectFactory.h"

svtkStandardNewMacro(svtkLocalExample);

//----------------------------------------------------------------------------
svtkLocalExample::svtkLocalExample() = default;

//----------------------------------------------------------------------------
svtkLocalExample::~svtkLocalExample() = default;

//----------------------------------------------------------------------------
void svtkLocalExample::PrintSelf(ostream& os, svtkIndent indent)
{
  this->Superclass::PrintSelf(os, indent);
}
