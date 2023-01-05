program FTest
  use iso_c_binding, only : c_ptr
  use DLite
  use Chemistry

  implicit none

  type(TChemistry) :: p
  integer(4) :: nelements = 4, nphases = 3
  integer :: status, i, j
  REAL(8) :: atvol0, tmp


  ! load a chemistry instance
  p = TChemistry(nelements, nphases)
  p%elements(1) = "Al"
  p%elements(2) = "Mg"
  p%elements(3) = "Si"
  p%elements(4) = "Fe"

  p%phases(1) = "FCC_A1"
  p%phases(2) = "MG2SI"
  p%phases(3) = "ALFESI_ALPHA"

  p%alloy = "Sample alloy..."

  p % X0(1) = 1.0
  p % X0(2) = 0.5e-2
  p % X0(3) = 0.5e-2
  p % X0(4) = 0.03e-2
  DO i = 2, nelements
     p % X0(1) = p % X0(1) - p % X0(i)
  END DO

  p % volfrac(1) = 0.98
  p % volfrac(2) = 0.01
  p % volfrac(3) = 0.01

  p % rpart(1) = 0.0
  p % rpart(2) = 1e-6
  p % rpart(3) = 10e-6

  p % atvol(1) = 16e-30
  p % atvol(2) = 24e-30
  p % atvol(3) = 20e-30

  p % Xp(2, 1) = 0.0
  p % Xp(2, 2) = 2.0 / 3.0
  p % Xp(2, 3) = 1.0 / 3.0
  p % Xp(2, 4) = 0.0

  p % Xp(3, 1) = 0.7
  p % Xp(3, 2) = 0.0
  p % Xp(3, 3) = 0.1
  p % Xp(3, 4) = 0.2
  p % Xp(1,:)  = p % X0(:)
  tmp = 0.0
  DO i = 2, nphases
     tmp = tmp + p % volfrac(i)/p  % atvol(i)
  END DO
  atvol0 = 1.0/SUM(p % volfrac(2:nphases)/p  % atvol(2:nphases))
  DO j = 2, nphases
     DO i = 1, nelements
        p % Xp(1,i) = p % Xp(1,i) - atvol0/p % atvol(j)*p % volfrac(j)*p % Xp(j, i)
     END DO
  END DO
  status = p%writeToSource("hdf5", "test.h5", "mode=w")




!  CHARACTER(len=255) :: dlite_root
!  CALL get_environment_variable("DLITE_ROOT", dlite_root)
!  storage = dlite_storage_open("json", TRIM(dlite_root) // "/share/dlite/examples/ex4/example-AlMgSi.json", "mode=r")
!   p = TChemistry("json",                                                              &
!        &                 TRIM(dlite_root) // "/share/dlite/examples/ex4/example-AlMgSi.json", &
!        &                 "mode=r",                                                            &
!        &                 "0dcd4925-4844-53ea-ae3d-a5f1cc0270ff"                               )

!  sta = dlite_storage_is_writable(storage)
!  write(*,*) "ftest.f90 is_writable answer:", sta

!  chem_c = p_load(storage, "0dcd4925-4844-53ea-ae3d-a5f1cc0270ff")

!  sta = dlite_storage_close(storage)
!  write(*,*) "ftest.f90 close status:", sta

!  dims = p_get_dims(chem_c)
!  write(*,*) "nelements =", dims%nelements
!  write(*,*) "nphases =", dims%nphases

!   save the same p instance in a new file
!  storage = dlite_storage_open("hdf5", "example2-6xxx.h5", "w")
!  sta = p_save(chem_c, storage)
!  write(*,*) "ftest.f90 p_save status:", sta
!  sta = dlite_storage_close(storage)
!  call p_free(chem_c)

end program FTest
