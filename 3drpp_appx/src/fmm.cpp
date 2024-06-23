#include "fmm.h"
#include "tree.h"
#include "mathfunc.h"
#include <omp.h>
#include "surface.h"

rtfmm::LaplaceFMM::LaplaceFMM(const Bodies3& bs_, const Argument& args_) 
    : bs(bs_), args(args_)
{
    assert_exit(bs.size() == args.n, "LaplaceFMM init body size error");
}

rtfmm::Bodies3 rtfmm::LaplaceFMM::solve()
{
    /* build tree */
    tbegin(build_and_traverse);
    tbegin(build_tree);
    Tree tree;
    tree.build(bs, args.x, args.r, args.ncrit, Tree::TreeType::nonuniform);
    cs = tree.get_cells();
    if(verbose && args.check_tree) check_tree(cs);
    tend(build_tree);

    /* traverse to get interaction list */
    tbegin(traverse);
    traverser.traverse(tree, args.cycle, args.images, args.P);
    cs = traverser.get_cells();
    if(verbose && args.check_tree) check_traverser(traverser);
    if(verbose && args.check_tree) check_cells(cs);
    tree_depth_range = get_min_max_depth(cs);
    if(verbose) std::cout<<"tree_depth_range = "<<tree_depth_range<<std::endl;
    tend(traverse);
    tend(build_and_traverse);

    tbegin(init_cell_matrix);
    init_cell_matrix(cs);
    init_reg_body(cs); // init body index in image-1 cells
    tend(init_cell_matrix);

    if(args.use_precompute)
    {
        TIME_BEGIN(precompute);
        TIME_BEGIN(precompute_others);
        kernel.precompute(args.P, args.r, args.images);
        TIME_END(precompute_others);
        TIME_BEGIN(precompute_m2l);
        kernel.precompute_m2l(args.P, args.r, cs, traverser.get_map(OperatorType::M2L), args.images);
        TIME_END(precompute_m2l);
        if(args.timing) {TIME_END(precompute);}
    }

    tbegin(FMM_kernels);
    P2M();
    M2M();
    M2L();
    P2L();
    L2L();
    L2P();
    M2P();
    P2P();
    bs = sort_bodies_by_idx(bs);
    for(int i = 0; i < traverser.leaf_cell_idx.size(); i++)
    {
        int idx = traverser.leaf_cell_idx[i];
        Cell3& cell = cs[idx];
        for(int j = 0; j < cell.bodies.size(); j++)
        {
            Body3& b = cell.bodies[j];
            bs[b.idx].p += b.p;
            bs[b.idx].f += b.f;
        }
    }
    tend(FMM_kernels);

    if(args.dipole_correction)
        dipole_correction(bs, args.cycle);
        
    if(args.divide_4pi)
        scale_bodies(bs);
    //bs = sort_bodies_by_idx(bs);
    return bs;
}

void rtfmm::LaplaceFMM::P2M()
{
    Indices lcidx = get_leaf_cell_indices(cs);
    int leaf_number = lcidx.size();
    if(verbose) std::cout<<"P2M leaf_number = "<<leaf_number<<std::endl;
    TIME_BEGIN(P2M);
    #pragma omp parallel for
    for(int i = 0; i < leaf_number; i++)
    {
        Cell3& c = cs[lcidx[i]];
        if(args.use_precompute)
            kernel.p2m_precompute(args.P, c);
        else
            kernel.p2m(args.P, bs, c);
    }
    if(args.timing)
    {
        TIME_END(P2M);
    }
}

void rtfmm::LaplaceFMM::M2M()
{
    TIME_BEGIN(M2M);
    for(int depth = tree_depth_range[1]; depth > tree_depth_range[0]; depth--)
    {
        Indices nlcidx = get_nonleaf_cell_indices(cs, depth);
        int num = nlcidx.size();
        #pragma omp parallel for
        for(int i = 0; i < num; i++)
        {
            Cell3& ci = cs[nlcidx[i]];
            if(depth >= 0)
            {
                if(args.use_precompute)
                    kernel.m2m_precompute(args.P, ci, cs);
                else
                    kernel.m2m(args.P, ci, cs);
            }
            else
            {
                if(args.use_precompute)
                    kernel.m2m_img_precompute(args.P, ci, cs, args.cycle * std::pow(3, -depth - 1));
                else
                    kernel.m2m_img(args.P, ci, cs, args.cycle * std::pow(3, -depth - 1));
            }
        }
    }
    if(args.timing)
    {
        TIME_END(M2M);
    }
}

void rtfmm::LaplaceFMM::M2L()
{
    TIME_BEGIN(M2L);
    //PeriodicInteractionPairs m2l_pairs = traverser.get_pairs(OperatorType::M2L);
    //PeriodicInteractionMap m2l_map = traverser.get_map(OperatorType::M2L);
    PeriodicM2LMap m2l_parent_map = traverser.get_M2L_parent_map();
    //PeriodicInteractionMap m2l_map2 = traverser.get_m2l_map_from_m2l_parent_map();
    if(args.use_precompute)
    {
        //kernel.m2l_fft_precompute_advanced2(args.P, cs, m2l_map2);
        //kernel.m2l_fft_precompute_advanced2(args.P, cs, m2l_map);
        //kernel.m2l_fft_precompute_advanced3(args.P, cs, m2l_map, m2l_pairs);
        kernel.m2l_fft_precompute_t(args.P, cs, m2l_parent_map);
    }
    else
    {
        PeriodicInteractionMap m2l_map = traverser.get_map(OperatorType::M2L);
        for(int i = 0; i < m2l_map.size(); i++)
        {
            auto m2l_list = m2l_map.begin();
            std::advance(m2l_list, i);
            Cell3& ctar = cs[m2l_list->first];
            for(int j = 0; j < m2l_list->second.size(); j++)
            {
                Cell3& csrc = cs[m2l_list->second[j].first];
                vec3r offset_src = m2l_list->second[j].second;
                if(args.use_fft) 
                {
                    kernel.m2l_fft(args.P, csrc, ctar, offset_src);
                }
                else
                {
                    kernel.m2l(args.P, csrc, ctar, offset_src);
                }
            }
        }
    }
    if(args.timing)
    {
        TIME_END(M2L);
    }
}

void rtfmm::LaplaceFMM::L2L()
{
    TIME_BEGIN(L2L);
    for(int depth = tree_depth_range[0]; depth < tree_depth_range[1]; depth++)
    {
        Indices nlcidx = get_nonleaf_cell_indices(cs, depth);
        int num = nlcidx.size();
        #pragma omp parallel for
        for(int i = 0; i < num; i++)
        {
            Cell3& ci = cs[nlcidx[i]];
            if(args.use_precompute)
            {
                if(depth >= 0)
                    kernel.l2l_precompute(args.P, ci, cs);
                else
                    kernel.l2l_img_precompute(args.P, ci, cs);
            }
            else
                kernel.l2l(args.P, ci, cs);
        }
    }
    if(args.timing)
    {
        TIME_END(L2L);
    }
}

void rtfmm::LaplaceFMM::L2P()
{
    TIME_BEGIN(L2P);
    Indices lcidx = get_leaf_cell_indices(cs);
    int leaf_number = lcidx.size();
    if(verbose) std::cout<<"L2P leaf_number = "<<leaf_number<<std::endl;
    #pragma omp parallel for
    for(int i = 0; i < leaf_number; i++)
    {
        Cell3& c = cs[lcidx[i]];
        if(args.use_precompute)
            kernel.l2p_precompute(args.P, c);
        else
            kernel.l2p(args.P, bs, c);
    }
    if(args.timing)
    {
        TIME_END(L2P);
    }
}

void rtfmm::LaplaceFMM::P2P()
{
    TIME_BEGIN(P2P);
    PeriodicInteractionMap p2p_map = traverser.get_map(OperatorType::P2P);
    if(verbose) std::cout<<"p2p_pair.size() = "<<traverser.get_pairs(OperatorType::P2P).size()<<std::endl;
    #pragma omp parallel for
    for(int i = 0; i < p2p_map.size(); i++)
    {
        auto p2p = p2p_map.begin();
        std::advance(p2p, i);
        Cell3& ctar = cs[p2p->first];
        if(args.use_simd)
        {
            kernel.p2p_1toN_256(cs, p2p->second, ctar);
        }
        else
        {
            for(int j = 0; j < p2p->second.size(); j++)
            {
                Cell3& csrc = cs[p2p->second[j].first];
                kernel.p2p(bs,bs,csrc,ctar,p2p->second[j].second, 0);
            }
        }
    }
    if(args.timing)
    {
        TIME_END(P2P);
    }
}

void rtfmm::LaplaceFMM::M2P()
{
    TIME_BEGIN(M2P);
    PeriodicInteractionPairs m2p_pairs = traverser.get_pairs(OperatorType::M2P);
    for(auto m2p : m2p_pairs)
    {
        Cell3& ctar = cs[m2p.first];
        Cell3& csrc = cs[m2p.second.first];
        kernel.m2p(args.P, bs, csrc, ctar, m2p.second.second);
    }
    if(args.timing)
    {
        TIME_END(M2P);
    }
}

void rtfmm::LaplaceFMM::P2L()
{
    TIME_BEGIN(P2L);
    PeriodicInteractionPairs p2l_pairs = traverser.get_pairs(OperatorType::P2L);
    for(auto p2l : p2l_pairs)
    {
        Cell3& ctar = cs[p2l.first];
        Cell3& csrc = cs[p2l.second.first];
        kernel.p2l(args.P, bs, csrc, ctar, p2l.second.second);
    }
    if(args.timing)
    {
        TIME_END(P2L);
    }
}

void rtfmm::LaplaceFMM::init_cell_matrix(Cells3& cells)
{
    for(int i = 0; i < cells.size(); i++)
    {
        Cell3& cell = cells[i];
        cell.q_equiv = Matrix(get_surface_point_num(args.P), 1);
        cell.p_check = Matrix(get_surface_point_num(args.P), 1);
    }
}

void rtfmm::LaplaceFMM::init_reg_body(Cells3& cells)
{
    int range = 0;
    for(int pz = -range; pz <= range; pz++)
    {
        for(int py = -range; py <= range; py++)
        {
            for(int px = -range; px <= range; px++)
            {
                for(int i = 0; i < bs.size(); i++)
                {
                    vec3r per = args.cycle * vec3r(px, py, pz);
                    vec3r ploc = bs[i].x + per;
                    for(int c = 0; c < cells.size(); c++)
                    {
                        Cell3& cell = cells[c];
                        if(cell.crange.number == 0 && !(i >= cell.brange.offset && i < cell.brange.offset + cell.brange.number))
                        {
                            vec3r dx = ploc - cell.x;
                            real w = get_w(dx, cell.r, args.rega);
                            if(w > 0)
                            {
                                cell.reg_body_idx.push_back(std::make_pair(i, per));
                            }
                        }
                    }
                }
            }
        }
    }

    //for(int c = 0; c < cells.size(); c++)
    std::vector<int> leaves = traverser.leaf_cell_idx;
    for(int leaf_idx = 0; leaf_idx < leaves.size(); leaf_idx++)
    {
        int c = leaves[leaf_idx];
        Cell3& cell = cells[c];
        for(int i = cell.brange.offset; i < cell.brange.offset + cell.brange.number; i++)
        {
            Body3 body = bs[i];
            vec3r dx = body.x - cell.x;
            real w = get_w(dx, cell.r, args.rega);
            if(w > 0)
            {
                vec3r temp = (body.x - args.x).abs() + vec3r(1,1,1) * args.rega;
                if(temp[0] >= args.r || temp[1] >= args.r || temp[2] >= args.r)
                {
                    printf("body %d %d\n", body.idx, i);
                    w = 1;
                }
                cell.bodies.push_back(body);
                cell.weights.push_back(w);
                if(w != 1)
                {
                    printf("inside body%d,%d w = %.4f c=%d,%d\n", body.idx,i, w, c, cell.crange.number);
                }
            }
        }
        for(int i = 0; i < cell.reg_body_idx.size(); i++)
        {
            Body3 body = bs[cell.reg_body_idx[i].first];
            body.x += cell.reg_body_idx[i].second; // move to image position
            vec3r dx = body.x - cell.x;
            real w = get_w(dx, cell.r, args.rega);
            if(w > 0)
            {
                cell.bodies.push_back(body);
                cell.weights.push_back(w);
                if(w != 1)
                {
                    printf("outside body%d,%d w = %.4f c=%d\n", body.idx,i, w, c);
                }
            }
        }
    }
}

rtfmm::vec2i rtfmm::LaplaceFMM::get_min_max_depth(const Cells3& cells)
{
    vec2i res = {cells[0].depth, cells[0].depth};
    for(int i = 0; i < cells.size(); i++)
    {
        res[0] = std::min(res[0], cells[i].depth);
        res[1] = std::max(res[1], cells[i].depth);
    }
    return res;
}

rtfmm::Indices rtfmm::LaplaceFMM::get_leaf_cell_indices(const Cells3& cells)
{
    Indices res;
    for(int i = 0; i < cells.size(); i++)
    {
        if(cells[i].crange.number == 0)
        {
            res.push_back(i);
        }
    }
    return res;
}

rtfmm::Indices rtfmm::LaplaceFMM::get_nonleaf_cell_indices(const Cells3& cells, int depth)
{
    Indices res;
    for(int i = 0; i < cells.size(); i++)
    {
        if(cells[i].depth == depth && cells[i].crange.number > 0)
        {
            res.push_back(i);
        }
    }
    return res;
}

void rtfmm::LaplaceFMM::check_tree(const Cells3& cells)
{
    printf("check tree\n");
    printf("cells.size() = %ld\n", cells.size());
    int bcnt = 0;
    for(int i = 0; i < cells.size(); i++)
    {
        Cell3 c = cells[i];
        for(int j = 0; j < c.brange.number; j++)
        {
            Body3 b = bs[c.brange.offset + j];
            vec3r dx = (b.x - c.x).abs();
            assert_exit(dx[0] <= c.r && dx[1] <= c.r && dx[2] <= c.r, "tree body range error");
        }
        if(c.crange.number == 0)
        {
            bcnt += c.brange.number;
        }
    }
    assert_exit(bcnt == bs.size(), "tree body sum error");
}

void rtfmm::LaplaceFMM::check_traverser(Traverser& traverser)
{
    PeriodicInteractionPairs P2P_pairs = traverser.get_pairs(OperatorType::P2P);
    PeriodicInteractionPairs M2L_pairs = traverser.get_pairs(OperatorType::M2L);
    PeriodicInteractionPairs M2P_pairs = traverser.get_pairs(OperatorType::M2P);
    PeriodicInteractionPairs P2L_pairs = traverser.get_pairs(OperatorType::P2L);

    int p2p_num = P2P_pairs.size();
    int m2l_num = M2L_pairs.size();
    int m2p_num = M2P_pairs.size();
    int p2l_num = P2L_pairs.size();

    std::cout<<"P2P_pair# : "<<p2p_num<<std::endl;
    std::cout<<"M2L_pair# : "<<m2l_num<<std::endl;
    std::cout<<"M2P_pair# : "<<m2p_num<<std::endl;
    std::cout<<"P2L_pair# : "<<p2l_num<<std::endl;
    std::cout<<"sum# : "<<p2p_num+m2l_num+m2p_num+p2l_num<<std::endl;

    // check pairs
    if(args.images == 0)
    {
        for(auto p2p : P2P_pairs)
        {
            int found = 0;
            for(auto p2p2 : P2P_pairs)
            {
                if(p2p.first == p2p2.second.first && p2p.second.first == p2p2.first)
                {
                    found++;
                }
            }
            assert_exit(found == 1, "treep p2p error");
        }
        std::cout<<"p2p check ok"<<std::endl;
        for(auto m2l : M2L_pairs)
        {
            int found = 0;
            for(auto m2l2 : M2L_pairs)
            {
                if(m2l.first == m2l2.second.first && m2l.second.first == m2l2.first)
                {
                    found++;
                }
            }
            assert_exit(found == 1, "treep m2l error");
        }
        std::cout<<"m2l check ok"<<std::endl;
    }

    for(auto m2p : M2P_pairs)
    {
        int found = 0;
        for(auto p2l : P2L_pairs)
        {
            if(m2p.first == p2l.second.first && m2p.second.first == p2l.first)
            {
                found++;
            }
        }
        assert_exit(found == 1, "treep p2l error");
    }
    std::cout<<"m2p check ok"<<std::endl;

    for(auto p2l : P2L_pairs)
    {
        int found = 0;
        for(auto m2p : M2P_pairs)
        {
            if(m2p.first == p2l.second.first && m2p.second.first == p2l.first)
            {
                found++;
            }
        }
        assert_exit(found == 1, "treep m2p error");
    }
    std::cout<<"p2l check ok"<<std::endl;
}

void rtfmm::LaplaceFMM::check_cells(const Cells3& cells)
{
    std::cout<<"check cells"<<std::endl;
    for(int i = 0; i < cells.size(); i++)
    {
        Cell3 c = cells[i];
        assert_exit(c.idx == i, "cell idx error");
    }
}

rtfmm::real rtfmm::LaplaceFMM::reg_w(real x)
{
    return 0.25 * (2 + 3 * x - x * x * x);
}

rtfmm::real rtfmm::LaplaceFMM::get_w_single(real dx, real R, real rega)
{
    real r = std::abs(dx) - R + rega;
    real x;
    if(r <= 0) x = 1;
    else if(r > 2 * rega) x = -1;
    else x = 1 - r / rega;
    return reg_w(x);
}

rtfmm::real rtfmm::LaplaceFMM::get_w(vec3r dx, real R, real rega)
{
    real w = 1;
    for(int d = 0; d < 3; d++)
    {
        w *= get_w_single(dx[d], R, rega);
    }
    return w;
}