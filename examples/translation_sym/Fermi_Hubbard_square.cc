#include <iostream>
#include <iomanip>
#include "qbasis.h"

// Fermi-Hubbard model on square lattice, x-dir periodic, y-dir periodic
int main() {
    std::cout << std::setprecision(10);
    // parameters
    double t = 1;
    double U = 1.1;
    MKL_INT Lx = 3;
    MKL_INT Ly = 3;
    MKL_INT Nup_total = 5;
    MKL_INT Ndn_total = 4;
    
    
    // lattice object
    // you are allowed to change "pbc" to "obc" to play,
    // just remember to delete the "assertion" lines at the bottom of this file
    std::vector<std::string> bc{"pbc", "pbc"};
    qbasis::lattice lattice("square",std::vector<MKL_INT>{Lx, Ly},bc);
    
    
    // local matrix representation
    auto c_up = std::vector<std::vector<std::complex<double>>>(4,std::vector<std::complex<double>>(4, 0.0));
    auto c_dn = std::vector<std::vector<std::complex<double>>>(4,std::vector<std::complex<double>>(4, 0.0));
    c_up[0][1] = std::complex<double>(1.0,0.0);
    c_up[2][3] = std::complex<double>(1.0,0.0);
    c_dn[0][2] = std::complex<double>(1.0,0.0);
    c_dn[1][3] = std::complex<double>(-1.0,0.0);
    
    
    // constructing the Hamiltonian in operator representation
    qbasis::model<std::complex<double>> Hubbard;
    qbasis::mopr<std::complex<double>> Nup;   // an operator representating total electron number
    qbasis::mopr<std::complex<double>> Ndown;
    for (MKL_INT x = 0; x < Lx; x++) {
        for (MKL_INT y = 0; y < Ly; y++) {
            MKL_INT site_i, site_j;
            lattice.coor2site(std::vector<MKL_INT>{x,y}, 0, site_i); // obtain site label of (x,y)
            // construct operators on each site
            auto c_up_i    = qbasis::opr<std::complex<double>>(site_i,0,true,c_up);
            auto c_dn_i    = qbasis::opr<std::complex<double>>(site_i,0,true,c_dn);
            auto c_up_dg_i = c_up_i; c_up_dg_i.dagger();
            auto c_dn_dg_i = c_dn_i; c_dn_dg_i.dagger();
            auto n_up_i    = c_up_dg_i * c_up_i;
            auto n_dn_i    = c_dn_dg_i * c_dn_i;
            
            // hopping to neighbor (x+1, y)
            if (bc[0] == "pbc" || (bc[0] == "obc" && x < Lx - 1)) {
                lattice.coor2site(std::vector<MKL_INT>{x+1,y}, 0, site_j);
                auto c_up_j    = qbasis::opr<std::complex<double>>(site_j,0,true,c_up);
                auto c_dn_j    = qbasis::opr<std::complex<double>>(site_j,0,true,c_dn);
                auto c_up_dg_j = c_up_j; c_up_dg_j.dagger();
                auto c_dn_dg_j = c_dn_j; c_dn_dg_j.dagger();
                Hubbard.add_offdiagonal_Ham(std::complex<double>(-t,0.0) * ( c_up_dg_i * c_up_j ));
                Hubbard.add_offdiagonal_Ham(std::complex<double>(-t,0.0) * ( c_up_dg_j * c_up_i ));
                Hubbard.add_offdiagonal_Ham(std::complex<double>(-t,0.0) * ( c_dn_dg_i * c_dn_j ));
                Hubbard.add_offdiagonal_Ham(std::complex<double>(-t,0.0) * ( c_dn_dg_j * c_dn_i ));
            }
            
            // hopping to neighbor (x, y+1)
            if (bc[1] == "pbc" || (bc[1] == "obc" && y < Ly - 1)) {
                lattice.coor2site(std::vector<MKL_INT>{x,y+1}, 0, site_j);
                auto c_up_j    = qbasis::opr<std::complex<double>>(site_j,0,true,c_up);
                auto c_dn_j    = qbasis::opr<std::complex<double>>(site_j,0,true,c_dn);
                auto c_up_dg_j = c_up_j; c_up_dg_j.dagger();
                auto c_dn_dg_j = c_dn_j; c_dn_dg_j.dagger();
                Hubbard.add_offdiagonal_Ham(std::complex<double>(-t,0.0) * ( c_up_dg_i * c_up_j ));
                Hubbard.add_offdiagonal_Ham(std::complex<double>(-t,0.0) * ( c_up_dg_j * c_up_i ));
                Hubbard.add_offdiagonal_Ham(std::complex<double>(-t,0.0) * ( c_dn_dg_i * c_dn_j ));
                Hubbard.add_offdiagonal_Ham(std::complex<double>(-t,0.0) * ( c_dn_dg_j * c_dn_i ));
            }
            
            // Hubbard repulsion, note that this operator is a sum (over sites) of diagonal matrices
            Hubbard.add_diagonal_Ham(std::complex<double>(U,0.0) * (n_up_i * n_dn_i));
            
            // total electron operator
            Nup   += n_up_i;
            Ndown += n_dn_i;
        }
    }
    

    // constructing the Hilbert space basis
    Hubbard.enumerate_basis_full_conserve(lattice.total_sites(), {"electron"}, {Nup,Ndown}, {static_cast<double>(Nup_total),static_cast<double>(Ndn_total)});
    std::cout << "dim_full = " << Hubbard.dim_full << std::endl;
    
    
    std::vector<double> energies;
    for (MKL_INT i = 0; i < Lx; i++) {
        for (MKL_INT j = 0; j < Ly; j++) {
            // constructing the subspace basis
            Hubbard.basis_init_repr(std::vector<MKL_INT>{i,j}, lattice);
            
            // generating matrix of the Hamiltonian in the subspace
            Hubbard.generate_Ham_sparse_repr();
            std::cout << std::endl;
            
            // obtaining the lowest eigenvals of the matrix
            Hubbard.locate_E0_repr();
            std::cout << std::endl;
            energies.push_back(Hubbard.eigenvals_repr[0]);
        }
    }
    
    
    // for the parameters considered, we should obtain:
    assert(std::abs(energies[0] + 10.146749232) < 1e-8);
    assert(std::abs(energies[1] + 12.683981731) < 1e-8);
    assert(std::abs(energies[2] + 12.683981731) < 1e-8);
    assert(std::abs(energies[3] + 12.683981731) < 1e-8);
    assert(std::abs(energies[4] + 10.101817578) < 1e-8);
    assert(std::abs(energies[5] + 10.101817578) < 1e-8);
    assert(std::abs(energies[6] + 12.683981731) < 1e-8);
    assert(std::abs(energies[7] + 10.101817578) < 1e-8);
    assert(std::abs(energies[8] + 10.101817578) < 1e-8);
}