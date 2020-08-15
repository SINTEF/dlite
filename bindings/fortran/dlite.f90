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
     procedure :: get_dimension_size_by_index => dlite_instance_get_dimension_size_by_index
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

     integer(c_size_t) &
     function dlite_instance_get_dimension_size_by_index_c(instance, i) &
         bind(C,name="dlite_instance_get_dimension_size_by_index")
       import c_ptr, c_size_t
       type(c_ptr), value, intent(in)                         :: instance
       integer(c_size_t), value, intent(in)                   :: i
     end function dlite_instance_get_dimension_size_by_index_c

  end interface

contains
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

end module DLite
