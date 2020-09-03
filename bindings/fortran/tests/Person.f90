!
! Fortran interface to Person entity from Person.json
!
MODULE Person
  USE iso_c_binding, only : c_ptr, c_int, c_char, c_null_char, c_size_t, &
                            c_double, c_null_ptr, c_f_pointer, c_associated, &
                            c_loc, c_float
  USE c_interface, only: c_f_string, c_strlen_safe, f_c_string
  USE DLite

  IMPLICIT NONE
  PRIVATE

  PUBLIC :: TPerson
  PUBLIC :: create_meta_person

  TYPE TPerson
    type(c_ptr)                    :: cinst
    !character(len=36)              :: uuid
    !type(TPersonDims)              :: dims
    integer(8)                     :: n
    integer(8)                     :: m
    character(len=45)              :: name
    real(4)                        :: age
    character(len=10), allocatable :: skills(:)
    real(4), allocatable           :: temperature(:)
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
    ! create a person and allocate the arrays
    module procedure createPerson32
    ! create a person and allocate the arrays
    module procedure createPerson64
  END INTERFACE TPerson

CONTAINS

  function create_meta_person() result(meta)
    type(DLiteMeta)      :: meta
    type(DLiteMetaModel) :: model
    integer              :: i

    model = DLiteMetaModel('http://meta.sintef.no/0.2/Person', 'http://meta.sintef.no/0.3/EntitySchema', '')
    i = model%add_string('description', 'A person.')
    i = model%add_dimension('N', 'Number of skills.')
    i = model%add_dimension('M', 'Number of temperature measurements.')
    i = model%add_property('name', 'string45', '', '', '')
    i = model%add_property('age', 'float', 'years', '', '')
    i = model%add_property('skills', 'string10', '', '', '')
    i = model%add_property_dim('skills', 'N')
    i = model%add_property('temperature', 'float', 'degC', '', '')
    i = model%add_property_dim('temperature', 'M')
    meta = DLiteMeta(model)
    call model%destroy()
  end function create_meta_person

  function createPerson32(n, m) result(person)
    implicit none
    type(TPerson)          :: person
    integer(4), intent(in) :: n
    integer(4), intent(in) :: m
    person%n = n
    person%m = m
    allocate(person%skills(n))
    allocate(person%temperature(m))
  end function createPerson32

  function createPerson64(n, m) result(person)
    implicit none
    type(TPerson)          :: person
    integer(8), intent(in) :: n
    integer(8), intent(in) :: m
    person%n = n
    person%m = m
    allocate(person%skills(n))
    allocate(person%temperature(m))
  end function createPerson64

  function personToInstance(person) result(instance)
    implicit none
    class(TPerson), intent(in)      :: person
    type(DLiteInstance)             :: instance
    !type(c_ptr)                     :: cptr
    !real(c_float), pointer          :: age_f
    !character(kind=c_char), pointer :: skills_f(:,:)
    !real(c_float), pointer          :: temperature_f(:)
    !integer(8)                      :: nstring, strlen, i
    ! create new DLiteInstance
    instance = DLiteInstance('http://meta.sintef.no/0.2/Person', &
                             [person%n, person%m], '')
    call dlite_instance_set_property_value(instance, 0, person%name)
    call dlite_instance_set_property_value(instance, 1, person%age)
    call dlite_instance_set_property_value(instance, 2, person%skills)
    call dlite_instance_set_property_value(instance, 3, person%temperature)
    ! name
    !call f_c_string(person%name, instance%get_property_by_index(0))
    ! age
    !cptr = instance%get_property_by_index(1)
    !call c_f_pointer(cptr, age_f)
    !age_f = person%age
    ! skills
    !cptr = instance%get_property_by_index(2)
    !nstring = person%n
    !strlen = 10
    !print *, len(person%skills(1)), len(person%skills(2))
    !call c_f_pointer(cptr, skills_f, [strlen, nstring])
    !do i = 1, nstring
    !  call f_c_string(person%skills(i), skills_f(:,i))
    !end do
    ! temperature    
    !cptr = instance%get_property_by_index(3)
    !call c_f_pointer(cptr, temperature_f, (/person%m/))
    !temperature_f = person%temperature
    !instance%cinst = person%cinst
  end function personToInstance

  ! Copy C data (DLiteInstance::instance) in Fortran type (TPerson::person)
  subroutine assignPerson(person, instance)
    implicit none
    type(TPerson), intent(inout)            :: person
    class(DLiteInstance), intent(in)        :: instance
    type(c_ptr)                             :: cptr
    real(c_float), pointer                  :: age_f
    character(kind=c_char), pointer         :: skills_f(:,:)
    real(c_float), pointer                  :: temperature_f(:)

    integer(8)                              :: nstring, strlen, i

    if (instance%check()) then
      person%cinst = instance%cinst
      !person%uuid = instance%uuid
      person%n = instance%get_dimension_size_by_index(0)
      person%m = instance%get_dimension_size_by_index(1)
      ! name
      cptr = instance%get_property_by_index(0)
      call c_f_string(cptr, person%name)
      ! age
      cptr = instance%get_property_by_index(1)
      call c_f_pointer(cptr, age_f)
      person%age = age_f
      ! skills
      cptr = instance%get_property_by_index(2)
      nstring = person%n
      strlen = 10
      call c_f_pointer(cptr, skills_f, [strlen, nstring])
      allocate(person%skills(nstring))
      do i = 1, nstring
        call c_f_string(skills_f(:,i), person%skills(i))
      end do
      ! temperature
      cptr = instance%get_property_by_index(3)
      call c_f_pointer(cptr, temperature_f, (/person%m/))
      person%temperature = temperature_f
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
