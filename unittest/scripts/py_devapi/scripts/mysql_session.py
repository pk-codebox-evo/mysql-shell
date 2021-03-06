# Assumptions: ensure_schema_does_not_exist is available
# Assumes __uripwd is defined as <user>:<pwd>@<host>:<mysql_port>
# validateMemer and validateNotMember are defined on the setup script
import mysql

#@ Session: validating members
classicSession = mysql.getClassicSession(__uripwd)
all_members = dir(classicSession)

# Remove the python built in members
sessionMembers = []
for member in all_members:
  if not member.startswith('__'):
    sessionMembers.append(member)


validateMember(sessionMembers, 'close')
validateMember(sessionMembers, 'createSchema')
validateMember(sessionMembers, 'getCurrentSchema')
validateMember(sessionMembers, 'getDefaultSchema')
validateMember(sessionMembers, 'getSchema')
validateMember(sessionMembers, 'getSchemas')
validateMember(sessionMembers, 'getUri')
validateMember(sessionMembers, 'setCurrentSchema')
validateMember(sessionMembers, 'runSql')
validateMember(sessionMembers, 'defaultSchema')
validateMember(sessionMembers, 'uri')
validateMember(sessionMembers, 'currentSchema')

#@ ClassicSession: validate dynamic members for system schemas
sessionMembers = dir(classicSession)
validateNotMember(sessionMembers, 'mysql')
validateNotMember(sessionMembers, 'information_schema')

#@ ClassicSession: accessing Schemas
schemas = classicSession.getSchemas()
print getSchemaFromList(schemas, 'mysql')
print getSchemaFromList(schemas, 'information_schema')

#@ ClassicSession: accessing individual schema
schema = classicSession.getSchema('mysql')
print schema.name
schema = classicSession.getSchema('information_schema')
print schema.name

#@ ClassicSession: accessing unexisting schema
schema = classicSession.getSchema('unexisting_schema')

#@ ClassicSession: current schema validations: nodefault
dschema = classicSession.getDefaultSchema()
cschema = classicSession.getCurrentSchema()
print dschema
print cschema

#@ ClassicSession: create schema success
ensure_schema_does_not_exist(classicSession, 'node_session_schema')

ss = classicSession.createSchema('node_session_schema')
print ss

#@ ClassicSession: create schema failure
sf = classicSession.createSchema('node_session_schema')

#@ Session: create quoted schema
ensure_schema_does_not_exist(classicSession, 'quoted schema');
qs = classicSession.createSchema('quoted schema');
print(qs);

#@ Session: validate dynamic members for created schemas
sessionMembers = dir(classicSession)
validateNotMember(sessionMembers, 'node_session_schema');
validateNotMember(sessionMembers, 'quoted schema');

#@ ClassicSession: Transaction handling: rollback
classicSession.setCurrentSchema('node_session_schema')

result = classicSession.runSql('create table sample (name varchar(50))')
classicSession.startTransaction()
res1 = classicSession.runSql('insert into sample values ("john")')
res2 = classicSession.runSql('insert into sample values ("carol")')
res3 = classicSession.runSql('insert into sample values ("jack")')
classicSession.rollback()

result = classicSession.runSql('select * from sample')
print 'Inserted Documents:', len(result.fetchAll())

#@ ClassicSession: Transaction handling: commit
classicSession.startTransaction()
res1 = classicSession.runSql('insert into sample values ("john")')
res2 = classicSession.runSql('insert into sample values ("carol")')
res3 = classicSession.runSql('insert into sample values ("jack")')
classicSession.commit()

result = classicSession.runSql('select * from sample')
print 'Inserted Documents:', len(result.fetchAll())

classicSession.dropSchema('node_session_schema')
classicSession.dropSchema('quoted schema')

#@ ClassicSession: current schema validations: nodefault, mysql
classicSession.setCurrentSchema('mysql')
dschema = classicSession.getDefaultSchema()
cschema = classicSession.getCurrentSchema()
print dschema
print cschema

#@ ClassicSession: current schema validations: nodefault, information_schema
classicSession.setCurrentSchema('information_schema')
dschema = classicSession.getDefaultSchema()
cschema = classicSession.getCurrentSchema()
print dschema
print cschema

#@ ClassicSession: current schema validations: default 
classicSession.close()
classicSession = mysql.getClassicSession(__uripwd + '/mysql')
dschema = classicSession.getDefaultSchema()
cschema = classicSession.getCurrentSchema()
print dschema
print cschema

#@ ClassicSession: current schema validations: default, information_schema
classicSession.setCurrentSchema('information_schema')
dschema = classicSession.getDefaultSchema()
cschema = classicSession.getCurrentSchema()
print dschema
print cschema

# Cleanup
classicSession.close()