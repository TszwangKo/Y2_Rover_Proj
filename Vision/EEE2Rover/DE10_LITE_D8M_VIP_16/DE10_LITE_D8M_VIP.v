
//=======================================================
//  This code is generated by Terasic System Builder
//=======================================================

`default_nettype none

module DE10_LITE_D8M_VIP(

	//////////// CLOCK //////////
	input 		          		ADC_CLK_10,
	input 		          		MAX10_CLK1_50,
	input 		          		MAX10_CLK2_50,

	//////////// SDRAM //////////
	output		    [12:0]		DRAM_ADDR,
	output		     [1:0]		DRAM_BA,
	output		          		DRAM_CAS_N,
	output		          		DRAM_CKE,
	output		          		DRAM_CLK,
	output		          		DRAM_CS_N,
	inout 		    [15:0]		DRAM_DQ,
	output		          		DRAM_LDQM,
	output		          		DRAM_RAS_N,
	output		          		DRAM_UDQM,
	output		          		DRAM_WE_N,

	//////////// SEG7 //////////
	output		     [7:0]		HEX0,
	output		     [7:0]		HEX1,
	output		     [7:0]		HEX2,
	output		     [7:0]		HEX3,
	output		     [7:0]		HEX4,
	output		     [7:0]		HEX5,

	//////////// KEY //////////
	input 		     [1:0]		KEY,

	//////////// LED //////////
	output		     [9:0]		LEDR,

	//////////// SW //////////
	input 		     [9:0]		SW,

	//////////// VGA //////////
	output		     [3:0]		VGA_B,
	output		     [3:0]		VGA_G,
	output		          		VGA_HS,
	output		     [3:0]		VGA_R,
	output		          		VGA_VS,

	//////////// Accelerometer //////////
	output		          		GSENSOR_CS_N,
	input 		     [2:1]		GSENSOR_INT,
	output		          		GSENSOR_SCLK,
	inout 		          		GSENSOR_SDI,
	inout 		          		GSENSOR_SDO,

	//////////// Arduino //////////
	inout 		    [15:0]		ARDUINO_IO,
	inout 		          		ARDUINO_RESET_N,

	//////////// GPIO, GPIO connect to D8M-GPIO //////////
	inout 		          		CAMERA_I2C_SCL,
	inout 		          		CAMERA_I2C_SDA,
	output		          		CAMERA_PWDN_n,
	output		          		MIPI_CS_n,
	inout 		          		MIPI_I2C_SCL,
	inout 		          		MIPI_I2C_SDA,
	output		          		MIPI_MCLK,
	input 		          		MIPI_PIXEL_CLK,
	input 		     [9:0]		MIPI_PIXEL_D,
	input 		          		MIPI_PIXEL_HS,
	input 		          		MIPI_PIXEL_VS,
	output		          		MIPI_REFCLK,
	output		          		MIPI_RESET_n
);



//=======================================================
//  REG/WIRE declarations
//=======================================================
wire          disp_clk;
wire          disp_hs;
wire          disp_vs;
wire  [23:0]  disp_data;
wire  [7 :0]  mVGA_R;
wire  [7 :0]  mVGA_G;
wire  [7 :0]  mVGA_B; 



//=======================================================
//  Structural coding
//=======================================================
assign  VGA_HS                   = disp_hs;
assign  VGA_VS                   = disp_vs;
assign  {mVGA_R, mVGA_G, mVGA_B} = disp_data;

assign  VGA_R                    = mVGA_R[7:4];
assign  VGA_G                    = mVGA_G[7:4];
assign  VGA_B                    = mVGA_B[7:4];

assign  MIPI_CS_n                = 1'b0;



///////////////////////////////////////
wire        MIPI_PIXEL_CLK_d;
reg         MIPI_PIXEL_VS_d;
reg         MIPI_PIXEL_HS_d;
reg   [9:0] MIPI_PIXEL_D_d;

assign MIPI_PIXEL_CLK_d = ~MIPI_PIXEL_CLK;

always @ (posedge MIPI_PIXEL_CLK_d) begin
   MIPI_PIXEL_VS_d <= MIPI_PIXEL_VS;
   MIPI_PIXEL_HS_d <= MIPI_PIXEL_HS;
   MIPI_PIXEL_D_d  <= MIPI_PIXEL_D;
end



Qsys u0 (
		.clk_clk                                   (MAX10_CLK1_50), 			//                              clk.clk
		.reset_reset_n                             (1'b1), 						//                            reset.reset_n
		
		.clk_sdram_clk                             (DRAM_CLK),					//                        clk_sdram.clk
		.clk_vga_clk                               (disp_clk),					//                          clk_vga.clk
		.d8m_xclkin_clk                            (MIPI_REFCLK),				//                       d8m_xclkin.clk
		
		.key_external_connection_export            (KEY),            			//          key_external_connection.export
		.led_external_connection_export            (),            				//          led_external_connection.export
		.sw_external_connection_export             (SW),             			//           sw_external_connection.export
		
		.i2c_opencores_camera_export_scl_pad_io    (CAMERA_I2C_SCL),    		//      i2c_opencores_camera_export.scl_pad_io
		.i2c_opencores_camera_export_sda_pad_io    (CAMERA_I2C_SDA),    		//                                 .sda_pad_io
		
		.i2c_opencores_mipi_export_scl_pad_io      (MIPI_I2C_SCL),      		//        i2c_opencores_mipi_export.scl_pad_io
		.i2c_opencores_mipi_export_sda_pad_io      (MIPI_I2C_SDA),      		//                                 .sda_pad_io
		
		.mipi_pwdn_n_external_connection_export    (CAMERA_PWDN_n),    			//  mipi_pwdn_n_external_connection.export
		.mipi_reset_n_external_connection_export   (MIPI_RESET_n),   			// mipi_reset_n_external_connection.export
		
		.sdram_wire_addr                           (DRAM_ADDR),					//                       sdram_wire.addr
		.sdram_wire_ba                             (DRAM_BA),					//                                 .ba
		.sdram_wire_cas_n                          (DRAM_CAS_N),				//                                 .cas_n
		.sdram_wire_cke                            (DRAM_CKE),					//                                 .cke
		.sdram_wire_cs_n                           (DRAM_CS_N),					//                                 .cs_n
		.sdram_wire_dq                             (DRAM_DQ),					//                                 .dq
		.sdram_wire_dqm                            ({DRAM_UDQM, DRAM_LDQM}),	//                                 .dqm
		.sdram_wire_ras_n                          (DRAM_RAS_N),				//                                 .ras_n
		.sdram_wire_we_n                           (DRAM_WE_N),					//                                 .we_n
		
		.terasic_camera_0_conduit_end_D            ({MIPI_PIXEL_D_d[9:0], 2'b00}),//     terasic_camera_0_conduit_end.D
		.terasic_camera_0_conduit_end_FVAL         (MIPI_PIXEL_VS_d),         	//                                 .FVAL
		.terasic_camera_0_conduit_end_LVAL         (MIPI_PIXEL_HS_d),         	//                                 .LVAL
		.terasic_camera_0_conduit_end_PIXCLK       (~MIPI_PIXEL_CLK_d),        	//                                 .PIXCLK
		
		.terasic_auto_focus_0_conduit_vcm_i2c_sda  (CAMERA_I2C_SDA),  			//     terasic_auto_focus_0_conduit.vcm_i2c_sda
		.terasic_auto_focus_0_conduit_clk50        (MAX10_CLK1_50),        		//                                 .clk50
		.terasic_auto_focus_0_conduit_vcm_i2c_scl  (CAMERA_I2C_SCL),  			//                                 .vcm_i2c_scl
		
		.alt_vip_itc_0_clocked_video_vid_clk       (disp_clk),       			//      alt_vip_itc_0_clocked_video.vid_clk
		.alt_vip_itc_0_clocked_video_vid_data      (disp_data),      			//                                 .vid_data
		.alt_vip_itc_0_clocked_video_underflow     (),     						//                                 .underflow
		.alt_vip_itc_0_clocked_video_vid_datavalid (), 							//                                 .vid_datavalid
		.alt_vip_itc_0_clocked_video_vid_v_sync    (disp_vs),    				//                                 .vid_v_sync
		.alt_vip_itc_0_clocked_video_vid_h_sync    (disp_hs),    				//                                 .vid_h_sync
		.alt_vip_itc_0_clocked_video_vid_f         (),         					//                                 .vid_f
		.alt_vip_itc_0_clocked_video_vid_h         (),         					//                                 .vid_h
		.alt_vip_itc_0_clocked_video_vid_v         (),         					//                                 .vid_v
		
		.altpll_0_areset_conduit_export            (),            				//          altpll_0_areset_conduit.export
		.altpll_0_locked_conduit_export            (),            				//          altpll_0_locked_conduit.export
		.altpll_0_phasedone_conduit_export         (),         					//       altpll_0_phasedone_conduit.export		
		
		.eee_imgproc_0_conduit_mode_new_signal     (SW[0]),
		
		.uart_rx_tx_rxd                          (ARDUINO_IO[1]),                          //                     uart_0_rx_tx.rxd
		.uart_rx_tx_txd                          (ARDUINO_IO[0])                           //
	);

FpsMonitor uFps(
	.clk50(MAX10_CLK2_50),
	.vs(MIPI_PIXEL_VS),
	
	.fps(),
	.hex_fps_h(HEX1),
	.hex_fps_l(HEX0)
);

assign  HEX2 = 7'h7F;
assign  HEX3 = 7'h7F;
assign  HEX4 = 7'h7F;
assign  HEX5 = 7'h7F;

endmodule
