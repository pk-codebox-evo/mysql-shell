/*
 * Copyright (c) 2014, 2016 Oracle and/or its affiliates. All rights reserved.
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

#include "mod_mysqlx_resultset.h"
#include "base_constants.h"
#include "mysqlx.h"
#include "shellcore/common.h"
#include <boost/bind.hpp>
#include "shellcore/shell_core_options.h"
#include "shellcore/obj_date.h"
#include "utils/utils_time.h"
#include "mysqlxtest_utils.h"

using namespace shcore;
using namespace mysh::mysqlx;

// -----------------------------------------------------------------------

BaseResult::BaseResult(boost::shared_ptr< ::mysqlx::Result> result) :
_result(result), _execution_time(0)
{
  add_method("getExecutionTime", boost::bind(&BaseResult::get_member_method, this, _1, "getExecutionTime", "executionTime"), NULL);
  add_method("getWarnings", boost::bind(&BaseResult::get_member_method, this, _1, "getWarnings", "warnings"), NULL);
  add_method("getWarningCount", boost::bind(&BaseResult::get_member_method, this, _1, "getWarningCount", "warningCount"), NULL);
}

std::vector<std::string> BaseResult::get_members() const
{
  std::vector<std::string> members(ShellBaseResult::get_members());
  members.push_back("executionTime");
  members.push_back("warningCount");
  members.push_back("warnings");
  return members;
}

bool BaseResult::has_member(const std::string &prop) const
{
  return ShellBaseResult::has_member(prop) ||
    prop == "executionTime" ||
    prop == "warningCount" ||
    prop == "warnings";
}

#ifdef DOXYGEN
/**
* The number of warnings produced by the last statement execution. See getWarnings() for more details.
* \return the number of warnings.
* This is the same value than C API mysql_warning_count, see https://dev.mysql.com/doc/refman/5.7/en/mysql-warning-count.html
* \sa warnings
*/
Integer BaseResult::getWarningCount(){};

/**
* Retrieves the warnings generated by the executed operation.
* \return A list containing a warning object for each generated warning.
* This is the same value than C API mysql_warning_count, see https://dev.mysql.com/doc/refman/5.7/en/mysql-warning-count.html
*
* Each warning object contains a key/value pair describing the information related to a specific warning.
* This information includes: Level, Code and Message.
*/
List BaseResult::getWarnings(){};

/**
* Retrieves a string value indicating the execution time of the executed operation.
*/
String BaseResult::getExecutionTime(){};

#endif
shcore::Value BaseResult::get_member(const std::string &prop) const
{
  Value ret_val;

  if (prop == "executionTime")
    return shcore::Value(get_execution_time());

  else if (prop == "warningCount")
    ret_val = Value(get_warning_count());

  else if (prop == "warnings")
  {
    boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);

    std::vector< ::mysqlx::Result::Warning> warnings = _result->getWarnings();

    if (warnings.size())
    {
      for (size_t index = 0; index < warnings.size(); index++)
      {
        mysh::Row *warning_row = new mysh::Row();

        warning_row->add_item("Level", shcore::Value(warnings[index].is_note ? "Note" : "Warning"));
        warning_row->add_item("Code", shcore::Value(warnings[index].code));
        warning_row->add_item("Message", shcore::Value(warnings[index].text));

        array->push_back(shcore::Value::wrap(warning_row));
      }
    }

    ret_val = shcore::Value(array);
  }
  else
    ret_val = ShellBaseResult::get_member(prop);

  return ret_val;
}

std::string BaseResult::get_execution_time() const
{
  return MySQL_timer::format_legacy(_execution_time, 2);
}

uint64_t BaseResult::get_warning_count() const
{
  return uint64_t(_result->getWarnings().size());
}

void BaseResult::buffer()
{
  _result->buffer();
}

bool BaseResult::rewind()
{
  return _result->rewind();
}

bool BaseResult::tell(size_t &dataset, size_t &record)
{
  return _result->tell(dataset, record);
}

bool BaseResult::seek(size_t dataset, size_t record)
{
  return _result->seek(dataset, record);
}

void BaseResult::append_json(shcore::JSON_dumper& dumper) const
{
  bool create_object = (dumper.deep_level() == 0);

  if (create_object)
    dumper.start_object();

  dumper.append_value("executionTime", get_member("executionTime"));

  if (Shell_core_options::get()->get_bool(SHCORE_SHOW_WARNINGS))
  {
    dumper.append_value("warningCount", get_member("warningCount"));
    dumper.append_value("warnings", get_member("warnings"));
  }

  if (create_object)
    dumper.end_object();
}

// -----------------------------------------------------------------------

Result::Result(boost::shared_ptr< ::mysqlx::Result> result) :
BaseResult(result)
{
  add_method("getAffectedItemCount", boost::bind(&BaseResult::get_member_method, this, _1, "getAffectedItemCount", "affectedItemCount"), NULL);
  add_method("getAutoIncrementValue", boost::bind(&BaseResult::get_member_method, this, _1, "getAutoIncrementValue", "autoIncrementValue"), NULL);
  add_method("getLastDocumentId", boost::bind(&BaseResult::get_member_method, this, _1, "getLastDocumentId", "lastDocumentId"), NULL);
  add_method("getLastDocumentIds", boost::bind(&BaseResult::get_member_method, this, _1, "getLastDocumentId", "lastDocumentIds"), NULL);
}

std::vector<std::string> Result::get_members() const
{
  std::vector<std::string> members(BaseResult::get_members());
  members.push_back("affectedItemCount");
  members.push_back("autoIncrementValue");
  members.push_back("lastDocumentId");
  members.push_back("lastDocumentIds");
  return members;
}

bool Result::has_member(const std::string &prop) const
{
  return BaseResult::has_member(prop) ||
    prop == "affectedItemCount" ||
    prop == "autoIncrementValue" ||
    prop == "lastDocumentId" ||
  prop == "lastDocumentIds";
}

#ifdef DOXYGEN
/**
* The the number of affected items for the last operation.
* \return the number of affected items.
* This is the value of the C API mysql_affected_rows(), see https://dev.mysql.com/doc/refman/5.7/en/mysql-affected-rows.html
*/
Integer Result::getAffectedItemCount(){};

/**
* The last insert id auto generated (from an insert operation)
* \return the integer representing the last insert id
* For more details, see https://dev.mysql.com/doc/refman/5.7/en/information-functions.html#function_last-insert-id
*
* Note that this value will be available only when the result is for a Table.insert operation.
*/
Integer Result::getAutoIncrementValue(){};

/**
* The id of the last document inserted into a collection.
* \return the string representing the if of the last inserted document.
*
* Note that this value will be available only when the result is for a Collection.add operation.
*/
String Result::getLastDocumentId(){};
#endif

shcore::Value Result::get_member(const std::string &prop) const
{
  Value ret_val;

  if (prop == "affectedItemCount")
    ret_val = Value(get_affected_item_count());

  else if (prop == "autoIncrementValue")
    ret_val = Value(get_auto_increment_value());

  else if (prop == "lastDocumentId")
    ret_val = Value(get_last_document_id());

  else if (prop == "lastDocumentIds")
  {
    shcore::Value::Array_type_ref ret_val(new shcore::Value::Array_type);
    std::vector<std::string> doc_ids = get_last_document_ids();

    for (auto doc_id : doc_ids)
      ret_val->push_back(shcore::Value(doc_id));

    return shcore::Value(ret_val);
  }
  else
    ret_val = BaseResult::get_member(prop);

  return ret_val;
}

int64_t Result::get_affected_item_count() const
{
  return _result->affectedRows();
}

int64_t Result::get_auto_increment_value() const
{
  return _result->lastInsertId();
}

std::string Result::get_last_document_id() const
{
  std::string ret_val;
  try
  {
    ret_val = _result->lastDocumentId();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION("Result.getLastDocumentId()");

  return ret_val;
}

const std::vector<std::string> Result::get_last_document_ids() const
{
  std::vector<std::string> ret_val;
  try
  {
    ret_val = _result->lastDocumentIds();
  }
  CATCH_AND_TRANSLATE_FUNCTION_EXCEPTION("Result.getLastDocumentIds()");
  return ret_val;
}

void Result::append_json(shcore::JSON_dumper& dumper) const
{
  dumper.start_object();

  BaseResult::append_json(dumper);

  dumper.append_value("affectedItemCount", get_member("affectedItemCount"));
  dumper.append_value("autoIncrementValue", get_member("autoIncrementValue"));
  dumper.append_value("lastDocumentId", get_member("lastDocumentId"));

  dumper.end_object();
}

// -----------------------------------------------------------------------
DocResult::DocResult(boost::shared_ptr< ::mysqlx::Result> result) :
BaseResult(result)
{
  add_method("fetchOne", boost::bind(&DocResult::fetch_one, this, _1), "nothing", shcore::String, NULL);
  add_method("fetchAll", boost::bind(&DocResult::fetch_all, this, _1), "nothing", shcore::String, NULL);
}

#ifdef DOXYGEN
/**
* Retrieves the next DbDoc on the DocResult.
* \return A DbDoc object representing the next Document in the result.
*/
Row DocResult::fetchOne(){};
#endif
shcore::Value DocResult::fetch_one(const shcore::Argument_list &args) const
{
  Value ret_val = Value::Null();

  args.ensure_count(0, "DocResult.fetchOne");

  if (_result->columnMetadata() && _result->columnMetadata()->size())
  {
    boost::shared_ptr< ::mysqlx::Row> r(_result->next());
    if (r.get())
      return Value::parse(r->stringField(0));
  }
  return shcore::Value();
}

#ifdef DOXYGEN
/**
* Returns a list of DbDoc objects which contains an element for every unread document.
* \return A List of DbDoc objects.
*
* If this function is called right after executing a query, it will return a DbDoc for every document on the resultset.
*
* If fetchOne is called before this function, when this function is called it will return a DbDoc for each of the remaining documents on the resultset.
*/
List DocResult::fetchAll(){};
#endif
shcore::Value DocResult::fetch_all(const shcore::Argument_list &args) const
{
  Value::Array_type_ref array(new Value::Array_type());

  args.ensure_count(0, "DocResult.fetchAll");

  // Gets the next document
  Value record = fetch_one(args);
  while (record)
  {
    array->push_back(record);
    record = fetch_one(args);
  }

  return Value(array);
}

shcore::Value DocResult::get_metadata() const
{
  if (!_metadata)
  {
    shcore::Value data_type = mysh::Constant::get_constant("mysqlx", "Type", "Json", shcore::Argument_list());

    // the plugin may not send these if they are equal to table/name respectively
    // We need to reconstruct them
    std::string orig_table = _result->columnMetadata()->at(0).original_table;
    std::string orig_name = _result->columnMetadata()->at(0).original_name;

    if (orig_table.empty())
      orig_table = _result->columnMetadata()->at(0).table;

    if (orig_name.empty())
      orig_name = _result->columnMetadata()->at(0).name;

    boost::shared_ptr<mysh::Column> metadata(new mysh::Column(
      _result->columnMetadata()->at(0).schema,
      orig_table,
      _result->columnMetadata()->at(0).table,
      orig_name,
      _result->columnMetadata()->at(0).name,
      data_type,
      _result->columnMetadata()->at(0).length,
      false, // IS NUMERIC
      _result->columnMetadata()->at(0).fractional_digits,
      false, // IS SIGNED
      Charset::item[_result->columnMetadata()->at(0).collation].collation,
      Charset::item[_result->columnMetadata()->at(0).collation].name,
      true)); // IS PADDED

    _metadata = shcore::Value(boost::static_pointer_cast<Object_bridge>(metadata));
  }

  return _metadata;
}

void DocResult::append_json(shcore::JSON_dumper& dumper) const
{
  dumper.start_object();

  dumper.append_value("documents", fetch_all(shcore::Argument_list()));

  BaseResult::append_json(dumper);

  dumper.end_object();
}

// -----------------------------------------------------------------------
RowResult::RowResult(boost::shared_ptr< ::mysqlx::Result> result) :
BaseResult(result)
{
  add_method("fetchOne", boost::bind(&RowResult::fetch_one, this, _1), "nothing", shcore::String, NULL);
  add_method("fetchAll", boost::bind(&RowResult::fetch_all, this, _1), "nothing", shcore::String, NULL);

  add_method("getColumnCount", boost::bind(&BaseResult::get_member_method, this, _1, "getColumnCount", "columnCount"), NULL);
  add_method("getColumns", boost::bind(&BaseResult::get_member_method, this, _1, "getColumns", "columns"), NULL);
  add_method("getColumnNames", boost::bind(&BaseResult::get_member_method, this, _1, "getColumnNames", "columnNames"), NULL);
}

std::vector<std::string> RowResult::get_members() const
{
  std::vector<std::string> members(BaseResult::get_members());
  members.push_back("columnCount");
  members.push_back("columns");
  members.push_back("columnNames");
  return members;
}

bool RowResult::has_member(const std::string &prop) const
{
  return BaseResult::has_member(prop) ||
    prop == "columnCount" ||
    prop == "columns" ||
    prop == "columnNames";
}

#ifdef DOXYGEN
/**
* Retrieves the number of columns on the current result.
* \return the number of columns on the current result.
*/
Integer RowResult::getColumnCount(){};

/**
* Gets the columns on the current result.
* \return A list with the names of the columns returned on the active result.
*/
List RowResult::getColumnNames(){};

/**
* Gets the column metadata for the columns on the active result.
* \return a list of Column objects containing information about the columns included on the active result.
*/
List RowResult::getColumns(){};
#endif
shcore::Value RowResult::get_member(const std::string &prop) const
{
  Value ret_val;
  if (prop == "columnCount")
    ret_val = shcore::Value(get_column_count());
  else if (prop == "columnNames")
  {
    boost::shared_ptr<shcore::Value::Array_type> array(new shcore::Value::Array_type);

    if (_result->columnMetadata())
    {
      size_t num_fields = _result->columnMetadata()->size();

      for (size_t i = 0; i < num_fields; i++)
        array->push_back(shcore::Value(_result->columnMetadata()->at(i).name));
    }

    ret_val = shcore::Value(array);
  }
  else if (prop == "columns")
    ret_val = shcore::Value(get_columns());
  else
    ret_val = BaseResult::get_member(prop);

  return ret_val;
}

int64_t RowResult::get_column_count() const
{
  size_t count = 0;
  if (_result->columnMetadata())
    count = _result->columnMetadata()->size();

  return uint64_t(count);
}

std::vector<std::string> RowResult::get_column_names() const
{
  std::vector<std::string> ret_val;

  if (_result->columnMetadata())
  {
    size_t num_fields = _result->columnMetadata()->size();

    for (size_t i = 0; i < num_fields; i++)
      ret_val.push_back(_result->columnMetadata()->at(i).name);
  }

  return ret_val;
}

shcore::Value::Array_type_ref RowResult::get_columns() const
{
  if (!_columns)
  {
    _columns.reset(new shcore::Value::Array_type);

    size_t num_fields = _result->columnMetadata()->size();
    for (size_t i = 0; i < num_fields; i++)
    {
      ::mysqlx::FieldType type = _result->columnMetadata()->at(i).type;
      bool is_numeric = type == ::mysqlx::SINT ||
        type == ::mysqlx::UINT ||
        type == ::mysqlx::DOUBLE ||
        type == ::mysqlx::FLOAT ||
        type == ::mysqlx::DECIMAL;

      std::string type_name;
      bool is_signed = false;
      bool is_padded = true;
      switch (_result->columnMetadata()->at(i).type)
      {
        case ::mysqlx::SINT:
          is_signed = true;
        case ::mysqlx::UINT:
          switch (_result->columnMetadata()->at(i).length)
          {
            case 3:
            case 4:
              type_name = "TinyInt";
              break;
            case 5:
            case 6:
              type_name = "SmallInt";
              break;
            case 8:
            case 9:
              type_name = "MediumInt";
              break;
            case 10:
            case 11:
              type_name = "Int";
              break;
            case 20:
              type_name = "BigInt";
              break;
          }
          break;
        case ::mysqlx::BIT:
          type_name = "Bit";
          break;
        case ::mysqlx::DOUBLE:
          type_name = "Double";
          is_signed = !(_result->columnMetadata()->at(i).flags & 0x001);
          break;
        case ::mysqlx::FLOAT:
          type_name = "Float";
          is_signed = !(_result->columnMetadata()->at(i).flags & 0x001);
          break;
        case ::mysqlx::DECIMAL:
          type_name = "Decimal";
          is_signed = !(_result->columnMetadata()->at(i).flags & 0x001);
          break;
        case ::mysqlx::BYTES:
          is_padded = is_signed = _result->columnMetadata()->at(i).flags & 0x001;

          switch (_result->columnMetadata()->at(i).content_type & 0x0003)
          {
            case 1:
              type_name = "Geometry";
              break;
            case 2:
              type_name = "Json";
              break;
            case 3:
              type_name = "Xml";
              break;
            default:
              if (Charset::item[_result->columnMetadata()->at(i).collation].collation == "Binary")
                type_name = "Bytes";
              else
                type_name = "String";
              break;
          }
          break;
        case ::mysqlx::TIME:
          type_name = "Time";
          break;
        case ::mysqlx::DATETIME:
          if (_result->columnMetadata()->at(i).flags & 0x001)
            type_name = "Timestamp";
          else if (_result->columnMetadata()->at(i).length == 10)
            type_name = "Date";
          else
            type_name = "DateTime";
          break;
        case ::mysqlx::SET:
          type_name = "Set";
          break;
        case ::mysqlx::ENUM:
          type_name = "Enum";
          break;
      }

      shcore::Value data_type = mysh::Constant::get_constant("mysqlx", "Type", type_name, shcore::Argument_list());

      // the plugin may not send these if they are equal to table/name respectively
      // We need to reconstruct them
      std::string orig_table = _result->columnMetadata()->at(i).original_table;
      std::string orig_name = _result->columnMetadata()->at(i).original_name;

      if (orig_table.empty())
        orig_table = _result->columnMetadata()->at(i).table;

      if (orig_name.empty())
        orig_name = _result->columnMetadata()->at(i).name;

      boost::shared_ptr<mysh::Column> column(new mysh::Column(
        _result->columnMetadata()->at(i).schema,
        orig_table,
        _result->columnMetadata()->at(i).table,
        orig_name,
        _result->columnMetadata()->at(i).name,
        data_type,
        _result->columnMetadata()->at(i).length,
  is_numeric,
        _result->columnMetadata()->at(i).fractional_digits,
        is_signed,
  Charset::item[_result->columnMetadata()->at(i).collation].collation,
  Charset::item[_result->columnMetadata()->at(i).collation].name,
        is_padded));

      _columns->push_back(shcore::Value(boost::static_pointer_cast<Object_bridge>(column)));
    }
  }

  return _columns;
}

#ifdef DOXYGEN
/**
* Retrieves the next Row on the RowResult.
* \return A Row object representing the next record on the result.
*/
Row RowResult::fetchOne(){};
#endif
shcore::Value RowResult::fetch_one(const shcore::Argument_list &args) const
{
  std::string function = class_name() + ".next";

  args.ensure_count(0, function.c_str());

  boost::shared_ptr<std::vector< ::mysqlx::ColumnMetadata> > metadata = _result->columnMetadata();
  if (metadata->size() > 0)
  {
    boost::shared_ptr< ::mysqlx::Row>row = _result->next();
    if (row)
    {
      mysh::Row *value_row = new mysh::Row();

      for (int index = 0; index < int(metadata->size()); index++)
      {
        Value field_value;

        if (row->isNullField(index))
          field_value = Value::Null();
        else
        {
          switch (metadata->at(index).type)
          {
            case ::mysqlx::SINT:
              field_value = Value(row->sInt64Field(index));
              break;
            case ::mysqlx::UINT:
              field_value = Value(row->uInt64Field(index));
              break;
            case ::mysqlx::DOUBLE:
              field_value = Value(row->doubleField(index));
              break;
            case ::mysqlx::FLOAT:
              field_value = Value(row->floatField(index));
              break;
            case ::mysqlx::BYTES:
              field_value = Value(row->stringField(index));
              break;
            case ::mysqlx::DECIMAL:
              field_value = Value(row->decimalField(index));
              break;
            case ::mysqlx::TIME:
              field_value = Value(row->timeField(index));
              break;
            case ::mysqlx::DATETIME:
            {
              ::mysqlx::DateTime date = row->dateTimeField(index);
              boost::shared_ptr<shcore::Date> shell_date(new shcore::Date(date.year(), date.month(), date.day(), date.hour(), date.minutes(), date.seconds()));
              field_value = Value(boost::static_pointer_cast<Object_bridge>(shell_date));
              break;
            }
            case ::mysqlx::ENUM:
              field_value = Value(row->enumField(index));
              break;
            case ::mysqlx::BIT:
              field_value = Value(row->bitField(index));
              break;
              //TODO: Fix the handling of SET
            case ::mysqlx::SET:
              //field_value = Value(row->setField(int(index)));
              break;
          }
        }
        value_row->add_item(metadata->at(index).name, field_value);
      }

      return shcore::Value::wrap(value_row);
    }
  }
  return shcore::Value();
}

#ifdef DOXYGEN
/**
* Returns a list of DbDoc objects which contains an element for every unread document.
* \return A List of DbDoc objects.
*/
List RowResult::fetchAll(){};
#endif
shcore::Value RowResult::fetch_all(const shcore::Argument_list &args) const
{
  Value::Array_type_ref array(new Value::Array_type());

  args.ensure_count(0, "RowResult.fetchAll");

  // Gets the next row
  Value record = fetch_one(args);
  while (record)
  {
    array->push_back(record);
    record = fetch_one(args);
  }

  return Value(array);
}

void RowResult::append_json(shcore::JSON_dumper& dumper) const
{
  bool create_object = (dumper.deep_level() == 0);

  if (create_object)
    dumper.start_object();

  BaseResult::append_json(dumper);

  dumper.append_value("rows", fetch_all(shcore::Argument_list()));

  if (create_object)
    dumper.end_object();
}

SqlResult::SqlResult(boost::shared_ptr< ::mysqlx::Result> result) :
RowResult(result)
{
  add_method("hasData", boost::bind(&SqlResult::has_data, this, _1), "nothing", shcore::String, NULL);
  add_method("nextDataSet", boost::bind(&SqlResult::next_data_set, this, _1), "nothing", shcore::String, NULL);
  add_method("getAffectedRowCount", boost::bind(&BaseResult::get_member_method, this, _1, "getAffectedRowCount", "affectedRowCount"), NULL);
  add_method("getAutoIncrementValue", boost::bind(&BaseResult::get_member_method, this, _1, "getAutoIncrementValue", "autoIncrementValue"), NULL);
}

#ifdef DOXYGEN
/**
* Returns true if the last statement execution has a result set.
*
*/
Bool SqlResult::hasData(){}
#endif
shcore::Value SqlResult::has_data(const shcore::Argument_list &args) const
{
  args.ensure_count(0, "SqlResult.hasData");

  return Value(_result->has_data());
}

std::vector<std::string> SqlResult::get_members() const
{
  std::vector<std::string> members(RowResult::get_members());
  members.push_back("autoIncrementValue");
  members.push_back("affectedRowCount");
  return members;
}

bool SqlResult::has_member(const std::string &prop) const
{
  return RowResult::has_member(prop) ||
    prop == "autoIncrementValue" ||
    prop == "affectedRowCount";
}

#ifdef DOXYGEN
/**
* Returns the identifier for the last record inserted.
*
* Note that this value will only be set if the executed statement inserted a record in the database and an ID was automatically generated.
*/
Integer SqlResult::getAutoIncrementValue(){};

/**
* Returns the number of rows affected by the executed query.
*/
Integer SqlResult::getAffectedRowCount(){};
#endif

shcore::Value SqlResult::get_member(const std::string &prop) const
{
  Value ret_val;
  if (prop == "autoIncrementValue")
    ret_val = Value(get_auto_increment_value());
  else if (prop == "affectedRowCount")
    ret_val = Value(get_affected_row_count());
  else
    ret_val = RowResult::get_member(prop);

  return ret_val;
}

int64_t SqlResult::get_affected_row_count() const
{
  return _result->affectedRows();
}

int64_t SqlResult::get_auto_increment_value() const
{
  return _result->lastInsertId();
}

#ifdef DOXYGEN
/**
* Prepares the SqlResult to start reading data from the next Result (if many results were returned).
* \return A boolean value indicating whether there is another result or not.
*/
Bool SqlResult::nextDataSet(){};
#endif
shcore::Value SqlResult::next_data_set(const shcore::Argument_list &args)
{
  args.ensure_count(0, "SqlResult.nextDataSet");

  return shcore::Value(_result->nextDataSet());
}

void SqlResult::append_json(shcore::JSON_dumper& dumper) const
{
  dumper.start_object();

  RowResult::append_json(dumper);

  dumper.append_value("hasData", has_data(shcore::Argument_list()));
  dumper.append_value("affectedRowCount", get_member("affectedRowCount"));
  dumper.append_value("autoIncrementValue", get_member("autoIncrementValue"));

  dumper.end_object();
}