/*
 * Copyright (c) 2014, 2016, Oracle and/or its affiliates. All rights reserved.
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License as
 * published by the Free Software Foundation; version 2 of the
 * License.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA
 * 02110-1301  USA
 */

#ifdef __clang__
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-private-field"
#endif

#include "utils/utils_sqlstring.h"
#include "mod_mysql_session.h"

#ifdef __clang__
#pragma clang diagnostic pop
#endif

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "shellcore/server_registry.h"

#include "shellcore/proxy_object.h"
#include "mysqlxtest_utils.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>
#include <set>

#include "mysql_connection.h"
#include "mod_mysql_resultset.h"
#include "mod_mysql_schema.h"
#include "utils/utils_general.h"

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace mysh;
using namespace mysh::mysql;
using namespace shcore;

REGISTER_OBJECT(mysql, ClassicSession);

ClassicSession::ClassicSession()
{
  init();
}

ClassicSession::ClassicSession(const ClassicSession& session) :
ShellDevelopmentSession(session), _conn(session._conn)
{
  init();
}

void ClassicSession::init()
{
  //_schema_proxy.reset(new Proxy_object(boost::bind(&ClassicSession::get_db, this, _1)));

  add_method("close", boost::bind(&ClassicSession::close, this, _1), "data");
  add_method("runSql", boost::bind(&ClassicSession::run_sql, this, _1),
    "stmt", shcore::String,
    NULL);
  add_method("setCurrentSchema", boost::bind(&ClassicSession::set_current_schema, this, _1), "name", shcore::String, NULL);
  add_method("getCurrentSchema", boost::bind(&ShellDevelopmentSession::get_member_method, this, _1, "getCurrentSchema", "currentSchema"), NULL);
  add_method("startTransaction", boost::bind(&ClassicSession::startTransaction, this, _1), "data");
  add_method("commit", boost::bind(&ClassicSession::commit, this, _1), "data");
  add_method("rollback", boost::bind(&ClassicSession::rollback, this, _1), "data");
  add_method("dropSchema", boost::bind(&ClassicSession::drop_schema, this, _1), "data");
  add_method("dropTable", boost::bind(&ClassicSession::drop_schema_object, this, _1, "Table"), "data");
  add_method("dropView", boost::bind(&ClassicSession::drop_schema_object, this, _1, "View"), "data");

  _schemas.reset(new shcore::Value::Map_type);

  // Prepares the cache handling
  auto generator = [this](const std::string& name){return shcore::Value::wrap<ClassicSchema>(new ClassicSchema(shared_from_this(), name)); };
  update_schema_cache = [generator, this](const std::string &name, bool exists){DatabaseObject::update_cache(name, generator, exists, _schemas); };
}

Connection *ClassicSession::connection()
{
  return _conn.get();
}

Value ClassicSession::connect(const Argument_list &args)
{
  args.ensure_count(1, 2, "ClassicSession.connect");

  try
  {
    // Retrieves the connection data, whatever the source is
    load_connection_data(args);

    // Performs the connection
    _conn.reset(new Connection(_host, _port, _sock, _user, _password, _schema, _ssl_ca, _ssl_cert, _ssl_key));

    _default_schema = _retrieve_current_schema();
  }
  CATCH_AND_TRANSLATE();

  return Value::Null();
}

#ifdef DOXYGEN
/**
* Closes the internal connection to the MySQL Server held on this session object.
*/
Undefined ClassicSession::close(){}
#endif
Value ClassicSession::close(const shcore::Argument_list &args)
{
  args.ensure_count(0, "ClassicSession.close");

  // Connection must be explicitly closed, we can't rely on the
  // automatic destruction because if shared across different objects
  // it may remain open
  if (_conn)
    _conn->close();

  _conn.reset();

  return shcore::Value();
}

#ifdef DOXYGEN
/**
* Executes a query against the database and returns a  ClassicResult object wrapping the result.
* \param query the SQL query to execute against the database.
* \return A ClassicResult object.
* \exception An exception is thrown if an error occurs on the SQL execution.
*/
ClassicResult ClassicSession::runSql(String query){}
#endif
Value ClassicSession::run_sql(const shcore::Argument_list &args) const
{
  args.ensure_count(1, "ClassicSession.sql");
  // Will return the result of the SQL execution
  // In case of error will be Undefined
  Value ret_val;
  if (!_conn)
    throw Exception::logic_error("Not connected.");
  else
  {
    // Options are the statement and optionally options to modify
    // How the resultset is created.
    std::string statement = args.string_at(0);

    if (statement.empty())
      throw Exception::argument_error("No query specified.");
    else
      ret_val = Value::wrap(new ClassicResult(boost::shared_ptr<Result>(_conn->run_sql(statement))));
  }

  return ret_val;
}

#ifdef DOXYGEN
/**
* Creates a schema on the database and returns the corresponding object.
* \param name A string value indicating the schema name.
* \return The created schema object.
* \exception An exception is thrown if an error occurs creating the Session.
*/
ClassicSchema ClassicSession::createSchema(String name){}
#endif
Value ClassicSession::create_schema(const shcore::Argument_list &args)
{
  args.ensure_count(1, "ClassicSession.createSchema");

  Value ret_val;
  if (!_conn)
    throw Exception::logic_error("Not connected.");
  else
  {
    // Options are the statement and optionally options to modify
    // How the resultset is created.
    std::string schema = args.string_at(0);

    if (schema.empty())
      throw Exception::argument_error("The schema name can not be empty.");
    else
    {
      std::string statement = sqlstring("create schema !", 0) << schema;
      ret_val = Value::wrap(new ClassicResult(boost::shared_ptr<Result>(_conn->run_sql(statement))));

      boost::shared_ptr<ClassicSchema> object(new ClassicSchema(shared_from_this(), schema));

      // If reached this point it indicates the schema was created successfully
      ret_val = shcore::Value(boost::static_pointer_cast<Object_bridge>(object));
      (*_schemas)[schema] = ret_val;
    }
  }

  return ret_val;
}

std::vector<std::string> ClassicSession::get_members() const
{
  std::vector<std::string> members(ShellDevelopmentSession::get_members());

  members.push_back("currentSchema");

  return members;
}

#ifdef DOXYGEN
/**
* Retrieves the ClassicSchema configured as default for the session.
* \return A ClassicSchema object or Null
*
* If the configured schema is not valid anymore Null wil be returned.
*/
ClassicSchema ClassicSession::getDefaultSchema(){}

/**
* Retrieves the ClassicSchema that is active as current on the session.
* \return A ClassicSchema object or Null
*
* The current schema is configured either throu setCurrentSchema(String name) or by using the USE statement.
*/
ClassicSchema ClassicSession::getCurrentSchema(){}

/**
* Returns the connection string passed to connect() method.
* \return A string representation of the connection data in URI format (excluding the password or the database).
*/
String ClassicSession::getUri(){}
#endif
Value ClassicSession::get_member(const std::string &prop) const
{
  // Retrieves the member first from the parent
  Value ret_val;

  // Check the member is on the base classes before attempting to
  // retrieve it since it may throw invalid member otherwise
  // If not on the parent classes and not here then we can safely assume
  // it is a schema and attempt loading it as such
  if (ShellDevelopmentSession::has_member(prop))
    ret_val = ShellDevelopmentSession::get_member(prop);
  else if (prop == "currentSchema")
  {
    ClassicSession *session = const_cast<ClassicSession *>(this);
    std::string name = session->_retrieve_current_schema();

    if (!name.empty())
    {
      shcore::Argument_list args;
      args.push_back(shcore::Value(name));
      ret_val = get_schema(args);
    }
    else
      ret_val = Value::Null();
  }

  return ret_val;
}

bool ClassicSession::has_member(const std::string &prop) const
{
  return ShellDevelopmentSession::has_member(prop) ||
    prop == "currentSchema";
}

std::string ClassicSession::_retrieve_current_schema()
{
  std::string name;

  if (_conn)
  {
    shcore::Argument_list query;
    query.push_back(Value("select schema()"));

    Value res = run_sql(query);

    boost::shared_ptr<ClassicResult> rset = res.as_object<ClassicResult>();
    Value next_row = rset->fetch_one(shcore::Argument_list());

    if (next_row)
    {
      boost::shared_ptr<mysh::Row> row = next_row.as_object<mysh::Row>();
      shcore::Value schema = row->get_member("schema()");

      if (schema)
        name = schema.as_string();
    }
  }

  return name;
}

void ClassicSession::_remove_schema(const std::string& name)
{
  if (_schemas->find(name) != _schemas->end())
    _schemas->erase(name);
}

#ifdef DOXYGEN
/**
* Retrieves a ClassicSchema object from the current session through it's name.
* \param name The name of the ClassicSchema object to be retrieved.
* \return The ClassicSchema object with the given name.
* \exception An exception is thrown if the given name is not a valid schema on the Session.
* \sa ClassicSchema
*/
ClassicSchema ClassicSession::getSchema(String name){}
#endif
shcore::Value ClassicSession::get_schema(const shcore::Argument_list &args) const
{
  std::string function_name = class_name() + ".getSchema";
  args.ensure_count(1, function_name.c_str());
  shcore::Value ret_val;

  std::string type = "Schema";
  std::string search_name = args.string_at(0);
  std::string name = db_object_exists(type, search_name, "");

  if (!name.empty())
  {
    update_schema_cache(name, true);

    ret_val = (*_schemas)[name];

    ret_val.as_object<ClassicSchema>()->update_cache();
  }
  else
  {
    update_schema_cache(search_name, false);

    throw Exception::runtime_error("Unknown database '" + search_name + "'");
  }

  return ret_val;
}

#ifdef DOXYGEN
/**
* Retrieves the Schemas available on the session.
* \return A List containing the ClassicSchema objects available o the session.
*/
List ClassicSession::getSchemas(){}
#endif
shcore::Value ClassicSession::get_schemas(const shcore::Argument_list &args) const
{
  shcore::Value::Array_type_ref schemas(new shcore::Value::Array_type);

  if (_conn)
  {
    shcore::Argument_list query;
    query.push_back(Value("show databases;"));

    Value res = run_sql(query);

    shcore::Argument_list args;
    boost::shared_ptr<ClassicResult> rset = res.as_object<ClassicResult>();
    Value next_row = rset->fetch_one(args);
    boost::shared_ptr<mysh::Row> row;

    while (next_row)
    {
      row = next_row.as_object<mysh::Row>();
      shcore::Value schema = row->get_member("Database");
      if (schema)
      {
        update_schema_cache(schema.as_string(), true);

        schemas->push_back((*_schemas)[schema.as_string()]);
      }

      next_row = rset->fetch_one(args);
    }
  }

  return shcore::Value(schemas);
}

#ifdef DOXYGEN
/**
* Sets the selected schema for this session's connection.
* \return The new schema.
*/
ClassicSchema ClassicSession::setCurrentSchema(String schema){}
#endif
shcore::Value ClassicSession::set_current_schema(const shcore::Argument_list &args)
{
  args.ensure_count(1, "ClassicSession.setCurrentSchema");

  if (_conn)
  {
    std::string name = args[0].as_string();

    shcore::Argument_list query;
    query.push_back(Value(sqlstring("use !", 0) << name));

    Value res = run_sql(query);
  }
  else
    throw Exception::runtime_error("ClassicSession not connected");

  return get_member("currentSchema");
}

boost::shared_ptr<shcore::Object_bridge> ClassicSession::create(const shcore::Argument_list &args)
{
  return connect_session(args, mysh::Classic);
}

#ifdef DOXYGEN
/**
* Drops the schema with the specified name.
* \return A ClassicResult object if succeeded.
* \exception An error is raised if the schema did not exist.
*/
ClassicResult ClassicSession::dropSchema(String name){}
#endif
shcore::Value ClassicSession::drop_schema(const shcore::Argument_list &args)
{
  std::string function = class_name() + ".dropSchema";

  args.ensure_count(1, function.c_str());

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error(function + ": Argument #1 is expected to be a string");

  std::string name = args[0].as_string();

  Value ret_val = Value::wrap(new ClassicResult(boost::shared_ptr<Result>(_conn->run_sql(sqlstring("drop schema !", 0) << name))));

  _remove_schema(name);

  return ret_val;
}

#ifdef DOXYGEN
/**
* Drops a table from the specified schema.
* \return A ClassicResult object if succeeded.
* \exception An error is raised if the table did not exist.
*/
ClassicResult ClassicSession::dropTable(String schema, String name){}

/**
* Drops a view from the specified schema.
* \return A ClassicResult object if succeeded.
* \exception An error is raised if the view did not exist.
*/
ClassicResult ClassicSession::dropView(String schema, String name){}
#endif
shcore::Value ClassicSession::drop_schema_object(const shcore::Argument_list &args, const std::string& type)
{
  std::string function = class_name() + ".drop" + type;

  args.ensure_count(2, function.c_str());

  if (args[0].type != shcore::String)
    throw shcore::Exception::argument_error(function + ": Argument #1 is expected to be a string");

  if (args[1].type != shcore::String)
    throw shcore::Exception::argument_error(function + ": Argument #2 is expected to be a string");

  std::string schema = args[0].as_string();
  std::string name = args[1].as_string();

  std::string statement;
  if (type == "Table")
    statement = "drop table !.!";
  else
    statement = "drop view !.!";

  statement = sqlstring(statement.c_str(), 0) << schema << name;

  Value ret_val = Value::wrap(new ClassicResult(boost::shared_ptr<Result>(_conn->run_sql(statement))));

  if (_schemas->count(schema))
  {
    boost::shared_ptr<ClassicSchema> schema_obj = boost::static_pointer_cast<ClassicSchema>((*_schemas)[schema].as_object());
    if (schema_obj)
      schema_obj->_remove_object(name, type);
  }

  return ret_val;
}

/*
* This function verifies if the given object exist in the database, works for schemas, tables and views.
* The check for tables and views is done is done based on the type.
* If type is not specified and an object with the name is found, the type will be returned.
*/

std::string ClassicSession::db_object_exists(std::string &type, const std::string &name, const std::string& owner) const
{
  std::string statement;
  std::string ret_val;

  if (type == "Schema")
  {
    statement = sqlstring("show databases like ?", 0) << name;
    Result *res = _conn->run_sql(statement);
    if (res->has_resultset())
    {
      Row *row = res->fetch_one();
      if (row)
        ret_val = row->get_value(0).as_string();
    }
  }
  else
  {
    statement = sqlstring("show full tables from ! like ?", 0) << owner << name;
    Result *res = _conn->run_sql(statement);

    if (res->has_resultset())
    {
      Row *row = res->fetch_one();

      if (row)
      {
        std::string db_type = row->get_value(1).as_string();

        if (type == "Table" && (db_type == "BASE TABLE" || db_type == "LOCAL TEMPORARY"))
          ret_val = row->get_value(0).as_string();
        else if (type == "View" && (db_type == "VIEW" || db_type == "SYSTEM VIEW"))
          ret_val = row->get_value(0).as_string();
        else if (type.empty())
        {
          ret_val = row->get_value(0).as_string();
          type = db_type;
        }
      }
    }
  }

  return ret_val;
}

#ifdef DOXYGEN
/**
* Starts a transaction context on the server.
* \return A ClassicResult object.
* Calling this function will turn off the autocommit mode on the server.
*
* All the operations executed after calling this function will take place only when commit() is called.
*
* All the operations executed after calling this function, will be discarded is rollback() is called.
*
* When commit() or rollback() are called, the server autocommit mode will return back to it's state before calling startTransaction().
*/
ClassicResult ClassicSession::startTransaction(){}
#endif
shcore::Value ClassicSession::startTransaction(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".startTransaction";
  args.ensure_count(0, function_name.c_str());

  return Value::wrap(new ClassicResult(boost::shared_ptr<Result>(_conn->run_sql("start transaction"))));
}

#ifdef DOXYGEN
/**
* Commits all the operations executed after a call to startTransaction().
* \return A ClassicResult object.
*
* All the operations executed after calling startTransaction() will take place when this function is called.
*
* The server autocommit mode will return back to it's state before calling startTransaction().
*/
ClassicResult ClassicSession::commit(){}
#endif
shcore::Value ClassicSession::commit(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".startTransaction";
  args.ensure_count(0, function_name.c_str());

  return Value::wrap(new ClassicResult(boost::shared_ptr<Result>(_conn->run_sql("commit"))));
}

#ifdef DOXYGEN
/**
* Discards all the operations executed after a call to startTransaction().
* \return A ClassicResult object.
*
* All the operations executed after calling startTransaction() will be discarded when this function is called.
*
* The server autocommit mode will return back to it's state before calling startTransaction().
*/
ClassicResult ClassicSession::rollback(){}
#endif
shcore::Value ClassicSession::rollback(const shcore::Argument_list &args)
{
  std::string function_name = class_name() + ".startTransaction";
  args.ensure_count(0, function_name.c_str());

  return Value::wrap(new ClassicResult(boost::shared_ptr<Result>(_conn->run_sql("rollback"))));
}

shcore::Value ClassicSession::get_status(const shcore::Argument_list &args)
{
  shcore::Value::Map_type_ref status(new shcore::Value::Map_type);

  Result *result;
  Row *row;

  result = _conn->run_sql("select DATABASE(), USER() limit 1");
  row = result->fetch_one();

  (*status)["SESSION_TYPE"] = shcore::Value("Classic");
  (*status)["DEFAULT_SCHEMA"] = shcore::Value(_default_schema);

  std::string current_schema = row->get_value(0).descr(true);
  if (current_schema == "null")
    current_schema = "";

  (*status)["CURRENT_SCHEMA"] = shcore::Value(current_schema);
  (*status)["CURRENT_USER"] = row->get_value(1);
  (*status)["CONNECTION_ID"] = shcore::Value(uint64_t(_conn->get_thread_id()));
  (*status)["SSL_CIPHER"] = shcore::Value(_conn->get_ssl_cipher());
  //(*status)["SKIP_UPDATES"] = shcore::Value(???);
  //(*status)["DELIMITER"] = shcore::Value(???);

  (*status)["SERVER_INFO"] = shcore::Value(_conn->get_server_info());

  (*status)["PROTOCOL_VERSION"] = shcore::Value(uint64_t(_conn->get_protocol_info()));
  (*status)["CONNECTION"] = shcore::Value(_conn->get_connection_info());
  //(*status)["INSERT_ID"] = shcore::Value(???);

  result = _conn->run_sql("select @@character_set_client, @@character_set_connection, @@character_set_server, @@character_set_database, @@version_comment limit 1");
  row = result->fetch_one();
  (*status)["CLIENT_CHARSET"] = row->get_value(0);
  (*status)["CONNECTION_CHARSET"] = row->get_value(1);
  (*status)["SERVER_CHARSET"] = row->get_value(2);
  (*status)["SCHEMA_CHARSET"] = row->get_value(3);
  (*status)["SERVER_VERSION"] = row->get_value(4);

  (*status)["SERVER_STATS"] = shcore::Value(_conn->get_stats());

  // TODO: Review retrieval from charset_info, mysql connection

  // TODO: Embedded library stuff
  //(*status)["TCP_PORT"] = row->get_value(1);
  //(*status)["UNIX_SOCKET"] = row->get_value(2);
  //(*status)["PROTOCOL_COMPRESSED"] = row->get_value(3);

  // STATUS

  // SAFE UPDATES

  return shcore::Value(status);
}