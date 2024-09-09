#pragma once

#include "type.h"
#include "body.h"
#include "tree.h"
#include "fmm.h"
#include "reg.h"

namespace rtfmm
{

void make_tar_cell(Cell3& cell_tar, const Bodies3& bs, vec3r root_x, real root_r)
{
    cell_tar.x = root_x;
    cell_tar.r = root_r;
    cell_tar.bodies = bs;
}

void make_reg_tar_cell(Cell3& cell_tar, const Bodies3& bs, vec3r root_x, real root_r, real cycle, real rega)
{
    cell_tar.x = root_x;
    cell_tar.r = root_r;
    
    for(int i = 0; i < bs.size(); i++)
    {
        const Body3& b = bs[i];
        real w = reg::get_w_xyz(b.x - cell_tar.x, cell_tar.r, rega).mul();
        cell_tar.bodies.push_back(b);
        cell_tar.weights.push_back(w);
        if(w != 1)
        {
            for(int z = -1; z <= 1; z++)
            {
                for(int y = -1; y <= 1; y++)
                {
                    for(int x = -1; x <= 1; x++)
                    {
                        vec3r offset = vec3r(x,y,z) * cycle;
                        if(offset != vec3r(0,0,0))
                        {
                            Body3 bb = b;
                            bb.x += offset;
                            real w = reg::get_w_xyz(bb.x - cell_tar.x, cell_tar.r, rega).mul();
                            if(w > 0)
                            {
                                cell_tar.bodies.push_back(bb);
                                cell_tar.weights.push_back(w);
                            }
                        }
                    }
                }
            }
        }
    }
}
    
void make_src_cell(Cell3& cell_src, const Bodies3& bs, vec3r root_x, real root_r)
{
    cell_src.x = root_x;
    cell_src.r = root_r;
    cell_src.bodies = bs;
}

int is_direction(const vec3r& x, const vec3r& dir)
{
    int res = 0;
    for(int i = 0; i < 3; i++)
    {
        if(dir[i] == 0) continue;
        if(x[i] * dir[i] > 0)
        {
            res = 1;
            break;
        }
    }
    return res;
}

void partial_regularization(Cell3& cell, const Bodies3& bs, const real& rega, const vec3r& direction, const vec3r& bshift)
{
    vec3r reverse_direction = -direction;
    for(int i = 0; i < bs.size(); i++)
    {
        Body3 b = bs[i];
        b.x += bshift;
        vec3r dx = b.x - cell.x;
        real w = reg::get_w_xyz(dx, cell.r, rega).mul();
        if(w == 1)
        {
            cell.bodies.push_back(b);
            cell.weights.push_back(1);
        }
        else if (w > 0)
        {
            // if body is at same direction
            //if(dx[0] * direction[0] >= 0 && dx[1] * direction[1] >= 0 && dx[2] * direction[2] >= 0)  
            if(is_direction(dx, direction))
            {
                cell.bodies.push_back(b);
                cell.weights.push_back(w);
            }
            else
            {
                cell.bodies.push_back(b);
                cell.weights.push_back(1);
                // if body is at reversed direction
                //if(dx[0] * reverse_direction[0] >= 0 && dx[1] * reverse_direction[1] >= 0 && dx[2] * reverse_direction[2] >= 0) 
                if(is_direction(dx, reverse_direction))
                {
                    Body3 bb = b;
                    bb.x += direction;
                    real ww = reg::get_w_xyz(bb.x - cell.x, cell.r, rega).mul();
                    cell.bodies.push_back(bb);
                    cell.weights.push_back(ww);
                }
            }
        }
    }
}

void make_reg_src_cell(Cells3& cell_srcs, const Bodies3& bs, vec3r root_x, real root_r, real cycle, real rega, int image)
{
    if(image == 0)
    {
        cell_srcs.push_back(Cell3());
        make_reg_tar_cell(cell_srcs[0], bs, root_x, root_r, cycle, rega); // when image is 0, src and tar are same, see #1
    }
    else if(image == 1)
    {
        // 1. add root
        cell_srcs.push_back(Cell3());
        cell_srcs[0].x = root_x;
        cell_srcs[0].r = root_r;
        for(int i = 0; i < bs.size(); i++)
        {
            const Body3& b = bs[i];
            cell_srcs[0].bodies.push_back(b);
            cell_srcs[0].weights.push_back(1);
        }

        // 2. add partial regularized boxes
        for(int z = -1; z <= 1; z++)
        {
            for(int y = -1; y <= 1; y++)
            {
                for(int x = -1; x <= 1; x++)
                {
                    if(x != 0 || y != 0 || z != 0)
                    {
                        RTLOG("add partial regularized cell (%d,%d,%d)\n", x,y,z);
                        // create partial cell/box
                        vec3r offset = vec3r(x,y,z) * cycle;
                        Cell3 cell_partial;
                        cell_partial.x = root_x + offset;
                        cell_partial.r = root_r;
                        partial_regularization(cell_partial, bs, rega, offset, offset);
                        cell_srcs.push_back(cell_partial);
                    }
                }
            }
        }
    }
    else if(image >= 2)
    {

    }
    else
    {
        RTLOG("image should be larger than 0!\n");
        exit(1);
    }
}

void fusion_bodies(Cell3& cell_tar, Bodies3& res)
{
    int bnum = cell_tar.bodies.size();
    for(int i = 0; i < bnum; i++)
    {
        Body3& b = cell_tar.bodies[i];
        res[b.idx].p += b.p;
        res[b.idx].f += b.f;
    }
}

}