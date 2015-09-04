/*
* Copyright (c) 2015, Oracle and/or its affiliates. All rights reserved.
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

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>

#include "gtest/gtest.h"
#include "test_utils.h"
#include "base_session.h"

namespace shcore {
  class Shell_py_mysql_session_tests : public Shell_core_test_wrapper
  {
  protected:
    // You can define per-test set-up and tear-down logic as usual.
    virtual void SetUp()
    {
      Shell_core_test_wrapper::SetUp();

      bool initilaized(false);
      _shell_core->switch_mode(Shell_core::Mode_Python, initilaized);
    }
  };

  // Tests session.getDefaultSchema()
  TEST_F(Shell_py_mysql_session_tests, mysql_classic_session_members)
  {
    exec_and_out_equals("import mysql");

    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

    // Ensures the right members exist
    exec_and_out_equals("members = dir(session)");
    exec_and_out_equals("index=members.index('close')");
    exec_and_out_equals("index=members.index('createSchema')");
    exec_and_out_equals("index=members.index('getDefaultSchema')");
    exec_and_out_equals("index=members.index('getSchema')");
    exec_and_out_equals("index=members.index('getSchemas')");
    exec_and_out_equals("index=members.index('getUri')");
    exec_and_out_equals("index=members.index('setDefaultSchema')");
    exec_and_out_equals("index=members.index('sql')");
    exec_and_out_equals("index=members.index('defaultSchema')");
    exec_and_out_equals("index=members.index('schemas')");
    exec_and_out_equals("index=members.index('uri')");

    exec_and_out_equals("session.close()");
  }

  // Tests session.getDefaultSchema()
  TEST_F(Shell_py_mysql_session_tests, mysql_base_session_get_uri)
  {
    exec_and_out_equals("import mysql");

    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

    exec_and_out_equals("print(session.getUri())", uri);

    exec_and_out_equals("session.close()");
  }

  // Tests session.getDefaultSchema()
  TEST_F(Shell_py_mysql_session_tests, mysql_base_session_uri)
  {
    exec_and_out_equals("import mysql");

    std::string uri = mysh::strip_password(_mysql_uri);

    exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

    exec_and_out_equals("print(session.uri)", uri);

    exec_and_out_equals("session.close()");
  }

  // Tests session.getDefaultSchema()
  TEST_F(Shell_py_mysql_session_tests, mysql_base_session_get_default_schema)
  {
    {
      SCOPED_TRACE("retrieving the default schema for first time");
      exec_and_out_equals("import mysql");

      exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

      // Attempts to get the default schema
      exec_and_out_equals("schema = session.getDefaultSchema()");

      exec_and_out_equals("print(schema)", "None");
    };

    {
      SCOPED_TRACE("Setting/getting default schema.");
      exec_and_out_equals("session.setDefaultSchema('mysql')");

      // Now uses the formal method
      exec_and_out_equals("schema = session.getDefaultSchema()");
      exec_and_out_equals("print(schema)", "<ClassicSchema:mysql>");
    };

    {
      // Sets a different default database
      exec_and_out_equals("session.setDefaultSchema('information_schema')");

      // Now uses the formal method
      exec_and_out_equals("schema = session.getDefaultSchema()");
      exec_and_out_equals("print(schema)", "<ClassicSchema:information_schema>");
    };

    exec_and_out_equals("session.close()");
  }

  // Tests session.defaultSchema
  TEST_F(Shell_py_mysql_session_tests, mysql_base_session_default_schema)
  {
    {
      SCOPED_TRACE("retrieving the default schema for first time");
      exec_and_out_equals("import mysql");

      exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

      // Attempts to get the default schema
      exec_and_out_equals("print(session.defaultSchema)", "None");
    };

    {
      SCOPED_TRACE("Setting/getting default schema.");
      exec_and_out_equals("session.setDefaultSchema('mysql')");

      // Now uses the formal method
      exec_and_out_equals("print(session.defaultSchema)", "<ClassicSchema:mysql>");
    };

    {
      // Sets a different default database
      exec_and_out_equals("session.setDefaultSchema('information_schema')");

      // Now uses the formal method
      exec_and_out_equals("print(session.defaultSchema)", "<ClassicSchema:information_schema>");
    };

    exec_and_out_equals("session.close()");
  }

  // Tests session.getSchemas()
  TEST_F(Shell_py_mysql_session_tests, mysql_base_session_get_schemas)
  {
    exec_and_out_equals("import mysql");

    exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

    // Triggers the schema load
    exec_and_out_equals("schemas = session.getSchemas()");

    // Now checks the objects can be accessed directly
    // because has been already set as a session attributes
    exec_and_out_equals("print(schemas.mysql)", "<ClassicSchema:mysql>");
    exec_and_out_equals("print(schemas.information_schema)", "<ClassicSchema:information_schema>");

    exec_and_out_equals("session.close()");
  }

  // Tests session.schemas
  TEST_F(Shell_py_mysql_session_tests, mysql_base_session_schemas)
  {
    exec_and_out_equals("import mysql");

    exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

    // Ensures the schemas have not been loaded
    exec_and_out_equals("print(session.schemas.mysql)", "<ClassicSchema:mysql>");
    exec_and_out_equals("print(session.schemas.information_schema)", "<ClassicSchema:information_schema>");

    exec_and_out_equals("session.close()");
  }

  // Tests session.getSchema()
  TEST_F(Shell_py_mysql_session_tests, mysql_base_session_get_schema)
  {
    exec_and_out_equals("import mysql");

    exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

    // Checks schema retrieval
    exec_and_out_equals("schema = session.getSchema('mysql')");
    exec_and_out_equals("print(schema)", "<ClassicSchema:mysql>");

    exec_and_out_equals("schema = session.getSchema('information_schema')");
    exec_and_out_equals("print(schema)", "<ClassicSchema:information_schema>");

    // Checks schema retrieval with invalid schema
    exec_and_out_contains("schema = session.getSchema('unexisting_schema')", "", "Unknown database 'unexisting_schema'");

    exec_and_out_equals("session.close()");
  }

  // Tests session.<schema>
  TEST_F(Shell_py_mysql_session_tests, mysql_base_session_schema)
  {
    exec_and_out_equals("import mysql");

    exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

    // Now direct and indirect access
    exec_and_out_equals("print(session.mysql)", "<ClassicSchema:mysql>");

    exec_and_out_equals("print(session.information_schema)", "<ClassicSchema:information_schema>");

    // TODO: add test case with schema that is named as another property

    // Now direct and indirect access
    exec_and_out_contains("print(session.unexisting_schema);", "", "unknown attribute: unexisting_schema");

    exec_and_out_equals("session.close()");
  }

  // Tests session.<schema>
  TEST_F(Shell_py_mysql_session_tests, create_schema)
  {
    exec_and_out_equals("import mysql");

    exec_and_out_equals("session = mysql.getClassicSession('" + _mysql_uri + "')");

    // Cleans environment
    exec_and_out_equals("session.sql('drop database if exists mysql_test_create_schema_1')");

    // Happy path
    exec_and_out_equals("s = session.createSchema('mysql_test_create_schema_1')");

    exec_and_out_equals("print(s)", "<ClassicSchema:mysql_test_create_schema_1>");

    // Error, existing schema
    exec_and_out_contains("s2 = session.createSchema('mysql_test_create_schema_1')", "", "Can't create database 'mysql_test_create_schema_1'; database exists");

    // Error, passing non string
    exec_and_out_contains("s2 = session.createSchema(45)", "", "Argument #1 is expected to be a string");

    // Drops the database
    exec_and_out_equals("session.sql('drop database mysql_test_create_schema_1')");

    exec_and_out_equals("session.close()");
  }
}