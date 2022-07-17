//---------------------------------------------------------------------------
//
//	X68000 EMULATOR "XM6"
//
//	Copyright (C) 2001-2006 ＰＩ．(ytanaka@ipc-tokai.or.jp)
//	[ MFC 識別子定義 ]
//
//---------------------------------------------------------------------------

#if defined(_WIN32)

#if !defined(mfc_res_h)
#define mfc_res_h

// 特殊コマンド
#define IDM_STDWIN						4096
#define IDM_US_STDWIN					9096

// メニュー、アクセラレータ、タイマー、アイコン、ダイアログ
#define IDI_APPICON						99
#define IDI_XICON						100
#define IDR_ACCELERATOR					101
#define IDR_DISMENU						102
#define IDR_MEMORYMENU					103
#define IDD_ABOUTDLG					104
#define IDR_MENU						105
#define IDD_ADDRDLG						106
#define IDB_X68K						107
#define IDB_CRT							108
#define IDR_MENUPOPUP					109
#define IDD_BASICPAGE					110
#define IDD_SOUNDPAGE					111
#define IDD_VOLPAGE						112
#define IDR_PCGMENU						113
#define IDR_REGMENU						114
#define IDD_REGDLG						115
#define IDD_DATADLG						116
#define IDR_BREAKPMENU					117
#define IDD_MISCPAGE					118
#define IDD_KBDPAGE						119
#define IDD_MOUSEPAGE					120
#define IDD_KBDMAPDLG					121
#define IDD_KEYINDLG					122
#define IDD_PORTPAGE					123
#define IDD_ALTERPAGE					124
#define IDD_FDIDLG						125
#define IDD_SASIPAGE					126
#define IDD_DISKDLG						127
#define IDD_DMAKEDLG					128
#define IDD_SXSIPAGE					129
#define IDD_MIDIPAGE					130
#define IDD_TKEYPAGE					131
#define IDD_JOYPAGE						132
#define IDD_JOYDETDLG					133
#define IDD_BTNSETPAGE					134
#define IDD_TRAPDLG						135
#define IDD_SCSIPAGE					136
#define IDD_RESUMEPAGE					137

// メニュー、アクセラレータ、タイマー、アイコン、ダイアログ(US)
#define IDR_US_DISMENU					152
#define IDR_US_MEMORYMENU				153
#define IDD_US_ABOUTDLG					154
#define IDR_US_MENU						155
#define IDD_US_ADDRDLG					156
#define IDR_US_MENUPOPUP				159
#define IDD_US_BASICPAGE				160
#define IDD_US_SOUNDPAGE				161
#define IDD_US_VOLPAGE					162
#define IDD_US_REGDLG					165
#define IDD_US_DATADLG					166
#define IDR_US_BREAKPMENU				167
#define IDD_US_MISCPAGE					168
#define IDD_US_KBDPAGE					169
#define IDD_US_MOUSEPAGE				170
#define IDD_US_KBDMAPDLG				171
#define IDD_US_KEYINDLG					172
#define IDD_US_PORTPAGE					173
#define IDD_US_ALTERPAGE				174
#define IDD_US_FDIDLG					175
#define IDD_US_SASIPAGE					176
#define IDD_US_DISKDLG					177
#define IDD_US_DMAKEDLG					178
#define IDD_US_SXSIPAGE					179
#define IDD_US_MIDIPAGE					180
#define IDD_US_TKEYPAGE					181
#define IDD_US_JOYPAGE					182
#define IDD_US_JOYDETDLG				183
#define IDD_US_BTNSETPAGE				184
#define IDD_US_TRAPDLG					185
#define IDD_US_SCSIPAGE					186
#define IDD_US_RESUMEPAGE				187

// バージョン情報ダイアログ
#define IDC_ABOUT_ICON					1000
#define IDC_ABOUT_XM6					1001
#define IDC_ABOUT_VER					1002
#define IDC_ABOUT_PI					1003
#define IDC_ABOUT_URL					1004
#define IDC_ABOUT_CORLETT				1005
#define IDC_ABOUT_CISC					1006

// アドレス入力ダイアログ
#define IDC_ADDR_ADDRL					1010
#define IDC_ADDR_ADDRE					1011

// 基本設定ページ
#define IDC_BASIC_HELP					1100
#define IDC_BASIC_CLOCKG				1101
#define IDC_BASIC_MEMORYG				1102
#define IDC_BASIC_MEMSWB				1103
#define IDC_BASIC_MEMORYC				1104
#define IDC_BASIC_MEMORYL				1105
#define IDC_BASIC_CLOCKL				1106
#define IDC_BASIC_CLOCKC				1107
#define IDC_BASIC_CPUFULLB				1108
#define IDC_BASIC_ALLFULLB				1109
#define IDS_BASIC_CLOCK0				1110
#define IDS_BASIC_CLOCK1				1111
#define IDS_BASIC_CLOCK2				1112
#define IDS_BASIC_CLOCK3				1113
#define IDS_BASIC_CLOCK4				1114
#define IDS_BASIC_CLOCK5				1115
#define IDS_BASIC_MEMORY0				1116
#define IDS_BASIC_MEMORY1				1117
#define IDS_BASIC_MEMORY2				1118
#define IDS_BASIC_MEMORY3				1119
#define IDS_BASIC_MEMORY4				1120
#define IDS_BASIC_MEMORY5				1121

// 基本設定ページ(US)
#define IDC_US_BASIC_CLOCKG				6101
#define IDC_US_BASIC_MEMORYG			6102
#define IDC_US_BASIC_MEMSWB				6103
#define IDC_US_BASIC_MEMORYC			6104
#define IDC_US_BASIC_MEMORYL			6105
#define IDC_US_BASIC_CLOCKL				6106
#define IDC_US_BASIC_CLOCKC				6107
#define IDC_US_BASIC_CPUFULLB			6108
#define IDC_US_BASIC_ALLFULLB			6109
#define IDS_US_BASIC_CLOCK0				6110
#define IDS_US_BASIC_CLOCK1				6111
#define IDS_US_BASIC_CLOCK2				6112
#define IDS_US_BASIC_CLOCK3				6113
#define IDS_US_BASIC_CLOCK4				6114
#define IDS_US_BASIC_CLOCK5				6115
#define IDS_US_BASIC_MEMORY0			6116
#define IDS_US_BASIC_MEMORY1			6117
#define IDS_US_BASIC_MEMORY2			6118
#define IDS_US_BASIC_MEMORY3			6119
#define IDS_US_BASIC_MEMORY4			6120
#define IDS_US_BASIC_MEMORY5			6121

// サウンドページ
#define IDC_SOUND_DEVICEG				1200
#define IDC_SOUND_DEVICEC				1201
#define IDC_SOUND_RATEG					1202
#define IDC_SOUND_RATE0					1203
#define IDC_SOUND_RATE1					1204
#define IDC_SOUND_RATE2					1205
#define IDC_SOUND_RATE3					1206
#define IDC_SOUND_RATE4					1207
#define IDC_SOUND_BUFFERG				1208
#define IDC_SOUND_BUF1L					1209
#define IDC_SOUND_BUF1E					1210
#define IDC_SOUND_BUF1S					1211
#define IDC_SOUND_BUF1MS				1212
#define IDC_SOUND_BUF2L					1213
#define IDC_SOUND_BUF2E					1214
#define IDC_SOUND_BUF2S					1215
#define IDC_SOUND_BUF2MS				1216
#define IDC_SOUND_OPTIONG				1217
#define IDC_SOUND_INTERP				1218
#define IDC_SOUND_HELP					1219

// サウンドページ(US)
#define IDC_US_SOUND_DEVICEG			6200
#define IDC_US_SOUND_DEVICEC			6201
#define IDC_US_SOUND_RATEG				6202
#define IDC_US_SOUND_RATE0				6203
#define IDC_US_SOUND_RATE1				6204
#define IDC_US_SOUND_RATE2				6205
#define IDC_US_SOUND_RATE3				6206
#define IDC_US_SOUND_RATE4				6207
#define IDC_US_SOUND_BUFFERG			6208
#define IDC_US_SOUND_BUF1L				6209
#define IDC_US_SOUND_BUF1E				6210
#define IDC_US_SOUND_BUF1S				6211
#define IDC_US_SOUND_BUF1MS				6212
#define IDC_US_SOUND_BUF2L				6213
#define IDC_US_SOUND_BUF2E				6214
#define IDC_US_SOUND_BUF2S				6215
#define IDC_US_SOUND_BUF2MS				6216
#define IDC_US_SOUND_OPTIONG			6217
#define IDC_US_SOUND_INTERP				6218
#define IDC_US_SOUND_HELP				6219

// 音量ページ
#define IDC_VOL_OUTG					1300
#define IDC_VOL_VOLL					1301
#define IDC_VOL_VOLS					1302
#define IDC_VOL_VOLN					1303
#define IDC_VOL_MASTERL					1304
#define IDC_VOL_MASTERS					1305
#define IDC_VOL_MASTERN					1306
#define IDC_VOL_SEPL					1307
#define IDC_VOL_SEPS					1308
#define IDC_VOL_SEPN					1309
#define IDC_VOL_GENG					1310
#define IDC_VOL_FML						1311
#define IDC_VOL_FMC						1312
#define IDC_VOL_FMS						1313
#define IDC_VOL_FMN						1314
#define IDC_VOL_ADPCML					1315
#define IDC_VOL_ADPCMC					1316
#define IDC_VOL_ADPCMS					1317
#define IDC_VOL_ADPCMN					1318
#define IDC_VOL_HELP					1319

// 音量ページ(US)
#define IDC_US_VOL_OUTG					6300
#define IDC_US_VOL_VOLL					6301
#define IDC_US_VOL_VOLS					6302
#define IDC_US_VOL_VOLN					6303
#define IDC_US_VOL_MASTERL				6304
#define IDC_US_VOL_MASTERS				6305
#define IDC_US_VOL_MASTERN				6306
#define IDC_US_VOL_SEPL					6307
#define IDC_US_VOL_SEPS					6308
#define IDC_US_VOL_SEPN					6309
#define IDC_US_VOL_GENG					6310
#define IDC_US_VOL_FML					6311
#define IDC_US_VOL_FMC					6312
#define IDC_US_VOL_FMS					6313
#define IDC_US_VOL_FMN					6314
#define IDC_US_VOL_ADPCML				6315
#define IDC_US_VOL_ADPCMC				6316
#define IDC_US_VOL_ADPCMS				6317
#define IDC_US_VOL_ADPCMN				6318
#define IDC_US_VOL_HELP					6319

// その他ページ
#define IDC_MISC_HELP					1400
#define IDC_MISC_OPTG					1401
#define IDC_MISC_FDSPEED				1402
#define IDC_MISC_FDLED					1403
#define IDC_MISC_POPUP					1404
#define IDC_MISC_AUTOMOUSE				1405
#define IDC_MISC_POWEROFF				1406
#define IDC_BUSCAR                      1407
#define IDC_EDIT1                       1408

// その他ページ(US)
#define IDC_US_MISC_HELP				6400
#define IDC_US_MISC_OPTG				6401
#define IDC_US_MISC_FDSPEED				6402
#define IDC_US_MISC_FDLED				6403
#define IDC_US_MISC_POPUP				6404
#define IDC_US_MISC_AUTOMOUSE			6405
#define IDC_US_MISC_POWEROFF			6406

// キーボードページ
#define IDC_KBD_HELP					1500
#define IDC_KBD_MAPG					1501
#define IDC_KBD_MAPL					1502
#define IDC_KBD_EDITB					1503
#define IDC_KBD_DEFB					1504
#define IDC_KBD_NOCONB					1505

// キーボードページ(US)
#define IDC_US_KBD_HELP					6500
#define IDC_US_KBD_MAPG					6501
#define IDC_US_KBD_MAPL					6502
#define IDC_US_KBD_EDITB				6503
#define IDC_US_KBD_DEFB					6504
#define IDC_US_KBD_NOCONB				6505

// マウスページ
#define IDC_MOUSE_HELP					1600
#define IDC_MOUSE_SPEEDG				1601
#define IDC_MOUSE_SLIDER				1602
#define IDC_MOUSE_PARS					1603
#define IDC_MOUSE_PORTG					1604
#define IDC_MOUSE_FPORT					1605
#define IDC_MOUSE_KPORT					1606
#define IDC_MOUSE_NPORT					1607
#define IDC_MOUSE_OPTG					1608
#define IDC_MOUSE_SWAPB					1609
#define IDC_MOUSE_MIDB					1610
#define IDC_MOUSE_TBG					1611

// マウスページ(US)
#define IDC_US_MOUSE_HELP				6600
#define IDC_US_MOUSE_SPEEDG				6601
#define IDC_US_MOUSE_SLIDER				6602
#define IDC_US_MOUSE_PARS				6603
#define IDC_US_MOUSE_PORTG				6604
#define IDC_US_MOUSE_FPORT				6605
#define IDC_US_MOUSE_KPORT				6606
#define IDC_US_MOUSE_NPORT				6607
#define IDC_US_MOUSE_OPTG				6608
#define IDC_US_MOUSE_SWAPB				6609
#define IDC_US_MOUSE_MIDB				6610
#define IDC_US_MOUSE_TBG				6611

// キーマップダイアログ、キー入力ダイアログ
#define IDC_KBDMAP_DISP					1700
#define IDC_KBDMAP_STAT					1701
#define IDC_KEYIN_LABEL					1702
#define IDC_KEYIN_STATIC				1703
#define IDC_KEYIN_KEY					1704

// ポートページ
#define IDC_PORT_HELP					1800
#define IDC_PORT_COMG					1801
#define IDC_PORT_COML					1802
#define IDC_PORT_COMC					1803
#define IDC_PORT_RECVL					1804
#define IDC_PORT_RECVE					1805
#define IDC_PORT_BAUDRATE				1806
#define IDC_PORT_LPTG					1807
#define IDC_PORT_LPTL					1808
#define IDC_PORT_LPTC					1809
#define IDC_PORT_SENDL					1810
#define IDC_PORT_SENDE					1811

// ポートページ(US)
#define IDC_US_PORT_HELP				6800
#define IDC_US_PORT_COMG				6801
#define IDC_US_PORT_COML				6802
#define IDC_US_PORT_COMC				6803
#define IDC_US_PORT_RECVL				6804
#define IDC_US_PORT_RECVE				6805
#define IDC_US_PORT_BAUDRATE			6806
#define IDC_US_PORT_LPTG				6807
#define IDC_US_PORT_LPTL				6808
#define IDC_US_PORT_LPTC				6809
#define IDC_US_PORT_SENDL				6810
#define IDC_US_PORT_SENDE				6811

// 改造ページ
#define IDC_ALTER_HELP					1900
#define IDC_ALTER_ALTERG				1901
#define IDC_ALTER_SRAM					1902
#define IDC_ALTER_SCC					1903
#define IDC_ALTER_POWERLED				1904
#define IDC_ALTER_2DD					1905
#define IDC_ALTER_SASIG					1906
#define IDC_ALTER_PARITY				1907

// 改造ページ(US)
#define IDC_US_ALTER_HELP				6900
#define IDC_US_ALTER_ALTERG				6901
#define IDC_US_ALTER_SRAM				6902
#define IDC_US_ALTER_SCC				6903
#define IDC_US_ALTER_POWERLED			6904
#define IDC_US_ALTER_2DD				6905
#define IDC_US_ALTER_SASIG				6906
#define IDC_US_ALTER_PARITY				6907

// SASIページ
#define IDC_SASI_GROUP					2000
#define IDC_SASI_DRIVEL					2001
#define IDC_SASI_DRIVEE					2002
#define IDC_SASI_DRIVES					2003
#define IDC_SASI_MEMSWB					2004
#define IDC_SASI_LIST					2005
#define IDC_SASI_HELP					2006

// SASIページ(US)
#define IDC_US_SASI_GROUP				7000
#define IDC_US_SASI_DRIVEL				7001
#define IDC_US_SASI_DRIVEE				7002
#define IDC_US_SASI_DRIVES				7003
#define IDC_US_SASI_MEMSWB				7004
#define IDC_US_SASI_LIST				7005
#define IDC_US_SASI_HELP				7006

// SxSIページ
#define IDC_SXSI_GROUP					2100
#define IDC_SXSI_DRIVEL					2101
#define IDC_SXSI_DRIVEE					2102
#define IDC_SXSI_MOCHECK				2103
#define IDC_SXSI_DRIVES					2104
#define IDC_SXSI_LIST					2105
#define IDC_SXSI_HELP					2106

// SxSIページ(US)
#define IDC_US_SXSI_GROUP				7100
#define IDC_US_SXSI_DRIVEL				7101
#define IDC_US_SXSI_DRIVEE				7102
#define IDC_US_SXSI_MOCHECK				7103
#define IDC_US_SXSI_DRIVES				7104
#define IDC_US_SXSI_LIST				7105
#define IDC_US_SXSI_HELP				7106

// MIDIページ
#define IDC_MIDI_BIDG					2200
#define IDC_MIDI_BID0					2201
#define IDC_MIDI_BID1					2202
#define IDC_MIDI_BID2					2203
#define IDC_MIDI_ILEVELG				2204
#define IDC_MIDI_ILEVEL4				2205
#define IDC_MIDI_ILEVEL2				2206
#define IDC_MIDI_RSTG					2207
#define IDC_MIDI_RSTGM					2208
#define IDC_MIDI_RSTGS					2209
#define IDC_MIDI_RSTXG					2210
#define	IDC_MIDI_RSTLA					2211
#define IDC_MIDI_DEVG					2212
#define IDC_MIDI_INS					2213
#define IDC_MIDI_INC					2214
#define IDC_MIDI_DLYIL					2215
#define IDC_MIDI_DLYIE					2216
#define IDC_MIDI_DLYIS					2217
#define IDC_MIDI_DLYIG					2218
#define IDC_MIDI_OUTS					2219
#define IDC_MIDI_OUTC					2220
#define IDC_MIDI_DLYOL					2221
#define IDC_MIDI_DLYOE					2222
#define IDC_MIDI_DLYOS					2223
#define IDC_MIDI_DLYOG					2224
#define IDC_MIDI_HELP					2225

// MIDIページ(US)
#define IDC_US_MIDI_BIDG				7200
#define IDC_US_MIDI_BID0				7201
#define IDC_US_MIDI_BID1				7202
#define IDC_US_MIDI_BID2				7203
#define IDC_US_MIDI_ILEVELG				7204
#define IDC_US_MIDI_ILEVEL4				7205
#define IDC_US_MIDI_ILEVEL2				7206
#define IDC_US_MIDI_RSTG				7207
#define IDC_US_MIDI_RSTGM				7208
#define IDC_US_MIDI_RSTGS				7209
#define IDC_US_MIDI_RSTXG				7210
#define IDC_US_MIDI_RSTLA				7211
#define IDC_US_MIDI_DEVG				7212
#define IDC_US_MIDI_INS					7213
#define IDC_US_MIDI_INC					7214
#define IDC_US_MIDI_DLYIL				7215
#define IDC_US_MIDI_DLYIE				7216
#define IDC_US_MIDI_DLYIS				7217
#define IDC_US_MIDI_DLYIG				7218
#define IDC_US_MIDI_OUTS				7219
#define IDC_US_MIDI_OUTC				7220
#define IDC_US_MIDI_DLYOL				7221
#define IDC_US_MIDI_DLYOE				7222
#define IDC_US_MIDI_DLYOS				7223
#define IDC_US_MIDI_DLYOG				7224
#define IDC_US_MIDI_HELP				7225

// TrueKeyページ
#define IDC_TKEY_PORTG					2300
#define IDC_TKEY_COML					2301
#define IDC_TKEY_COMC					2302
#define IDC_TKEY_RTS					2303
#define IDC_TKEY_ASSIGNG				2304
#define IDC_TKEY_VMC					2305
#define IDC_TKEY_WINC					2306
#define IDC_TKEY_MAPG					2307
#define IDC_TKEY_LIST					2308
#define IDC_TKEY_HELP					2309

// TrueKeyページ(US)
#define IDC_US_TKEY_PORTG				7300
#define IDC_US_TKEY_COML				7301
#define IDC_US_TKEY_COMC				7302
#define IDC_US_TKEY_RTS					7303
#define IDC_US_TKEY_ASSIGNG				7304
#define IDC_US_TKEY_VMC					7305
#define IDC_US_TKEY_WINC				7306
#define IDC_US_TKEY_MAPG				7307
#define IDC_US_TKEY_LIST				7308
#define IDC_US_TKEY_HELP				7309

// ジョイスティックページ
#define IDC_JOY_PORTG					2400
#define IDC_JOY_PORTL1					2401
#define IDC_JOY_PORTC1					2402
#define IDC_JOY_PORTD1					2403
#define IDC_JOY_PORTL2					2404
#define IDC_JOY_PORTC2					2405
#define IDC_JOY_PORTD2					2406
#define IDC_JOY_DEVG					2407
#define IDC_JOY_DEVLA					2408
#define IDC_JOY_DEVCA					2409
#define IDC_JOY_DEVAA					2410
#define IDC_JOY_DEVLB					2411
#define IDC_JOY_DEVCB					2412
#define IDC_JOY_DEVAB					2413
#define IDC_JOY_DEVLC					2414
#define IDC_JOY_DEVCC					2415
#define IDC_JOY_DEVAC					2416
#define IDC_JOY_HELP					2417

// ジョイスティックページ(US)
#define IDC_US_JOY_PORTG				7400
#define IDC_US_JOY_PORTL1				7401
#define IDC_US_JOY_PORTC1				7402
#define IDC_US_JOY_PORTD1				7403
#define IDC_US_JOY_PORTL2				7404
#define IDC_US_JOY_PORTC2				7405
#define IDC_US_JOY_PORTD2				7406
#define IDC_US_JOY_DEVG					7407
#define IDC_US_JOY_DEVLA				7408
#define IDC_US_JOY_DEVCA				7409
#define IDC_US_JOY_DEVAA				7410
#define IDC_US_JOY_DEVLB				7411
#define IDC_US_JOY_DEVCB				7412
#define IDC_US_JOY_DEVAB				7413
#define IDC_US_JOY_DEVLC				7414
#define IDC_US_JOY_DEVCC				7415
#define IDC_US_JOY_DEVAC				7416
#define IDC_US_JOY_HELP					7417

// ジョイスティック詳細ダイアログ
#define IDC_JOYDET_NAMEL				2420
#define IDC_JOYDET_AXISL				2421
#define IDC_JOYDET_AXISS				2422
#define IDC_JOYDET_BUTTONL				2423
#define IDC_JOYDET_BUTTONS				2424
#define IDC_JOYDET_TYPEL				2425
#define IDC_JOYDET_TYPES				2426
#define IDC_JOYDET_DATASL				2427
#define IDC_JOYDET_DATASS				2428

// ジョイスティックボタン設定ページ
#define IDC_BTNSET_BTNG					2447
#define IDC_BTNSET_BTNL					2448
#define IDC_BTNSET_ASSIGNL				2449
#define IDC_BTNSET_RAPIDL				2450
#define IDC_BTNSET_BTNL1				2451
#define IDC_BTNSET_ASSIGNC1				2452
#define IDC_BTNSET_RAPIDC1				2453
#define IDC_BTNSET_RAPIDL1				2454
#define IDC_BTNSET_BTNL2				2455
#define IDC_BTNSET_ASSIGNC2				2456
#define IDC_BTNSET_RAPIDC2				2457
#define IDC_BTNSET_RAPIDL2				2458
#define IDC_BTNSET_BTNL3				2459
#define IDC_BTNSET_ASSIGNC3				2460
#define IDC_BTNSET_RAPIDC3				2461
#define IDC_BTNSET_RAPIDL3				2462
#define IDC_BTNSET_BTNL4				2463
#define IDC_BTNSET_ASSIGNC4				2464
#define IDC_BTNSET_RAPIDC4				2465
#define IDC_BTNSET_RAPIDL4				2466
#define IDC_BTNSET_BTNL5				2467
#define IDC_BTNSET_ASSIGNC5				2468
#define IDC_BTNSET_RAPIDC5				2469
#define IDC_BTNSET_RAPIDL5				2470
#define IDC_BTNSET_BTNL6				2471
#define IDC_BTNSET_ASSIGNC6				2472
#define IDC_BTNSET_RAPIDC6				2473
#define IDC_BTNSET_RAPIDL6				2474
#define IDC_BTNSET_BTNL7				2475
#define IDC_BTNSET_ASSIGNC7				2476
#define IDC_BTNSET_RAPIDC7				2477
#define IDC_BTNSET_RAPIDL7				2478
#define IDC_BTNSET_BTNL8				2479
#define IDC_BTNSET_ASSIGNC8				2480
#define IDC_BTNSET_RAPIDC8				2481
#define IDC_BTNSET_RAPIDL8				2482
#define IDC_BTNSET_BTNL9				2483
#define IDC_BTNSET_ASSIGNC9				2484
#define IDC_BTNSET_RAPIDC9				2485
#define IDC_BTNSET_RAPIDL9				2486
#define IDC_BTNSET_BTNL10				2487
#define IDC_BTNSET_ASSIGNC10			2488
#define IDC_BTNSET_RAPIDC10				2489
#define IDC_BTNSET_RAPIDL10				2490
#define IDC_BTNSET_BTNL11				2491
#define IDC_BTNSET_ASSIGNC11			2492
#define IDC_BTNSET_RAPIDC11				2493
#define IDC_BTNSET_RAPIDL11				2494
#define IDC_BTNSET_BTNL12				2495
#define IDC_BTNSET_ASSIGNC12			2496
#define IDC_BTNSET_RAPIDC12				2497
#define IDC_BTNSET_RAPIDL12				2498

// ジョイスティックボタン設定ページ(US)
#define IDC_US_BTNSET_BTNG				7447
#define IDC_US_BTNSET_BTNL				7448
#define IDC_US_BTNSET_ASSIGNL			7449
#define IDC_US_BTNSET_RAPIDL			7450
#define IDC_US_BTNSET_BTNL1				7451
#define IDC_US_BTNSET_ASSIGNC1			7452
#define IDC_US_BTNSET_RAPIDC1			7453
#define IDC_US_BTNSET_RAPIDL1			7454
#define IDC_US_BTNSET_BTNL2				7455
#define IDC_US_BTNSET_ASSIGNC2			7456
#define IDC_US_BTNSET_RAPIDC2			7457
#define IDC_US_BTNSET_RAPIDL2			7458
#define IDC_US_BTNSET_BTNL3				7459
#define IDC_US_BTNSET_ASSIGNC3			7460
#define IDC_US_BTNSET_RAPIDC3			7461
#define IDC_US_BTNSET_RAPIDL3			7462
#define IDC_US_BTNSET_BTNL4				7463
#define IDC_US_BTNSET_ASSIGNC4			7464
#define IDC_US_BTNSET_RAPIDC4			7465
#define IDC_US_BTNSET_RAPIDL4			7466
#define IDC_US_BTNSET_BTNL5				7467
#define IDC_US_BTNSET_ASSIGNC5			7468
#define IDC_US_BTNSET_RAPIDC5			7469
#define IDC_US_BTNSET_RAPIDL5			7470
#define IDC_US_BTNSET_BTNL6				7471
#define IDC_US_BTNSET_ASSIGNC6			7472
#define IDC_US_BTNSET_RAPIDC6			7473
#define IDC_US_BTNSET_RAPIDL6			7474
#define IDC_US_BTNSET_BTNL7				7475
#define IDC_US_BTNSET_ASSIGNC7			7476
#define IDC_US_BTNSET_RAPIDC7			7477
#define IDC_US_BTNSET_RAPIDL7			7478
#define IDC_US_BTNSET_BTNL8				7479
#define IDC_US_BTNSET_ASSIGNC8			7480
#define IDC_US_BTNSET_RAPIDC8			7481
#define IDC_US_BTNSET_RAPIDL8			7482
#define IDC_US_BTNSET_BTNL9				7483
#define IDC_US_BTNSET_ASSIGNC9			7484
#define IDC_US_BTNSET_RAPIDC9			7485
#define IDC_US_BTNSET_RAPIDL9			7486
#define IDC_US_BTNSET_BTNL10			7487
#define IDC_US_BTNSET_ASSIGNC10			7488
#define IDC_US_BTNSET_RAPIDC10			7489
#define IDC_US_BTNSET_RAPIDL10			7490
#define IDC_US_BTNSET_BTNL11			7491
#define IDC_US_BTNSET_ASSIGNC11			7492
#define IDC_US_BTNSET_RAPIDC11			7493
#define IDC_US_BTNSET_RAPIDL11			7494
#define IDC_US_BTNSET_BTNL12			7495
#define IDC_US_BTNSET_ASSIGNC12			7496
#define IDC_US_BTNSET_RAPIDC12			7497
#define IDC_US_BTNSET_RAPIDL12			7498

// SCSIページ
#define IDC_SCSI_IFG					2500
#define IDC_SCSI_NONEB					2501
#define IDC_SCSI_INTB					2502
#define IDC_SCSI_EXTB					2503
#define IDC_SCSI_GROUP					2504
#define IDC_SCSI_DRIVEL					2505
#define IDC_SCSI_DRIVEE					2506
#define IDC_SCSI_DRIVES					2507
#define IDC_SCSI_MOCHECK				2508
#define IDC_SCSI_LIST					2509
#define IDC_SCSI_HELP					2510

// SCSIページ(US)
#define IDC_US_SCSI_IFG					7500
#define IDC_US_SCSI_NONEB				7501
#define IDC_US_SCSI_INTB				7502
#define IDC_US_SCSI_EXTB				7503
#define IDC_US_SCSI_GROUP				7504
#define IDC_US_SCSI_DRIVEL				7505
#define IDC_US_SCSI_DRIVEE				7506
#define IDC_US_SCSI_DRIVES				7507
#define IDC_US_SCSI_MOCHECK				7508
#define IDC_US_SCSI_LIST				7509
#define IDC_US_SCSI_HELP				7510

// レジュームページ
#define IDC_RESUME_DISKG				2600
#define IDC_RESUME_FDC					2601
#define IDC_RESUME_MOC					2602
#define IDC_RESUME_CDC					2603
#define IDC_RESUME_STATEG				2604
#define IDC_RESUME_XM6C					2605
#define IDC_RESUME_MISCG				2606
#define IDC_RESUME_SCREENC				2607
#define IDC_RESUME_DIRC					2608
#define IDC_RESUME_HELP					2609

// レジュームページ(US)
#define IDC_US_RESUME_DISKG				7600
#define IDC_US_RESUME_FDC				7601
#define IDC_US_RESUME_MOC				7602
#define IDC_US_RESUME_CDC				7603
#define IDC_US_RESUME_STATEG			7604
#define IDC_US_RESUME_XM6C				7605
#define IDC_US_RESUME_MISCG				7606
#define IDC_US_RESUME_SCREENC			7607
#define IDC_US_RESUME_DIRC				7608
#define IDC_US_RESUME_HELP				7609

// フロッピーディスクイメージダイアログ
#define IDC_FDI_FILEL					3000
#define IDC_FDI_FILEE					3001
#define IDC_FDI_FILEB					3002
#define IDC_FDI_NAMEL					3003
#define IDC_FDI_NAMEE					3004
#define IDC_FDI_PHYG					3005
#define IDC_FDI_2HD						3006
#define IDC_FDI_2HDA					3007
#define IDC_FDI_2HS						3008
#define IDC_FDI_2HC						3009
#define IDC_FDI_2HDE					3010
#define IDC_FDI_2HQ						3011
#define IDC_FDI_2DD						3012
#define IDC_FDI_OS9						3013
#define IDC_FDI_LOGG					3014
#define IDC_FDI_LOGC					3015
#define IDC_FDI_MOUNTG					3016
#define IDC_FDI_DRIVE0					3017
#define IDC_FDI_DRIVE1					3018
#define IDC_FDI_NOTMOUNT				3019

// 大容量ディスクイメージダイアログ
#define IDC_DISK_FILEL					3100
#define IDC_DISK_FILEE					3101
#define IDC_DISK_FILEB					3102
#define IDC_DISK_SIZEG					3103
#define IDC_DISK_SIZE128				3104
#define IDC_DISK_SIZE230				3105
#define IDC_DISK_SIZE540				3106
#define IDC_DISK_SIZE640				3107
#define IDC_DISK_SIZEE					3108
#define IDC_DISK_SIZES					3109
#define IDC_DISK_SIZEL					3110
#define IDC_DISK_LOGG					3111
#define IDC_DISK_LOGI					3112
#define IDC_DISK_LOGS					3113
#define IDC_DISK_LOGN					3114
#define IDC_DISK_LOGC					3115
#define IDC_DISK_OPTG					3116
#define IDC_DISK_MOUNTB					3117

// ディスク作成進行ダイアログ
#define IDC_DMAKE_PROGRESS				3200
#define IDC_DMAKE_STATIC				3201

// trap #0ダイアログ
#define IDC_TRAP_D0L					3300
#define IDC_TRAP_D0E					3301
#define IDC_TRAP_D0S					3302
#define IDC_TRAP_WARN					3304

// ファイルメニュー
#define IDM_OPEN						40000
#define IDM_SAVE						40001
#define IDM_SAVEAS						40002
#define IDM_RESET						40003
#define IDM_INTERRUPT					40004
#define IDM_POWER						40005
#define IDM_XM6_MRU0					40006
#define IDM_XM6_MRU1					40007
#define IDM_XM6_MRU2					40008
#define IDM_XM6_MRU3					40009
#define IDM_XM6_MRU4					40010
#define IDM_XM6_MRU5					40011
#define IDM_XM6_MRU6					40012
#define IDM_XM6_MRU7					40013
#define IDM_XM6_MRU8					40014
#define IDM_EXIT						40015
#define IDM_SAVECUSTOMCONFIG			40016
#define IDM_SAVEGLOBALCONFIG			40017

// フロッピー0メニュー
#define IDM_D0OPEN						40020
#define IDM_D0EJECT 					40021
#define IDM_D0WRITEP					40022
#define IDM_D0FORCE						40023
#define IDM_D0INVALID					40024
#define IDM_D0_MEDIA0					40025
#define IDM_D0_MEDIA1					40026
#define IDM_D0_MEDIA2					40027
#define IDM_D0_MEDIA3					40028
#define IDM_D0_MEDIA4					40029
#define IDM_D0_MEDIA5					40030
#define IDM_D0_MEDIA6					40031
#define IDM_D0_MEDIA7					40032
#define IDM_D0_MEDIA8					40033
#define IDM_D0_MEDIA9					40034
#define IDM_D0_MEDIAA					40035
#define IDM_D0_MEDIAB					40036
#define IDM_D0_MEDIAC					40037
#define IDM_D0_MEDIAD					40038
#define IDM_D0_MEDIAE					40039
#define IDM_D0_MEDIAF					40040
#define IDM_D0_MRU0						40041
#define IDM_D0_MRU1						40042
#define IDM_D0_MRU2						40043
#define IDM_D0_MRU3						40044
#define IDM_D0_MRU4						40045
#define IDM_D0_MRU5						40046
#define IDM_D0_MRU6						40047
#define IDM_D0_MRU7						40048
#define IDM_D0_MRU8						40049

// フロッピー1メニュー
#define IDM_D1OPEN						40050
#define IDM_D1EJECT 					40051
#define IDM_D1WRITEP					40052
#define IDM_D1FORCE						40053
#define IDM_D1INVALID					40054
#define IDM_D1_MEDIA0					40055
#define IDM_D1_MEDIA1					40056
#define IDM_D1_MEDIA2					40057
#define IDM_D1_MEDIA3					40058
#define IDM_D1_MEDIA4					40059
#define IDM_D1_MEDIA5					40060
#define IDM_D1_MEDIA6					40061
#define IDM_D1_MEDIA7					40062
#define IDM_D1_MEDIA8					40063
#define IDM_D1_MEDIA9					40064
#define IDM_D1_MEDIAA					40065
#define IDM_D1_MEDIAB					40066
#define IDM_D1_MEDIAC					40067
#define IDM_D1_MEDIAD					40068
#define IDM_D1_MEDIAE					40069
#define IDM_D1_MEDIAF					40070
#define IDM_D1_MRU0						40071
#define IDM_D1_MRU1						40072
#define IDM_D1_MRU2						40073
#define IDM_D1_MRU3						40074
#define IDM_D1_MRU4						40075
#define IDM_D1_MRU5						40076
#define IDM_D1_MRU6						40077
#define IDM_D1_MRU7						40078
#define IDM_D1_MRU8						40079

// MOメニュー
#define IDM_MOOPEN						40080
#define IDM_MOEJECT						40081
#define IDM_MOWRITEP					40082
#define IDM_MOFORCE						40083
#define IDM_MO_MRU0						40084
#define IDM_MO_MRU1						40085
#define IDM_MO_MRU2						40086
#define IDM_MO_MRU3						40087
#define IDM_MO_MRU4						40088
#define IDM_MO_MRU5						40089
#define IDM_MO_MRU6						40090
#define IDM_MO_MRU7						40091
#define IDM_MO_MRU8						40092

// CDメニュー
#define IDM_CDOPEN						40100
#define IDM_CDEJECT						40101
#define IDM_CDFORCE						40102
#define IDM_CD_MRU0						40103
#define IDM_CD_MRU1						40104
#define IDM_CD_MRU2						40105
#define IDM_CD_MRU3						40106
#define IDM_CD_MRU4						40107
#define IDM_CD_MRU5						40108
#define IDM_CD_MRU6						40109
#define IDM_CD_MRU7						40110
#define IDM_CD_MRU8						40111

// 表示メニュー(システム)
#define IDM_LOG							40120
#define IDM_SCHEDULER					40121
#define IDM_DEVICE						40122

// 表示メニュー(プロセッサ)
#define IDM_CPUREG						40123
#define IDM_INT							40124
#define IDM_DISASM						40125
#define IDM_MEMORY						40126
#define IDM_BREAKP						40127

// 表示メニュー(デバイス)
#define IDM_MFP							40128
#define IDM_DMAC						40129
#define IDM_CRTC						40130
#define IDM_VC							40131
#define IDM_RTC							40132
#define IDM_OPM							40133
#define IDM_KEYBOARD					40134
#define IDM_FDD							40135
#define IDM_FDC							40136
#define IDM_SCC							40137
#define IDM_CYNTHIA						40138
#define IDM_SASI						40139
#define IDM_MIDI						40140
#define IDM_SCSI						40141

// 表示メニュー(ビデオ)
#define IDM_TVRAM						40142
#define IDM_G1024						40143
#define IDM_G16P0						40144
#define IDM_G16P1						40145
#define IDM_G16P2						40146
#define IDM_G16P3						40147
#define IDM_G256P0						40148
#define IDM_G256P1						40149
#define IDM_G64K						40150
#define IDM_PCG							40151
#define IDM_BG0							40152
#define IDM_BG1							40153
#define IDM_SPRITE						40154
#define IDM_PALET						40155

// 表示メニュー(レンダラ)
#define IDM_REND_TEXT					40156
#define IDM_REND_GP0					40157
#define IDM_REND_GP1					40158
#define IDM_REND_GP2					40159
#define IDM_REND_GP3					40160
#define IDM_REND_PCG					40161
#define IDM_REND_BG0					40162
#define IDM_REND_BG1					40163
#define IDM_REND_BGSPRITE				40164
#define IDM_REND_PALET					40165
#define IDM_REND_MIX					40166

// 表示メニュー(Win32)
#define IDM_COMPONENT					40167
#define IDM_OSINFO						40168
#define IDM_SOUND						40169
#define IDM_INPUT						40170
#define IDM_PORT						40171
#define IDM_BITMAP						40172
#define IDM_MIDIDRV						40173

// 表示メニュー
#define IDM_CAPTION						40177
#define IDM_MENU						40178
#define IDM_STATUS						40179
#define IDM_REFRESH						40180
#define IDM_STRETCH						40181
#define IDM_FULLSCREEN					40182

// デバッグメニュー
#define IDM_EXEC 						40185
#define IDM_BREAK						40186
#define IDM_TRACE						40187

// ツールメニュー
#define IDM_MOUSEMODE					40190
#define IDM_SOFTKEY						40191
#define IDM_TIMEADJ						40192
#define IDM_TRAP0						40193
#define IDM_SAVEWAV						40194
#define IDM_NEWFD						40195
#define IDM_NEWSASI						40196
#define IDM_NEWSCSI						40197
#define IDM_NEWMO						40198
#define IDM_OPTIONS						40199

// ウィンドウメニュー(最大ウィンドウ数72個を確保)
#define IDM_CASCADE						40220
#define IDM_TILE						40221
#define IDM_ICONIC						40222
#define IDM_ARRANGEICON					40223
#define IDM_HIDE						40224
#define IDM_RESTORE						40225
#define IDM_SWND_START					40226
#define IDM_SWND_END					40298

// ヘルプメニュー
#define IDM_ABOUT						40299

// 逆アセンブルメニュー
#define IDM_DIS_NEWWIN					40300
#define IDM_DIS_PC						40301
#define IDM_DIS_ADDR					40302
#define IDM_DIS_SYNC					40303

#define IDM_DIS_BREAKP0					40310
#define IDM_DIS_BREAKP1					40311
#define IDM_DIS_BREAKP2					40312
#define IDM_DIS_BREAKP3					40313
#define IDM_DIS_BREAKP4					40314
#define IDM_DIS_BREAKP5					40315
#define IDM_DIS_BREAKP6					40316
#define IDM_DIS_BREAKP7					40317

#define IDM_DIS_RESET					40320
#define IDM_DIS_BUSERR					40321
#define IDM_DIS_ADDRERR					40322
#define IDM_DIS_INST					40323
#define IDM_DIS_ZERODIV					40324
#define IDM_DIS_CHK						40325
#define IDM_DIS_TRAPV					40326
#define IDM_DIS_PRIV					40327
#define IDM_DIS_TRACE					40328
#define IDM_DIS_ALINE					40329
#define IDM_DIS_FLINE					40330
#define IDM_DIS_TRAP0					40331
#define IDM_DIS_TRAP1					40332
#define IDM_DIS_TRAP2					40333
#define IDM_DIS_TRAP3					40334
#define IDM_DIS_TRAP4					40335
#define IDM_DIS_TRAP5					40336
#define IDM_DIS_TRAP6					40337
#define IDM_DIS_TRAP7					40338
#define IDM_DIS_TRAP8					40339
#define IDM_DIS_TRAP9					40340
#define IDM_DIS_TRAPA					40341
#define IDM_DIS_TRAPB					40342
#define IDM_DIS_TRAPC					40343
#define IDM_DIS_TRAPD					40344
#define IDM_DIS_TRAPE					40345
#define IDM_DIS_TRAPF					40346
#define IDM_DIS_MFP0					40347
#define IDM_DIS_MFP1					40348
#define IDM_DIS_MFP2					40349
#define IDM_DIS_MFP3					40350
#define IDM_DIS_MFP4					40351
#define IDM_DIS_MFP5					40352
#define IDM_DIS_MFP6					40353
#define IDM_DIS_MFP7					40354
#define IDM_DIS_MFP8					40355
#define IDM_DIS_MFP9					40356
#define IDM_DIS_MFPA					40357
#define IDM_DIS_MFPB					40358
#define IDM_DIS_MFPC					40359
#define IDM_DIS_MFPD					40360
#define IDM_DIS_MFPE					40361
#define IDM_DIS_MFPF					40362
#define IDM_DIS_SCC0					40363
#define IDM_DIS_SCC1					40364
#define IDM_DIS_SCC2					40365
#define IDM_DIS_SCC3					40366
#define IDM_DIS_SCC4					40367
#define IDM_DIS_SCC5					40368
#define IDM_DIS_SCC6					40369
#define IDM_DIS_SCC7					40370
#define IDM_DIS_DMAC0					40371
#define IDM_DIS_DMAC1					40372
#define IDM_DIS_DMAC2					40373
#define IDM_DIS_DMAC3					40374
#define IDM_DIS_DMAC4					40375
#define IDM_DIS_DMAC5					40376
#define IDM_DIS_DMAC6					40377
#define IDM_DIS_DMAC7					40378
#define IDM_DIS_IOSC0					40379
#define IDM_DIS_IOSC1					40380
#define IDM_DIS_IOSC2					40381
#define IDM_DIS_IOSC3					40382

// メモリメニュー
#define IDM_MEMORY_NEWWIN				40400
#define IDM_MEMORY_ADDR					40401
#define IDM_MEMORY_BYTE					40402
#define IDM_MEMORY_WORD					40403
#define IDM_MEMORY_LONG					40404
#define IDM_MEMORY_0					40405
#define IDM_MEMORY_1					40406
#define IDM_MEMORY_2					40407
#define IDM_MEMORY_3					40408
#define IDM_MEMORY_4					40409
#define IDM_MEMORY_5					40410
#define IDM_MEMORY_6					40411
#define IDM_MEMORY_7					40412
#define IDM_MEMORY_8					40413
#define IDM_MEMORY_9					40414
#define IDM_MEMORY_A					40415
#define IDM_MEMORY_B					40416
#define IDM_MEMORY_C					40417
#define IDM_MEMORY_D					40418
#define IDM_MEMORY_E					40419
#define IDM_MEMORY_F					40420

// エリアメニュー
#define IDM_AREA_MPU					40430
#define IDM_AREA_IOCS					40431
#define IDM_AREA_DOS					40432
#define IDM_AREA_HUMAN					40433
#define IDM_AREA_GVRAM					40434
#define IDM_AREA_TVRAM					40435
#define IDM_AREA_CRTC					40436
#define IDM_AREA_VC						40437
#define IDM_AREA_DMAC					40438
#define IDM_AREA_AREA					40439
#define IDM_AREA_MFP					40440
#define IDM_AREA_RTC					40441
#define IDM_AREA_PRN					40442
#define IDM_AREA_SYSP					40443
#define IDM_AREA_OPM					40444
#define IDM_AREA_APCM					40445
#define IDM_AREA_FDC					40446
#define IDM_AREA_SASI					40447
#define IDM_AREA_SCC					40448
#define IDM_AREA_PPI					40449
#define IDM_AREA_IOSC					40450
#define IDM_AREA_WDRV					40451
#define IDM_AREA_SCSI					40452
#define IDM_AREA_MIDI					40453
#define IDM_AREA_SPREG					40454
#define IDM_AREA_SPMEM					40455
#define IDM_AREA_MERC					40456
#define IDM_AREA_NEPX					40457
#define IDM_AREA_SRSW					40458
#define IDM_AREA_SRUSER					40459
#define IDM_AREA_CGROM					40460
#define IDM_AREA_SCSIROM				40461
#define IDM_AREA_IPLROM					40462

// ヒストリメニュー
#define IDM_HISTORY_0					40470
#define IDM_HISTORY_1					40471
#define IDM_HISTORY_2					40472
#define IDM_HISTORY_3					40473
#define IDM_HISTORY_4					40474
#define IDM_HISTORY_5					40475
#define IDM_HISTORY_6					40476
#define IDM_HISTORY_7					40477
#define IDM_HISTORY_8					40478
#define IDM_HISTORY_9					40479

// スタックメニュー
#define IDM_STACK_0						40480
#define IDM_STACK_1						40481
#define IDM_STACK_2						40482
#define IDM_STACK_3						40483
#define IDM_STACK_4						40484
#define IDM_STACK_5						40485
#define IDM_STACK_6						40486
#define IDM_STACK_7						40487
#define IDM_STACK_8						40488
#define IDM_STACK_9						40489
#define IDM_STACK_A						40490
#define IDM_STACK_B						40491
#define IDM_STACK_C						40492
#define IDM_STACK_D						40493
#define IDM_STACK_E						40494
#define IDM_STACK_F						40495

// PCGメニュー
#define IDM_PCG_0						40500
#define IDM_PCG_1						40501
#define IDM_PCG_2						40502
#define IDM_PCG_3						40503
#define IDM_PCG_4						40504
#define IDM_PCG_5						40505
#define IDM_PCG_6						40506
#define IDM_PCG_7						40507
#define IDM_PCG_8						40508
#define IDM_PCG_9						40509
#define IDM_PCG_A						40510
#define IDM_PCG_B						40511
#define IDM_PCG_C						40512
#define IDM_PCG_D						40513
#define IDM_PCG_E						40514
#define IDM_PCG_F						40515

// レジスタメニュー
#define IDM_REG_D0						40600
#define IDM_REG_D1						40601
#define IDM_REG_D2						40602
#define IDM_REG_D3						40603
#define IDM_REG_D4						40604
#define IDM_REG_D5						40605
#define IDM_REG_D6						40606
#define IDM_REG_D7						40607
#define IDM_REG_A0						40608
#define IDM_REG_A1						40609
#define IDM_REG_A2						40610
#define IDM_REG_A3						40611
#define IDM_REG_A4						40612
#define IDM_REG_A5						40613
#define IDM_REG_A6						40614
#define IDM_REG_A7						40615
#define IDM_REG_USP						40616
#define IDM_REG_SSP						40617
#define IDM_REG_PC						40618
#define IDM_REG_SR						40619

// ブレークポイントメニュー
#define IDM_BREAKP_ENABLE				40700
#define IDM_BREAKP_CLEAR				40701
#define IDM_BREAKP_DEL					40702
#define IDM_BREAKP_ADDR					40703
#define IDM_BREAKP_ALL					40704
#define ID_FILE_CARGAR40006				40705

// 文字列(サブウィンドウ)
#define IDS_SWND_LOG					41000
#define IDS_SWND_LOG_NUMBER				41001
#define IDS_SWND_LOG_TIME				41002
#define IDS_SWND_LOG_PC					41003
#define IDS_SWND_LOG_LEVEL				41004
#define IDS_SWND_LOG_DEVICE				41005
#define IDS_SWND_LOG_MSG				41006
#define IDS_SWND_LOG_DETAIL				41007
#define IDS_SWND_LOG_NORMAL				41008
#define IDS_SWND_LOG_WARNING			41009
#define IDS_SWND_SCHEDULER				41010
#define IDS_SWND_DEVICE					41011

#define IDS_SWND_CPUREG					41100
#define IDS_SWND_INT					41101
#define IDS_SWND_DISASM					41102
#define IDS_SWND_MEMORY					41103
#define IDS_SWND_BREAKP					41104

#define IDS_SWND_MFP					41200
#define IDS_SWND_DMAC					41201
#define IDS_SWND_CRTC					41202
#define IDS_SWND_VC						41203
#define IDS_SWND_RTC					41204
#define IDS_SWND_OPM					41205
#define IDS_SWND_KEYBOARD				41206
#define IDS_SWND_FDD					41207
#define IDS_SWND_FDC					41208
#define IDS_SWND_SCC					41209
#define IDS_SWND_CYNTHIA				41210
#define IDS_SWND_SASI					41211
#define IDS_SWND_MIDI					41212
#define IDS_SWND_SCSI					41213

#define IDS_SWND_TVRAM					41300
#define IDS_SWND_G1024					41301
#define IDS_SWND_G16P0					41302
#define IDS_SWND_G16P1					41303
#define IDS_SWND_G16P2					41304
#define IDS_SWND_G16P3					41305
#define IDS_SWND_G256P0					41306
#define IDS_SWND_G256P1					41307
#define IDS_SWND_G64K					41308
#define IDS_SWND_PCG					41309
#define IDS_SWND_BG0					41310
#define IDS_SWND_BG1					41311
#define IDS_SWND_SPRITE					41312
#define IDS_SWND_PALET					41313

#define IDS_SWND_REND_TEXT				41400
#define IDS_SWND_REND_GP0				41401
#define IDS_SWND_REND_GP1				41402
#define IDS_SWND_REND_GP2				41403
#define IDS_SWND_REND_GP3				41404
#define IDS_SWND_REND_PCG				41405
#define IDS_SWND_REND_BG0				41406
#define IDS_SWND_REND_BG1				41407
#define IDS_SWND_REND_BGSPRITE			41408
#define IDS_SWND_REND_PALET				41409
#define IDS_SWND_REND_MIX				41410

#define IDS_SWND_COMPONENT				41500
#define IDS_SWND_OSINFO					41501
#define IDS_SWND_SOUND					41502
#define IDS_SWND_INPUT					41503
#define IDS_SWND_PORT					41504
#define IDS_SWND_BITMAP					41505
#define IDS_SWND_MIDIDRV				41506

#define IDS_SWND_SOFTKEY				41600

// 文字列(その他)
#define IDS_CAPTION_RUN					42000
#define IDS_CAPTION_STOP				42001
#define IDS_INIT_VMERR					42002
#define IDS_INIT_COMERR					42003
#define IDS_INIT_IPLERR					42004
#define IDS_INIT_CGERR					42005
#define IDS_FDOPEN						42006
#define IDS_FDERR						42007
#define IDS_NEWFD						42008
#define IDS_SASIOPEN					42009
#define IDS_NEWSASI						42010
#define IDS_CREATEERR					42011
#define IDS_OPTIONS						42012
#define IDS_MOUSE_X68K					42013
#define IDS_MOUSE_WIN					42014
#define IDS_WAVOPEN						42015
#define IDS_WAVSTART					42016
#define IDS_WAVSTOP						42017
#define IDS_KBD_NO						42018
#define IDS_KBD_KEYTOP					42019
#define IDS_KBD_DIRECTX					42020
#define IDS_RESET						42021
#define IDS_KBDMAP_GUIDE				42022
#define IDS_STDWIN						42023
#define IDS_SASI_CAPACITY				42024
#define IDS_SASI_FILENAME				42025
#define IDS_SASI_DEVERROR				42026
#define IDS_NEWSCSI						42027
#define IDS_NEWMO						42028
#define IDS_SCSIOPEN					42029
#define IDS_MOOPEN						42030
#define IDS_CANCEL						42031
#define IDS_MOERR						42032
#define IDS_SXSI_CAPACITY				42033
#define IDS_SXSI_FILENAME				42034
#define IDS_SXSI_SASI					42035
#define IDS_SXSI_MO						42036
#define IDS_SXSI_INIT					42037
#define IDS_SXSI_NONE					42038
#define IDS_SXSI_DEVERROR				42039
#define IDS_FDI_DISKNAME				42040
#define IDS_FDI_COMMENT					42041
#define IDS_PORT_NOASSIGN				42042
#define IDS_MIDI_NOASSIGN				42043
#define IDS_MIDI_MAPPER					42044
#define IDS_SOUND_NOASSIGN				42045
#define IDS_BADFDI_WARNING				42046
#define IDS_TKEY_NOASSIGN				42047
#define IDS_TKEY_NO						42048
#define IDS_TKEY_KEYTOP					42049
#define IDS_TKEY_VK						42050
#define IDS_INTERRUPT					42051
#define IDS_POWEROFF					42052
#define IDS_JOY_NOASSIGN				42053
#define IDS_JOY_ATARI					42054
#define IDS_JOY_ATARISS					42055
#define IDS_JOY_CYBERA					42056
#define IDS_JOY_CYBERD					42057
#define IDS_JOY_MD3						42058
#define IDS_JOY_MD6						42059
#define IDS_JOY_CPSF					42060
#define IDS_JOY_CPSFMD					42061
#define IDS_JOY_MAGICAL					42062
#define IDS_JOY_LR						42063
#define IDS_JOY_PACLAND					42064
#define IDS_JOY_BM						42065
#define IDS_KBD_DELTITLE				42066
#define IDS_KBD_DELMSG					42067
#define IDS_JOYSET						42068
#define IDS_JOYSET_NOASSIGN				42069
#define IDS_JOYSET_BTNPORT				42070
#define IDS_JOYSET_BTNLABEL				42071
#define IDS_JOYDET_ANALOG				42072
#define IDS_XM6OPEN						42073
#define IDS_XM6LOADOK					42074
#define IDS_XM6LOADERR					42075
#define IDS_XM6SAVEOK					42076
#define IDS_XM6SAVEERR					42077
#define IDS_EXIT						42078
#define IDS_XM6LOADFILE					42079
#define IDS_XM6LOADHDR					42080
#define IDS_XM6LOADVER					42081
#define IDS_FILEUPDATE					42082
#define IDS_SCSI_CAPACITY				42083
#define IDS_SCSI_FILENAME				42084
#define IDS_SCSI_MO						42085
#define IDS_SCSI_CD						42086
#define IDS_SCSI_INIT					42087
#define IDS_SCSI_NONE					42088
#define IDS_SCSI_DEVERROR				42089
#define IDS_CDOPEN						42090
#define IDS_CDERR						42091
#define IDS_SCHDSIZE					42092
#define IDS_MPUSXSI						42093
#define IDS_SAVECLOSE					42094
#define IDS_PROCESSOR					42095

// メニュー文字列(US)
#define IDM_US_OPEN						45000
#define IDM_US_SAVE						45001
#define IDM_US_SAVEAS					45002
#define IDM_US_RESET					45003
#define IDM_US_INTERRUPT				45004
#define IDM_US_POWER					45005
#define IDM_US_EXIT						45015

#define IDM_US_D0OPEN					45020
#define IDM_US_D0EJECT 					45021
#define IDM_US_D0WRITEP					45022
#define IDM_US_D0FORCE					45023
#define IDM_US_D0INVALID				45024
#define IDM_US_D1OPEN					45050
#define IDM_US_D1EJECT 					45051
#define IDM_US_D1WRITEP					45052
#define IDM_US_D1FORCE					45053
#define IDM_US_D1INVALID				45054
#define IDM_US_MOOPEN					45080
#define IDM_US_MOEJECT					45081
#define IDM_US_MOWRITEP					45082
#define IDM_US_MOFORCE					45083
#define IDM_US_CDOPEN					45100
#define IDM_US_CDEJECT					45101
#define IDM_US_CDFORCE					45102

#define IDM_US_CPUREG					45123
#define IDM_US_INT	 					45124
#define IDM_US_DISASM					45125
#define IDM_US_MEMORY					45126
#define IDM_US_BREAKP					45127
#define IDM_US_MFP						45128
#define IDM_US_DMAC						45129
#define IDM_US_CRTC						45130
#define IDM_US_VC						45131
#define IDM_US_RTC						45132
#define IDM_US_OPM						45133
#define IDM_US_KEYBOARD					45134
#define IDM_US_FDD						45135
#define IDM_US_FDC						45136
#define IDM_US_SCC						45137
#define IDM_US_CYNTHIA					45138
#define IDM_US_SASI						45139
#define IDM_US_MIDI						45140
#define IDM_US_SCSI						45141
#define IDM_US_COMPONENT				45167
#define IDM_US_OSINFO					45168
#define IDM_US_SOUND					45169
#define IDM_US_INPUT					45170
#define IDM_US_PORT						45171
#define IDM_US_BITMAP					45172
#define IDM_US_MIDIDRV					45173
#define IDM_US_CAPTION					45177
#define IDM_US_MENU						45178
#define IDM_US_STATUS					45179
#define IDM_US_REFRESH					45180
#define IDM_US_STRETCH					45181
#define IDM_US_FULLSCREEN				45182
#define IDM_US_MOUSEMODE				45190
#define IDM_US_SOFTKEY					45191
#define IDM_US_TIMEADJ					45192
#define IDM_US_TRAP0					45193
#define IDM_US_SAVEWAV					45194
#define IDM_US_NEWFD					45195
#define IDM_US_NEWSASI					45196
#define IDM_US_NEWSCSI					45197
#define IDM_US_NEWMO					45198
#define IDM_US_OPTIONS					45199
#define IDM_US_CASCADE					45220
#define IDM_US_TILE						45221
#define IDM_US_ICONIC					45222
#define IDM_US_ARRANGEICON				45223
#define IDM_US_HIDE						45224
#define IDM_US_RESTORE					45225
#define IDM_US_ABOUT					45299

// 文字列(サブウィンドウ, US)
#define IDS_US_SWND_LOG					46000
#define IDS_US_SWND_LOG_NUMBER			46001
#define IDS_US_SWND_LOG_TIME			46002
#define IDS_US_SWND_LOG_PC				46003
#define IDS_US_SWND_LOG_LEVEL			46004
#define IDS_US_SWND_LOG_DEVICE			46005
#define IDS_US_SWND_LOG_MSG				46006
#define IDS_US_SWND_LOG_DETAIL			46007
#define IDS_US_SWND_LOG_NORMAL			46008
#define IDS_US_SWND_LOG_WARNING			46009
#define IDS_US_SWND_SCHEDULER			46010
#define IDS_US_SWND_DEVICE				46011

#define IDS_US_SWND_CPUREG				46100
#define IDS_US_SWND_INT					46101
#define IDS_US_SWND_DISASM				46102
#define IDS_US_SWND_MEMORY				46103
#define IDS_US_SWND_BREAKP				46104

#define IDS_US_SWND_MFP					46200
#define IDS_US_SWND_DMAC				46201
#define IDS_US_SWND_CRTC				46202
#define IDS_US_SWND_VC					46203
#define IDS_US_SWND_RTC					46204
#define IDS_US_SWND_OPM					46205
#define IDS_US_SWND_KEYBOARD			46206
#define IDS_US_SWND_FDD					46207
#define IDS_US_SWND_FDC					46208
#define IDS_US_SWND_SCC					46209
#define IDS_US_SWND_CYNTHIA				46210
#define IDS_US_SWND_SASI				46211
#define IDS_US_SWND_MIDI				46212
#define IDS_US_SWND_SCSI				46213

#define IDS_US_SWND_TVRAM				46300
#define IDS_US_SWND_G1024				46301
#define IDS_US_SWND_G16P0				46302
#define IDS_US_SWND_G16P1				46303
#define IDS_US_SWND_G16P2				46304
#define IDS_US_SWND_G16P3				46305
#define IDS_US_SWND_G256P0				46306
#define IDS_US_SWND_G256P1				46307
#define IDS_US_SWND_G64K				46308
#define IDS_US_SWND_PCG					46309
#define IDS_US_SWND_BG0					46310
#define IDS_US_SWND_BG1					46311
#define IDS_US_SWND_SPRITE				46312
#define IDS_US_SWND_PALET				46313

#define IDS_US_SWND_REND_TEXT			46400
#define IDS_US_SWND_REND_GP0			46401
#define IDS_US_SWND_REND_GP1			46402
#define IDS_US_SWND_REND_GP2			46403
#define IDS_US_SWND_REND_GP3			46404
#define IDS_US_SWND_REND_PCG			46405
#define IDS_US_SWND_REND_BG0			46406
#define IDS_US_SWND_REND_BG1			46407
#define IDS_US_SWND_REND_BGSPRITE		46408
#define IDS_US_SWND_REND_PALET			46409
#define IDS_US_SWND_REND_MIX			46410

#define IDS_US_SWND_COMPONENT			46500
#define IDS_US_SWND_OSINFO				46501
#define IDS_US_SWND_SOUND				46502
#define IDS_US_SWND_INPUT				46503
#define IDS_US_SWND_PORT				46504
#define IDS_US_SWND_BITMAP				46505
#define IDS_US_SWND_MIDIDRV 			46506

#define IDS_US_SWND_SOFTKEY				46600

// 文字列(その他, US)
#define IDS_US_CAPTION_RUN				47000
#define IDS_US_CAPTION_STOP				47001
#define IDS_US_INIT_VMERR				47002
#define IDS_US_INIT_COMERR				47003
#define IDS_US_INIT_IPLERR				47004
#define IDS_US_INIT_CGERR				47005
#define IDS_US_FDOPEN					47006
#define IDS_US_FDERR					47007
#define IDS_US_NEWFD					47008
#define IDS_US_SASIOPEN					47009
#define IDS_US_NEWSASI					47010
#define IDS_US_CREATEERR				47011
#define IDS_US_OPTIONS					47012
#define IDS_US_MOUSE_X68K				47013
#define IDS_US_MOUSE_WIN				47014
#define IDS_US_WAVOPEN					47015
#define IDS_US_WAVSTART					47016
#define IDS_US_WAVSTOP					47017
#define IDS_US_KBD_NO					47018
#define IDS_US_KBD_KEYTOP				47019
#define IDS_US_KBD_DIRECTX				47020
#define IDS_US_RESET					47021
#define IDS_US_KBDMAP_GUIDE				47022
#define IDS_US_STDWIN					47023
#define IDS_US_SASI_CAPACITY			47024
#define IDS_US_SASI_FILENAME			47025
#define IDS_US_SASI_DEVERROR			47026
#define IDS_US_NEWSCSI					47027
#define IDS_US_NEWMO					47028
#define IDS_US_SCSIOPEN					47029
#define IDS_US_MOOPEN					47030
#define IDS_US_CANCEL					47031
#define IDS_US_MOERR					47032
#define IDS_US_SXSI_CAPACITY			47033
#define IDS_US_SXSI_FILENAME			47034
#define IDS_US_SXSI_SASI				47035
#define IDS_US_SXSI_MO					47036
#define IDS_US_SXSI_INIT				47037
#define IDS_US_SXSI_NONE				47038
#define IDS_US_SXSI_DEVERROR			47039
#define IDS_US_FDI_DISKNAME				47040
#define IDS_US_FDI_COMMENT				47041
#define IDS_US_PORT_NOASSIGN			47042
#define IDS_US_MIDI_NOASSIGN			47043
#define IDS_US_MIDI_MAPPER				47044
#define IDS_US_SOUND_NOASSIGN			47045
#define IDS_US_BADFDI_WARNING			47046
#define IDS_US_TKEY_NOASSIGN			47047
#define IDS_US_TKEY_NO					47048
#define IDS_US_TKEY_KEYTOP				47049
#define IDS_US_TKEY_VK					47050
#define IDS_US_INTERRUPT				47051
#define IDS_US_POWEROFF					47052
#define IDS_US_JOY_NOASSIGN				47053
#define IDS_US_JOY_ATARI				47054
#define IDS_US_JOY_ATARISS				47055
#define IDS_US_JOY_CYBERA				47056
#define IDS_US_JOY_CYBERD				47057
#define IDS_US_JOY_MD3					47058
#define IDS_US_JOY_MD6					47059
#define IDS_US_JOY_CPSF					47060
#define IDS_US_JOY_CPSFMD				47061
#define IDS_US_JOY_MAGICAL				47062
#define IDS_US_JOY_LR					47063
#define IDS_US_JOY_PACLAND				47064
#define IDS_US_JOY_BM					47065
#define IDS_US_KBD_DELTITLE				47066
#define IDS_US_KBD_DELMSG				47067
#define IDS_US_JOYSET					47068
#define IDS_US_JOYSET_NOASSIGN			47069
#define IDS_US_JOYSET_BTNPORT			47070
#define IDS_US_JOYSET_BTNLABEL			47071
#define IDS_US_JOYDET_ANALOG			47072
#define IDS_US_XM6OPEN					47073
#define IDS_US_XM6LOADOK				47074
#define IDS_US_XM6LOADERR				47075
#define IDS_US_XM6SAVEOK				47076
#define IDS_US_XM6SAVEERR				47077
#define IDS_US_EXIT						47078
#define IDS_US_XM6LOADFILE				47079
#define IDS_US_XM6LOADHDR				47080
#define IDS_US_XM6LOADVER				47081
#define IDS_US_FILEUPDATE				47082
#define IDS_US_SCSI_CAPACITY			47083
#define IDS_US_SCSI_FILENAME			47084
#define IDS_US_SCSI_MO					47085
#define IDS_US_SCSI_CD					47086
#define IDS_US_SCSI_INIT				47087
#define IDS_US_SCSI_NONE				47088
#define IDS_US_SCSI_DEVERROR			47089
#define IDS_US_CDOPEN					47090
#define IDS_US_CDERR					47091
#define IDS_US_SCHDSIZE					47092
#define IDS_US_MPUSXSI					47093
#define IDS_US_SAVECLOSE				47094
#define IDS_US_PROCESSOR				47095

#endif	// mfc_res_h
#endif	// _WIN32
