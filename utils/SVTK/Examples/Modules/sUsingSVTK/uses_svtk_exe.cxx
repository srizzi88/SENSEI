#include "uses_svtk.h"

int main(int argc, char* argv[])
{
  if (svtkObject_class_name() == "svtkObject")
  {
    return 0;
  }
  else
  {
    return 1;
  }
}
