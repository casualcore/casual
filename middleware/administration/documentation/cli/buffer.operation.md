# casual buffer

```bash
>$ casual --help buffer

  buffer [0..1]
        buffer related 'tools'

    SUB OPTIONS
      --field-from-human [0..1]  (json, yaml, xml, ini) [0..1]
            transform human readable fielded buffer to actual buffers
            
            reads from stdin and assumes a human readable structure in the supplied format
            for a casual-fielded-buffer, and transform this to an actual casual-fielded-buffer,
            and forward this to stdout for other downstream in the pipeline to consume
            
            @note: part of casual-pipe

      --field-to-human [0..1]  (json, yaml, xml, ini) [0..1]
            reads from stdin and assumes a casual-fielded-buffer
            
            and transform this to a human readable structure in the supplied format,
            and prints this to stdout
            
            @note: part of casual-pipe
            @note: this is a 'casual-pipe' termination - no internal representation will be sent downstream

      --compose [0..1]  (X_OCTET/, .binary/, .yaml/, .xml/, .json/, .ini/) [0..1]
            reads 'binary' data from stdin and compose one actual buffer
            
            with the supplied type, and forward this to stdout for other downstream 'components'
            in the pipeline to consume
            
            if no 'type' is provided, `X_OCTET/` is used
            
            @note: part of casual-pipe

      --duplicate [0..1]  (<value>) [1]
            duplicates buffers read from stdin and send them downstream via stdout
            
            `count` amount of times.
            
            @note: part of casual-pipe

      --extract [0..1]
            read the buffers from stdin and extract the payload and sends it to stdout
            
            if --verbose is provided the type of the buffer will be sent to stderr.
            
            @note: part of casual-pipe
            @note: this is a 'casual-pipe' termination - no internal representation will be sent downstream

```
