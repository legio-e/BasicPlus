/*
 * GENERADO por scripts/gen_builtins.py de bpstdlib/Core.bp (el módulo raíz, H1) — NO EDITAR A MANO.
 * Cada builtin es una función de Core.bp con cuerpo `inline CALL_BUILTIN <id>`; el id es el número
 * escrito allí y aquí es el ordinal(). Para añadir uno: declararlo en Core.bp (al final, con el
 * número siguiente), implementarlo en VirtualMachine.dispatchBuiltin y en builtins.c, y regenerar.
 */
package edu.bpgenvm.bytecode;

import java.util.HashMap;
import java.util.Map;

public enum Builtin {
    STRLEN("strlen", 8),   // [0] strlen(s: string): integer
    PARSE_INT("parseInt", 8),   // [1] parseInt(s: string): integer
    PARSE_FLOAT("parseFloat", 8),   // [2] parseFloat(s: string): float
    INT_TO_STRING("intToString"),   // [3] intToString(n: integer): string
    FLOAT_TO_STRING("floatToString"),   // [4] floatToString(x: float): string
    BOOL_TO_STRING("boolToString"),   // [5] boolToString(b: boolean): string
    UPPER("upper", 8),   // [6] upper(s: string): string
    LOWER("lower", 8),   // [7] lower(s: string): string
    TRIM("trim", 8),   // [8] trim(s: string): string
    SUBSTRING("substring", 16),   // [9] substring(s: string, start: integer, hasta: integer): string
    INDEX_OF("indexOf", 16, 8),   // [10] indexOf(s: string, target: string): integer
    STARTS_WITH("startsWith", 16, 8),   // [11] startsWith(s: string, prefix: string): boolean
    ENDS_WITH("endsWith", 16, 8),   // [12] endsWith(s: string, suffix: string): boolean
    CONTAINS("contains", 16, 8),   // [13] contains(s: string, sub: string): boolean
    CHAR_AT("charAt", 12),   // [14] charAt(s: string, i: integer): string
    REPLACE("replace", 24, 16, 8),   // [15] replace(s: string, target: string, replacement: string): string
    ABS("abs"),   // [16] abs(x: integer): integer
    MIN("min"),   // [17] min(a: integer, b: integer): integer
    MAX("max"),   // [18] max(a: integer, b: integer): integer
    SQRT("sqrt"),   // [19] sqrt(x: float): float
    POW("pow"),   // [20] pow(b: float, e: float): float
    LOG("log"),   // [21] log(x: float): float
    LOG10("log10"),   // [22] log10(x: float): float
    EXP("exp"),   // [23] exp(x: float): float
    SIN("sin"),   // [24] sin(x: float): float
    COS("cos"),   // [25] cos(x: float): float
    TAN("tan"),   // [26] tan(x: float): float
    RANDOM("random"),   // [27] random(): float
    RANDOM_INT("randomInt"),   // [28] randomInt(lo: integer, hi: integer): integer
    PI("pi"),   // [29] pi(): float
    E("e"),   // [30] e(): float
    FLOOR("floor"),   // [31] floor(x: float): integer
    CEIL("ceil"),   // [32] ceil(x: float): integer
    ROUND("round"),   // [33] round(x: float): integer
    NOW("now"),   // [34] now(): integer
    SLEEP("sleep"),   // [35] sleep(ms: integer)
    SPLIT("split", 16, 8),   // [36] split(s: string, sep: string): string[]
    INPUT("input"),   // [37] input(): string
    READ_FILE("readFile", 8),   // [38] readFile(path: string): string
    WRITE_FILE("writeFile", 16, 8),   // [39] writeFile(path: string, content: string)
    APPEND_FILE("appendFile", 16, 8),   // [40] appendFile(path: string, content: string)
    FILE_EXISTS("fileExists", 8),   // [41] fileExists(path: string): boolean
    LIST_DIR("listDir", 8),   // [42] listDir(path: string): string[]
    GC("gc"),   // [43] gc()
    NEW_REF_ARRAY("__newRefArray"),   // [44] __newRefArray(cap: integer): integer[]
    GROW_REF_ARRAY("__growRefArray"),   // [45] __growRefArray(old: integer[], newCap: integer): integer[]
    GROW_INT_ARRAY("__growIntArray"),   // [46] __growIntArray(old: integer[], newCap: integer): integer[]
    CHARS_TO_STRING("__charsToString"),   // [47] __charsToString(chars: integer[], len: integer): string
    CHAR_CODE_AT("charCodeAt", 12),   // [48] charCodeAt(s: string, i: integer): integer
    THREAD_START("__threadStart"),   // [49] __threadStart(t: any)
    THREAD_JOIN("__threadJoin"),   // [50] __threadJoin(t: any)
    YIELD("yield"),   // [51] yield()
    MUTEX_CREATE("__mutexCreate"),   // [52] __mutexCreate(): integer
    MUTEX_LOCK("__mutexLock"),   // [53] __mutexLock(m: any)
    MUTEX_UNLOCK("__mutexUnlock"),   // [54] __mutexUnlock(m: any)
    MOVE("move"),   // [55] move(src: any, dst: any, srcStart: integer, dstStart: integer, count: integer)
    SIGN_I("__sign_i"),   // [56] __sign_i(x: integer): integer
    SIGN_F("__sign_f"),   // [57] __sign_f(x: float): integer
    ASIN("__asin"),   // [58] __asin(x: float): float
    ACOS("__acos"),   // [59] __acos(x: float): float
    ATAN("__atan"),   // [60] __atan(x: float): float
    ATAN2("__atan2"),   // [61] __atan2(y: float, x: float): float
    FACTORIAL_I("__factorial_i"),   // [62] __factorial_i(n: integer): integer
    GAMMA_F("__gamma_f"),   // [63] __gamma_f(x: float): float
    PATH_JOIN("__pathJoin", 16, 8),   // [64] __pathJoin(a: string, b: string): string
    PATH_PARENT("__pathParent", 8),   // [65] __pathParent(p: string): string
    PATH_BASENAME("__pathBasename", 8),   // [66] __pathBasename(p: string): string
    PATH_EXTENSION("__pathExtension", 8),   // [67] __pathExtension(p: string): string
    PATH_ABSOLUTE("__pathAbsolute", 8),   // [68] __pathAbsolute(p: string): string
    MKDIR("__mkdir", 8),   // [69] __mkdir(p: string)
    RMDIR("__rmdir", 8),   // [70] __rmdir(p: string)
    REMOVE_FILE("__removeFile", 8),   // [71] __removeFile(p: string)
    RENAME("__rename", 16, 8),   // [72] __rename(src: string, dst: string)
    COPY_FILE("__copyFile", 16, 8),   // [73] __copyFile(src: string, dst: string)
    FILE_SIZE("__fileSize", 8),   // [74] __fileSize(p: string): integer
    IS_DIRECTORY("__isDirectory", 8),   // [75] __isDirectory(p: string): boolean
    LAST_MODIFIED("__lastModified", 8),   // [76] __lastModified(p: string): integer
    PROMPT("__prompt", 8),   // [77] __prompt(spec: string): string
    GPIO_INIT("__gpioInit"),   // [78] __gpioInit(pin: integer, mode: integer)
    GPIO_WRITE("__gpioWrite"),   // [79] __gpioWrite(pin: integer, value: integer)
    GPIO_READ("__gpioRead"),   // [80] __gpioRead(pin: integer): integer
    GPIO_PULL("__gpioPull"),   // [81] __gpioPull(pin: integer, pullMode: integer)
    I2C_INIT("__i2cInit"),   // [82] __i2cInit(bus: integer, sda: integer, scl: integer, baudrate: integer)
    I2C_WRITE("__i2cWrite"),   // [83] __i2cWrite(bus: integer, addr: integer, data: integer[], count: integer): integer
    I2C_READ("__i2cRead"),   // [84] __i2cRead(bus: integer, addr: integer, data: integer[], count: integer): integer
    NEW_INT_ARRAY("newIntArray"),   // [85] newIntArray(size: integer): integer[]
    SPI_INIT("__spiInit"),   // [86] __spiInit(bus: integer, sck: integer, mosi: integer, miso: integer, baudrate: integer, mode: integer)
    SPI_WRITE("__spiWrite"),   // [87] __spiWrite(bus: integer, data: integer[], count: integer): integer
    SPI_READ("__spiRead"),   // [88] __spiRead(bus: integer, data: integer[], count: integer): integer
    SPI_TRANSFER("__spiTransfer"),   // [89] __spiTransfer(bus: integer, tx: integer[], rx: integer[], count: integer): integer
    UART_INIT("__uartInit"),   // [90] __uartInit(bus: integer, tx: integer, rx: integer, baudrate: integer, dataBits: integer, stopBits: integer, parity: integer)
    UART_WRITE("__uartWrite"),   // [91] __uartWrite(bus: integer, data: integer[], count: integer): integer
    UART_READ("__uartRead"),   // [92] __uartRead(bus: integer, data: integer[], count: integer, timeoutMs: integer): integer
    UART_AVAILABLE("__uartAvailable"),   // [93] __uartAvailable(bus: integer): integer
    PULSE_INIT("__pulseInit"),   // [94] __pulseInit(pin: integer, edgeKind: integer): integer
    PULSE_START("__pulseStart"),   // [95] __pulseStart(counterId: integer)
    PULSE_STOP("__pulseStop"),   // [96] __pulseStop(counterId: integer)
    PULSE_VALUE("__pulseValue"),   // [97] __pulseValue(counterId: integer): integer
    PULSE_RESET("__pulseReset"),   // [98] __pulseReset(counterId: integer)
    PWM_INIT("__pwmInit"),   // [99] __pwmInit(pin: integer, freqHz: integer): integer
    PWM_SET_FREQ("__pwmSetFreq"),   // [100] __pwmSetFreq(sliceId: integer, freqHz: integer)
    PWM_SET_DUTY("__pwmSetDuty"),   // [101] __pwmSetDuty(sliceId: integer, pin: integer, dutyPct: integer)
    PWM_START("__pwmStart"),   // [102] __pwmStart(sliceId: integer)
    PWM_STOP("__pwmStop"),   // [103] __pwmStop(sliceId: integer)
    MACHINE_UNIQUE_ID("__machineUniqueId"),   // [104] __machineUniqueId(): string
    MACHINE_BOARD_NAME("__machineBoardName"),   // [105] __machineBoardName(): string
    MACHINE_TEMP_C("__machineTempC"),   // [106] __machineTempC(): float
    MACHINE_CPU_FREQ_HZ("__machineCpuFreqHz"),   // [107] __machineCpuFreqHz(): integer
    MACHINE_UPTIME_MS("__machineUptimeMs"),   // [108] __machineUptimeMs(): integer
    SLEEP_SEC("sleepSec"),   // [109] sleepSec(s: integer)
    SLEEP_US("sleepUs"),   // [110] sleepUs(us: integer)
    MACHINE_SET_CPU_FREQ_MHZ("__machineSetCpuFreqMHz"),   // [111] __machineSetCpuFreqMHz(mhz: integer): boolean
    RTC_NOW_SEC("__rtcNowSec"),   // [112] __rtcNowSec(): integer
    RTC_SET_NOW_SEC("__rtcSetNowSec"),   // [113] __rtcSetNowSec(sec: integer)
    ADC_INIT_CHANNEL("__adcInitChannel"),   // [114] __adcInitChannel(ch: integer): integer
    ADC_READ_CHANNEL("__adcReadChannel"),   // [115] __adcReadChannel(ch: integer): integer
    WDT_ENABLE("__wdtEnable"),   // [116] __wdtEnable(timeoutMs: integer)
    WDT_FEED("__wdtFeed"),   // [117] __wdtFeed()
    WDT_DISABLE("__wdtDisable"),   // [118] __wdtDisable()
    TO_BYTES("toBytes", 8),   // [119] toBytes(s: string): byte[]
    FROM_BYTES("fromBytes"),   // [120] fromBytes(b: byte[]): string
    HEAP_FRAG("heapFrag"),   // [121] heapFrag(): string
    HEAP_MAP("heapMap"),   // [122] heapMap(cols: integer): string
    MACHINE_GPIO_COUNT("__machineGpioCount"),   // [123] __machineGpioCount(): integer
    NP_INIT("__npInit"),   // [124] __npInit(pin: integer)
    NP_SHOW("__npShow"),   // [125] __npShow(pin: integer, grb: integer[], count: integer)
    READ_FILE_BYTES("readFileBytes", 8),   // [126] readFileBytes(path: string): byte[]
    WRITE_FILE_BYTES("writeFileBytes", 16),   // [127] writeFileBytes(path: string, data: byte[])
    THROW_RTE("__throwRte", 8),   // [128] __throwRte(msg: string)
    LONG_TO_STRING("__longToString"),   // [129] __longToString(v: long): string
    DOUBLE_TO_STRING("__doubleToString"),   // [130] __doubleToString(v: double): string
    TCP_CONNECT("__tcpConnect", 16),   // [131] __tcpConnect(host: string, port: integer, timeoutMs: integer): integer
    TCP_SEND("__tcpSend"),   // [132] __tcpSend(h: integer, data: byte[]): integer
    TCP_RECV("__tcpRecv"),   // [133] __tcpRecv(h: integer, max: integer, timeoutMs: integer): byte[]
    TCP_CLOSE("__tcpClose"),   // [134] __tcpClose(h: integer)
    GUI_SCREEN_ACTIVE("__guiScreenActive"),   // [135] __guiScreenActive(): integer
    GUI_CREATE_OBJ("__guiCreateObj"),   // [136] __guiCreateObj(parent: integer): integer
    GUI_CREATE_LABEL("__guiCreateLabel"),   // [137] __guiCreateLabel(parent: integer): integer
    GUI_CREATE_BUTTON("__guiCreateButton"),   // [138] __guiCreateButton(parent: integer): integer
    GUI_SET_TEXT("__guiSetText", 8),   // [139] __guiSetText(handle: integer, t: string)
    GUI_SET_WIDTH("__guiSetWidth"),   // [140] __guiSetWidth(handle: integer, w: integer)
    GUI_SET_HEIGHT("__guiSetHeight"),   // [141] __guiSetHeight(handle: integer, h: integer)
    GUI_ALIGN("__guiAlign"),   // [142] __guiAlign(handle: integer, a: integer, dx: integer, dy: integer)
    GUI_SET_BG_COLOR("__guiSetBgColor"),   // [143] __guiSetBgColor(handle: integer, rgb: integer)
    GUI_SET_TEXT_COLOR("__guiSetTextColor"),   // [144] __guiSetTextColor(handle: integer, rgb: integer)
    GUI_SET_FONT("__guiSetFont"),   // [145] __guiSetFont(handle: integer, fontId: integer)
    GUI_CLEAN("__guiClean"),   // [146] __guiClean(handle: integer)
    GUI_DELETE("__guiDelete"),   // [147] __guiDelete(handle: integer)
    GUI_SCREEN_LOAD("__guiScreenLoad"),   // [148] __guiScreenLoad(handle: integer)
    GUI_RUN("__guiRun"),   // [149] __guiRun()
    GUI_DUMP_TREE("__guiDumpTree"),   // [150] __guiDumpTree(): string
    GUI_BIND_CLICK("__guiBindClick"),   // [151] __guiBindClick(handle: integer, self: any)
    GUI_CLICK("__guiClick"),   // [152] __guiClick(w: any)
    GUI_SET_X("__guiSetX"),   // [153] __guiSetX(handle: integer, x: integer)
    GUI_GET_X("__guiGetX"),   // [154] __guiGetX(handle: integer): integer
    GUI_SET_Y("__guiSetY"),   // [155] __guiSetY(handle: integer, y: integer)
    GUI_GET_Y("__guiGetY"),   // [156] __guiGetY(handle: integer): integer
    GUI_GET_WIDTH("__guiGetWidth"),   // [157] __guiGetWidth(handle: integer): integer
    GUI_GET_HEIGHT("__guiGetHeight"),   // [158] __guiGetHeight(handle: integer): integer
    GUI_SET_SCROLL_DIR("__guiSetScrollDir"),   // [159] __guiSetScrollDir(handle: integer, dir: integer)
    GUI_GET_SCROLL_DIR("__guiGetScrollDir"),   // [160] __guiGetScrollDir(handle: integer): integer
    GUI_REFRESH("__guiRefresh"),   // [161] __guiRefresh(handle: integer)
    GUI_CREATE_CHECKBOX("__guiCreateCheckbox"),   // [162] __guiCreateCheckbox(parent: integer): integer
    GUI_SET_CHECKED("__guiSetChecked"),   // [163] __guiSetChecked(handle: integer, v: boolean)
    GUI_GET_CHECKED("__guiGetChecked"),   // [164] __guiGetChecked(handle: integer): boolean
    GUI_CHANGE("__guiChange"),   // [165] __guiChange(w: any)
    GUI_CREATE_SWITCH("__guiCreateSwitch"),   // [166] __guiCreateSwitch(parent: integer): integer
    GUI_CREATE_SLIDER("__guiCreateSlider"),   // [167] __guiCreateSlider(parent: integer): integer
    GUI_CREATE_BAR("__guiCreateBar"),   // [168] __guiCreateBar(parent: integer): integer
    GUI_SET_VALUE("__guiSetValue"),   // [169] __guiSetValue(handle: integer, v: integer)
    GUI_GET_VALUE("__guiGetValue"),   // [170] __guiGetValue(handle: integer): integer
    GUI_SET_RANGE("__guiSetRange"),   // [171] __guiSetRange(handle: integer, mn: integer, mx: integer)
    GUI_CREATE_SPINBOX("__guiCreateSpinbox"),   // [172] __guiCreateSpinbox(parent: integer): integer
    GUI_CREATE_LED("__guiCreateLed"),   // [173] __guiCreateLed(parent: integer): integer
    GUI_CREATE_DROPDOWN("__guiCreateDropdown"),   // [174] __guiCreateDropdown(parent: integer): integer
    GUI_SET_OPTIONS("__guiSetOptions", 8),   // [175] __guiSetOptions(handle: integer, opts: string)
    GUI_CREATE_TEXTAREA("__guiCreateTextarea"),   // [176] __guiCreateTextarea(parent: integer): integer
    GUI_GET_TEXT("__guiGetText"),   // [177] __guiGetText(handle: integer): string
    GUI_CREATE_LIST("__guiCreateList"),   // [178] __guiCreateList(parent: integer): integer
    GUI_CREATE_KEYBOARD("__guiCreateKeyboard"),   // [179] __guiCreateKeyboard(parent: integer): integer
    GUI_KEYBOARD_SET_TEXTAREA("__guiKeyboardSetTextarea"),   // [180] __guiKeyboardSetTextarea(handle: integer, ta: integer)
    GUI_CREATE_MSGBOX("__guiCreateMsgbox"),   // [181] __guiCreateMsgbox(parent: integer): integer
    GUI_SET_BUTTONS("__guiSetButtons", 8),   // [182] __guiSetButtons(handle: integer, labels: string)
    GUI_CREATE_TABVIEW("__guiCreateTabview"),   // [183] __guiCreateTabview(parent: integer): integer
    GUI_TABVIEW_ADD_TAB("__guiTabviewAddTab", 8),   // [184] __guiTabviewAddTab(handle: integer, name: string): integer
    GUI_CREATE_TABLE("__guiCreateTable"),   // [185] __guiCreateTable(parent: integer): integer
    GUI_TABLE_SET_GRID("__guiTableSetGrid"),   // [186] __guiTableSetGrid(handle: integer, rows: integer, cols: integer)
    GUI_TABLE_SET_CELL("__guiTableSetCell", 8),   // [187] __guiTableSetCell(handle: integer, row: integer, col: integer, text: string)
    GUI_TABLE_GET_CELL("__guiTableGetCell"),   // [188] __guiTableGetCell(handle: integer, row: integer, col: integer): string
    GUI_IMAGE_NEW("__guiImageNew"),   // [189] __guiImageNew(): integer
    GUI_IMAGE_LOAD_FILE("__guiImageLoadFile", 8),   // [190] __guiImageLoadFile(handle: integer, path: string): integer
    GUI_IMAGE_WIDTH("__guiImageWidth"),   // [191] __guiImageWidth(handle: integer): integer
    GUI_IMAGE_HEIGHT("__guiImageHeight"),   // [192] __guiImageHeight(handle: integer): integer
    GUI_CREATE_IMAGE_VIEW("__guiCreateImageView"),   // [193] __guiCreateImageView(parent: integer): integer
    GUI_IMAGE_VIEW_SET_IMAGE("__guiImageViewSetImage"),   // [194] __guiImageViewSetImage(view: integer, image: integer)
    GUI_IMAGE_VIEW_REFRESH("__guiImageViewRefresh"),   // [195] __guiImageViewRefresh(view: integer)
    GUI_SET_FONT_SIZE("__guiSetFontSize"),   // [196] __guiSetFontSize(handle: integer, px: integer)
    GUI_GET_FONT_SIZE("__guiGetFontSize"),   // [197] __guiGetFontSize(handle: integer): integer
    GUI_TEXTAREA_SET_READONLY("__guiTextareaSetReadonly"),   // [198] __guiTextareaSetReadonly(handle: integer, ro: integer)
    GUI_TEXTAREA_GET_READONLY("__guiTextareaGetReadonly"),   // [199] __guiTextareaGetReadonly(handle: integer): integer
    EVAL("eval", 8),   // [200] eval(expr: string): float
    MACHINE_RESET_CAUSE("__machineResetCause"),   // [201] __machineResetCause(): string
    MACHINE_SET_MARK("__machineSetMark"),   // [202] __machineSetMark(code: integer)
    MACHINE_MARK_COUNT("__machineMarkCount"),   // [203] __machineMarkCount(): integer
    MACHINE_MARK_AT("__machineMarkAt"),   // [204] __machineMarkAt(i: integer): integer
    MACHINE_BOOT_COUNT("__machineBootCount"),   // [205] __machineBootCount(): integer
    GUI_INVOKE_BY_NAME("__guiInvokeByName", 16),   // [206] __guiInvokeByName(host: Object, name: string, sender: Object)
    GUI_INVOKE_BY_SLOT("__guiInvokeBySlot"),   // [207] __guiInvokeBySlot(win: Object, slot: integer, sender: Object)
    MACHINE_ADC_CHANNELS("__machineAdcChannels"),   // [208] __machineAdcChannels(): integer
    MACHINE_PWM_SLICES("__machinePwmSlices"),   // [209] __machinePwmSlices(): integer
    GUI_LOAD_FONT("__guiLoadFont", 8),   // [210] __guiLoadFont(path: string): integer
    APP_MAIN_MODULE("__appMainModule"),   // [211] __appMainModule(): string
    APP_MAIN_MODULE_PATH("__appMainModulePath"),   // [212] __appMainModulePath(): string
    APP_PROJECT_PATH("__appProjectPath"),   // [213] __appProjectPath(): string
    GUI_SET_ROTATION("__guiSetRotation"),   // [214] __guiSetRotation(deg: integer)
    GUI_CREATE_CHART("__guiCreateChart"),   // [215] __guiCreateChart(parent: integer): integer
    GUI_CHART_SET_POINTS("__guiChartSetPoints"),   // [216] __guiChartSetPoints(handle: integer, n: integer)
    GUI_CHART_ADD_SERIES("__guiChartAddSeries"),   // [217] __guiChartAddSeries(handle: integer, rgb: integer): integer
    GUI_CHART_PUSH("__guiChartPush"),   // [218] __guiChartPush(handle: integer, series: integer, v: integer)
    GUI_CHART_SET_VALUE("__guiChartSetValue"),   // [219] __guiChartSetValue(handle: integer, series: integer, idx: integer, v: integer)
    GUI_CHART_SET_TYPE("__guiChartSetType"),   // [220] __guiChartSetType(handle: integer, t: integer)
    EVENT_RAISE("__eventRaise"),   // [221] __eventRaise(recv: any, dest: any, nargs: integer, masks: integer)
    GUI_RUN_ONCE("__guiRunOnce"),   // [222] __guiRunOnce(): integer
    GUI_SLOT_OF("__guiSlotOf", 8),   // [223] __guiSlotOf(obj: Object, name: string): integer
    PACK_NEXT("__packNext"),   // [224] __packNext(cur: integer): integer
    PACK_INFO("__packInfo"),   // [225] __packInfo(cur: integer): string
    PACK_ENTRY_NEXT("__packEntryNext"),   // [226] __packEntryNext(pack: integer, cur: integer): integer
    PACK_ENTRY_INFO("__packEntryInfo"),   // [227] __packEntryInfo(pack: integer, cur: integer): string
    CLAMP_F("__clamp_f"),   // [228] __clamp_f(x: float, lo: float, hi: float): float
    WRAP_F("__wrap_f"),   // [229] __wrap_f(x: float, lo: float, hi: float): float
    HYPOT_F("__hypot_f"),   // [230] __hypot_f(x: float, y: float): float
    REMAP_F("__remap_f"),   // [231] __remap_f(x: float, inLo: float, inHi: float, outLo: float, outHi: float): float
    RUN_ARG("__runArg", 8),   // [232] __runArg(defecto: string): string
    MACHINE_MICRO("__machineMicro"),   // [233] __machineMicro(): string
    GUI_GET_ALIGN("__guiGetAlign"),   // [234] __guiGetAlign(handle: integer): integer
    GUI_GET_ALIGN_DX("__guiGetAlignDx"),   // [235] __guiGetAlignDx(handle: integer): integer
    GUI_GET_ALIGN_DY("__guiGetAlignDy"),   // [236] __guiGetAlignDy(handle: integer): integer
    GUI_GET_AUTH_WIDTH("__guiGetAuthWidth"),   // [237] __guiGetAuthWidth(handle: integer): integer
    GUI_GET_AUTH_HEIGHT("__guiGetAuthHeight"),   // [238] __guiGetAuthHeight(handle: integer): integer
    GUI_SHOT("__guiShot", 8),   // [239] __guiShot(path: string): integer
    FSTATE_SIZE("__fstateSize", 8),   // [240] __fstateSize(path: string): integer
    FOPEN("__fopen", 12),   // [241] __fopen(st: byte[], path: string, mode: integer)
    FCLOSE("__fclose"),   // [242] __fclose(st: byte[])
    FREAD("__fread"),   // [243] __fread(st: byte[], dst: byte[], off: integer, n: integer): integer
    FWRITE("__fwrite"),   // [244] __fwrite(st: byte[], src: byte[], off: integer, n: integer)
    FSEEK("__fseek"),   // [245] __fseek(st: byte[], pos: integer)
    FTELL("__ftell"),   // [246] __ftell(st: byte[]): integer
    FSIZE("__fsize"),   // [247] __fsize(st: byte[]): integer
    FFLUSH("__fflush"),   // [248] __fflush(st: byte[])
    FREAD_LINE("__freadLine"),   // [249] __freadLine(st: byte[]): string
    CAPS_ARRAY("__capsArray"),   // [250] __capsArray(kind: integer): integer[]
    CAPS_INT("__capsInt"),   // [251] __capsInt(kind: integer): integer
    CAPS_STR("__capsStr"),   // [252] __capsStr(kind: integer): string
    CAPS_PIN("__capsPin"),   // [253] __capsPin(pin: integer): integer
    GPIO_RELEASE("__gpioRelease"),   // [254] __gpioRelease(pin: integer)
    ADC_INIT_PIN("__adcInitPin"),   // [255] __adcInitPin(pin: integer): integer
    SPI_RELEASE("__spiRelease"),   // [256] __spiRelease(bus: integer)
    I2C_RELEASE("__i2cRelease"),   // [257] __i2cRelease(bus: integer)
    SPI_DEV_OPEN("__spiDevOpen"),   // [258] __spiDevOpen(bus: integer, cs: integer, baudrate: integer, mode: integer)
    SPI_DEV_XFER("__spiDevXfer"),   // [259] __spiDevXfer(bus: integer, cs: integer, baudrate: integer, mode: integer, op: integer, tx: integer[], ntx: integer, rx: integer[], nrx: integer): integer
    SPI_DEV_EXEC("__spiDevExec"),   // [260] __spiDevExec(bus: integer, cs: integer, baudrate: integer, mode: integer, ops: integer[], nops: integer, tx: integer[], rx: integer[]): integer
    I2C_WRITE_READ("__i2cWriteRead"),   // [261] __i2cWriteRead(bus: integer, addr: integer, tx: integer[], ntx: integer, rx: integer[], nrx: integer): integer
    I2C_EXEC("__i2cExec"),   // [262] __i2cExec(bus: integer, addr: integer, ops: integer[], nops: integer, tx: integer[], rx: integer[]): integer
    STR_COPY("__strCopy", 20),   // [263] __strCopy(src: string, dst: byte[], pos: integer): integer
    HEAP_SIZE("__heapSize"),   // [264] __heapSize(): integer
    HEAP_FREE("__heapFree"),   // [265] __heapFree(): integer
    HEAP_LARGEST_FREE("__heapLargestFree"),   // [266] __heapLargestFree(): integer
    HEAP_FREE_BLOCKS("__heapFreeBlocks"),   // [267] __heapFreeBlocks(): integer
    HEAP_HANDLES("__heapHandles"),   // [268] __heapHandles(): integer
    HEAP_COLLECTIONS("__heapCollections"),   // [269] __heapCollections(): integer
    HEAP_COMPACT("__heapCompact"),   // [270] __heapCompact()
    MACHINE_GET_ENV("__machineGetEnv", 8),   // [271] __machineGetEnv(key: string): string
    LIBERA_SI_SIN_DESTRUCTORES("__liberaSiSinDestructores"),   // [272] __liberaSiSinDestructores(x: any): boolean
    ORDEN_DESTRUCCION("__ordenDestruccion"),   // [273] __ordenDestruccion(x: any): any[]
    PREPARA_DESTRUCTOR("__preparaDestructor"),   // [274] __preparaDestructor(x: any): integer
    LIBERA_SOLO("__liberaSolo"),   // [275] __liberaSolo(x: any)
    AVISA_DESTRUCTOR("__avisaDestructor", 8),   // [276] __avisaDestructor(x: any, msg: string)
    SIGUIENTE_PENDIENTE("__siguientePendiente"),   // [277] __siguientePendiente(): any
    PWM_RELEASE("__pwmRelease"),   // [278] __pwmRelease(sliceId: integer, pin: integer)
    PULSE_RELEASE("__pulseRelease"),   // [279] __pulseRelease(counterId: integer, pin: integer)
    NP_RELEASE("__npRelease"),   // [280] __npRelease(pin: integer)
    UART_RELEASE("__uartRelease"),   // [281] __uartRelease(bus: integer, tx: integer, rx: integer)
    STR_COMPARE("__strCompare", 16, 8),   // [282] __strCompare(a: string, b: string): integer
    STR_NACE("__strNace"),   // [283] __strNace(b: byte[]): string
    FROM_BYTES_I("__fromBytesI"),   // [284] __fromBytesI(b: byte[], ignorarError: boolean): string
    READ_FILE_I("__readFileI", 12),   // [285] __readFileI(path: string, ignorarError: boolean): string
    FREAD_LINE_I("__freadLineI"),   // [286] __freadLineI(st: byte[], ignorarError: boolean): string
    SOUND_PLAY("__soundPlay", 8),   // [287] __soundPlay(path: string)
    SOUND_STOP("__soundStop"),   // [288] __soundStop()
    SOUND_IS_PLAYING("__soundIsPlaying"),   // [289] __soundIsPlaying(): boolean
    SOUND_SET_VOLUME("__soundSetVolume"),   // [290] __soundSetVolume(v: integer)
    SOUND_GET_VOLUME("__soundGetVolume"),   // [291] __soundGetVolume(): integer
    SOUND_IS_AVAILABLE("__soundIsAvailable"),   // [292] __soundIsAvailable(): boolean
    SOUND_GET_POSITION("__soundGetPosition"),   // [293] __soundGetPosition(): integer
    SOUND_GET_LENGTH("__soundGetLength"),   // [294] __soundGetLength(): integer
    EOL("eol"),   // [295] eol(): string
    OVERLAY_LOAD("__overlayLoad", 8),   // [296] __overlayLoad(ruta: string)
    OVERLAY_EXE("__overlayExe", 8),   // [297] __overlayExe(arg: string)
    OVERLAY_CLOSE("__overlayClose"),   // [298] __overlayClose()
    OVERLAY_FREE("__overlayFree"),   // [299] __overlayFree()
    OVERLAY_TRACE("__overlayTrace"),   // [300] __overlayTrace(): string
    CLASS_NAME("__className"),   // [301] __className(o: any): string
    OVERLAY_THREADS("__overlayThreads"),   // [302] __overlayThreads(): integer
    WIFI_TURN_ON("__wifiTurnOn"),   // [303] __wifiTurnOn()
    WIFI_TURN_OFF("__wifiTurnOff"),   // [304] __wifiTurnOff()
    WIFI_GET_STATE("__wifiGetState"),   // [305] __wifiGetState(): integer
    WIFI_HAS_IP("__wifiHasIp"),   // [306] __wifiHasIp(): boolean
    WIFI_IS_AVAILABLE("__wifiIsAvailable"),   // [307] __wifiIsAvailable(): boolean
    THROW_ERROR("__throwError", 8),   // [308] __throwError(tipo: integer, msg: string)
    BT_IS_AVAILABLE("__btIsAvailable"),   // [309] __btIsAvailable(): boolean
    BT_TURN_ON("__btTurnOn"),   // [310] __btTurnOn()
    BT_TURN_OFF("__btTurnOff"),   // [311] __btTurnOff()
    BT_GET_STATE("__btGetState"),   // [312] __btGetState(): integer
    BT_SCAN("__btScan"),   // [313] __btScan(ms: integer)
    BT_FOUND_COUNT("__btFoundCount"),   // [314] __btFoundCount(): integer
    BT_FOUND_NAME("__btFoundName"),   // [315] __btFoundName(i: integer): string
    BT_FOUND_ADDRESS("__btFoundAddress"),   // [316] __btFoundAddress(i: integer): string
    BT_FOUND_ADDRESS_TYPE("__btFoundAddressType"),   // [317] __btFoundAddressType(i: integer): string
    BT_FOUND_KIND("__btFoundKind"),   // [318] __btFoundKind(i: integer): string
    BT_FOUND_RSSI("__btFoundRssi"),   // [319] __btFoundRssi(i: integer): integer
    BT_CONNECT("__btConnect", 16, 8),   // [320] __btConnect(address: string, addressType: string): boolean
    BT_DISCONNECT("__btDisconnect", 8),   // [321] __btDisconnect(address: string)
    BT_FORGET("__btForget", 8),   // [322] __btForget(address: string)
    BT_IS_CONNECTED("__btIsConnected", 8),   // [323] __btIsConnected(address: string): boolean
    BT_KIND_OF("__btKindOf", 8),   // [324] __btKindOf(address: string): string
    KB_OPEN("__kbOpen"),   // [325] __kbOpen(t: any)
    KB_CLOSE("__kbClose"),   // [326] __kbClose(t: any)
    KB_HAS_KEY("__kbHasKey"),   // [327] __kbHasKey(): boolean
    KB_READ_KEY("__kbReadKey"),   // [328] __kbReadKey(): integer
    KB_READ_CHAR("__kbReadChar"),   // [329] __kbReadChar(): integer
    KB_INJECT("__kbInject"),   // [330] __kbInject(k: integer, cp: integer)
    KB_IS_CONNECTED("__kbIsConnected"),   // [331] __kbIsConnected(): boolean
    MS_OPEN("__msOpen"),   // [332] __msOpen(t: any)
    MS_CLOSE("__msClose"),   // [333] __msClose(t: any)
    MS_GET_X("__msGetX"),   // [334] __msGetX(): integer
    MS_GET_Y("__msGetY"),   // [335] __msGetY(): integer
    MS_GET_BUTTONS("__msGetButtons"),   // [336] __msGetButtons(): integer
    MS_INJECT("__msInject"),   // [337] __msInject(x: integer, y: integer, botones: integer, rueda: integer)
    MS_IS_CONNECTED("__msIsConnected"),   // [338] __msIsConnected(): boolean
    GUI_KEY_TARGET("__guiKeyTarget");   // [339] __guiKeyTarget(t: any)

    public final String bpName;
    public final int id;
    /** #774 — dónde está cada parámetro string al llamarlo (bytes bajo sp): un null ahí es «referencia nula», salvo
     *  en los que lo tratan como "" (nuloVale); #805, y una referencia caducada, en todos, «objeto eliminado». */
    public final int[] cadenas;

    Builtin(String bpName, int... cadenas) { this.bpName = bpName; this.id = ordinal(); this.cadenas = cadenas; }

    /** #774 — el nombre del mensaje «referencia nula: <nombre> de un string null»: el BP, sin los __, salvo los que
     *  despistan (NOMBRE_DE_ERROR en el generador; el mismo texto que builtin_cadenas.h). */
    public String nombreDeError() {
        switch (this) {
            case FOPEN: return "File";
            case FSTATE_SIZE: return "File";
            case OVERLAY_EXE: return "Overlay.tryExe";
            case OVERLAY_LOAD: return "Overlay.load";
            case READ_FILE_I: return "readFile";
            default: return bpName.startsWith("__") ? bpName.substring(2) : bpName;
        }
    }

    /** #774/#805 — ¿trata el null de sus parámetros string como ""? (NULL_ES_VACIA en el generador; el mismo
     *  que la marca nulo_vale de builtin_cadenas.h). Una referencia caducada no vale en ninguno. */
    public boolean nuloVale() {
        switch (this) {
            case AVISA_DESTRUCTOR: return true;
            case STR_COPY: return true;
            default: return false;
        }
    }

    private static final Map<String, Builtin> BY_NAME = new HashMap<>();
    static {
        for (Builtin b : values()) BY_NAME.put(b.bpName, b);
        BY_NAME.put("newObjArray", NEW_REF_ARRAY);   // alias en Core.bp
        BY_NAME.put("growObjArray", GROW_REF_ARRAY);   // alias en Core.bp
        BY_NAME.put("__newStrArray", NEW_REF_ARRAY);   // alias en Core.bp
        BY_NAME.put("__growStrArray", GROW_REF_ARRAY);   // alias en Core.bp
        BY_NAME.put("__refToString", LONG_TO_STRING);   // alias en Core.bp
        BY_NAME.put("fromBytes", FROM_BYTES_I);   // alias en Core.bp
        BY_NAME.put("readFile", READ_FILE_I);   // alias en Core.bp
        BY_NAME.put("len", STRLEN);   // alias en Core.bp
    }

    public static Builtin byName(String name) { return BY_NAME.get(name); }
    public static Builtin byId(int id) { return values()[id]; }
}
