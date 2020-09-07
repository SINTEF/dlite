! You must set environment variable DLITE_STORAGES='*.json' and run
! this test from the same directory as this file


program ftest_person

  use DLite
  use dlite_config, only: dlite_fortran_test_dir
  use Person
  use Scan3D

  implicit none

  type(DLiteStorage) :: storage
  type(TPerson)      :: person, john
  type(TScan3D)      :: scan
  integer            :: status

  person = TPerson( &
       "json", &
       dlite_fortran_test_dir // "persons.json", &
       "mode=r", &
       "d473aa6f-2da3-4889-a88d-0c96186c3fa2")

  if (.not. person%check() ) then
     print *, 'could not read person...'
     stop
  end if

  print *, 'n =', person%n
  print *, 'm =', person%m
  print *, 'name =', person%name
  print *, 'age =', person%age
  print *, 'skills =', person%skills
  print *, 'temperature =', person%temperature

  person%age = 34

  john = TPerson(2, 4)
  john%name = 'John Doe'
  john%age = 45.0
  john%skills(1) = 'c++'
  john%skills(2) = 'py'
  john%temperature = [37.5, 37.6, 37.9, 36.8]

  status = john%writeToSource( &
       "json", &
       "persons2.json", &
       "mode=w")

scan = TScan3D(5, 3)
scan%date = '2020-09-07'
scan%points(1,:) = [1, 1, 1]

storage = DLiteStorage("json", &
                       dlite_fortran_test_dir // "inputs.json", &
                       "mode=r")
person = TPerson(storage, "b04965e6-a9bb-591f-8f8a-1adcb2c8dc39")
scan = TScan3D(storage, "4b166dbe-d99d-5091-abdd-95b83330ed3a")
status = storage%close()

print *, scan%points
status = scan%writeToURL("json://scans.json")

end program ftest_person
