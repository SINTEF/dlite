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
     COMMAND ${Python3_EXECUTABLE} setup.py install
       --prefix=${CMAKE_INSTALL_PREFIX} --force
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
  dlite-validate -p ../tests/Person.json -i 51c0d700-9ab0-43ea-9183-6ea22012ebee
    ../tests/persons.json
)
test_success(
  dlite-validate-persons2
  dlite-validate
    --storage-path=../tests/Person.json
    --id=13c5007b-c665-4709-a11c-255919306547
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