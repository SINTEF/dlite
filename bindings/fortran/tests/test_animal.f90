! You must set environment variable DLITE_STORAGES='*.json' and run
! this test from the same directory as this file

program ftest_animal

  use iso_c_binding
  use DLite
  ! use dlite_config, only: dlite_fortran_test_dir
  use Animal

  implicit none

  type(TAnimal)      :: dog
  type(TAnimal)      :: cat
  integer            :: status

  print *, "test_animal.f90: create animals"

  dog = TAnimal()
  dog%name = 'dog'
  dog%young = 'puppy'

  cat = TAnimal()
  cat%name = 'cat'
  cat%young = 'kitten'

  status = dog%writeToURL("json://animals.json?mode=w")
  status = cat%writeToURL("json://animals.json?mode=append")

  status = dog%destroy()
  status = cat%destroy()

end program ftest_animal
