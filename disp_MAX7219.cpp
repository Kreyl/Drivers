/*
 * disp_MAX7219.cpp
 *
 *  Created on: 02 апр. 2017 г.
 *      Author: Elessar, Eldalim
 */

#include "disp_MAX7219.h"

void Disp_MAX7219_t::BlinkTaskI() {
    static bool lit = true;
    if (lit) WriteRegister(SegBlinking, symEmpty);
    else WriteRegister(SegBlinking, DispBuffer[SegBlinking-1]);
    lit = !lit;
}

void Disp_MAX7219_t::PrintChar(const char *Str) {
    uint8_t SymCode;
    uint8_t Seg = SegCount;
#if defined dmBCDcode
    while (*Str != 0) {
        switch (*Str) {
            case '-': SymCode = symMinus; break;
            case ' ': SymCode = symEmpty; break;
            case 'E': SymCode = sym_E; break;
            case 'H': SymCode = sym_H; break;
            case 'L': SymCode = sym_L; break;
            case 'P': SymCode = sym_P; break;
            case '0':
            case 'O':
            case 'D': SymCode = sym_0; break;
            case '1': SymCode = sym_1; break;
            case '2': SymCode = sym_2; break;
            case '3': SymCode = sym_3; break;
            case '4': SymCode = sym_4; break;
            case '5':
            case 'S': SymCode = sym_5; break;
            case '6': SymCode = sym_6; break;
            case '7': SymCode = sym_7; break;
            case '8':
            case 'B': SymCode = sym_8; break;
            case '9': SymCode = sym_9; break;
            default : SymCode = symEmpty; break;
        }
        Str++;
        if (*Str == '.' or *Str == ',') {
            SymCode |= symPoint;
            Str++;
        }
        WriteBuffer(Seg, SymCode);
        Seg --;
    }
#elif defined dmNoDecode
    while (*Str != 0) {
        switch (*Str) {
            case '-': SymCode = symMinus; break;
            case '_': SymCode = symSpace; break;
            case ' ': SymCode = symEmpty; break;
            case 'A':
            case 'R': SymCode = sym_A; break;
            case 'b': SymCode = sym_b; break;
            case 'C': SymCode = sym_C; break;
            case 'c': SymCode = sym_c; break;
            case 'd': SymCode = sym_d; break;
            case 'E': SymCode = sym_E; break;
            case 'F': SymCode = sym_F; break;
            case 'G': SymCode = sym_G; break;
            case 'H': SymCode = sym_H; break;
            case 'h': SymCode = sym_h; break;
            case 'I': SymCode = sym_I; break;
            case 'J': SymCode = sym_J; break;
            case 'L': SymCode = sym_L; break;
            case 'n': SymCode = sym_n; break;
            case 'o': SymCode = sym_o; break;
            case 'P': SymCode = sym_P; break;
            case 'r': SymCode = sym_r; break;
            case 't': SymCode = sym_t; break;
            case 'U': SymCode = sym_U; break;
            case 'u': SymCode = sym_u; break;
            case 'Y': SymCode = sym_Y; break;
            case '0':
            case 'O':
            case 'D': SymCode = sym_0; break;
            case '1': SymCode = sym_1; break;
            case '2': SymCode = sym_2; break;
            case '3': SymCode = sym_3; break;
            case '4': SymCode = sym_4; break;
            case '5':
            case 'S': SymCode = sym_5; break;
            case '6': SymCode = sym_6; break;
            case '7': SymCode = sym_7; break;
            case '8':
            case 'B': SymCode = sym_8; break;
            case '9': SymCode = sym_9; break;
            default : SymCode = symEmpty; break;
        }
        Str++;
        if (*Str == '.' or *Str == ',') {
            SymCode |= symPoint;
            Str++;
        }
        WriteBuffer(Seg, SymCode);
        Seg --;
    }
#endif
}

void Disp_MAX7219_t::Print(const char *Str, int32_t Data, uint8_t Decade) {
    uint8_t SymCode;
    uint8_t Seg = 1;
    bool negative = false;
    Decade++;
    ClearBuffer();

    // PrintChar
    PrintChar(Str);

    // PrintNumber
    if (Data != INT32_MAX) {
        if (Data < 0) { negative = true; Data = -Data; }
        do {
            uint32_t temp = Data/10;
#if defined dmBCDcode
            SymCode = Data-(temp*10);
#elif defined dmNoDecode
            SymCode = SymCodeNum[Data-(temp*10)];
#endif
            if (Seg == Decade and Seg > 1) SymCode |= symPoint;
            WriteBuffer(Seg, SymCode);
            Seg++;
            Data = temp;
        } while (Data > 0);
        while (Seg < Decade) { WriteBuffer(Seg, sym_0); Seg++; }  // дописать нули
        if (Seg == Decade) { WriteBuffer(Seg, sym_0 | symPoint); Seg++; };
        if (negative) WriteBuffer(Seg, symMinus);
    }

    // Send Data
    for (uint8_t Seg=1; Seg<=SegCount; Seg++) {
        if ( !(chVTIsArmed(&TmrBlink) and Seg == SegBlinking) )       // если текущий сегмент не мигает
            WriteRegister(Seg, DispBuffer[Seg-1]);
    }
}
#if !defined dmBCDcode
void  Disp_MAX7219_t::PrintHex(const char *Str, int32_t HexData) {
    uint8_t SymCode;
    uint8_t Seg = 1;
    bool negative = false;
    ClearBuffer();

    // PrintChar
    PrintChar(Str);

    // PrintNumber
    if (HexData != INT32_MAX) {
        if (HexData < 0) { negative = true; HexData = -HexData; }
        do {
            SymCode = SymCodeNum[HexData & 0b1111];
            WriteBuffer(Seg, SymCode);
            Seg++;
            HexData >>= 4;
        } while (HexData > 0);
    }
    if (negative) WriteBuffer(Seg, symMinus);

    // Send Data
    for (uint8_t Seg=1; Seg<=SegCount; Seg++) {
        if ( !(chVTIsArmed(&TmrBlink) and Seg == SegBlinking) )       // если текущий сегмент не мигает
            WriteRegister(Seg, DispBuffer[Seg-1]);
    }
}
#endif

