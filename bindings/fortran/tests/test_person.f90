! You must set environment variable DLITE_STORAGES='*.json' and run
! this test from the same directory as this file


program ftest_person

  use DLite
  use dlite_config, only: dlite_fortran_test_dir
  use Person

  implicit none

  type(TPerson)      :: person, john
  integer            :: status

  type(DLiteMeta) :: meta

  meta = create_meta_person()
  print *, meta%has_dimension('N'), meta%has_property('name')

  person = TPerson( &
       "json", &
       dlite_fortran_test_dir // "persons.json", &
       !"/workspaces/dlite/bindings/fortran/tests/persons.json", &
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


end program ftest_person
