// This is a part of the decode module used in the CPU we're designing
// Written by Tong Qi, Last Updated 11/11/2008
// Debugged last 11/14/2008


`include "macro_defs.h"

//NEED TO DO CONTROL REGISTER READ/WRITES 
module decode_unit(
    // Inputs - 
    instruction, 
    // reset,
    
    // Outputs
    o_rd,
    o_rd_type,
    o_rd_valid,
    o_rd2,
    o_rd2_type,
    o_rd2_valid,
    o_rs1,
    o_rs1_type,
    o_rs1_valid, 
    o_rs2,
    o_rs2_type,
    o_rs2_valid, 
    o_rs3,
    o_rs3_type,
    o_rs3_valid,
    o_rs4,
    o_rs4_type,
    o_rs4_valid, 
    o_m_type,
    o_m_futype,
    o_m_opcode,
    o_m_branch_type,
    o_m_imm,
    o_m_ccshift,
    o_m_access_size,
    o_m_flags,
    o_fail
    
    ); //other ports to go to renaming module??? 

//Port declarations

    output [5:0] o_rd;
    output [5:0] o_rd2;
    output [5:0] o_rs1;
    output [5:0] o_rs2;
    output [5:0] o_rs3;
    output [5:0] o_rs4;
    output [7:0] o_m_flags;
    output [7:0] o_m_access_size;
    output [2:0] o_m_type; //
    output [3:0] o_m_futype;
    output [8:0] o_m_opcode;
    output [3:0] o_m_branch_type;
    output [63:0] o_m_imm;
    output [7:0] o_m_ccshift;
    output [3:0] o_rs1_type;
    output [3:0] o_rs2_type;
    output [3:0] o_rs3_type;
    output [3:0] o_rs4_type;
    output [3:0] o_rd_type;
    output [3:0] o_rd2_type;
    output o_rd_valid;
    output o_rd2_valid;
    output o_rs1_valid;
    output o_rs2_valid;
    output o_rs3_valid;
    output o_rs4_valid;
    output o_fail;
    

    input [31:0] instruction; //32-bit instruction that has been fetched
    // input reset ;

//Internal wiring

    reg [1:0] op; //bits 31-30 of inst
    reg [2:0] op2; //bits 24-22
    reg [5:0] op3; //bits 24-19
    reg [5:0] rd; //bits 29-25
    reg [3:0] rd_type;
    reg [5:0] rd2;
    reg [3:0] rd2_type;
    reg [5:0] rs1; //bits 18-14
    reg [3:0] rs1_type;
    reg [5:0] rs2; //bits 4-0
    reg [3:0] rs2_type;
    reg [5:0] rs3;
    reg [3:0] rs3_type;
    reg [5:0] rs4;
    reg [3:0] rs4_type;
    reg [4:0] opf_hi; //bits 13-9
    reg [3:0] opf_lo; //bits 8-5
    reg [3:0] cond; //bits 28-25
    reg [3:0] mcond; //bits 17-14
    reg [2:0] rcond; //bits 12-10
    reg i; //bit 13
    reg [7:0] asi; //bits 12-5     
    reg [21:0] x; //
    reg [1:0] movcc2; //
    reg [2:0] sdq; //
    reg rd_valid;
    reg rd2_valid;
    reg rs1_valid;
    reg rs2_valid;
    reg rs3_valid;
    reg rs4_valid;
    reg fail;
    // reg z;//general variable used for loops
    reg [7:0] m_flags;
    reg [7:0] m_access_size;
    reg [2:0] m_type; //
    reg [3:0] m_futype;
    reg [8:0] m_opcode;
    reg [3:0] m_branch_type;
    reg [63:0] m_imm;
    reg [7:0] m_ccshift;
    
    // Control register map between simics and opal

    // reg [5:0] app_control_reg_map[0:44] ;
    // reg [5:0] os_control_reg_map[0:44] ;

    // initial begin
    //     app_control_reg_map[0] = 0 ;
    //     app_control_reg_map[0] = 2 ;
    //     app_control_reg_map[1] = 43 ;
    //     app_control_reg_map[2] = 3 ;
    //     app_control_reg_map[3] = 6 ;
    //     app_control_reg_map[4] = 43 ;
    //     app_control_reg_map[5] = 0 ;
    //     app_control_reg_map[6] = 43 ;
    //     app_control_reg_map[7] = 43 ;
    //     app_control_reg_map[8] = 43 ;
    //     app_control_reg_map[9] = 43 ;
    //     app_control_reg_map[10] = 43 ;
    //     app_control_reg_map[11] = 43 ;
    //     app_control_reg_map[12] = 43 ;
    //     app_control_reg_map[13] = 43 ;
    //     app_control_reg_map[14] = 43 ;
    //     app_control_reg_map[15] = 43 ;
    //     app_control_reg_map[16] = 43 ;
    //     app_control_reg_map[17] = 43 ;
    //     app_control_reg_map[18] = 43 ;
    //     app_control_reg_map[19] = 8 ;
    //     app_control_reg_map[20] = 43 ;
    //     app_control_reg_map[21] = 43 ;
    //     app_control_reg_map[22] = 43 ;
    //     app_control_reg_map[23] = 9 ;
    //     app_control_reg_map[24] = 43 ;
    //     app_control_reg_map[25] = 43 ;
    //     app_control_reg_map[26] = 43 ;
    //     app_control_reg_map[27] = 43 ;
    //     app_control_reg_map[28] = 43 ;
    //     app_control_reg_map[29] = 43 ;
    //     app_control_reg_map[30] = 43 ;
    //     app_control_reg_map[31] = 43 ;
    //     app_control_reg_map[32] = 43 ;
    //     app_control_reg_map[33] = 43 ;
    //     app_control_reg_map[34] = 43 ;
    //     app_control_reg_map[35] = 43 ;
    //     app_control_reg_map[36] = 43 ;
    //     app_control_reg_map[37] = 43 ;
    //     app_control_reg_map[38] = 43 ;
    //     app_control_reg_map[39] = 43 ;
    //     app_control_reg_map[40] = 43 ;
    //     app_control_reg_map[41] = 43 ;
    //     app_control_reg_map[42] = 43 ;
    //     app_control_reg_map[43] = 43 ;
    //     app_control_reg_map[44] = 43 ;

    //     os_control_reg_map[0] = 14;
    //     os_control_reg_map[1] = 19;
    //     os_control_reg_map[2] = 24;
    //     os_control_reg_map[3] = 29;
    //     os_control_reg_map[4] = 43;
    //     os_control_reg_map[5] = 34;
    //     os_control_reg_map[6] = 11;
    //     os_control_reg_map[7] = 12;
    //     os_control_reg_map[8] = 13;
    //     os_control_reg_map[9] = 36;
    //     os_control_reg_map[10] = 37;
    //     os_control_reg_map[11] = 38;
    //     os_control_reg_map[12] = 41;
    //     os_control_reg_map[13] = 39;
    //     os_control_reg_map[14] = 40;
    //     os_control_reg_map[15] = 43;
    //     os_control_reg_map[16] = 43;
    //     os_control_reg_map[17] = 43;
    //     os_control_reg_map[18] = 43;
    //     os_control_reg_map[19] = 43;
    //     os_control_reg_map[20] = 43;
    //     os_control_reg_map[21] = 43;
    //     os_control_reg_map[22] = 43;
    //     os_control_reg_map[23] = 43;
    //     os_control_reg_map[24] = 43;
    //     os_control_reg_map[25] = 43;
    //     os_control_reg_map[26] = 43;
    //     os_control_reg_map[27] = 43;
    //     os_control_reg_map[28] = 43;
    //     os_control_reg_map[29] = 43;
    //     os_control_reg_map[30] = 43;
    //     os_control_reg_map[31] = 43;
    //     os_control_reg_map[32] = 43;
    //     os_control_reg_map[33] = 43;
    //     os_control_reg_map[34] = 43;
    //     os_control_reg_map[35] = 43;
    //     os_control_reg_map[36] = 43;
    //     os_control_reg_map[37] = 43;
    //     os_control_reg_map[38] = 43;
    //     os_control_reg_map[39] = 43;
    //     os_control_reg_map[40] = 43;
    //     os_control_reg_map[41] = 43;
    //     os_control_reg_map[42] = 43;
    //     os_control_reg_map[43] = 43;
    //     os_control_reg_map[44] = 43;
    // end


    assign o_rd			   = rd;
    assign o_rd2		   = rd2;
    assign o_rs1		   = rs1;
    assign o_rs2		   = rs2;
    assign o_rs3		   = rs3;
    assign o_rs4		   = rs4;

    assign o_rs1_type      = rs1_type;
    assign o_rs2_type      = rs2_type;
    assign o_rs3_type      = rs3_type;
    assign o_rs4_type      = rs4_type;
    assign o_rd_type       = rd_type;
    assign o_rd2_type      = rd2_type;

    assign o_rd_valid      = rd_valid;
    assign o_rd2_valid     = rd2_valid;
    assign o_rs1_valid     = rs1_valid;
    assign o_rs2_valid     = rs2_valid;
    assign o_rs3_valid     = rs3_valid;
    assign o_rs4_valid     = rs4_valid;

    assign o_m_flags       = m_flags;
    assign o_m_access_size = m_access_size;
    assign o_m_type        = m_type; //
    assign o_m_futype      = m_futype;
    assign o_m_opcode      = m_opcode;
    assign o_m_branch_type = m_branch_type;
    assign o_m_imm         = m_imm;
    assign o_m_ccshift     = m_ccshift;
    assign o_fail          = fail;

	// wire [5:0] map1 ;
	// wire [5:0] map2 ;
	// wire [5:0] map3 ;
	// wire [5:0] map4 ;
	// wire [5:0] map5 ;
	// app_control_reg_map MAP1(map1, rs1) ;
	// os_control_reg_map MAP2(map2, rs1) ;
	// app_control_reg_map MAP3(map3, rd) ;
	// app_control_reg_map MAP4(map4, rd) ;
	// os_control_reg_map MAP5(map5, rd) ;

//Code begins here

always @(instruction) // or reset)
begin
 // if(reset) begin
    rd  = 6'd0 ;
    rd2 = 6'd0 ;
    rs1 = 6'd0 ;
    rs2 = 6'd0 ;
    rs3 = 6'd0 ;
    rs4 = 6'd0 ;
    m_flags = 8'd0 ;
    m_access_size = 8'd0 ;
    m_type = 3'd0 ; //
    m_futype = 4'd0 ;
    m_opcode = 9'd0 ;
    m_branch_type = 4'd0 ;
    m_imm = 64'h ffffffffffffffff ;
    m_ccshift = 8'd0 ;
    rs1_type = 4'd0 ;
    rs2_type = 4'd0 ;
    rs3_type = 4'd0 ;
    rs4_type = 4'd0 ;
    rd_type = 4'd0 ;
    rd2_type = 4'd0 ;
    rd_valid = 1'd0 ;
    rd2_valid = 1'd0 ;
    rs1_valid = 1'd0 ;
    rs2_valid = 1'd0 ;
    rs3_valid = 1'd0 ;
    rs4_valid = 1'd0 ;
    fail = 1'd0 ;

 // end else begin
     op     = instruction[31:30] ;
     op2    = instruction[24:22] ;
     op3    = instruction[24:19] ;
     rd     = instruction[29:25] ;
     rs1    = instruction[18:14] ;
     rs2    = instruction[4:0] ;
     opf_hi = instruction[13:9] ;
     opf_lo = instruction[8:5] ;
     cond   = instruction[28:25] ;
     mcond  = instruction[17:14] ;
     rcond  = instruction[12:10] ;
     i      = instruction[13] ;
     asi    = instruction[12:5] ;
     x      = instruction[21:0] ;

     case(op) 
    0:  begin
        case(op2) 
        0: // trap
            begin
              m_type = `DYN_EXECUTE;
              m_futype = `FU_NONE;
              m_opcode = `i_nop;
            end
            //break;

        1: // bpcc
            begin
              m_type = `DYN_CONTROL;
              m_futype = `FU_BRANCH;
              m_branch_type = `BRANCH_PCOND;
              m_imm = {{45{instruction[18]}},instruction[18:0]} ;
              // for(z=19;z<64;z=z+1) begin
              //   m_imm[z] = instruction[18];
              // end
              m_imm = m_imm << 2 ;
              m_ccshift = instruction[21:20] << 1; //NEEDS 1LSHFT
              if(m_ccshift != 8'd0 && m_ccshift !=8'd4) 
                  fail = 1 ;
              rs4 = `CONTROL_ISREADY ;
              rs4_type = `RID_CONTROL ;
              rs4_valid = 1 ;
              m_flags = m_flags | `SI_COMPLEX_OP ;

              case(cond) 
              0:     begin
                      m_branch_type = `BRANCH_UNCOND;// bpn
                      m_opcode = `i_bpn;
                      //break;
                     end
              1:    m_opcode = `i_bpe;// bpe
                  //break;
              2:    m_opcode = `i_bple;// bple
                  //break;
              3:    m_opcode = `i_bpl;// bpl
                  //break;
              4:    m_opcode = `i_bpleu;// bpleu
                  //break;
              5:    m_opcode = `i_bpcs;// bpcs
                  //break;
              6:    m_opcode = `i_bpneg;// bpneg
                  //break;
              7:    m_opcode = `i_bpvs;// bpvs
                  //break;
              8:    begin
			  		m_branch_type = `BRANCH_UNCOND ;
					m_opcode = `i_bpa;// bpa
					end
                  //break;
              9:    m_opcode = `i_bpne;// bpne
                  //break;
              10:    m_opcode = `i_bpg;// bpg
                  //break;
              11:    m_opcode = `i_bpge;// bpge
                  //break;
              12:    m_opcode = `i_bpgu;// bpgu
                  //break;
              13:    m_opcode = `i_bpcc;// bpcc
                  //break;
              14:    m_opcode = `i_bppos;// bppos
                  //break;
              15:    m_opcode = `i_bpvc;// bppos
                  //break;
              default : fail=1;
              endcase 
            end
            //break;

        2: // bicc
            begin
            m_type = `DYN_CONTROL;
            m_futype = `FU_BRANCH;
            m_branch_type = `BRANCH_COND;
            m_imm = {{42{instruction[21]}},instruction[21:0]};//NEEDS SEXT
            // for(z=22;z<64;z=z+1) begin
            //   m_imm[z] = instruction[21];
            // end
            m_imm = m_imm << 2 ;
            m_ccshift = 0;
            rs3 = `REG_CC_CCR ;
            rs3_type = `RID_CC;
            rs3_valid = 1 ;
            rs4 = `CONTROL_ISREADY;
            rs4_type = `RID_CONTROL ;
            rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP;
            
            case(cond)
            0:    begin
                m_branch_type = `BRANCH_UNCOND;// bn
                m_opcode = `i_bn;
                end
                //break;
            1:    m_opcode = `i_be;// be //break;
            2:    m_opcode = `i_ble;// ble //break;
            3:    m_opcode = `i_bl;// bl //break;
            4:    m_opcode = `i_bleu;// bleu //break;
            5:    m_opcode = `i_bcs;// bcs //break;
            6:    m_opcode = `i_bneg;// bneg //break;
            7:    m_opcode = `i_bvs;// bvs //break;
            8:    begin
                m_branch_type = `BRANCH_UNCOND;// ba
                m_opcode = `i_ba;
                end
                //break;
            9:    m_opcode = `i_bne;// bne //break;
            10:    m_opcode = `i_bg;// bg //break;
            11:    m_opcode = `i_bge;// bge //break;
            12:    m_opcode = `i_bgu;// bgu //break;
            13:    m_opcode = `i_bcc;// bcc //break;
            14:    m_opcode = `i_bpos;// bpos //break;
            15:    m_opcode = `i_bpos;// bvs //break;
            default : fail = 1;// 
            endcase 
            end
            //break;
        
        3: // bpr
            begin
                m_type = `DYN_CONTROL;
                m_futype = `FU_BRANCH;
                m_branch_type = `BRANCH_PCOND;
                if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                else rs1_type = `RID_INT;
                rs1_valid = 1 ;
                x = ((instruction[21:20] << 14) | instruction[13:0]); //INSTRUCTION 21:20 needs 14LSHFT!
				m_imm = {{48{x[15]}},x[15:0]};
                // for(z=22;z<64;z=z+1) begin
                //   m_imm[z] = x[21];
                // end
                m_imm = m_imm << 2 ;
                rs4 = `CONTROL_ISREADY ;
                rs4_type = `RID_CONTROL ;
                rs4_valid = 1 ;
                m_flags = m_flags | `SI_COMPLEX_OP;

                
                case(cond)
                0:  ;    //reserved //break;
                1:     m_opcode = `i_brz; //break;
                2:    m_opcode = `i_brlez; //break;
                3:    m_opcode = `i_brlz; //break;
                4:    ; //reserved //break;
                5:    m_opcode = `i_brnz; //break;
                6:    m_opcode = `i_brgz; //break;
                7:    m_opcode = `i_brgez; //break;
                default : fail = 1;// 
                endcase
            end
            //break;

        4: // sethi
            begin
                m_type = `DYN_EXECUTE;
                m_futype = `FU_NONE;
                x = instruction[21:0];
                if(rd==0 && x==0)
                begin
                    m_opcode = `i_nop;
                end else if(rd==0 && x!=0) begin
                    m_opcode = `i_sethi;
                end else begin
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    m_opcode = `i_sethi;
                    m_imm = instruction[21:0] << 10;//NEEDS 10LSHFT
                    m_flags = m_flags | `SI_ISIMMEDIATE;
                end
            end
            //break;
     
        5: // fpbfcc
            begin
            m_type = `DYN_CONTROL;
            m_futype = `FU_BRANCH;
            m_branch_type = `BRANCH_PCOND;
			m_imm = {{46{instruction[18]}},instruction[18:0]};
            // for(z=18;z<64;z=z+1) begin
            //   m_imm[z] = x[18];
            // end
            m_imm = m_imm << 2 ;
            rs4 = `CONTROL_ISREADY;
            rs4_type = `RID_CONTROL ;
            rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP;
            
            movcc2 = instruction[21:20];
            rs3_type = `RID_CC ;
            rs3_valid = 1 ;
            case(movcc2) 
                0:    rs3 = `REG_CC_FCC0;
                1:    rs3 = `REG_CC_FCC1;
                2:    rs3 = `REG_CC_FCC2;    
                3:    rs3 = `REG_CC_FCC3;
                default: fail = 1;
            endcase
            
            case(cond) 
            0:    m_opcode = `i_fbpn;// fbpn
            1:  m_opcode = `i_fbpne;// fbpne
            2:  m_opcode = `i_fbplg;// fbplg
            3:  m_opcode = `i_fbpul;// fbpul
            4:    m_opcode = `i_fbpl;// fbpl
            5:  m_opcode = `i_fbpug;// fbpug
            6:  m_opcode = `i_fbpg;// fbpg
            7:  m_opcode = `i_fbpu;// fbpu
            8:  m_opcode = `i_fbpa;// fbpa
            9:  m_opcode = `i_fbpe;// fbpe
            10: m_opcode = `i_fbpue;// fbpue
            11: m_opcode = `i_fbpge;// fbpge
            12: m_opcode = `i_fbpuge;// fbpuge
            13: m_opcode = `i_fbple;// fbple
            14: m_opcode = `i_fbpule;// fbpule
            15: m_opcode = `i_fbpo;// fbpo
            default : fail = 1;//
            endcase 
            end
            //break;
        
        6: //fbfcc
            begin
            m_type = `DYN_CONTROL;
            m_branch_type = `BRANCH_COND;
            // m_imm = instruction[21:0] ;//NEEDS SEXT
			m_imm = {{42{instruction[21]}},instruction[21:0]};
            //for(z=22;z<64;z=z+1) begin
              // m_imm[z] = instruction[21];
            // end
            m_imm = m_imm << 2 ;
            rs3 = `REG_CC_FCC0 ;
            rs3_type = `RID_CC;
            rs3_valid = 1 ;
            rs4 = `CONTROL_ISREADY ;
            rs4_type = `RID_CONTROL ;
            rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP;

            case(cond) 
                0:     begin
                        m_branch_type = `BRANCH_UNCOND;// fbn
                        m_opcode = `i_fbn;
                    end
                1:  m_opcode = `i_fbne;// fbne
                2:  m_opcode = `i_fblg;// fblg
                3:  m_opcode = `i_fbul;// fbul
                4:  m_opcode = `i_fbl;// fbl
                5:  m_opcode = `i_fbug;// fbug
                6:  m_opcode = `i_fbg;// fbg
                7:  m_opcode = `i_fbu;// fbu
                8:  begin
                        m_branch_type = `BRANCH_UNCOND;// fba
                        m_opcode = `i_fba;
                    end
                9:  m_opcode = `i_fbe;// fbe
                10: m_opcode = `i_fbue;// fbue
                11: m_opcode = `i_fbge;// fbge
                12: m_opcode = `i_fbuge;// fbuge
                13: m_opcode = `i_fble;// fble
                14: m_opcode = `i_fbule;// fbule
                15: m_opcode = `i_fbo;// fbo
                default : fail = 1;//  
            endcase 
            end
        
        7:  fail = 1;//
        default : fail = 1;// 
        endcase 
        end
        ////break;//BREAK OUT OF OP0 INSTRUCTIONS
        

    1:    begin
            m_type = `DYN_CONTROL;
            m_futype = `FU_BRANCH;
            m_branch_type = `BRANCH_CALL;
            m_opcode = `i_call;
			m_imm = {{32{instruction[29]}},instruction[29:0]} << 2 ;
            // m_imm = instruction[29:0];//NEEDS SEXT
            // for(z=30;z<64;z=z+1) begin
            //   m_imm[z] = instruction[29];
            // end
            rd_type = `RID_INT;
            rd = 15 ;
            rd_valid = 1 ;
        ////break;//BREAK OUT OF OP1 INSTRUCTIONS
        end

    2:    begin
        m_type = `DYN_EXECUTE;
        
        case(op3)
            0:     begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
						m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                        //   for(z=13;z<64;z=z+1) begin
                        //     m_imm[z] = instruction[12];
                        //  end
                    end
                    m_opcode = `i_add; //add
                end
            1:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                        //   for(z=13;z<64;z=z+1) begin
                        //     m_imm[z] = instruction[12];
                        //   end
                    end
                    m_opcode = `i_and; //and
                end
            2:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
                        // m_imm = instruction[12:0];//NEEDS SEXT
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm[63:13] = {50{m_imm[12]}};
                          // for(z=13;z<64;z=z+1) begin
                        //     m_imm[z] = instruction[12];
                          // end
                    end
                    if(rs1==0)
                        m_opcode = `i_mov; //or
                    else
                        m_opcode = `i_or;
                end
            3:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                        //   for(z=13;z<64;z=z+1) begin
                        //     m_imm[z] = instruction[12];
                        //   end
                    end
                    m_opcode = `i_xor; //xor
                end
            4:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                        //   for(z=13;z<64;z=z+1) begin
                        //     m_imm[z] = instruction[12];
                        //  end
                    end
                    m_opcode = `i_sub; //sub
                end
            5:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                        // for(z=13;z<64;z=z+1) begin
                        // m_imm[z] = instruction[12];
                        // end
                    end
                    m_opcode = `i_andn; //andn
                end
            6:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                      // for(z=13;z<64;z=z+1) begin
                        // m_imm[z] = instruction[12];
                      // end
                    end
                    m_opcode = `i_orn; //orn
                end
            7:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                      // for(z=13;z<64;z=z+1) begin
                        // m_imm[z] = instruction[12];
                      // end
                    end
                    if(rs2==0 && i==0)
                        m_opcode = `i_not; //xnor
                    else    m_opcode = `i_xnor;
                end
            8:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                         // end
                    end    
                    rs3 = `REG_CC_CCR ;
                    rs3_type = `RID_CC;
					rs3_valid = 1 ;
                    m_opcode = `i_addc; //addc
                end
            9:    begin
                    m_futype = `FU_INTMULT;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    m_opcode = `i_mulx; //mulx
                end
            10:    begin
                    m_futype = `FU_INTMULT;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                        // for(z=13;z<64;z=z+1) begin
                        // m_imm[z] = instruction[12];
                        // end
                    end
                    rs4 = `CONTROL_Y ;
                    rd2 = `CONTROL_Y ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    rd2_type = `RID_CONTROL;
                    rd2_valid = 1 ;
                    m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP;
                    m_opcode = `i_umul; //umul
                end
            11:    begin
                    m_futype = `FU_INTMULT;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    rs4 = `CONTROL_Y ;
                    rd2 = `CONTROL_Y ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    rd2_type = `RID_CONTROL;
                    rd2_valid = 1 ;
                    m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP;
                    m_opcode = `i_smul; //smul
                end
            12:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    rs3 = `REG_CC_CCR ;
                    rs3_type = `RID_CC;
					rs3_valid = 1 ;
                    m_opcode = `i_subc; //subc
                end
            13:    begin
                    m_futype = `FU_INTDIV;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    m_opcode = `i_udivx; //udivx
                end
            14:    begin
                    m_futype = `FU_INTDIV;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rd_valid = 1 ;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    rs4 = `CONTROL_Y ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    m_flags = m_flags | `SI_COMPLEX_OP;
                    m_opcode = `i_udiv; //udiv
                    // m_flags = m_flags | `SI_COMPLEX_OP;
                end
            15:    begin
                    m_futype = `FU_INTDIV;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    rs4 = `CONTROL_Y ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    m_flags = m_flags | `SI_COMPLEX_OP;
                    m_opcode = `i_sdiv; //sdiv
                    // m_flags = m_flags | `SI_COMPLEX_OP;
                end
            16:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    rd2 = `REG_CC_CCR ;
                    rd2_type = `RID_CC ;
                    rd2_valid = 1 ;
                    m_opcode = `i_addcc; //addcc
                end
            17:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    rd2 = `REG_CC_CCR ;
                    rd2_type = `RID_CC ;
                    rd2_valid = 1 ;
                    // rs3_type = `RID_CC;
                    m_opcode = `i_andcc; //andcc
                end
            18:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    rd2 = `REG_CC_CCR ;
                    rd2_type = `RID_CC ;
                    rd2_valid = 1 ;
                    // rs3_type = `RID_CC;
                    m_opcode = `i_orcc; //orcc
                end
            19:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    rd2 = `REG_CC_CCR ;
                    rd2_type = `RID_CC ;
                    rd2_valid = 1 ;
                    // rs3_type = `RID_CC;
                    m_opcode = `i_xorcc; //xorcc
                end
            20:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    rd2 = `REG_CC_CCR ;
                    rd2_type = `RID_CC ;
                    rd2_valid = 1 ;
                    // rs3_type = `RID_CC;
                    if(rd==0) m_opcode = `i_cmp; //subcc
                    else    m_opcode = `i_subcc;
                end
            21:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    rd2 = `REG_CC_CCR ;
                    rd2_type = `RID_CC ;
                    rd2_valid = 1 ;
                    // rs3_type = `RID_CC;
                    m_opcode = `i_andncc; //andncc
                end
            22:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    rd2 = `REG_CC_CCR ;
                    rd2_type = `RID_CC ;
                    rd2_valid = 1 ;
                    // rs3_type = `RID_CC;
                    m_opcode = `i_orncc; //orncc
                end
            23:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    rd2 = `REG_CC_CCR ;
                    rd2_type = `RID_CC ;
                    rd2_valid = 1 ;
                    // rs3_type = `RID_CC;
                    m_opcode = `i_xnorcc; //xnorcc
                end
            24:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    rd2 = `REG_CC_CCR ;
                    rd2_type = `RID_CC ;
                    rd2_valid = 1 ;
                    rs3 = `REG_CC_CCR ;
                    rs3_type = `RID_CC;
					rs3_valid = 1 ;
                    m_opcode = `i_addccc; //addccc
                end
            25:    begin
                    fail = 1;  //FAIL
                end
            26:    begin
                    m_futype = `FU_INTMULT;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end                
                    m_opcode = `i_umulcc; //umulcc
                    rs4 = `CONTROL_Y ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    rd2 = 0 ;
                    rd2_type = `RID_CONTAINER ;
                    rd2_valid = 1 ;
                    m_flags = m_flags | `SI_WRITE_CONTROL;
                end
            27:    begin
                    m_futype = `FU_INTMULT;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end        
                    rs4 = `CONTROL_Y ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    rd2 = 0 ;
                    rd2_type = `RID_CONTAINER;
                    rd2_valid = 1 ;
                    m_flags = m_flags | `SI_WRITE_CONTROL;
                    m_opcode = `i_smulcc; //smulcc
                end
            28:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end        
                    rs3 = `REG_CC_CCR ;
                    rs3_type = `RID_CC;
					rs3_valid = 1 ;
                    m_opcode = `i_subccc; //subccc
                end
            29:    begin
                    fail = 1; //FAIL
                end
            30:    begin
                    m_futype = `FU_INTDIV;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end        
                    rd2 = `REG_CC_CCR ;
                    rd2_type = `RID_CC ;
                    rd2_valid = 1 ;
                    rs4 = `CONTROL_Y ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    m_flags = m_flags | `SI_COMPLEX_OP;
                    m_opcode = `i_udivcc; //udivcc
                // m_flags = m_flags | `SI_COMPLEX_OP;
                end
            31:    begin
                    m_futype = `FU_INTDIV;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end        
                    rd2 = `REG_CC_CCR ;
                    rd2_type = `RID_CC ;
                    rd2_valid = 1 ;
                    rs4 = `CONTROL_Y ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    m_flags = m_flags | `SI_COMPLEX_OP;
                    m_opcode = `i_sdivcc; //sdivcc
                    // m_flags = m_flags | `SI_CONTROL_OP;
                end
            32:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end        
                    m_opcode = `i_taddcc; //taddcc
                end
            33:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end    
                    m_opcode = `i_tsubcc; //tsubcc
                end
            34:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end    
                    m_opcode = `i_taddcctv; //taddcctv
                end
            35:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
						rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end    
                    m_opcode = `i_tsubcctv; //tsubcctv
                end
            36:    begin
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end    
                    rs3 = `REG_CC_CCR ;
                    rs3_type = `RID_CC;
					rs3_valid = 1 ;
                    rs4 = `CONTROL_Y ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    rd2 = 0 ;
                    rd2_type = `RID_CONTAINER;
                    rd2_valid = 1 ;
                    m_flags = m_flags | `SI_COMPLEX_OP;
                    m_flags = m_flags | `SI_WRITE_CONTROL;
                    m_opcode = `i_mulscc; //mulscc
                end
            37:    begin
                    x=instruction[12];
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    end else if(x==1) begin
                        m_imm = instruction[5:0];
                        m_flags = m_flags | `SI_ISIMMEDIATE;
                    end else if(x==0) begin
                        m_imm = instruction[4:0];
                        m_flags = m_flags | `SI_ISIMMEDIATE;
                    end
                    if(x) m_opcode = `i_sllx; //sll
                    else     m_opcode = `i_sll;
                end
            38:    begin
                    x=instruction[12];
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    if(i==0) begin
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    end else if(x==1) begin
                        m_imm = instruction[5:0];
                        m_flags = m_flags | `SI_ISIMMEDIATE;
                    end else if(x==0) begin
                        m_imm = instruction[4:0];
                        m_flags = m_flags | `SI_ISIMMEDIATE;
                    end
                    x=instruction[12];
                    if(x)
                        m_opcode = `i_srlx; //srl
                    else     m_opcode = `i_srl;
                end
            39:    begin
                    x=instruction[12];
                    m_futype = `FU_INTALU;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
                    rs1_valid = 1 ;
                    if(i==0) begin
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    end else if(x==1) begin
                        m_imm = instruction[5:0];//NEEDS SEXT
                        m_flags = m_flags | `SI_ISIMMEDIATE;
                    end else if(x==0) begin
                        m_imm = instruction[4:0];//NEEDS SEXT
                        m_flags = m_flags | `SI_ISIMMEDIATE;
                    end
                    if(x) m_opcode = `i_srax; //sra
                    else m_opcode = `i_sra;
                end
            40:    begin
                     m_futype = `FU_NONE;
                     if(rs1==15 && rd==0) begin
                         m_type = `DYN_EXECUTE;
                         m_opcode = `i_stbar; //rd
                     end else begin
                        m_type = `DYN_EXECUTE;
                        if(rd<8) rd_type = `RID_INT_GLOBAL;
                        else rd_type = `RID_INT;
                        rd_valid = 1 ;
						case(rs1)	
					    0:  x[5:0] = 2 ;
					    1:  x[5:0] = 43 ;
					    2:  x[5:0] = 3 ;
					    3:  x[5:0] = 6 ;
					    4:  x[5:0] = 43 ;
					    5:  x[5:0] = 0 ;
					    6:  x[5:0] = 43 ;
					    7:  x[5:0] = 43 ;
					    8:  x[5:0] = 43 ;
					    9:  x[5:0] = 43 ;
					    10: x[5:0] = 43 ;
					    11: x[5:0] = 43 ;
					    12: x[5:0] = 43 ;
					    13: x[5:0] = 43 ;
					    14: x[5:0] = 43 ;
					    15: x[5:0] = 43 ;
					    16: x[5:0] = 43 ;
					    17: x[5:0] = 43 ;
					    18: x[5:0] = 43 ;
					    19: x[5:0] = 8 ;
					    20: x[5:0] = 43 ;
					    21: x[5:0] = 43 ;
					    22: x[5:0] = 43 ;
					    23: x[5:0] = 9 ;
					    24: x[5:0] = 43 ;
					    25: x[5:0] = 43 ;
					    26: x[5:0] = 43 ;
					    27: x[5:0] = 43 ;
					    28: x[5:0] = 43 ;
					    29: x[5:0] = 43 ;
					    30: x[5:0] = 43 ;
					    31: x[5:0] = 43 ;
					    32: x[5:0] = 43 ;
					    33: x[5:0] = 43 ;
					    34: x[5:0] = 43 ;
					    35: x[5:0] = 43 ;
					    36: x[5:0] = 43 ;
					    37: x[5:0] = 43 ;
					    38: x[5:0] = 43 ;
					    39: x[5:0] = 43 ;
					    40: x[5:0] = 43 ;
					    41: x[5:0] = 43 ;
					    42: x[5:0] = 43 ;
					    43: x[5:0] = 43 ;
					    44: x[5:0] = 43 ;
						endcase
						x[21:6] = 0 ;
						// x[5:0] = map1 ;
						// app_control_reg_map MAP1(x[5:0], rs1) ;
                        // x = app_control_reg_map[rs1] ;
                         if(x==`CONTROL_CCR) begin
                            rs3 = `REG_CC_CCR ;
                             rs3_type = `RID_CC;
							 rs3_valid = 1 ;
                             m_opcode = `i_rdcc;
                         end else begin
						 	rs4 = x ;    
						 	rs4_type = `RID_CONTROL;
						 	rs4_valid = 1 ;
							m_flags = m_flags | `SI_COMPLEX_OP;
							m_opcode = `i_rd;
                             // m_flags = m_flags | `SI_COMPLEX_OP;
                         end
                     end
                end
            41:    begin
                     fail = 1; //FAIL
                 end
            42:    begin
                    m_futype = `FU_NONE;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
					case(rs1)	
				    0:  rs4 = 14;
				    1:  rs4 = 19;
				    2:  rs4 = 24;
				    3:  rs4 = 29;
				    4:  rs4 = 43;
				    5:  rs4 = 34;
				    6:  rs4 = 11;
				    7:  rs4 = 12;
				    8:  rs4 = 13;
				    9:  rs4 = 36;
				    10: rs4 = 37;
				    11: rs4 = 38;
				    12: rs4 = 41;
				    13: rs4 = 39;
				    14: rs4 = 40;
				    15: rs4 = 43;
				    16: rs4 = 43;
				    17: rs4 = 43;
				    18: rs4 = 43;
				    19: rs4 = 43;
				    20: rs4 = 43;
				    21: rs4 = 43;
				    22: rs4 = 43;
				    23: rs4 = 43;
				    24: rs4 = 43;
				    25: rs4 = 43;
				    26: rs4 = 43;
				    27: rs4 = 43;
				    28: rs4 = 43;
				    29: rs4 = 43;
				    30: rs4 = 43;
				    31: rs4 = 43;
				    32: rs4 = 43;
				    33: rs4 = 43;
				    34: rs4 = 43;
				    35: rs4 = 43;
				    36: rs4 = 43;
				    37: rs4 = 43;
				    38: rs4 = 43;
				    39: rs4 = 43;
				    40: rs4 = 43;
				    41: rs4 = 43;
				    42: rs4 = 43;
				    43: rs4 = 43;
				    44: rs4 = 43;
					endcase
					// rs4 = map2 ;
					// os_control_reg_map MAP2(rs4, rs1) ;
                    // rs4 = os_control_reg_map[rs1] ;
                    rs4_type = `RID_CONTROL ;
					rs4_valid = 1 ;
                    m_flags = m_flags | `SI_COMPLEX_OP ;
                    m_opcode = `i_rdpr; //rdpr
                end
            43:    begin
                    m_futype = `FU_NONE;
                    if(rd==0 && instruction[18:0]==0) begin
                        rs4 = `CONTROL_CWP ;
                        rs4_type = `RID_CONTROL;
						rs4_valid = 1 ;
                        m_flags = m_flags | `SI_COMPLEX_OP;
                        m_opcode = `i_flushw; //flushw
                        // m_flags = m_flags | `SI_COMPLEX_OP;
                    end else fail=1;
                end
            44:    begin
                    m_futype = `FU_NONE;
                    movcc2=instruction[18];
                    m_futype = `FU_NONE;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{53{instruction[10]}},instruction[10:0]};
                        // m_imm = instruction[10:0];//NEEDS SEXT
                        //   for(z=11;z<64;z=z+1) begin
                        //     m_imm[z] = instruction[10];
                        //   end
                    end
                    if(movcc2) begin
                        rs3 = `REG_CC_CCR ;
                        rs3_type = `RID_CC;
						rs3_valid = 1 ;
                        m_ccshift = instruction[12:11] << 1;//NEEDS 1LSHFT
                        if(m_ccshift !=0 && m_ccshift !=4)
                            fail = 1;
                        case(mcond)
                            0:    m_opcode = `i_movn; //movn
                            1:    m_opcode = `i_move; //move
                            2:    m_opcode = `i_movle; //movle
                            3:    m_opcode = `i_movl; //movl
                            4:    m_opcode = `i_movleu; //movleu
                            5:    m_opcode = `i_movcs; //movcs
                            6:    m_opcode = `i_movneg; //movneg
                            7:    m_opcode = `i_movvs; //movvs
                            8:    m_opcode = `i_mova; //mova
                            9:    m_opcode = `i_movne; //movne
                            10:    m_opcode = `i_movg; //movg
                            11:    m_opcode = `i_movge; //movge
                            12:    m_opcode = `i_movgu; //movgu
                            13:    m_opcode = `i_movcc; //movcc
                            14:    m_opcode = `i_movpos; //movpos
                            15:    m_opcode = `i_movvc; //movvc
                            default: fail = 1;//FAIL
                        endcase 
                    end else begin
                        movcc2 = instruction[12:11];
                        rs3_type = `RID_CC ;
						rs3_valid = 1 ;
                        case(movcc2) //SETTING UP CONTROL REGISTERS
                            0:    rs3 = `REG_CC_FCC0;
                            1:    rs3 = `REG_CC_FCC1;
                            2:    rs3 = `REG_CC_FCC2;
                            3:    rs3 = `REG_CC_FCC3;
                            default: fail = 1;
                        endcase 
                        case(mcond)
                            0:    m_opcode = `i_movfn; //movfn
                            1:    m_opcode = `i_movfne; //movfne
                            2:    m_opcode = `i_movflg; //movflg
                            3:    m_opcode = `i_movful; //movful
                            4:    m_opcode = `i_movfl; //movfl
                            5:    m_opcode = `i_movfug; //movfug
                            6:    m_opcode = `i_movfg; //movfg
                            7:    m_opcode = `i_movfu; //movfu
                            8:    m_opcode = `i_movfa; //movfa
                            9:    m_opcode = `i_movfe; //movfe
                            10:    m_opcode = `i_movfue; //movfue
                            11:    m_opcode = `i_movfge; //movfge
                            12:    m_opcode = `i_movfuge; //movfuge
                            13:    m_opcode = `i_movfle; //movfle
                            14:    m_opcode = `i_movfule; //movfule
                            15:    m_opcode = `i_movfo; //movfo
                            default: fail = 1;//FAIL
                        endcase 
                    end
                end
            45:    begin
                    m_futype = `FU_INTDIV ;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
					rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end    
                    m_opcode = `i_sdivx; //sdivx
                end
            46:    begin
                    case(rs1)
                    0:     begin
                            m_futype = `FU_INTALU;
                            if(rd<8) rd_type = `RID_INT_GLOBAL;
                            else rd_type = `RID_INT;
                            rd_valid = 1 ;
                            if(i==0) begin
                                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                                if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                                else rs2_type = `RID_INT;
                                rs2_valid = 1 ;
                            end else begin
                                m_flags = m_flags | `SI_ISIMMEDIATE;
                                m_imm=instruction[12:0]; //NEED SEXT
                            end
                            m_opcode = `i_popc; // popc
                        end
                    default: fail = 1; //FAIL
                    endcase
                end 
            47:    begin
                    m_futype = `FU_NONE;
                    if(rd<8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
					rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{54{instruction[9]}},instruction[9:0]};
                        // m_imm = instruction[9:0];//NEEDS SEXT
                        //   for(z=10;z<64;z=z+1) begin
                        //     m_imm[z] = instruction[9];
                        //   end
                    end
					rs3 = rs1 ;
                    if(rs1<8) rs3_type = `RID_INT_GLOBAL;
                    else rs3_type = `RID_INT;
					rs3_valid = 1 ;

					rs1 = rd ;
                    if(rd<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
					rs1_valid = 1 ;

                    case(rcond)
                        0:     fail = 1;
                        1:    m_opcode = `i_movrz; //movrz
                        2:    m_opcode = `i_movrlez; //movrlez
                        3:    m_opcode = `i_movrlz; //movrlz
                        4:    fail = 1;
                        5:    m_opcode = `i_movrnz; //movrnz
                        6:    m_opcode = `i_movrgz; //movrgz
                        7:    m_opcode = `i_movrgez; //movrgez
                        default: fail = 1;
                    endcase 
                end
            48:    begin
                    m_futype = `FU_NONE;
                    case(rd)
                        2:    begin
                                rd2 = `REG_CC_CCR ;
                                rd2_type = `RID_CC ;
                                rd2_valid = 1 ;
                                m_opcode = `i_wrcc; //wrcc
                            end
                        default: begin
								case(rd)	
							    0:  begin rs4 = 2 ;  rd2 = 2 ;end
							    1:  begin rs4 = 43 ; rd2 = 43 ;end
							    2:  begin rs4 = 3 ;  rd2 = 3 ;end
							    3:  begin rs4 = 6 ;  rd2 = 6 ;end
							    4:  begin rs4 = 43 ; rd2 = 43 ;end
							    5:  begin rs4 = 0 ;  rd2 = 0 ;end
							    6:  begin rs4 = 43 ; rd2 = 43 ;end
							    7:  begin rs4 = 43 ; rd2 = 43 ;end
							    8:  begin rs4 = 43 ; rd2 = 43 ;end
							    9:  begin rs4 = 43 ; rd2 = 43 ;end
							    10: begin rs4 = 43 ; rd2 = 43 ;end
							    11: begin rs4 = 43 ; rd2 = 43 ;end
							    12: begin rs4 = 43 ; rd2 = 43 ;end
							    13: begin rs4 = 43 ; rd2 = 43 ;end
							    14: begin rs4 = 43 ; rd2 = 43 ;end
							    15: begin rs4 = 43 ; rd2 = 43 ;end
							    16: begin rs4 = 43 ; rd2 = 43 ;end
							    17: begin rs4 = 43 ; rd2 = 43 ;end
							    18: begin rs4 = 43 ; rd2 = 43 ;end
							    19: begin rs4 = 8 ;  rd2 = 8 ;end
							    20: begin rs4 = 43 ; rd2 = 43 ;end
							    21: begin rs4 = 43 ; rd2 = 43 ;end
							    22: begin rs4 = 43 ; rd2 = 43 ;end
							    23: begin rs4 = 9 ;  rd2 = 9 ;end
							    24: begin rs4 = 43 ; rd2 = 43 ;end
							    25: begin rs4 = 43 ; rd2 = 43 ;end
							    26: begin rs4 = 43 ; rd2 = 43 ;end
							    27: begin rs4 = 43 ; rd2 = 43 ;end
							    28: begin rs4 = 43 ; rd2 = 43 ;end
							    29: begin rs4 = 43 ; rd2 = 43 ;end
							    30: begin rs4 = 43 ; rd2 = 43 ;end
							    31: begin rs4 = 43 ; rd2 = 43 ;end
							    32: begin rs4 = 43 ; rd2 = 43 ;end
							    33: begin rs4 = 43 ; rd2 = 43 ;end
							    34: begin rs4 = 43 ; rd2 = 43 ;end
							    35: begin rs4 = 43 ; rd2 = 43 ;end
							    36: begin rs4 = 43 ; rd2 = 43 ;end
							    37: begin rs4 = 43 ; rd2 = 43 ;end
							    38: begin rs4 = 43 ; rd2 = 43 ;end
							    39: begin rs4 = 43 ; rd2 = 43 ;end
							    40: begin rs4 = 43 ; rd2 = 43 ;end
							    41: begin rs4 = 43 ; rd2 = 43 ;end
							    42: begin rs4 = 43 ; rd2 = 43 ;end
							    43: begin rs4 = 43 ; rd2 = 43 ;end
							    44: begin rs4 = 43 ; rd2 = 43 ;end
								endcase 
								// rd2 = map4 ;
								// app_control_reg_map MAP3(rs4, rd) ;
								// app_control_reg_map MAP4(rd2, rd) ;
                                // rs4 = app_control_reg_map[rd] ;
                                // rd2 = app_control_reg_map[rd] ;
                                rs4_type = `RID_CONTROL ;
								rs4_valid = 1 ;
                                rd2_type = `RID_CONTROL ;
								rd2_valid = 1 ;
                                m_flags = m_flags | `SI_COMPLEX_OP | `SI_WRITE_CONTROL ;
                                m_opcode = `i_wr; //wr
                            end
                    endcase
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
					rs1_valid = 1 ;
  
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE; 
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                    end
                end
            49:    begin
                    case(rd)
                        0:    begin
                                m_opcode = `i_saved; //saved
                                m_futype = `FU_NONE;
                                rs4 = `CONTROL_PSTATE ;
                                rd2 = `CONTROL_PSTATE ;
                                rs4_type = `RID_CONTROL;
								rs4_valid = 1 ;
                                rd2_type = `RID_CONTROL;
                                m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP;
                                // m_flags = m_flags | `SI_COMPLEX_OP;
                            end
                        1:    begin
                                m_opcode = `i_restored; //restored
                                m_futype = `FU_NONE;
                                rs4 = `CONTROL_PSTATE ;
                                rd2 = `CONTROL_PSTATE ;
                                rs4_type = `RID_CONTROL;
								rs4_valid = 1 ;
                                rd2_type = `RID_CONTROL;
								rd2_valid = 1 ;
								m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP;
                                // m_flags = m_flags | `SI_COMPLEX_OP;
                            end
                        default: fail = 1;
                    endcase 
                end
            50:    begin
                    m_futype = `FU_NONE;
					case(rd)	
				    6'd0:  x[5:0] = 14;
				    6'd1:  x[5:0] = 19;
				    6'd2:  x[5:0] = 24;
				    6'd3:  x[5:0] = 29;
				    6'd4:  x[5:0] = 43;
				    6'd5:  x[5:0] = 34;
				    6'd6:  x[5:0] = 11;
				    6'd7:  x[5:0] = 12;
				    6'd8:  x[5:0] = 13;
				    6'd9:  x[5:0] = 36;
				    6'd10: x[5:0] = 37;
				    6'd11: x[5:0] = 38;
				    6'd12: x[5:0] = 41;
				    6'd13: x[5:0] = 39;
				    6'd14: x[5:0] = 40;
				    6'd15: x[5:0] = 43;
				    6'd16: x[5:0] = 43;
				    6'd17: x[5:0] = 43;
				    6'd18: x[5:0] = 43;
				    6'd19: x[5:0] = 43;
				    6'd20: x[5:0] = 43;
				    6'd21: x[5:0] = 43;
				    6'd22: x[5:0] = 43;
				    6'd23: x[5:0] = 43;
				    6'd24: x[5:0] = 43;
				    6'd25: x[5:0] = 43;
				    6'd26: x[5:0] = 43;
				    6'd27: x[5:0] = 43;
				    6'd28: x[5:0] = 43;
				    6'd29: x[5:0] = 43;
				    6'd30: x[5:0] = 43;
				    6'd31: x[5:0] = 43;
				    6'd32: x[5:0] = 43;
				    6'd33: x[5:0] = 43;
				    6'd34: x[5:0] = 43;
				    6'd35: x[5:0] = 43;
				    6'd36: x[5:0] = 43;
				    6'd37: x[5:0] = 43;
				    6'd38: x[5:0] = 43;
				    6'd39: x[5:0] = 43;
				    6'd40: x[5:0] = 43;
				    6'd41: x[5:0] = 43;
				    6'd42: x[5:0] = 43;
				    6'd43: x[5:0] = 43;
				    6'd44: x[5:0] = 43;
					endcase
					x[21:6] = 0 ;
					// x[5:0] = map5 ;
					// os_control_reg_map MAP5(x[5:0], rd) ;
                    // x = os_control_reg_map[rd] ;
                    if (x == `CONTROL_PSTATE ||
                            x == `CONTROL_TL ||
                            x == `CONTROL_CWP ) begin
                        m_type = `DYN_CONTROL;
                        m_futype = `FU_BRANCH;
                        m_branch_type = `BRANCH_PRIV;
                    end
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;  
                    else rs1_type = `RID_INT;
					rs1_valid = 1 ;
                    rs4 = x ;
                    rd2 = x ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    rd2_type = `RID_CONTROL;
                    m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                          // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                          // end
                    end
                    m_opcode = `i_wrpr; //wrpr
                end
            51:    begin
                fail = 1; //FAIL
                end
            52:    begin
                m_futype = `FU_NONE;
                case(opf_hi)
                    0:    begin //fmov
                        case(opf_lo)
                            0:    fail=1;
                            1:    begin //fmovs
                                rd_type = `RID_SINGLE;
                                rd_valid = 1 ;
                                rs2_type = `RID_SINGLE;
								rs2_valid = 1 ;
                                m_opcode = `i_fmovs;
                                end
                            2:    begin //fmovd
                                rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                                rd_type = `RID_DOUBLE;
                                rd_valid = 1 ;
                                rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                                rs2_type = `RID_DOUBLE;
								rs2_valid = 1 ;
                                m_opcode = `i_fmovd;
                                end
                            3:    begin //fmovq
                                m_opcode = `i_fmovq;
                                end
                            4:    fail=1;
                            5:    begin //fnegs
                                rd_type = `RID_SINGLE;
                                rd_valid = 1 ;
                                rs2_type = `RID_SINGLE;
								rs2_valid = 1 ;
                                m_futype = `FU_FLOATADD;
                                m_opcode = `i_fnegs;
                                end
                            6:    begin //fnegd
                                rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                                rd_type = `RID_DOUBLE;
                                rd_valid = 1 ;
                                rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                                rs2_type = `RID_DOUBLE;
                                m_futype = `FU_FLOATADD;
                                m_opcode = `i_fnegd;
                                end
                            7:    begin //fnegq
                                m_futype = `FU_FLOATADD;
                                m_opcode = `i_fnegq;
                                end
                            8:    fail = 1;
                            9:    begin //fabss
                                rd_type = `RID_SINGLE;
                                rd_valid = 1 ;
                                rs2_type = `RID_SINGLE;
								rs2_valid = 1 ;
                                m_futype = `FU_FLOATADD;
                                m_opcode = `i_fabss;
                                end
                            10:    begin //fabsd
                                rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                                rd_type = `RID_DOUBLE;
                                rd_valid = 1 ;
                                rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                                rs2_type = `RID_DOUBLE;
								rs2_valid = 1 ;
                                m_futype = `FU_FLOATADD;
                                m_opcode = `i_fabsd;
                                end
                            11:    begin //fabsq
                                m_futype = `FU_FLOATADD;
                                m_opcode = `i_fabsq;
                                end
                            default: fail = 1;
                        endcase 
                        end
                    2:    begin
                        case(opf_lo)
                            9:    begin //fsqrts
                                m_futype = `FU_FLOATSQRT;
                                rd_type = `RID_SINGLE;
                                rd_valid = 1 ;
                                rs2_type = `RID_SINGLE;
								rs2_valid = 1 ;
                                m_opcode = `i_fsqrts;
                                end
                            10:    begin //fsqrtd
                                m_futype = `FU_FLOATSQRT;
                                rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                                rd_type = `RID_DOUBLE;
                                rd_valid = 1 ;
                                rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                                rs2_type = `RID_DOUBLE;
								rs2_valid = 1 ;
                                m_opcode = `i_fsqrtd;
                                end
                            11:    begin //fsqrtq
                                m_futype = `FU_FLOATSQRT;
                                m_opcode = `i_fsqrtq;
                                end
                            default: fail = 1;
                        endcase 
                        end
                    4:    begin
                        case(opf_lo)
                        1:    begin //fadds
                            m_futype = `FU_FLOATADD;
                            rd_type = `RID_SINGLE;
                            rd_valid = 1 ;
                            rs1_type = `RID_SINGLE;
							rs1_valid = 1 ;
                            rs2_type = `RID_SINGLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fadds;
                            end
                        2:    begin //faddd
                            m_futype = `FU_FLOATADD;
                            rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                            rd_type = `RID_DOUBLE;
                            rd_valid = 1 ;
                            rs1 = (((rs1&6'b000001)<<5) | (rs1&6'b111110)) ;
                            rs1_type = `RID_DOUBLE;
							rs1_valid = 1 ;
                            rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                            rs2_type = `RID_DOUBLE;
							rs2_valid = 1 ;
                            m_opcode = `i_faddd;
                            end
                        3:    begin //faddq
                            m_futype = `FU_FLOATADD;
                            m_opcode = `i_faddq;
                            end
                        5:    begin //fsubs
                            m_futype = `FU_FLOATADD;
                            rd_type = `RID_SINGLE;
                            rd_valid = 1 ;
                            rs1_type = `RID_SINGLE;
							rs1_valid = 1 ;
                            rs2_type = `RID_SINGLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fsubs;
                            end
                        6:    begin //fsubd
                            m_futype = `FU_FLOATADD;
                            rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                            rs1 = (((rs1&6'b000001)<<5) | (rs1&6'b111110)) ;
                            rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                            rd_type = `RID_DOUBLE;
                            rd_valid = 1 ;
                            rs1_type = `RID_DOUBLE;
							rs1_valid = 1 ;
                            rs2_type = `RID_DOUBLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fsubd;
                            end
                        7:    begin //fsubq
                            m_futype = `FU_FLOATADD;
                            m_opcode = `i_fsubq;
                            end
                        9:    begin //fmuls
                            m_futype = `FU_FLOATMULT;
                            rd_type = `RID_SINGLE;
                            rd_valid = 1 ;
                            rs1_type = `RID_SINGLE;
							rs1_valid = 1 ;
                            rs2_type = `RID_SINGLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fmuls;
                            end
                        10:    begin //fmuld
                            m_futype = `FU_FLOATMULT;
                            rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                            rs1 = (((rs1&6'b000001)<<5) | (rs1&6'b111110)) ;
                            rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                            rd_type = `RID_DOUBLE;
                            rd_valid = 1 ;
                            rs1_type = `RID_DOUBLE;
							rs1_valid = 1 ;
                            rs2_type = `RID_DOUBLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fmuld;
                            end
                        11:    begin //fmulq
                            m_futype = `FU_FLOATMULT;
                            m_opcode = `i_fmulq;
                            end
                        13:    begin //fdivs
                            m_futype = `FU_FLOATDIV;
                            rd_type = `RID_SINGLE;
                            rd_valid = 1 ;
                            rs1_type = `RID_SINGLE;
							rs1_valid = 1 ;
                            rs2_type = `RID_SINGLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fdivs;
                            end
                        14:    begin //fdivd
                            m_futype = `FU_FLOATDIV;
                            rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                            rs1 = (((rs1&6'b000001)<<5) | (rs1&6'b111110)) ;
                            rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                            rd_type = `RID_DOUBLE;
                            rd_valid = 1 ;
                            rs1_type = `RID_DOUBLE;
							rs1_valid = 1 ;
                            rs2_type = `RID_DOUBLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fdivd;
                            end
                        15:    begin //fdivq
                            m_futype = `FU_FLOATDIV;
                            m_opcode = `i_fdivq;
                            end
                        default: fail = 1;
                        endcase
                        end
                    6:    begin
                        case(opf_lo)
                        9:    begin //fsmuld
                            m_futype = `FU_FLOATMULT;
                            rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                            rd_type = `RID_DOUBLE;
                            rd_valid = 1 ;
                            rs1_type = `RID_SINGLE;
							rs1_valid = 1 ;
                            rs2_type = `RID_SINGLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fsmuld;
                            end
                        14:    begin //fdmulq
                            m_futype = `FU_FLOATMULT;
                            rs1 = (((rs1&6'b000001)<<5) | (rs1&6'b111110)) ;
                            rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                            rs1_type = `RID_DOUBLE;
							rs1_valid = 1 ;
                            rs2_type = `RID_DOUBLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fdmulq;
                            end
                        default: fail = 1;
                        endcase 
                        end
                    8:    begin
                        case(opf_lo)
                        1:    begin //fstox
                            rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                            rd_type = `RID_DOUBLE;
                            rd_valid = 1 ;
                            rs2_type = `RID_SINGLE;   
							rs2_valid = 1 ;
                            m_opcode = `i_fstox;
                            end
                        2:    begin //fdtox
                            rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                            rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                            rd_type = `RID_DOUBLE;
                            rd_valid = 1 ;
                            rs2_type = `RID_DOUBLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fdtox;
                            end
                        3:    begin //fqtox
                            rs2_type = `RID_SINGLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fqtox;
                            end
                        4:    begin //fxtos
                            rd_type = `RID_SINGLE;
                            rd_valid = 1 ;
                            rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                            rs2_type = `RID_DOUBLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fxtos;
                            end
                        8:    begin //fxtod
                            rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                            rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                            rd_type = `RID_DOUBLE;
                            rd_valid = 1 ;
                            rs2_type = `RID_DOUBLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fxtod;
                            end
                        12: begin //fxtoq
                            rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                            rs2_type = `RID_DOUBLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fxtoq;
                            end
                        default: fail = 1;
                        endcase
                        end
                    12:    begin
                        m_futype = `FU_FLOATCVT;
                        case(opf_lo)
                        9:    begin //fstod
                            rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                            rd_type = `RID_DOUBLE;
                            rd_valid = 1 ;
                            rs2_type = `RID_SINGLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fstod;
                            end
                        13:    begin //fstoq
                            rs2_type = `RID_SINGLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fstoq;
                            end
                        6:    begin //fdtos
                            rd_type = `RID_SINGLE;
                            rd_valid = 1 ;
                            rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                            rs2_type = `RID_DOUBLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fdtos;
                            end
                        14:    begin //fdtoq
                            rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                            rs2_type = `RID_DOUBLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fdtoq;
                            end
                        7:    begin //fqtos
                            rd_type = `RID_SINGLE;
                            rd_valid = 1 ;
                            m_opcode = `i_fqtos;
                            end    
                        11:    begin //fqtod
                            rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                            rd_type = `RID_DOUBLE;
                            rd_valid = 1 ;
                            m_opcode = `i_fqtod;
                            end
                        4:    begin //fitos
                            rd_type = `RID_SINGLE;
                            rd_valid = 1 ;
                            rs2_type = `RID_SINGLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fitos;
                            end
                        8:    begin //fitod
                            rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                            rd_type = `RID_DOUBLE;
                            rd_valid = 1 ;
                            rs2_type = `RID_SINGLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fitod;
                            end
                        12:    begin //fitoq
                            rs2_type = `RID_SINGLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fitoq;
                            end
                        default: fail = 1;
                        endcase
                        end 
                    13:    begin
                        case(opf_lo)
                        1:    begin //fstoi
                            rd_type = `RID_SINGLE;
                            rd_valid = 1 ;
                            rs2_type = `RID_SINGLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fstoi;
                            end
                        2:    begin //fdtoi
                            rd_type = `RID_SINGLE;
                            rd_valid = 1 ;
                            rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                            rs2_type = `RID_DOUBLE;
							rs2_valid = 1 ;
                            m_opcode = `i_fdtoi;
                            end
                        3:    begin //fstoi
                            rd_type = `RID_SINGLE;
                            rd_valid = 1 ;
                            m_opcode = `i_fqtoi;
                            end
                        default: fail = 1;
                        endcase 
                        end
                    default: fail = 1;
                endcase 
                end
            53:    begin
                m_futype = `FU_NONE;
                x = instruction[29:27];
                movcc2 = instruction[13];
                sdq = instruction[7:5];

                if(x==0 && opf_hi ==5) begin
                    m_futype = `FU_FLOATCMP;
                    movcc2 = instruction[26:25];
                    rd_type = `RID_CC ;
                    rd_valid = 1 ;
                    case(movcc2) // SETTING CONTROL REGISTERS
                    0:    rd = `REG_CC_FCC0;
                    1:    rd = `REG_CC_FCC1;
                    2:    rd = `REG_CC_FCC2;
                    3:    rd = `REG_CC_FCC3;
                    default: fail = 1;
                    endcase
                    case(opf_lo)
                    1:    begin //fcmps
                        rs1_type = `RID_SINGLE;
						rs1_valid = 1 ;
                        rs2_type = `RID_SINGLE;
						rs2_valid = 1 ;
                        m_opcode = `i_fcmps;
                        end
                    2:    begin //fcmpd
                        rs1 = (((rs1&6'b000001)<<5) | (rs1&6'b111110)) ;
						rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
						rs1_type = `RID_DOUBLE;
						rs1_valid = 1 ;
						rs2_type = `RID_DOUBLE;
						rs2_valid = 1 ;
                        m_opcode = `i_fcmpd;
                        end
                    3:    begin //fcmpq
                        m_opcode = `i_fcmpq;
                        end
                    5:    begin //fcmpes
                        rs1_type = `RID_SINGLE;
						rs1_valid = 1 ;
                        rs2_type = `RID_SINGLE;
						rs2_valid = 1 ;
                        m_opcode = `i_fcmpes;
                        end
                    6:    begin //fcmped
                        rs1 = (((rs1&6'b000001)<<5) | (rs1&6'b111110)) ;
                        rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                         rs1_type = `RID_DOUBLE;
						 rs1_valid = 1 ;
                        rs2_type = `RID_DOUBLE;
						rs2_valid = 1 ;
                        m_opcode = `i_fcmped;
                        end
                    7:    begin //fcmpeq
                        m_opcode = `i_fcmpeq;
                        end
                    default: fail = 1;
                    endcase 
                end else if(movcc2) begin //integer op
                    case(sdq)
                    1:    begin //single
                        rd_type = `RID_SINGLE;
                        rd_valid = 1 ;
                        rs1_type = `RID_SINGLE;
						rs1_valid = 1 ;
                        rs2_type = `RID_SINGLE;
						rs2_valid = 1 ;
                        rs3 = `REG_CC_CCR ;
                        rs3_type = `RID_CC;
						rs3_valid = 1 ;
                        case(mcond)
                        0:  m_opcode = `i_fmovsn;
                        1:  m_opcode = `i_fmovse;
                        2:  m_opcode = `i_fmovsle;
                        3:  m_opcode = `i_fmovsl;
                        4:  m_opcode = `i_fmovsleu;
                        5:  m_opcode = `i_fmovscs;
                        6:  m_opcode = `i_fmovsneg;
                        7:  m_opcode = `i_fmovsvs;
                        8:  m_opcode = `i_fmovsa;
                        9:  m_opcode = `i_fmovsne;
                        10:  m_opcode = `i_fmovsg;
                        11:  m_opcode = `i_fmovsge;
                        12:  m_opcode = `i_fmovsgu;
                        13:  m_opcode = `i_fmovscc;
                        14:  m_opcode = `i_fmovspos;
                        15:  m_opcode = `i_fmovsvc;
                        default: fail = 1;
                        endcase
                        end
                    2:    begin //double
                        rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                        rs1 = (((rs1&6'b000001)<<5) | (rs1&6'b111110)) ;
                        rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                        rd_type = `RID_DOUBLE;
                        rd_valid = 1 ;
                        rs1_type = `RID_DOUBLE;
						rs1_valid = 1 ;
                        rs2_type = `RID_DOUBLE;
						rs2_valid = 1 ;
                        rs3 = `REG_CC_CCR ;
                        rs3_type = `RID_CC;
						rs3_valid = 1 ;
                        case(mcond) 
                        0:  m_opcode = `i_fmovdn;
                        1:  m_opcode = `i_fmovde;
                        2:  m_opcode = `i_fmovdle;
                        3:  m_opcode = `i_fmovdl;
                        4:  m_opcode = `i_fmovdleu;
                        5:  m_opcode = `i_fmovdcs;
                        6:  m_opcode = `i_fmovdneg;
                        7:  m_opcode = `i_fmovdvs;
                        8:  m_opcode = `i_fmovda;
                        9:  m_opcode = `i_fmovdne;
                        10:  m_opcode = `i_fmovdg;
                        11:  m_opcode = `i_fmovdge;
                        12:  m_opcode = `i_fmovdgu;
                        13:  m_opcode = `i_fmovdcc;
                        14:  m_opcode = `i_fmovdpos;
                        15:  m_opcode = `i_fmovdvc;
                        default: fail = 1;
                        endcase
                    end
                    3:    begin //quad
                        rs3 = `REG_CC_CCR ;
                        rs3_type = `RID_CC;
						rs3_valid = 1 ;
                        case(mcond) 
                        0:  m_opcode = `i_fmovqn;
                        1:  m_opcode = `i_fmovqe;
                        2:  m_opcode = `i_fmovqle;
                        3:  m_opcode = `i_fmovql;
                        4:  m_opcode = `i_fmovqleu;
                        5:  m_opcode = `i_fmovqcs;
                        6:  m_opcode = `i_fmovqneg;
                        7:  m_opcode = `i_fmovqvs;
                        8:  m_opcode = `i_fmovqa;
                        9:  m_opcode = `i_fmovqne;
                        10:  m_opcode = `i_fmovqg;
                        11:  m_opcode = `i_fmovqge;
                        12:  m_opcode = `i_fmovqgu;
                        13:  m_opcode = `i_fmovqcc;
                        14:  m_opcode = `i_fmovqpos;
                        15:  m_opcode = `i_fmovqvc;
                        default: fail = 1;
                        endcase
                    end
                    default: fail = 1;
                    endcase // double single quad
                end else begin
                    movcc2 = instruction[12:11];
                    rs3_type = `RID_CC ;
					rs3_valid = 1 ;
                    case(movcc2)
                    0:    rs3 = `REG_CC_FCC0;
                    1:    rs3 = `REG_CC_FCC1;
                    2:    rs3 = `REG_CC_FCC2;
                    3:    rs3 = `REG_CC_FCC3;
                    default: fail = 1;
                    endcase
                    case(sdq)
                    1:    begin //single
                        rd_type = `RID_SINGLE;
                        rd_valid = 1 ;
                        rs1_type = `RID_SINGLE;
						rs1_valid = 1 ;
                        rs2_type = `RID_SINGLE;
						rs2_valid = 1 ;
                        case(mcond) 
                        0:  m_opcode = `i_fmovfsn;
                        1:  m_opcode = `i_fmovfsne;
                        2:  m_opcode = `i_fmovfslg;
                        3:  m_opcode = `i_fmovfsul;
                        4:  m_opcode = `i_fmovfsl;
                        5:  m_opcode = `i_fmovfsug;
                        6:  m_opcode = `i_fmovfsg;
                        7:  m_opcode = `i_fmovfsu;
                        8:  m_opcode = `i_fmovfsa;
                        9:  m_opcode = `i_fmovfse;
                        10:  m_opcode = `i_fmovfsue;
                        11:  m_opcode = `i_fmovfsge;
                        12:  m_opcode = `i_fmovfsuge;
                        13:  m_opcode = `i_fmovfsle;
                        14:  m_opcode = `i_fmovfsule;
                        15:  m_opcode = `i_fmovfso;
                        default: fail = 1;
                        endcase
                    end
                    2:    begin //double
						rs1 = rd ;
                        rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                        rs1 = (((rs1&6'b000001)<<5) | (rs1&6'b111110)) ;
                        rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                        rd_type = `RID_DOUBLE;
                        rd_valid = 1 ;
                        rs1_type = `RID_DOUBLE;
						rs1_valid = 1 ;
                        rs2_type = `RID_DOUBLE;
						rs2_valid = 1 ;
                        case(mcond) 
                        0:  m_opcode = `i_fmovfdn;
                        1:  m_opcode = `i_fmovfdne;
                        2:  m_opcode = `i_fmovfdlg;
                        3:  m_opcode = `i_fmovfdul;
                        4:  m_opcode = `i_fmovfdl;
                        5:  m_opcode = `i_fmovfdug;
                        6:  m_opcode = `i_fmovfdg;
                        7:  m_opcode = `i_fmovfdu;
                        8:  m_opcode = `i_fmovfda;
                        9:  m_opcode = `i_fmovfde;
                        10:  m_opcode = `i_fmovfdue;
                        11:  m_opcode = `i_fmovfdge;
                        12:  m_opcode = `i_fmovfduge;
                        13:  m_opcode = `i_fmovfdle;
                        14:  m_opcode = `i_fmovfdule;
                        15:  m_opcode = `i_fmovfdo;
                        default: fail = 1;
                        endcase
                    end
                    3:    begin //quad
                        case(mcond) 
                        0:  m_opcode = `i_fmovfqn;
                        1:  m_opcode = `i_fmovfqne;
                        2:  m_opcode = `i_fmovfqlg;
                        3:  m_opcode = `i_fmovfqul;
                        4:  m_opcode = `i_fmovfql;
                        5:  m_opcode = `i_fmovfqug;
                        6:  m_opcode = `i_fmovfqg;
                        7:  m_opcode = `i_fmovfqu;
                        8:  m_opcode = `i_fmovfqa;
                        9:  m_opcode = `i_fmovfqe;
                        10:  m_opcode = `i_fmovfque;
                        11:  m_opcode = `i_fmovfqge;
                        12:  m_opcode = `i_fmovfquge;
                        13:  m_opcode = `i_fmovfqle;
                        14:  m_opcode = `i_fmovfqule;
                        15:  m_opcode = `i_fmovfqo;
                        default: fail = 1;
                        endcase
                    end
                    5:    begin // single based on icc ixx
                        rd_type = `RID_SINGLE;
                        rd_valid = 1 ;
                        rs1_type = `RID_SINGLE;
						rs1_valid = 1 ;
                        rs2_type = `RID_SINGLE;
						rs2_valid = 1 ;
                        rs3 = rs1 ;
                        if(rs1<8) rs3_type = `RID_INT_GLOBAL;
                        else rs3_type = `RID_INT;
						rs3_valid = 1 ;
                        m_flags = m_flags | `SI_COMPLEX_OP;
                        case(rcond) 
                        0:  fail = 1;//FAIL
                        1:  m_opcode = `i_fmovrsz;
                        2:  m_opcode = `i_fmovrslez;
                        3:  m_opcode = `i_fmovrslz;
                        4:  fail = 1;// FAIL
                        5:  m_opcode = `i_fmovrsnz;
                        6:  m_opcode = `i_fmovrsgz;
                        7:  m_opcode = `i_fmovrsgez;
                        default: fail = 1;
                        endcase
                    end
                    6:    begin // double based on icc ixx
                        rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                        rs1 = (((rs1&6'b000001)<<5) | (rs1&6'b111110)) ;
                        rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                        rd_type = `RID_DOUBLE;
                        rd_valid = 1 ;
                        rs1_type = `RID_DOUBLE;
						rs1_valid = 1 ;
                        rs2_type = `RID_DOUBLE;
						rs2_valid = 1 ;
                        rs3 = rs1 ;
                        if(rs1<8) rs3_type = `RID_INT_GLOBAL;
                        else rs3_type = `RID_INT;
						rs3_valid = 1 ;
                        m_flags = m_flags | `SI_COMPLEX_OP;
                        case(rcond) 
                        0:  fail = 1;//FAIL
                        1:  m_opcode = `i_fmovrdz;
                        2:  m_opcode = `i_fmovrdlez;
                        3:  m_opcode = `i_fmovrdlz;
                        4:  fail = 1;// FAIL
                        5:  m_opcode = `i_fmovrdnz;
                        6:  m_opcode = `i_fmovrdgz;
                        7:  m_opcode = `i_fmovrdgez;
                        default: fail = 1;
                        endcase
                    end
                    7:    begin // quad based on icc ixx
                        rs3 = rs1 ;
                        if(rs1<8) rs3_type = `RID_INT_GLOBAL;
                        else rs3_type = `RID_INT;
						rs3_valid = 1 ;
                        m_flags = m_flags | `SI_COMPLEX_OP;
                        case(rcond) 
                        0:  fail = 1;//FAIL
                        1:  m_opcode = `i_fmovrqz;
                        2:  m_opcode = `i_fmovrqlez;
                        3:  m_opcode = `i_fmovrqlz;
                        4:  fail = 1;// FAIL
                        5:  m_opcode = `i_fmovrqnz;
                        6:  m_opcode = `i_fmovrqgz;
                        7:  m_opcode = `i_fmovrqgez;
                        default: fail = 1;
                        endcase
                    end
                    default: fail = 1;
                    endcase // double single quad
                end    
            end
            54:    begin
                m_futype = `FU_NONE;
                case(opf_hi)
                1:    begin
                    case(opf_lo)
                    8:    begin //alignaddr
                        m_opcode = `i_alignaddr;
                        if(rd<8) rd_type = `RID_INT_GLOBAL;
                        else rd_type = `RID_INT;
                        rd_valid = 1 ;
                        if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                        else rs1_type = `RID_INT;
						rs1_valid = 1 ;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
						rs2_valid = 1 ;
						rs4 = `CONTROL_GSR ;
						rd2 = `CONTROL_GSR ;
                        rs4_type = `RID_CONTROL;
						rs4_valid = 1 ;
                        rd2_type = `RID_CONTROL;
						rd2_valid = 1 ;
                        m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP;
                    end
                    9:    begin //bmask
                        m_opcode = `i_bmask;
                        m_futype = `FU_INTALU;
                        if(rd<8) rd_type = `RID_INT_GLOBAL;
                        else rd_type = `RID_INT;
                        rd_valid = 1 ;
                        if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                        else rs1_type = `RID_INT;
						rs1_valid = 1 ;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
						rs2_valid = 1 ;
						rs4 = `CONTROL_GSR ;
						rd2 = `CONTROL_GSR ;
                        rs4_type = `RID_CONTROL;
						rs4_valid = 1 ;
                        rd2_type = `RID_CONTROL;
						rd2_valid = 1 ;
                        m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP;
                    end
                    10:    begin //alignaddrl
                        m_opcode = `i_alignaddrl;
                        if(rd<8) rd_type = `RID_INT_GLOBAL;
                        else rd_type = `RID_INT;
                        rd_valid = 1 ;
                        if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                        else rs1_type = `RID_INT;
						rs1_valid = 1 ;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
						rs2_valid = 1 ;
                        rs2_valid = 1 ;
                        rs4_type = `RID_CONTROL;
						rs4_valid = 1 ;
                        rd2_type = `RID_CONTROL;
						rd2_valid = 1 ;
                        m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP;
                        end
                    default: fail = 1;
                    endcase
                end
                2:    begin 
                    m_futype = `FU_FLOATCMP;
                    case(opf_lo)
                    0:    m_opcode = `i_fcmple16;//fcmple16
                    2:    m_opcode = `i_fcmpne16;//fcmpne16
                    4:    m_opcode = `i_fcmple32;//fcmple32
                    6:    m_opcode = `i_fcmpne32;//fcmpne32
                    8:    m_opcode = `i_fcmpgt16;//fcmpgt16
                    10:    m_opcode = `i_fcmpeq16;//fcmpeq16
                    12:    m_opcode = `i_fcmpgt32;//fcmpgt32
                    14:    m_opcode = `i_fcmpeq32;//fcmpeq32
                    default: fail = 1;
                    endcase
                end
                4:    begin
                    case(opf_lo)
                    8:    begin //faligndata
                        m_opcode = `i_faligndata;
                        rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                        rs1 = (((rs1&6'b000001)<<5) | (rs1&6'b111110)) ;
                        rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                        rd_type = `RID_DOUBLE;
                        rd_valid = 1 ;
                        rs1_type = `RID_DOUBLE;
						rs1_valid = 1 ;
                        rs2_type = `RID_DOUBLE;
						rs2_valid = 1 ;
                        rs4 = `CONTROL_GSR ;
                        rs4_type = `RID_CONTROL;
						rs4_valid = 1 ;
                        m_flags = m_flags | `SI_COMPLEX_OP;
                    end
                    12:    begin 
                        m_opcode = `i_bshuffle;
                        m_futype = `FU_INTALU;
                        rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                        rs1 = (((rs1&6'b000001)<<5) | (rs1&6'b111110)) ;
                        rs2 = (((rs2&6'b000001)<<5) | (rs2&6'b111110)) ;
                        rd_type = `RID_DOUBLE;
						rd_valid = 1 ;
						rs1_type = `RID_DOUBLE;
						rs1_valid = 1 ;
						rs2_type = `RID_DOUBLE;
						rs2_valid = 1 ;
						rs4 = `CONTROL_GSR ;
						rs4_type = `RID_CONTROL;
						rs4_valid = 1 ;
                        m_flags = m_flags | `SI_COMPLEX_OP;
                    end
                    default: fail = 1;
                    endcase
                end
                6:    begin
                    case(opf_lo)
                    0:    begin
                        m_opcode = `i_fzero;//fzero
                        rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                         rd_type = `RID_DOUBLE;
                        rd_valid = 1 ;
                    end
                    1:    begin
                        m_opcode = `i_fzeros;//fzeros
                        rd_type = `RID_SINGLE;
                        rd_valid = 1 ;
                    end
                    default: fail = 1;
                    endcase
                end
                7:    begin
                    case(opf_lo)
                    4:    begin
                        m_opcode = `i_fsrc1;//fsrc1
                        rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
                        rs1 = (((rs1&6'b000001)<<5) | (rs1&6'b111110)) ;
                        rd_type = `RID_DOUBLE;
                        rd_valid = 1 ;
                        rs1_type = `RID_DOUBLE;
						rs1_valid = 1 ;
                    end
                    default: fail = 1;
                    endcase
                end
                default: fail = 1;
                endcase
            end
            55:    begin
                m_opcode = `i_impdep2; //impdep2
                end
            56:    begin
                m_type = `DYN_CONTROL;
                m_futype = `FU_NONE;
                m_branch_type = `BRANCH_INDIRECT;
                if(rd<8) rd_type = `RID_INT_GLOBAL;
                else rd_type = `RID_INT;
                rd_valid = 1 ;
                if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                else rs1_type = `RID_INT;
				rs1_valid = 1 ;
                if(i==0) begin
                    m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
                end else begin
                    m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                    // m_imm = instruction[12:0];//NEEDS SEXT
                      // for(z=13;z<64;z=z+1) begin
                        // m_imm[z] = instruction[12];
                      // end
                end
                rs4 = `CONTROL_PSTATE ;
                rs4_type = `RID_CONTROL ;
				rs4_valid = 1 ;
                if(rd==0 &&
                        (i==1 && (rs1==15 || rs1==31)
                         && (instruction[12:0] == 8))) begin
                    m_opcode = `i_jmpl;//jmpl
                    m_branch_type = `BRANCH_RETURN;
                end else if(rd==15) begin
                    m_branch_type = `BRANCH_CALL;
                    m_opcode = `i_jmpl;
                    end else m_opcode = `i_jmpl;
                m_flags = m_flags | `SI_COMPLEX_OP;
            end
            57:    begin
                case(rd)
                0:    begin
                    m_type = `DYN_CONTROL;
                    m_futype = `FU_BRANCH;
                    m_branch_type = `BRANCH_RETURN;
                    if(rd<8) rd_type =`RID_INT_GLOBAL;
                    else rd_type = `RID_INT;
                    rd_valid = 1 ;
                    if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;
					rs1_valid = 1 ;
                    if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                        // for(z=13;z<64;z=z+1) begin
                            // m_imm[z] = instruction[12];
                        // end
                    end
                    rd_valid = 0;
                    rs4 = `CONTROL_ISREADY ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    m_flags = m_flags | `SI_COMPLEX_OP;
                    m_opcode = `i_retrn;//retrn
                end
                default: fail = 1;
                endcase
            end
            58:    begin
                m_type = `DYN_CONTROL;
                m_futype = `FU_BRANCH;
                m_branch_type = `BRANCH_TRAP;
                if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                else rs1_type = `RID_INT;
				rs1_valid = 1 ;

                if(i==0) begin
                    m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
                end else begin
                    m_flags = m_flags | `SI_ISIMMEDIATE;
                    m_imm = instruction[6:0];
                end
                rs3 = `REG_CC_CCR ;
                rs3_type = `RID_CC ;
				rs3_valid = 1 ;
                m_ccshift = instruction[12:11] << 1;//NEEDS 1LSHFT
                if(m_ccshift != 0 && m_ccshift !=4)
                    fail = 1;
                case(cond)
                0:  m_opcode = `i_tn;
                1:  m_opcode = `i_te;
                2:  m_opcode = `i_tle;
                3:  m_opcode = `i_tl;
                4:  m_opcode = `i_tleu;
                5:  m_opcode = `i_tcs;
                6:  m_opcode = `i_tneg;
                7:  m_opcode = `i_tvs;
                8:  m_opcode = `i_ta;
                9:  m_opcode = `i_tne;
                10:  m_opcode = `i_tg;
                11:  m_opcode = `i_tge;
                12:  m_opcode = `i_tgu;
                13:  m_opcode = `i_tcc;
                14:  m_opcode = `i_tpos;
                15:  m_opcode = `i_tvc;
                default: fail = 1;
                endcase
            end
            59:    begin
                case(rd)
                    0:    begin
                        m_futype = `FU_NONE;
                        if(rs1<8) rs1_type = `RID_INT_GLOBAL;
                        else rs1_type = `RID_INT;
						rs1_valid = 1 ;

                        if(i==0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;

                        end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                        // m_imm = instruction[12:0];//NEEDS SEXT
                        // for(z=13;z<64;z=z+1) begin
                        // m_imm[z] = instruction[12];
                        // end
                        end
                        m_opcode = `i_flush;//flush
                    end
                    default: fail = 1;
                endcase    
                end
            60:    begin
                m_type = `DYN_CONTROL;
                m_futype = `FU_BRANCH;
                m_futype = `FU_INTALU;
                if(rd<8) rd_type = `RID_INT_GLOBAL;
                else rd_type = `RID_INT;
                rd_valid = 1 ;
                if(rs1<8) rs1_type = `RID_INT_GLOBAL;
				else rs1_type = `RID_INT;
				rs1_valid = 1 ;
                if(i==0) begin
                    m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
                end else begin
                    m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                    // m_imm = instruction[12:0];//NEEDS SEXT
                      // for(z=13;z<64;z=z+1) begin
                        // m_imm[z] = instruction[12];
                      // end
                end                    
                m_branch_type = `BRANCH_CWP;
                m_opcode = `i_save;                    
                rs4 = `CONTROL_PSTATE ;
                rd2 = `CONTROL_PSTATE ;
                rs4_type = `RID_CONTROL;
				rs4_valid = 1 ;
                rd2_type = `RID_CONTROL;
				rd2_valid = 1 ;
                m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP | `SI_UPDATE_CWP;
            end
            61:    begin
                m_type = `DYN_CONTROL;
                m_futype = `FU_BRANCH;
                m_futype = `FU_INTALU;
                if(rd<8) rd_type = `RID_INT_GLOBAL;
                else rd_type = `RID_INT;
                rd_valid = 1 ;
                if(rs1<8) rs1_type = `RID_INT_GLOBAL;
				else rs1_type = `RID_INT;
				rs1_valid = 1 ;
                if(i==0) begin
                    m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
                end else begin
                    m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                    // m_imm = instruction[12:0];//NEEDS SEXT
                      // for(z=13;z<64;z=z+1) begin
                        // m_imm[z] = instruction[12];
                      // end
                end                    m_branch_type = `BRANCH_CWP;
                m_opcode = `i_restore; //restore
                rs4 = `CONTROL_PSTATE ;
                rd2 = `CONTROL_PSTATE ;
                rs4_type = `RID_CONTROL;
				rs4_valid = 1 ;
                rd2_type = `RID_CONTROL;
				rd2_valid = 1 ;
                m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP | `SI_UPDATE_CWP;
            end
            62:    begin
                m_futype = `FU_NONE;
                case(rd)
                0:    begin
                    m_type = `DYN_CONTROL;
                    m_futype = `FU_BRANCH;
                    rd = `REG_CC_CCR ;
                    rs4 = `CONTROL_PSTATE ;
                    rd2 = `CONTROL_PSTATE ;
                    rd_type = `RID_CC;
                    rd_valid = 1 ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    rd2_type = `RID_CONTROL;
					rd2_valid = 1 ;
                    m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP;
                    m_branch_type = `BRANCH_TRAP_RETURN;
                    m_opcode = `i_done;//done
                end
                1:    begin
                    m_type = `DYN_CONTROL;
                    m_futype = `FU_BRANCH;
                    rd = `REG_CC_CCR ;
                    rs4 = `CONTROL_PSTATE ;
                    rd2 = `CONTROL_PSTATE ;
                    rd_type = `RID_CC;
                    rd_valid = 1 ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    rd2_type = `RID_CONTROL;
					rd2_valid = 1 ;
                    m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP;
                    m_branch_type = `BRANCH_TRAP_RETURN;
                    m_opcode = `i_retry;//retry
                end
                default: fail = 1;
                endcase
            end
            63:    fail = 1;
            default: fail = 1;//  
            endcase
        end
        //BREAK OUT OF OP2 INSTRUCTIONS

    3:    begin
        case(op3)
        0:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_lduw;// lduw
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
            if (i == 0) begin
              m_flags = m_flags & ~`SI_ISIMMEDIATE;
              if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
              else rs2_type = `RID_INT;
              rs2_valid = 1 ;
            end else begin
              m_flags = m_flags | `SI_ISIMMEDIATE;
			  m_imm = {{51{instruction[12]}},instruction[12:0]};
              // m_imm = instruction[12:0];//NEEDS SEXT;
                // for(z=13;z<64;z=z+1) begin
                  // m_imm[z] = instruction[12];
                // end
            end
            m_access_size = `REG_WORD;
        end
        1:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_ldub;// ldub
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
                  if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[10:0];//NEEDS SEXT;
                  // for(z=11;z<64;z=z+1) begin
                    // m_imm[z] = instruction[10];
                  // end
              end
            m_access_size = `REG_BYTE;
        end
        2:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_lduh;// lduh
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
                  if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[10:0];//NEEDS SEXT;
                  // for(z=11;z<64;z=z+1) begin
                    // m_imm[z] = instruction[10];
                  // end

              end
                   m_access_size = `REG_HALF;
            end
        3:     begin // ldd
            if(rd&6'b000001) fail = 1;
                else begin 
                m_type = `DYN_LOAD;
                m_opcode = `i_ldd;
                m_futype = `FU_RDPORT;
                if(rd < 8) rd_type = `RID_INT_GLOBAL;
                else rd_type = `RID_INT;   
                rd_valid = 1 ;
                if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
                else rs1_type = `RID_INT;    
				rs1_valid = 1 ;
                  if (i == 0) begin
                    m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
                end else begin
                    m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{53{instruction[10]}},instruction[10:0]};
                    // m_imm = instruction[10:0];//NEEDS SEXT;
                      // for(z=11;z<64;z=z+1) begin
                        // m_imm[z] = instruction[10];
                      // end
                  end
                rd2 = rd+1 ;
                if(rd+1<8) rd2_type = `RID_INT_GLOBAL;
                else rd2_type = `RID_INT;
				rd2_valid = 1 ;
                m_access_size = `REG_DOUBLE;
                m_flags = m_flags | `SI_COMPLEX_OP;
            end
        end
        4:    begin 
            m_type = `DYN_STORE;
            m_opcode = `i_stw;// stw
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
            m_access_size = `REG_WORD;
        end
        5:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_stb;// stb
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
            m_access_size = `REG_BYTE;
        end
        6:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_sth;// sth
            m_futype = `FU_WRPORT;
			rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
            m_access_size = `REG_HALF;
        end
        7:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_std;// std
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                // for(z=13;z<64;z=z+1) begin
                //     m_imm[z] = instruction[12];
                // end
            end
            m_access_size = `REG_DOUBLE;
        end
        8:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_ldsw;// ldsw
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
                  if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{53{instruction[10]}},instruction[10:0]};
                // m_imm = instruction[10:0];//NEEDS SEXT;
                  // for(z=11;z<64;z=z+1) begin
                    // m_imm[z] = instruction[10];
                  // end
              end
                    m_access_size = `REG_WORD | `REG_SIGNED;
        end
        9:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_ldsb;// ldsb
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
                  if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
				// m_imm = instruction[10:0];//NEEDS SEXT;
				// for(z=11;z<64;z=z+1) begin
				// m_imm[z] = instruction[10];
				// end
              end
            m_access_size = `REG_BYTE|`REG_SIGNED;
        end
        10:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_ldsh;// ldsh
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
                  if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{53{instruction[10]}},instruction[10:0]};
                // m_imm = instruction[10:0];//NEEDS SEXT;
                  // for(z=11;z<64;z=z+1) begin
                    // m_imm[z] = instruction[10];
                  // end
              end
            m_access_size = `REG_HALF|`REG_SIGNED;
        end
        11:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_ldx;// ldx
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
                  if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[10:0];//NEEDS SEXT;
                  // for(z=11;z<64;z=z+1) begin
                    // m_imm[z] = instruction[10];
                  // end
              end
            m_access_size = `REG_DOUBLE;
        end
        12:    fail = 1;//FAIL
        13:    begin
            m_type = `DYN_ATOMIC;
            m_opcode = `i_ldstub;// ldstub
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
            if(rd<8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;
            rd_valid = 1 ;

            m_access_size = `REG_BYTE;
            m_flags = m_flags | `SI_COMPLEX_OP;
            end
        14:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_stx;// stx
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
            m_access_size = `REG_DOUBLE;
        end
        15:    begin
            m_type = `DYN_ATOMIC;
            m_opcode = `i_swap;// swap
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
            if(rd<8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;
            rd_valid = 1 ;
            m_access_size = `REG_WORD;
            m_flags = m_flags | `SI_COMPLEX_OP;
        end
        16:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_lduwa;// lduwa
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
                  if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{53{instruction[10]}},instruction[10:0]};
                // m_imm = instruction[10:0];//NEEDS SEXT;
                  // for(z=11;z<64;z=z+1) begin
                    // m_imm[z] = instruction[10];
                  // end

              end
            m_access_size = `REG_WORD;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP;
            m_flags = m_flags | `SI_ISASI;
        end
        17:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_lduba;// lduba
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
                  if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{53{instruction[10]}},instruction[10:0]};
                // m_imm = instruction[10:0];//NEEDS SEXT;
                  // for(z=11;z<64;z=z+1) begin
                    // m_imm[z] = instruction[10];
                  // end
              end
            m_access_size = `REG_BYTE;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI;
            end
        18:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_lduha;// lduha
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
                  if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{53{instruction[10]}},instruction[10:0]};
                // m_imm = instruction[10:0];//NEEDS SEXT;
                  // for(z=11;z<64;z=z+1) begin
                    // m_imm[z] = instruction[10];
                  // end

              end
            m_access_size = `REG_HALF;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI;
            end
        19:    begin
                if(rd&6'b000001) fail = 1;
                else begin
                    m_type = `DYN_LOAD;
                    m_futype = `FU_RDPORT;
                    if(rd < 8) rd_type = `RID_INT_GLOBAL;
                    else rd_type = `RID_INT;   
                    rd_valid = 1 ;
                    if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
                    else rs1_type = `RID_INT;    
					rs1_valid = 1 ;
                      if (i == 0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{53{instruction[10]}},instruction[10:0]};
                        // m_imm = instruction[10:0];//NEEDS SEXT;
                          // for(z=11;z<64;z=z+1) begin
                            // m_imm[z] = instruction[10];
                          // end
                    end
					rd2 = rd +1;
                    if(rd2<8) rd2_type = `RID_INT_GLOBAL;
                    else rd2_type = `RID_INT;
					rd2_valid = 1 ;

                    rs4 = `CONTROL_ASI ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI;
                    if(i==0 && (asi==8'h24 || asi==8'h2c)) begin
                        m_access_size = `REG_QUAD ;
                        m_opcode = `i_ldqa;
                    end else begin
                        m_access_size = `REG_DOUBLE;
                        m_opcode = `i_ldda;
                    end
                end
            end
        20:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_stwa; // stwa
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
                    m_access_size = `REG_WORD;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI | `SI_FETCH_BARRIER ;
            end
        21:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_stba; // stba
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
                    m_access_size = `REG_BYTE;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI | `SI_FETCH_BARRIER;
            end
        22:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_stha;// stha
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
            m_access_size = `REG_HALF;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI | `SI_FETCH_BARRIER;
            end
        23:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_stda;// stda
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
            m_access_size = `REG_DOUBLE;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI | `SI_FETCH_BARRIER;
            end
        24:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_ldswa;// ldswa
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
                  if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{53{instruction[10]}},instruction[10:0]};
                // m_imm = instruction[10:0];//NEEDS SEXT;
                  // for(z=11;z<64;z=z+1) begin
                    // m_imm[z] = instruction[10];
                  // end

              end
            m_access_size = `REG_WORD|`REG_SIGNED;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI;
            end
        25:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_ldsba;// ldsba
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
                  if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{53{instruction[10]}},instruction[10:0]};
                // m_imm = instruction[10:0];//NEEDS SEXT;
                  // for(z=11;z<64;z=z+1) begin
                    // m_imm[z] = instruction[10];
                  // end

              end
            m_access_size = `REG_BYTE|`REG_SIGNED;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI;
            end
        26:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_ldsha;// ldsha
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
              if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{53{instruction[10]}},instruction[10:0]};
                // m_imm = instruction[10:0];//NEEDS SEXT;
                  // for(z=11;z<64;z=z+1) begin
                    // m_imm[z] = instruction[10];
                  // end
            end
            m_access_size = `REG_HALF | `REG_SIGNED;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI;
        end
        27:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_ldxa;// ldxa
            m_futype = `FU_RDPORT;
            if(rd < 8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;   
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;    
			rs1_valid = 1 ;
                  if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[10:0];//NEEDS SEXT;
                  // for(z=11;z<64;z=z+1) begin
                    // m_imm[z] = instruction[10];
                  // end
              end
            m_access_size = `REG_DOUBLE;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI;
            end
        28:    fail = 1;
        29:    begin
            m_type = `DYN_ATOMIC;
            m_opcode = `i_ldstuba;//ldstuba
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
            if(rd<8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;
            rd_valid = 1 ;

            m_access_size = `REG_BYTE;
			rs4 = `CONTROL_ASI ;
			rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI;
            end
        30:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_stxa;// stxa
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
                    m_access_size = `REG_DOUBLE;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP;
            m_flags = m_flags | `SI_ISASI;
            m_flags = m_flags | `SI_FETCH_BARRIER;
            if (i == 0 || ( (i == 1) && (m_imm == 64'h54 || m_imm == 64'h5c) )) begin
                rs4 = `CONTROL_PSTATE ;
                rd2 = `CONTROL_PSTATE ;
                rs4_type = `RID_CONTROL;
				rs4_valid = 1 ;
                rd2_type = `RID_CONTROL;
				rd2_valid = 1 ;
                m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP;
            end
        end
        31:    begin
            m_type = `DYN_ATOMIC;
            m_opcode = `i_swapa;// swapa
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
            if(rd<8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;
            rd_valid = 1 ;

            m_access_size = `REG_WORD;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI | `SI_FETCH_BARRIER | `SI_COMPLEX_OP;
            end
        32:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_ldf;// ldf
            m_futype = `FU_RDPORT;
            rd_type = `RID_SINGLE;
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i==0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
                   m_access_size = `REG_WORD;
            end
        33:    begin // ldfsr
            m_type = `DYN_LOAD;
            if(rd==0) begin
                m_opcode = `i_ldfsr;
                m_futype = `FU_RDPORT;
                if(rd < 8) rd_type = `RID_INT_GLOBAL;
                else rd_type = `RID_INT;   
                rd_valid = 1 ;
                if(rd < 8) rs1_type = `RID_INT_GLOBAL;
                else rs1_type = `RID_INT;    
				rs1_valid = 1 ;
                  if (i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
                end else begin
                    m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{53{instruction[10]}},instruction[10:0]};
                    // m_imm = instruction[10:0];//NEEDS SEXT;
                      // for(z=11;z<64;z=z+1) begin
                        // m_imm[z] = instruction[10];
                      // end
                  end
                m_access_size = `REG_WORD;
                rd_valid = 0;
                rs4 = `CONTROL_FSR ;
                rd2 = `CONTROL_FSR ;
                rs4_type = `RID_CONTROL;
				rs4_valid = 1 ;
                rd2_type = `RID_CONTROL;
				rd2_valid = 1 ;
                m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP;
            end else if(rd==1) begin
                m_opcode = `i_ldxfsr;
                m_futype = `FU_RDPORT;
                if(rd < 8) rd_type = `RID_INT_GLOBAL;
                else rd_type = `RID_INT;   
                rd_valid = 1 ;
                if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
                else rs1_type = `RID_INT;    
				rs1_valid = 1 ;
                      if (i == 0) begin
                        m_flags = m_flags & ~`SI_ISIMMEDIATE;
                        if(rs2 < 8 ) rs2_type = `RID_INT_GLOBAL;
                        else rs2_type = `RID_INT;
                        rs2_valid = 1 ;
                    end else begin
                        m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{53{instruction[10]}},instruction[10:0]};
                        // m_imm = instruction[10:0];//NEEDS SEXT;
                          // for(z=11;z<64;z=z+1) begin
                            // m_imm[z] = instruction[10];
                          // end
                      end
                    m_access_size = `REG_DOUBLE;
                    rd_valid = 0;
                    rs4 = `CONTROL_FSR ;
                    rd2 = `CONTROL_FSR ;
                    rs4_type = `RID_CONTROL;
					rs4_valid = 1 ;
                    rd2_type = `RID_CONTROL;
					rd2_valid = 1 ;
                    m_flags = m_flags | `SI_WRITE_CONTROL | `SI_COMPLEX_OP;
                end else fail = 1;
            end
        34:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_ldqf;// ldqf
            m_access_size = `REG_QUAD;
            end
        35:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_lddf;// lddf
            m_futype = `FU_RDPORT;
            rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
            rd_type = `RID_DOUBLE;
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i==0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
                    m_access_size = `REG_DOUBLE;
            end
        36:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_stf; // stf
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            rs3_type = `RID_SINGLE;
			rs3_valid = 1 ;
            if(rs1<8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i==0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
            end
                    m_access_size = `REG_WORD;
            end
        37:    begin // stfsr
            if(rd==0) begin
                m_type = `DYN_STORE;
                m_opcode = `i_stfsr;
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            rs3_type = `RID_SINGLE;
			rs3_valid = 1 ;
            if(rs1<8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i==0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
            end
                        m_access_size = `REG_WORD;
            end else if(rd==1) begin
                m_type = `DYN_STORE;
                m_opcode = `i_stxfsr;
            m_futype = `FU_WRPORT;
            rs3 = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
            rs3_type = `RID_DOUBLE;
			rs3_valid = 1 ;
            if(rs1<8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i==0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
            end
                        m_access_size = `REG_DOUBLE;
            end else fail = 1;
            end
        38:    begin 
            m_type = `DYN_STORE;
            m_opcode = `i_stqf;// stqf
            m_futype = `FU_WRPORT;
            rs3 = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
            rs3_type = `RID_DOUBLE;
			rs3_valid = 1 ;
            if(rs1<8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i==0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
            end
                    m_access_size = `REG_QUAD;
            end
        39:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_stdf;
            m_futype = `FU_WRPORT;
            rs3 = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
            rs3_type = `RID_DOUBLE;
			rs3_valid = 1 ;
            if(rs1<8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i==0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
            end
                    m_access_size = `REG_DOUBLE;
            end
        40:    fail = 1;
        41:    fail = 1;
        42:    fail = 1;
        43:    fail = 1;
        44:    fail = 1;
        45:    begin
                m_type = `DYN_PREFETCH;
                m_futype = `FU_RDPORT;
                m_opcode = `i_prefetch;//prefetch
                m_futype = `FU_WRPORT;
                rs3 = rd ;
                if(rd < 8) rs3_type = `RID_INT_GLOBAL;
                else rs3_type = `RID_INT;
				rs3_valid = 1 ;
                if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
                else rs1_type = `RID_INT;
				rs1_valid = 1 ;
                if(i == 0) begin
                    m_flags = m_flags & ~`SI_ISIMMEDIATE;
                    if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                    else rs2_type = `RID_INT;
                    rs2_valid = 1 ;
                end else begin
                    m_flags = m_flags | `SI_ISIMMEDIATE;
					m_imm = {{51{instruction[12]}},instruction[12:0]};
                    // m_imm = instruction[12:0];//NEEDS SEXT;
                      // for(z=13;z<64;z=z+1) begin
                        // m_imm[z] = instruction[12];
                      // end
                end
                rs3_valid = 0;
                m_access_size = `REG_WORD;
            end
        46:    fail = 1;
        47:    fail = 1;
        48:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_ldfa;//ldfa
            m_futype = `FU_RDPORT;
            rd_type = `RID_SINGLE;
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i==0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
            m_access_size = `REG_WORD;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP| `SI_ISASI;
            end
        49:    fail = 1;
        50:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_ldqfa;//ldqfa
            m_access_size = `REG_QUAD;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI;
            end
        51:    begin
            m_type = `DYN_LOAD;
            m_opcode = `i_lddfa;//lddfa
            m_futype = `FU_RDPORT;
            rd = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
            rd_type = `RID_DOUBLE;
            rd_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i==0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
						m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
			rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP| `SI_ISASI;
            rd_type = `RID_CONTAINER;
            rd_valid = 1 ;
            rs3 = rd ;
            rs3_type = `RID_CONTAINER;
			rs3_valid = 1 ;
            m_access_size = `REG_BLOCK;
            // m_flags = m_flags | `SI_COMPLEX_OP;
            end
        52:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_stfa;//stfa
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            rs3_type = `RID_SINGLE;
			rs3_valid = 1 ;
            if(rs1<8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i==0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
            end
                        m_access_size = `REG_WORD;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP| `SI_ISASI| `SI_FETCH_BARRIER;
            end
        53:    fail = 1;
        54:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_stqfa;//stqfa
            m_futype = `FU_WRPORT;
            rs3 = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
            rs3_type = `RID_DOUBLE;
			rs3_valid = 1 ;
            if(rs1<8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i==0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
            end
            m_access_size = `REG_QUAD;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP| `SI_ISASI| `SI_FETCH_BARRIER;
            end
        55:    begin
            m_type = `DYN_STORE;
            m_opcode = `i_stdfa;//stdfa
            m_futype = `FU_WRPORT;
            rs3 = (((rd&6'b000001)<<5) | (rd&6'b111110)) ;
            if(rs1<8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i==0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2<8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
            end
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP| `SI_ISASI| `SI_FETCH_BARRIER;
            rs3_type = `RID_CONTAINER;
			rs3_valid = 1 ;
            m_access_size = `REG_BLOCK;
            end
        56:    fail = 1;
        57:    fail = 1;
        58:    fail = 1;
        59:    fail = 1;
        60:    begin
            m_type = `DYN_ATOMIC;
            m_futype = `FU_WRPORT;
            m_opcode = `i_casa;//casa
            rs3 = rd ;
            if(rd<8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;
            rd_valid = 1 ;
            if(rs1<8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(rs2<8) rs2_type = `RID_INT_GLOBAL;
            else rs2_type = `RID_INT;
            rs2_valid = 1 ;
            if(rd<8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            m_access_size = `REG_WORD;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP| `SI_ISASI| `SI_FETCH_BARRIER| `SI_ISIMMEDIATE;
            m_imm = 0;
            end
        61:    begin
            m_type = `DYN_PREFETCH;
            m_futype = `FU_RDPORT;
            m_opcode = `i_prefetcha;//prefetcha
            m_futype = `FU_WRPORT;
            rs3 = rd ;
            if(rd < 8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            if(rs1 < 8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(i == 0) begin
                m_flags = m_flags & ~`SI_ISIMMEDIATE;
                if(rs2 < 8) rs2_type = `RID_INT_GLOBAL;
                else rs2_type = `RID_INT;
                rs2_valid = 1 ;
            end else begin
                m_flags = m_flags | `SI_ISIMMEDIATE;
				m_imm = {{51{instruction[12]}},instruction[12:0]};
                // m_imm = instruction[12:0];//NEEDS SEXT;
                  // for(z=13;z<64;z=z+1) begin
                    // m_imm[z] = instruction[12];
                  // end
            end
            rs3_valid = 0;
            m_access_size = `REG_WORD;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI;
            end
        62:    begin
            m_type = `DYN_ATOMIC;
            m_futype = `FU_WRPORT;
            m_opcode = `i_casxa;//casxa
            m_imm = 0;
            rs3 = rd ;
            if(rd<8) rd_type = `RID_INT_GLOBAL;
            else rd_type = `RID_INT;
            rd_valid = 1 ;
            if(rs1<8) rs1_type = `RID_INT_GLOBAL;
            else rs1_type = `RID_INT;
			rs1_valid = 1 ;
            if(rs2<8) rs2_type = `RID_INT_GLOBAL;
            else rs2_type = `RID_INT;
            rs2_valid = 1 ;
            if(rd<8) rs3_type = `RID_INT_GLOBAL;
            else rs3_type = `RID_INT;
			rs3_valid = 1 ;
            m_access_size = `REG_DOUBLE;
            rs4 = `CONTROL_ASI ;
            rs4_type = `RID_CONTROL;
			rs4_valid = 1 ;
            m_flags = m_flags | `SI_COMPLEX_OP | `SI_ISASI| `SI_FETCH_BARRIER| `SI_ISIMMEDIATE;
            end
        63:    begin
            m_type = `DYN_EXECUTE;
            m_futype = `FU_NONE;
            if(instruction[0]==0) m_opcode = `i_mop;
            end
        default: fail = 1;
        endcase
        end

    default: fail=1;
endcase
// end
end 

endmodule //decode_unit
