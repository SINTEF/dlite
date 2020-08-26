!
! Fortran interface to Person entity from Person.json
!
MODULE Person
  USE iso_c_binding, only : c_ptr, c_int, c_char, c_null_char, c_size_t, &
       c_double, c_null_ptr, c_f_pointer, c_associated
  USE DLite

  IMPLICIT NONE
  PRIVATE

  PUBLIC :: TPerson

  TYPE TPerson
    type(c_ptr)                    :: cinst
    !character(len=36)              :: uuid
    !type(TPersonDims)              :: dims
    integer(8)                     :: n
    integer(8)                     :: m
    character(len=45)              :: name
    real(8)                        :: age
    character(len=10), allocatable :: skills(:)
    real(8), allocatable           :: temperature(:)
  contains
    procedure :: check => checkPerson
    procedure :: writeToURL => writePersonToURL
    procedure :: writeToSource => writePersonToSource
    procedure :: writeToStorage => writePersonToStorage
  END TYPE TPerson

  INTERFACE TPerson
    ! read a person from an url
    module procedure readPersonFromURL
    ! read a person from a data source (open and close the storage)
    module procedure readPersonFromSource
    ! read a person from a open storage
    module procedure readPersonFromStorage
  END INTERFACE TPerson

CONTAINS

!  type(c_ptr)
!  function create_meta_person() result(meta)
!    info = dlite_metainfo_create(uri, iri, description)
!    dlite_metainfo_add_dim(info, "N", "Dim descriion")
!    prop1 = dlite_metainfo_add_prop(info, "name", "")
!    dlite_metainfo_add_prop_dim(prop1, "N")
!    meta = dlite_meta_create_form_info(info)
!  end function create_meta_person

  function personToInstance(person) result(instance)
    implicit none
    class(TPerson), intent(in) :: person
    type(DLiteInstance)        :: instance
    instance%cinst = person%cinst
  end function personToInstance

  subroutine assignPerson(person, instance)
    implicit none
    type(TPerson), intent(inout)     :: person
    class(DLiteInstance), intent(in) :: instance
    type(c_ptr)                      :: age_c
    real(8), pointer                 :: age_p
    if (instance%check()) then
       person%cinst = instance%cinst
       !person%uuid = instance%uuid
       person%n = instance%get_dimension_size_by_index(0)
       person%m = instance%get_dimension_size_by_index(1)
       age_c = instance%get_property("age")
       ! FIXME - correct assignment of age
       call c_f_pointer(age_c, age_p)
       person%age = age_p
    else
       person%cinst = c_null_ptr
    end if
  end subroutine assignPerson

  function checkPerson(person) result(status)
    implicit none
    class(TPerson), intent(in) :: person
    logical                    :: status
    status = c_associated(person%cinst)
  end function checkPerson


  function readPersonFromURL(url) result(person)
    implicit none
    character(len=*), intent(in)    :: url
    type(TPerson)                   :: person
    type(DLiteInstance)             :: instance
    instance = DLiteInstance(url)
    call assignPerson(person, instance)
  end function readPersonFromURL

  function readPersonFromSource(driver, location, options, uid) result(person)
    implicit none
    character(len=*), intent(in) :: driver
    character(len=*), intent(in) :: location
    character(len=*), intent(in) :: options
    character(len=*), intent(in) :: uid
    type(TPerson)                :: person
    type(DLiteStorage)           :: storage
    integer                      :: status
    storage = DLiteStorage(driver, location, options)
    person = readPersonFromStorage(storage, uid)
    status = storage%close()
  end function readPersonFromSource

  function readPersonFromStorage(storage, uid) result(person)
    implicit none
    type(DLiteStorage), intent(in) :: storage
    character(len=*), intent(in)    :: uid
    type(TPerson)                   :: person
    type(DLiteInstance)             :: instance
    instance = DLiteInstance(storage, uid)
    call assignPerson(person, instance)
  end function readPersonFromStorage

  function writePersonToURL(person, url) result(status)
    implicit none
    class(TPerson), intent(in)     :: person
    character(len=*), intent(in)   :: url
    type(DLiteInstance)            :: instance
    integer                        :: status
    instance = personToInstance(person)
    status = instance%save_url(url)
  end function writePersonToURL

  function writePersonToSource(person, driver, location, options) result(status)
    implicit none
    class(TPerson), intent(in)   :: person
    character(len=*), intent(in) :: driver
    character(len=*), intent(in) :: location
    character(len=*), intent(in) :: options
    type(DLiteStorage)           :: storage
    integer                      :: status, status2
    storage = DLiteStorage(driver, location, options)
    status = person%writeToStorage(storage)
    status2 = storage%close()
  end function writePersonToSource

  function writePersonToStorage(person, storage) result(status)
    implicit none
    class(TPerson), intent(in)     :: person
    type(DLiteStorage), intent(in) :: storage
    type(DLiteInstance)            :: instance
    integer                        :: status
    instance = personToInstance(person)
    status = instance%save(storage)
  end function writePersonToStorage

END MODULE Person
