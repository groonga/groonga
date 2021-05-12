.. -*- rst -*-

Output
======

Groonga supports the following output format types:

  * `JSON <http://www.json.org/>`_
  * `XML <http://www.w3.org/XML/>`_
  * TSV (Tab Separated Values)
  * `MessagePack <http://msgpack.org/>`_

JSON is the default output format.

Usage
-----

Groonga has the following query interfaces:

  * command line
  * HTTP

They provides different ways to change output format type.

Command line
^^^^^^^^^^^^

You can use command line query interface by ``groonga
DB_PATH`` or ``groonga -c``. Those groonga commands shows
``>`` prompt. In this query interface, you can specify
output format type by ``output_type`` option.

If you don't specify ``output_type`` option, you will get
a result in JSON format::

  > status
  [[0,1327721628.10738,0.000131845474243164],{"alloc_count":142,"starttime":1327721626,"uptime":2,"version":"1.2.9-92-gb87d9f8","n_queries":0,"cache_hit_rate":0.0,"command_version":1,"default_command_version":1,"max_command_version":2}]

You can specify ``json`` as ``output_type`` value to get a
result in JSON format explicitly::

  > status --output_type json
  [[0,1327721639.08321,7.93933868408203e-05],{"alloc_count":144,"starttime":1327721626,"uptime":13,"version":"1.2.9-92-gb87d9f8","n_queries":0,"cache_hit_rate":0.0,"command_version":1,"default_command_version":1,"max_command_version":2}]

You need to specify ``xml`` as ``output_type`` value to
get a result in XML format::

  > status --output_type xml
  <?xml version="1.0" encoding="utf-8"?>
  <RESULT CODE="0" UP="1327721649.61095" ELAPSED="0.000126361846923828">
  <RESULT>
  <TEXT>alloc_count</TEXT>
  <INT>146</INT>
  <TEXT>starttime</TEXT>
  <INT>1327721626</INT>
  <TEXT>uptime</TEXT>
  <INT>23</INT>
  <TEXT>version</TEXT>
  <TEXT>1.2.9-92-gb87d9f8</TEXT>
  <TEXT>n_queries</TEXT>
  <INT>0</INT>
  <TEXT>cache_hit_rate</TEXT>
  <FLOAT>0.0</FLOAT>
  <TEXT>command_version</TEXT>
  <INT>1</INT>
  <TEXT>default_command_version</TEXT>
  <INT>1</INT>
  <TEXT>max_command_version</TEXT>
  <INT>2</INT></RESULT>
  </RESULT>

You need to specify ``tsv`` as ``output_type`` value to
get a result in TSV format::

  > status --output_type tsv
  0	1327721664.82675	0.000113964080810547
  "alloc_count"	146
  "starttime"	1327721626
  "uptime"	38
  "version"	"1.2.9-92-gb87d9f8"
  "n_queries"	0
  "cache_hit_rate"	0.0
  "command_version"	1
  "default_command_version"	1
  "max_command_version"	2
  END


You need to specify ``msgpack`` as ``output_type`` value to
get a result in MessagePack format::

  > status --output_type msgpack
  (... omitted because MessagePack is binary data format. ...)

HTTP
^^^^

You can use HTTP query interface by ``groonga --protocol
http -s DB_PATH``. Groonga HTTP server starts on port 10041
by default. In this query interface, you can specify
output format type by extension.

If you don't specify extension, you will get a result in
JSON format::

  % curl http://localhost:10041/d/status
  [[0,1327809294.54311,0.00082087516784668],{"alloc_count":155,"starttime":1327809282,"uptime":12,"version":"1.2.9-92-gb87d9f8","n_queries":0,"cache_hit_rate":0.0,"command_version":1,"default_command_version":1,"max_command_version":2}]

You can specify ``json`` as extension to get a result in
JSON format explicitly::

  % curl http://localhost:10041/d/status.json
  [[0,1327809319.01929,9.5367431640625e-05],{"alloc_count":157,"starttime":1327809282,"uptime":37,"version":"1.2.9-92-gb87d9f8","n_queries":0,"cache_hit_rate":0.0,"command_version":1,"default_command_version":1,"max_command_version":2}]

You need to specify ``xml`` as extension to get a result in
XML format::

  % curl http://localhost:10041/d/status.xml
  <?xml version="1.0" encoding="utf-8"?>
  <RESULT CODE="0" UP="1327809339.5782" ELAPSED="9.56058502197266e-05">
  <RESULT>
  <TEXT>alloc_count</TEXT>
  <INT>159</INT>
  <TEXT>starttime</TEXT>
  <INT>1327809282</INT>
  <TEXT>uptime</TEXT>
  <INT>57</INT>
  <TEXT>version</TEXT>
  <TEXT>1.2.9-92-gb87d9f8</TEXT>
  <TEXT>n_queries</TEXT>
  <INT>0</INT>
  <TEXT>cache_hit_rate</TEXT>
  <FLOAT>0.0</FLOAT>
  <TEXT>command_version</TEXT>
  <INT>1</INT>
  <TEXT>default_command_version</TEXT>
  <INT>1</INT>
  <TEXT>max_command_version</TEXT>
  <INT>2</INT></RESULT>
  </RESULT>

You need to specify ``tsv`` as extension to get a result in
TSV format::

  % curl http://localhost:10041/d/status.tsv
  0	1327809366.84187	8.44001770019531e-05
  "alloc_count"	159
  "starttime"	1327809282
  "uptime"	84
  "version"	"1.2.9-92-gb87d9f8"
  "n_queries"	0
  "cache_hit_rate"	0.0
  "command_version"	1
  "default_command_version"	1
  "max_command_version"	2
  END

You need to specify ``msgpack`` as extension to get a result
in MessagePack format::

  % curl http://localhost:10041/d/status.msgpack
  (... omitted because MessagePack is binary data format. ...)
