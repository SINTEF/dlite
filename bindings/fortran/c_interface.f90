!
! file   : c_interface.f90
! purpose: copy string from C to Fortran and copy string from Fortran to C
! origin : http://fortranwiki.org/fortran/show/c_interface_module
!
module c_interface
    use iso_c_binding

    implicit none

    interface c_f_string
        module procedure c_f_string_ptr
        module procedure c_f_string_chars
    end interface f_c_string
    
    interface f_c_string
        module procedure f_c_string_ptr
        module procedure f_c_string_chars
    end interface f_c_string

    contains

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
        do while(c_string(i)/=NUL .and. i<=len(f_string))
          f_string(i:i) = f_string(i)
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
        if (.not. C_associated(C_string)) then
          return
        end if
        call C_F_pointer(C_string,p_chars,[strlen+1])
        forall (i=1:strlen)
          p_chars(i) = F_string(i:i)
        end forall
        p_chars(strlen+1) = NUL
      end subroutine F_C_string_ptr    
        
end module c_interface
