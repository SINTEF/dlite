! Fortran interface to DLite

module DLite
  use iso_c_binding, only : c_ptr, c_int, c_char, c_null_char

  implicit none
  private

{public}

  interface

{bind_c}

  end interface

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

{call_c}

end module DLite
