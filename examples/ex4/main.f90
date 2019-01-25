program FTest
  use iso_c_binding, only : c_ptr
  use DLite
  !use Chemistry

  implicit none

  type(c_ptr) :: storage, chem_c
  integer :: sta
  !type(TChemistryDims) :: dims

  ! load a chemistry instance
  storage =  dlite_storage_open("json", "example-AlMgSi.json", "mode=r")
  sta = dlite_storage_is_writable(storage)
  write(*,*) "ftest.f90 is_writable answer:", sta

  !chem_c = chemistry_load(storage, "0dcd4925-4844-53ea-ae3d-a5f1cc0270ff")

  sta = dlite_storage_close(storage)
  write(*,*) "ftest.f90 close status:", sta

  !dims = chemistry_get_dims(chem_c)
  !write(*,*) "nelements =", dims%nelements
  !write(*,*) "nphases =", dims%nphases

  ! save the same chemistry instance in a new file
  !storage = dlite_storage_open("hdf5", "example2-6xxx.h5", "w")
  !sta = chemistry_save(chem_c, storage)
  !write(*,*) "ftest.f90 chemistry_save status:", sta
  !sta = dlite_storage_close(storage)
  !call chemistry_free(chem_c)

end program FTest