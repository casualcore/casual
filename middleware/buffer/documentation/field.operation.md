# field operation

## environment

If the application is using a casual-field-repository, the environment-variable `CASUAL_FIELD_TABLE` needs to be present and its value should reference `1..*` _repository-files_, separated by the delimiter `|`

### example

```bash
$ export CASUAL_FIELD_TABLE=path/to/repository/file.yaml
```

```bash
$ export CASUAL_FIELD_TABLE=path/to/file1.yaml|path/to/file2.json
```


## samples 

* [JSON](./../sample/field.json)
* [XML](./../sample/field.xml)
* [INI](./../sample/field.ini)
* [YAML](./../sample/field.yaml)
