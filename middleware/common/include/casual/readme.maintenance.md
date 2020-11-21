# include paths

We need to have all our _exported/public_ headers under `<installdir>/include/casual/<public-header>`
So we get stricter (by far) include paths for the user, and enable the possibility to install
`casual` under default install paths, i.e `/usr/include`, `/opt/include` and so on...

So, the objective is to have all our headers under the _include root_ `casual`. But we start with 
the _public_ ones.

