Execution example:

```shell
extract \
  --extractors ExtractorHTML \
  --value "<html><body>He&lt;ll&gt;o</body></html>"
# [[0,1337566253.89858,0.000355720520019531],{"extracted":"He<ll>o"}]
```
