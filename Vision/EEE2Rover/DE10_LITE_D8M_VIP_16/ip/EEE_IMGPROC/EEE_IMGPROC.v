module EEE_IMGPROC(
	// global clock & reset
	clk,
	reset_n,
	
	// mm slave
	s_chipselect,
	s_read,
	s_write,
	s_readdata,
	s_writedata,
	s_address,

	// stream sink
	sink_data,
	sink_valid,
	sink_ready,
	sink_sop,
	sink_eop,
	
	// streaming source
	source_data,
	source_valid,
	source_ready,
	source_sop,
	source_eop,
	
	// conduit
	mode
	
);


// global clock & reset
input	clk;
input	reset_n;

// mm slave
input							s_chipselect;
input							s_read;
input							s_write;
output	reg	[31:0]	s_readdata;
input	[31:0]				s_writedata;
input	[2:0]					s_address;


// streaming sink
input	[23:0]            	sink_data;
input								sink_valid;
output							sink_ready;
input								sink_sop;
input								sink_eop;

// streaming source
output	[23:0]			  	   source_data;
output								source_valid;
input									source_ready;
output								source_sop;
output								source_eop;

// conduit export
input                         mode;

////////////////////////////////////////////////////////////////////////
//
parameter IMAGE_W = 11'd640;
parameter IMAGE_H = 11'd480;
parameter MESSAGE_BUF_MAX = 256;
parameter MSG_INTERVAL = 16;
parameter BB_COL_DEFAULT = 24'h00ff00;


wire [7:0]   red, green, blue, grey;
wire [7:0]   red_out, green_out, blue_out;

wire         sop, eop, in_valid, out_ready;



////////////////////////////////////////////////////////////////////////
////////////         Converting from RGB to HSV      ///////////////////
////////////////////////////////////////////////////////////////////////

wire [8:0]   hsv_h0;
wire [7:0]   hsv_s0,hsv_v0;

wire			 out_valid;

// set filtered_mode
reg filter_mode;
reg in_val;
reg [7:0] r,g,b;
always@(clk)begin
	if(filter_mode)begin
		r <= red;
		g <= green;
		b <= blue;
		in_val <= in_valid;
	end else begin
		r <= red_f;
		g <= green_f;
		b <= blue_f;
		in_val <= in_valid_f;
	end
end

RGB2HSV  RGB2HSV_inst(
	.clk(clk),

//  .valid_in(in_valid_f),
//	.rgb_r(red_f),
//	.rgb_g(green_f),
//	.rgb_b(blue_f),	
	
//	.valid_in(in_valid),
//	.rgb_r(red),
//	.rgb_g(green),
//	.rgb_b(blue),	

	.valid_in(in_val),
	.rgb_r(r),
	.rgb_g(g),
	.rgb_b(b),	

   .valid_out(out_valid),
	.hsv_h(hsv_h0),//  0 - 360
	.hsv_s(hsv_s0),//  0 - 255
	.hsv_v(hsv_v0) //  0 - 255
);

////////////////////////////////////////////////////////////////////////
// Detect red, blue, \
//green, pink, yellow areas
wire red_detect,blue_detect,yellow_detect,green_detect,pink_detect;
//wire [8:0] blue_hl, blue_hh;

//assign red_detect = red[7] & ~green[7] & ~blue[7];
assign red_detect = (hsv_h0 > red_hl) & (hsv_h0 < red_hu) & (hsv_s0 > 40) & (hsv_v0 > 20);
assign blue_detect = (hsv_h0 > blue_hl) & (hsv_h0 < blue_hu) & (hsv_s0 > 40) & (hsv_v0 > 25);
assign yellow_detect = (hsv_h0 > yellow_hl) & (hsv_h0 < yellow_hu) & (hsv_s0 > 5) & (hsv_v0 > 40);
assign green_detect = (hsv_h0 > green_hl) & (hsv_h0 < green_hu) & (hsv_s0 > 70) & (hsv_v0 > 20);
assign pink_detect = (hsv_h0 > pink_hl) & (hsv_h0 < pink_hu) & (hsv_s0 > 30) & (hsv_v0 > 20) ;

// Find boundary of cursor box

// Highlight detected areas
wire [23:0] red_high, blue_high, yellow_high,green_high, pink_high;
assign grey = green[7:1] + red[7:2] + blue[7:2]; //Grey = green/2 + red/4 + blue/4
assign red_high  =  red_detect ? {8'hff, 8'h0, 8'h0} : {grey, grey, grey};
assign blue_high  =  blue_detect ? {8'h00, 8'h00, 8'hff} : {grey, grey, grey};
assign yellow_high  =  yellow_detect ? {8'hff, 8'hff, 8'h00} : {grey, grey, grey};
assign green_high  =  green_detect ? {8'h00, 8'hff, 8'h00} : {grey, grey, grey};
assign pink_high  =  pink_detect ? {8'hff, 8'hcf, 8'hE2} : {grey, grey, grey};


// Show bounding boxs
wire [23:0] new_image;
wire bb_active_r, bb_active_b, bb_active_y, bb_active_g, bb_active_p;
assign bb_active_r = (x == left_r) | (x == right_r) | (y == top_r) | (y == bottom_r);
assign bb_active_b = (x == left_b) | (x == right_b) | (y == top_b) | (y == bottom_b);
assign bb_active_y = (x == left_y) | (x == right_y) | (y == top_y) | (y == bottom_y);
assign bb_active_g = (x == left_g) | (x == right_g) | (y == top_g) | (y == bottom_g);
assign bb_active_p = (x == left_p) | (x == right_p) | (y == top_p) | (y == bottom_p);

//always@(*) begin
//	if (bb_active_r) begin
//		new_image = bb_col;
//	end
//	else begin
//		new_image = red_high;
//	end
//	
//end
//always@(*) begin
//	if(bb_active_r) begin
//		new_image = bb_col;
//	end
//	else if(bb_active_g) begin
//		new_image = blue_high;
//	end
//	else begin
//		new_image = red_high;
//	end
//end

assign new_image = bb_active_r ? bb_col : 
						 blue_detect ? blue_high : 
						 yellow_detect ? yellow_high :
						 green_detect ? green_high :
						 pink_detect ? pink_high : 
						 red_high;

// Switch output pixels depending on mode switch
// Don't modify the start-of-packet word - it's a packet discriptor
// Don't modify data in non-video packets
assign {red_out, green_out, blue_out} = (mode & ~sop & packet_video) ? new_image : {red,green,blue};

//Count valid pixels to get the image coordinates. Reset and detect packet type on Start of Packet.
reg [10:0] x, y;
reg packet_video;
always@(posedge clk) begin
	if (sop) begin
		x <= 11'h0;
		y <= 11'h0;
		packet_video <= (blue[3:0] == 3'h0);
	end
	else if (in_valid) begin
		if (x == IMAGE_W-1) begin
			x <= 11'h0;
			y <= y + 11'h1;
		end
		else begin
			x <= x + 11'h1;
		end
	end
end

//Find first and last red pixels
reg [10:0] x_min_r, y_min_r, x_max_r, y_max_r;
reg [10:0] x_min_b, y_min_b, x_max_b, y_max_b;
reg [10:0] x_min_y, y_min_y, x_max_y, y_max_y;
reg [10:0] x_min_g, y_min_g, x_max_g, y_max_g;
reg [10:0] x_min_p, y_min_p, x_max_p, y_max_p;

always@(posedge clk) begin
	if (red_detect & in_valid) begin	//Update bounds when the pixel is red
		if (x < x_min_r) x_min_r <= x;
		if (x > x_max_r) x_max_r <= x;
		if (y < y_min_r) y_min_r <= y;
		y_max_r <= y;
	end
	if (blue_detect & in_valid) begin	//Update bounds when the pixel is red
		if (x < x_min_b) x_min_b <= x;
		if (x > x_max_b) x_max_b <= x;
		if (y < y_min_b) y_min_b <= y;
		y_max_b <= y;
	end
	if (yellow_detect & in_valid) begin	//Update bounds when the pixel is red
		if (x < x_min_y) x_min_y <= x;
		if (x > x_max_y) x_max_y <= x;
		if (y < y_min_y) y_min_y <= y;
		y_max_y <= y;
	end
	if (green_detect & in_valid) begin	//Update bounds when the pixel is red
		if (x < x_min_g) x_min_g <= x;
		if (x > x_max_g) x_max_g <= x;
		if (y < y_min_g) y_min_g <= y;
		y_max_g <= y;
	end
	if (pink_detect & in_valid) begin	//Update bounds when the pixel is red
		if (x < x_min_p) x_min_p <= x;
		if (x > x_max_p) x_max_p <= x;
		if (y < y_min_p) y_min_p <= y;
		y_max_p <= y;
	end
	if (sop & in_valid) begin	//Reset bounds on start of packet
		x_min_r <= IMAGE_W-11'h1;
		x_max_r <= 0;
		y_min_r <= IMAGE_H-11'h1;
		y_max_r <= 0;
		
		x_min_b <= IMAGE_W-11'h1;
		x_max_b <= 0;
		y_min_b <= IMAGE_H-11'h1;
		y_max_b <= 0;
		
		x_min_y <= IMAGE_W-11'h1;
		x_max_y <= 0;
		y_min_y <= IMAGE_H-11'h1;
		y_max_y <= 0;
		
		x_min_g <= IMAGE_W-11'h1;
		x_max_g <= 0;
		y_min_g <= IMAGE_H-11'h1;
		y_max_g <= 0;
		
		x_min_p <= IMAGE_W-11'h1;
		x_max_p <= 0;
		y_min_p <= IMAGE_H-11'h1;
		y_max_p <= 0;
	end
end

//Process bounding box at the end of the frame.
reg [3:0] msg_state;
reg [10:0] left_r, right_r, top_r, bottom_r;
reg [10:0] left_b, right_b, top_b, bottom_b;
reg [10:0] left_y, right_y, top_y, bottom_y;
reg [10:0] left_g, right_g, top_g, bottom_g;
reg [10:0] left_p, right_p, top_p, bottom_p;
reg [7:0] frame_count;
always@(posedge clk) begin
	if (eop & in_valid & packet_video) begin  //Ignore non-video packets
		
		//Latch edges for display overlay on next frame
		left_r <= x_min_r;
		right_r <= x_max_r;
		top_r <= y_min_r;
		bottom_r <= y_max_r;
		
		
		//Start message writer FSM once every MSG_INTERVAL frames, if there is room in the FIFO
		frame_count <= frame_count - 1;
		
		if (frame_count == 0 && msg_buf_size < MESSAGE_BUF_MAX - 15) begin
			msg_state <= 4'b0001;
			frame_count <= MSG_INTERVAL-1;
		end
	end
	
	//Cycle through message writer states once started
	if (msg_state != 4'b000) msg_state <= msg_state + 4'b0001;

end
	
//Generate output messages for CPU
reg [31:0] msg_buf_in; 
wire [31:0] msg_buf_out;
reg msg_buf_wr;
wire msg_buf_rd, msg_buf_flush;
wire [7:0] msg_buf_size;
wire msg_buf_empty;

`define RED_BOX_MSG_RID "RBB"
`define RED_BOX_MSG_GID "GBB"
`define RED_BOX_MSG_BID "BBB"
`define RED_BOX_MSG_YID "YBB"
`define RED_BOX_MSG_PID "PBB"


always@(*) begin	//Write words to FIFO as state machine advances
	case(msg_state)
		4'h0: begin
			msg_buf_in = 32'b0;
			msg_buf_wr = 1'b0;
		end
		4'h1: begin
			msg_buf_in = `RED_BOX_MSG_RID;	//Message ID
			msg_buf_wr = 1'b1;
		end
		4'h2: begin
			msg_buf_in = {5'b0, x_min_r, 5'b0, x_max_r};	//Top left coordinate
			msg_buf_wr = 1'b1;
		end
		4'h3: begin
			msg_buf_in = {5'b0, y_min_r, 5'b0, y_max_r}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'h4: begin
			msg_buf_in = `RED_BOX_MSG_BID;	//Message ID
			msg_buf_wr = 1'b1;
		end
		4'h5: begin
			msg_buf_in = {5'b0, x_min_b, 5'b0, x_max_b}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'h6: begin
			msg_buf_in = {5'b0, y_min_b, 5'b0, y_max_b}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'h7: begin
			msg_buf_in = `RED_BOX_MSG_YID;	//Message ID
			msg_buf_wr = 1'b1;
		end
		4'h8: begin
			msg_buf_in = {5'b0, x_min_y, 5'b0, x_max_y}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'h9: begin
			msg_buf_in = {5'b0, y_min_y, 5'b0, y_max_y}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'ha: begin
			msg_buf_in = `RED_BOX_MSG_GID;	//Message ID
			msg_buf_wr = 1'b1;
		end
		4'hb: begin
			msg_buf_in = {5'b0, x_min_g, 5'b0, x_max_g}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'hc: begin
			msg_buf_in = {5'b0, y_min_g, 5'b0, y_max_g}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'hd: begin
			msg_buf_in = `RED_BOX_MSG_PID;	//Message ID
			msg_buf_wr = 1'b1;
		end
		4'he: begin
			msg_buf_in = {5'b0, x_min_p, 5'b0, x_max_p}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		4'hf: begin
			msg_buf_in = {5'b0, y_min_p, 5'b0, y_max_p}; //Bottom right coordinate
			msg_buf_wr = 1'b1;
		end
		default : begin
			msg_buf_in = 32'b0;
			msg_buf_wr = 1'b0;
		end
	endcase
end


//Output message FIFO
MSG_FIFO	MSG_FIFO_inst (
	.clock (clk),
	.data (msg_buf_in),
	.rdreq (msg_buf_rd),
	.sclr (~reset_n | msg_buf_flush),
	.wrreq (msg_buf_wr),
	.q (msg_buf_out),
	.usedw (msg_buf_size),
	.empty (msg_buf_empty)
	);


//Streaming registers to buffer video signal
STREAM_REG #(.DATA_WIDTH(26)) in_reg (
	.clk(clk),
	.rst_n(reset_n),
	.ready_out(sink_ready),
	.valid_out(in_valid),
	.data_out({red,green,blue,sop,eop}),
	.ready_in(out_ready),
	.valid_in(sink_valid),
	.data_in({sink_data,sink_sop,sink_eop})
);

STREAM_REG #(.DATA_WIDTH(26)) out_reg (
	.clk(clk),
	.rst_n(reset_n),
	.ready_out(out_ready),
	.valid_out(source_valid),
	.data_out({source_data,source_sop,source_eop}),
	.ready_in(source_ready),
	.valid_in(in_valid),
	.data_in({red_out, green_out, blue_out, sop, eop})
);
wire [7:0] red_f,green_f,blue_f;
wire in_valid_f;

Gaus_filter filter_inst (
	.data_in({red,green,blue}),
	.sop(sop),
	.eop(eop),
	.valid_in(in_valid),
	.clk(clk),
	.rst(reset_n),
	
	.data_out({red_f,green_f,blue_f}),
	.sop_out(),
	.eop_out(),
	.valid_out(in_valid_f)
);

/////////////////////////////////
/// Memory-mapped port		 /////
/////////////////////////////////

// Addresses
`define REG_STATUS    			0
`define READ_MSG    				1
`define READ_ID    				2
`define REG_BBCOL					3

//Status register bits
// 31:16 - unimplemented
// 15:8 - number of words in message buffer (read only)
// 7:5 - unused
// 4 - flush message buffer (write only - read as 0)
// 3:0 - unused


// Process write

reg	[7:0]   reg_status;
reg	[23:0]	bb_col;
reg	[8:0]		blue_hl, blue_hu;
reg	[8:0]		green_hl, green_hu;
reg	[8:0]		red_hl, red_hu;
reg	[8:0]		yellow_hl, yellow_hu;
reg	[8:0]		pink_hl, pink_hu;
reg	[7:0]		blue_sl, blue_su;
reg	[7:0]		green_sl, green_su;
reg	[7:0]		red_sl, red_su;
reg	[7:0]		yellow_sl, yellow_su;
reg	[7:0]		pink_sl, pink_su;
reg	[7:0]		blue_vl, blue_vu;
reg	[7:0]		green_vl, green_vu;
reg	[7:0]		red_vl, red_vu;
reg	[7:0]		yellow_vl, yellow_vu;
reg	[7:0]		pink_vl, pink_vu;

integer step = 9'd5;
always @ (posedge clk)
begin
	if (~reset_n)
	begin
		reg_status <= 8'b0;
		bb_col <= BB_COL_DEFAULT;
		blue_hl <= 9'd200;
		blue_hu <= 9'd230;
		blue_sl <= 8'd0;
		blue_su <= 8'hff;
		blue_vl <= 8'd0;
		blue_vu <= 8'hff;
		
		green_hl <= 9'd150;
		green_hu <= 9'd190;
		green_sl <= 8'd0;
		green_su <= 8'hff;
		green_vl <= 8'd0;
		green_vu <= 8'hff;
		
		red_hl <= 9'd0;
		red_hu <= 9'd40;
		red_sl <= 8'd0;
		red_su <= 8'hff;
		red_vl <= 8'd0;
		red_vu <= 8'hff;
		
		yellow_hl <= 9'd60;
		yellow_hu <= 9'd90;
		yellow_sl <= 8'd0;
		yellow_su <= 8'hff;
		yellow_vl <= 8'd0;
		yellow_vu <= 8'hff;
		
		pink_hl <= 9'd260;
		pink_hu <= 9'd320;
		pink_sl <= 8'd0;
		pink_su <= 8'hff;
		pink_vl <= 8'd0;
		pink_vu <= 8'hff;
		
		
	end
	else begin
		if(s_chipselect & s_write) begin
		   if      (s_address == `REG_STATUS)	reg_status <= s_writedata[7:0];
			if		  (s_address == `READ_MSG) begin
						if(s_writedata[31:0] == 32'h0001) begin
							blue_hl <= blue_hl + step;
						end else if (s_writedata[31:0] == 32'h0002) begin
							blue_hl <= blue_hl - step;
						end else if (s_writedata[31:0] == 32'h0003) begin
							blue_hu <= blue_hu + step;
						end else if (s_writedata[31:0] == 32'h0004) begin
							blue_hu <= blue_hu - step;
						end else if (s_writedata[31:0] == 32'h0005) begin
							blue_sl <= blue_hu + step;
						end else if (s_writedata[31:0] == 32'h0006) begin
							blue_sl <= blue_hu - step;
						end else if (s_writedata[31:0] == 32'h0007) begin
							blue_su <= blue_hu + step;
						end else if (s_writedata[31:0] == 32'h0008) begin
							blue_su <= blue_hu - step;
						end else if (s_writedata[31:0] == 32'h0009) begin
							blue_vl <= blue_hu + step;
						end else if (s_writedata[31:0] == 32'h000a) begin
							blue_vl <= blue_hu - step;
						end else if (s_writedata[31:0] == 32'h000b) begin
							blue_vu <= blue_hu + step;
						end else if (s_writedata[31:0] == 32'h000c) begin
							blue_vu <= blue_hu - step;
						end 
						
						
						
						else if(s_writedata[31:0] == 32'h1001) begin
							green_hl <= green_hl + step;
						end else if (s_writedata[31:0] == 32'h1002) begin
							green_hl <= green_hl - step;
						end else if (s_writedata[31:0] == 32'h1003) begin
							green_hu <= green_hu + step;
						end else if (s_writedata[31:0] == 32'h1004) begin
							green_hu <= green_hu - step;
						end else if (s_writedata[31:0] == 32'h1005) begin
							green_sl <= green_hu + step;
						end else if (s_writedata[31:0] == 32'h1006) begin
							green_sl <= green_hu - step;
						end else if (s_writedata[31:0] == 32'h1007) begin
							green_su <= green_hu + step;
						end else if (s_writedata[31:0] == 32'h1008) begin
							green_su <= green_hu - step;
						end else if (s_writedata[31:0] == 32'h1009) begin
							green_vl <= green_hu + step;
						end else if (s_writedata[31:0] == 32'h100a) begin
							green_vl <= green_hu - step;
						end else if (s_writedata[31:0] == 32'h100b) begin
							green_vu <= green_hu + step;
						end else if (s_writedata[31:0] == 32'h100c) begin
							green_vu <= green_hu - step;
						end 
						else if(s_writedata[31:0] == 32'h2001) begin
							red_hl <= red_hl + step;
						end else if (s_writedata[31:0] == 32'h2002) begin
							red_hl <= red_hl - step;
						end else if (s_writedata[31:0] == 32'h2003) begin
							red_hu <= red_hu + step;
						end else if (s_writedata[31:0] == 32'h2004) begin
							red_hu <= red_hu - step;
						end else if (s_writedata[31:0] == 32'h2005) begin
							red_sl <= red_hu + step;
						end else if (s_writedata[31:0] == 32'h2006) begin
							red_sl <= red_hu - step;
						end else if (s_writedata[31:0] == 32'h2007) begin
							red_su <= red_hu + step;
						end else if (s_writedata[31:0] == 32'h2008) begin
							red_su <= red_hu - step;
						end else if (s_writedata[31:0] == 32'h2009) begin
							red_vl <= red_hu + step;
						end else if (s_writedata[31:0] == 32'h200a) begin
							red_vl <= red_hu - step;
						end else if (s_writedata[31:0] == 32'h200b) begin
							red_vu <= red_hu + step;
						end else if (s_writedata[31:0] == 32'h200c) begin
							red_vu <= red_hu - step;
						end 
						
						else if(s_writedata[31:0] == 32'h3001) begin
							yellow_hl <= yellow_hl + step;
						end else if (s_writedata[31:0] == 32'h3002) begin
							yellow_hl <= yellow_hl - step;
						end else if (s_writedata[31:0] == 32'h3003) begin
							yellow_hu <= yellow_hu + step;
						end else if (s_writedata[31:0] == 32'h3004) begin
							yellow_hu <= yellow_hu - step;
						end  else if (s_writedata[31:0] == 32'h3005) begin
							yellow_sl <= yellow_hu + step;
						end else if (s_writedata[31:0] == 32'h3006) begin
							yellow_sl <= yellow_hu - step;
						end else if (s_writedata[31:0] == 32'h3007) begin
							yellow_su <= yellow_hu + step;
						end else if (s_writedata[31:0] == 32'h3008) begin
							yellow_su <= yellow_hu - step;
						end else if (s_writedata[31:0] == 32'h3009) begin
							yellow_vl <= yellow_hu + step;
						end else if (s_writedata[31:0] == 32'h300a) begin
							yellow_vl <= yellow_hu - step;
						end else if (s_writedata[31:0] == 32'h300b) begin
							yellow_vu <= yellow_hu + step;
						end else if (s_writedata[31:0] == 32'h300c) begin
							yellow_vu <= yellow_hu - step;
						end 
						else if(s_writedata[31:0] == 32'h4001) begin
							pink_hl <= pink_hl + step;
						end else if (s_writedata[31:0] == 32'h4002) begin
							pink_hl <= pink_hl - step;
						end else if (s_writedata[31:0] == 32'h4003) begin
							pink_hu <= pink_hu + step;
						end else if (s_writedata[31:0] == 32'h4004) begin
							pink_hu <= pink_hu - step;
						end else if (s_writedata[31:0] == 32'h4005) begin
							pink_sl <= pink_hu + step;
						end else if (s_writedata[31:0] == 32'h4006) begin
							pink_sl <= pink_hu - step;
						end else if (s_writedata[31:0] == 32'h4007) begin
							pink_su <= pink_hu + step;
						end else if (s_writedata[31:0] == 32'h4008) begin
							pink_su <= pink_hu - step;
						end else if (s_writedata[31:0] == 32'h4009) begin
							pink_vl <= pink_hu + step;
						end else if (s_writedata[31:0] == 32'h400a) begin
							pink_vl <= pink_hu - step;
						end else if (s_writedata[31:0] == 32'h400b) begin
							pink_vu <= pink_hu + step;
						end else if (s_writedata[31:0] == 32'h400c) begin
							pink_vu <= pink_hu - step;
						end 
						else if(s_writedata[31:0] == 32'h0100) begin
							filter_mode <= !filter_mode;
						end 
			end
		   if      (s_address == `REG_BBCOL)	bb_col <= s_writedata[23:0];
		end
	end
end

//Flush the message buffer if 1 is written to status register bit 4
assign msg_buf_flush = (s_chipselect & s_write & (s_address == `REG_STATUS) & s_writedata[4]);


// Process reads
reg read_d; //Store the read signal for correct updating of the message buffer

// Copy the requested word to the output port when there is a read.
always @ (posedge clk)
begin
   if (~reset_n) begin
	   s_readdata <= {32'b0};
		read_d <= 1'b0;
	end
	
	else if (s_chipselect & s_read) begin
		if   (s_address == `REG_STATUS) s_readdata <= {15'b0,msg_buf_size,reg_status};
		if   (s_address == `READ_MSG) s_readdata <= {msg_buf_out};
		if   (s_address == `READ_ID) s_readdata <= 32'h1234EEE2;
		if   (s_address == `REG_BBCOL) s_readdata <= {8'h0, bb_col};
	end
	
	read_d <= s_read;
end

//Fetch next word from message buffer after read from READ_MSG
assign msg_buf_rd = s_chipselect & s_read & ~read_d & ~msg_buf_empty & (s_address == `READ_MSG);
						


endmodule

