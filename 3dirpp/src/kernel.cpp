#include "kernel.h"
#include <set>

void rtfmm::LaplaceKernel::direct(Bodies& bs_src, Bodies& bs_tar, int images, real cycle)
{
    vec4d zero(0);
    vec4d one(1);
    int num_tar = bs_tar.size();
    int num_src = bs_src.size();
    int dm = (std::pow(3, images) - 1) / 2;
    #pragma omp parallel for
    for(int j = 0; j < num_tar; j+=4)
    {
        Body* btar = &bs_tar[j];
        vec4d xj,yj,zj;
        int padding = j + 4 - num_tar;
        padding = padding <= 0 ? 0 : padding;
        for(int k = 0; k < 4 - padding; k++)
        {
            xj[k] = (btar + k)->x[0];
            yj[k] = (btar + k)->x[1];
            zj[k] = (btar + k)->x[2];
        }
        vec4d pj(0);
        vec4d fxj(0);
        vec4d fyj(0);
        vec4d fzj(0);
        vec4d dx, dy, dz, qi, invr, f, qinvr;
        
        for(int pz = -dm; pz <= dm; pz++)
        {
            for(int py = -dm; py <= dm; py++)
            {
                for(int px = -dm; px <= dm; px++)
                {
                    vec3r offset(px * cycle, py * cycle, pz * cycle);
                    for(int i = 0; i < num_src; i++)
                    {
                        Body& bsrc = bs_src[i];
                        vec3r bsrc_xi = bsrc.x + offset;
                        dx = vec4d(bsrc_xi[0]);
                        dy = vec4d(bsrc_xi[1]);
                        dz = vec4d(bsrc_xi[2]);
                        qi = vec4d(bsrc.q);
                        dx = xj - dx;
                        dy = yj - dy;
                        dz = zj - dz;
                        invr = dx * dx + dy * dy + dz * dz;
                        f = invr > zero;
                        invr = one / invr.sqrt();
                        invr &= f;
                        qinvr = qi * invr;
                        pj += qinvr;
                        qinvr = qinvr * invr * invr;
                        fxj += qinvr * dx;
                        fyj += qinvr * dy;
                        fzj += qinvr * dz;
                    }
                }
            }
        }
        for(int k = 0; k < 4 - padding; k++)
        {
            (btar + k)->p    += pj[k];
            (btar + k)->f[0] -= fxj[k];
            (btar + k)->f[1] -= fyj[k];
            (btar + k)->f[2] -= fzj[k];
        }
    }
}

void rtfmm::LaplaceKernel::p2p(Cells& cs, std::vector<int>& p2ps, Cell& cell_tar)
{
    vec4d zero(0);
    vec4d one(1);
    vec4d dx, dy, dz, qi, invr, f, qinvr;
    for(int j = 0; j < cell_tar.bs.size(); j+=4)
    {
        Body* btar = &cell_tar.bs[j];
        vec4d xj(0),yj(0),zj(0);
        int padding = j + 4 - cell_tar.bs.size();
        padding = padding <= 0 ? 0 : padding;
        for(int k = 0; k < 4 - padding; k++)
        {
            xj[k] = (btar+k)->x[0];
            yj[k] = (btar+k)->x[1];
            zj[k] = (btar+k)->x[2];
        }
        vec4d pj(0),fxj(0),fyj(0),fzj(0);
        for(int si = 0; si < p2ps.size(); si++)
        {
            Cell& cell_src = cs[p2ps[si]];
            for(int i = 0; i < cell_src.bs.size(); i++)
            {
                Body& bsrc = cell_src.bs[i];
                vec3r bsrc_xi = bsrc.x;
                dx = vec4d(bsrc_xi[0]);
                dy = vec4d(bsrc_xi[1]);
                dz = vec4d(bsrc_xi[2]);
                qi = vec4d(bsrc.q);
                dx = xj - dx;
                dy = yj - dy;
                dz = zj - dz;
                invr = dx * dx + dy * dy + dz * dz;
                f = invr > zero;
                invr = one / invr.sqrt();
                invr &= f;
                qinvr = qi * invr;
                pj += qinvr;
                qinvr = qinvr * invr * invr;
                fxj += qinvr * dx;
                fyj += qinvr * dy;
                fzj += qinvr * dz;
            }
        }
        for(int k = 0; k < 4 - padding; k++)
        {
            (btar + k)->p    += pj[k];
            (btar + k)->f[0] -= fxj[k];
            (btar + k)->f[1] -= fyj[k];
            (btar + k)->f[2] -= fzj[k];
        }
    }
}

void rtfmm::LaplaceKernel::p2m(int P, Cell& csrc)
{
    int num_surf = get_surface_point_num(P);
    real scale = std::pow(0.5, csrc.xlogic[0]);
    std::vector<vec3r> x_check = get_surface_points(P, csrc.r * 2.95, csrc.xphys);
    Matrix buffer(num_surf, 1);
    Matrix& p_check = csrc.q_equiv; // use q_equiv as p_check_buffer temporarily
    vec4d zero(0),one(1);
    vec4d dx,dy,dz,invr,f;
    for(int j = 0; j < num_surf; j += 4)
    {
        vec4d xj(0),yj(0),zj(0),pj(0);
        int padding = j + 4 - num_surf;
        padding = padding <= 0 ? 0 : padding;
        for(int idx = 0; idx < 4 - padding; idx++)
        {
            xj[idx] = x_check[j+idx][0];
            yj[idx] = x_check[j+idx][1];
            zj[idx] = x_check[j+idx][2];
        }
        Body* bi = &csrc.bs[0];
        for(int i = 0; i < csrc.bs.size(); i++)
        {
            dx = xj - vec4d(bi->x[0]);
            dy = yj - vec4d(bi->x[1]);
            dz = zj - vec4d(bi->x[2]);
            invr = dx * dx + dy * dy + dz * dz;
            f = invr > zero;
            invr = one / invr.sqrt();
            pj += vec4d(bi->q) * (invr & f);
            bi++;
        }
        for(int k = 0; k < 4 - padding; k++)
        {
            p_check[j+k] += pj[k];
        }
    }
    mat_vec_mul(UT_p2m_precompute, p_check, buffer);
    mat_vec_mul(VSinv_p2m_precompute, buffer, csrc.q_equiv, scale);
}

void rtfmm::LaplaceKernel::m2m(int P, Cell& c, Tree& tree)
{
    Matrix q_equiv_parent(get_surface_point_num(P), 1);
    for(int i = 0; i < 8; i++)
    {
        Cell& child = tree.cs[tree.find_cidx(c.xlogic.child(i))];
        mat_vec_mul(matrix_m2m[i], child.q_equiv, q_equiv_parent);
        mat_mat_increment(c.q_equiv, q_equiv_parent);
    }
}

void rtfmm::LaplaceKernel::l2l(int P, Cell& c, Tree& tree)
{
    Matrix p_check_child(get_surface_point_num(P), 1);
    for(int i = 0; i < 8; i++)
    {
        Cell& child = tree.cs[tree.find_cidx(c.xlogic.child(i))];
        mat_vec_mul(matrix_l2l[i], c.p_check, p_check_child);
        mat_mat_increment(child.p_check, p_check_child);
    }
}

void rtfmm::LaplaceKernel::l2p(int P, Cell& ctar)
{
    int num_surf = get_surface_point_num(P);
    std::vector<rtfmm::vec3r> x_equiv = get_surface_points(P, ctar.r * 2.95, ctar.xphys);

    real scale = std::pow(0.5, ctar.xlogic[0]);
    Matrix UTb(num_surf, 1);
    mat_vec_mul(UT_l2p_precompute, ctar.p_check, UTb);
    Matrix& q_equiv = ctar.p_check; //as in p2m, we can use p_check as buffer here
    mat_vec_mul(VSinv_l2p_precompute, UTb, q_equiv, scale);

    vec4d zero(0), one(1);
    vec4d dx, dy, dz, qi, invr, f, qinvr;
    for(int j = 0; j < ctar.bs.size(); j += 4)
    {
        vec4d xj(0), yj(0), zj(0);
        vec4d pj(0),fxj(0),fyj(0),fzj(0);
        int padding = j + 4 - ctar.bs.size();
        padding = padding <= 0 ? 0 : padding;
        for(int idx = 0; idx < 4 - padding; idx++)
        {
            xj[idx] = ctar.bs[idx].x[0];
            yj[idx] = ctar.bs[idx].x[1];
            zj[idx] = ctar.bs[idx].x[2];
        }
        for(int i = 0; i < num_surf; i++)
        {
            dx = vec4d(x_equiv[i][0]);
            dy = vec4d(x_equiv[i][1]);
            dz = vec4d(x_equiv[i][2]);
            qi = vec4d(q_equiv[i]);
            dx = xj - dx;
            dy = yj - dy;
            dz = zj - dz;
            invr = dx * dx + dy * dy + dz * dz;
            f = invr > zero;
            invr = one / invr.sqrt();
            invr &= f;
            qinvr = qi * invr;
            pj += qinvr;
            qinvr *= invr * invr;
            fxj += qinvr * dx;
            fyj += qinvr * dy;
            fzj += qinvr * dz;
        }
        for(int idx = 0; idx < 4 - padding; idx++)
        {
            ctar.bs[idx].p += pj[idx];
            ctar.bs[idx].f[0] -= fxj[idx];
            ctar.bs[idx].f[1] -= fyj[idx];
            ctar.bs[idx].f[2] -= fzj[idx];
        }
    }
}

void rtfmm::LaplaceKernel::m2p(int P, Cell& cell_src, Cell& cell_tar)
{
    std::vector<rtfmm::vec3r> x_equiv_src = get_surface_points(P, cell_src.r * 1.05, cell_src.xphys);
    std::vector<vec3r> x_tar;
    for(int i = 0; i < cell_tar.bs.size(); i++)
    {
        x_tar.push_back(cell_tar.bs[i].x);
    }
    Matrix e2t = get_p2p_matrix(x_equiv_src, x_tar);
    Matrix p_tar = mat_vec_mul(e2t, cell_src.q_equiv);
    Matriv f_tar = get_force_naive(x_equiv_src, x_tar, cell_src.q_equiv);
    for(int i = 0; i < cell_tar.bs.size(); i++)
    {
        Body& b = cell_tar.bs[i];
        b.p += p_tar[i];
        b.f += f_tar[i];
    }
}

void rtfmm::LaplaceKernel::p2l(int P, Cell& cell_src, Cell& cell_tar)
{
    std::vector<vec3r> x_src;
    for(int i = 0; i < cell_src.bs.size(); i++)
    {
        x_src.push_back(cell_src.bs[i].x);
    }
    Matrix q_src = get_bodies_q(cell_src.bs, cell_src.brange);
    std::vector<rtfmm::vec3r> x_check_tar = get_surface_points(P, cell_tar.r * 1.05, cell_tar.xphys);
    Matrix s2c = get_p2p_matrix(x_src, x_check_tar);
    Matrix p_check = mat_vec_mul(s2c, q_src);
    cell_tar.p_check = mat_mat_add(cell_tar.p_check, p_check);
}

void rtfmm::LaplaceKernel::hadamard_8x8(
    int fft_size, 
    conv_grid_setting& cgrid,
    std::vector<size_t>& interaction_count_offset_8x8,
    std::vector<size_t>& interaction_offset_f_8x8,
    AlignedVec& Qk_all, 
    AlignedVec& Pk_all
)
{
    const int MAX_CHILD_NUM = 8;
    AlignedVec zero_vec0(fft_size, 0.);
    AlignedVec zero_vec1;
    zero_vec1.reserve(fft_size);
    size_t max_num_interaction = 0;
    for(int i = 0; i < 26; i++)
    {
        size_t offset0 = (i == 0 ? 0 : interaction_count_offset_8x8[i - 1]);
        size_t offset1 = interaction_count_offset_8x8[i];
        size_t num_interaction = offset1 - offset0;
        max_num_interaction = std::max(max_num_interaction, num_interaction);
    }
    int BLOCK_SIZE = max_num_interaction + 1; // padding 1 to store ptr of zeros
    std::vector<real*> Qk_ptrs(26 * BLOCK_SIZE);
    std::vector<real*> Pk_ptrs(26 * BLOCK_SIZE);
    #pragma omp parallel for
    for(int i = 0; i < 26; i++)
    {
        size_t offset0 = (i == 0 ? 0 : interaction_count_offset_8x8[i - 1]);
        size_t offset1 = interaction_count_offset_8x8[i];
        size_t interaction_count = offset1 - offset0;
        for(size_t j = 0; j < interaction_count; j++)
        {
            Qk_ptrs[i * BLOCK_SIZE + j] = &Qk_all[interaction_offset_f_8x8[2*(offset0 + j) + 0]];
            Pk_ptrs[i * BLOCK_SIZE + j] = &Pk_all[interaction_offset_f_8x8[2*(offset0 + j) + 1]];
        }
        Qk_ptrs[i * BLOCK_SIZE + interaction_count]  = &zero_vec0[0];
        Pk_ptrs[i * BLOCK_SIZE + interaction_count] = &zero_vec1[0];
    }
    #pragma omp parallel for
    for(int f = 0; f < cgrid.N_freq; f++)
    {
        for(int ipos = 0; ipos < 26; ipos++)
        {
            size_t offset0 = (ipos == 0 ? 0 : interaction_count_offset_8x8[ipos - 1]);
            size_t offset1 = interaction_count_offset_8x8[ipos]; 
            size_t num_interaction = offset1 - offset0;
            real** Qk_ptr  = &Qk_ptrs [ipos * BLOCK_SIZE];
            real** Pk_ptr = &Pk_ptrs[ipos * BLOCK_SIZE];
            real* ccGk = &ccGks_8x8[ipos][f * 2 * MAX_CHILD_NUM * MAX_CHILD_NUM];
            for(size_t j = 0; j < num_interaction; j += 2)
            {
                real* Gk = ccGk;
                real* Qk1 = Qk_ptr[j + 0] + f * MAX_CHILD_NUM * 2; // Qk of children in frequency f, size = 8 * 2
                real* Qk2 = Qk_ptr[j + 1] + f * MAX_CHILD_NUM * 2; 
                real* Pk1 = Pk_ptr[j + 0] + f * MAX_CHILD_NUM * 2; // Pk of children in frequency f, size = 8 * 2
                real* Pk2 = Pk_ptr[j + 1] + f * MAX_CHILD_NUM * 2;
                c2c_8x8x2_avx(Gk, Qk1, Qk2, Pk1, Pk2);
            }
        }
    }
}

/*void rtfmm::LaplaceKernel::hadamard_1x27(
    int fft_size, 
    conv_grid_setting& cgrid,
    std::vector<size_t>& interaction_count_offset_1x27,
    std::vector<size_t>& interaction_offset_f_1x27,
    AlignedVec& Qk_all, 
    AlignedVec& Pk_all
)
{
    const int MAX_CHILD_NUM = 8;
    int max_num_interaction = 0;
    for(int i = 0; i < 26; i++)
    {
        size_t offset0 = (i == 0 ? 0 : interaction_count_offset_1x27[i - 1]);
        size_t offset1 = interaction_count_offset_1x27[i];
        int num_interaction = offset1 - offset0;
        max_num_interaction = std::max(max_num_interaction, num_interaction);
    }
    int& BLOCK_SIZE = max_num_interaction;
    std::vector<real*> Qk_ptrs(26 * BLOCK_SIZE);
    std::vector<real*> Pk_ptrs(26 * BLOCK_SIZE);
    #pragma omp parallel for
    for(int i = 0; i < 26; i++)
    {
        size_t offset0 = (i == 0 ? 0 : interaction_count_offset_1x27[i - 1]);
        size_t offset1 = interaction_count_offset_1x27[i];
        size_t interaction_count = offset1 - offset0;
        for(size_t j = 0; j < interaction_count; j++)
        {
            Qk_ptrs[i * BLOCK_SIZE + j] = &Qk_all[interaction_offset_f_1x27[2 * offset0 + 2 * j + 0]];
            Pk_ptrs[i * BLOCK_SIZE + j] = &Pk_all[interaction_offset_f_1x27[2 * offset0 + 2 * j + 1]];
        }
    }
    #pragma omp parallel for
    for(int f = 0; f < cgrid.N_freq; f++)
    {
        for(int i = 0; i < 26; i++)
        {
            size_t offset0 = (i == 0 ? 0 : interaction_count_offset_1x27[i - 1]);
            size_t offset1 = interaction_count_offset_1x27[i]; 
            size_t num_interaction = offset1 - offset0;
            real** Qk_ptr = &Qk_ptrs[i * BLOCK_SIZE];
            real** Pk_ptr = &Pk_ptrs[i * BLOCK_SIZE];
            real* ccGk = &ccGks_1x27[i][f * 2 * 1 * 28];
            for(size_t j = 0; j < num_interaction; j++)
            {
                real* Gk = ccGk;
                real* Qk = Qk_ptr[j + 0] + f * MAX_CHILD_NUM * 2; //src
                real* Pk = Pk_ptr[j + 0] + f * MAX_CHILD_NUM * 2; //tar
                //c2c_1x27x1_naive(Gk, Qk, Pk);
                c2c_1x27x1_AVX(Gk, Qk, Pk);
            }
        }
    }
}*/


void rtfmm::LaplaceKernel::m2l(int P, Cells& cs, std::map<int, std::vector<int>>& m2l_parent_map)
{
    // setup
    tbegin(setup);
    const int MAX_CHILD_NUM = 8;
    int num_cell = cs.size();
    int num_surf = get_surface_point_num(P);
    conv_grid_setting cgrid(P, cs[0].r);
    int dim[3] = {cgrid.N, cgrid.N, cgrid.N};
    std::map<int,rtfmm::vec3i> surf_conv_map = get_surface_conv_map(P);
    size_t fft_size = 2 * MAX_CHILD_NUM * cgrid.N_freq;
    if(verbose) std::cout<<"M2L "<<cgrid<<std::endl;

    std::vector<int> tar_cell_idxs;
    std::set<int> src_cell_idxs_set;
    for(auto m2l_parent : m2l_parent_map)
    {
        tar_cell_idxs.push_back(m2l_parent.first);
        for(int i = 0; i < m2l_parent.second.size(); i++)
        {
            src_cell_idxs_set.insert(m2l_parent.second[i]);
        }
    }
    std::vector<int> src_cell_idxs;
    for(auto src_cell_idx : src_cell_idxs_set)
    {
        src_cell_idxs.push_back(src_cell_idx);
    }
    int num_src_cell = src_cell_idxs.size();
    int num_tar_cell = tar_cell_idxs.size();
    if(verbose) printf("num_src_cell = %d, num_tar_cell = %d\n", num_src_cell, num_tar_cell);

    std::map<int, int> src_cell_local_map;  //  src_cell_idx -> local src index
    for(int i = 0; i < num_src_cell; i++)
    {
        int src_idx = src_cell_idxs[i];
        src_cell_local_map[src_idx] = i;
    }
    std::vector<size_t> fft_offset(num_src_cell);
    std::vector<size_t> ifft_offset(num_tar_cell);
    for(int i = 0; i < num_src_cell; i++)
    {
        fft_offset[i] = cs[src_cell_idxs[i]].crange.offset * num_surf;
    }
    for(int i = 0; i < num_tar_cell; i++)
    {
        ifft_offset[i] = cs[tar_cell_idxs[i]].crange.offset * num_surf;
    }

    std::map<int, int> rel2idx_map = get_relx_idx_map(1,1);
    std::map<int,std::map<int,vec2i>> m2l_parent_octant_map; 
    for(int i = 0; i < tar_cell_idxs.size(); i++)
    {
        int tar_idx = tar_cell_idxs[i];
        Cell3& tar_cell = cs[tar_idx];
        std::vector<PeriodicParentSource> m2l_list = m2l_parent_map[tar_idx];
        for(int d = 0; d < 26; d++)
        {
            m2l_parent_octant_map[tar_idx][d] = -1;
        }
        for(int j = 0; j < m2l_list.size(); j++)
        {
            int src_idx = m2l_list[j].idx;
            vec3r src_offset = m2l_list[j].offset;
            int is_image = m2l_list[j].is_single_parent;
            Cell3& src_cell = cs[src_idx];
            vec3r rv = (src_cell.x + src_offset - tar_cell.x) / (tar_cell.r * 2);
            vec3i rvi(std::round(rv[0]), std::round(rv[1]), std::round(rv[2]));
            int octant = rel2idx_map[hash(rvi)];
            m2l_parent_octant_map[tar_idx][octant] = {src_idx, is_image}; // cell located at tar's octant direction is src(or -1)
        }
    }

    std::vector<size_t> interaction_offset_f_8x8;
    std::vector<size_t> interaction_count_offset_8x8;
    size_t interaction_cnt_8x8 = 0;
    std::vector<size_t> interaction_offset_f_1x27;
    std::vector<size_t> interaction_count_offset_1x27;
    size_t interaction_cnt_1x27 = 0;
    for(int k = 0; k < 26; k++)
    {
        for(int i = 0; i < tar_cell_idxs.size(); i++)
        {
            int tar_idx = tar_cell_idxs[i];
            int src_idx = m2l_parent_octant_map[tar_idx][k][0];
            int is_image_interaction = m2l_parent_octant_map[tar_idx][k][1];
            if(src_idx != -1)
            {
                int src_cell_local_idx = src_cell_local_map[src_idx];
                if(is_image_interaction == 0)
                {
                    interaction_offset_f_8x8.push_back(src_cell_local_idx * fft_size);
                    interaction_offset_f_8x8.push_back(i * fft_size);
                    interaction_cnt_8x8++;
                }
                else
                {
                    interaction_offset_f_1x27.push_back(src_cell_local_idx * fft_size);
                    interaction_offset_f_1x27.push_back(i * fft_size);
                    interaction_cnt_1x27++;
                }
            }
        }
        interaction_count_offset_8x8.push_back(interaction_cnt_8x8);
        interaction_count_offset_1x27.push_back(interaction_cnt_1x27);
    }
    std::vector<int> surf_conv_idx_map_up(num_surf);
    for (int k = 0; k < num_surf; k++) 
    {
        rtfmm::vec3i idx3 = surf_conv_map[k];
        int idx = idx3[2] * cgrid.N * cgrid.N + idx3[1] * cgrid.N + idx3[0];
        surf_conv_idx_map_up[k] = idx;
    }
    std::vector<int> surf_conv_idx_map_down(num_surf);
    for (int k = 0; k < num_surf; k++) 
    {
        rtfmm::vec3i idx3 = surf_conv_map[k] + vec3i(P-1,P-1,P-1); // vec3i(P-1,P-1,P-1) is conv offset, see test_fft.cpp
        int idx = idx3[2] * cgrid.N * cgrid.N + idx3[1] * cgrid.N + idx3[0];
        surf_conv_idx_map_down[k] = idx;
    }
    tend(setup);

    tbegin(prepare_memory);
    std::vector<real> Q_all, P_all;
    AlignedVec Qk_all, Pk_all;
    Q_all.reserve(num_cell * num_surf);
    P_all.reserve(num_cell * num_surf);
    Qk_all.reserve(num_src_cell * fft_size);
    Pk_all.reserve(num_tar_cell * fft_size);
    #pragma omp parallel for collapse(2)
    for (int i = 0; i < num_cell; i++) 
    {
        for (int k = 0; k < num_surf; k++) 
        {
            Q_all[i * num_surf + k] = cs[i].q_equiv[k];
            P_all[i * num_surf + k] = cs[i].p_check[k];
        }
    }
    /* According to Fourier transformation addition theorem, interaction from different srcs are added up in Fourier space, so we need set Pk to zero in advance. */
    #pragma omp parallel for
    for(size_t i = 0; i < num_tar_cell; i++)
    {
        std::memset(Pk_all.data() + i * fft_size, 0, fft_size * sizeof(real));
    }
    tend(prepare_memory);

    /* up */
    tbegin(up);
    AlignedVec fftw_in(cgrid.N3 * MAX_CHILD_NUM);
    AlignedVec fftw_out(fft_size);
    fftw_plan plan_up = fftw_plan_many_dft_r2c(
        3, dim, MAX_CHILD_NUM,
        (real*)&fftw_in[0]         , nullptr, 1, cgrid.N3,
        (fftw_complex*)&fftw_out[0], nullptr, 1, cgrid.N_freq,
        FFTW_ESTIMATE
    );
    #pragma omp parallel for
    for (int isrc = 0; isrc < num_src_cell; isrc++)
    {
        std::vector<real> buffer;
        buffer.reserve(fft_size);
        real* Q_children = &Q_all[fft_offset[isrc]];
        real* Qk_children = &Qk_all[fft_size * isrc]; // Qk of isrc's children
        std::memset(Qk_children, 0, fft_size * sizeof(real));
        for (int k = 0; k < num_surf; k++) 
        {
            int idx = surf_conv_idx_map_up[k];
            int cpidx = src_cell_idxs[isrc];
            Cell3& cp = cs[cpidx];
            for (int i = 0; i < cp.crange.number; i++)
            {
                int octant = cs[cp.crange.offset + i].octant;
                octant = (octant == 13 ? 0 : octant); // for an image cell, it has only one child, so we store it in position 0
                Qk_children[octant * cgrid.N3 + idx] = Q_children[i * num_surf + k]; // insert Q into conv grid
            }
        }
        fftw_execute_dft_r2c(plan_up, Qk_children, (fftw_complex*)&buffer[0]);
        for (int f = 0; f < cgrid.N_freq; f++) 
        {
            for (int j = 0; j < MAX_CHILD_NUM; j++) 
            {
                // since FFTW's output is MAX_CHILD_NUM by Nfreq, we should transpose it to Nfreq by MAX_CHILD_NUM for faster hadamard product
                Qk_children[f * 2 * MAX_CHILD_NUM + 2 * j + 0] = buffer[j * 2 * cgrid.N_freq + 2 * f + 0];
                Qk_children[f * 2 * MAX_CHILD_NUM + 2 * j + 1] = buffer[j * 2 * cgrid.N_freq + 2 * f + 1]; 
            }
        }
    }
    fftw_destroy_plan(plan_up);
    tend(up);

    /* hadamard product */
    {
        tbegin(hadamard_8x8);
        hadamard_8x8(fft_size, cgrid, interaction_count_offset_8x8, interaction_offset_f_8x8, Qk_all, Pk_all);
        tend(hadamard_8x8);
    }
    if(interaction_cnt_1x27 > 0)
    {
        tbegin(hadamard_1x27);
        hadamard_1x27(fft_size, cgrid, interaction_count_offset_1x27, interaction_offset_f_1x27, Qk_all, Pk_all);
        tend(hadamard_1x27);
    }

    /* down */
    tbegin(down);
    AlignedVec fftw_in2(fft_size);
    AlignedVec fftw_out2(cgrid.N3 * MAX_CHILD_NUM);
    fftw_plan plan_down = fftw_plan_many_dft_c2r(
        3, dim, MAX_CHILD_NUM,
        (fftw_complex*)&fftw_in2[0], nullptr, 1, cgrid.N_freq,
        (real*)&fftw_out2[0], nullptr, 1, cgrid.N3,
        FFTW_ESTIMATE
    );
    #pragma omp parallel for
    for(int i = 0; i < num_tar_cell; i++)
    {
        std::vector<real> buffer0(fft_size, 0);
        std::vector<real> buffer1(fft_size, 0);
        real* Pk_children = &Pk_all[i * fft_size];
        real* P_children = &P_all[ifft_offset[i]];
        for(int f = 0; f < cgrid.N_freq; f++)
        {
            for(int j = 0; j < MAX_CHILD_NUM; j++)
            {
                // since FFTW need frequent-first order, we should transpose Pk here
                buffer0[j * 2 * cgrid.N_freq + 2 * f + 0] = Pk_children[f * 2 * MAX_CHILD_NUM + 2 * j + 0];
                buffer0[j * 2 * cgrid.N_freq + 2 * f + 1] = Pk_children[f * 2 * MAX_CHILD_NUM + 2 * j + 1];
            }
        }
        fftw_execute_dft_c2r(plan_down, (fftw_complex*)&buffer0[0], (real*)&buffer1[0]);
        int tar_idx = tar_cell_idxs[i];
        Cell3& ctar = cs[tar_idx];
        real scale = std::pow(ctar.depth >= -1 ? 2 : 3, ctar.depth + 1); // since we store children's P, the depth should + 1
        for(int k = 0; k < num_surf; k++)
        {
            int idx = surf_conv_idx_map_down[k];
            for(int j = 0; j < ctar.crange.number; j++)
            {
                int octant = cs[ctar.crange.offset + j].octant;
                octant = (octant == 13 ? 0 : octant); // for an image cell, it has only one child, so we extract it from position 0
                P_children[j * num_surf + k] += buffer1[cgrid.N3 * octant + idx] / cgrid.N3 * scale;
            }
        }
    }
    fftw_destroy_plan(plan_down);
    tend(down);

    tbegin(store);
    #pragma omp parallel for
    for(int i = 0; i < num_cell; i++)
    {
        for(int k = 0; k < num_surf; k++)
        {
            cs[i].p_check[k] += P_all[i * num_surf + k];
        }
    }
    tend(store);
}

void rtfmm::LaplaceKernel::precompute(int P, real r0, int images)
{
    if(verbose) printf("precompute\n");

    /* p2m */
    std::vector<vec3r> x_check_up = get_surface_points(P, r0 * 2.95);
    std::vector<vec3r> x_equiv_up = get_surface_points(P, r0 * 1.05);
    Matrix e2c_up_precompute = get_p2p_matrix(x_equiv_up, x_check_up);
    Matrix U, S, VT;
    svd(e2c_up_precompute, U, S, VT);
    UT_p2m_precompute = transpose(U);
    V_p2m_precompute = transpose(VT);
    Sinv_p2m_precompute = pseudo_inverse(S);
    VSinv_p2m_precompute = mat_mat_mul(V_p2m_precompute, Sinv_p2m_precompute);

    /* m2m */
    std::vector<vec3r>& x_equiv_up_parent = x_equiv_up;
    std::vector<vec3r>& x_check_up_parent = x_check_up;
    Matrix& UT_m2m_precompute = UT_p2m_precompute;
    Matrix& V_m2m_precompute = V_p2m_precompute;
    Matrix& Sinv_m2m_precompute = Sinv_p2m_precompute;
    Matrix& VSinv_m2m_precompute = VSinv_p2m_precompute;
    #pragma omp parallel for
    for(int octant = 0; octant < 8; octant++)
    {
        vec3r offset_child = Tree::get_child_cell_x(vec3r(0,0,0), r0, octant);
        std::vector<vec3r> x_equiv_child_up = get_surface_points(P, r0 / 2 * 1.05, offset_child);
        Matrix ce2pc_up_precompute = get_p2p_matrix(x_equiv_child_up, x_check_up_parent);
        Matrix m1 = mat_mat_mul(UT_m2m_precompute, ce2pc_up_precompute);
        matrix_m2m[octant] = mat_mat_mul(VSinv_m2m_precompute, m1); // (VSinv)(UTG)
    }

    /* l2l */
    #pragma omp parallel for
    for(int octant = 0; octant < 8; octant++)
    {
        matrix_l2l[octant] = transpose(matrix_m2m[octant]); // (G(VSinv))UT
    }

    /* l2p */
    UT_l2p_precompute = transpose(V_p2m_precompute);
    V_l2p_precompute = transpose(UT_p2m_precompute);
    Sinv_l2p_precompute = Sinv_p2m_precompute;
    VSinv_l2p_precompute = mat_mat_mul(V_l2p_precompute, Sinv_l2p_precompute);
    VSinvUT_l2p_precompute = mat_mat_mul(VSinv_l2p_precompute, UT_l2p_precompute);
}

void rtfmm::LaplaceKernel::precompute_m2l(int P, real r0, int images)
{
    conv_grid_setting conv_grid(P, r0);
    if(verbose) std::cout<<"precompute M2L, "<<conv_grid<<std::endl;
    std::vector<real> G0(conv_grid.N3);
    std::vector<complexr> Gk0(conv_grid.N_freq);
    fftw_plan plan_G = fftw_plan_dft_r2c_3d(conv_grid.N, conv_grid.N, conv_grid.N, G0.data(), reinterpret_cast<fftw_complex*>(Gk0.data()), FFTW_ESTIMATE);
    int range = images >= 2 ? 4 : 3; // when images <=1, possible M2L range is [-3,3], otherwise [-4,4]
    if(verbose) std::cout<<"M2L precompute range="<<range<<std::endl;
    /* 
        hash(relx) -> index, 
        where relx are all possible relative positions for any M2L pair
        note, here is 'M2L pair', not 'M2l parent pair', therefore total number is at 316 instead of 26
    */
    int min_distance = 2;
    std::map<int, int> m2l_gk_relx_idx_map = get_relx_idx_map(range, min_distance);
    int m2l_gk_relx_idx_map_cnt = m2l_gk_relx_idx_map.size();
    std::vector<std::vector<rtfmm::complexr>> m2l_Gks_ordered(m2l_gk_relx_idx_map_cnt);
    #pragma omp parallel for collapse(3)
    for(int k = -range; k <= range; k++)
    {
        for(int j = -range; j <= range; j++)
        {
            for(int i = -range; i <= range; i++)
            {
                if(std::abs(i) >= min_distance || std::abs(j) >= min_distance || std::abs(k) >= min_distance)
                {
                    vec3i relative_idx(i,j,k);
                    vec3r relative_pos = vec3r(i,j,k) * r0 * 2; // relative_pos mean src's position
                    std::vector<rtfmm::vec3r> grid = get_conv_grid(conv_grid.N, conv_grid.gsize, conv_grid.delta, relative_pos);
                    std::vector<real> G = get_G_matrix(grid, conv_grid.N); 
                    std::vector<complexr> Gk(conv_grid.N_freq);                   
                    fftw_execute_dft_r2c(plan_G, G.data(), reinterpret_cast<fftw_complex*>(Gk.data()));
                    #pragma omp critical // std::map is not thread-safe
                    {
                        int hv = hash(relative_idx);
                        // since the order for generating Gk is same as indices in m2l_gk_relx_idx_map, first generated Gk is stored in [0], second Gk in [1], ...
                        // so it is called m2l_Gks_'ordered'
                        m2l_Gks_ordered[m2l_gk_relx_idx_map[hv]] = Gk; 
                    }
                }
            }
        }
    }
    fftw_destroy_plan(plan_G);

    /* 
        Q : What happened behind m2l_Gks_ordered[m2l_gk_relx_idx_map[hash(relx)]] ?
        A : relx -> index -> Gk
    */

    /* 
        Precomputation for M2L_precompute_t
        For two neighbour cells, generate(store) interaction matrix Gk for their children.
        Since each cell has at most 8 child for nonperiodic M2L, 8*8=64 matrix(real Gk or dummy matrix) will be stored for each neighbour pattern.
        Also, since there are at most 26 patterns of neighouring, the total # of matrices stored in ccGks is 26 * 64.
    */
    std::vector<vec3r> src_par_rels = get_relx<real>(1,1); // all possible relative coordinates for any M2L pair's parents
    int m2l_par_rel_num = src_par_rels.size(); // # of neighbouring patterns, 26 for nonperiodic M2L
    ccGks_8x8.resize(m2l_par_rel_num, AlignedVec(8 * 8 * conv_grid.N_freq * 2));
    #pragma omp parallel for
    for(int i = 0; i < m2l_par_rel_num; i++)
    {
        vec3r par_x = src_par_rels[i] * 2;
        for(int isrc = 0; isrc < 8; isrc++)
        {
            vec3r xsrc = Tree::get_child_cell_x(par_x,1,isrc);
            for(int itar = 0; itar < 8; itar++)
            {
                vec3r xtar = Tree::get_child_cell_x(vec3r(0,0,0),1,itar);
                vec3i dx = (xsrc - xtar).round();
                int hv = hash(dx);
                if(m2l_gk_relx_idx_map.find(hv) != m2l_gk_relx_idx_map.end())
                {
                    int ccidx = isrc * 8 + itar;
                    int gk_idx = m2l_gk_relx_idx_map[hv];
                    for(int f = 0; f < conv_grid.N_freq; f++)
                    {
                        int idx = f * 2 * 8 * 8 + ccidx * 2;
                        ccGks_8x8[i][idx + 0] = m2l_Gks_ordered[gk_idx][f].r;
                        ccGks_8x8[i][idx + 1] = m2l_Gks_ordered[gk_idx][f].i;
                    }
                }
            }
        }
    }
}

rtfmm::Matrix rtfmm::LaplaceKernel::get_p2p_matrix(
    std::vector<vec3r>& x_src, 
    std::vector<vec3r>& x_tar
)
{
    int num_src = x_src.size();
    int num_tar = x_tar.size();
    Matrix matrix_p2p(num_tar, num_src);
    for(int j = 0; j < num_tar; j++)
    {
        vec3r xtar = x_tar[j];
        for(int i = 0; i < num_src; i++)
        {
            vec3r xsrc = x_src[i];
            vec3r dx = xtar - xsrc;
            real r = dx.r();
            real invr = r == 0 ? 0 : 1 / r;
            matrix_p2p[j * num_src + i] = invr;
        }
    }
    return matrix_p2p;
}