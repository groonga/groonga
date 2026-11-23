Execution example:

```shell
extract \
  --extractors 'ExtractorHTML("selector", "article")' \
  --value "<nav>Menu</nav><article>Hello</article><footer>Footer</footer>"
# [[0,1337566253.89858,0.000355720520019531],{"extracted":"Hello"}]
```
