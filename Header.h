#pragma once

#ifdef MATHLIBRARY_EXPORTS
#define MATHLIBRARY_API __declspec(dllexport)
#else
#define MATHLIBRARY_API __declspec(dllimport)
#endif

#ifndef TC358768SETUP_H
#define TC358768SETUP_H

#include <vector>



//  data type alias
#ifndef u32
#define u32 unsigned long
#endif

#ifndef u16
#define u16 unsigned short
#endif

#ifndef u8
#define u8  unsigned char
#endif

//  constraint define
#define V_ACTIVE_MAX    9999
#define V_ACTIVE_MIN    16
#define V_FP_MAX        1023
#define V_FP_MIN        2
#define V_BP_MAX        1023
#define V_BP_MIN        2
#define V_SW_MAX        1023
#define V_SW_MIN        2

#define H_ACTIVE_MAX    9999
#define H_ACTIVE_MIN    16
#define H_FP_MAX        1023
#define H_FP_MIN        2
#define H_BP_MAX        1023
#define H_BP_MIN        2
#define H_SW_MAX        1023
#define H_SW_MIN        2

#define PCLK_MAX        166.0
#define PCLK_MIN        6.0
#define LANE_SPEED_MAX  1000.0
#define LANE_SPEED_MIN  65.0
#define REFCLK_MAX      40
#define REFCLK_MIN      6

#define NUM_LANES_MAX   4
#define NUM_LANES_MIN   1

//#ifndef LONG_ID_ARRAY
//    #define LONG_ID_ARRAY
//    u16 LONG_ID[]={0x29,0x39};
//#endif

extern "C" MATHLIBRARY_API class addrdata {
public:
    u16 addr, data;
    void GetBytes(char* c);
};

extern "C" MATHLIBRARY_API class packet {
public:
    u16     type_id;
    bool    order;          // 0: before HS mode; 1: after HS mode
    u16     delay;
    std::vector<u8> data;
    packet();
    //    ~packet();
    packet(u16 tid, bool odr);
    //    packet(u8 tid, bool odr);
    packet(u16 tid, bool odr, u8* d, u16 cnt);
    //    packet(u8 tid, bool odr, u8 *d, u16 cnt);
    void SetTID(u16 tid, bool odr);
    //    void SetTID(u8 tid, bool odr);
    void SetPacket(u16 tid, bool odr, u8* d, u16 cnt);
    //    void SetPacket(u8 tid, bool odr, u8 *d, u16 cnt);
    void Reset();

    void SetData(u8* d, u16 cnt);
    void SetDelay(u16 Delay);
private:
    //   std::vector<u16> LONG_ID;
    void Set_LONG_ID(void);
};

extern "C" MATHLIBRARY_API class TC358768Setup {
public:
    std::vector<addrdata>   codelist;
    std::vector<packet>     plist;

    TC358768Setup();
    //    ~TC358768Setup();
        //  panel
    void SetPanelInfo(u32 H, u32 HFP, u32 HBP, u32 HSW, u32 V, u32 VFP, u32 VBP, u32 VSW, float PCLK);
    void SetVHActive(bool p);
    void SetPixelFormat(u32 PixSize, u32 PixFormat);     //max 24
    //  MIPI DSI
    void SetMIPI(u32 NumDataLane, float LaneSpeed);
    void SetTxMode(u32  TxMode);      //  0:pulse  1:event
    //  **reserved using**
    void SetClkSorceMHz(float RefClk, u32 SrcSel);    // 1: pclk, 0: refclk

    //  packets process
    //  add a packet
    void AddPacket(packet pk);
    void EraseAllPacket();
    void RearrangePackets();        //  distinguish LP or HS
    void TransPacketToCodelist(packet pk);
    unsigned short TransVCOMPacketToCodelist(packet pk, u32 VCOM_byte_loc);
    //    void

        //  generate config (must call them as following order)
    void GenSoftwareReset();
    unsigned int GenPLLClkSetting();
    unsigned int GenPhySetting();
    unsigned int GenPPISetting();
    unsigned int GenTxTiming();
    void GenLPPackets();
    void GenSwitchToHS();
    void GenHSPackets();
    void GenEndOfCode();

    //  generate all (support for lazy guy)
    unsigned int GenAll();

    //  quick test used functions
    void cpt_OTM1283A_packets();
    void cpt_OTM1283A_packets_back1();
    void cpt_OTM1283A_packets_back2();

    //private:
    u32 h, hfp, hsw, hbp;
    u32 v, vfp, vsw, vbp;
    float hfp_cns;
    u32 pixelsize, pixformat, numdatalane, srcclk;
    u32 txmode;
    bool    polar;
    double  hfclk, pclk, hsbyteclk, desirelaneclk, reallaneclk, refclk;
    u32     fbd, prd, frs;
};

#endif // TC358768SETUP_H
