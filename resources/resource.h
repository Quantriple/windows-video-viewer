#pragma once

// 图标 ID (当前未使用图标资源)

// 菜单资源 ID
#define IDR_MAIN_MENU                   201
#define IDR_ACCELERATOR                 202

// 对话框资源 ID
#define IDD_ABOUT                       2001
#define IDD_SETTINGS                    2002
#define IDD_SEARCH                      2003

// 字符串资源 ID
#define IDS_APP_TITLE                   401
#define IDS_MAIN_WINDOW_CLASS           402
#define IDS_SCANNING                    403
#define IDS_READY                       404
#define IDS_FILES_FOUND                 405
#define IDS_ERROR_TITLE                 406
#define IDS_INFO_TITLE                  407
#define IDS_CONFIRM_TITLE               408
#define IDS_DELETE_CONFIRM              409
#define IDS_PLAY_ERROR                  410
#define IDS_DELETE_ERROR                411
#define IDS_SCAN_ERROR                  412

// 菜单项 ID
#define ID_FILE_SCAN                    1001
#define ID_FILE_REFRESH                 1002
#define ID_FILE_EXIT                    1003
#define ID_VIEW_LIST                    1004
#define ID_VIEW_DETAILS                 1005
#define ID_VIEW_THUMBNAILS              1006
#define ID_TOOLS_SEARCH                 1007
#define ID_TOOLS_SETTINGS               1008
#define ID_HELP_ABOUT                   1009

// 右键菜单 ID 在 MainWindow.h 中定义为枚举

// 控件 ID
#define IDC_STATIC                      -1
#define IDC_SCAN_PATHS                  3001
#define IDC_ADD_PATH                    3002
#define IDC_REMOVE_PATH                 3003
#define IDC_AUTO_REFRESH                3004
#define IDC_PLAYER_PATH                 3005
#define IDC_BROWSE_PLAYER               3006
#define IDC_USE_DEFAULT_PLAYER          3007
#define IDC_SHOW_THUMBNAILS             3008
#define IDC_SORT_BY                     3009
#define IDC_SORT_ASCENDING              3010
#define IDC_SORT_DESCENDING             3011
#define IDC_CACHE_PATH                  3012
#define IDC_BROWSE_CACHE                3013
#define IDAPPLY                         3014

// 搜索对话框控件 ID
#define IDC_SEARCH_TEXT                 3015
#define IDC_SEARCH_BUTTON               3016
#define IDC_CLEAR_SEARCH                3017
#define IDC_SEARCH_RESULTS              3018

// 资源编辑器生成的符号
#ifdef APSTUDIO_INVOKED
#ifndef APSTUDIO_READONLY_SYMBOLS
#define _APS_NEXT_RESOURCE_VALUE        501
#define _APS_NEXT_COMMAND_VALUE         3001
#define _APS_NEXT_CONTROL_VALUE         3001
#define _APS_NEXT_SYMED_VALUE           501
#endif
#endif