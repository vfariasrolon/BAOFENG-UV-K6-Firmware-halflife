#ifndef __ICONS_H
#define __ICONS_H

#include "PublType.h"

typedef enum {
    ICON_VFO = 0,
    ICON_MR,
    ICON_FM,
    ICON_TX,
    ICON_RX,
    ICON_SCAN,
    ICON_BAT_0,
    ICON_BAT_25,
    ICON_BAT_50,
    ICON_BAT_75,
    ICON_BAT_100,
    ICON_SKULL,
    ICON_FLAG_WHITE,
    ICON_FLAG_BLACK,
    ICON_ENVELOPE,
    ICON_MAP,
    ICON_LAMBDA,
    ICON_MAX
} IconID_Enum;

void LCD_DrawIcon(IconID_Enum iconID, U8 x, U8 y);

#endif /* __ICONS_H */
