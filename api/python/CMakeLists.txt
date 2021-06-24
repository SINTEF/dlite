#
# WITH_PYTHON_BUILD_WHEEL=ON
# Generate a Python virtual environment for the base interpreter
# (and eventually for other Python versions), Configure the setup.py
# script, build the wheel in the venev.
#
# WITH_PYTHON_TEST_WHEEL=ON
# Install the wheel in the venv and run python-tests in the virtual environment.
#
# WITH_PYTHON_VENVTEST_ONLY=ON
# Must build and test the wheel
#

find_package(Python3 COMPONENTS Interpreter REQUIRED)

if(NOT Python3_FOUND)
    message(WARNING "Python executable not found, dlite-python wheel could not be built.")
else()
    
    configure_file(setup.py-testenv.in setup.py)

    # Setup a virtual environment
    find_program(VIRTUALENV virtualenv)
    if(NOT VIRTUALENV)
        message(FATAL_ERROR "Could not find `virtualenv` in PATH. pip install virtualenv.")
    endif()
    message(STATUS "Python base Interpreter: ${Python3_VERSION} path=${Python3_EXECUTABLE}")

    # This lists allows other python wheel versions to be build
    # Build the wheel only for the base interpreter for now
    set(python_versions_minor
      ${Python3_VERSION_MINOR}
    )
    set(python_versions_patch
      ${Python3_VERSION_PATCH}
    )

    foreach(python_version_minor python_version_patch IN ZIP_LISTS python_versions_minor python_versions_patch)

      set(PYTHON_VENV_VERSION "3.${python_version_minor}.${python_version_patch}")
      set(PYTHON_VENV_NAME "venv${PYTHON_VENV_VERSION}")

      if(WIN32)
        set(PYTHON_VENV_PYTHON ${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_VENV_NAME}/Scripts/python)
        set(PYTHON_VENV_PIP ${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_VENV_NAME}/Scripts/pip)
        set(WHEEL_BDIST_EXT win_amd64.whl)
      else()
        set(PYTHON_VENV_PYTHON ${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_VENV_NAME}/bin/python)
        set(PYTHON_VENV_PIP ${CMAKE_CURRENT_BINARY_DIR}/${PYTHON_VENV_NAME}/bin/pip)
        set(WHEEL_BDIST_EXT linux_x86_64.whl)
      endif()

      set(CURRENT_WHEEL_NAME dlite_python-${dlite_VERSION}-cp3${python_version_minor}-cp3${python_version_minor}-${WHEEL_BDIST_EXT})

      add_custom_target(dlite-python-wheel3${python_version_minor} ALL DEPENDS python_package dlite-plugins-python
        COMMAND ${VIRTUALENV} -p python${PYTHON_VERSION} ${PYTHON_VENV_NAME}
        COMMAND ${PYTHON_VENV_PIP} install wheel
        COMMAND ${PYTHON_VENV_PYTHON} ${CMAKE_CURRENT_BINARY_DIR}/setup.py bdist_wheel
      )
      #if(WITH_HDF5)
      #  add_dependencies ?
      #endif()
      #if(HAVE_REDLAND)
      #  add_dependencies ?
      #endif()

      if(WITH_PYTHON_TEST_WHEEL OR WITH_PYTHON_VENVTEST_ONLY)

        add_custom_target(dlite-python-wheel3${python_version_minor}_install ALL DEPENDS dlite-python-wheel3${python_version_minor}
          COMMAND ${PYTHON_VENV_PIP} install ${CMAKE_CURRENT_BINARY_DIR}/dist/${CURRENT_WHEEL_NAME}
        )
  
        set(tests
        test_python_bindings
        test_collection
        test_entity
        test_factory
        test_misc
        test_python_storage
        test_storage
        )
    
        foreach(test ${tests})
          set(name ${test}-py_venv_python${PYTHON_VENV_VERSION})
          add_test(
              NAME ${name}
              COMMAND ${RUNNER} ${PYTHON_VENV_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/../../bindings/python/tests/${test}.py
              )
        endforeach()
    
        # More tests in ./examples, add them here
    
        set(additional_tests
          read-csv
        )
    
        foreach(test ${additional_tests})
          set(name ${test}-py_venv_python${PYTHON_VENV_VERSION})
          add_test(
              NAME ${name}
              COMMAND ${RUNNER} ${PYTHON_VENV_PYTHON} ${CMAKE_CURRENT_SOURCE_DIR}/../../examples/${test}/main.py
              )
        endforeach()
      endif()
    endforeach()
    
endif()

if(WITH_PYTHON_BUILD_WHEEL)
    install(
        DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}/dist
        DESTINATION .
    )
endif()
