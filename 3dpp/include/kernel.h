#pragma once
#include "type.h"
#include "body.h"
#include "tree.h"
#include <map>

namespace rtfmm
{
    void dipole_correction(Bodies3& bs, real cycle);
class LaplaceKernel
{
public:
    LaplaceKernel();

    void p2p(Bodies3& bs_src, Bodies3& bs_tar, Cell3& cell_src, Cell3& cell_tar, vec3r offset = vec3r(0,0,0));

    void p2p_matrix(Bodies3& bs_src, Bodies3& bs_tar, Cell3& cell_src, Cell3& cell_tar, vec3r offset = vec3r(0,0,0));
    
    void p2m(int P, Bodies3& bs_src, Cell3& cell_src);

    void m2m(int P, Cell3& cell_parent, Cells3& cs);

    void m2m_img(int P, Cell3& cell_parent, Cells3& cs, real cycle);

    void m2l(int P, Cell3& cell_src, Cell3& cell_tar, vec3r offset = vec3r(0,0,0));

    void m2l_fft(int P, Cell3& cell_src, Cell3& cell_tar, vec3r offset = vec3r(0,0,0));

    void l2l(int P, Cell3& cell_parent, Cells3& cs);

    void l2p(int P, Bodies3& bs_tar, Cell3& cell_tar);

    void m2p(int P, Bodies3& bs_tar, Cell3& cell_src, Cell3& cell_tar, vec3r offset = vec3r(0,0,0));

    void p2l(int P, Bodies3& bs_src, Cell3& cell_src, Cell3& cell_tar, vec3r offset = vec3r(0,0,0));

    void precompute();

private:
    Matrix get_p2p_matrix(
        std::vector<vec3r>& x_src, 
        std::vector<vec3r>& x_tar
    );

    Matriv get_force_naive(
        std::vector<vec3r> x_src, 
        std::vector<vec3r> x_tar, 
        Matrix q_src
    );

    std::vector<rtfmm::real> get_G_matrix(std::vector<rtfmm::vec3r>& grid, int N);
    std::vector<rtfmm::real> get_Q_matrix(Matrix& surface_q, int N, std::map<int,rtfmm::vec3i> surf_conv_map);
    std::map<int,rtfmm::vec3i> get_surface_conv_map(int p);
};
};