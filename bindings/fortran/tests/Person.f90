!
! Fortran interface to Person entity from Person.json
!
MODULE Person

  USE DLite

  IMPLICIT NONE
  PRIVATE

  PUBLIC :: TPersonDims
  PUBLIC :: TPerson

  PUBLIC :: readPerson
  PUBLIC :: writePerson

  TYPE TPersonDims
    integer(c_size_t):: n
    integer(c_size_t):: m
  END TYPE TPersonDims

  TYPE TPerson
    type(TPersonDims)              :: dims
    character(len=45)              :: name
    real(c_double)                 :: age
    character(len=10), allocatable :: skills(:)
    real(c_double), allocatable    :: temperature(:)
  END TYPE TPerson

  INTERFACE readPerson
    ! read a person from a data source (open and close the storage)
    module procedure readPersonFromSource
    ! read a person from a open storage
    module procedure readPersonFromStorage
  END INTERFACE

  INTERFACE writePerson
    ! write a person to a data source (open and close the storage)
    module procedure writePersonToSource
    ! write a person to a open storage
    module procedure writePersonToStorage
  END INTERFACE  

CONTAINS

TPerson
function readPersonFromSource(driver, uri, options, uid)
  character(len=*), intent(in) :: driver
  character(len=*), intent(in) :: uri
  character(len=*), intent(in) :: options
  character(len=*), intent(in) :: uid
  DLiteStorage                 :: storage
  TPerson                      :: person

  storage%open(driver, uri, "mode=r")
  person = readPersonFromStorage(storage, uid)
  storage%close()
  
  readPersonFromSource = person

end function readPersonFromSource

function readPersonFromStorage(storage, uid)
  DLiteStorage, intent(in)     :: storage
  character(len=*), intent(in) :: uid
  TPerson                      :: person

end function readPersonFromStorage

integer
function writePersonToSource(driver, uri, person)
  character(len=*), intent(in) :: driver
  character(len=*), intent(in) :: uri
  character(len=*), intent(in) :: uid
  DLiteStorage                 :: storage
  TPerson                      :: person

  storage%open(driver, uri, "mode=w")
  status = writePersonToStorage(storage, uid)
  storage%close()
  
  writePersonToSource = status

end function writePersonToSource

integer
function writePersonToStorage(storage, person)

end function writePersonToStorage

END MODULE Person