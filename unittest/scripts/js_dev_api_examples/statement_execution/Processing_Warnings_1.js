
var mysqlx = require('mysqlx').mysqlx;

function process_warnings(result){
  if (result.getWarningCount()){
    var warnings = result.getWarnings();
    for (index in warnings){
      var warning = warnings[index];
      print ('Type ['+ warning.Level + '] (Code ' + warning.Code + '): ' + warning.Message + '\n');
    }
  }
  else{
    print ("No warnings were returned.\n");
  }
}

// Connect to server
var mySession = mysqlx.getNodeSession( {
  host: 'localhost', port: 33060,
  dbUser: 'mike', dbPassword: 's3cr3t!' } );

// Disables warning generation
mySession.setFetchWarnings(false);
var result = mySession.sql('drop schema if exists unexisting').execute();
process_warnings(result);

// Enables warning generation
mySession.setFetchWarnings(true);
var result = mySession.sql('drop schema if exists unexisting').execute();
process_warnings(result);
