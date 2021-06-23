#ifndef svtkMySQLDatabasePrivate_h
#define svtkMySQLDatabasePrivate_h

#ifdef _WIN32
#include <winsock.h> // mysql.h relies on the typedefs from here
#endif

#include "svtkIOMySQLModule.h" // For export macro
#include <mysql.h>            // needed for MYSQL typedefs

class SVTKIOMYSQL_EXPORT svtkMySQLDatabasePrivate
{
public:
  svtkMySQLDatabasePrivate()
    : Connection(nullptr)
  {
    mysql_init(&this->NullConnection);
  }

  MYSQL NullConnection;
  MYSQL* Connection;
};

#endif // svtkMySQLDatabasePrivate_h
// SVTK-HeaderTest-Exclude: svtkMySQLDatabasePrivate.h
