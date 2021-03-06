/* Copyright (c) 2016, Oracle and/or its affiliates. All rights reserved.

 This program is free software; you can redistribute it and/or modify
 it under the terms of the GNU General Public License as published by
 the Free Software Foundation; version 2 of the License.

 This program is distributed in the hope that it will be useful,
 but WITHOUT ANY WARRANTY; without even the implied warranty of
 MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 GNU General Public License for more details.

 You should have received a copy of the GNU General Public License
 along with this program; if not, write to the Free Software
 Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA */

#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <string>
#include <boost/shared_ptr.hpp>
#include <boost/pointer_cast.hpp>

#include "gtest/gtest.h"
#include "shellcore/types.h"
#include "shellcore/lang_base.h"
#include "shellcore/types_cpp.h"
#include "shellcore/common.h"

#include "shellcore/shell_core.h"
#include "shellcore/shell_sql.h"
#include "shellcore/shell_notifications.h"
#include "../modules/base_session.h"
#include "../modules/base_resultset.h"
#include "../src/shell_resultset_dumper.h"
#include "test_utils.h"
#include "utils/utils_file.h"
#include <boost/bind.hpp>
#include <queue>

namespace shcore
{
  namespace shell_core_tests
  {
    struct Notification
    {
      std::string name;
      shcore::Object_bridge_ref sender;
      shcore::Value::Map_type_ref data;
    };

    class Shell_notifications_test : public Shell_core_test_wrapper, public shcore::NotificationObserver
    {
    protected:
      std::queue<Notification> _notifications;

      virtual void SetUp()
      {
        Shell_core_test_wrapper::SetUp();

        // Initializes the JS mode to point to the right module paths
        std::string js_modules_path = MYSQLX_SOURCE_HOME;
        js_modules_path += "/scripting/modules/js";
        _interactive_shell->process_line("shell.js.module_paths[shell.js.module_paths.length] = '" + js_modules_path + "'; ");
      }

      virtual void handle_notification(const std::string &name, shcore::Object_bridge_ref sender, shcore::Value::Map_type_ref data)
      {
        _notifications.push({ name, sender, data });
      }
    };

    TEST_F(Shell_notifications_test, test_sn_session_connected_global_commands)
    {
      Notification n;

      this->observe_notification("SN_SESSION_CONNECTED");

      _interactive_shell->process_line("\\connect " + _uri);

      ASSERT_EQ(1, _notifications.size());
      n = _notifications.front();
      _notifications.pop();
      ASSERT_EQ("SN_SESSION_CONNECTED", n.name);
      ASSERT_EQ("XSession", n.sender->class_name());

      _interactive_shell->process_line("session.close()");

      _interactive_shell->process_line("\\connect -c " + _mysql_uri);

      ASSERT_EQ(1, _notifications.size());
      n = _notifications.front();
      _notifications.pop();
      ASSERT_EQ("SN_SESSION_CONNECTED", n.name);
      ASSERT_EQ("ClassicSession", n.sender->class_name());

      _interactive_shell->process_line("session.close()");

      _interactive_shell->process_line("\\connect -n " + _uri);

      ASSERT_EQ(1, _notifications.size());
      n = _notifications.front();
      _notifications.pop();
      ASSERT_EQ("SN_SESSION_CONNECTED", n.name);
      ASSERT_EQ("NodeSession", n.sender->class_name());

      this->ignore_notification("SN_SESSION_CONNECTED");

      _interactive_shell->process_line("session.close()");

      _interactive_shell->process_line("\\connect " + _uri);

      ASSERT_EQ(0, _notifications.size());

      _interactive_shell->process_line("session.close()");

      _interactive_shell->process_line("\\connect -c " + _mysql_uri);

      ASSERT_EQ(0, _notifications.size());

      _interactive_shell->process_line("session.close()");

      _interactive_shell->process_line("\\connect -n " + _uri);

      ASSERT_EQ(0, _notifications.size());

      _interactive_shell->process_line("session.close()");
    }

    TEST_F(Shell_notifications_test, test_sn_session_connected_javascript)
    {
      Notification n;

      this->observe_notification("SN_SESSION_CONNECTED");

      _interactive_shell->process_line("var mysqlx = require('mysqlx').mysqlx;");
      _interactive_shell->process_line("var mysql = require('mysql').mysql;");
      _interactive_shell->process_line("var session = mysqlx.getSession('" + _uri + "');");

      ASSERT_EQ(1, _notifications.size());
      n = _notifications.front();
      _notifications.pop();
      ASSERT_EQ("SN_SESSION_CONNECTED", n.name);
      ASSERT_EQ("XSession", n.sender->class_name());

      _interactive_shell->process_line("session.close()");

      _interactive_shell->process_line("var session = mysql.getClassicSession('" + _mysql_uri + "');");

      ASSERT_EQ(1, _notifications.size());
      n = _notifications.front();
      _notifications.pop();
      ASSERT_EQ("SN_SESSION_CONNECTED", n.name);
      ASSERT_EQ("ClassicSession", n.sender->class_name());

      _interactive_shell->process_line("session.close()");

      _interactive_shell->process_line("var session = mysqlx.getNodeSession('" + _uri + "');");

      ASSERT_EQ(1, _notifications.size());
      n = _notifications.front();
      _notifications.pop();
      ASSERT_EQ("SN_SESSION_CONNECTED", n.name);
      ASSERT_EQ("NodeSession", n.sender->class_name());

      _interactive_shell->process_line("session.close()");

      this->ignore_notification("SN_SESSION_CONNECTED");

      _interactive_shell->process_line("var session = mysqlx.getSession('" + _uri + "');");

      ASSERT_EQ(0, _notifications.size());

      _interactive_shell->process_line("session.close()");

      _interactive_shell->process_line("var session = mysql.getClassicSession('" + _mysql_uri + "');");

      ASSERT_EQ(0, _notifications.size());

      _interactive_shell->process_line("session.close()");

      _interactive_shell->process_line("var session = mysqlx.getNodeSession('" + _uri + "');");

      ASSERT_EQ(0, _notifications.size());

      _interactive_shell->process_line("session.close()");
    }

    TEST_F(Shell_notifications_test, test_sn_session_connected_python)
    {
      Notification n;

      this->observe_notification("SN_SESSION_CONNECTED");

      _interactive_shell->process_line("\\py");
      _interactive_shell->process_line("import mysqlx");
      _interactive_shell->process_line("import mysql");

      _interactive_shell->process_line("session = mysqlx.getSession('" + _uri + "');");

      ASSERT_EQ(1, _notifications.size());
      n = _notifications.front();
      _notifications.pop();
      ASSERT_EQ("SN_SESSION_CONNECTED", n.name);
      ASSERT_EQ("XSession", n.sender->class_name());

      _interactive_shell->process_line("session.close()");

      _interactive_shell->process_line("session = mysql.getClassicSession('" + _mysql_uri + "');");

      ASSERT_EQ(1, _notifications.size());
      n = _notifications.front();
      _notifications.pop();
      ASSERT_EQ("SN_SESSION_CONNECTED", n.name);
      ASSERT_EQ("ClassicSession", n.sender->class_name());

      _interactive_shell->process_line("session.close()");

      _interactive_shell->process_line("session = mysqlx.getNodeSession('" + _uri + "');");

      ASSERT_EQ(1, _notifications.size());
      n = _notifications.front();
      _notifications.pop();
      ASSERT_EQ("SN_SESSION_CONNECTED", n.name);
      ASSERT_EQ("NodeSession", n.sender->class_name());

      _interactive_shell->process_line("session.close()");

      this->ignore_notification("SN_SESSION_CONNECTED");

      _interactive_shell->process_line("session = mysqlx.getSession('" + _uri + "');");

      ASSERT_EQ(0, _notifications.size());

      _interactive_shell->process_line("session.close()");

      _interactive_shell->process_line("session = mysql.getClassicSession('" + _mysql_uri + "');");

      ASSERT_EQ(0, _notifications.size());

      _interactive_shell->process_line("session.close()");

      _interactive_shell->process_line("session = mysqlx.getNodeSession('" + _uri + "');");

      ASSERT_EQ(0, _notifications.size());

      _interactive_shell->process_line("session.close()");
    }
  }
}