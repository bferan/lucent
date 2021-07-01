#pragma once

#define LC_KEY_UNKNOWN            -1

#define LC_KEY_SPACE              32
#define LC_KEY_APOSTROPHE         39  /* ' */
#define LC_KEY_COMMA              44  /* , */
#define LC_KEY_MINUS              45  /* - */
#define LC_KEY_PERIOD             46  /* . */
#define LC_KEY_SLASH              47  /* / */
#define LC_KEY_0                  48
#define LC_KEY_1                  49
#define LC_KEY_2                  50
#define LC_KEY_3                  51
#define LC_KEY_4                  52
#define LC_KEY_5                  53
#define LC_KEY_6                  54
#define LC_KEY_7                  55
#define LC_KEY_8                  56
#define LC_KEY_9                  57
#define LC_KEY_SEMICOLON          59  /* ; */
#define LC_KEY_EQUAL              61  /* = */
#define LC_KEY_A                  65
#define LC_KEY_B                  66
#define LC_KEY_C                  67
#define LC_KEY_D                  68
#define LC_KEY_E                  69
#define LC_KEY_F                  70
#define LC_KEY_G                  71
#define LC_KEY_H                  72
#define LC_KEY_I                  73
#define LC_KEY_J                  74
#define LC_KEY_K                  75
#define LC_KEY_L                  76
#define LC_KEY_M                  77
#define LC_KEY_N                  78
#define LC_KEY_O                  79
#define LC_KEY_P                  80
#define LC_KEY_Q                  81
#define LC_KEY_R                  82
#define LC_KEY_S                  83
#define LC_KEY_T                  84
#define LC_KEY_U                  85
#define LC_KEY_V                  86
#define LC_KEY_W                  87
#define LC_KEY_X                  88
#define LC_KEY_Y                  89
#define LC_KEY_Z                  90
#define LC_KEY_LEFT_BRACKET       91  /* [ */
#define LC_KEY_BACKSLASH          92  /* \ */
#define LC_KEY_RIGHT_BRACKET      93  /* ] */
#define LC_KEY_GRAVE_ACCENT       96  /* ` */
#define LC_KEY_WORLD_1            161 /* non-US #1 */
#define LC_KEY_WORLD_2            162 /* non-US #2 */

#define LC_KEY_ESCAPE             256
#define LC_KEY_ENTER              257
#define LC_KEY_TAB                258
#define LC_KEY_BACKSPACE          259
#define LC_KEY_INSERT             260
#define LC_KEY_DELETE             261
#define LC_KEY_RIGHT              262
#define LC_KEY_LEFT               263
#define LC_KEY_DOWN               264
#define LC_KEY_UP                 265
#define LC_KEY_PAGE_UP            266
#define LC_KEY_PAGE_DOWN          267
#define LC_KEY_HOME               268
#define LC_KEY_END                269
#define LC_KEY_CAPS_LOCK          280
#define LC_KEY_SCROLL_LOCK        281
#define LC_KEY_NUM_LOCK           282
#define LC_KEY_PRINT_SCREEN       283
#define LC_KEY_PAUSE              284
#define LC_KEY_F1                 290
#define LC_KEY_F2                 291
#define LC_KEY_F3                 292
#define LC_KEY_F4                 293
#define LC_KEY_F5                 294
#define LC_KEY_F6                 295
#define LC_KEY_F7                 296
#define LC_KEY_F8                 297
#define LC_KEY_F9                 298
#define LC_KEY_F10                299
#define LC_KEY_F11                300
#define LC_KEY_F12                301
#define LC_KEY_F13                302
#define LC_KEY_F14                303
#define LC_KEY_F15                304
#define LC_KEY_F16                305
#define LC_KEY_F17                306
#define LC_KEY_F18                307
#define LC_KEY_F19                308
#define LC_KEY_F20                309
#define LC_KEY_F21                310
#define LC_KEY_F22                311
#define LC_KEY_F23                312
#define LC_KEY_F24                313
#define LC_KEY_F25                314
#define LC_KEY_KP_0               320
#define LC_KEY_KP_1               321
#define LC_KEY_KP_2               322
#define LC_KEY_KP_3               323
#define LC_KEY_KP_4               324
#define LC_KEY_KP_5               325
#define LC_KEY_KP_6               326
#define LC_KEY_KP_7               327
#define LC_KEY_KP_8               328
#define LC_KEY_KP_9               329
#define LC_KEY_KP_DECIMAL         330
#define LC_KEY_KP_DIVIDE          331
#define LC_KEY_KP_MULTIPLY        332
#define LC_KEY_KP_SUBTRACT        333
#define LC_KEY_KP_ADD             334
#define LC_KEY_KP_ENTER           335
#define LC_KEY_KP_EQUAL           336
#define LC_KEY_LEFT_SHIFT         340
#define LC_KEY_LEFT_CONTROL       341
#define LC_KEY_LEFT_ALT           342
#define LC_KEY_LEFT_SUPER         343
#define LC_KEY_RIGHT_SHIFT        344
#define LC_KEY_RIGHT_CONTROL      345
#define LC_KEY_RIGHT_ALT          346
#define LC_KEY_RIGHT_SUPER        347
#define LC_KEY_MENU               348

#define LC_KEY_LAST               LC_KEY_MENU


#define LC_MOD_SHIFT           0x0001
#define LC_MOD_CONTROL         0x0002
#define LC_MOD_ALT             0x0004
#define LC_MOD_SUPER           0x0008
#define LC_MOD_CAPS_LOCK       0x0010
#define LC_MOD_NUM_LOCK        0x0020


#define LC_MOUSE_BUTTON_1         0
#define LC_MOUSE_BUTTON_2         1
#define LC_MOUSE_BUTTON_3         2
#define LC_MOUSE_BUTTON_4         3
#define LC_MOUSE_BUTTON_5         4
#define LC_MOUSE_BUTTON_6         5
#define LC_MOUSE_BUTTON_7         6
#define LC_MOUSE_BUTTON_8         7
#define LC_MOUSE_BUTTON_LAST      LC_MOUSE_BUTTON_8
#define LC_MOUSE_BUTTON_LEFT      LC_MOUSE_BUTTON_1
#define LC_MOUSE_BUTTON_RIGHT     LC_MOUSE_BUTTON_2
#define LC_MOUSE_BUTTON_MIDDLE    LC_MOUSE_BUTTON_3