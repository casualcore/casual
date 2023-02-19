# generic buffer cli 

## casual-buffer-generic-compose

A generic _buffer composer_ that compeses a (possible) `casual` conformant
buffer from `stdin` and stream it to `stdout`


### example

```bash
$ echo "poop" |  casual-buffer-generic-compose --type "X_OCTET/" | casual call --service casual/example/echo > /dev/null
```

## casual-buffer-generic-extract

A generic _buffer extractor_ that extracts the payload of a generic buffer from `stdin` and stream it to `stdout`

### example

```bash
$ casual queue --dequeue foo | casual-buffer-generic-extract
```


## combination

In the spirit of being _unix friendly_ one can combine this stuff in ways that the _casual crew_ could not predict,
which is the point of being _unix friendly_

### example

```bash
$ cat some-important-file.yaml | casual-buffer-generic-compose | casual call --service casual/example/echo | casual-buffer-generic-extract | tee payload.log.txt | casual-buffer-generic-compose |  casual queue --enqueue foo 
```

... and so on...

## further information

Use the _--help_ option on the stuff you want to know more about. As with every other `casual` executable

## attention

The _commands_ in the examples do **NOT** guarantee transactional semantics in general, although `casual` will do 
it's best to make it as safe as possible, but it's not possible to maintain total transactional consistency.


