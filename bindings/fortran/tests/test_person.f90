! You must set environment variable DLITE_STORAGES='*.json' and run
! this test from the same directory as this file

program ftest_person

  use iso_c_binding
  use DLite
  use dlite_config, only: dlite_fortran_test_dir
  use Person
  use Scan3D

  implicit none

  type(TPerson)      :: person

  type(DLiteStorage) :: storage
  type(TPerson)      :: john
  type(TScan3D)      :: scan
  integer            :: status, i

  print *, "test_person.f90: load persons.json"
  person = TPerson("json", &
                   dlite_fortran_test_dir // "persons.json", &
                   "mode=r", &
                   "d473aa6f-2da3-4889-a88d-0c96186c3fa2")

  print *, 'uuid        = ', person%uuid
  print *, 'n           = ', person%n
  print *, 'm           = ', person%m
  print *, 'name        = ', person%name
  print *, 'age         = ', person%age
  print *, 'skills      = ', person%skills
  print *, 'temperature = ', person%temperature

  print *, "test_person.f90: load persons_names.json"
  person = TPerson("json", &
                   dlite_fortran_test_dir // "persons_names.json", &
                   "mode=r;useid=keep", &
                   "Joe Doe")

  print *, 'uuid        = ', person%uuid
  print *, 'n           = ', person%n
  print *, 'm           = ', person%m
  print *, 'name        = ', person%name
  print *, 'age         = ', person%age
  print *, 'skills      = ', person%skills
  print *, 'temperature = ', person%temperature  

  john = TPerson(2, 4)
  john%name = 'John Doe'
  !john%name = 'first name middle name last name --!--- names !!'
  john%age = 45.0
  john%skills(1) = 'c++'
  john%skills(2) = 'py'
  john%temperature = [37.5, 37.6, 37.9, 36.8]

  status = john%writeToSource("json", "persons2.json", "mode=w")

  storage = DLiteStorage("json", &
       dlite_fortran_test_dir // "inputs.json", &
       "mode=r")
  print *, "test_person.f90: load person in inputs.json"
  person = TPerson(storage, "b04965e6-a9bb-591f-8f8a-1adcb2c8dc39")
  print *, "test_person.f90: load person in inputs.json"
  scan = TScan3D(storage, "4b166dbe-d99d-5091-abdd-95b83330ed3a")
  print *, 'scan date = ', scan%date
  status = storage%close()

  print *, "Array, shape=(", size(scan%points, 1), ",", size(scan%points, 2), ")"
  do i = 1, size(scan%points, 1)
    print *, scan%points(i, :)
  end do
  status = scan%writeToURL("json://scans.json")

  scan = TScan3D(5, 3)
  scan%date = '2020-09-07'
  scan%points(:,:) = 11.0
  scan%points(1,1) = 1
  scan%points(1,2) = 2
  scan%points(1,3) = 3
  print *, "Array, shape=(", size(scan%points, 1), ",", size(scan%points, 2), ")"
  do i = 1, size(scan%points, 1)
    print *, scan%points(i, :)
  end do
  status = scan%writeToURL("json://scans2.json")

  status = person%destroy()
  status = john%destroy()
  status = scan%destroy()
end program ftest_person
