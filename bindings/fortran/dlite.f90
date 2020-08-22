!
! Fortran interface to DLite
!
module DLite
  use iso_c_binding, only : c_ptr, c_int, c_size_t, c_char, c_null_char, &
       c_associated

  implicit none
  private

  type, public :: DLiteStorage
     type(c_ptr) :: storage
     integer     :: readonly
   contains
     procedure :: close => dlite_storage_close
     procedure :: is_writable => dlite_storage_is_writable
  end type DLiteStorage

  interface DLiteStorage
     module procedure dlite_storage_open ! User-defined constructor
  end interface DLiteStorage


  type, public :: DLiteInstance
     type(c_ptr)                   :: cinst
     !character(36,kind=c_char)     :: uuid
     !character(len=1, kind=c_char) :: uri
     !character(len=1, kind=c_char) :: iri
   contains
     procedure :: check => check_instance
     procedure :: save => dlite_instance_save
     procedure :: save_url => dlite_instance_save_url
     procedure :: get_uuid => dlite_instance_get_uuid
     procedure :: get_dimension_size => dlite_instance_get_dimension_size
     procedure :: get_dimension_size_by_index => dlite_instance_get_dimension_size_by_index
     procedure :: get_property => dlite_instance_get_property
     procedure :: get_property_by_index => dlite_instance_get_property_by_index
  end type DLiteInstance

  interface DLiteInstance
     module procedure dlite_instance_create_from_id
     module procedure dlite_instance_load
     module procedure dlite_instance_load_url
     module procedure dlite_instance_load_casted
  end interface DLiteInstance

  ! --------------------------------------------------------
  ! C interface
  ! --------------------------------------------------------
  interface

     ! Storage
     type(c_ptr) &
     function dlite_storage_open_c(driver, uri, options) &
         bind(C,name="dlite_storage_open")
       import c_ptr, c_char
       character(len=1,kind=c_char), dimension(*), intent(in) :: driver
       character(len=1,kind=c_char), dimension(*), intent(in) :: uri
       character(len=1,kind=c_char), dimension(*), intent(in) :: options
     end function dlite_storage_open_c

     integer(c_int) &
     function dlite_storage_close_c(storage) &
         bind(C,name="dlite_storage_close")
       import c_ptr, c_int
       type(c_ptr), value, intent(in) :: storage
     end function dlite_storage_close_c

     integer(c_int) &
     function dlite_storage_is_writable_c(storage) &
         bind(C,name="dlite_storage_is_writable")
       import c_ptr, c_int
       type(c_ptr), value, intent(in) :: storage
     end function dlite_storage_is_writable_c

     ! Instance
     type(c_ptr) &
     function dlite_instance_create_from_id_c(metaid, dims, id) &
         bind(C,name="dlite_instance_create_from_id")
       import c_ptr, c_size_t, c_char
       character(len=1,kind=c_char), dimension(*), intent(in) :: metaid
       integer(c_size_t), dimension(*), intent(in)            :: dims
       character(len=1,kind=c_char), dimension(*), intent(in) :: id
     end function dlite_instance_create_from_id_c

     type(c_ptr) &
     function dlite_instance_load_c(storage, id) &
         bind(C,name="dlite_instance_load")
       import c_ptr, c_char
       type(c_ptr), value, intent(in)                         :: storage
       character(len=1,kind=c_char), dimension(*), intent(in) :: id
     end function dlite_instance_load_c

     type(c_ptr) &
     function dlite_instance_load_url_c(url) &
         bind(C,name="dlite_instance_load_url")
       import c_ptr, c_char
       character(len=1,kind=c_char), dimension(*), intent(in) :: url
     end function dlite_instance_load_url_c

     type(c_ptr) &
     function dlite_instance_load_casted_c(storage, id, metaid) &
         bind(C,name="dlite_instance_load_casted")
       import c_ptr, c_char
       type(c_ptr), value, intent(in)                         :: storage
       character(len=1,kind=c_char), dimension(*), intent(in) :: id
       character(len=1,kind=c_char), dimension(*), intent(in) :: metaid
     end function dlite_instance_load_casted_c

     integer(c_int) &
     function dlite_instance_save_c(storage, instance) &
         bind(C,name="dlite_instance_save")
       import c_ptr, c_int
       type(c_ptr), value, intent(in)                         :: storage
       type(c_ptr), value, intent(in)                         :: instance
     end function dlite_instance_save_c

     integer(c_int) &
     function dlite_instance_save_url_c(url, instance) &
         bind(C,name="dlite_instance_save_url")
       import c_ptr, c_char, c_int
       character(len=1,kind=c_char), dimension(*), intent(in) :: url
       type(c_ptr), value, intent(in)                         :: instance
     end function dlite_instance_save_url_c

     type(c_ptr) &
     function dlite_instance_get_uuid_c(instance) &
         bind(C,name='dlite_instance_get_uuid')
       import c_ptr
       type(c_ptr), value, intent(in)                         :: instance
     end function dlite_instance_get_uuid_c

     integer(c_size_t) &
     function dlite_instance_get_dimension_size_c(instance, name) &
         bind(C,name="dlite_instance_get_dimension_size")
       import c_ptr, c_char, c_size_t
       type(c_ptr), value, intent(in)                         :: instance
       character(len=1,kind=c_char), dimension(*), intent(in) :: name
     end function dlite_instance_get_dimension_size_c

     integer(c_size_t) &
     function dlite_instance_get_dimension_size_by_index_c(instance, i) &
         bind(C,name="dlite_instance_get_dimension_size_by_index")
       import c_ptr, c_size_t
       type(c_ptr), value, intent(in)                         :: instance
       integer(c_size_t), value, intent(in)                   :: i
     end function dlite_instance_get_dimension_size_by_index_c

     type(c_ptr) &
     function dlite_instance_get_property_c(instance, name) &
         bind(C,name="dlite_instance_get_property")
       import c_ptr, c_char
       type(c_ptr), value, intent(in)                         :: instance
       character(len=1,kind=c_char), dimension(*), intent(in) :: name
     end function dlite_instance_get_property_c

     type(c_ptr) &
     function dlite_instance_get_property_by_index_c(instance, i) &
         bind(C,name="dlite_instance_get_property_by_index")
       import c_ptr, c_size_t
       type(c_ptr), value, intent(in)                         :: instance
       integer(c_size_t), value, intent(in)                   :: i
     end function dlite_instance_get_property_by_index_c

  end interface

contains
  ! --------------------------------------------------------
  ! Generic help functions
  ! --------------------------------------------------------

  ! Returns length of `c_string`
  function c_string_length(c_string) result(length)
    use, intrinsic :: iso_c_binding, only: c_ptr
    type(c_ptr), intent(in) :: c_string
    integer                 :: length
    interface
      ! use std c library function rather than writing our own.
      function strlen(s) bind(c, name='strlen')
        use, intrinsic :: iso_c_binding, only: c_ptr, c_size_t
        implicit none
        type(c_ptr), intent(in), value :: s
        integer(c_size_t) :: strlen
      end function strlen
    end interface
    length = int(strlen(c_string))
  end function c_string_length


  function f_c_string_func(f_string) result (c_string)
    character(len=*), intent(in) :: f_string
    character(len=1,kind=c_char) :: c_string(len_trim(f_string)+1)
    integer                      :: n, i

    n = len_trim(f_string)
    do i = 1, n
      c_string(i) = f_string(i:i)
    end do
    c_string(n + 1) = c_null_char

  end function f_c_string_func


  !function c_f_string(cptr) result(f_string)
  !  ! Convert a null-terminated c string into a fortran character array pointer
  !  type(c_ptr), intent(in) :: cptr ! the c address
  !  character(kind=c_char), dimension(:), pointer :: f_string
  !  character(kind=c_char), dimension(1), target :: dummy_string="?"
  !
  !  interface ! strlen is a standard c function from <string.h>
  !     ! int strlen(char *string)
  !     function strlen(string) result(len) bind(c,name="strlen")
  !       use iso_c_binding
  !       type(c_ptr), value      :: string
  !       integer(kind=c_size_t)  :: len
  !     end function strlen
  !  end interface
  !
  !  if(c_associated(cptr)) then
  !     call c_f_pointer(f_string, cptr, [strlen(cptr)])
  !  else
  !     ! To avoid segfaults, associate f_string with a dummy target
  !     f_string=>dummy_string
  !  end if
  !
  !end function c_f_string

  !! Copy C string to Fortran string
  !subroutine c_string_copy(c_string, f_string)
  !  use, intrinsic :: iso_c_binding, only: c_ptr, c_char
  !  type(c_ptr), intent(in)             :: c_string
  !  character, allocatable, intent(out) :: f_string(:)
  !  integer                             :: i
  !  interface
  !     subroutine strncpy(dest, src, n) bind(c,name="strncpy")
  !       use iso_c_binding
  !       type(c_ptr), intent(out)      :: dest
  !       type(c_ptr), intent(in)       :: src
  !       integer(kind=c_size_t), value :: n
  !     end subroutine strncpy
  !  end interface
  !
  !  call strncpy(c_loc(f_string), c_string, size(f_string))
  !  do i = size(f_string) - c_string_length(c_string), size(f_string)
  !     f_string(i:i) = ' '
  !  end do
  !end subroutine c_string_copy


  ! --------------------------------------------------------
  ! Fortran methods for DLiteStorage
  ! --------------------------------------------------------

  function check_instance(instance) result(status)
    implicit none
    class(DLiteInstance), intent(in) :: instance
    logical                          :: status
    status = c_associated(instance%cinst)
  end function check_instance


  function dlite_storage_open(driver, uri, options) result(storage)
    character(len=*), intent(in) :: driver
    character(len=*), intent(in) :: uri
    character(len=*), intent(in) :: options
    character(len=1,kind=c_char) :: driver_c(len_trim(driver)+1)
    character(len=1,kind=c_char) :: uri_c(len_trim(uri)+1)
    character(len=1,kind=c_char) :: options_c(len_trim(options)+1)
    type(DliteStorage)           :: storage

    driver_c = f_c_string_func(driver)
    uri_c = f_c_string_func(uri)
    options_c = f_c_string_func(options)

    storage%storage = dlite_storage_open_c(driver_c, uri_c, options_c)
    storage%readonly = dlite_storage_is_writable_c(storage%storage)
  end function dlite_storage_open

  function dlite_storage_close(storage) result(status)
    class(DLiteStorage), intent(in) :: storage
    integer(c_int)                  :: c_status
    integer                         :: status
    c_status = dlite_storage_close_c(storage%storage)
    status = c_status
  end function dlite_storage_close

  function dlite_storage_is_writable(storage) result(status)
    class(DLiteStorage), intent(in) :: storage
    integer(c_int)                  :: c_status
    integer                         :: status
    c_status = dlite_storage_is_writable_c(storage%storage)
    status = c_status
  end function dlite_storage_is_writable


  ! --------------------------------------------------------
  ! Fortran methods for DLiteInstance
  ! --------------------------------------------------------

  function dlite_instance_create_from_id(metaid, dims, id) result(instance)
    character(len=*), intent(in)          :: metaid
    integer(8), dimension(*), intent(in)  :: dims
    character(len=*), intent(in)          :: id
    character(len=1,kind=c_char)          :: metaid_c(len_trim(metaid)+1)
    character(len=1,kind=c_char)          :: id_c(len_trim(id)+1)
    type(DliteInstance)                   :: instance

    metaid_c = f_c_string_func(metaid)
    id_c = f_c_string_func(id)

    instance%cinst = dlite_instance_create_from_id_c(metaid_c, dims, id_c)
    !instance%uuid =
    !instance%uri =
    !instance%iri =
  end function dlite_instance_create_from_id

  function dlite_instance_load(storage, id) result(instance)
    type(DLiteStorage), intent(in)        :: storage
    character(len=*), intent(in)          :: id
    character(len=1,kind=c_char)          :: id_c(len_trim(id)+1)
    type(DliteInstance)                   :: instance

    id_c = f_c_string_func(id)
    instance%cinst = dlite_instance_load_c(storage%storage, id_c)
  end function dlite_instance_load

  function dlite_instance_load_url(url) result(instance)
    character(len=*), intent(in)          :: url
    character(len=1,kind=c_char)          :: url_c(len_trim(url)+1)
    type(DliteInstance)                   :: instance

    url_c = f_c_string_func(url)
    instance%cinst = dlite_instance_load_url_c(url_c)
  end function dlite_instance_load_url

  function dlite_instance_load_casted(storage, id, metaid) result(instance)
    type(DLiteStorage), intent(in)        :: storage
    character(len=*), intent(in)          :: id
    character(len=*), intent(in)          :: metaid
    character(len=1,kind=c_char)          :: id_c(len_trim(id)+1)
    character(len=1,kind=c_char)          :: metaid_c(len_trim(metaid)+1)
    type(DliteInstance)                   :: instance

    id_c = f_c_string_func(id)
    metaid_c = f_c_string_func(metaid)

    instance%cinst = &
         dlite_instance_load_casted_c(storage%storage, id_c, metaid_c)
  end function dlite_instance_load_casted

  function dlite_instance_save(instance, storage) result(status)
    class(DLiteInstance), intent(in) :: instance
    type(DLiteStorage), intent(in)   :: storage
    integer                          :: status
    status = dlite_instance_save_c(storage%storage, instance%cinst)
  end function dlite_instance_save

  function dlite_instance_save_url(instance, url) result(status)
    class(DLiteInstance), intent(in) :: instance
    character(len=*), intent(in)     :: url
    character(len=1,kind=c_char)     :: url_c(len_trim(url)+1)
    integer                          :: status
    url_c = f_c_string_func(url)
    status = dlite_instance_save_url_c(url_c, instance%cinst)
  end function dlite_instance_save_url

  function dlite_instance_get_uuid(instance) result(uuid)
    class(DLiteInstance), intent(in) :: instance
    character(len=36)                :: uuid

    ! FIXME - get UUID from instance
    !integer :: i, n
    integer :: n
    type(c_ptr) :: cptr
    !character(kind=c_char), dimension(:), pointer :: fptr
    cptr = dlite_instance_get_uuid_c(instance%cinst)
    n = c_string_length(cptr)
    uuid = 'xxx'
    !c_f_pointer(cptr, fptr, [n])
    !do i = 1, n
    !   uuid(i:i) = fptr(i:i)
    !end do
  end function dlite_instance_get_uuid

  function dlite_instance_get_dimension_size(instance, name) result(n)
    class(DLiteInstance), intent(in) :: instance
    character(len=*), intent(in)     :: name
    character(len=1,kind=c_char)     :: name_c(len_trim(name)+1)
    integer                          :: n
    integer(kind=c_size_t)           :: n_c
    ! FIXME - correct call to dlite_instance_get_dimension_size_c()
    name_c = f_c_string_func(name)
    n_c = dlite_instance_get_dimension_size_c(instance%cinst, name_c)
    n = int(n_c)
  end function dlite_instance_get_dimension_size

  function dlite_instance_get_dimension_size_by_index(instance, i) result(n)
    class(DLiteInstance), intent(in) :: instance
    integer, intent(in)              :: i
    integer(kind=c_size_t)           :: i_c
    integer                          :: n
    integer(kind=c_size_t)           :: n_c
    i_c = i
    n_c = dlite_instance_get_dimension_size_by_index_c(instance%cinst, i_c)
    n = int(n_c)
  end function dlite_instance_get_dimension_size_by_index

  function dlite_instance_get_property(instance, name) result(ptr)
    class(DLiteInstance), intent(in) :: instance
    character(len=*), intent(in)     :: name
    character(len=1,kind=c_char)     :: name_c(len_trim(name)+1)
    type(c_ptr)                      :: ptr
    ! FIXME - correct call to dlite_instance_get_property_c()
    name_c = f_c_string_func(name)
    ptr = dlite_instance_get_property_c(instance%cinst, name_c)
  end function dlite_instance_get_property

  function dlite_instance_get_property_by_index(instance, i) result(ptr)
    class(DLiteInstance), intent(in) :: instance
    integer, intent(in)              :: i
    integer(kind=c_size_t)           :: i_c
    type(c_ptr)                      :: ptr
    i_c = i
    ptr = dlite_instance_get_property_by_index_c(instance%cinst, i_c)
  end function dlite_instance_get_property_by_index

end module DLite
