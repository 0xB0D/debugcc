// SPDX-License-Identifier: BSD-3-Clause
/* Copyright (c) 2022, Linaro Limited */

#include <sys/mman.h>
#include <err.h>
#include <fcntl.h>
#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "debugcc.h"

static struct debug_mux cpu_cc = {
	.phys = 0xfaa0000,
	.size = 0x1000,
	.block_name = "cpu",

	.enable_reg = 0x18,
	.enable_mask = BIT(0),

	.mux_reg = 0x18,
	.mux_mask = 0x7f << 4,
	.mux_shift = 4,

	.div_reg = 0x18,
	.div_mask = 0xf << 11,
	.div_shift = 11,
	.div_val = 8,
};

static struct debug_mux disp_cc = {
	.phys = 0x5f00000,
	.size = 0x20000,
	.block_name = "disp",

	.enable_reg = 0x3004,
	.enable_mask = BIT(0),

	.mux_reg = 0x7000,
	.mux_mask = 0xff,

	.div_reg = 0x3000,
	.div_mask = 0xf,
	.div_val = 4,
};

static struct debug_mux gcc = {
	.phys = 0x1400000,
	.size = 0x1f0000,

	.enable_reg = 0x30004,
	.enable_mask = BIT(0),

	.mux_reg = 0x62000,
	.mux_mask = 0x3ff,

	.div_reg = 0x30000,
	.div_mask = 0xf,
	.div_val = 1,

	.xo_div4_reg = 0x28008,
	.debug_ctl_reg = 0x62038,
	.debug_status_reg = 0x6203c,
};

static struct debug_mux gpu_cc = {
	.phys = 0x5990000,
	.size = 0x9000,
	.block_name = "gpu",

	.enable_reg = 0x1100,
	.enable_mask = BIT(0),

	.mux_reg = 0x1568,
	.mux_mask = 0xff,

	.div_reg = 0x10fc,
	.div_mask = 0xf,
	.div_val = 2,
};

static struct debug_mux mc_cc = {
	/* It's really <0x447d200 0x100>, but we need to reserve a whole 4096-long page.. */
	.phys = 0x447d000,
	.size = 0x1000,
	.block_name = "mc",

	.measure = measure_mccc,
};

static struct measure_clk sm6375_clocks[] = {
	{ "l3_clk", &gcc, 0xbf, &cpu_cc, 0x41 },
	{ "perfcl_clk", &gcc, 0xbf, &cpu_cc, 0x25 },
	{ "pwrcl_clk", &gcc, 0xbf, &cpu_cc, 0x21 },
	//{ "cpu_cc_debug_mux", &gcc, 0xbf },
	//{ "disp_cc_debug_mux", &gcc, 0x43 },
	{ "gcc_ahb2phy_csi_clk", &gcc, 0x67 },
	{ "gcc_ahb2phy_usb_clk", &gcc, 0x68 },
	{ "gcc_bimc_gpu_axi_clk", &gcc, 0x9d },
	{ "gcc_boot_rom_ahb_clk", &gcc, 0x84 },
	{ "gcc_cam_throttle_nrt_clk", &gcc, 0x4d },
	{ "gcc_cam_throttle_rt_clk", &gcc, 0x4c },
	{ "gcc_camss_axi_clk", &gcc, 0x154 },
	{ "gcc_camss_cci_0_clk", &gcc, 0x151 },
	{ "gcc_camss_cci_1_clk", &gcc, 0x152 },
	{ "gcc_camss_cphy_0_clk", &gcc, 0x140 },
	{ "gcc_camss_cphy_1_clk", &gcc, 0x141 },
	{ "gcc_camss_cphy_2_clk", &gcc, 0x142 },
	{ "gcc_camss_cphy_3_clk", &gcc, 0x143 },
	{ "gcc_camss_csi0phytimer_clk", &gcc, 0x130 },
	{ "gcc_camss_csi1phytimer_clk", &gcc, 0x131 },
	{ "gcc_camss_csi2phytimer_clk", &gcc, 0x132 },
	{ "gcc_camss_csi3phytimer_clk", &gcc, 0x133 },
	{ "gcc_camss_mclk0_clk", &gcc, 0x134 },
	{ "gcc_camss_mclk1_clk", &gcc, 0x135 },
	{ "gcc_camss_mclk2_clk", &gcc, 0x136 },
	{ "gcc_camss_mclk3_clk", &gcc, 0x137 },
	{ "gcc_camss_mclk4_clk", &gcc, 0x138 },
	{ "gcc_camss_nrt_axi_clk", &gcc, 0x158 },
	{ "gcc_camss_ope_ahb_clk", &gcc, 0x150 },
	{ "gcc_camss_ope_clk", &gcc, 0x14e },
	{ "gcc_camss_rt_axi_clk", &gcc, 0x15a },
	{ "gcc_camss_tfe_0_clk", &gcc, 0x139 },
	{ "gcc_camss_tfe_0_cphy_rx_clk", &gcc, 0x13d },
	{ "gcc_camss_tfe_0_csid_clk", &gcc, 0x144 },
	{ "gcc_camss_tfe_1_clk", &gcc, 0x13a },
	{ "gcc_camss_tfe_1_cphy_rx_clk", &gcc, 0x13e },
	{ "gcc_camss_tfe_1_csid_clk", &gcc, 0x146 },
	{ "gcc_camss_tfe_2_clk", &gcc, 0x13b },
	{ "gcc_camss_tfe_2_cphy_rx_clk", &gcc, 0x13f },
	{ "gcc_camss_tfe_2_csid_clk", &gcc, 0x148 },
	{ "gcc_camss_top_ahb_clk", &gcc, 0x153 },
	{ "gcc_cfg_noc_usb3_prim_axi_clk", &gcc, 0x1f },
	{ "gcc_disp_gpll0_div_clk_src", &gcc, 0x48 },
	{ "gcc_disp_hf_axi_clk", &gcc, 0x3e },
	{ "gcc_disp_sleep_clk", &gcc, 0x4e },
	{ "gcc_disp_throttle_core_clk", &gcc, 0x4a },
	{ "gcc_gp1_clk", &gcc, 0xca },
	{ "gcc_gp2_clk", &gcc, 0xcb },
	{ "gcc_gp3_clk", &gcc, 0xcc },
	{ "gcc_gpu_gpll0_clk_src", &gcc, 0xff },
	{ "gcc_gpu_gpll0_div_clk_src", &gcc, 0x100 },
	{ "gcc_gpu_memnoc_gfx_clk", &gcc, 0xfc },
	{ "gcc_gpu_snoc_dvm_gfx_clk", &gcc, 0xfe },
	{ "gcc_gpu_throttle_core_clk", &gcc, 0x103 },
	{ "gcc_pdm2_clk", &gcc, 0x81 },
	{ "gcc_pdm_ahb_clk", &gcc, 0x7f },
	{ "gcc_pdm_xo4_clk", &gcc, 0x80 },
	{ "gcc_prng_ahb_clk", &gcc, 0x82 },
	{ "gcc_qmip_camera_nrt_ahb_clk", &gcc, 0x3b },
	{ "gcc_qmip_camera_rt_ahb_clk", &gcc, 0x49 },
	{ "gcc_qmip_disp_ahb_clk", &gcc, 0x3c },
	{ "gcc_qmip_gpu_cfg_ahb_clk", &gcc, 0x101 },
	{ "gcc_qmip_video_vcodec_ahb_clk", &gcc, 0x3a },
	{ "gcc_qupv3_wrap0_core_2x_clk", &gcc, 0x6e },
	{ "gcc_qupv3_wrap0_core_clk", &gcc, 0x6d },
	{ "gcc_qupv3_wrap0_s0_clk", &gcc, 0x6f },
	{ "gcc_qupv3_wrap0_s1_clk", &gcc, 0x70 },
	{ "gcc_qupv3_wrap0_s2_clk", &gcc, 0x71 },
	{ "gcc_qupv3_wrap0_s3_clk", &gcc, 0x72 },
	{ "gcc_qupv3_wrap0_s4_clk", &gcc, 0x73 },
	{ "gcc_qupv3_wrap0_s5_clk", &gcc, 0x74 },
	{ "gcc_qupv3_wrap1_core_2x_clk", &gcc, 0x78 },
	{ "gcc_qupv3_wrap1_core_clk", &gcc, 0x77 },
	{ "gcc_qupv3_wrap1_s0_clk", &gcc, 0x79 },
	{ "gcc_qupv3_wrap1_s1_clk", &gcc, 0x7a },
	{ "gcc_qupv3_wrap1_s2_clk", &gcc, 0x7b },
	{ "gcc_qupv3_wrap1_s3_clk", &gcc, 0x7c },
	{ "gcc_qupv3_wrap1_s5_clk", &gcc, 0x7e },
	{ "gcc_qupv3_wrap_0_m_ahb_clk", &gcc, 0x6b },
	{ "gcc_qupv3_wrap_0_s_ahb_clk", &gcc, 0x6c },
	{ "gcc_sdcc1_ahb_clk", &gcc, 0x108 },
	{ "gcc_sdcc1_apps_clk", &gcc, 0x107 },
	{ "gcc_sdcc1_ice_core_clk", &gcc, 0x109 },
	{ "gcc_sdcc2_ahb_clk", &gcc, 0x6a },
	{ "gcc_sdcc2_apps_clk", &gcc, 0x69 },
	{ "gcc_sys_noc_cpuss_ahb_clk", &gcc, 0x9 },
	{ "gcc_sys_noc_ufs_phy_axi_clk", &gcc, 0x1b },
	{ "gcc_sys_noc_usb3_prim_axi_clk", &gcc, 0x1a },
	{ "gcc_ufs_phy_ahb_clk", &gcc, 0x127 },
	{ "gcc_ufs_phy_axi_clk", &gcc, 0x126 },
	{ "gcc_ufs_phy_ice_core_clk", &gcc, 0x12d },
	{ "gcc_ufs_phy_phy_aux_clk", &gcc, 0x12e },
	{ "gcc_ufs_phy_rx_symbol_0_clk", &gcc, 0x129 },
	{ "gcc_ufs_phy_tx_symbol_0_clk", &gcc, 0x128 },
	{ "gcc_ufs_phy_unipro_core_clk", &gcc, 0x12c },
	{ "gcc_usb30_prim_master_clk", &gcc, 0x5e },
	{ "gcc_usb30_prim_mock_utmi_clk", &gcc, 0x60 },
	{ "gcc_usb30_prim_sleep_clk", &gcc, 0x5f },
	{ "gcc_usb3_prim_phy_com_aux_clk", &gcc, 0x61 },
	{ "gcc_usb3_prim_phy_pipe_clk", &gcc, 0x62 },
	{ "gcc_vcodec0_axi_clk", &gcc, 0x160 },
	{ "gcc_venus_ahb_clk", &gcc, 0x161 },
	{ "gcc_venus_ctl_axi_clk", &gcc, 0x15f },
	{ "gcc_video_axi0_clk", &gcc, 0x3d },
	{ "gcc_video_throttle_core_clk", &gcc, 0x4b },
	{ "gcc_video_vcodec0_sys_clk", &gcc, 0x15d },
	{ "gcc_video_venus_ctl_clk", &gcc, 0x15b },
	{ "gcc_video_xo_clk", &gcc, 0x3f },
	//{ "gpu_cc_debug_mux", &gcc, 0xfb },
	//{ "mc_cc_debug_mux", &gcc, 0xae },
	{ "measure_only_cnoc_clk", &gcc, 0x1d },
	{ "measure_only_gcc_camera_ahb_clk", &gcc, 0x38 },
	{ "measure_only_gcc_camera_xo_clk", &gcc, 0x40 },
	{ "measure_only_gcc_cpuss_gnoc_clk", &gcc, 0xba },
	{ "measure_only_gcc_disp_ahb_clk", &gcc, 0x39 },
	{ "measure_only_gcc_disp_xo_clk", &gcc, 0x41 },
	{ "measure_only_gcc_gpu_cfg_ahb_clk", &gcc, 0xf9 },
	{ "measure_only_gcc_qupv3_wrap1_s4_clk", &gcc, 0x7d },
	{ "measure_only_gcc_qupv3_wrap_1_m_ahb_clk", &gcc, 0x75 },
	{ "measure_only_gcc_qupv3_wrap_1_s_ahb_clk", &gcc, 0x76 },
	{ "measure_only_gcc_video_ahb_clk", &gcc, 0x37 },
	{ "measure_only_hwkm_ahb_clk", &gcc, 0x166 },
	{ "measure_only_hwkm_km_core_clk", &gcc, 0x167 },
	{ "measure_only_ipa_2x_clk", &gcc, 0xd7 },
	{ "measure_only_pka_ahb_clk", &gcc, 0x162 },
	{ "measure_only_pka_core_clk", &gcc, 0x163 },
	{ "measure_only_snoc_clk", &gcc, 0x7 },

	{ "disp_cc_mdss_ahb_clk", &gcc, 0x43, &disp_cc, 0x14 },
	{ "disp_cc_mdss_byte0_clk", &gcc, 0x43, &disp_cc, 0xc },
	{ "disp_cc_mdss_byte0_intf_clk", &gcc, 0x43, &disp_cc, 0xd },
	{ "disp_cc_mdss_esc0_clk", &gcc, 0x43, &disp_cc, 0xe },
	{ "disp_cc_mdss_mdp_clk", &gcc, 0x43, &disp_cc, 0x8 },
	{ "disp_cc_mdss_mdp_lut_clk", &gcc, 0x43, &disp_cc, 0xa },
	{ "disp_cc_mdss_non_gdsc_ahb_clk", &gcc, 0x43, &disp_cc, 0x15 },
	{ "disp_cc_mdss_pclk0_clk", &gcc, 0x43, &disp_cc, 0x7 },
	{ "disp_cc_mdss_rot_clk", &gcc, 0x43, &disp_cc, 0x9 },
	{ "disp_cc_mdss_rscc_ahb_clk", &gcc, 0x43, &disp_cc, 0x17 },
	{ "disp_cc_mdss_rscc_vsync_clk", &gcc, 0x43, &disp_cc, 0x16 },
	{ "disp_cc_mdss_vsync_clk", &gcc, 0x43, &disp_cc, 0xb },
	{ "measure_only_disp_cc_sleep_clk", &gcc, 0x43, &disp_cc, 0x1d },
	{ "measure_only_disp_cc_xo_clk", &gcc, 0x43, &disp_cc, 0x1e },

	{ "gpu_cc_ahb_clk", &gcc, 0xfb, &gpu_cc, 0x11 },
	{ "gpu_cc_cx_gfx3d_clk", &gcc, 0xfb, &gpu_cc, 0x1a },
	{ "gpu_cc_cx_gfx3d_slv_clk", &gcc, 0xfb, &gpu_cc, 0x1b },
	{ "gpu_cc_cx_gmu_clk", &gcc, 0xfb, &gpu_cc, 0x19 },
	{ "gpu_cc_cx_snoc_dvm_clk", &gcc, 0xfb, &gpu_cc, 0x16 },
	{ "gpu_cc_cxo_aon_clk", &gcc, 0xfb, &gpu_cc, 0xb },
	{ "gpu_cc_cxo_clk", &gcc, 0xfb, &gpu_cc, 0xa },
	{ "gpu_cc_gx_cxo_clk", &gcc, 0xfb, &gpu_cc, 0xf },
	{ "gpu_cc_gx_gfx3d_clk", &gcc, 0xfb, &gpu_cc, 0xc },
	{ "gpu_cc_gx_gmu_clk", &gcc, 0xfb, &gpu_cc, 0x10 },
	{ "gpu_cc_sleep_clk", &gcc, 0xfb, &gpu_cc, 0x17 },

	{ "mccc_clk", &gcc, 0xae, &mc_cc, 0x220 },
	{}
};

struct debugcc_platform sm6375_debugcc = {
	"sm6375",
	sm6375_clocks,
};
