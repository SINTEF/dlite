!
! Fortran interface to DLite
!
module DLite
  use iso_c_binding, only : c_ptr, c_int, c_size_t, c_char, c_null_char, &
                            c_associated, c_null_ptr, c_bool, c_float,   &
                            c_double, c_f_pointer, c_loc
  use c_interface, only: c_f_string, f_c_string

  implicit none
  private

  public :: dlite_instance_set_property_value

  type, public :: DLiteStorage
     type(c_ptr) :: cptr
     logical     :: readonly
   contains
     procedure :: close => dlite_storage_close
     procedure :: is_writable => dlite_storage_is_writable
  end type DLiteStorage

  interface DLiteStorage
     module procedure dlite_storage_open ! User-defined constructor
  end interface DLiteStorage

  interface dlite_instance_set_property_value

    module procedure set_property_value_string
    module procedure set_property_array_string

  end interface dlite_instance_set_property_value

  type, public :: DLiteInstance
     type(c_ptr)                   :: cinst
   contains
     procedure :: check => check_instance
     procedure :: save => dlite_instance_save
     procedure :: save_url => dlite_instance_save_url
     procedure :: get_uuid => dlite_instance_get_uuid
     procedure :: get_uri => dlite_instance_get_uri
     procedure :: get_dimension_size => dlite_instance_get_dimension_size
     procedure :: get_dimension_size_by_index => dlite_instance_get_dimension_size_by_index
     procedure :: get_property => dlite_instance_get_property
     procedure :: get_property_by_index => dlite_instance_get_property_by_index
     procedure :: get_property_dims_by_index => dlite_instance_get_property_dims_by_index
     procedure :: destroy => dlite_instance_decref
  end type DLiteInstance

  interface DLiteInstance
     module procedure dlite_instance_create_from_id
     module procedure dlite_instance_load
     module procedure dlite_instance_load_url
     module procedure dlite_instance_load_casted
  end interface DLiteInstance

  type, public :: DLiteMeta
    type(c_ptr) :: cptr
  contains
    procedure :: check => dlite_meta_check
    procedure :: has_dimension => dlite_meta_has_dimension
    procedure :: has_property => dlite_meta_has_property
    procedure :: destroy => dlite_meta_decref
  end type DLiteMeta

  interface DLiteMeta
     module procedure dlite_meta_create_from_metamodel
  end interface DLiteMeta

  type, public :: DLiteMetaModel
    type(c_ptr) :: cptr
  contains
    procedure :: add_string => dlite_metamodel_add_string
    procedure :: add_dimension => dlite_metamodel_add_dimension
    procedure :: add_property => dlite_metamodel_add_property
    procedure :: add_property_dim => dlite_metamodel_add_property_dim
    procedure :: destroy => dlite_metamodel_free
  end type DLiteMetaModel

  interface DLiteMetaModel
     module procedure dlite_metamodel_create
  end interface DLiteMetaModel


  ! --------------------------------------------------------
  ! C interface for DLite
  ! --------------------------------------------------------
  interface
  ! --------------------------------------------------------
  ! C interface to C standard library
  ! --------------------------------------------------------
  subroutine free_c(ptr) &
    bind(C,name="free")
    import c_ptr
    type(c_ptr), value, intent(in) :: ptr
  end subroutine free_c
  ! --------------------------------------------------------
  ! C interface for DLiteStorage
  ! --------------------------------------------------------
  type(c_ptr) function dlite_storage_open_c(driver, uri, options) &
    bind(C,name="dlite_storage_open")
    import c_ptr, c_char
    character(len=1,kind=c_char), dimension(*), intent(in) :: driver
    character(len=1,kind=c_char), dimension(*), intent(in) :: uri
    character(len=1,kind=c_char), dimension(*), intent(in) :: options
  end function dlite_storage_open_c

  integer(c_int) function dlite_storage_close_c(storage) &
    bind(C,name="dlite_storage_close")
    import c_ptr, c_int
    type(c_ptr), value, intent(in) :: storage
  end function dlite_storage_close_c

  integer(c_int) function dlite_storage_is_writable_c(storage) &
    bind(C,name="dlite_storage_is_writable")
    import c_ptr, c_int
    type(c_ptr), value, intent(in) :: storage
  end function dlite_storage_is_writable_c

  ! --------------------------------------------------------
  ! C interface for DLiteInstance
  ! --------------------------------------------------------
  type(c_ptr) function dlite_instance_create_from_id_c(metaid, dims, id) &
      bind(C,name="dlite_instance_create_from_id")
    import c_ptr, c_size_t, c_char
    character(len=1,kind=c_char), dimension(*), intent(in) :: metaid
    integer(c_size_t), dimension(*), intent(in)            :: dims
    character(len=1,kind=c_char), dimension(*), intent(in) :: id
  end function dlite_instance_create_from_id_c

  type(c_ptr) function dlite_instance_load_c(storage, id) &
      bind(C,name="dlite_instance_load")
    import c_ptr, c_char
    type(c_ptr), value, intent(in)                         :: storage
    character(len=1,kind=c_char), dimension(*), intent(in) :: id
  end function dlite_instance_load_c

  type(c_ptr) function dlite_instance_load_url_c(url) &
      bind(C,name="dlite_instance_load_url")
    import c_ptr, c_char
    character(len=1,kind=c_char), dimension(*), intent(in) :: url
  end function dlite_instance_load_url_c

  type(c_ptr) function dlite_instance_load_casted_c(storage, id, metaid) &
      bind(C,name="dlite_instance_load_casted")
    import c_ptr, c_char
    type(c_ptr), value, intent(in)                         :: storage
    character(len=1,kind=c_char), dimension(*), intent(in) :: id
    character(len=1,kind=c_char), dimension(*), intent(in) :: metaid
  end function dlite_instance_load_casted_c

  integer(c_int) function dlite_instance_save_c(storage, instance) &
      bind(C,name="dlite_instance_save")
    import c_ptr, c_int
    type(c_ptr), value, intent(in)                         :: storage
    type(c_ptr), value, intent(in)                         :: instance
  end function dlite_instance_save_c

  integer(c_int) function dlite_instance_save_url_c(url, instance) &
      bind(C,name="dlite_instance_save_url")
    import c_ptr, c_char, c_int
    character(len=1,kind=c_char), dimension(*), intent(in) :: url
    type(c_ptr), value, intent(in)                         :: instance
  end function dlite_instance_save_url_c

  type(c_ptr) function dlite_instance_get_uuid_c(instance) &
      bind(C,name='dlite_instance_get_uuid')
    import c_ptr
    type(c_ptr), value, intent(in)                         :: instance
  end function dlite_instance_get_uuid_c

  type(c_ptr) function dlite_instance_get_uri_c(instance) &
      bind(C,name='dlite_instance_get_uri')
    import c_ptr
    type(c_ptr), value, intent(in)                         :: instance
  end function dlite_instance_get_uri_c

  integer(c_size_t) function dlite_instance_get_dimension_size_c(instance, name) &
      bind(C,name="dlite_instance_get_dimension_size")
    import c_ptr, c_char, c_size_t
    type(c_ptr), value, intent(in)                         :: instance
    character(len=1,kind=c_char), dimension(*), intent(in) :: name
  end function dlite_instance_get_dimension_size_c

  integer(c_size_t) function dlite_instance_get_dimension_size_by_index_c(instance, i) &
      bind(C,name="dlite_instance_get_dimension_size_by_index")
    import c_ptr, c_size_t
    type(c_ptr), value, intent(in)                         :: instance
    integer(c_size_t), value, intent(in)                   :: i
  end function dlite_instance_get_dimension_size_by_index_c

  type(c_ptr) function dlite_instance_get_property_c(instance, name) &
      bind(C,name="dlite_instance_get_property")
    import c_ptr, c_char
    type(c_ptr), value, intent(in)                         :: instance
    character(len=1,kind=c_char), dimension(*), intent(in) :: name
  end function dlite_instance_get_property_c

  type(c_ptr) function dlite_instance_get_property_by_index_c(instance, i) &
    bind(C,name="dlite_instance_get_property_by_index")
    import c_ptr, c_size_t
    type(c_ptr), value, intent(in)                         :: instance
    integer(c_size_t), value, intent(in)                   :: i
  end function dlite_instance_get_property_by_index_c

  integer(c_int) function dlite_instance_get_property_ndims_by_index_c(instance, i) &
    bind(C,name="dlite_instance_get_property_ndims_by_index")
    import c_ptr, c_size_t, c_int
    type(c_ptr), value, intent(in)                         :: instance
    integer(c_size_t), value, intent(in)                   :: i
  end function dlite_instance_get_property_ndims_by_index_c

  type(c_ptr) function dlite_instance_get_property_dims_by_index_c(instance, i) &
    bind(C,name="dlite_instance_get_property_dims_by_index")
    import c_ptr, c_size_t, c_int
    type(c_ptr), value, intent(in)                         :: instance
    integer(c_size_t), value, intent(in)                   :: i
  end function dlite_instance_get_property_dims_by_index_c

  ! int dlite_instance_decref(DLiteInstance *inst)
  integer(c_int) function dlite_instance_decref_c(instance) &
    bind(C,name="dlite_instance_decref")
    import c_ptr, c_int
    type(c_ptr), value, intent(in)                         :: instance
  end function dlite_instance_decref_c

  ! int dlite_meta_decref(DLiteMeta *meta)
  integer(c_int) function dlite_meta_decref_c(meta) &
    bind(C,name="dlite_meta_decref")
    import c_ptr, c_int
    type(c_ptr), value, intent(in)                         :: meta
  end function dlite_meta_decref_c

  ! --------------------------------------------------------
  ! C interface for DLiteMetaModel
  ! --------------------------------------------------------
  !
  ! DLiteMetaModel *dlite_metamodel_create(const char *uri,
  !                                        const char *metaid);
  type(c_ptr) function dlite_metamodel_create_c(uri, metaid) &
    bind(C,name="dlite_metamodel_create")
    import c_char, c_ptr
    character(len=1,kind=c_char), dimension(*), intent(in) :: uri
    character(len=1,kind=c_char), dimension(*), intent(in) :: metaid
  end function dlite_metamodel_create_c

  ! void dlite_metamodel_free(DLiteMetaModel *model);
  subroutine dlite_metamodel_free_c(model) &
    bind(C,name="dlite_metamodel_free")
    import c_ptr
    type(c_ptr), value, intent(in) :: model
  end subroutine dlite_metamodel_free_c

  ! int dlite_metamodel_add_string(DLiteMetaModel *model, const char *name,
  !                                const void *value);
  integer(c_int) function dlite_metamodel_add_string_c(model, name, value) &
    bind(C,name="dlite_metamodel_add_string")
    import c_int, c_ptr, c_char
    type(c_ptr), value, intent(in)                         :: model
    character(len=1,kind=c_char), dimension(*), intent(in) :: name
    character(len=1,kind=c_char), dimension(*), intent(in) :: value
  end function dlite_metamodel_add_string_c

  ! int dlite_metamodel_add_dimension(DLiteMetaModel *model,
  !                                   const char *name,
  !                                   const char *description);
  integer(c_int) function dlite_metamodel_add_dimension_c(model, name, description) &
    bind(C,name="dlite_metamodel_add_dimension")
    import c_int, c_ptr, c_char
    type(c_ptr), value, intent(in)                         :: model
    character(len=1,kind=c_char), dimension(*), intent(in) :: name
    character(len=1,kind=c_char), dimension(*), intent(in) :: description
  end function dlite_metamodel_add_dimension_c

  ! int dlite_metamodel_add_property(DLiteMetaModel *model,
  !                                  const char *name,
  !                                  const char *typename,
  !                                  const char *unit,
  !                                  const char *description);
  integer(c_int) function dlite_metamodel_add_property_c(model, name, typename, unit, description) &
    bind(C,name="dlite_metamodel_add_property")
    import c_int, c_ptr, c_char
    type(c_ptr), value, intent(in)                         :: model
    character(len=1,kind=c_char), dimension(*), intent(in) :: name
    character(len=1,kind=c_char), dimension(*), intent(in) :: typename
    character(len=1,kind=c_char), dimension(*), intent(in) :: unit
    character(len=1,kind=c_char), dimension(*), intent(in) :: description
  end function dlite_metamodel_add_property_c

  ! int dlite_metamodel_add_property_dim(DLiteMetaModel *model,
  !                                      const char *name,
  !                                      const char *expr);
  integer(c_int) function dlite_metamodel_add_property_dim_c(model, name, expr) &
    bind(C,name="dlite_metamodel_add_property_dim")
    import c_int, c_ptr, c_char
    type(c_ptr), value, intent(in)                         :: model
    character(len=1,kind=c_char), dimension(*), intent(in) :: name
    character(len=1,kind=c_char), dimension(*), intent(in) :: expr
  end function dlite_metamodel_add_property_dim_c

  ! --------------------------------------------------------
  ! C interface for DLiteMeta
  ! --------------------------------------------------------

  ! DLiteMeta *dlite_meta_create_from_metamodel(DLiteMetaModel *model);
  type(c_ptr) function dlite_meta_create_from_metamodel_c(model) &
    bind(C,name="dlite_meta_create_from_metamodel")
    import c_ptr
    type(c_ptr), value, intent(in)                         :: model
  end function dlite_meta_create_from_metamodel_c

  ! bool dlite_meta_has_dimension(DLiteMeta *meta, const char *name);
  logical(c_bool) function dlite_meta_has_dimension_c(meta, name) &
    bind(C,name="dlite_meta_has_dimension")
    import c_ptr, c_bool, c_char
    type(c_ptr), value, intent(in)                         :: meta
    character(len=1,kind=c_char), dimension(*), intent(in) :: name
  end function dlite_meta_has_dimension_c

  ! bool dlite_meta_has_property(DLiteMeta *meta, const char *name);
  logical(c_bool) function dlite_meta_has_property_c(meta, name) &
    bind(C,name="dlite_meta_has_property")
    import c_ptr, c_bool, c_char
    type(c_ptr), value, intent(in)                         :: meta
    character(len=1,kind=c_char), dimension(*), intent(in) :: name
  end function dlite_meta_has_property_c

  ! End C interface for DLite
  end interface

contains

  ! --------------------------------------------------------
  ! Fortran methods for DLiteStorage
  ! --------------------------------------------------------

  function check_instance(instance) result(status)
    implicit none
    class(DLiteInstance), intent(in) :: instance
    logical                          :: status
    status = c_associated(instance%cinst)
  end function check_instance

  ! --------------------------------------------------------
  ! Fortran methods for DLiteStorage
  ! --------------------------------------------------------

  function dlite_storage_open(driver, uri, options) result(storage)
    character(len=*), intent(in) :: driver
    character(len=*), intent(in) :: uri
    character(len=*), intent(in) :: options
    character(len=1,kind=c_char) :: driver_c(len_trim(driver)+1)
    character(len=1,kind=c_char) :: uri_c(len_trim(uri)+1)
    character(len=1,kind=c_char) :: options_c(len_trim(options)+1)
    type(DliteStorage)           :: storage

    call f_c_string(driver, driver_c)
    call f_c_string(uri, uri_c)
    call f_c_string(options, options_c)

    storage%cptr = dlite_storage_open_c(driver_c, uri_c, options_c)
    if (c_associated(storage%cptr)) then
      if (dlite_storage_is_writable(storage) .eq. 0) then
        storage%readonly = .true.
      else
        storage%readonly = .false.
      end if
    end if
  end function dlite_storage_open

  function dlite_storage_close(storage) result(status)
    class(DLiteStorage), intent(in) :: storage
    integer(c_int)                  :: c_status
    integer                         :: status
    c_status = dlite_storage_close_c(storage%cptr)
    status = c_status
  end function dlite_storage_close

  function dlite_storage_is_writable(storage) result(status)
    class(DLiteStorage), intent(in) :: storage
    integer(c_int)                  :: c_status
    integer                         :: status
    c_status = dlite_storage_is_writable_c(storage%cptr)
    status = c_status
  end function dlite_storage_is_writable


  ! --------------------------------------------------------
  ! Fortran methods for DLiteInstance
  ! --------------------------------------------------------

  function dlite_instance_create_from_id(metaid, ndim, dims, id) result(instance)
    character(len=*), intent(in)          :: metaid
    integer(8), dimension(*), intent(in)  :: dims
    integer, intent(in)                   :: ndim
    character(len=*), intent(in)          :: id
    character(len=1,kind=c_char)          :: metaid_c(len_trim(metaid)+1)
    character(len=1,kind=c_char)          :: id_c(len_trim(id)+1)
    type(DliteInstance)                   :: instance
    integer(c_size_t), allocatable, target:: dims1(:)
    integer(c_size_t), pointer            :: dims1_p(:)
    type(c_ptr)                           :: cptr
    integer                               :: i

    ! copy metaid in a C string
    call f_c_string(metaid, metaid_c)

    ! copy shape in a C pointer (size_t*)
    allocate(dims1(ndim))
    do i=1, ndim
       dims1(i) = dims(i)
    end do
    dims1_p => dims1
    cptr = c_loc(dims1_p)

    ! check if uuid is empty or not
    if (len_trim(id) == 36) then
      ! copy uuid in a C string
      call f_c_string(id, id_c)
      instance%cinst = dlite_instance_create_from_id_c(metaid_c, dims1, id_c)
    else
      instance%cinst = dlite_instance_create_from_id_c(metaid_c, dims1, c_null_char)
    endif
    !instance%uuid =
    !instance%uri =
  end function dlite_instance_create_from_id

  function dlite_instance_load(storage, id) result(instance)
    type(DLiteStorage), intent(in)        :: storage
    character(len=*), intent(in)          :: id
    character(len=1,kind=c_char)          :: id_c(len_trim(id)+1)
    type(DliteInstance)                   :: instance

    call f_c_string(id, id_c)
    instance%cinst = dlite_instance_load_c(storage%cptr, id_c)
  end function dlite_instance_load

  function dlite_instance_load_url(url) result(instance)
    character(len=*), intent(in)          :: url
    character(len=1,kind=c_char)          :: url_c(len_trim(url)+1)
    type(DliteInstance)                   :: instance

    call f_c_string(url, url_c)
    instance%cinst = dlite_instance_load_url_c(url_c)
  end function dlite_instance_load_url

  function dlite_instance_load_casted(storage, id, metaid) result(instance)
    type(DLiteStorage), intent(in)        :: storage
    character(len=*), intent(in)          :: id
    character(len=*), intent(in)          :: metaid
    character(len=1,kind=c_char)          :: id_c(len_trim(id)+1)
    character(len=1,kind=c_char)          :: metaid_c(len_trim(metaid)+1)
    type(DliteInstance)                   :: instance

    call f_c_string(metaid, metaid_c)
    call f_c_string(id, id_c)
    instance%cinst = &
        dlite_instance_load_casted_c(storage%cptr, id_c, metaid_c)
  end function dlite_instance_load_casted

  function dlite_instance_save(instance, storage) result(status)
    class(DLiteInstance), intent(in) :: instance
    type(DLiteStorage), intent(in)   :: storage
    integer                          :: status
    status = dlite_instance_save_c(storage%cptr, instance%cinst)
  end function dlite_instance_save

  function dlite_instance_save_url(instance, url) result(status)
    class(DLiteInstance), intent(in) :: instance
    character(len=*), intent(in)     :: url
    character(len=1,kind=c_char)     :: url_c(len_trim(url)+1)
    integer                          :: status
    call f_c_string(url, url_c)
    status = dlite_instance_save_url_c(url_c, instance%cinst)
  end function dlite_instance_save_url

  function dlite_instance_get_uuid(instance) result(uuid)
    class(DLiteInstance), intent(in) :: instance
    character(len=36)                :: uuid
    type(c_ptr) :: cptr
    cptr = dlite_instance_get_uuid_c(instance%cinst)
    call c_f_string(cptr, uuid)
  end function dlite_instance_get_uuid

  function dlite_instance_get_uri(instance) result(uri)
    class(DLiteInstance), intent(in) :: instance
    character(len=256)               :: uri
    type(c_ptr) :: cptr
    cptr = dlite_instance_get_uri_c(instance%cinst)
    call c_f_string(cptr, uri)
  end function dlite_instance_get_uri

  function dlite_instance_get_dimension_size(instance, name) result(n)
    class(DLiteInstance), intent(in) :: instance
    character(len=*), intent(in)     :: name
    character(len=1,kind=c_char)     :: name_c(len_trim(name)+1)
    integer(8)                       :: n
    integer(kind=c_size_t)           :: n_c
    call f_c_string(name, name_c)
    n_c = dlite_instance_get_dimension_size_c(instance%cinst, name_c)
    n = n_c
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
    call f_c_string(name, name_c)
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

  subroutine dlite_instance_get_property_dims_by_index(instance, i, shape, reverse)
    class(DLiteInstance), intent(in)     :: instance
    integer, intent(in)                  :: i
    integer(kind=c_size_t)               :: i_c
    type(c_ptr)                          :: ptr
    integer(c_size_t), pointer           :: dims_p(:)
    integer(c_int)                       :: ndim_c
    integer(8)                           :: ndim, j
    integer(8), allocatable, intent(out) :: shape(:)
    logical, optional                    :: reverse
    i_c = i
    ndim_c = dlite_instance_get_property_ndims_by_index_c(instance%cinst, i_c)
    ndim = ndim_c
    ptr = dlite_instance_get_property_dims_by_index_c(instance%cinst, i_c)
    call c_f_pointer(ptr, dims_p, [ndim])
    allocate( shape(ndim))
    if (present(reverse)) then
      if (reverse .and. (ndim .gt. 1)) then
        do j = 1, ndim
          shape(j) = dims_p(ndim - j + 1)
        end do
      else
        shape = dims_p
      endif
    else
      shape = dims_p
    endif
    call free_c(ptr)
  end subroutine dlite_instance_get_property_dims_by_index

  function dlite_instance_decref(instance) result(count)
    class(DLiteInstance)                 :: instance
    integer(c_int)                       :: count_c
    integer                              :: count
    count_c = dlite_instance_decref_c(instance%cinst)
    count = count_c
    if (count .le. 0) then
      instance%cinst = c_null_ptr
      count = 0
    endif
  end function dlite_instance_decref

  function dlite_meta_decref(meta) result(count)
    class(DLiteMeta)             :: meta
    integer(c_int)               :: count_c
    integer                      :: count
    count_c = dlite_meta_decref_c(meta%cptr)
    count = count_c
    if (count .le. 0) then
      meta%cptr = c_null_ptr
      count = 0
    endif
  end function dlite_meta_decref

  ! --------------------------------------------------------
  ! Fortran methods for DLiteMetaModel
  ! --------------------------------------------------------

  function dlite_metamodel_create(uri, metaid) result(model)
    character(len=*), intent(in) :: uri
    character(len=*), intent(in) :: metaid
    character(len=1,kind=c_char) :: uri_c(len_trim(uri)+1)
    character(len=1,kind=c_char) :: metaid_c(len_trim(metaid)+1)
    type(DLiteMetaModel)         :: model

    call f_c_string(uri, uri_c)
    call f_c_string(metaid, metaid_c)
    model%cptr = dlite_metamodel_create_c(uri_c, metaid_c)
  end function dlite_metamodel_create

  subroutine dlite_metamodel_free(model)
    class(DLiteMetaModel) :: model
    call dlite_metamodel_free_c(model%cptr)
    model%cptr = c_null_ptr
  end subroutine dlite_metamodel_free

  function dlite_metamodel_add_string(model, name, value) result(status)
    class(DLiteMetaModel)        :: model
    character(len=*), intent(in) :: name
    character(len=*), intent(in) :: value
    character(len=1,kind=c_char) :: name_c(len_trim(name)+1)
    character(len=1,kind=c_char) :: value_c(len_trim(value)+1)
    integer(c_int)                  :: status_c
    integer                      :: status

    call f_c_string(name, name_c)
    call f_c_string(value, value_c)
    status_c = dlite_metamodel_add_string_c(model%cptr, name_c, value_c)
    status = status_c
  end function dlite_metamodel_add_string

  function dlite_metamodel_add_dimension(model, name, description) result(status)
    class(DLiteMetaModel)        :: model
    character(len=*), intent(in) :: name
    character(len=*), intent(in) :: description
    character(len=1,kind=c_char) :: name_c(len_trim(name)+1)
    character(len=1,kind=c_char) :: description_c(len_trim(description)+1)
    integer(c_int)                  :: status_c
    integer                      :: status

    call f_c_string(name, name_c)
    call f_c_string(description, description_c)
    status_c = dlite_metamodel_add_dimension_c(model%cptr, name_c, description_c)
    status = status_c
  end function dlite_metamodel_add_dimension

  function dlite_metamodel_add_property(model, name, typename, &
                                        unit, description) result(status)
    class(DLiteMetaModel)        :: model
    character(len=*), intent(in) :: name
    character(len=*), intent(in) :: typename
    character(len=*), intent(in) :: unit
    character(len=*), intent(in) :: description
    character(len=1,kind=c_char) :: name_c(len_trim(name)+1)
    character(len=1,kind=c_char) :: typename_c(len_trim(typename)+1)
    character(len=1,kind=c_char) :: unit_c(len_trim(unit)+1)
    character(len=1,kind=c_char) :: description_c(len_trim(description)+1)
    integer(c_int)                  :: status_c
    integer                      :: status
    call f_c_string(name, name_c)
    call f_c_string(typename, typename_c)
    call f_c_string(unit, unit_c)
    call f_c_string(description, description_c)
    status_c = dlite_metamodel_add_property_c(model%cptr, name_c, typename_c, &
                                              unit_c, description_c)
    status = status_c
  end function dlite_metamodel_add_property

  function dlite_metamodel_add_property_dim(model, name, expr) result(status)
    class(DLiteMetaModel)        :: model
    character(len=*), intent(in) :: name
    character(len=*), intent(in) :: expr
    character(len=1,kind=c_char) :: name_c(len_trim(name)+1)
    character(len=1,kind=c_char) :: expr_c(len_trim(expr)+1)
    integer(c_int)                  :: status_c
    integer                      :: status

    call f_c_string(name, name_c)
    call f_c_string(expr, expr_c)
    status_c = dlite_metamodel_add_property_dim_c(model%cptr, name_c, expr_c)
    status = status_c
  end function dlite_metamodel_add_property_dim

  ! --------------------------------------------------------
  ! Fortran methods for DLiteMeta
  ! --------------------------------------------------------

  function dlite_meta_create_from_metamodel(model) result(meta)
    class(DLiteMetaModel):: model
    type(DLiteMeta)      :: meta
    meta%cptr = dlite_meta_create_from_metamodel_c(model%cptr)
  end function dlite_meta_create_from_metamodel

  function dlite_meta_check(meta) result(status)
    class(DLiteMeta)             :: meta
    logical                      :: status
    status = c_associated(meta%cptr)
  end function dlite_meta_check

  function dlite_meta_has_dimension(meta, name) result(answer)
    class(DLiteMeta)             :: meta
    character(len=*), intent(in) :: name
    character(len=1,kind=c_char) :: name_c(len_trim(name)+1)
    logical(c_bool)              :: answer_c
    logical                      :: answer
    call f_c_string(name, name_c)
    answer_c = dlite_meta_has_dimension_c(meta%cptr, name_c)
    answer = answer_c
  end function dlite_meta_has_dimension

  function dlite_meta_has_property(meta, name) result(answer)
    class(DLiteMeta)             :: meta
    character(len=*), intent(in) :: name
    character(len=1,kind=c_char) :: name_c(len_trim(name)+1)
    logical(c_bool)              :: answer_c
    logical                      :: answer
    call f_c_string(name, name_c)
    answer_c = dlite_meta_has_property_c(meta%cptr, name_c)
    answer = answer_c
  end function dlite_meta_has_property

  subroutine set_property_value_string(inst, index, prop)
    type(DLiteInstance), intent(in)  :: inst
    integer, intent(in)              :: index
    character(len=*), intent(in)     :: prop

    call f_c_string(prop, inst%get_property_by_index(index))

  end subroutine set_property_value_string

  subroutine set_property_array_string(inst, index, prop)
    type(DLiteInstance), intent(in)        :: inst
    integer, intent(in)                    :: index
    character(*), dimension(*), intent(in) :: prop(:)
    type(c_ptr)                            :: cptr
    character(kind=c_char), pointer        :: prop_p(:,:)
    integer(8), dimension(*), allocatable  :: shape(:), shape2(:)
    integer(8)                             :: i

    call dlite_instance_get_property_dims_by_index(inst, index, shape)
    allocate(shape2(2))
    shape2(1) = len(prop(1))+1
    shape2(2) = shape(1)
    cptr = inst%get_property_by_index(index)
    call c_f_pointer(cptr, prop_p, shape2)
    do i = 1, shape(1)
      call f_c_string(prop(i), prop_p(:,i))
    end do

  end subroutine set_property_array_string

end module DLite
