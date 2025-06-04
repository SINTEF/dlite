# -- Macro simplifying calling dlite-codegen
#
# dlite_codegen(output template url [options])
#
#   Generate source code using dlite-codegen.
#
# Arguments:
#   output
#       Path to generated output file to generate
#   template
#       Name of or path to the template to use
#   url
#       url of entity to generate code for
#   [ENV_OPTIONS options]
#       A comma-separated list of options to dlite-env
#   {options}
#       Additional options to dlite-codegen may be provided after the
#       ordinary arguments.  Relevant options include:
#         --built-in              Whether `url` refers to a built-in instance
#         --build-root            Whether to load plugins from build directory
#         --native-typenames      Whether to generate native typenames
#         --storage-plugins=PATH  Additional paths to look for storage plugins
#         --variables=STRING      Assignment of additional variables as a
#                                 semicolon-separated string of VAR=VALUE pairs
#
# Additional variables that are considered:
#
#   PATH
#       Path with the dlite library, Windows only
#   dlite_LD_LIBRARY_PATH
#       Path with the dlite library, Linux only
#   dlite_STORAGE_PLUGINS
#       Path to needed dlite storage plugins
#   dlite_TEMPLATES
#       Path to template files if `template` is not a path to an existing
#       template file
#   dlite_PATH
#       Path to look for dlite-codegen if it is not a target of the current
#       cmake session

macro(dlite_codegen output template url)

  include(CMakeParseArguments)
  cmake_parse_arguments(CODEGEN "" "ENV_OPTIONS" "" ${ARGN})

  string(REPLACE "," ";" env_options "${CODEGEN_ENV_OPTIONS}")
  set(codegen_dependencies "")
  set(codegen_extra_options ${CODEGEN_UNPARSED_ARGUMENTS})

  if(EXISTS ${template})
    set(template_option --template-file=${template})
    list(APPEND codegen_dependencies ${template})
  else()
    set(template_option --format=${template})
    foreach(dir ${dlite_TEMPLATES})
      if(EXISTS ${dir}/${template}.txt)
        list(APPEND codegen_dependencies "${dir}/${template}.txt")
      endif()
    endforeach()
  endif()

  if(EXISTS ${url})
    list(APPEND codegen_dependencies ${url})
  endif()

  if(WIN32)
    # Windows is really a headache. For some reason calling dlite-env
    # doesn't work.  Try instead to generate a bat file and run it.
    include(dliteGenEnv)

    set(out "")
    list(APPEND out "${output}")

    include(MakePlatformPaths)
    make_platform_paths(
      PREFIX e_
      PATHS
        out
        DLITE_CODEGEN
        dlite-tools_BINARY_DIR
        dlite_BINARY_DIR
        dlite_SOURCE_DIR
        dlite_LD_LIBRARY_PATH
        dlite_PATH_EXTRA
        dlite_PYTHONPATH
        dlite_PYTHON_STORAGE_PLUGINS
        dlite_PYTHON_MAPPING_PLUGINS
        dlite_TEMPLATES
      MULTI_CONFIG_PATHS
        dlite_PATH
        dlite_STORAGE_PLUGINS
        dlite_MAPPING_PLUGINS
      )

    get_filename_component(basename ${output} NAME)
    string(REPLACE . _ basename "${basename}")
    set(batfile "${basename}.bat")

    if(EXISTS ${dlite_SOURCE_DIR}/cmake/dliteCodeGen.bat.in)
      set(dlitecodegen_bat_in ${dlite_SOURCE_DIR}/cmake/dliteCodeGen.bat.in)
    else()
      find_file(dlitecodegen_bat_in
        NAMES dliteCodeGen.bat.in
        PATHS
          ${dlite_SOURCE_DIR}/cmake
          ${DLITE_ROOT}/share/dlite/cmake
          ${DLITE_ROOT}/cmake
        )
    endif()

    set(_url "${url}")
    configure_file(
      ${dlitecodegen_bat_in}
      ${batfile}
      @ONLY
      NEWLINE_STYLE CRLF
      )

    add_custom_command(
      OUTPUT ${output}
      COMMAND ${RUNNER} cmd /C ${batfile}
      DEPENDS ${codegen_dependencies}
      COMMENT "Generate ${output}"
      )

  else()
    # Run the code generator via dlite-env (we could run it directly on
    # Unix, but we keep it like this in case we some day figure out
    # how to make this work on Windows too...

    if(TARGET dlite-codegen)
      set(DLITE_CODEGEN $<TARGET_FILE:dlite-codegen>)
      list(APPEND codegen_dependencies dlite dlite-codegen)
      if(WITH_JSON)
        list(APPEND codegen_dependencies dlite-plugins-json)
      endif()
    else()
      find_program(DLITE_CODEGEN
        NAMES dlite-codegen dlite-codegen.exe
        PATHS
          ${DLITE_ROOT}/${DLITE_RUNTIME_DIR}
          ${dlite-tools_BINARY_DIR}
          ${dlite_PATH}
        )
      list(APPEND codegen_dependencies ${DLITE_CODEGEN})
    endif()

    if(TARGET dlite-env)
      set(DLITE_ENV $<TARGET_FILE:dlite-env>)
      list(APPEND codegen_dependencies dlite-env)
    else()
      find_program(DLITE_ENV
        NAMES dlite-env
        PATHS
          ${DLITE_ROOT}/${DLITE_RUNTIME_DIR}
          ${dlite-tools_BINARY_DIR}
          ${dlite_PATH}
        )
      list(APPEND codegen_dependencies ${DLITE_ENV})
    endif()


    add_custom_command(
      OUTPUT ${output}
      COMMAND
        ${DLITE_ENV}
          ${env_options}
          --
          ${DLITE_CODEGEN}
            --output=${output}
            ${template_option}
            ${url}
            ${codegen_extra_options}
      DEPENDS ${codegen_dependencies}
      COMMENT "Generate ${output}"
      )
  endif()

endmacro()
