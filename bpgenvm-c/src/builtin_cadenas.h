/* builtin_cadenas.h — GENERADO por scripts/gen_builtins.py de bpstdlib/Core.bp (el módulo raíz, H1).
 * NO EDITAR A MANO. Sólo lo incluye builtins.c.
 *
 * #774 (V8, 2-nov) — LOS PARÁMETROS string DE CADA BUILTIN: dónde está su referencia al llamarlo (bytes bajo sp;
 * el último argumento, arriba). Un null ahí es «referencia nula: <nombre> de un string null», antes de entrar
 * (bpvm_call_builtin); lo mismo que Builtin.cadenas en miVM. Los que tratan el null como "" (concatenar y el
 * aviso de un destructor: NULL_ES_VACIA en el generador), con su marca: nulo_vale.
 * #805 (V8, 7-nov) — y una referencia a una cadena que ya no existe, en todos: «referencia a objeto eliminado
 * (use-after-free)», también antes de entrar. */
#ifndef BPVM_BUILTIN_CADENAS_H
#define BPVM_BUILTIN_CADENAS_H

#include <stdint.h>
#include "builtin_ids.h"

typedef struct {
    const char* nombre;   /* el del mensaje: el nombre BP, sin los __ */
    uint8_t     off[3];   /* bytes bajo sp; 0, no hay más */
    uint8_t     nulo_vale;   /* 1: el null es "" (NULL_ES_VACIA); una referencia caducada no vale nunca */
} bpvm_builtin_cadenas_t;

static const bpvm_builtin_cadenas_t bpvm_builtin_cadenas[68] = {
    { "", { 0, 0, 0 }, 0 },
    { "strlen",              {   8,   0,   0 }, 0 },   /* [0] strlen(s: string): integer */
    { "parseInt",            {   8,   0,   0 }, 0 },   /* [1] parseInt(s: string): integer */
    { "parseFloat",          {   8,   0,   0 }, 0 },   /* [2] parseFloat(s: string): float */
    { "upper",               {   8,   0,   0 }, 0 },   /* [6] upper(s: string): string */
    { "lower",               {   8,   0,   0 }, 0 },   /* [7] lower(s: string): string */
    { "trim",                {   8,   0,   0 }, 0 },   /* [8] trim(s: string): string */
    { "substring",           {  16,   0,   0 }, 0 },   /* [9] substring(s: string, start: integer, hasta: integer): string */
    { "indexOf",             {  16,   8,   0 }, 0 },   /* [10] indexOf(s: string, target: string): integer */
    { "startsWith",          {  16,   8,   0 }, 0 },   /* [11] startsWith(s: string, prefix: string): boolean */
    { "endsWith",            {  16,   8,   0 }, 0 },   /* [12] endsWith(s: string, suffix: string): boolean */
    { "contains",            {  16,   8,   0 }, 0 },   /* [13] contains(s: string, sub: string): boolean */
    { "charAt",              {  12,   0,   0 }, 0 },   /* [14] charAt(s: string, i: integer): string */
    { "replace",             {  24,  16,   8 }, 0 },   /* [15] replace(s: string, target: string, replacement: string): string */
    { "split",               {  16,   8,   0 }, 0 },   /* [36] split(s: string, sep: string): string[] */
    { "readFile",            {   8,   0,   0 }, 0 },   /* [38] readFile(path: string): string */
    { "writeFile",           {  16,   8,   0 }, 0 },   /* [39] writeFile(path: string, content: string) */
    { "appendFile",          {  16,   8,   0 }, 0 },   /* [40] appendFile(path: string, content: string) */
    { "fileExists",          {   8,   0,   0 }, 0 },   /* [41] fileExists(path: string): boolean */
    { "listDir",             {   8,   0,   0 }, 0 },   /* [42] listDir(path: string): string[] */
    { "charCodeAt",          {  12,   0,   0 }, 0 },   /* [48] charCodeAt(s: string, i: integer): integer */
    { "pathJoin",            {  16,   8,   0 }, 0 },   /* [64] __pathJoin(a: string, b: string): string */
    { "pathParent",          {   8,   0,   0 }, 0 },   /* [65] __pathParent(p: string): string */
    { "pathBasename",        {   8,   0,   0 }, 0 },   /* [66] __pathBasename(p: string): string */
    { "pathExtension",       {   8,   0,   0 }, 0 },   /* [67] __pathExtension(p: string): string */
    { "pathAbsolute",        {   8,   0,   0 }, 0 },   /* [68] __pathAbsolute(p: string): string */
    { "mkdir",               {   8,   0,   0 }, 0 },   /* [69] __mkdir(p: string) */
    { "rmdir",               {   8,   0,   0 }, 0 },   /* [70] __rmdir(p: string) */
    { "removeFile",          {   8,   0,   0 }, 0 },   /* [71] __removeFile(p: string) */
    { "rename",              {  16,   8,   0 }, 0 },   /* [72] __rename(src: string, dst: string) */
    { "copyFile",            {  16,   8,   0 }, 0 },   /* [73] __copyFile(src: string, dst: string) */
    { "fileSize",            {   8,   0,   0 }, 0 },   /* [74] __fileSize(p: string): integer */
    { "isDirectory",         {   8,   0,   0 }, 0 },   /* [75] __isDirectory(p: string): boolean */
    { "lastModified",        {   8,   0,   0 }, 0 },   /* [76] __lastModified(p: string): integer */
    { "prompt",              {   8,   0,   0 }, 0 },   /* [77] __prompt(spec: string): string */
    { "toBytes",             {   8,   0,   0 }, 0 },   /* [119] toBytes(s: string): byte[] */
    { "readFileBytes",       {   8,   0,   0 }, 0 },   /* [126] readFileBytes(path: string): byte[] */
    { "writeFileBytes",      {  16,   0,   0 }, 0 },   /* [127] writeFileBytes(path: string, data: byte[]) */
    { "throwRte",            {   8,   0,   0 }, 0 },   /* [128] __throwRte(msg: string) */
    { "tcpConnect",          {  16,   0,   0 }, 0 },   /* [131] __tcpConnect(host: string, port: integer, timeoutMs: integer): integer */
    { "guiSetText",          {   8,   0,   0 }, 0 },   /* [139] __guiSetText(handle: integer, t: string) */
    { "guiSetOptions",       {   8,   0,   0 }, 0 },   /* [175] __guiSetOptions(handle: integer, opts: string) */
    { "guiSetButtons",       {   8,   0,   0 }, 0 },   /* [182] __guiSetButtons(handle: integer, labels: string) */
    { "guiTabviewAddTab",    {   8,   0,   0 }, 0 },   /* [184] __guiTabviewAddTab(handle: integer, name: string): integer */
    { "guiTableSetCell",     {   8,   0,   0 }, 0 },   /* [187] __guiTableSetCell(handle: integer, row: integer, col: integer, text: string) */
    { "guiImageLoadFile",    {   8,   0,   0 }, 0 },   /* [190] __guiImageLoadFile(handle: integer, path: string): integer */
    { "eval",                {   8,   0,   0 }, 0 },   /* [200] eval(expr: string): float */
    { "guiInvokeByName",     {  16,   0,   0 }, 0 },   /* [206] __guiInvokeByName(host: Object, name: string, sender: Object) */
    { "guiLoadFont",         {   8,   0,   0 }, 0 },   /* [210] __guiLoadFont(path: string): integer */
    { "guiSlotOf",           {   8,   0,   0 }, 0 },   /* [223] __guiSlotOf(obj: Object, name: string): integer */
    { "runArg",              {   8,   0,   0 }, 0 },   /* [232] __runArg(defecto: string): string */
    { "guiShot",             {   8,   0,   0 }, 0 },   /* [239] __guiShot(path: string): integer */
    { "File",                {   8,   0,   0 }, 0 },   /* [240] __fstateSize(path: string): integer */
    { "File",                {  12,   0,   0 }, 0 },   /* [241] __fopen(st: byte[], path: string, mode: integer) */
    { "strCopy",             {  20,   0,   0 }, 1 },   /* [263] __strCopy(src: string, dst: byte[], pos: integer): integer */
    { "machineGetEnv",       {   8,   0,   0 }, 0 },   /* [271] __machineGetEnv(key: string): string */
    { "avisaDestructor",     {   8,   0,   0 }, 1 },   /* [276] __avisaDestructor(x: any, msg: string) */
    { "strCompare",          {  16,   8,   0 }, 0 },   /* [282] __strCompare(a: string, b: string): integer */
    { "readFile",            {  12,   0,   0 }, 0 },   /* [285] __readFileI(path: string, ignorarError: boolean): string */
    { "soundPlay",           {   8,   0,   0 }, 0 },   /* [287] __soundPlay(path: string) */
    { "Overlay.load",        {   8,   0,   0 }, 0 },   /* [296] __overlayLoad(ruta: string) */
    { "Overlay.tryExe",      {   8,   0,   0 }, 0 },   /* [297] __overlayExe(arg: string) */
    { "throwError",          {   8,   0,   0 }, 0 },   /* [308] __throwError(tipo: integer, msg: string) */
    { "btConnect",           {  16,   8,   0 }, 0 },   /* [320] __btConnect(address: string, addressType: string): boolean */
    { "btDisconnect",        {   8,   0,   0 }, 0 },   /* [321] __btDisconnect(address: string) */
    { "btForget",            {   8,   0,   0 }, 0 },   /* [322] __btForget(address: string) */
    { "btIsConnected",       {   8,   0,   0 }, 0 },   /* [323] __btIsConnected(address: string): boolean */
    { "btKindOf",            {   8,   0,   0 }, 0 },   /* [324] __btKindOf(address: string): string */
};

/* La fila de arriba de cada builtin, por su id (0: ningún parámetro string que mirar). */
static const uint8_t bpvm_builtin_cadena_de[BUILTIN__CUANTOS] = {
     1,  2,  3,  0,  0,  0,  4,  5,  6,  7,  8,  9, 10, 11, 12, 13,   /* 0.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 16.. */
     0,  0,  0,  0, 14,  0, 15, 16, 17, 18, 19,  0,  0,  0,  0,  0,   /* 32.. */
    20,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 48.. */
    21, 22, 23, 24, 25, 26, 27, 28, 29, 30, 31, 32, 33, 34,  0,  0,   /* 64.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 80.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 96.. */
     0,  0,  0,  0,  0,  0,  0, 35,  0,  0,  0,  0,  0,  0, 36, 37,   /* 112.. */
    38,  0,  0, 39,  0,  0,  0,  0,  0,  0,  0, 40,  0,  0,  0,  0,   /* 128.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 144.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 41,   /* 160.. */
     0,  0,  0,  0,  0,  0, 42,  0, 43,  0,  0, 44,  0,  0, 45,  0,   /* 176.. */
     0,  0,  0,  0,  0,  0,  0,  0, 46,  0,  0,  0,  0,  0, 47,  0,   /* 192.. */
     0,  0, 48,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 49,   /* 208.. */
     0,  0,  0,  0,  0,  0,  0,  0, 50,  0,  0,  0,  0,  0,  0, 51,   /* 224.. */
    52, 53,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 240.. */
     0,  0,  0,  0,  0,  0,  0, 54,  0,  0,  0,  0,  0,  0,  0, 55,   /* 256.. */
     0,  0,  0,  0, 56,  0,  0,  0,  0,  0, 57,  0,  0, 58,  0, 59,   /* 272.. */
     0,  0,  0,  0,  0,  0,  0,  0, 60, 61,  0,  0,  0,  0,  0,  0,   /* 288.. */
     0,  0,  0,  0, 62,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 304.. */
    63, 64, 65, 66, 67,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 320.. */
     0,  0,  0,  0,   /* 336.. */
};

/* #807 (V8, 13-nov) — LOS DEMÁS PARÁMETROS REFERENCIA DE CADA BUILTIN (los arrays, any, Object): dónde están al
 * llamarlo (bytes bajo sp). Una referencia caducada ahí es «referencia a objeto eliminado (use-after-free)», antes
 * de entrar (bpvm_call_builtin); el null lo trata cada builtin. Sin los de SIN_MIRAR_REFERENCIAS en el generador
 * (el compilador los apila a su manera, o son de los destructores). Lo mismo que Builtin.referencias() en miVM. */
typedef struct {
    uint8_t off[3];   /* bytes bajo sp; 0, no hay más */
} bpvm_builtin_referencias_t;

static const bpvm_builtin_referencias_t bpvm_builtin_referencias[49] = {
    { { 0, 0, 0 } },
    { {  12,   0,   0 } },   /* [45] __growRefArray(old: integer[], newCap: integer): integer[] */
    { {  12,   0,   0 } },   /* [46] __growIntArray(old: integer[], newCap: integer): integer[] */
    { {  12,   0,   0 } },   /* [47] __charsToString(chars: integer[], len: integer): string */
    { {   8,   0,   0 } },   /* [49] __threadStart(t: any) */
    { {   8,   0,   0 } },   /* [50] __threadJoin(t: any) */
    { {   8,   0,   0 } },   /* [53] __mutexLock(m: any) */
    { {   8,   0,   0 } },   /* [54] __mutexUnlock(m: any) */
    { {  28,  20,   0 } },   /* [55] move(src: any, dst: any, srcStart: integer, dstStart: integer, count: integer) */
    { {  12,   0,   0 } },   /* [83] __i2cWrite(bus: integer, addr: integer, data: integer[], count: integer): integer */
    { {  12,   0,   0 } },   /* [84] __i2cRead(bus: integer, addr: integer, data: integer[], count: integer): integer */
    { {  12,   0,   0 } },   /* [87] __spiWrite(bus: integer, data: integer[], count: integer): integer */
    { {  12,   0,   0 } },   /* [88] __spiRead(bus: integer, data: integer[], count: integer): integer */
    { {  20,  12,   0 } },   /* [89] __spiTransfer(bus: integer, tx: integer[], rx: integer[], count: integer): integer */
    { {  12,   0,   0 } },   /* [91] __uartWrite(bus: integer, data: integer[], count: integer): integer */
    { {  16,   0,   0 } },   /* [92] __uartRead(bus: integer, data: integer[], count: integer, timeoutMs: integer): integer */
    { {   8,   0,   0 } },   /* [120] fromBytes(b: byte[]): string */
    { {  12,   0,   0 } },   /* [125] __npShow(pin: integer, grb: integer[], count: integer) */
    { {   8,   0,   0 } },   /* [127] writeFileBytes(path: string, data: byte[]) */
    { {   8,   0,   0 } },   /* [132] __tcpSend(h: integer, data: byte[]): integer */
    { {   8,   0,   0 } },   /* [151] __guiBindClick(handle: integer, self: any) */
    { {   8,   0,   0 } },   /* [152] __guiClick(w: any) */
    { {   8,   0,   0 } },   /* [165] __guiChange(w: any) */
    { {  24,   8,   0 } },   /* [206] __guiInvokeByName(host: Object, name: string, sender: Object) */
    { {  20,   8,   0 } },   /* [207] __guiInvokeBySlot(win: Object, slot: integer, sender: Object) */
    { {  16,   0,   0 } },   /* [223] __guiSlotOf(obj: Object, name: string): integer */
    { {  20,   0,   0 } },   /* [241] __fopen(st: byte[], path: string, mode: integer) */
    { {   8,   0,   0 } },   /* [242] __fclose(st: byte[]) */
    { {  24,  16,   0 } },   /* [243] __fread(st: byte[], dst: byte[], off: integer, n: integer): integer */
    { {  24,  16,   0 } },   /* [244] __fwrite(st: byte[], src: byte[], off: integer, n: integer) */
    { {  12,   0,   0 } },   /* [245] __fseek(st: byte[], pos: integer) */
    { {   8,   0,   0 } },   /* [246] __ftell(st: byte[]): integer */
    { {   8,   0,   0 } },   /* [247] __fsize(st: byte[]): integer */
    { {   8,   0,   0 } },   /* [248] __fflush(st: byte[]) */
    { {   8,   0,   0 } },   /* [249] __freadLine(st: byte[]): string */
    { {  24,  12,   0 } },   /* [259] __spiDevXfer(bus: integer, cs: integer, baudrate: integer, mode: integer, op: integer, tx: integer[], ntx: integer, rx: integer[], nrx: integer): integer */
    { {  28,  16,   8 } },   /* [260] __spiDevExec(bus: integer, cs: integer, baudrate: integer, mode: integer, ops: integer[], nops: integer, tx: integer[], rx: integer[]): integer */
    { {  24,  12,   0 } },   /* [261] __i2cWriteRead(bus: integer, addr: integer, tx: integer[], ntx: integer, rx: integer[], nrx: integer): integer */
    { {  28,  16,   8 } },   /* [262] __i2cExec(bus: integer, addr: integer, ops: integer[], nops: integer, tx: integer[], rx: integer[]): integer */
    { {  12,   0,   0 } },   /* [263] __strCopy(src: string, dst: byte[], pos: integer): integer */
    { {   8,   0,   0 } },   /* [283] __strNace(b: byte[]): string */
    { {  12,   0,   0 } },   /* [284] __fromBytesI(b: byte[], ignorarError: boolean): string */
    { {  12,   0,   0 } },   /* [286] __freadLineI(st: byte[], ignorarError: boolean): string */
    { {   8,   0,   0 } },   /* [301] __className(o: any): string */
    { {   8,   0,   0 } },   /* [325] __kbOpen(t: any) */
    { {   8,   0,   0 } },   /* [326] __kbClose(t: any) */
    { {   8,   0,   0 } },   /* [332] __msOpen(t: any) */
    { {   8,   0,   0 } },   /* [333] __msClose(t: any) */
    { {   8,   0,   0 } },   /* [339] __guiKeyTarget(t: any) */
};

/* La fila de arriba de cada builtin, por su id (0: ningún parámetro referencia que mirar). */
static const uint8_t bpvm_builtin_referencia_de[BUILTIN__CUANTOS] = {
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 0.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 16.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1,  2,  3,   /* 32.. */
     0,  4,  5,  0,  0,  6,  7,  8,  0,  0,  0,  0,  0,  0,  0,  0,   /* 48.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 64.. */
     0,  0,  0,  9, 10,  0,  0, 11, 12, 13,  0, 14, 15,  0,  0,  0,   /* 80.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 96.. */
     0,  0,  0,  0,  0,  0,  0,  0, 16,  0,  0,  0,  0, 17,  0, 18,   /* 112.. */
     0,  0,  0,  0, 19,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 128.. */
     0,  0,  0,  0,  0,  0,  0, 20, 21,  0,  0,  0,  0,  0,  0,  0,   /* 144.. */
     0,  0,  0,  0,  0, 22,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 160.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 176.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 23, 24,   /* 192.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 25,   /* 208.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 224.. */
     0, 26, 27, 28, 29, 30, 31, 32, 33, 34,  0,  0,  0,  0,  0,  0,   /* 240.. */
     0,  0,  0, 35, 36, 37, 38, 39,  0,  0,  0,  0,  0,  0,  0,  0,   /* 256.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 40, 41,  0, 42,  0,   /* 272.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 43,  0,  0,   /* 288.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,   /* 304.. */
     0,  0,  0,  0,  0, 44, 45,  0,  0,  0,  0,  0, 46, 47,  0,  0,   /* 320.. */
     0,  0,  0, 48,   /* 336.. */
};

#endif
