Execution example:

```shell
table_create Pages TABLE_NO_KEY
# [[0,1337566253.89858,0.000355720520019531],true]
column_create Pages html COLUMN_SCALAR Text
# [[0,1337566253.89858,0.000355720520019531],true]
load --table Pages
[
{"html": "<html><body>He&lt;ll&gt;o</body></html>"},
{"html": "<nav>Menu</nav><article>Hello</article><footer>Footer</footer>"}
]
# [[0,1337566253.89858,0.000355720520019531],2]
```
