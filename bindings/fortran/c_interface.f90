!
! file   : c_interface.f90
! purpose: copy string from C to Fortran and copy string from Fortran to C
! origin : http://fortranwiki.org/fortran/show/c_interface_module
!
module c_interface
  use iso_c_binding, only : c_ptr, c_int, c_size_t, c_char, c_null_char, &
  c_associated, c_f_pointer

  implicit none

  interface c_f_string
      module procedure c_f_string_ptr
      module procedure c_f_string_chars
  end interface c_f_string
  
  interface f_c_string
      module procedure f_c_string_ptr
      module procedure f_c_string_chars
  end interface f_c_string

  interface
  
  ! Return the length of S.
  ! extern size_t strlen (const char *s)
  function c_strlen(s) result(result) bind(C,name="strlen")
    import c_ptr, c_size_t
    integer(c_size_t) :: result
    type(c_ptr), value, intent(in) :: s
  end function c_strlen  

  end interface

  contains

  function c_strlen_safe(s) result(length)
    integer(c_size_t) :: length
    type(c_ptr), value, intent(in) :: s
    if (.not. c_associated(s)) then
      length = 0
    else
      length = c_strlen(s)
    end if
  end function c_strlen_safe

  ! Copy a C string, passed by pointer, to a Fortran string.
  ! If the C pointer is NULL, the Fortran string is blanked.
  ! C_string must be NUL terminated, or at least as long as F_string.
  ! If C_string is longer, it is truncated. Otherwise, F_string is
  ! blank-padded at the end.
  subroutine c_f_string_ptr(c_string, f_string)
    type(c_ptr), intent(in)                             :: c_string
    character(len=*), intent(out)                       :: f_string
    character(len=1,kind=c_char), dimension(:), pointer :: p_chars
    integer                                             :: i
    if (.not. c_associated(c_string)) then
      f_string = ' '
    else
      call c_f_pointer(c_string,p_chars,[huge(0)])
      i=1
      do while(p_chars(i)/=c_null_char .and. i<=len(f_string))
        f_string(i:i) = p_chars(i)
        i=i+1
      end do
      if (i<len(f_string)) f_string(i:) = ' '
    end if
  end subroutine c_f_string_ptr

  ! Copy a C string, passed as a char-array reference, to a Fortran string.
  subroutine c_f_string_chars(c_string, f_string)
    character(len=1,kind=c_char), intent(in) :: c_string(*)
    character(len=*), intent(out)            :: f_string
    integer                                  :: i
    i=1
    do while(c_string(i)/=c_null_char .and. i<=len(f_string))
      f_string(i:i) = c_string(i)
      i=i+1
    end do
    if (i<len(f_string)) f_string(i:) = ' '
  end subroutine c_f_string_chars

  ! Copy a Fortran string to an allocated C string pointer.
  ! If the C pointer is NULL, no action is taken. (Maybe auto allocate via libc call?)
  ! If the length is not passed, the C string must be at least: len(F_string)+1
  ! If the length is passed and F_string is too long, it is truncated.
  subroutine f_c_string_ptr(f_string, c_string, c_string_len)
    character(len=*), intent(in) :: F_string
    type(C_ptr), intent(in) :: C_string ! target = intent(out)
    integer, intent(in), optional :: C_string_len
    character(len=1,kind=C_char), dimension(:), pointer :: p_chars
    integer :: i, strlen
    strlen = len(F_string)
    if (present(C_string_len)) then
      if (C_string_len <= 0) return
      strlen = min(strlen,C_string_len-1)
    end if
    if (.not. c_associated(C_string)) then
      return
    end if
    call c_f_pointer(C_string,p_chars,[strlen+1])
    forall (i=1:strlen)
      p_chars(i) = F_string(i:i)
    end forall
    p_chars(strlen+1) = c_null_char
    end subroutine f_c_string_ptr
      
  ! Copy a Fortran string to a C string passed by char-array reference.
  ! If the length is not passed, the C string must be at least: len(F_string)+1
  ! If the length is passed and F_string is too long, it is truncated.
  subroutine f_c_string_chars(f_string, c_string, c_string_len)
    character(len=*), intent(in)                            :: f_string
    character(len=1,kind=C_char), dimension(*), intent(out) :: c_string
    ! Max string length, INCLUDING THE TERMINAL NULL CHAR
    integer, intent(in), optional                           :: c_string_len
    integer                                                 :: i
    integer                                                 :: strlen
    strlen = len(f_string)
    if (present(c_string_len)) then
      if (c_string_len <= 0) return
      strlen = min(strlen,c_string_len-1)
    end if
    forall (i=1:strlen)
      c_string(i) = f_string(i:i)
    end forall
    c_string(strlen+1) = c_null_char
  end subroutine f_c_string_chars      
        
end module c_interface
