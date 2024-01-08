! This example will create a DLite entity based on the metadata description in
! Chemistry-0.1.json. The empty instance will then be populated with values,
! and the instance is then stored to disk as a json-file
program FTest
  use iso_c_binding, only : c_ptr
  use DLite
  use Chemistry

  implicit none

  type(TChemistry) :: p
  integer(4) :: nelements = 4, nphases = 3
  integer :: status, i, j
  real(8) :: atvol0

!   This example creates an alloy with four elements (Aluminium, Manganese,
!   Silicon and Iron) with three different phases. The number of elements and
!   number of phases determines the size of the dimension in the DLite instance,
!   and is required to allocate the correct amount of memory in the constructor
!   of the instance

  ! load a chemistry entity instance
  p = TChemistry(nelements, nphases)

  ! Set name of elements
  p % elements(1) = "Al"
  p % elements(2) = "Mg"
  p % elements(3) = "Si"
  p % elements(4) = "Fe"

  ! Set name of phases
  p % phases(1) = "FCC_A1"
  p % phases(2) = "MG2SI"
  p % phases(3) = "ALFESI_ALPHA"

  ! Set alloy description
  p % alloy = "Sample alloy..."

  ! Set nominal composition and make sure it sums to 1.0
  p % X0(1) = 1.0
  p % X0(2) = 0.5e-2
  p % X0(3) = 0.5e-2
  p % X0(4) = 0.03e-2
  do i = 2, nelements
     p % X0(1) = p % X0(1) - p % X0(i)
  end do

  ! Set volume fraction of each phase, excluding matrix
  p % volfrac(1) = 0.98
  p % volfrac(2) = 0.01
  p % volfrac(3) = 0.01

  !Set average particle radius of each phase, excluding matrix
  p % rpart(1) = 0.0
  p % rpart(2) = 1e-6
  p % rpart(3) = 10e-6

  ! Set average volume per atom for each phase
  p % atvol(1) = 16e-30
  p % atvol(2) = 24e-30
  p % atvol(3) = 20e-30

  ! Set average composition for phase 2
  p % Xp(2, 1) = 0.0
  p % Xp(2, 2) = 2.0 / 3.0
  p % Xp(2, 3) = 1.0 / 3.0
  p % Xp(2, 4) = 0.0

  ! Set average composition for phase 3
  p % Xp(3, 1) = 0.7
  p % Xp(3, 2) = 0.0
  p % Xp(3, 3) = 0.1
  p % Xp(3, 4) = 0.2
  p % Xp(1, :) = p % X0(:)
  atvol0 = 1.0/SUM(p % volfrac(2:nphases)/p  % atvol(2:nphases))

  ! Calculate average composition for phase 1
  do j = 2, nphases
     do i = 1, nelements
        p % Xp(1,i) = p % Xp(1,i) - atvol0/p % atvol(j)*p % volfrac(j)*p % Xp(j, i)
     end do
  end do

  !Write the populated entity instance to the hdf5 file test.h5
  status = p%writeToSource("hdf5", "test.h5", "mode=w")

end program FTest
