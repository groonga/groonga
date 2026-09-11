Execution example:

```shell
extract \
  --extractors 'ExtractorHTML("expand_character_reference", false)' \
  --value "<html><body>He&lt;ll&gt;o</body></html>"
# [[0,1337566253.89858,0.000355720520019531],{"extracted":"He&lt;ll&gt;o"}]
```
