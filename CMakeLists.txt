cmake_minimum_required(VERSION 3.16)
project(Unbit CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Otimizacoes do proprio compilador Unbit (nao do codigo alvo).
set(CMAKE_CXX_FLAGS_RELEASE "-O2")

add_executable(unbit
    main.cpp
    include/lexer/lexer.cpp
    include/parser/parser.cpp
    include/compiler/CodeGenerator.cpp
    include/otimizer/AssemblyOptimizer.cpp
)

target_include_directories(unbit PRIVATE ${CMAKE_SOURCE_DIR})

# Nome amigavel exigido pelo usuario: ./compiler
set_target_properties(unbit PROPERTIES OUTPUT_NAME "unbit")

# Copia/linka o binario como "compiler" tambem, para "./compiler -asm".
add_custom_command(TARGET unbit POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy
        $<TARGET_FILE:unbit>
        ${CMAKE_SOURCE_DIR}/compiler
    COMMENT "Copiando binario para ./compiler")
