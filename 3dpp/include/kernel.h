#pragma once
#include "type.h"
#include "body.h"
#include "tree.h"
#include <map>
#include "traverser.h"
#include <fftw3.h>
#include "align.h"

namespace rtfmm
{
    void dipole_correction(Bodies3& bs, real cycle);
class LaplaceKernel
{
public:
    LaplaceKernel();

    //static void p2p_p(std::vector<vec3r>& xs_src, std::vector<real>& qs_src, std::vector<vec3r>& xs_tar, std::vector<real>& ps_tar, const vec3r& offset = vec3r(0,0,0));

    static void p2p_pf(std::vector<vec3r>& xs_src, std::vector<real>& qs_src, std::vector<vec3r>& xs_tar, std::vector<real>& ps_tar, std::vector<vec3r>& fs_tar, const vec3r& offset = vec3r(0,0,0));

    void p2p(Bodies3& bs_src, Bodies3& bs_tar, Cell3& cell_src, Cell3& cell_tar, vec3r offset = vec3r(0,0,0), int use_simd = 1);

    void p2p_1toN_128(Bodies3& bs_src, Bodies3& bs_tar, Cells3& cs, std::vector<std::pair<int, vec3r>>& p2ps, Cell3& cell_tar);

    void p2p_1toN_256(Bodies3& bs_src, Bodies3& bs_tar, Cells3& cs, std::vector<std::pair<int, vec3r>>& p2ps, Cell3& cell_tar);

    void p2m(int P, Bodies3& bs_src, Cell3& cell_src);

    void p2m_precompute(int P, Bodies3& bs_src, Cell3& cell_src);

    void m2m(int P, Cell3& cell_parent, Cells3& cs);

    void m2m_precompute(int P, Cell3& cell_parent, Cells3& cs);

    void m2m_img(int P, Cell3& cell_parent, Cells3& cs, real cycle);

    void m2m_img_precompute(int P, Cell3& cell_parent, Cells3& cs, real cycle);

    void m2l(int P, Cell3& cell_src, Cell3& cell_tar, vec3r offset = vec3r(0,0,0));

    void m2l_fft(int P, Cell3& cell_src, Cell3& cell_tar, vec3r offset = vec3r(0,0,0));

    void m2l_fft_precompute_naive(int P, Cells3& cs, PeriodicInteractionMap& m2l_map, PeriodicInteractionPairs& m2l_pairs);

    void m2l_fft_precompute_advanced(int P, Cells3& cs, PeriodicInteractionMap& m2l_map, PeriodicInteractionPairs& m2l_pairs);

    void m2l_fft_precompute_advanced2(int P, Cells3& cs, PeriodicInteractionMap& m2l_map);

    void m2l_fft_precompute_advanced3(int P, Cells3& cs, PeriodicInteractionMap& m2l_map, PeriodicInteractionPairs& m2l_pairs);

    void m2l_fft_precompute_t(int P, Cells3& cs, PeriodicInteractionMap& m2l_parent_map);

    void l2l(int P, Cell3& cell_parent, Cells3& cs);

    void l2l_precompute(int P, Cell3& cell_parent, Cells3& cs);

    void l2l_img_precompute(int P, Cell3& cell_parent, Cells3& cs);

    void l2p(int P, Bodies3& bs_tar, Cell3& cell_tar);

    void l2p_precompute(int P, Bodies3& bs_tar, Cell3& cell_tar);

    void m2p(int P, Bodies3& bs_tar, Cell3& cell_src, Cell3& cell_tar, vec3r offset = vec3r(0,0,0));

    void p2l(int P, Bodies3& bs_src, Cell3& cell_src, Cell3& cell_tar, vec3r offset = vec3r(0,0,0));

    void precompute(int P, real r0, int images);

    void precompute_m2l(int P, real r0, Cells3 cs, PeriodicInteractionMap m2l_map, int images);

    Matrix get_p2p_matrix(
        std::vector<vec3r>& x_src, 
        std::vector<vec3r>& x_tar
    );

    Matriv get_force_naive(
        std::vector<vec3r>& x_src, 
        std::vector<vec3r>& x_tar, 
        Matrix& q_src
    );

    std::vector<rtfmm::real> get_G_matrix(std::vector<rtfmm::vec3r>& grid, int N);
    std::vector<rtfmm::real> get_Q_matrix(Matrix& surface_q, int N, std::map<int,rtfmm::vec3i>& surf_conv_map);
    void get_Q_matrix(real* Q, Matrix& surface_q, int N, std::map<int,rtfmm::vec3i>& surf_conv_map);
    std::map<int,rtfmm::vec3i> get_surface_conv_map(int p);

private:
    Matrix UT_p2m_precompute;
    Matrix V_p2m_precompute;
    Matrix Sinv_p2m_precompute;
    Matrix VSinv_p2m_precompute;

    Matrix UT_l2p_precompute;
    Matrix V_l2p_precompute;
    Matrix Sinv_l2p_precompute;
    Matrix VSinv_l2p_precompute;
    Matrix VSinvUT_l2p_precompute;

    Matrix matrix_m2m[8];
    Matrix matrix_m2m_img[27];
    Matrix matrix_l2l[8];
    Matrix matrix_l2l_img;

    std::vector<int> m2l_tars;
    std::vector<int> m2l_srcs;
    std::map<int, std::vector<complexr>> m2l_Gks;
    std::map<int, std::pair<std::vector<complexr>, int>> m2l_Gk_idx;

    void matmult_8x8x1(real*& M_, real*& IN0, real*& OUT0);
    
    void matmult_8x8x1_naive(real*& M_, real*& IN0, real*& OUT0);

    void matmult_8x8x2(real*& M_, real*& IN0, real*& IN1, real*& OUT0, real*& OUT1);

    void matmult_8x8x2_avx(double*& M_, double*& IN0, double*& IN1, double*& OUT0, double*& OUT1);

    /**
     * @brief matrix vector storing child-child interaction Gk of 2 neighbour cells
     * @note ### number of matrix = 26, size of each matrix = N_freq * 8 * 8 * complex
    */
    std::vector<AlignedVec> ccGks;
};
};