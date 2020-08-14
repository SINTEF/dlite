!
! Fortran interface to DLite
!
module DLite
  use iso_c_binding, only : c_ptr, c_int, c_char, c_null_char

  implicit none
  private

  public :: DLiteStorage

  type :: DLiteStorage
     type(c_ptr) :: storage
     integer     :: readonly
   contains
     procedure :: open => dlite_storage_open
     procedure :: close => dlite_storage_close
  end type DLiteStorage

  interface

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
       type(c_ptr), value :: storage
     end function dlite_storage_close_c

     integer(c_int) &
     function dlite_storage_is_writable_c(storage) &
         bind(C,name="dlite_storage_is_writable")
       import c_ptr, c_int
       type(c_ptr), value :: storage
     end function dlite_storage_is_writable_c

  end interface


!  INTERFACE

!     TYPE(c_ptr) FUNCTION dlite_storage_open(driver, uri, options) BIND(C)
!       IMPORT :: c_ptr
!       TYPE(c_ptr), VALUE :: driver
!       TYPE(c_ptr), VALUE :: uri
!       TYPE(c_ptr), VALUE :: options
!     END FUNCTION dlite_storage_open

!     INTEGER(c_int) FUNCTION dlite_storage_close(storage) BIND(C)
!       IMPORT :: c_ptr, c_int
!       TYPE(c_ptr), VALUE :: storage
!     END FUNCTION dlite_storage_close


!  END INTERFACE

contains

  function f_c_string_func (f_string) result (c_string)
    character(len=*), intent(in) :: f_string
    character(len=1,kind=c_char) :: c_string(len_trim(f_string)+1)
    integer                      :: n, i

    n = len_trim(f_string)
    do i = 1, n
      c_string(i) = f_string(i:i)
    end do
    c_string(n + 1) = c_null_char

  end function f_c_string_func
  
  DliteStorage function dlite_storage_open(driver, uri, options)
    character(len=*), intent(in) :: driver
    character(len=*), intent(in) :: uri
    character(len=*), intent(in) :: options
    character(len=1,kind=c_char) :: driver_c(len_trim(driver)+1)
    character(len=1,kind=c_char) :: uri_c(len_trim(uri)+1)
    character(len=1,kind=c_char) :: options_c(len_trim(options)+1)
    DliteStorage                 :: storage

    driver_c = f_c_string_func(driver)
    uri_c = f_c_string_func(uri)
    options_c = f_c_string_func(options)

    storage%storage = dlite_storage_open_c(driver_c, uri_c, options_c)
    storage%readonly = dlite_storage_is_writable_c(storage%storage)

    dlite_storage_open = storage

  end function dlite_storage_open

  integer function dlite_storage_close(storage)
    type(c_ptr) :: storage
    integer(c_int) :: sta
    sta = dlite_storage_close_c(storage)
    dlite_storage_close = sta
  end function dlite_storage_close

  integer function dlite_storage_is_writable(storage)
    type(c_ptr) :: storage
    integer(c_int) :: sta
    sta = dlite_storage_is_writable_c(storage)
    dlite_storage_is_writable = sta
  end function dlite_storage_is_writable


end module DLite