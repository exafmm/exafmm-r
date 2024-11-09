#include "gtest/gtest.h"
#include <stdexcept>
#include <iostream>
#include "type.h"
#include "body.h"
#include "tree.h"
#include "fmm.h"
#include "argument.h"
#include "ewald.h"
#include <omp.h>


unsigned int random_seed = 5;
rtfmm::Argument env_args;
#define GTEST_WARNING std::cerr << "\u001b[33m[ WARN     ] \u001b[0m" << std::flush
#define GTEST_COUT std::cerr << "[          ] " << std::flush

TEST(FmmTest, basic) 
{
    rtfmm::Argument args;

    //rtfmm::verbose = 1;
    args.n = 1000;
    args.P = 4;
    args.images = 0;
    args.rega = 0;
    args.seed = random_seed;
    if(env_args.override_gtest_setting)
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    if(args.body0_idx != -1)
    {
        RTLOG("move body %d\n", args.body0_idx);
        bs[args.body0_idx].x = rtfmm::vec3r(args.x0, args.y0, args.z0);
    }

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve by ewald */
    rtfmm::Bodies3 res_ewald;
    if(args.enable_ewald)
    {
        rtfmm::EwaldSolver ewald(bs, args);
        TIME_BEGIN(EWALD);
        res_ewald = ewald.solve();
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_ewald);
        if(args.timing) {TIME_END_stdout(EWALD);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(args.print_body_number)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
        if(args.enable_ewald) rtfmm::print_bodies(res_ewald, args.print_body_number, 0, "ewald");
    }
    if(args.body0_idx != -1)
    {
        RTLOG("check : ");
        rtfmm::Body3& cb = res_fmm[args.check_body_idx];
        GTEST_COUT << "idx=" << cb.idx << ","
                   << "p=" << cb.p << ","
                   << "f=" << cb.f << std::endl;
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
    }
    if(args.enable_fmm && args.enable_ewald)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_ewald, "FMM", "Ewald", args.num_compare);
        res.show();
    }
    if(args.enable_direct && args.enable_ewald)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_direct, res_ewald, "Direct", "Ewald", args.num_compare);
        res.show();
    }
}

TEST(FmmTest, n100_p4) 
{
    rtfmm::Argument args;

    args.n = 1000;
    args.P = 4;
    args.images = 0;
    args.rega = 0;
    args.seed = random_seed;
    if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve by ewald */
    rtfmm::Bodies3 res_ewald;
    if(args.enable_ewald)
    {
        rtfmm::EwaldSolver ewald(bs, args);
        TIME_BEGIN(EWALD);
        res_ewald = ewald.solve();
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_ewald);
        if(args.timing) {TIME_END_stdout(EWALD);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
        if(args.enable_ewald) rtfmm::print_bodies(res_ewald, args.print_body_number, 0, "ewald");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 1.1e-4);
        EXPECT_LE(res.l2e, 2.9e-4);
    }
    if(args.enable_fmm && args.enable_ewald)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_ewald, "FMM", "Ewald", args.num_compare);
        res.show();
    }
    if(args.enable_direct && args.enable_ewald)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_direct, res_ewald, "Direct", "Ewald", args.num_compare);
        res.show();
    }
}

TEST(FmmTest, n24000_p6_noreg) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 6;
    args.images = 0;
    args.rega = 0;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.enable_ewald = 0;
    args.seed = random_seed;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve by ewald */
    rtfmm::Bodies3 res_ewald;
    if(args.enable_ewald)
    {
        rtfmm::EwaldSolver ewald(bs, args);
        TIME_BEGIN(EWALD);
        res_ewald = ewald.solve();
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_ewald);
        if(args.timing) {TIME_END_stdout(EWALD);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
        if(args.enable_ewald) rtfmm::print_bodies(res_ewald, args.print_body_number, 0, "ewald");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
    if(args.enable_fmm && args.enable_ewald)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_ewald, "FMM", "Ewald", args.num_compare);
        res.show();
    }
    if(args.enable_direct && args.enable_ewald)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_direct, res_ewald, "Direct", "Ewald", args.num_compare);
        res.show();
    }
}

TEST(FmmTest, n24000_p10_image5) 
{
    rtfmm::Argument args;

    args.enable_direct = 0;
    args.n = 24000;
    args.num_compare = 24000;
    args.P = 10;
    args.images = 5;
    args.rega = 0;
    args.ncrit = 128;
    args.dipole_correction = 1;
    args.zero_netcharge = 1;
    args.divide_4pi = 0;
    args.setting_t = 0;
    args.seed = random_seed;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve by ewald */
    rtfmm::Bodies3 res_ewald;
    if(args.enable_ewald)
    {
        rtfmm::EwaldSolver ewald(bs, args);
        TIME_BEGIN(EWALD);
        res_ewald = ewald.solve();
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_ewald);
        if(args.timing) {TIME_END_stdout(EWALD);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
        if(args.enable_ewald) rtfmm::print_bodies(res_ewald, args.print_body_number, 0, "ewald");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
    }
    if(args.enable_fmm && args.enable_ewald)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_ewald, "FMM", "Ewald", args.num_compare);
        EXPECT_LE(res.l2f, 3e-7);
        EXPECT_LE(res.l2e, 2e-5);
        res.show();
    }
    if(args.enable_direct && args.enable_ewald)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_direct, res_ewald, "Direct", "Ewald", args.num_compare);
        res.show();
    }
}

TEST(FmmTest, n24000_p6_reg0001) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 6;
    args.images = 0;
    args.rega = 0.001;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p6_reg001) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 6;
    args.images = 0;
    args.rega = 0.01;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    rtfmm::verbose = 1;
    args.seed = random_seed;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p6_reg0025) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 6;
    args.images = 0;
    args.rega = 0.025;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p6_reg005) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 6;
    args.images = 0;
    args.rega = 0.05;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p6_reg0075) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 6;
    args.images = 0;
    args.rega = 0.075;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p6_reg01) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 6;
    args.images = 0;
    args.rega = 0.1;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p6_reg015) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 6;
    args.images = 0;
    args.rega = 0.15;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p7_noreg) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 7;
    args.images = 0;
    args.rega = 0.0;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    //rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p7_reg0001) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 7;
    args.images = 0;
    args.rega = 0.001;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p7_reg001) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 7;
    args.images = 0;
    args.rega = 0.01;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    rtfmm::verbose = 1;
    args.seed = random_seed;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p7_reg0025) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 7;
    args.images = 0;
    args.rega = 0.025;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p7_reg005) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 7;
    args.images = 0;
    args.rega = 0.05;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p7_reg0075) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 7;
    args.images = 0;
    args.rega = 0.075;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p7_reg01) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 7;
    args.images = 0;
    args.rega = 0.1;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p7_reg015) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 7;
    args.images = 0;
    args.rega = 0.15;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p8_noreg) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 8;
    args.images = 0;
    args.rega = 0.0;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    //rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p8_reg0001) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 8;
    args.images = 0;
    args.rega = 0.001;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p8_reg001) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 8;
    args.images = 0;
    args.rega = 0.01;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    rtfmm::verbose = 1;
    args.seed = random_seed;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p8_reg0025) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 8;
    args.images = 0;
    args.rega = 0.025;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p8_reg005) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 8;
    args.images = 0;
    args.rega = 0.05;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p8_reg0075) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 8;
    args.images = 0;
    args.rega = 0.075;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p8_reg01) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 8;
    args.images = 0;
    args.rega = 0.1;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
    rtfmm::verbose = 1;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

TEST(FmmTest, n24000_p8_reg015) 
{
    rtfmm::Argument args;

    args.n = 24000;
    args.num_compare = 24000;
    args.P = 8;
    args.images = 0;
    args.rega = 0.15;
    args.ncrit = 128;
    args.dipole_correction = 0;
    args.zero_netcharge = 0;
    args.divide_4pi = 1;
    args.setting_t = 1;
    args.seed = random_seed;
if(env_args.override_gtest_setting) 
    { 
        GTEST_WARNING << "default settings were overrided!\n"; 
        args=env_args;
    }
    args.show();

    omp_set_dynamic(0);
    omp_set_num_threads(args.th_num);
    RTLOG("# of threads = %d\n", omp_get_max_threads());

    /* prepare bodies */
    rtfmm::Bodies3 bs = rtfmm::generate_random_bodies(args.n, args.r, args.x, args.seed, args.zero_netcharge);

    /* solve by FMM */
    rtfmm::Bodies3 res_fmm;
    if(args.enable_fmm)
    {
        rtfmm::LaplaceFMM fmm(bs, args);
        TIME_BEGIN(FMM);
        res_fmm = fmm.solve();
        if(args.timing) {TIME_END_stdout(FMM);}
    }

    /* solve directly */
    rtfmm::Bodies3 res_direct = bs;
    if(args.enable_direct)
    {
        rtfmm::LaplaceKernel kernel;
        rtfmm::Cell3 cell_src;
        cell_src.brange = {0, args.n};
        rtfmm::Cell3 cell_tar;
        cell_tar.brange = {0, args.num_compare};
        TIME_BEGIN(DIRECT);
        kernel.direct(res_direct, res_direct, args.images, args.cycle);
        if(args.dipole_correction)
            rtfmm::dipole_correction(res_direct, args.cycle);
        if(args.divide_4pi)
            rtfmm::scale_bodies(res_direct);
        if(args.timing) {TIME_END_stdout(DIRECT);}
    }

    /* compare */
    if(rtfmm::verbose)
    {
        if(args.enable_fmm) rtfmm::print_bodies(res_fmm, args.print_body_number, 0, "fmm");
        if(args.enable_direct) rtfmm::print_bodies(res_direct, args.print_body_number, 0, "direct");
    }
    if(args.enable_fmm && args.enable_direct)
    {
        rtfmm::BodyCompareResult res = rtfmm::compare(res_fmm, res_direct, "FMM", "Direct", args.num_compare);
        res.show();
        EXPECT_LE(res.l2f, 5e-6);
        EXPECT_LE(res.l2e, 5e-7);
    }
}

int main(int argc, char** argv)
{
    testing::InitGoogleTest(&argc, argv);
    env_args = rtfmm::Argument(argc, argv);
    return RUN_ALL_TESTS();
}