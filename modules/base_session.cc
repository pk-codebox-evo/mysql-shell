/*
 * Copyright (c) 2015, 2016 Oracle and/or its affiliates. All rights reserved.
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

#include "base_session.h"

#include "shellcore/object_factory.h"
#include "shellcore/shell_core.h"
#include "shellcore/lang_base.h"
#include "shellcore/common.h"
#include "shellcore/server_registry.h"
#include "shellcore/shell_notifications.h"

#include "shellcore/proxy_object.h"

#include "utils/utils_general.h"
#include "utils/utils_file.h"
#include "mysqlxtest_utils.h"

#include <boost/bind.hpp>
#include <boost/lexical_cast.hpp>
#include <boost/format.hpp>
#include <boost/pointer_cast.hpp>

#ifdef HAVE_LIBMYSQLCLIENT
#include "mod_mysql_session.h"
#endif
#include "mod_mysqlx_session.h"

#define MAX_COLUMN_LENGTH 1024
#define MIN_COLUMN_LENGTH 4

using namespace mysh;
using namespace shcore;

boost::shared_ptr<mysh::ShellDevelopmentSession> mysh::connect_session(const shcore::Argument_list &args, SessionType session_type)
{
  boost::shared_ptr<ShellDevelopmentSession> ret_val;

  switch (session_type)
  {
    case Application:
      ret_val.reset(new mysh::mysqlx::XSession());
      break;
    case Node:
      ret_val.reset(new mysh::mysqlx::NodeSession());
      break;
#ifdef HAVE_LIBMYSQLCLIENT
    case Classic:
      ret_val.reset(new mysql::ClassicSession());
      break;
#endif
    default:
      throw shcore::Exception::argument_error("Invalid session type specified for MySQL connection.");
      break;
  }

  ret_val->connect(args);

  ShellNotifications::get()->notify("SN_SESSION_CONNECTED", ret_val);

  return ret_val;
}

ShellBaseSession::ShellBaseSession() :
_port(0)
{
  init();
}

ShellBaseSession::ShellBaseSession(const ShellBaseSession& s) :
_user(s._user), _password(s._password), _host(s._host), _port(s._port), _sock(s._sock), _schema(s._schema),
_ssl_ca(s._ssl_ca), _ssl_cert(s._ssl_cert), _ssl_key(s._ssl_key)
{
  init();
}

void ShellBaseSession::init()
{
  add_method("getUri", boost::bind(&ShellBaseSession::get_member_method, this, _1, "getUri", "uri"), NULL);
  add_method("isOpen", boost::bind(&ShellBaseSession::is_open, this, _1), NULL);
}

std::string &ShellBaseSession::append_descr(std::string &s_out, int UNUSED(indent), int UNUSED(quote_strings)) const
{
  if (!is_connected())
    s_out.append("<" + class_name() + ":disconnected>");
  else
    s_out.append("<" + class_name() + ":" + _uri + ">");
  return s_out;
}

std::string &ShellBaseSession::append_repr(std::string &s_out) const
{
  return append_descr(s_out, false);
}

void ShellBaseSession::append_json(shcore::JSON_dumper& dumper) const
{
  dumper.start_object();

  dumper.append_string("class", class_name());
  dumper.append_bool("connected", is_connected());

  if (is_connected())
    dumper.append_string("uri", _uri);

  dumper.end_object();
}

std::vector<std::string> ShellBaseSession::get_members() const
{
  std::vector<std::string> members(Cpp_object_bridge::get_members());
  members.push_back("uri");
  return members;
}

shcore::Value ShellBaseSession::get_member(const std::string &prop) const
{
  shcore::Value ret_val;

  if (Cpp_object_bridge::has_member(prop))
    ret_val = Cpp_object_bridge::get_member(prop);
  else if (prop == "uri")
    ret_val = shcore::Value(_uri);

  return ret_val;
}

bool ShellBaseSession::has_member(const std::string &prop) const
{
  return Cpp_object_bridge::has_member(prop) ||
    prop == "uri";
}

shcore::Value ShellBaseSession::get_member_method(const shcore::Argument_list &args, const std::string& method, const std::string& prop)
{
  std::string function = class_name() + "." + method;
  args.ensure_count(0, function.c_str());

  return get_member(prop);
}

void ShellBaseSession::load_connection_data(const shcore::Argument_list &args)
{
  // The connection data can come from different sources
  std::string uri;
  std::string app;
  std::string auth_method;
  std::string connections_file; // The default connection file or the indicated on the map as dataSourceFile
  shcore::Value::Map_type_ref options; // Map with the connection data

  //-----------------------------------------------------
  // STEP 1: Identifies the source of the connection data
  //-----------------------------------------------------
  if (args[0].type == String)
  {
    std::string temp = args.string_at(0);

    // The connection data to be loaded from the stored sessions
    if (temp[0] == '$')
      app = temp.substr(1);

    // The connection data comes in an URI
    else
      uri = temp;
  }

  // Connection data comes in a dictionary
  else if (args[0].type == Map)
  {
    options = args.map_at(0);

    // Connection data should be loaded from a stored session
    if (options->has_key("app"))
      app = (*options)["app"].as_string();

    // Use a custom stored sessions file, rather than the default one
    if (options->has_key("dataSourceFile"))
      connections_file = (*options)["dataSourceFile"].as_string();
  }
  else
    throw shcore::Exception::argument_error("Unexpected argument on connection data.");

  //-------------------------------------------------------------------------
  // STEP 2: Gets the individual connection parameters whatever the source is
  //-------------------------------------------------------------------------
  // Handles the case where an URI was received
  if (!uri.empty())
  {
    std::string protocol;
    int pwd_found;
    parse_mysql_connstring(uri, protocol, _user, _password, _host, _port, _sock, _schema, pwd_found, _ssl_ca, _ssl_cert, _ssl_key);
  }
  else if (!app.empty())
  {
    // If no custom connection file is indicated, then uses the default one
    if (connections_file.empty())
      connections_file = shcore::get_default_config_path();

    // Loads the connection data from a stored session
    shcore::Server_registry sr(connections_file);
    try { sr.load(); }
    CATCH_AND_TRANSLATE();

    shcore::Connection_options& conn = sr.get_connection_options(app);

    _user = conn.get_user();
    _host = conn.get_server();
    std::string str_port = conn.get_port();
    if (!str_port.empty())
    {
      int tmp_port = boost::lexical_cast<int>(str_port);
      if (tmp_port)
        _port = tmp_port;
    }
    _schema = conn.get_schema();

    _ssl_ca = conn.get_value_if_exists("ssl_ca");
    _ssl_cert = conn.get_value_if_exists("ssl_cert");
    _ssl_key = conn.get_value_if_exists("ssl_key");
  }

  // If the connection data came in a dictionary, the values in the dictionary override whatever
  // is already loaded: i.e. if the dictionary indicated a stored session, that info is already
  // loaded but will be overriden with whatever extra values exist on the dictionary
  if (options)
  {
    if (options->has_key("host"))
      _host = (*options)["host"].as_string();

    if (options->has_key("port"))
      _port = (*options)["port"].as_int();

    if (options->has_key("socket"))
      _sock = (*options)["socket"].as_string();

    if (options->has_key("schema"))
      _schema = (*options)["schema"].as_string();

    if (options->has_key("dbUser"))
      _user = (*options)["dbUser"].as_string();

    if (options->has_key("dbPassword"))
      _password = (*options)["dbPassword"].as_string();

    if (options->has_key("ssl_ca"))
      _ssl_ca = (*options)["ssl_ca"].as_string();

    if (options->has_key("ssl_cert"))
      _ssl_cert = (*options)["ssl_cert"].as_string();

    if (options->has_key("ssl_key"))
      _ssl_key = (*options)["ssl_key"].as_string();

    if (options->has_key("authMethod"))
      _auth_method = (*options)["authMethod"].as_string();
  }

  // If password is received as parameter, then it overwrites
  // Anything found on any of the indicated sources: URI, options map and stored session
  if (2 == args.size())
    _password = args.string_at(1).c_str();

  if (_port == 0 && _sock.empty())
    _port = get_default_port();

  std::string sock_port = (_port == 0) ? _sock : boost::lexical_cast<std::string>(_port);

  if (_schema.empty())
    _uri = (boost::format("%1%@%2%:%3%") % _user % _host % sock_port).str();
  else
    _uri = (boost::format("%1%@%2%:%3%/%4%") % _user % _host % sock_port % _schema).str();
}

bool ShellBaseSession::operator == (const Object_bridge &other) const
{
  return class_name() == other.class_name() && this == &other;
}

std::string ShellBaseSession::get_quoted_name(const std::string& name)
{
  size_t index = 0;
  std::string quoted_name(name);

  while ((index = quoted_name.find("`", index)) != std::string::npos)
  {
    quoted_name.replace(index, 1, "``");
    index += 2;
  }

  quoted_name = "`" + quoted_name + "`";

  return quoted_name;
}

shcore::Value ShellBaseSession::is_open(const shcore::Argument_list &args)
{
  std::string function = class_name() + ".isOpen";
  args.ensure_count(0, function.c_str());
  
  return shcore::Value(is_connected());
}

ShellDevelopmentSession::ShellDevelopmentSession() :
ShellBaseSession()
{
  init();
}

ShellDevelopmentSession::ShellDevelopmentSession(const ShellDevelopmentSession& s) :
ShellBaseSession(s)
{
  init();
}

std::vector<std::string> ShellDevelopmentSession::get_members() const
{
  std::vector<std::string> members(ShellBaseSession::get_members());
  members.push_back("defaultSchema");
  return members;
}

shcore::Value ShellDevelopmentSession::get_member(const std::string &prop) const
{
  shcore::Value ret_val;

  if (ShellBaseSession::has_member(prop))
    ret_val = ShellBaseSession::get_member(prop);
  else if (prop == "defaultSchema")
  {
    if (!_default_schema.empty())
    {
      shcore::Argument_list args;
      args.push_back(shcore::Value(_default_schema));
      ret_val = get_schema(args);
    }
    else
      ret_val = Value::Null();
  }
  else
    throw Exception::attrib_error("Invalid object member " + prop);

  return ret_val;
}

bool ShellDevelopmentSession::has_member(const std::string &prop) const
{
  return ShellBaseSession::has_member(prop) ||
    prop == "defaultSchema";
}

void ShellDevelopmentSession::init()
{
  add_method("createSchema", boost::bind(&ShellDevelopmentSession::create_schema, this, _1), "name", shcore::String, NULL);
  add_method("getDefaultSchema", boost::bind(&ShellDevelopmentSession::get_member_method, this, _1, "getDefaultSchema", "defaultSchema"), NULL);
  add_method("getSchema", boost::bind(&ShellDevelopmentSession::get_schema, this, _1), "name", shcore::String, NULL);
  add_method("getSchemas", boost::bind(&ShellDevelopmentSession::get_schemas, this, _1), NULL);
}