Execution example:

```shell
extract \
  --extractors 'ExtractorJSON("path", "$.values[*][*]")' \
  --value '{"values": [[1, 10], [100, 1000]]}'
# [[0,1337566253.89858,0.000355720520019531],{"extracted":[1,10,100,1000]}]
```
