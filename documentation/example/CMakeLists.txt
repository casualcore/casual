# domain examples
install(
    FILES
        domain/example.md
    DESTINATION
        documentation/example/domain
)

# single domain example
install(
    FILES
        domain/single/readme.md
        domain/single/domain.env
    DESTINATION
        documentation/example/domain/single
)

install(
    FILES
        domain/single/configuration/domain.yaml
    DESTINATION
        documentation/example/domain/single/configuration
)

# multi domain example
install(
    FILES
        domain/multiple/readme.md
    DESTINATION
        documentation/example/domain/multiple
)

install(
    FILES
        domain/multiple/diagram/scenario.svg
        domain/multiple/diagram/scenario.puml
    DESTINATION
        documentation/example/domain/multiple/diagram
)

install(
    FILES
        domain/multiple/A/domain.env
    DESTINATION
        documentation/example/domain/multiple/A
)

install(
    FILES
        domain/multiple/A/configuration/domain.yaml
    DESTINATION
        documentation/example/domain/multiple/A/configuration
)

install(
    FILES
        domain/multiple/B/domain.env
    DESTINATION
        documentation/example/domain/multiple/B
)

install(
    FILES
        domain/multiple/B/configuration/domain.yaml
    DESTINATION
        documentation/example/domain/multiple/B/configuration
)