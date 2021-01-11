// MathLibrary.cpp : Defines the exported functions for the DLL.
#include "pch.h"// use stdafx.h in Visual Studio 2017 and earlier
#include "Header.h"
#include <math.h>
//#include <QList>

static std::vector<u16> LONG_ID;

void addrdata::GetBytes(char* c) {
    c[0] = addr & 0x0FF;
    c[1] = (addr & 0xff00) >> 8;
    c[2] = data & 0x0FF;
    c[3] = (data & 0xff00) >> 8;
}

packet::packet() {
    this->SetTID(0, 0);
    this->delay = 0;
}

packet::packet(u16 tid, bool odr) {

    this->SetTID(tid, odr);
    this->delay = 0;
}

packet::packet(u16 tid, bool odr, u8* d, u16 cnt) {
    this->SetTID(tid, odr);
    this->delay = 0;
    this->SetData(d, cnt);
}

void packet::SetData(u8* d, u16 cnt) {
    u32 i;
    //  erase all if not empty
    if (!this->data.empty())
        this->data.clear();
    //  push new data content
    for (i = 0; i < cnt; i++) {
        this->data.push_back(d[i]);
    }
}

void packet::SetTID(u16 tid, bool odr) {
    u32 i, isLong;

    tid &= 0x00FF;
    //  check if long id array is empty
    if (tid) {
        //  normal mipi packet
        isLong = 0;
        if (LONG_ID.empty())    //  do initialize
            this->Set_LONG_ID();
        for (i = 0;i < LONG_ID.size();i++) {
            if (tid == LONG_ID.at(i)) {
                this->type_id = tid | 0x4000;
                isLong = 1;
                break;
            }
        }
        if (!isLong) {
            this->type_id = tid | 0x1000;
        }
    }
    else {
        //  delay
        this->type_id = 0;
    }
    this->order = odr;
}

void packet::SetPacket(u16 tid, bool odr, u8* d, u16 cnt) {
    SetTID(tid, odr);
    SetData(d, cnt);
}

void packet::SetDelay(u16 Delay) {
    this->delay = Delay;
}

void packet::Set_LONG_ID() {
    LONG_ID.push_back(0x29);
    LONG_ID.push_back(0x39);
    //  add more
}

void packet::Reset() {
    this->data.clear();
    this->SetTID(0, 0);
    this->delay = 0;
}

TC358768Setup::TC358768Setup()
{
    //    //  default parameter
    //    packet pp(0x4039,0);

    //    packet pp3(0x4019,0);
    //    u8      dd[]={0x11,0x22,0x33,0x44,0x55,0x66,0x77,0x88,0x99};
    refclk = 25.0;
    srcclk = 0;
    txmode = 1;
    polar = 0;      // active low
//    pclk = 26.2;
//    numdatalane = 2;
//    v = 800;
//    vbp = 15;
//    vsw = 10;
//    vfp = 12;
//    h = 480;
//    hbp = 14;
//    hsw = 6;
//    hfp = 21;
    pclk = 61.4;
    numdatalane = 4;
    desirelaneclk = 400.0;
    v = 1280;
    vbp = 14;
    vsw = 2;
    vfp = 16;
    h = 720;
    hbp = 34;
    hsw = 2;
    hfp = 24;

    pixelsize = 24;
    pixformat = 0x3E;
    //    pp.type_id = 0x4039;
    //    pp.order = 0;
    //    pp.SetData(dd,9);
    //    dd = {0x99,0x88,0x77,0x66,0x55,0x44,0x33};
    //    packet pp2(0x4029,1,dd,7);
    //    this->plist.push_back(pp);
    //    this->plist.push_back(pp2);
    //    this->plist.push_back(pp3);
}

void TC358768Setup::GenSoftwareReset() {
    addrdata ad;

    ad.addr = 0x0002;
    ad.data = 0x0001;
    codelist.push_back(ad);
    ad.addr = 0xFFFF;
    ad.data = 0x0001;
    codelist.push_back(ad);
    ad.addr = 0x0002;
    ad.data = 0x0000;
    codelist.push_back(ad);
}

void TC358768Setup::GenEndOfCode() {
    addrdata ad;

    ad.addr = 0xFFF0;
    ad.data = 0x0000;
    codelist.push_back(ad);
}

//  $$$$ WARNING $$$$
//  this function will not check errors
//  it maybe not work if there are errors within generating
unsigned int TC358768Setup::GenAll() {
    this->GenSoftwareReset();
    this->GenPLLClkSetting();
    this->GenPhySetting();
    this->GenPPISetting();
    this->GenTxTiming();
    this->RearrangePackets();
    this->GenLPPackets();
    this->GenSwitchToHS();
    this->GenEndOfCode();
    return 0;
}

void TC358768Setup::SetPanelInfo(u32 H, u32 HFP, u32 HBP, u32 HSW, u32 V, u32 VFP, u32 VBP, u32 VSW, float PCLK) {
    h = H;
    hfp = HFP;
    hsw = HSW;
    hbp = HBP;
    v = V;
    vfp = VFP;
    vsw = VSW;
    vbp = VBP;
    pclk = PCLK;
}

void TC358768Setup::SetVHActive(bool p) {
    polar = p;
}

void TC358768Setup::SetPixelFormat(u32 PixSize, u32 PixFormat) {
    pixelsize = PixSize;
    pixformat = PixFormat;
}

void TC358768Setup::SetMIPI(u32 NumDataLane, float LaneSpeed) {
    numdatalane = NumDataLane;
    desirelaneclk = LaneSpeed;
}

void TC358768Setup::SetTxMode(u32 TxMode) {
    txmode = TxMode;
}

void TC358768Setup::SetClkSorceMHz(float RefClk, u32 SrcSel) {
    refclk = RefClk;
    srcclk = SrcSel;
}

unsigned int TC358768Setup::GenPLLClkSetting() {
    double  usedclk;
    double  ftmp, pllratio, diff, bestdiff;
    double  prd_max, prd_min;
    addrdata    ad;

    u32     i;

    //  find current used clk
    if (srcclk) {     //  use refclk
        usedclk = pclk / 4;
    }
    else {          //  use pclk
        usedclk = refclk;
    }

    //  set FRS
    if (desirelaneclk <= 1000.0 && desirelaneclk > 500.0) {
        frs = 0;
    }
    else if (desirelaneclk <= 500.0 && desirelaneclk > 250.0) {
        frs = 1;
    }
    else if (desirelaneclk <= 250.0 && desirelaneclk > 125.0) {
        frs = 2;
    }
    else if (desirelaneclk <= 125.0 && desirelaneclk >= 62.5) {
        frs = 3;
    }
    else {
        // error
        return 1;
    }
    //  find PRD range
    i = 1;
    //  get prd_min
    while (usedclk / i > 40.0) i++;
    prd_min = i;
    //  get prd_max
    i++;
    while (usedclk / i > 6.0)  i++;
    prd_max = i;

    //  set FBD and PRD
    pllratio = (desirelaneclk / usedclk) * pow(2.0, (double)frs);
    bestdiff = 1.0;
    for (i = prd_min; i < prd_max; i++) {
        ftmp = pllratio * i;
        if (ftmp > pow(2.0, 9.0))
            break;
        else {
            diff = fabs(floor(ftmp + 0.5) - ftmp);
            if (diff < bestdiff) {
                prd = i;
                fbd = (u32)floor(ftmp + 0.5);
                bestdiff = diff;
                if (bestdiff == 0)
                    break;
            }
        }
    }
    reallaneclk = usedclk * fbd / prd / pow(2.0, (double)frs);
    prd--;
    fbd--;

    ad.addr = 0x0016;
    ad.data = (prd << 12) | fbd;
    this->codelist.push_back(ad);

    ad.addr = 0x0018;
    ad.data = (frs << 10) | 0x0203;
    this->codelist.push_back(ad);

    ad.addr = 0xffff;
    ad.data = 2;
    this->codelist.push_back(ad);

    ad.addr = 0x0018;
    ad.data = (frs << 10) | 0x0213;
    this->codelist.push_back(ad);
    return 0;
}

unsigned int TC358768Setup::GenPhySetting() {
    addrdata    ad;

    hsbyteclk = reallaneclk / 8;
    hfclk = hsbyteclk / 2;
    //  check timing
    if (hsw < H_SW_MIN || hsw > H_SW_MAX)    return 1;
    if (hbp < H_BP_MIN || hbp > H_BP_MAX)    return 2;
    if (h < H_ACTIVE_MIN || h > H_ACTIVE_MAX)    return 3;
    if (hfp < H_FP_MIN || hfp > H_FP_MAX)    return 4;
    if ((h + hsw + hbp + hfp) < (H_SW_MIN + H_BP_MIN + H_ACTIVE_MIN + H_FP_MIN) ||
        (h + hsw + hbp + hfp) > 4096) return 5;
    if (vsw < V_SW_MIN || vsw > V_SW_MAX)    return 6;
    if (vbp < V_BP_MIN || vbp > V_BP_MAX)    return 7;
    if (v < V_ACTIVE_MIN || v > V_ACTIVE_MAX)    return 8;
    if (vfp < V_FP_MIN || vfp > V_FP_MAX)    return 9;
    if ((v + vsw + vbp + vfp) < (V_SW_MIN + V_BP_MIN + V_ACTIVE_MIN + V_FP_MIN) ||
        (v + vsw + vbp + vfp) > 8192) return 10;

    //  set fifo (VS delay)
    u32     hsw_cns = ceil(hsw * numdatalane * hsbyteclk / pclk);
    u32     hbp_cns = ceil(hbp * numdatalane * hsbyteclk / pclk);
    float   hswbp_tns = 1000 * (hsw_cns + hbp_cns) / hsbyteclk / numdatalane;
    float   htotal_tns = 1000 * (h + hsw + hbp + hfp) / pclk;
    //    hfp_cns     = htotal_tns-hsw_cns-hbp_cns-1000*pixelsize*h/(reallaneclk*numdatalane);
    float   hdp_tns = 1000 * h / pclk;
    float   hpw_tns = 1000 * hsw / pclk;
    float   hbp_tns = 1000 * hbp / pclk;
    //    float   hfp_tns     = 1000*hfp/pclk;
    float   vsd_max_tns = htotal_tns + hpw_tns + hbp_tns + hdp_tns / 4 - hswbp_tns - 1000 * pixelsize * h / (reallaneclk * numdatalane);
    float   vsd_min_tns = (hpw_tns + hbp_tns + hdp_tns) - (hswbp_tns + 1000 * pixelsize * h / (reallaneclk * numdatalane));
    //  use middle value
    //  use 90% value
    //  use dynamic value
    /*
        Higher PCLK use fewer FIFO depth, and vise versa
    */
    float   vsd_check_tns = 200;
    //    float   vsd_90p_tns = (vsd_max_tns-vsd_min_tns)*0.9+vsd_min_tns;
    //    float   vsd_mid_tns = (vsd_max_tns+vsd_min_tns)/2;
    //    u16     vsd         = ceil(vsd_mid_tns/(4*8*1000/(pixelsize*pclk)));
    u16     vsd_min = ceil(vsd_min_tns / (4 * 8 * 1000 / (pixelsize * pclk)));
    u16     vsd_max = ceil(vsd_max_tns / (4 * 8 * 1000 / (pixelsize * pclk)));
    if (vsd_max > 0x03ff)    vsd_max = 0x03ff;

    u16     vsd = (vsd_max - vsd_min) * (1 - (pclk - PCLK_MIN) / (PCLK_MAX - PCLK_MIN)) + vsd_min;

    //  check again if match another condition
    for (;vsd < vsd_max;vsd++) {
        vsd_check_tns = vsd * (4 * 8 * 1000 / (pixelsize * pclk));
        if ((vsd_check_tns + hswbp_tns) >= (hpw_tns + hbp_tns)) {
            break;
        }
    }
    //    float   step1_vsmin_tns= hpw_tns+hbp_tns-hsw_cns-hbp_cns+100;
    hfp_cns = htotal_tns - hswbp_tns - (1000 * pixelsize * h / reallaneclk / numdatalane);
    float   hsw_bp_delay_cns = hswbp_tns + vsd_check_tns;
    float   h_sw_bp_delay_cns = hsw_bp_delay_cns + (1000 * pixelsize * h / reallaneclk / numdatalane);

    if (hsw_bp_delay_cns < (hpw_tns + hbp_tns))    return 12;
    if (h_sw_bp_delay_cns < (hpw_tns + hbp_tns + hdp_tns))    return 13;
    ad.addr = 0x0006;
    ad.data = vsd;
    this->codelist.push_back(ad);

    ad.data = 0x0000;
    for (ad.addr = 0x0140; ad.addr < 0x0153; ad.addr += 2) {
        this->codelist.push_back(ad);
    }
    return 0;
}

unsigned int TC358768Setup::GenPPISetting() {
    addrdata    ad;
    u32			d[12];
    u32			tmptck, tmpths, tmprxgo;
    float       THS_TRAILCNT_min, THS_TRAILCNT_min_sub;

#define     lineinitcnt		d[0]
#define     lptxtimecnt		d[1]
#define     tclkzerocnt		d[2]
#define     tclkpreparecnt	tmptck
#define     tclktrailcnt	d[3]
#define     thszerocnt		d[4]
#define     thspreparecnt	tmpths
#define     twakeupcnt		d[5]
#define     tclkpostcnt		d[6]
#define     thstrailcnt		d[7]
#define     hstxvregcnt		d[8]
#define     laneenable      d[9]
#define     clockmode       d[10]
#define     txtagocnt       d[11]
#define     rxtasurecnt     tmprxgo
    //#define     startcntrl      d[12]

    lineinitcnt = (u32)(hfclk * 100);
    //  add 200 to ensure exceeding 100us
    lineinitcnt += 200;

    //**
    lptxtimecnt = (u32)(floor(50 * hsbyteclk / 1000));
    //    float   lptx_slave =
    //    lptxtimecnt++;

        //  38~95ns
    tclkpreparecnt = (u32)(floor(45 * hsbyteclk / 1000));
    //    tclkpreparecnt = 1;
    //    while(tclkpreparecnt/hsbyteclk<0.038){
    //        tclkpreparecnt++;
    //    }
    //    tclkpreparecnt--;

    tclkzerocnt = (u32)floor(0.3 * hsbyteclk);

    //  tclktrailcnt is don't care when continual clock enabled
    tclktrailcnt = 1;

    thspreparecnt = (u32)floor((0.04 + 4 / reallaneclk) * hsbyteclk);

    thszerocnt = (u32)ceil((0.145 + 10 / reallaneclk) * hsbyteclk);
    thszerocnt = thszerocnt - thspreparecnt;

    //  add 2500
    twakeupcnt = (u32)(1000 / (lptxtimecnt + 1) * hsbyteclk);
    twakeupcnt += 2500;

    tclkpostcnt = (u32)ceil((0.06 + 52 / reallaneclk) * hsbyteclk);

    THS_TRAILCNT_min = 0.06 + 4 / reallaneclk;
    THS_TRAILCNT_min_sub = 8 / reallaneclk;
    if (THS_TRAILCNT_min < THS_TRAILCNT_min_sub)
        THS_TRAILCNT_min = THS_TRAILCNT_min_sub;

    thstrailcnt = 0;
    //    while(((1/hsbyteclk)*4))
    while ((4 + thstrailcnt) / hsbyteclk < (THS_TRAILCNT_min + 11 / reallaneclk)) {
        thstrailcnt++;
    }
    if (thstrailcnt > 0)
        thstrailcnt--;
    THS_TRAILCNT_min *= 1000;

    hstxvregcnt = 5;        // this is default

    laneenable = 0x0000001f;
    //#$# revious
    clockmode = 1;
    rxtasurecnt = ((lptxtimecnt + 1) * 2) - 3;

    txtagocnt = (u32)lptxtimecnt;

    //***   recheck all parameters  ***
    //  TCLK_PREPARECNT
    float TCLK_PREPARECNT;
    for (;;) {
        TCLK_PREPARECNT = (1 / hsbyteclk) * (tclkpreparecnt + 1) * 1000;
        if (TCLK_PREPARECNT - 5 >= 38) {
            if (TCLK_PREPARECNT + 5 <= 95 && tclkpreparecnt <= 127)
                break;
            else    //  fail
                return 1;
        }
        else
            tclkpreparecnt++;
    }

    //  TCLK_ZEROCNT
    float TCLK_ZEROCNT;
    for (;;) {
        TCLK_ZEROCNT = ((1 / hsbyteclk) * ((float)tclkzerocnt + 2.5) + (1 / reallaneclk) * 3.5) * 1000;
        if (TCLK_ZEROCNT - 5 >= 300) {
            if (tclkzerocnt <= 255)
                break;
            else    //  fail
                return 2;
        }
        else
            tclkzerocnt++;
    }

    //  THS_PREPARECNT
    float       THS_PREPARECNT, THS_PREPARECNT_max, THS_PREPARECNT_min;
    THS_PREPARECNT_min = 40 + 4 * 1000 / reallaneclk;
    THS_PREPARECNT_max = 85 + 6 * 1000 / reallaneclk;
    for (;;) {
        THS_PREPARECNT = (thspreparecnt + 1) / hsbyteclk * 1000;
        if (THS_PREPARECNT - 5 >= THS_PREPARECNT_min) {
            if (THS_PREPARECNT + 5 <= THS_PREPARECNT_max && thspreparecnt <= 127)
                break;
            else    //  fail
                return 3;
        }
        else
            thspreparecnt++;
    }

    //  THS_ZEROCNT
    float   THS_ZEROCNT, THS_ZEROCNT_min;
    THS_ZEROCNT_min = 145 + (1 / reallaneclk) * 1000 * 10;
    for (;;) {
        THS_ZEROCNT = ((1 / hsbyteclk) * 4 + (1 / hsbyteclk) * (thszerocnt + 7) + (1 / reallaneclk) * 11) * 1000;
        if (THS_ZEROCNT - 5 >= THS_ZEROCNT_min) {
            if (thszerocnt <= 127)
                break;
            else    //  fail
                return 4;
        }
        else
            thszerocnt++;
    }

    //  THS_TRAILCNT
    float THS_TRAILCNT, THS_TRAILCNT_max;
    THS_TRAILCNT_max = 105 + (1 / reallaneclk) * 1000 * 12 - 30;
    for (;;) {
        THS_TRAILCNT = ((1 / hsbyteclk) * 4 + (1 / hsbyteclk) * (thstrailcnt + 1) - (1 / reallaneclk) * 11) * 1000;
        if (THS_TRAILCNT - 5 >= THS_TRAILCNT_min) {
            if (THS_TRAILCNT + 5 <= THS_TRAILCNT_max && thstrailcnt <= 15)
                break;
            else    //  fail
                return 5;
        }
        else
            thstrailcnt++;
    }

    //  RXTASURECNT
    float RXTASURECNT_h, RXTASURECNT_l, RXTASURECNT_min, RXTASURECNT_max;
    RXTASURECNT_min = (1 / hsbyteclk) * (lptxtimecnt + 1) * 1000;
    RXTASURECNT_max = RXTASURECNT_min * 2;
    for (;;) {
        RXTASURECNT_l = (rxtasurecnt + 2) * (1 / hsbyteclk) * 1000;
        RXTASURECNT_h = (rxtasurecnt + 3) * (1 / hsbyteclk) * 1000;
        if (RXTASURECNT_l >= RXTASURECNT_min) {
            if (RXTASURECNT_h <= RXTASURECNT_max && rxtasurecnt <= 15)
                break;
            else    //  fail
                return 6;
        }
        else
            rxtasurecnt++;
    }

    //  TXTAGOCNT
    float TXTAGOCNT = (txtagocnt + 1) * 4 * (1 / hsbyteclk) * 1000;
    if (TXTAGOCNT != RXTASURECNT_min * 4 && txtagocnt <= 15)
        return 7;

    //  HFP total time check
//    float   TCLK_TRAILCNT = (4/hsbyteclk+((tclktrailcnt+1)/hsbyteclk)-(2.5/reallaneclk))*1000;
//    float   TCLK_POSTCNT = (1/hsbyteclk+(1/hsbyteclk)*((float)tclkpostcnt+2.5)+(1/reallaneclk)*2.5)*1000;
    float   HFP_CNS_min = (THS_TRAILCNT + RXTASURECNT_min + 25 / hsbyteclk + RXTASURECNT_min + THS_ZEROCNT) * 2;
    //    float   HFP_CNS_min =   THS_TRAILCNT +
    //                            TCLK_POSTCNT +
    //                            TCLK_TRAILCNT +
    //                            RXTASURECNT_max +
    //                            1000*22/hsbyteclk +
    //                            RXTASURECNT_min +
    //                            TCLK_ZEROCNT +
    //                            (13+lptxtimecnt*8)/hsbyteclk*1000 +
    //                            RXTASURECNT_min +
    //                            THS_ZEROCNT;
    if (hfp_cns < HFP_CNS_min)
        return 8;

    //  rearrange data
    tclkzerocnt = (tclkzerocnt << 8) | tclkpreparecnt;
    thszerocnt = (thszerocnt << 8) | thspreparecnt;
    txtagocnt = (txtagocnt << 16) | rxtasurecnt;

    ad.addr = 0x0210;
    for (tmptck = 0; tmptck < 12; tmptck++) {
        ad.data = d[tmptck] & 0x0000FFFF;
        this->codelist.push_back(ad);
        ad.addr += 2;
        ad.data = (d[tmptck] & 0xFFFF0000) >> 16;
        this->codelist.push_back(ad);
        ad.addr += 2;
    }

    ad.addr = 0x0204;
    ad.data = 0x0001;
    this->codelist.push_back(ad);
    ad.addr = 0x0206;
    ad.data = 0x0000;
    this->codelist.push_back(ad);

    return 0;
    //    ad.addr = 0;
}

unsigned int TC358768Setup::GenTxTiming() {
    u16         d[7];
    u32         i;
    addrdata    ad;
#define     tx_msel     d[0]
#define     tx_vsw      d[1]
#define     tx_vbp      d[2]
#define     tx_val      d[3]
#define     tx_hsw      d[4]
#define     tx_hbp      d[5]
#define     tx_hal      d[6]
    //    u16     tmpv,tmph;

    if (txmode)
        tx_msel = 1;
    else
        tx_msel = 0;

    //  v
    tx_vsw = vsw;
    tx_vbp = vbp;
    tx_val = v;

    //  h
    tx_hsw = (u32)ceil(hsw * hsbyteclk / pclk * numdatalane);
    tx_hbp = (u32)ceil(hbp * hsbyteclk / pclk * numdatalane);
    tx_hal = (u32)ceil((h) * (double)(pixelsize / 8));

    //  common
//    tx_vsw += tx_vbp;
    if (txmode) {
        tx_vsw += tx_vbp;
        tx_hsw += tx_hbp;
    }

    ad.addr = 0x0620;
    for (i = 0;i < 7;i++) {
        ad.data = d[i];
        this->codelist.push_back(ad);
        ad.addr += 2;
    }

    ad.addr = 0x0518;
    ad.data = 1;
    this->codelist.push_back(ad);
    ad.addr += 2;
    ad.data = 0;
    this->codelist.push_back(ad);
    return 0;
}

void TC358768Setup::GenSwitchToHS() {
    //    u16         d[];
    addrdata    ad;

    if (pixformat == 0x0E) {
        ad.data = 0x0057;
    }
    else if (pixformat == 0x1E) {
        ad.data = 0x0047;
    }
    else if (pixformat == 0x2E) {
        ad.data = 0x004F;
    }
    else if (pixformat == 0x3E) {
        ad.data = 0x0037;
    }
    ad.addr = 0x0008;
    this->codelist.push_back(ad);

    ad.addr = 0x0050;
    ad.data = pixformat;
    this->codelist.push_back(ad);

    ad.addr = 0x0500;
    if (numdatalane == 1)
        ad.data = 0x0080;
    else if (numdatalane == 2)
        ad.data = 0x0082;
    else if (numdatalane == 3)
        ad.data = 0x0084;
    else if (numdatalane == 4)
        ad.data = 0x0086;
    this->codelist.push_back(ad);

    ad.addr = 0x0502;
    ad.data = 0xA300;
    this->codelist.push_back(ad);
    ad.addr = 0x0500;
    ad.data = 0x8000;
    this->codelist.push_back(ad);
    ad.addr = 0x0502;
    ad.data = 0xC300;
    this->codelist.push_back(ad);
    //  add polarity config *********** danger *************
//    if(polar){
//        ad.addr = 0x0032;
//        ad.data = 0x0000;
//        this->codelist.push_back(ad);
//        ad.data = 0x0064;
//    }else{
//        ad.addr = 0x0032;
//        ad.data = 0x0001;
//        this->codelist.push_back(ad);
//        ad.data = 0x0044;
//    }
    if (polar)
        ad.data = 0x0064;
    else
        ad.data = 0x0044;
    ad.addr = 0x0004;
    this->codelist.push_back(ad);

}

unsigned short TC358768Setup::TransVCOMPacketToCodelist(packet pk, u32 VCOM_byte_loc) {
    u32         i;
    u16         tmp;
    u16         VCOM_adr_cnt = 0, VCOM_adr = 0;
    addrdata    ad;

    if ((pk.type_id & 0xFF00) == 0x4000) {        //  long packet
        if (pk.data.size() <= 8) {      // <= 8 bytes, general method is enough
            ad.addr = 0x0602;
            ad.data = pk.type_id;
            this->codelist.push_back(ad);

            ad.addr += 2;
            ad.data = pk.data.size();
            this->codelist.push_back(ad);
            VCOM_adr_cnt += 8;

            ad.addr = 0x0610;
            for (i = 0; i < pk.data.size(); i++) {
                if (i == VCOM_byte_loc)
                    VCOM_adr = VCOM_adr_cnt + 2;
                ad.data = pk.data.at(i);
                i++;
                if (i == VCOM_byte_loc)
                    VCOM_adr = VCOM_adr_cnt + 3;
                if (i < pk.data.size()) {
                    tmp = pk.data.at(i);
                    ad.data |= (tmp << 8);
                }
                this->codelist.push_back(ad);
                VCOM_adr_cnt += 4;
                ad.addr += 2;
            }

            ad.addr = 0x0600;
            ad.data = 1;
            this->codelist.push_back(ad);
        }
        else {      //  > 8 byte, AXBG only
            ad.addr = 0x0008;
            ad.data = 0x0001;
            this->codelist.push_back(ad);

            ad.addr = 0x0050;
            ad.data = pk.type_id & 0x00FF;
            this->codelist.push_back(ad);

            ad.addr = 0x0022;
            ad.data = pk.data.size();
            this->codelist.push_back(ad);

            ad.addr = 0x00E0;
            ad.data = 0x8000;
            this->codelist.push_back(ad);
            VCOM_adr_cnt += 16;

            //  put data
            i = 0;
            ad.addr = 0x00E8;
            while (i < pk.data.size()) {
                if (i == VCOM_byte_loc)
                    VCOM_adr = VCOM_adr_cnt + 2;
                ad.data = pk.data.at(i);
                i++;
                if (i == VCOM_byte_loc)
                    VCOM_adr = VCOM_adr_cnt + 3;
                if (i < pk.data.size()) {
                    tmp = pk.data.at(i);
                    ad.data |= (tmp << 8);
                }
                this->codelist.push_back(ad);

                i++;
                if (i == VCOM_byte_loc)
                    VCOM_adr = VCOM_adr_cnt + 6;
                if (i < pk.data.size()) {
                    ad.data = pk.data.at(i);
                    i++;
                    if (i == VCOM_byte_loc)
                        VCOM_adr = VCOM_adr_cnt + 7;
                    if (i < pk.data.size()) {
                        tmp = pk.data.at(i);
                        ad.data |= (tmp << 8);
                    }
                }
                else {
                    ad.data = 0x0000;
                    i++;
                }
                this->codelist.push_back(ad);
                VCOM_adr_cnt += 8;
                i++;
            }

            //  send packet
            ad.addr = 0x00E0;
            ad.data = 0xE000;
            this->codelist.push_back(ad);

            ad.addr = 0xFFFF;
            ad.data = 0x0001;
            this->codelist.push_back(ad);

            ad.addr = 0x00E0;
            ad.data = 0x2000;
            this->codelist.push_back(ad);

            ad.data = 0x0000;
            this->codelist.push_back(ad);
        }
    }
    else if ((pk.type_id & 0xFF00) == 0x1000) {  //  short packet
       //  support for read packet
        if ((pk.type_id & 0x000F) == 4 || (pk.type_id & 0x000F) == 6) {
            //  a read packet
            //  pid
            ad.addr = 0xfff1;
            ad.data = pk.type_id;
            this->codelist.push_back(ad);
            //  param
            switch ((pk.type_id >> 4) & 0x0f) {
            case 0:     //  no param
                ad.data = 0;
                break;
            case 1:     //  1 param
                ad.data = pk.data.at(0);
                break;
            case 2:     //  2 param
                ad.data = pk.data.at(1);
                ad.data <<= 8;
                ad.data |= pk.data.at(0);
                break;
            default:    //  0.0 WTF!
                ad.data = 0;
                break;
            }
            ad.addr = 0xfff2;
            this->codelist.push_back(ad);
            //  max read bytes (1~65535)
            ad.addr = 0xfff3;
            ad.data = pk.data.at(pk.data.size() - 2);
            ad.data <<= 8;
            ad.data |= pk.data.back();
            this->codelist.push_back(ad);
            VCOM_adr = 0;
        }
        else {
            //  a write packet
            ad.addr = 0x0602;
            ad.data = pk.type_id;
            this->codelist.push_back(ad);

            ad.addr += 2;
            ad.data = 0;
            this->codelist.push_back(ad);

            ad.addr = 0x0610;
            ad.data = pk.data.at(0);
            if (pk.data.size() > 1) {
                tmp = pk.data.at(1);
                ad.data |= (tmp << 8);
                VCOM_adr = 11;
            }
            else {
                VCOM_adr = 10;
            }
            this->codelist.push_back(ad);

            ad.addr = 0x0600;
            ad.data = 0x0001;
            this->codelist.push_back(ad);
        }
    }

    //  **optional add a delay for waiting transmission
    //  normal packet, asign delay normally
    if (!pk.delay) {
        ad.data = 0x0001;
    }
    else {
        ad.data = pk.delay;
    }
    ad.addr = 0xffff;
    this->codelist.push_back(ad);
    return VCOM_adr;
}

void TC358768Setup::TransPacketToCodelist(packet pk) {
    u32         i;
    u16         tmp;
    addrdata    ad;

    if ((pk.type_id & 0xFF00) == 0x4000) {        //  long packet
        if (pk.data.size() <= 8) {      // <= 8 bytes, general method is enough
            ad.addr = 0x0602;
            ad.data = pk.type_id;
            this->codelist.push_back(ad);

            ad.addr += 2;
            ad.data = pk.data.size();
            this->codelist.push_back(ad);

            ad.addr = 0x0610;
            for (i = 0; i < pk.data.size(); i++) {
                ad.data = pk.data.at(i);
                i++;
                if (i < pk.data.size()) {
                    tmp = pk.data.at(i);
                    ad.data |= (tmp << 8);
                }
                this->codelist.push_back(ad);
                ad.addr += 2;
            }

            ad.addr = 0x0600;
            ad.data = 1;
            this->codelist.push_back(ad);
        }
        else {      //  > 8 byte, AXBG only
            ad.addr = 0x0008;
            ad.data = 0x0001;
            this->codelist.push_back(ad);

            ad.addr = 0x0050;
            ad.data = pk.type_id & 0x00FF;
            this->codelist.push_back(ad);

            ad.addr = 0x0022;
            ad.data = pk.data.size();
            this->codelist.push_back(ad);

            ad.addr = 0x00E0;
            ad.data = 0x8000;
            this->codelist.push_back(ad);

            //  put data
            i = 0;
            ad.addr = 0x00E8;
            while (i < pk.data.size()) {
                ad.data = pk.data.at(i);
                i++;
                if (i < pk.data.size()) {
                    tmp = pk.data.at(i);
                    ad.data |= (tmp << 8);
                }
                this->codelist.push_back(ad);

                i++;
                if (i < pk.data.size()) {
                    ad.data = pk.data.at(i);
                    i++;
                    if (i < pk.data.size()) {
                        tmp = pk.data.at(i);
                        ad.data |= (tmp << 8);
                    }
                }
                else {
                    ad.data = 0x0000;
                    i++;
                }
                this->codelist.push_back(ad);
                i++;
            }

            //  send packet
            ad.addr = 0x00E0;
            ad.data = 0xE000;
            this->codelist.push_back(ad);

            ad.addr = 0xFFFF;
            ad.data = 0x0001;
            this->codelist.push_back(ad);

            ad.addr = 0x00E0;
            ad.data = 0x2000;
            this->codelist.push_back(ad);

            ad.data = 0x0000;
            this->codelist.push_back(ad);
        }
    }
    else if ((pk.type_id & 0xFF00) == 0x1000) {  //  short packet
       //  support for read packet
        if ((pk.type_id & 0x000F) == 4 || (pk.type_id & 0x000F) == 6) {
            //  a read packet
            //  pid
            ad.addr = 0xfff1;
            ad.data = pk.type_id;
            this->codelist.push_back(ad);
            //  param
            switch ((pk.type_id >> 4) & 0x0f) {
            case 0:     //  no param
                ad.data = 0;
                break;
            case 1:     //  1 param
                ad.data = pk.data.at(0);    //  DCS no param or genaric 1 param
                break;
            case 2:     //  2 param
                ad.data = pk.data.at(1);
                ad.data <<= 8;
                ad.data |= pk.data.at(0);
                break;
            default:    //  0.0 WTF!
                ad.data = 0;
                break;
            }
            ad.addr = 0xfff2;
            this->codelist.push_back(ad);
            //  max read bytes (1~65535)
            ad.addr = 0xfff3;
            ad.data = pk.data.at(pk.data.size() - 2);
            ad.data <<= 8;
            ad.data |= pk.data.back();
            this->codelist.push_back(ad);
        }
        else {
            //  a write packet
            ad.addr = 0x0602;
            ad.data = pk.type_id;
            this->codelist.push_back(ad);

            ad.addr += 2;
            ad.data = 0;
            this->codelist.push_back(ad);

            ad.addr = 0x0610;
            ad.data = pk.data.at(0);
            if (pk.data.size() > 1) {
                tmp = pk.data.at(1);
                ad.data |= (tmp << 8);
            }
            this->codelist.push_back(ad);

            ad.addr = 0x0600;
            ad.data = 0x0001;
            this->codelist.push_back(ad);
        }
    }

    //  **optional add a delay for waiting transmission
//    if(pk.type_id){
    //  normal packet, asign delay normally
    if (!pk.delay) {
        ad.data = 0x0001;
    }
    else {
        ad.data = pk.delay;
    }
    //    }else{
            //  packet ID = 0 is defined as a null packet, only delay is available
            //ad.addr = 0xffff;
    //        ad.data = pk.data.at(0);
    //    }
    ad.addr = 0xffff;
    this->codelist.push_back(ad);

}

void TC358768Setup::GenLPPackets() {
    u32     i;

    if (this->plist.size() == 0)
        return;
    this->RearrangePackets();

    i = 0;
    while (!this->plist.at(i).order) {
        this->TransPacketToCodelist(this->plist.at(i++));
        if (i >= this->plist.size())
            break;
    }
}

void TC358768Setup::GenHSPackets() {
    u32        i;

    //  no need to rearrange if lazy use...
    //  this->RearrangePackets();

    if (this->plist.size() == 0)
        return;
    i = 0;
    while (!this->plist.at(i++).order);

    while (i < this->plist.size()) {
        this->TransPacketToCodelist(this->plist.at(i++));
        if (i >= this->plist.size())
            break;
    }
}

void TC358768Setup::RearrangePackets() {
    std::vector<packet>     lplist, hslist;
    u32     i;

    if (this->plist.size() == 0)
        return;
    for (i = 0;i < this->plist.size();i++) {
        if (this->plist.at(i).order) {        //HS
            hslist.push_back(this->plist.at(i));
        }
        else {          //LP
            lplist.push_back(this->plist.at(i));
        }
    }

    this->plist.clear();
    this->plist.insert(this->plist.begin(), lplist.begin(), lplist.end());
    this->plist.insert(this->plist.end(), hslist.begin(), hslist.end());
}

void TC358768Setup::AddPacket(packet pk) {
    //if(!pk.type_id)     //  delay packet
    //    pk.delay = pk.data.
    this->plist.push_back(pk);
}

