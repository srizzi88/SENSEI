#include "uses_svtk.h"

#include "svtkNew.h"
#include "svtkObject.h"

std::string svtkObject_class_name()
{
  svtkNew<svtkObject> obj;
  return obj->GetClassName();
}
