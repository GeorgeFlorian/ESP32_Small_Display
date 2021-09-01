#include "SmartMatrix3.h"

// use this for all linear panels (e.g. panels that draw a single left-to-right line for each RGB channel)
const PanelMappingEntry defaultPanelMap[] =
{
    {0, 0, DEFAULT_PANEL_WIDTH_FOR_LINEAR_PANELS},
    {0, 0, 0}   // last entry is all zeros
};

const PanelMappingEntry panelMap32x16Mod2[] =
{
    {0, 71,  -8},
    {0, 87,  -8},
    {0, 103, -8},
    {0, 119, -8},
    {2, 72,   8},
    {2, 88,   8},
    {2, 104,  8},
    {2, 120,  8},
    {4,  7,  -8},
    {4, 23,  -8},
    {4, 39,  -8},
    {4, 55,  -8},
    {6,  8,   8},
    {6, 24,   8},
    {6, 40,   8},
    {6, 56,   8},
    {0, 0, 0}   // last entry is all zeros
};

const PanelMappingEntry panelMapHub12_32x16Mod4[] =
{
    {0,  24,  8},
    {0,  56,  8},
    {0,  88,  8},
    {0,  120, 8},
    {4,  16,  8},
    {4,  48,  8},
    {4,  80,  8},
    {4,  112, 8}, 
    {8,  8,   8},
    {8,  40,  8},
    {8,  72,  8},
    {8,  104, 8},
    {12, 0,   8},
    {12, 32,  8},
    {12, 64,  8},
    {12, 96,  8},
    {0, 0, 0}   // last entry is all zeros
};

const PanelMappingEntry panelMap32x16Mod4[] =
{
    {0, 0,  8},
    {0, 16, 8},
    {0, 32, 8},
    {0, 48, 8},
    {4, 8,  8},
    {4, 24, 8},
    {4, 40, 8},
    {4, 56, 8}, 
    {0, 0, 0}   // last entry is all zeros
};

const PanelMappingEntry panelMap32x16Mod4V2[] =
{
    {0, 15, -8},
    {0, 31, -8},
    {0, 47, -8},
    {0, 63, -8},
    {4, 0,   8},
    {4, 16,  8},
    {4, 32,  8},
    {4, 48,  8}, 
    {0, 0, 0}   // last entry is all zeros
};
const PanelMappingEntry panelMapL540_2727_64x32[] = {
    {0,0,16},
    {0,48,32},
    {0,112,16},
    {8,16,32},
    {8,80,32},
    {0,0,0}
};

const PanelMappingEntry panelMapP5_2727_64x32[] = {
    {0,64,64},
    {8,0,64},
    {0,0,0}
};

const PanelMappingEntry panelMapP5_2727_128x32[] = {
    {0,192,64},
    {0,64,64},
    {8,128,64},
    {8,0,64},
    {0,0,0}
};

const PanelMappingEntry panelMapP5_2727_128x64[] = {
    {0,  448, 64},
    {0,  320, 64},
    {8,  384, 64},
    {8,  256, 64},
    {32, 192, 64},
    {32, 64,  64},
    {40, 128, 64},
    {40, 0,   64},
    {0,  0,   0}
};

const PanelMappingEntry * getMultiRowRefreshPanelMap(unsigned char panelType) {
    switch(panelType) {
        case SMARTMATRIX_HUB75_16ROW_32COL_MOD2SCAN:
            return panelMap32x16Mod2;
        case SMARTMATRIX_HUB12_16ROW_32COL_MOD4SCAN:
            return panelMapHub12_32x16Mod4;
        case SMARTMATRIX_HUB75_16ROW_32COL_MOD4SCAN:
            return panelMap32x16Mod4;
        case SMARTMATRIX_HUB75_16ROW_32COL_MOD4SCAN_V2:
            return panelMap32x16Mod4V2;
        case SMARTMATRIX_HUB75_32ROW_64COL_MOD8SCAN_L540_2727:
            return panelMapL540_2727_64x32;
        case SMARTMATRIX_HUB75_32ROW_64COL_MOD8SCAN_P5_2727:
            return panelMapP5_2727_64x32;
        case SMARTMATRIX_HUB75_32ROW_128COL_MOD8SCAN_P5_2727:
            return panelMapP5_2727_128x32;
        case SMARTMATRIX_HUB75_64ROW_128COL_MOD8SCAN_P5_2727:
            return panelMapP5_2727_128x64;
        default:
            return defaultPanelMap;
    }
}
