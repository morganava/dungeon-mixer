add_compile_options(-Wall)
add_compile_options(-Werror)
add_compile_options(-Wfatal-errors)
add_compile_options(-O3)
add_compile_options(-g)

add_library(nuklear STATIC
	nuklear.c)

