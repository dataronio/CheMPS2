/*
   CheMPS2: a spin-adapted implementation of DMRG for ab initio quantum chemistry
   Copyright (C) 2013, 2014 Sebastian Wouters

   This program is free software; you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation; either version 2 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License along
   with this program; if not, write to the Free Software Foundation, Inc.,
   51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
*/

#ifndef FCI_CHEMPS2_H
#define FCI_CHEMPS2_H

#include "Hamiltonian.h"

namespace CheMPS2{
/** FCI class.
    \author Sebastian Wouters <sebastianwouters@gmail.com>
    \date November 6, 2014
    
    The FCI class performs the full configuration interaction ground state calculation in a given particle number, irrep, and spin projection sector of a given Hamiltonian. It also contains the functionality to calculate Green's functions.
*/
   class FCI{

      public:
         
         //! Constructor
         /** \param Ham The Hamiltonian matrix elements
             \param Nel_up The number of up (alpha) electrons
             \param Nel_down The number of down (beta) electrons
             \param TargetIrrep The targeted point group irrep
             \param maxMemWorkMB Maximum workspace size in MB to be used for matrix vector product (this does not include the FCI vectors as stored for example in GSDavidson!!)
             \param FCIverbose The FCI verbose level: 0 print nothing, 1 print start and solution, 2 print everything */
         FCI(CheMPS2::Hamiltonian * Ham, const unsigned int Nel_up, const unsigned int Nel_down, const int TargetIrrep, const double maxMemWorkMB=100.0, const int FCIverbose=2);
         
         //! Destructor
         virtual ~FCI();
         
//==========> Getters of basic information: all these variables are set by the constructor
         
         //! Getter for the number of orbitals
         /** \return The number of orbitals */
         unsigned int getL() const{ return L; }

         //! Getter for the number of up or alpha electrons
         /** \return The number of up or alpha electrons */
         unsigned int getNel_up() const{ return Nel_up; }

         //! Getter for the number of down or beta electrons
         /** \return The number of down or beta electrons */
         unsigned int getNel_down() const{ return Nel_down; }
         
         //! Getter for the number of variables in the vector " E_ij | FCI vector > " ; where irrep_center = I_i x I_j
         /** \param irrep_center The single electron excitation irrep I_i x I_j
             \return The number of variables in the corresponding vector */
         unsigned long long getVecLength(const int irrep_center) const{ return irrep_center_jumps[ irrep_center ][ NumIrreps ]; }
         
         //! Get the number of irreps
         /** \return The number of irreps */
         unsigned int getNumIrreps() const{ return NumIrreps; }
         
         //! Get the target irrep
         /** \return The target irrep */
         int getTargetIrrep() const{ return TargetIrrep; }
         
         //! Get the direct product of two irreps (Psi4's convention is used): product( irrep1 , irrep2 ) = irrep1 XOR irrep2.
         /** \param Irrep1 The first irrep
             \param Irrep2 The second irrep
             \return The direct product Irrep1 x Irrep2 */
         static int getIrrepProduct(const int Irrep1, const int Irrep2){ return Irrep1 ^ Irrep2; }
         
         //! Get an orbital irrep
         /** \param orb The orbital index
             \return The irrep of orbital orb */
         int getOrb2Irrep(const int orb) const{ return orb2irrep[ orb ]; }
         
         //! Function which returns the electron repulsion integral (in chemists notation: integral dr1 dr2 orb1(r1) * orb2(r1) * orb3(r2) * orb4(r2) / |r1-r2|)
         /** \param orb1 First orbital index (electron at position r1)
             \param orb2 Second orbital index (electron at position r1)
             \param orb3 Third orbital index (electron at position r2)
             \param orb4 Fourth orbital index (electron at position r2)
             \return The desired electron repulsion integral */
         double getERI(const int orb1, const int orb2, const int orb3, const int orb4) const{ return ERI[ orb1 + L * ( orb2 + L * ( orb3 + L * orb4 ) ) ]; }
         
         //! Function which returns the AUGMENTED one-body matrix elements ( G_{ij} = T_{ij} - 0.5 * sum_k ERI_{ikkj} ; see Chem. Phys. Lett. 111 (4-5), 315-321 (1984) )
         /** \param orb1 First orbital index
             \param orb2 Second orbital index
             \return The desired AUGMENTED one-body matrix element */
         double getGmat(const int orb1, const int orb2) const{ return Gmat[ orb1 + L * orb2 ]; }
         
         //! Function which returns the nuclear repulsion energy
         /** \return The nuclear repulsion energy */
         double getEconst() const{ return Econstant; }
         
//==========> The core routines for users
         
         //! Calculates the FCI ground state with Davidson's algorithm
         /** \param inoutput If inoutput!=NULL, vector with getVecLength(0) variables which contains the initial guess at the start, and on exit the solution of the FCI calculation
             \param DAVIDSON_NUM_VEC The maximum number of vectors to use in Davidson's algorithm; adjustable in case memory becomes an issue
             \return The ground state energy */
         double GSDavidson(double * inoutput=NULL, const int DAVIDSON_NUM_VEC=CheMPS2::HEFF_DAVIDSON_NUM_VEC) const;
         
         //! Return the global counter of the Slater determinant with the lowest energy
         /** \return The global counter of the Slater determinant with the lowest energy */
         unsigned long long LowestEnergyDeterminant() const;
         
         //! Construct the (spin-summed) 2-RDM of a FCI vector: Gamma^2(i,j,k,l) = sum_sigma,tau < a^+_i,sigma a^+j,tau a_l,tau a_k,sigma > = TwoRDM[ i + L * ( j + L * ( k + L * l ) ) ]
         /** \param vector The FCI vector of length getVecLength(0)
             \param TwoRDM To store the 2-RDM; needs to be of size getL()^4; point group symmetry shows in 2-RDM elements being zero
             \return The energy of the given FCI vector, calculated by contraction of the 2-RDM with Gmat and ERI */
         double Fill2RDM(double * vector, double * TwoRDM) const;
         
         //! Measure S(S+1) (spin squared)
         /** \param vector The FCI vector of length getVecLength(0)
             \return Measured value of S(S+1) */
         double CalcSpinSquared(double * vector) const;

         //! Fill a vector with random numbers in the interval [-1,1[; used when output for GSDavidson is desired but no specific input can be given
         /** \param vecLength The length of the vector; when used for GSDavidson it should be getVecLength(0)
             \param vec The vector to fill with random numbers */
         static void FillRandom(const unsigned long long vecLength, double * vec);
         
         //! Set the entries of a vector to zero
         /** \param vecLength The vector length
             \param vec The vector which has to be set to zero */
         static void ClearVector(const unsigned long long vecLength, double * vec);
         
//==========> Green's functions functionality
         
         //! Calculate the retarded Green's function (= addition + removal amplitude)
         /** \param omega The frequency value
             \param eta The regularization parameter (... + I*eta in the denominator)
             \param orb_alpha The first orbital index
             \param orb_beta The second orbital index
             \param isUp If true, the spin projection value of the second quantized operators is up, otherwise it will be down
             \param GSenergy The ground state energy returned by GSDavidson
             \param GSvector The ground state vector as calculated by GSDavidson
             \param Ham The Hamiltonian, which contains the matrix elements
             \param RePartGF If not NULL, on exit RePartGF[0] contains the real part of the retarded Green's function
             \param ImPartGF If not NULL, on exit ImPartGF[0] contains the imaginary part of the retarded Green's function */
         void RetardedGF(const double omega, const double eta, const unsigned int orb_alpha, const unsigned int orb_beta, const bool isUp, const double GSenergy, double * GSvector, CheMPS2::Hamiltonian * Ham, double * RePartGF, double * ImPartGF) const;
         
         //! Calculate the addition part of the retarded Green's function: <GSvector| a_{alpha, spin(isUp)} [ omega - Ham + GSenergy + I*eta ]^{-1} a^+_{beta, spin(isUp)} |GSvector>
         /** \param omega The frequency value
             \param eta The regularization parameter
             \param orb_alpha The first orbital index
             \param orb_beta The second orbital index
             \param isUp If true, the spin projection value of the second quantized operators is up, otherwise it will be down
             \param GSenergy The ground state energy returned by GSDavidson
             \param GSvector The ground state vector as calculated by GSDavidson
             \param Ham The Hamiltonian, which contains the matrix elements
             \param RePartGF If not NULL, on exit RePartGF[0] contains the real part of the addition part of the retarded Green's function
             \param ImPartGF If not NULL, on exit ImPartGF[0] contains the imaginary part of the addition part of the retarded Green's function
             \param TwoRDMreal If not NULL and RePartGF not NULL, on exit the 2-RDM of Re{ [ omega - Ham + GSenergy + I*eta ]^{-1} a^+_{beta, spin(isUp)} | GSvector > }
             \param TwoRDMimag If not NULL and ImPartGF not NULL, on exit the 2-RDM of Im{ [ omega - Ham + GSenergy + I*eta ]^{-1} a^+_{beta, spin(isUp)} | GSvector > } */
         void RetardedGF_addition(const double omega, const double eta, const unsigned int orb_alpha, const unsigned int orb_beta, const bool isUp, const double GSenergy, double * GSvector, CheMPS2::Hamiltonian * Ham, double * RePartGF, double * ImPartGF, double * TwoRDMreal, double * TwoRDMimag) const;
         
         //! Calculate the removal part of the retarded Green's function: <GSvector| a^+_{beta, spin(isUp)} [ omega + Ham - GSenergy + I*eta ]^{-1} a_{alpha, spin(isUp)} |GSvector>
         /** \param omega The frequency value
             \param eta The regularization parameter
             \param orb_alpha The first orbital index
             \param orb_beta The second orbital index
             \param isUp If true, the spin projection value of the second quantized operators is up, otherwise it will be down
             \param GSenergy The ground state energy returned by GSDavidson
             \param GSvector The ground state vector as calculated by GSDavidson
             \param Ham The Hamiltonian, which contains the matrix elements
             \param RePartGF If not NULL, on exit RePartGF[0] contains the real part of the removal part of the retarded Green's function
             \param ImPartGF If not NULL, on exit ImPartGF[0] contains the imaginary part of the removal part of the retarded Green's function
             \param TwoRDMreal If not NULL and RePartGF not NULL, on exit the 2-RDM of Re{ [ omega + Ham - GSenergy + I*eta ]^{-1} a_{alpha, spin(isUp)} | GSvector > }
             \param TwoRDMimag If not NULL and ImPartGF not NULL, on exit the 2-RDM of Im{ [ omega + Ham - GSenergy + I*eta ]^{-1} a_{alpha, spin(isUp)} | GSvector > } */
         void RetardedGF_removal(const double omega, const double eta, const unsigned int orb_alpha, const unsigned int orb_beta, const bool isUp, const double GSenergy, double * GSvector, CheMPS2::Hamiltonian * Ham, double * RePartGF, double * ImPartGF, double * TwoRDMreal, double * TwoRDMimag) const;
         
         //! Calculate the density response Green's function (= forward - backward propagating part)
         /** \param omega The frequency value
             \param eta The regularization parameter (... + I*eta in the denominator)
             \param orb_alpha The first orbital index
             \param orb_beta The second orbital index
             \param GSenergy The ground state energy returned by GSDavidson
             \param GSvector The ground state vector as calculated by GSDavidson
             \param RePartGF If not NULL, on exit RePartGF[0] contains the real part of the density response Green's function
             \param ImPartGF If not NULL, on exit ImPartGF[0] contains the imaginary part of the density response Green's function */
         void DensityResponseGF(const double omega, const double eta, const unsigned int orb_alpha, const unsigned int orb_beta, const double GSenergy, double * GSvector, double * RePartGF, double * ImPartGF) const;
         
         //! Calculate the forward propagating part of the density response Green's function: <GSvector| ( n_alpha - <GSvector| n_alpha |GSvector> ) [ omega - Ham + GSenergy + I*eta ]^{-1} ( n_beta - <GSvector| n_beta |GSvector> ) |GSvector>
         /** \param omega The frequency value
             \param eta The regularization parameter
             \param orb_alpha The first orbital index
             \param orb_beta The second orbital index
             \param GSenergy The ground state energy returned by GSDavidson
             \param GSvector The ground state vector as calculated by GSDavidson
             \param RePartGF If not NULL, on exit RePartGF[0] contains the real part of the forward propagating part of the density response Green's function
             \param ImPartGF If not NULL, on exit ImPartGF[0] contains the imaginary part of the forward propagating part of the density response Green's function
             \param TwoRDMreal If not NULL and RePartGF not NULL, on exit the 2-RDM of Re{ [ omega - Ham + GSenergy + I*eta ]^{-1} ( n_beta - <GSvector| n_beta |GSvector> ) |GSvector> }
             \param TwoRDMimag If not NULL and ImPartGF not NULL, on exit the 2-RDM of Im{ [ omega - Ham + GSenergy + I*eta ]^{-1} ( n_beta - <GSvector| n_beta |GSvector> ) |GSvector> } */
         void DensityResponseGF_forward(const double omega, const double eta, const unsigned int orb_alpha, const unsigned int orb_beta, const double GSenergy, double * GSvector, double * RePartGF, double * ImPartGF, double * TwoRDMreal, double * TwoRDMimag) const;
         
         //! Calculate the backward propagating part of the density response Green's function: <GSvector| ( n_beta - <GSvector| n_beta |GSvector> ) [ omega + Ham - GSenergy + I*eta ]^{-1} ( n_alpha - <GSvector| n_alpha |GSvector> ) |GSvector>
         /** \param omega The frequency value
             \param eta The regularization parameter
             \param orb_alpha The first orbital index
             \param orb_beta The second orbital index
             \param GSenergy The ground state energy returned by GSDavidson
             \param GSvector The ground state vector as calculated by GSDavidson
             \param RePartGF If not NULL, on exit RePartGF[0] contains the real part of the backward propagating part of the density response Green's function
             \param ImPartGF If not NULL, on exit ImPartGF[0] contains the imaginary part of the backward propagating part of the density response Green's function
             \param TwoRDMreal If not NULL and RePartGF not NULL, on exit the 2-RDM of Re{ [ omega + Ham - GSenergy + I*eta ]^{-1} ( n_alpha - <GSvector| n_alpha |GSvector> ) |GSvector> }
             \param TwoRDMimag If not NULL and ImPartGF not NULL, on exit the 2-RDM of Im{ [ omega + Ham - GSenergy + I*eta ]^{-1} ( n_alpha - <GSvector| n_alpha |GSvector> ) |GSvector> } */
         void DensityResponseGF_backward(const double omega, const double eta, const unsigned int orb_alpha, const unsigned int orb_beta, const double GSenergy, double * GSvector, double * RePartGF, double * ImPartGF, double * TwoRDMreal, double * TwoRDMimag) const;
         
         //! Calculate the solution of the equation ( alpha + beta * Hamiltonian + I * eta ) Solution = RHS with conjugate gradient
         /** \param alpha The real part of the scalar in the operator
             \param beta The real-valued prefactor of the Hamiltonian in the operator
             \param eta The imaginary part of the scalar in the operator
             \param RHS The real-valued right-hand side of the equation with length getVecLength(0)
             \param RealSol If not NULL, on exit this array of length getVecLength(0) contains the real part of the solution
             \param ImagSol If not NULL, on exit this array of length getVecLength(0) contains the imaginary part of the solution
             \param checkError If true, the RMS error without preconditioner will be calculated and printed after convergence */
         void CGSolveSystem(const double alpha, const double beta, const double eta, double * RHS, double * RealSol, double * ImagSol, const bool checkError=true) const;
         
         //void CheckHamDEBUG() const;
         
      protected:
      
//==========> Functions involving Hamiltonian matrix elements
         
         //! Function which returns the diagonal elements of the FCI Hamiltonian (without Econstant!!) = Slater determinant energies
         /** \param diag Vector with getVecLength(0) variables which contains on exit the diagonal elements of the FCI Hamiltonian */
         void DiagHam(double * diag) const;
         
         //! Function which returns the diagonal elements of the FCI Hamiltonian squared (without Econstant!!)
         /** \param output Vector with getVecLength(0) variables which contains on exit the diagonal elements of the FCI Hamiltonian squared */
         void DiagHamSquared(double * output) const;
      
         //! Function which performs the Hamiltonian times Vector product (without Econstant!!) making use of the (ij|kl) = (ji|kl) = (ij|lk) = (ji|lk) symmetry of the electron repulsion integrals
         /** \param input The vector of length getVecLength(0) on which the Hamiltonian should act
             \param output Vector of length getVecLength(0) which contains on exit the Hamiltonian times input */
         void HamTimesVec(double * input, double * output) const;
         
         //! Sandwich the Hamiltonian between two Slater determinants (return a specific element) (without Econstant!!)
         /** \param bits_bra_up Bit representation of the <bra| Slater determinant of the up (alpha) electrons (length L)
             \param bits_bra_down Bit representation of the <bra| Slater determinant of the down (beta) electrons (length L)
             \param bits_ket_up Bit representation of the |ket> Slater determinant of the up (alpha) electrons (length L)
             \param bits_ket_down Bit representation of the |ket> Slater determinant of the down (beta) electrons (length L)
             \param work Work array of length 8
             \return The FCI Hamiltonian element which connects the given two Slater determinants */
         double GetMatrixElement(int * bits_bra_up, int * bits_bra_down, int * bits_ket_up, int * bits_ket_down, int * work) const;
         
//==========> Basic conversions between bit string representations
      
         //! Function which returns a FCI coefficient
         /** \param bits_up The bit string representation of the up or alpha electron Slater determinant
             \param bits_down The bit string representation of the down or beta electron Slater determinant
             \param vector The FCI vector with getVecLength(0) variables from which a coefficient is desired
             \return The corresponding FCI coefficient; 0.0 if bits_up and bits_down do not form a valid FCI determinant */
         double getFCIcoeff(int * bits_up, int * bits_down, double * vector) const;
      
         //! Find the bit representation of a global counter corresponding to " E_ij | FCI vector > " ; where irrep_center = I_i x I_j
         /** \param irrep_center The single electron excitation irrep I_i x I_j
             \param counter The given global counter corresponding to " E_ij | FCI vector > "
             \param bits_up Array of length L to store the bit representation of the up (alpha) electrons in
             \param bits_down Array of length L to store the bit representation of the down (beta) electrons in */
         void getBitsOfCounter(const int irrep_center, const unsigned long long counter, int * bits_up, int * bits_down) const;
         
         //! Convertor between two representations of a same spin-projection Slater determinant
         /** \param Lvalue The number of orbitals
             \param bitstring The input integer, whos bits are the occupation numbers of the orbitals
             \param bits Contains on exit the Lvalue bits of bitstring */
         static void str2bits(const unsigned int Lvalue, const unsigned int bitstring, int * bits);
         
         //! Convertor between two representations of a same spin-projection Slater determinant
         /** \param Lvalue The number of orbitals
             \param bits Array with the Lvalue bits which should be combined to a single integer
             \return The single integer which represents the bits in bits */
         static unsigned int bits2str(const unsigned int Lvalue, int * bits);
         
         //! Find the irrep of the up Slater determinant of a global counter corresponding to " E_ij | FCI vector > " ; where irrep_center = I_i x I_j
         /** \param irrep_center The single electron excitation irrep I_i x I_j
             \param counter The given global counter corresponding to " E_ij | FCI vector > "
             \return The corresponding irrep of the up Slater determinant */
         int getUpIrrepOfCounter(const int irrep_center, const unsigned long long counter) const;
         
//==========> Some lapack like routines which work with unsigned long long's (not necessary anymore as StartupIrrepCenter() checks "assert( max_integer >= maxVecLength );" )

         //! Take the inproduct of two vectors
         /** \param vecLength The vector length
             \param vec1 The first vector
             \param vec2 The second vector
             \return The inproduct < vec1 | vec2 > */
         static double FCIddot(const unsigned long long vecLength, double * vec1, double * vec2);
         
         //! Copy a vector
         /** \param vecLength The vector length
             \param origin Vector to be copied
             \param target Where to copy the vector to */
         static void FCIdcopy(const unsigned long long vecLength, double * origin, double * target);
         
         //! Calculate the 2-norm of a vector
         /** \param vecLength The vector length
             \param vec The vector
             \return The 2-norm of vec */
         static double FCIfrobeniusnorm(const unsigned long long vecLength, double * vec);
         
         //! Do lapack's daxpy vec_y += alpha * vec_x
         /** \param vecLength The vector length
             \param alpha The scalar factor
             \param vec_x The vector which has to be added to vec_y in rescaled form
             \param vec_y The target vector */
         static void FCIdaxpy(const unsigned long long vecLength, const double alpha, double * vec_x, double * vec_y);
         
         //! Do lapack's dscal vec *= alpha
         /** \param vecLength The vector length
             \param alpha The scalar factor
             \param vec The vector which has to be rescaled */
         static void FCIdscal(const unsigned long long vecLength, const double alpha, double * vec);
         
//==========> Protected functions regarding the Green's functions
         
         //! Set thisVector to a creator/annihilator acting on otherVector
         /** \param whichOperator With which operator should be acted on the other FCI state: C means creator and A means annihilator
             \param isUp Boolean which denotes if the operator corresponds to an up (alpha) or down (beta) electron
             \param orbIndex Orbital index on which the operator acts
             \param thisVector Vector with length getVecLength(0) where the result of the operation should be stored
             \param otherFCI FCI instance which corresponds to the FCI vector otherVector on which is acted
             \param otherVector Vector with length otherFCI->getVecLength(0) which contains the FCI vector on which is acted */
         void ActWithSecondQuantizedOperator(const char whichOperator, const bool isUp, const unsigned int orbIndex, double * thisVector, const FCI * otherFCI, double * otherVector) const;
         
         //! Set resultVector to the number operator of a specific site acting on sourceVector
         /** \param orbIndex Orbital index of the number operator
             \param resultVector Vector with length getVecLength(0) where the result of the operation should be stored
             \param sourceVector Vector with length getVecLength(0) on which the number operator acts */
         void ActWithNumberOperator(const unsigned int orbIndex, double * resultVector, double * sourceVector) const;

         //! Calculate the solution of the system Operator |Sol> = |RESID> with Operator = precon * [ ( alpha + beta * H )^2 + eta^2 ] * precon
         /** \param alpha The parameter alpha of the operator
             \param beta The parameter beta of the operator
             \param eta The parameter eta of the operator
             \param precon The diagonal preconditioner
             \param Sol On entry this array of size getVecLength(0) contains the initial guess; on exit the solution is stored here
             \param RESID On entry the RHS of the equation is stored in this array of size getVecLength(0), on exit it is overwritten
             \param PVEC Workspace of size getVecLength(0)
             \param OxPVEC Workspace of size getVecLength(0)
             \param temp Workspace of size getVecLength(0)
             \param temp2 Workspace of size getVecLength(0) */
         void CGCoreSolver(const double alpha, const double beta, const double eta, double * precon, double * Sol, double * RESID, double * PVEC, double * OxPVEC, double * temp, double * temp2) const;

         //! Calculate out = (alpha + beta * Hamiltonian) * in (Econstant is taken into account!!)
         /** \param alpha The parameter alpha of the operator
             \param beta The parameter beta of the operator
             \param in On entry the vector of size getVecLength(0) on which the operator should be applied; unchanged on exit
             \param out Array of size getVecLength(0), which contains on exit (alpha + beta * Hamiltonian) * in */
         void CGAlphaPlusBetaHAM(const double alpha, const double beta, double * in, double * out) const;
         
         //! Calculate out = precon * [(alpha + beta * Hamiltonian)^2 + eta^2] * precon * in (Econstant is taken into account!!)
         /** \param alpha The parameter alpha of the operator
             \param beta The parameter beta of the operator
             \param eta The parameter eta of the operator
             \param precon The diagonal preconditioner
             \param in On entry the vector of size getVecLength(0) on which the operator should be applied; unchanged on exit
             \param out Array of size getVecLength(0), which contains on exit precon * [(alpha + beta * Hamiltonian)^2 + eta^2] * precon * in
             \param temp Workspace of size getVecLength(0)
             \param temp2 Workspace of size getVecLength(0) */
         void CGOperator(const double alpha, const double beta, const double eta, double * precon, double * in, double * temp, double * temp2, double * out) const;
         
         //! Calculate (without approximation) precon = (diag[(alpha + beta * Hamiltonian)^2 + eta^2])^{ -1/2 } (Econstant is taken into account!!)
         /** \param alpha The parameter alpha of the operator
             \param beta The parameter beta of the operator
             \param eta The parameter eta of the operator
             \param precon Array of size getVecLength(0), which contains on exit (diag[(alpha + beta * Hamiltonian)^2 + eta^2])^{ -1/2 }
             \param workspace Workspace of size getVecLength(0) */
         void CGDiagPrecond(const double alpha, const double beta, const double eta, double * precon, double * workspace) const;
      
      private:
      
         //! The FCI verbose level: 0 print nothing, 1 print start and solution, 2 print everything
         int FCIverbose;
         
         //! The maximum number of MB which can be used to store both HXVworkbig1 and HXVworkbig2
         double maxMemWorkMB;
         
         //! The constant term of the Hamiltonian
         double Econstant;
         
         //! The AUGMENTED one-body matrix elements Gmat[ i + L * j ] = T[ i + L * j ] - 0.5 * sum_k getERI(i, k, k, j)
         double * Gmat;
         
         //! The electron repulsion integrals (in chemists notation: \int dr1 dr2 orb1(r1) * orb2(r1) * orb3(r2) * orb4(r2) / |r1-r2| = ERI[ orb1 + L * ( orb2 + L * ( orb3 + L * orb4 ) ) ])
         double * ERI;
         
         //! The number of irreps of the molecule's Abelian point group with real-valued character table
         unsigned int NumIrreps;
         
         //! The targeted irrep of the total FCI wavefunction ( 0 <= TargetIrrep < NumIrreps )
         int TargetIrrep;
         
         //! Array of length L which contains for each orbital the irrep ( 0 <= orb < L : 0 <= orb2irrep[ orb ] < NumIrreps )
         int * orb2irrep;
         
         //! The number of orbitals ( Hubbard type orbitals with 4 possible occupations each )
         unsigned int L;
         
         //! The number of up (alpha) electrons
         unsigned int Nel_up;
         
         //! The number of down (beta) electrons
         unsigned int Nel_down;
         
         //! The number of up (alpha) Slater determinants with irrep "irrep" and Nel_up electrons is given by numPerIrrep_up[ irrep ]
         unsigned int * numPerIrrep_up;
         
         //! The number of down (beta) Slater determinants with irrep "irrep" and Nel_down electrons is given by numPerIrrep_down[ irrep ]
         unsigned int * numPerIrrep_down;
         
         //! For irrep "irrep" and bit string "bitstring", str2cnt_up[ irrep ][ bitstring ] returns the counter (0 <= counter < numPerIrrep_up[ irrep ]) of the up (alpha) Slater determinant within the irrep block (or -1 if invalid)
         int ** str2cnt_up;
         
         //! For irrep "irrep" and bit string "bitstring", str2cnt_down[ irrep ][ bitstring ] returns the counter (0 <= counter < numPerIrrep_down[ irrep ]) of the down (beta) Slater determinant within the irrep block (or -1 if invalid)
         int ** str2cnt_down;
         
         //! For irrep "irrep" and counter of the up (alpha) Slater determinant "counter" (0 <= counter < numPerIrrep_up[ irrep ]) cnt2str_up[ irrep ][ counter ] returns the bitstring representation of the corresponding up (alpha) Slater determinant
         unsigned int ** cnt2str_up;
         
         //! For irrep "irrep" and counter of the down (beta) Slater determinant "counter" (0 <= counter < numPerIrrep_down[ irrep ]) cnt2str_down[ irrep ][ counter ] returns the bitstring representation of the corresponding down (beta) Slater determinant
         unsigned int ** cnt2str_down;
         
         //! For irrep "irrep_result" and up (alpha) Slater determinant counter "result" lookup_cnt_alpha[ irrep_result ][ i + L * ( j + L * result ) ] returns the counter "origin" which corresponds to | result > = +/- E^{alpha}_ij | origin > or | origin > = +/- E^{alpha}_ji | result >
         int ** lookup_cnt_alpha;
         
         //! For irrep "irrep_result" and down (beta) Slater determinant counter "result" lookup_cnt_beta[ irrep_result ][ i + L * ( j + L * result ) ] returns the counter "origin" which corresponds to | result > = +/- E^{beta}_ij | origin > or | origin > = +/- E^{beta}_ji | result >
         int ** lookup_cnt_beta;
         
         //! For irrep "irrep_result" and up (alpha) Slater determinant counter "result" lookup_irrep_alpha[ irrep_result ][ i + L * ( j + L * result ) ] returns the irrep "irrep_origin" which corresponds to | result > = +/- E^{alpha}_ij | origin >
         int ** lookup_irrep_alpha;
         
         //! For irrep "irrep_result" and down (beta) Slater determinant counter "result" lookup_irrep_beta[ irrep_result ][ i + L * ( j + L * result ) ] returns the irrep "irrep_origin" which corresponds to | result > = +/- E^{beta}_ij | origin >
         int ** lookup_irrep_beta;
         
         //! For irrep "irrep_result" and up (alpha) Slater determinant counter "result" lookup_sign_alpha[ irrep_result ][ i + L * ( j + L * result ) ] returns the sign s which corresponds to | result > = s * E^{alpha}_ij | origin >
         int ** lookup_sign_alpha;
         
         //! For irrep "irrep_result" and down (beta) Slater determinant counter "result" lookup_sign_beta[ irrep_result ][ i + L * ( j + L * result ) ] returns the sign s which corresponds to | result > = s * E^{beta}_ij | origin >
         int ** lookup_sign_beta;
         
         //! For irrep_center = irrep_creator x irrep_annihilator the number of corresponding excitation pairs E_{creator <= annihilator} is given by irrep_center_num[ irrep_center ]
         unsigned int *  irrep_center_num;
         
         //! For irrep_center = irrep_creator x irrep_annihilator the creator orbital index of the nth (0 <= n < irrep_center_num[ irrep_center ]) excitation pair is given by irrep_center_crea_orb[ irrep_center ][ n ]
         unsigned int ** irrep_center_crea_orb;
         
         //! For irrep_center = irrep_creator x irrep_annihilator the annihilator orbital index of the nth (0 <= n < irrep_center_num[ irrep_center ]) excitation pair is given by irrep_center_anni_orb[ irrep_center ][ n ]
         unsigned int ** irrep_center_anni_orb;
         
         //! The global index corresponding to a vector " E_{ij} | FCI vector > " with irrep_center = irrep_i x irrep_j is given by irrep_center_jumps[ irrep_center ][ irrep_alpha ] + count_alpha + numPerIrrep_up[ irrep_alpha ] * cnt_beta where count_alpha and count_beta are the up (alpha) and down (beta) Slater determinants with resp. irreps irrep_alpha and irrep_beta = TargetIrrep x irrep_alpha x irrep_center
         unsigned long long ** irrep_center_jumps;
         
         //! Number of doubles in each of the HVXworkbig arrays
         unsigned long long HXVsizeWorkspace;
         
         //! Work space of size L*L*L*L
         double * HXVworksmall;
         
         //! Work space of size HXVsizeWorkspace
         double * HXVworkbig1;
         
         //! Work space of size HXVsizeWorkspace
         double * HXVworkbig2;
         
         //! Initialize a part of the private variables
         void StartupCountersVsBitstrings();
         
         //! Initialize a part of the private variables
         void StartupLookupTables();
         
         //! Initialize a part of the private variables
         void StartupIrrepCenter();

   };

}

#endif