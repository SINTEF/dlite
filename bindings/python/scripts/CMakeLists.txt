project(dlite-bindings-python-scripts C)

# Convinience functions for check for success
# Usage: test_success(name, command...)
function(test_success name)
add_test(
  NAME ${name}
  COMMAND ${RUNNER} ${Python3_EXECUTABLE} ${ARGN}
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
endfunction()

# Convinience functions for check for failure
# Usage: test_failure(name, command...)
function(test_failure name)
  test_success(${name} ${ARGN})
  set_property(TEST ${name} PROPERTY WILL_FAIL true)
endfunction()


# Install
install(
  CODE "
   execute_process(
     COMMAND ${Python3_EXECUTABLE} -m pip install
       --prefix=${CMAKE_INSTALL_PREFIX} --force .
     WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
   )"
)


# Tests
test_success(
  dlite-validate-Person
  dlite-validate ../tests/Person.json
)
test_success(
  dlite-validate-persons
  dlite-validate -p ../tests/Person.json -i a1d8d35f-723c-5ea1-abaf-8fc8f1d0982f
    ../tests/persons.json
)
test_success(
  dlite-validate-persons2
  dlite-validate
    --storage-path=../tests
    --id=Ada
    --show
    ../tests/persons.json
  )

# No --id for storage with multiple instances
test_failure(
  dlite-validate-persons-fail2
  dlite-validate --storage-path ../tests/Person.json --show ../tests/persons.json
)

# The "dimensions" keyword is required
test_failure(
  dlite-validate-no-dimensions
  dlite-validate NoDimensions.json
)

# Ok with an empty entity as long as the "dimensions" and "properties" keys are there.
test_success(
  dlite-validate-empty
  dlite-validate Empty.json
)
