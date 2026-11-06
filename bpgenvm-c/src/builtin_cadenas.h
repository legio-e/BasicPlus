/* builtin_cadenas.h — GENERADO por scripts/gen_builtins.py de bpstdlib/Core.bp (el módulo raíz, H1).
 * NO EDITAR A MANO. Sólo lo incluye builtins.c.
 *
 * #774 (V8, 2-nov) — LOS PARÁMETROS string DE CADA BUILTIN: dónde está su referencia al llamarlo (bytes bajo sp;
 * el último argumento, arriba). Un null ahí es «referencia nula: <nombre> de un string null», antes de entrar
 * (bpvm_call_builtin); lo mismo que Builtin.cadenas en miVM. Los que tratan el null como "" (concatenar,
 * escribir: NULL_ES_VACIA en el generador), sin ninguno. */
#ifndef BPVM_BUILTIN_CADENAS_H
#define BPVM_BUILTIN_CADENAS_H

#include <stdint.h>
#include "builtin_ids.h"

typedef struct {
    const char* nombre;   /* el del mensaje: el nombre BP, sin los __ */
    uint8_t     off[3];   /* bytes bajo sp; 0, no hay más */
} bpvm_builtin_cadenas_t;

static const bpvm_builtin_cadenas_t bpvm_builtin_cadenas[60] = {
    { "", { 0, 0, 0 } },
    { "strlen",              {   8,   0,   0 } },   /* [0] strlen(s: string): integer */
    { "parseInt",            {   8,   0,   0 } },   /* [1] parseInt(s: string): integer */
    { "parseFloat",          {   8,   0,   0 } },   /* [2] parseFloat(s: string): float */
    { "upper",               {   8,   0,   0 } },   /* [6] upper(s: string): string */
    { "lower",               {   8,   0,   0 } },   /* [7] lower(s: string): string */
    { "trim",                {   8,   0,   0 } },   /* [8] trim(s: string): string */
    { "substring",           {  16,   0,   0 } },   /* [9] substring(s: string, start: integer, hasta: integer): string */
    { "indexOf",             {  16,   8,   0 } },   /* [10] indexOf(s: string, target: string): integer */
    { "startsWith",          {  16,   8,   0 } },   /* [11] startsWith(s: string, prefix: string): boolean */
    { "endsWith",            {  16,   8,   0 } },   /* [12] endsWith(s: string, suffix: string): boolean */
    { "contains",            {  16,   8,   0 } },   /* [13] contains(s: string, sub: string): boolean */
    { "charAt",              {  12,   0,   0 } },   /* [14] charAt(s: string, i: integer): string */
    { "replace",             {  24,  16,   8 } },   /* [15] replace(s: string, target: string, replacement: string): string */
    { "split",               {  16,   8,   0 } },   /* [36] split(s: string, sep: string): string[] */
    { "readFile",            {   8,   0,   0 } },   /* [38] readFile(path: string): string */
    { "writeFile",           {  16,   8,   0 } },   /* [39] writeFile(path: string, content: string) */
    { "appendFile",          {  16,   8,   0 } },   /* [40] appendFile(path: string, content: string) */
    { "fileExists",          {   8,   0,   0 } },   /* [41] fileExists(path: string): boolean */
    { "listDir",             {   8,   0,   0 } },   /* [42] listDir(path: string): string[] */
    { "charCodeAt",          {  12,   0,   0 } },   /* [48] charCodeAt(s: string, i: integer): integer */
    { "pathJoin",            {  16,   8,   0 } },   /* [64] __pathJoin(a: string, b: string): string */
    { "pathParent",          {   8,   0,   0 } },   /* [65] __pathParent(p: string): string */
    { "pathBasename",        {   8,   0,   0 } },   /* [66] __pathBasename(p: string): string */
    { "pathExtension",       {   8,   0,   0 } },   /* [67] __pathExtension(p: string): string */
    { "pathAbsolute",        {   8,   0,   0 } },   /* [68] __pathAbsolute(p: string): string */
    { "mkdir",               {   8,   0,   0 } },   /* [69] __mkdir(p: string) */
    { "rmdir",               {   8,   0,   0 } },   /* [70] __rmdir(p: string) */
    { "removeFile",          {   8,   0,   0 } },   /* [71] __removeFile(p: string) */
    { "rename",              {  16,   8,   0 } },   /* [72] __rename(src: string, dst: string) */
    { "copyFile",            {  16,   8,   0 } },   /* [73] __copyFile(src: string, dst: string) */
    { "fileSize",            {   8,   0,   0 } },   /* [74] __fileSize(p: string): integer */
    { "isDirectory",         {   8,   0,   0 } },   /* [75] __isDirectory(p: string): boolean */
    { "lastModified",        {   8,   0,   0 } },   /* [76] __lastModified(p: string): integer */
    { "prompt",              {   8,   0,   0 } },   /* [77] __prompt(spec: string): string */
    { "toBytes",             {   8,   0,   0 } },   /* [119] toBytes(s: string): byte[] */
    { "readFileBytes",       {   8,   0,   0 } },   /* [126] readFileBytes(path: string): byte[] */
    { "writeFileBytes",      {  16,   0,   0 } },   /* [127] writeFileBytes(path: string, data: byte[]) */
    { "throwRte",            {   8,   0,   0 } },   /* [128] __throwRte(msg: string) */
    { "tcpConnect",          {  16,   0,   0 } },   /* [131] __tcpConnect(host: string, port: integer, timeoutMs: integer): integer */
    { "guiSetText",          {   8,   0,   0 } },   /* [139] __guiSetText(handle: integer, t: string) */
    { "guiSetOptions",       {   8,   0,   0 } },   /* [175] __guiSetOptions(handle: integer, opts: string) */
    { "guiSetButtons",       {   8,   0,   0 } },   /* [182] __guiSetButtons(handle: integer, labels: string) */
    { "guiTabviewAddTab",    {   8,   0,   0 } },   /* [184] __guiTabviewAddTab(handle: integer, name: string): integer */
    { "guiTableSetCell",     {   8,   0,   0 } },   /* [187] __guiTableSetCell(handle: integer, row: integer, col: integer, text: string) */
    { "guiImageLoadFile",    {   8,   0,   0 } },   /* [190] __guiImageLoadFile(handle: integer, path: string): integer */
    { "eval",                {   8,   0,   0 } },   /* [200] eval(expr: string): float */
    { "guiInvokeByName",     {  16,   0,   0 } },   /* [206] __guiInvokeByName(host: Object, name: string, sender: Object) */
    { "guiLoadFont",         {   8,   0,   0 } },   /* [210] __guiLoadFont(path: string): integer */
    { "guiSlotOf",           {   8,   0,   0 } },   /* [223] __guiSlotOf(obj: Object, name: string): integer */
    { "runArg",              {   8,   0,   0 } },   /* [232] __runArg(defecto: string): string */
    { "guiShot",             {   8,   0,   0 } },   /* [239] __guiShot(path: string): integer */
    { "File",                {   8,   0,   0 } },   /* [240] __fstateSize(path: string): integer */
    { "File",                {  12,   0,   0 } },   /* [241] __fopen(st: byte[], path: string, mode: integer) */
    { "machineGetEnv",       {   8,   0,   0 } },   /* [271] __machineGetEnv(key: string): string */
    { "strCompare",          {  16,   8,   0 } },   /* [282] __strCompare(a: string, b: string): integer */
    { "readFile",            {  12,   0,   0 } },   /* [285] __readFileI(path: string, ignorarError: boolean): string */
    { "soundPlay",           {   8,   0,   0 } },   /* [287] __soundPlay(path: string) */
    { "Overlay.load",        {   8,   0,   0 } },   /* [296] __overlayLoad(ruta: string) */
    { "Overlay.tryExe",      {   8,   0,   0 } },   /* [297] __overlayExe(arg: string) */
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
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 54,   /* 256.. */
     0,  0,  0,  0,  0,  0,  0,  0,  0,  0, 55,  0,  0, 56,  0, 57,   /* 272.. */
     0,  0,  0,  0,  0,  0,  0,  0, 58, 59,  0,  0,  0,  0,  0,  0,   /* 288.. */
     0,  0,  0,  0,   /* 304.. */
};

#endif
