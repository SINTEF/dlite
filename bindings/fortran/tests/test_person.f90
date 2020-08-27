! You must set environment variable DLITE_STORAGES='*.json' and run
! this test from the same directory as this file


program ftest_person

  use DLite
  use dlite_config, only: dlite_fortran_test_dir
  use Person

  implicit none

  type(TPerson)      :: person
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

  status = person%writeToSource( &
       "json", &
       "persons2.json", &
       "mode=w")


end program ftest_person
