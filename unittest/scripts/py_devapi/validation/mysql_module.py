#@ mysql module: exports
|Exported Items: 1|
|getClassicSession: <type 'builtin_function_or_method'>|

#@ mysql module: getClassicSession through URI
|<ClassicSession:|
|Session using right URI|

#@ mysql module: getClassicSession through URI and password
|<ClassicSession:|
|Session using right URI|

#@ mysql module: getClassicSession through data
|<ClassicSession:|
|Session using right URI|

#@ mysql module: getClassicSession through data and password
|<ClassicSession:|
|Session using right URI|

#@ Stored Sessions, session from data dictionary
|<ClassicSession:|
|Session using right URI|

#@ Stored Sessions, session from data dictionary removed
||IndexError: unknown attribute: mysql_data

#@ Stored Sessions, session from uri
|<ClassicSession:|
|Session using right URI|

#@ Stored Sessions, session from uri removed
||IndexError: unknown attribute: mysql_uri