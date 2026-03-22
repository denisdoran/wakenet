set(requires
    boa_audio
)

idf_component_register(
    SRCS "main.c"
    INCLUDE_DIRS "."
    REQUIRES ${requires}
)
