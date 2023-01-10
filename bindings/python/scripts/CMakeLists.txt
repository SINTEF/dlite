project(dlite-bindings-python-scripts C)


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
add_test(
  NAME dlite-validate-Person
  COMMAND ${RUNNER} ${Python3_EXECUTABLE} dlite-validate
    ../tests/Person.json
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
  NAME dlite-validate-persons
  COMMAND ${RUNNER} ${Python3_EXECUTABLE} dlite-validate
    -p ../tests/Person.json
    -i 51c0d700-9ab0-43ea-9183-6ea22012ebee
    ../tests/persons.json
  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

add_test(
  NAME dlite-validate-persons2
  COMMAND ${RUNNER} ${Python3_EXECUTABLE} dlite-validate
    --storage-path ../tests/Person.json
    --id 13c5007b-c665-4709-a11c-255919306547
    --show
    ../tests/persons.json

  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)

# No --id for storage with multiple instances
add_test(
  NAME dlite-validate-persons-fail
  COMMAND ${RUNNER} ${Python3_EXECUTABLE} dlite-validate
    --storage-path ../tests/Person.json
    --show
    ../tests/persons.json

  WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
)
set_property(
  TEST dlite-validate-persons-fail
  PROPERTY WILL_FAIL true
)
