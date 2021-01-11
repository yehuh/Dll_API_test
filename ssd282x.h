#pragma once
#ifndef SSD282X_H
#define SSD282X_H

#include "Header.h"
#include <math.h>

class haha {
public:
    haha();
    int xx;
    int yy;
};

extern "C" MATHLIBRARY_API class SSD282X : public TC358768Setup
{
public:
    SSD282X();
    void SetBridgeIC(u32 ic);
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
    //private:
    u32 bridge_ic;
    TC358768Setup* toshiba_bridge;
};

SSD282X::SSD282X(){
    bridge_ic = 0;      //default is 768 AXBG
    toshiba_bridge = static_cast<TC358768Setup*>(this);
}

void SSD282X::SetBridgeIC(u32 ic) {
    bridge_ic = ic;
    //  clear all codes after bridge IC changed
    codelist.clear();
}

void SSD282X::TransPacketToCodelist(packet pk) {
    if (bridge_ic == 0)
        toshiba_bridge->TransPacketToCodelist(pk);
    else {
        addrdata    ad;
        u32 i;

        if ((pk.type_id & 0x000F) == 4 || (pk.type_id & 0x000F) == 6) {
            //  a read packet
            //  pid
            ad.addr = 0xfff1;
            ad.data = pk.type_id;
            codelist.push_back(ad);
            //  param
            switch ((pk.type_id >> 4) & 0x0f) {
            case 0:     //  no param
                if ((pk.type_id & 0x000F) == 6)	//	must 1 param if it is a DCS read
                    ad.data = pk.data.at(0);
                else
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
            codelist.push_back(ad);
            //  max read bytes (1~65535)
            ad.addr = 0xfff3;
            ad.data = pk.data.at(pk.data.size() - 2);
            ad.data <<= 8;
            ad.data |= pk.data.back();
            codelist.push_back(ad);
        }
        else if (pk.type_id & 0x00FF) {
            //  a write packet
            //  0xE0b7 : tell MCU to clear reg[b7] with corresponding bits
            //  0xE1b7 : tell MCU to set reg[b7] with corresponding bits

            if ((pk.type_id & 0x000f) == 0x0005 || (pk.type_id & 0x00ff) == 0x0039) {
                //  DCS write
                ad.addr = 0xE1b7;
                ad.data = 0x0040;
            }
            else {
                //  genaric write
                ad.addr = 0xE0b7;
                ad.data = 0x0040;
            }
            codelist.push_back(ad);

            //  set packet size
            ad.addr = 0x00bc;
            ad.data = pk.data.size();
            codelist.push_back(ad);

            //  push packet data
            for (i = 0;i < pk.data.size();i += 2) {
                if (i == 0) {
                    ad.addr = 0x00bf;
                }
                else {
                    ad.addr = 0xffE2;
                }
                if (i + 1 < pk.data.size()) {
                    ad.data = pk.data.at(i + 1);
                    ad.data <<= 8;
                    ad.data |= pk.data.at(i);
                }
                else {
                    ad.data = pk.data.at(i);
                }
                codelist.push_back(ad);
            }
        }

        if (!pk.delay) {
            ad.data = 0x0001;
        }
        else {
            ad.data = pk.delay;
        }
        ad.addr = 0xffff;
        codelist.push_back(ad);
    }
}

unsigned short SSD282X::TransVCOMPacketToCodelist(packet pk, u32 VCOM_byte_loc) {
    if (bridge_ic == 0)
        return toshiba_bridge->TransVCOMPacketToCodelist(pk, VCOM_byte_loc);
    else {
        addrdata    ad;
        u32 i;
        u16 VCOM_adr = 0, VCOM_adr_done = 0;

        if ((pk.type_id & 0x000F) == 4 || (pk.type_id & 0x000F) == 6) {
            //  a read packet
            //  pid
            ad.addr = 0xfff1;
            ad.data = pk.type_id;
            codelist.push_back(ad);
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
            codelist.push_back(ad);
            //  max read bytes (1~65535)
            ad.addr = 0xfff3;
            ad.data = pk.data.at(pk.data.size() - 2);
            ad.data <<= 8;
            ad.data |= pk.data.back();
            codelist.push_back(ad);

        }
        else if (pk.type_id & 0x00FF) {
            //  a write packet
            //  0xE0b7 : tell MCU to clear reg[b7] with corresponding bits
            //  0xE1b7 : tell MCU to set reg[b7] with corresponding bits

            if ((pk.type_id & 0x000f) == 0x0005 || (pk.type_id & 0x00ff) == 0x0039) {
                //  DCS write
                ad.addr = 0xE1b7;
                ad.data = 0x0040;
            }
            else {
                //  genaric write
                ad.addr = 0xE0b7;
                ad.data = 0x0040;
            }
            codelist.push_back(ad);

            //  set packet size
            ad.addr = 0x00bc;
            ad.data = pk.data.size();
            codelist.push_back(ad);

            VCOM_adr += 8;

            //  push packet data
            for (i = 0;i < pk.data.size();i += 2) {
                if (i == 0) {
                    ad.addr = 0x00bf;
                }
                else {
                    ad.addr = 0xffE2;
                }
                if (i + 1 < pk.data.size()) {
                    ad.data = pk.data.at(i + 1);
                    ad.data <<= 8;
                    ad.data |= pk.data.at(i);
                }
                else {
                    ad.data = pk.data.at(i);
                }
                codelist.push_back(ad);
                if (i == VCOM_byte_loc) {
                    //  find vcom address
                    VCOM_adr += 2;
                    VCOM_adr_done = 1;
                }
                else if (i + 1 == VCOM_byte_loc) {
                    //  vcom address is located at next byte
                    VCOM_adr += 3;
                    VCOM_adr_done = 1;
                }
                else {
                    if (!VCOM_adr_done)
                        VCOM_adr += 4;
                }
            }
        }

        if (!pk.delay) {
            ad.data = 0x0001;
        }
        else {
            ad.data = pk.delay;
        }
        ad.addr = 0xffff;
        codelist.push_back(ad);
        return VCOM_adr;
    }
}

void SSD282X::GenSoftwareReset() {
    if (bridge_ic == 0)
        toshiba_bridge->GenSoftwareReset();
    else {
        addrdata    ad;
        ad.addr = 0x00c0;
        ad.data = 0x0100;
        codelist.push_back(ad);
        ad.addr = 0xFFFF;
        ad.data = 0x000A;
        codelist.push_back(ad);
    }
}

unsigned int SSD282X::GenPLLClkSetting() {
    if (bridge_ic == 0)
        return toshiba_bridge->GenPLLClkSetting();
    else {
        addrdata    ad;
        //        double  usedclk;
        double  ftmp, diff_p, diff, bestdiff;
        double  ms_max, ns_max;

        u16 lpd, ms, ns;    //  default LP clk is about 9MHz
        u32 i, j;

        ad.addr = 0x00b9;
        ad.data = 0x0000;
        codelist.push_back(ad);

        //  set FRS
        if (desirelaneclk <= 1250.0 && desirelaneclk > 500.0) {
            frs = 3;
        }
        else if (desirelaneclk <= 500.0 && desirelaneclk > 250.0) {
            frs = 2;
        }
        else if (desirelaneclk <= 250.0 && desirelaneclk > 125.0) {
            frs = 1;
        }
        else if (desirelaneclk <= 125.0 && desirelaneclk >= 62.5) {
            frs = 0;
        }
        else {
            // error
            return 1;
        }

        ms_max = 31;
        ns_max = 255;
        bestdiff = 1250.0;

        diff_p = bestdiff;
        ms = 1;
        ns = 1;

        for (i = 1;i <= ms_max;i++) {
            for (j = 1;j < ns_max;j++) {
                ftmp = refclk / i * j;
                if (ftmp >= desirelaneclk) {
                    diff = ftmp - desirelaneclk;
                    if (diff == 0) {
                        bestdiff = 0;
                        ns = j;
                        ms = i;
                    }
                    else if (diff < diff_p) {
                        if (diff < bestdiff) {
                            bestdiff = diff;
                            ns = j;
                            ms = i;
                        }
                    }
                    else {
                        if (diff_p < bestdiff) {
                            bestdiff = diff_p;
                            ns = j;
                            ms = i;
                        }
                    }

                }
                else {
                    diff_p = desirelaneclk - ftmp;
                }
            }

            if (bestdiff == 0)
                break;
        }

        reallaneclk = refclk / ms * ns;
        ftmp = reallaneclk / 8;
        ftmp /= 9;  //  9MHz
        lpd = round(ftmp);

        ad.addr = 0x00ba;
        ad.data = frs << 6;
        ad.data |= ms;
        ad.data <<= 8;
        ad.data |= ns;
        codelist.push_back(ad);

        ad.addr = 0x00bb;
        ad.data = lpd - 1;
        codelist.push_back(ad);

        ad.addr = 0xffff;
        ad.data = 10;
        codelist.push_back(ad);

        ad.addr = 0x00b9;
        ad.data = 0x0001;
        codelist.push_back(ad);
        return 0;
    }
}

unsigned int SSD282X::GenPhySetting() {
    if (bridge_ic == 0)
        return toshiba_bridge->GenPhySetting();
    else {
        addrdata ad;
        ad.addr = 0x00de;
        ad.data = numdatalane - 1;
        codelist.push_back(ad);

        ad.addr = 0x00d6;
        ad.data = 0x0004;
        codelist.push_back(ad);
        return 0;
    }
}

unsigned int SSD282X::GenPPISetting() {
    if (bridge_ic == 0)
        return toshiba_bridge->GenPPISetting();
    else {
        addrdata ad;
        double  ui_period_ns = 1000 / reallaneclk;
        double  nibble_clk_period_ns = ui_period_ns * 4;

        double  ths_prepare_min = 40 + 4 * ui_period_ns;
        double  ths_prepare_max = 85 + 6 * ui_period_ns;
        double  ftmp;
        u16     ths_prepare;
        //  THS_PREPARE
        for (ths_prepare = 1;ths_prepare < 256;ths_prepare++) {
            ftmp = (4 + ths_prepare) * nibble_clk_period_ns;
            if (ftmp >= ths_prepare_min) {
                if (ftmp <= ths_prepare_max) {
                    //  pass
                    break;
                }
                else {
                    //  can not find THS_PREPARE
                    return 1;
                }
            }
        }
        if (ths_prepare > 255) {
            //  can not find THS_PREPARE
            return 1;
        }

        //  THS_ZERO
        double  ths_zero_min = 145 + 10 * ui_period_ns;
        u16 ths_zero;

        for (ths_zero = 1;ths_zero < 256;ths_zero++) {
            ftmp = ths_zero * nibble_clk_period_ns;
            if (ftmp > ths_zero_min) {
                //  pass
                break;
            }
        }
        if (ths_zero > 255) {
            return 2;
        }

        //  TCLK_PREPARE    (38~95)
        u16 tclk_prepare;
        for (tclk_prepare = 1;tclk_prepare < 0x0100;tclk_prepare++) {
            ftmp = (3 + tclk_prepare) * nibble_clk_period_ns;
            if (ftmp >= 38.0) {
                if (ftmp <= 95.0) {
                    //  pass
                    break;
                }
                else {
                    return 3;
                }
            }
        }
        if (tclk_prepare > 255) {
            return 3;
        }

        //  TCLK_ZERO   (>300)
        u16 tclk_zero;
        for (tclk_zero = 1; tclk_zero < 0x0100;tclk_zero++) {
            ftmp = tclk_zero * nibble_clk_period_ns;
            if (ftmp > 300) {
                //  pass
                break;
            }
        }
        if (tclk_zero > 255) {
            return 4;
        }

        //  CPED = 4, no need to change
        //  CPTD
        double  clk_post_period_ns_min = 60 + 52 * ui_period_ns;
        u16 clk_post;
        for (clk_post = 1;clk_post < 0x0100;clk_post++) {
            ftmp = clk_post * nibble_clk_period_ns;
            if (ftmp > clk_post_period_ns_min) {
                //  pass
                break;
            }
        }
        if (clk_post > 255) {
            return 5;
        }

        //  THS_TRAIL
        double  ths_trail_min_ns = 60 + 4 * ui_period_ns;
        ftmp = 8 * ui_period_ns;
        if (ths_trail_min_ns < ftmp)
            ths_trail_min_ns = ftmp;
        u16 ths_trail;
        for (ths_trail = 1; ths_trail < 0x0100; ths_trail++) {
            ftmp = ths_trail * nibble_clk_period_ns;
            if (ftmp > ths_trail_min_ns) {
                //  pass
                break;
            }
        }
        if (ths_trail > 255) {
            return 6;
        }

        //  TCLK_TRAIL      (>60)
        u16 tclk_trail;
        for (tclk_trail = 1; tclk_trail < 0x0100;tclk_trail++) {
            ftmp = tclk_trail * nibble_clk_period_ns;
            if (ftmp > 60) {
                //  pass
                break;
            }
        }
        if (tclk_trail > 255) {
            return 7;
        }

        ad.addr = 0x00c9;
        ad.data = ths_zero << 8;
        ad.data |= ths_prepare;
        codelist.push_back(ad);

        ad.addr = 0x00ca;
        ad.data = tclk_zero << 8;
        ad.data |= tclk_prepare;
        codelist.push_back(ad);

        ad.addr = 0x00cb;
        ad.data = clk_post | 0x0400;
        codelist.push_back(ad);

        ad.addr = 0x00cc;
        ad.data = tclk_trail << 8;
        ad.data |= ths_trail;
        codelist.push_back(ad);

        return 0;
    }
}

unsigned int SSD282X::GenTxTiming() {
    if (bridge_ic == 0)
        return toshiba_bridge->GenTxTiming();
    else {
        u16 tmp;
        addrdata ad;

        tmp = vsw;
        ad.addr = 0x00b1;
        ad.data = tmp << 8;
        ad.data |= hsw;
        codelist.push_back(ad);

        ad.addr = 0x00b2;
        if (txmode) {
            //  burst or event
            tmp = vsw + vbp;
            ad.data = tmp << 8;
            tmp = hsw + hbp;
            ad.data |= tmp;
        }
        else {
            //  pulse
            ad.data = vbp << 8;
            ad.data |= hbp;
        }
        codelist.push_back(ad);

        ad.addr = 0x00b3;
        ad.data = vfp << 8;
        ad.data |= hfp;
        codelist.push_back(ad);

        ad.addr = 0x00b4;
        ad.data = h;
        codelist.push_back(ad);

        ad.addr = 0x00b5;
        ad.data = v;
        codelist.push_back(ad);

        ad.addr = 0x00b8;
        ad.data = 0;
        codelist.push_back(ad);

        ad.addr = 0x00b6;
        ad.data = 0;
        if (txmode < 3)
            ad.data = ad.data | (u16)(txmode << 2);
        else
            return 1;

        if (pixformat == 0x0E) {
            //  565
            ad.data |= 0x0000;
        }
        else if (pixformat == 0x1E) {
            //  666
            ad.data |= 0x0001;
        }
        else if (pixformat == 0x2E) {
            //  666 loosely
            ad.data |= 0x0002;
        }
        else if (pixformat == 0x3E) {
            //  888
            ad.data |= 0x0003;
        }
        else {
            return 2;
        }
        ad.data |= 0x0040;		//	send non-video packets in LP mode
        codelist.push_back(ad);

        ad.addr = 0x00b7;
        ad.data = 0x0340;
        codelist.push_back(ad);

        return 0;
    }
}

void SSD282X::GenLPPackets() {
    if (bridge_ic == 0)
        return toshiba_bridge->GenLPPackets();
    else {
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
}

void SSD282X::GenHSPackets() {
    if (bridge_ic == 0)
        return toshiba_bridge->GenLPPackets();
    else {
        u32     i;
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
}

void SSD282X::GenSwitchToHS() {
    if (bridge_ic == 0)
        toshiba_bridge->GenSwitchToHS();
    else {
        addrdata    ad;
        ad.addr = 0x00b7;
        ad.data = 0x034b;
        codelist.push_back(ad);
    }
}

void SSD282X::GenEndOfCode() {
    if (bridge_ic == 0)
        toshiba_bridge->GenEndOfCode();
    else {
        addrdata    ad;
        ad.addr = 0xfff0;
        ad.data = 0x0000;
        codelist.push_back(ad);
    }
}

#endif // SSD282X_H