module Gaus_filter (
	input [23:0] data_in,
	input sop,
	input eop,
	input valid_in,
	input clk,
	input rst,
	
	output reg [23:0] data_out,
	output reg sop_out,
	output reg eop_out,
	output reg valid_out
//	output reg [10:0] x,
//	output reg [10:0] y,
//	output reg [11:0] b10,
//	output reg [11:0] b11,
//	output reg [11:0] b12,
//	output reg [11:0] b20,
//	output reg [11:0] b21,
//	output reg [11:0] b22,
//	output reg [11:0] b30,
//	output reg [11:0] b31,
//	output reg [11:0] b32
);


//Setting up parameter for different images

parameter IMG_W = 11'd640;
parameter IMG_H = 11'd480;

integer i;
//Get Data into buffer
reg [26:0] buffer;

wire cond;


always@(posedge clk) begin
	buffer[0] <= valid_in;
//	if(valid_in) begin
	
		buffer[26:1] <= {data_in,sop,eop};
		
//	end
	
end



wire [26:0] tap640,tap1280;

//Connecting data to row buffer
Row_buffer	Row_buffer_inst (
	.aclr ( rst ),
	.clock ( clk ),
	.shiftin ( buffer ),
	.shiftout (),
	.taps0x ( tap640 ),
	.taps1x ( tap1280 )

	);



reg [2:0] sig_out, sig_out1, sig_out2;

//Creating Counter to keep track of position of file
reg [10:0] x,y;
reg packet_video;
wire cond_2;
assign cond_2 = sig_out2[0];
always@(posedge clk) begin

	if (sop) begin
		x <= 11'h0;
		y <= 11'h0;
		packet_video <= (data_in[3:0] == 3'h0);
	end
	else if ( valid_in || cond_2 == 1 ) begin
	
		if (x == IMG_W-11'h1) begin
			x <= 11'h0;
			y <= y + 11'h1;
		end
		else begin
			x <= x + 11'h1;
		end
	end
	
end


reg [11:0] buf_r1 [2:0],buf_r2 [2:0],buf_r3 [2:0];
reg [11:0] buf_g1 [2:0],buf_g2 [2:0],buf_g3 [2:0];
reg [11:0] buf_b1 [2:0],buf_b2 [2:0],buf_b3 [2:0];

reg [7:0] red,green,blue;

//Set-up data pipeline, so that 9 pixels appears at the same time
always@(posedge clk) begin

	{buf_r1[0][7:0],buf_g1[0][7:0],buf_b1[0][7:0]} <= buffer[26:3];
	buf_r1[1] <= buf_r1[0];
	buf_r1[2] <= buf_r1[1];
	buf_g1[1] <= buf_g1[0];
	buf_g1[2] <= buf_g1[1];
	buf_b1[1] <= buf_b1[0];
	buf_b1[2] <= buf_b1[1];
	
//	b10[7:0] <= buffer[10:3];
//	b11[7:0] <= buf_b1[0];
//	b12[7:0] <= buf_b1[1];
//	
	{buf_r2[0][7:0],buf_g2[0][7:0],buf_b2[0][7:0],sig_out} <= tap640;
	buf_r2[1] <= buf_r2[0];
	buf_r2[2] <= buf_r2[1];
	buf_g2[1] <= buf_g2[0];
	buf_g2[2] <= buf_g2[1];
	buf_b2[1] <= buf_b2[0];
	buf_b2[2] <= buf_b2[1];
	
//	b20[7:0] <= tap640[10:3];
//	b21[7:0] <= buf_b2[0];
//	b22[7:0] <= buf_b2[1];
//	
	{buf_r3[0][7:0],buf_g3[0][7:0],buf_b3[0][7:0]} <= tap1280[26:3];
	buf_r3[1] <= buf_r3[0];
	buf_r3[2] <= buf_r3[1];
	buf_g3[1] <= buf_g3[0];
	buf_g3[2] <= buf_g3[1];
	buf_b3[1] <= buf_b3[0];
	buf_b3[2] <= buf_b3[1];
//	
//	b30[7:0] <= tap1280[10:3];
//	b31[7:0] <= buf_b3[0];
//	b32[7:0] <= buf_b3[1];
end

always @(posedge clk) begin	
	if(packet_video) begin
		if (x == 2 & y == 1) begin //pass on sop data (should be 0)
			red <= buf_r2[1] ;
			green <= buf_g2[1] ;
			blue <=  buf_b2[1] ;
		end
		else if( x == 3 & y == 1 )begin // top left pixel
			red <= ( buf_r1[0] + buf_r1[1] + buf_r2[0] + buf_r2[1] ) >> 2;
			green <= ( buf_g1[0] + buf_g1[1] + buf_g2[0] + buf_g2[1] ) >> 2;
			blue <= ( buf_b1[0] + buf_b1[1] + buf_b2[0] + buf_b2[1] ) >> 2;
		end 
		else if ( ( x > 3 & x < IMG_W & y == 1 ) | ( x < 2 & y == 2 ) ) begin //first line 
			red <= ( buf_r1[0] + buf_r1[1] + buf_r1[2] + buf_r2[0] + buf_r2[1] + buf_r2[2] )/6;
			green <= ( buf_g1[0] + buf_g1[1] + buf_g1[2] + buf_g2[0] + buf_g2[1] + buf_g2[2] )/6;
			blue <= ( buf_b1[0] + buf_b1[1] + buf_b1[2] + buf_b2[0] + buf_b2[1] + buf_b2[2] )/6;
		end
		else if ( x == 2 & y == 2 ) begin//top right pixel
			red <= ( buf_r1[2] + buf_r1[1] + buf_r2[2] + buf_r2[1] ) >> 2;
			green <= ( buf_g1[2] + buf_g1[1] + buf_g2[2] + buf_g2[1] ) >> 2;
			blue <= ( buf_b1[2] + buf_b1[1] + buf_b2[2] + buf_b2[1] ) >> 2;
		end 
		else if ( x == 3 & y > 1 & y < IMG_H ) begin//left pixels
			red <= ( buf_r1[0] + buf_r1[1] + buf_r2[0] + buf_r2[1] + buf_r3[0] + buf_r3[1] )/6;
			green <= ( buf_g1[0] + buf_g1[1] + buf_g2[0] + buf_g2[1] + buf_g3[0] + buf_g3[1] )/6;
			blue <= ( buf_b1[0] + buf_b1[1] + buf_b2[0] + buf_b2[1] + buf_b3[0] + buf_b3[1] )/6;
		end 
		else if ( ( x > 3 & x < IMG_W & y > 1 & y < IMG_H ) | ( x < 2 & y > 2 & y < IMG_H + 1) ) begin// middle pixels
			red <= ( buf_r1[0] + buf_r1[1] + buf_r1 [2] + buf_r2[0] + buf_r2[1] + buf_r2[2] + buf_r3[0] + buf_r3[1] + buf_r3[2] ) /9;
			green <= ( buf_g1[0] + buf_g1[1] + buf_g1 [2] + buf_g2[0] + buf_g2[1] + buf_g2[2] + buf_g3[0] + buf_g3[1] + buf_g3[2] ) /9;
			blue <= ( buf_b1[0] + buf_b1[1] + buf_b1 [2] + buf_b2[0] + buf_b2[1] + buf_b2[2] + buf_b3[0] + buf_b3[1] + buf_b3[2] ) /9;
		end
		else if ( x == 2 & y > 2 & y < IMG_H + 1 ) begin //right pixels
			red <= ( buf_r1[2] + buf_r1[1] + buf_r2[2] + buf_r2[1] + buf_r3[2] + buf_r3[1] )/6;
			green <= ( buf_g1[2] + buf_g1[1] + buf_g2[2] + buf_g2[1] + buf_g3[2] + buf_g3[1] )/6;
			blue <= ( buf_b1[2] + buf_b1[1] + buf_b2[2] + buf_b2[1] + buf_b3[2] + buf_b3[1] )/6;
		end
		else if ( x == 3 & y == IMG_H ) begin //bottom left pixel
			red <= ( buf_r2[0] + buf_r2[1] + buf_r3[0] + buf_r3[1] ) >> 2;
			green <= ( buf_g2[0] + buf_g2[1] + buf_g3[0] + buf_g3[1] ) >> 2;
			blue <= ( buf_b2[0] + buf_b2[1] + buf_b3[0] + buf_b3[1] ) >> 2;
		end 
		else if ( ( x > 3 & x < IMG_W & y == IMG_H ) | ( x < 2 & y == IMG_H + 1) ) begin //Last line 
			red <= ( buf_r2[0] + buf_r2[1] + buf_r2[2] + buf_r3[0] + buf_r3[1] + buf_r3[2] )/6;
			green <= ( buf_g2[0] + buf_g2[1] + buf_g2[2] + buf_g3[0] + buf_g3[1] + buf_g3[2] )/6;
			blue <= ( buf_b2[0] + buf_b2[1] + buf_b2[2] + buf_b3[0] + buf_b3[1] + buf_b3[2] )/6;
		end 
		else if ( x == 2 & y == IMG_H + 1) begin//bottom right pixel
			red <= ( buf_r2[1] + buf_r2[2] + buf_r3[1] + buf_r3[2] ) >> 2;
			green <= ( buf_g2[1] + buf_g2[2] + buf_g3[1] + buf_g3[2] ) >> 2;
			blue <= ( buf_b2[1] + buf_b2[2] + buf_b3[1] + buf_b3[2] ) >> 2;
		end
		else begin
			red <= 8'b0;
			green <= 8'b0;
			blue <=8'b0;
		end
	end else begin
		red <= buf_r2[1];
		green <= buf_g2[1];
		blue <= buf_b2[1];
	end
end

always@(posedge clk) begin
	
	sig_out1 <= sig_out;
	sig_out2 <= sig_out1;
	{sop_out,eop_out,valid_out} <= sig_out2;
	data_out <= {red,green,blue};
end


endmodule
		