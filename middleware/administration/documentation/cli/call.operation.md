# casual call

[//]: # (Attention! this is a generated markdown from casual-administration-cli-documentation - do not edit this file!)

```console
host# casual --help call

call [0..1]
     generic service call
     
     Reads buffer(s) from stdin and call the provided service, prints the reply buffer(s) to stdout.
     Assumes that the input buffer to be in a conformant format, ie, created by casual.
     Errors will be printed to stderr
     
     @note: part of casual-pipe

   SUB OPTIONS:

      -s, --service [0..1]  (<service>) [1]
           service to call

      --iterations [0..1]  (<value>) [1]
           number of iterations (default: 1) - this could be helpful for testing load

      --examples [0..1]
           prints several examples of how casual call can be used

      [deprecated] --asynchronous [0..1]  (<value>) [1]
           [removed] use `casual --block true|false call ...` instead

      [deprecated] --transaction [0..1]  (<value>) [1]
           [removed] use `casual transaction --begin` instead

```
