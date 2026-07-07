/*
 *  Copyright (c) 2021-2025 Qualcomm Innovation Center, Inc. All rights reserved.
 *  SPDX-License-Identifier: BSD-3-Clause-Clear
 */

#include "tafSmsPdu.hpp"
#include <string.h>

#define TP_TYPE_MASK                 0x03
#define TP_TYPE_SMS_DELIVER          0x00
#define TP_TYPE_SMS_SUBMIT           0x01
#define TP_TYPE_SMS_STATUS_REPORT    0x02
#define TP_TYPE_RESERVED             0x03

#define TYPE_ADDRESS_UNKNOWN         0x81
#define TYPE_ADDRESS_INTERNATIONAL   0x91

#define PDU_TYPE_TP_SRR  0x20

#define PDU_TYPE_TP_MTI_SMS_SUBMIT  0x01
#define PDU_TYPE_TP_VP_A_INTEGER    0x10

#define TP_DCS_7_BIT    0x00
#define TP_DCS_8_BIT    0x04
#define TP_DCS_16_BIT   0x08

#define NON_PRINTABLE_7_BIT    63
#define NON_PRINTABLE_8_BIT    '?'

#define ESCAPE_7BIT            0x1B
#define ESCAPE_SHIFT_INDEX     128

#define FORM_FEED_8BIT         12

#define FORM_FEED_7BIT_ESC            10     /*  [FF]  */
#define CIRCUMFLEX_ACCENT_7BIT_ESC    20     /*  ^     */
#define LEFT_CURLY_BRACKET_7BIT_ESC   40     /*  {     */
#define RIGHT_CURLY_BRACKET_7BIT_ESC  41     /*  }     */
#define REVERSE_SOLIDUS_7BIT_ESC      47     /*  \     */
#define LEFT_SQUARE_BRACKET_7BIT_ESC  60     /*  [     */
#define TILDE_7BIT_ESC                61     /*  ~     */
#define RIGHT_SQUARE_BRACKET_7BIT_ESC 62     /*  ]     */
#define VERTICAL_BAR_ESC              64     /*  |     */


const uint8_t ascii_8_to_7[] = {
    NON_PRINTABLE_7_BIT,       /*     0    [NUL]                            */
    NON_PRINTABLE_7_BIT,       /*     1    [SOH]                            */
    NON_PRINTABLE_7_BIT,       /*     2    [STX]                            */
    NON_PRINTABLE_7_BIT,       /*     3    [ETX]                            */
    NON_PRINTABLE_7_BIT,       /*     4    [EOT]                            */
    NON_PRINTABLE_7_BIT,       /*     5    [ENQ]                            */
    NON_PRINTABLE_7_BIT,       /*     6    [ACK]                            */
    NON_PRINTABLE_7_BIT,       /*     7    [BEL]                            */
    NON_PRINTABLE_7_BIT,       /*     8    [BS]                             */
    NON_PRINTABLE_7_BIT,       /*     9    [HT]                             */
    10,                        /*    10    [LF]                             */
    NON_PRINTABLE_7_BIT,       /*    11    [VT]                             */
    10+128,                    /*    12    [FF]                             */
    13,                        /*    13    [CR]                             */
    NON_PRINTABLE_7_BIT,       /*    14    [SO]                             */
    NON_PRINTABLE_7_BIT,       /*    15    [SI]                             */
    NON_PRINTABLE_7_BIT,       /*    16    [DLE]                            */
    NON_PRINTABLE_7_BIT,       /*    17    [DC1]                            */
    NON_PRINTABLE_7_BIT,       /*    18    [DC2]                            */
    NON_PRINTABLE_7_BIT,       /*    19    [DC3]                            */
    NON_PRINTABLE_7_BIT,       /*    20    [DC4]                            */
    NON_PRINTABLE_7_BIT,       /*    21    [NAK]                            */
    NON_PRINTABLE_7_BIT,       /*    22    [SYN]                            */
    NON_PRINTABLE_7_BIT,       /*    23    [ETB]                            */
    NON_PRINTABLE_7_BIT,       /*    24    [CAN]                            */
    NON_PRINTABLE_7_BIT,       /*    25    [EM]                             */
    NON_PRINTABLE_7_BIT,       /*    26    [SUB]                            */
    NON_PRINTABLE_7_BIT,       /*    27    [ESC]                            */
    NON_PRINTABLE_7_BIT,       /*    28    [FS]                             */
    NON_PRINTABLE_7_BIT,       /*    29    [GS]                             */
    NON_PRINTABLE_7_BIT,       /*    30    [RS]                             */
    NON_PRINTABLE_7_BIT,       /*    31    [US]                             */
    32,                        /*    32    (space)                          */
    33,                        /*    33    !                                */
    34,                        /*    34    "                                */
    35,                        /*    35    #                                */
    2,                         /*    36    $                                */
    37,                        /*    37    %                                */
    38,                        /*    38    &                                */
    39,                        /*    39    '                                */
    40,                        /*    40    (                                */
    41,                        /*    41    )                                */
    42,                        /*    42    *                                */
    43,                        /*    43    +                                */
    44,                        /*    44    ,                                */
    45,                        /*    45    -                                */
    46,                        /*    46    .                                */
    47,                        /*    47    /                                */
    48,                        /*    48    0                                */
    49,                        /*    49    1                                */
    50,                        /*    50    2                                */
    51,                        /*    51    3                                */
    52,                        /*    52    4                                */
    53,                        /*    53    5                                */
    54,                        /*    54    6                                */
    55,                        /*    55    7                                */
    56,                        /*    56    8                                */
    57,                        /*    57    9                                */
    58,                        /*    58    :                                */
    59,                        /*    59    ;                                */
    60,                        /*    60    <                                */
    61,                        /*    61    =                                */
    62,                        /*    62    >                                */
    63,                        /*    63    ?                                */
    0,                         /*    64    @                                */
    65,                        /*    65    A                                */
    66,                        /*    66    B                                */
    67,                        /*    67    C                                */
    68,                        /*    68    D                                */
    69,                        /*    69    E                                */
    70,                        /*    70    F                                */
    71,                        /*    71    G                                */
    72,                        /*    72    H                                */
    73,                        /*    73    I                                */
    74,                        /*    74    J                                */
    75,                        /*    75    K                                */
    76,                        /*    76    L                                */
    77,                        /*    77    M                                */
    78,                        /*    78    N                                */
    79,                        /*    79    O                                */
    80,                        /*    80    P                                */
    81,                        /*    81    Q                                */
    82,                        /*    82    R                                */
    83,                        /*    83    S                                */
    84,                        /*    84    T                                */
    85,                        /*    85    U                                */
    86,                        /*    86    V                                */
    87,                        /*    87    W                                */
    88,                        /*    88    X                                */
    89,                        /*    89    Y                                */
    90,                        /*    90    Z                                */
    60+128,                    /*    91    [                                */
    47+128,                    /*    92    \                                */
    62+128,                    /*    93    ]                                */
    20+128,                    /*    94    ^                                */
    17,                        /*    95    _                                */
    (uint8_t)-39,              /*    96    `                                */
    97,                        /*    97    a                                */
    98,                        /*    98    b                                */
    99,                        /*    99    c                                */
    100,                       /*   100    d                                */
    101,                       /*   101    e                                */
    102,                       /*   102    f                                */
    103,                       /*   103    g                                */
    104,                       /*   104    h                                */
    105,                       /*   105    i                                */
    106,                       /*   106    j                                */
    107,                       /*   107    k                                */
    108,                       /*   108    l                                */
    109,                       /*   109    m                                */
    110,                       /*   110    n                                */
    111,                       /*   111    o                                */
    112,                       /*   112    p                                */
    113,                       /*   113    q                                */
    114,                       /*   114    r                                */
    115,                       /*   115    s                                */
    116,                       /*   116    t                                */
    117,                       /*   117    u                                */
    118,                       /*   118    v                                */
    119,                       /*   119    w                                */
    120,                       /*   120    x                                */
    121,                       /*   121    y                                */
    122,                       /*   122    z                                */
    40+128,                    /*   123    {                                */
    64+128,                    /*   124    |                                */
    41+128,                    /*   125    }                                */
    61+128,                    /*   126    ~                                */
    NON_PRINTABLE_7_BIT,       /*   127    [DEL]                            */
    NON_PRINTABLE_7_BIT,       /*   128                                     */
    NON_PRINTABLE_7_BIT,       /*   129                                     */
    39,                        /*   130    (low left rising single quote)   */
    102,                       /*   131    (lowercase italic f)             */
    34,                        /*   132    (low left rising double quote)   */
    NON_PRINTABLE_7_BIT,       /*   133    (low horizontal ellipsis)        */
    NON_PRINTABLE_7_BIT,       /*   134    (dagger mark)                    */
    NON_PRINTABLE_7_BIT,       /*   135    (double dagger mark)             */
    NON_PRINTABLE_7_BIT,       /*   136    (letter modifying circumflex)    */
    NON_PRINTABLE_7_BIT,       /*   137    (per thousand (mille) sign)      */
    83,                        /*   138    (uppercase S caron or hacek)     */
    39,                        /*   139    (left single angle quote mark)   */
    214,                       /*   140    (uppercase OE ligature)          */
    NON_PRINTABLE_7_BIT,       /*   141                                     */
    NON_PRINTABLE_7_BIT,       /*   142                                     */
    NON_PRINTABLE_7_BIT,       /*   143                                     */
    NON_PRINTABLE_7_BIT,       /*   144                                     */
    39,                        /*   145    (left single quotation mark)     */
    39,                        /*   146    (right single quote mark)        */
    34,                        /*   147    (left double quotation mark)     */
    34,                        /*   148    (right double quote mark)        */
    42,                        /*   149    (round filled bullet)            */
    45,                        /*   150    (en dash)                        */
    45,                        /*   151    (em dash)                        */
    39,                        /*   152    (small spacing tilde accent)     */
    NON_PRINTABLE_7_BIT,       /*   153    (trademark sign)                 */
    115,                       /*   154    (lowercase s caron or hacek)     */
    39,                        /*   155    (right single angle quote mark)  */
    111,                       /*   156    (lowercase oe ligature)          */
    NON_PRINTABLE_7_BIT,       /*   157                                     */
    NON_PRINTABLE_7_BIT,       /*   158                                     */
    89,                        /*   159    (uppercase Y dieresis or umlaut) */
    32,                        /*   160    (non-breaking space)             */
    64,                        /*   161    ¡                                */
    99,                        /*   162    ¢                                */
    1,                         /*   163    £                                */
    36,                        /*   164    €                                */
    3,                         /*   165    ¥                                */
    33,                        /*   166    Š                                */
    95,                        /*   167    §                                */
    34,                        /*   168    š                                */
    NON_PRINTABLE_7_BIT,       /*   169    ©                                */
    NON_PRINTABLE_7_BIT,       /*   170    ª                                */
    60,                        /*   171    «                                */
    NON_PRINTABLE_7_BIT,       /*   172    ¬                                */
    45,                        /*   173    ­                                */
    NON_PRINTABLE_7_BIT,       /*   174    ®                                */
    NON_PRINTABLE_7_BIT,       /*   175    ¯                                */
    NON_PRINTABLE_7_BIT,       /*   176    °                                */
    NON_PRINTABLE_7_BIT,       /*   177    ±                                */
    50,                        /*   178    ²                                */
    51,                        /*   179    ³                                */
    39,                        /*   180    Ž                                */
    117,                       /*   181    µ                                */
    NON_PRINTABLE_7_BIT,       /*   182    ¶                                */
    NON_PRINTABLE_7_BIT,       /*   183    ·                                */
    NON_PRINTABLE_7_BIT,       /*   184    ž                                */
    49,                        /*   185    ¹                                */
    NON_PRINTABLE_7_BIT,       /*   186    º                                */
    62,                        /*   187    »                                */
    NON_PRINTABLE_7_BIT,       /*   188    Œ                                */
    NON_PRINTABLE_7_BIT,       /*   189    œ                                */
    NON_PRINTABLE_7_BIT,       /*   190    Ÿ                                */
    96,                        /*   191    ¿                                */
    65,                        /*   192    À                                */
    65,                        /*   193    Á                                */
    65,                        /*   194    Â                                */
    65,                        /*   195    Ã                                */
    91,                        /*   196    Ä                                */
    14,                        /*   197    Å                                */
    28,                        /*   198    Æ                                */
    9,                         /*   199    Ç                                */
    31,                        /*   200    È                                */
    31,                        /*   201    É                                */
    31,                        /*   202    Ê                                */
    31,                        /*   203    Ë                                */
    73,                        /*   204    Ì                                */
    73,                        /*   205    Í                                */
    73,                        /*   206    Î                                */
    73,                        /*   207    Ï                                */
    68,                        /*   208    Ð                                */
    93,                        /*   209    Ñ                                */
    79,                        /*   210    Ò                                */
    79,                        /*   211    Ó                                */
    79,                        /*   212    Ô                                */
    79,                        /*   213    Õ                                */
    92,                        /*   214    Ö                                */
    42,                        /*   215    ×                                */
    11,                        /*   216    Ø                                */
    85,                        /*   217    Ù                                */
    85,                        /*   218    Ú                                */
    85,                        /*   219    Û                                */
    94,                        /*   220    Ü                                */
    89,                        /*   221    Ý                                */
    NON_PRINTABLE_7_BIT,       /*   222    Þ                                */
    30,                        /*   223    ß                                */
    127,                       /*   224    à                                */
    97,                        /*   225    á                                */
    97,                        /*   226    â                                */
    97,                        /*   227    ã                                */
    123,                       /*   228    ä                                */
    15,                        /*   229    å                                */
    29,                        /*   230    æ                                */
    9,                         /*   231    ç                                */
    4,                         /*   232    è                                */
    5,                         /*   233    é                                */
    101,                       /*   234    ê                                */
    101,                       /*   235    ë                                */
    7,                         /*   236    ì                                */
    7,                         /*   237    í                                */
    105,                       /*   238    î                                */
    105,                       /*   239    ï                                */
    NON_PRINTABLE_7_BIT,       /*   240    ð                                */
    125,                       /*   241    ñ                                */
    8,                         /*   242    ò                                */
    111,                       /*   243    ó                                */
    111,                       /*   244    ô                                */
    111,                       /*   245    õ                                */
    24,                        /*   246    ö                                */
    47,                        /*   247    ÷                                */
    12,                        /*   248    ø                                */
    6,                         /*   249    ù                                */
    117,                       /*   250    ú                                */
    117,                       /*   251    û                                */
    126,                       /*   252    ü                                */
    121,                       /*   253    ý                                */
    NON_PRINTABLE_7_BIT,       /*   254    þ                                */
    121                        /*   255    ÿ                                */
};

const uint8_t ascii_7_to_8[] = {
    64,                        /*  0      @ */
    163,                       /*  1      £ */
    36,                        /*  2      $ */
    165,                       /*  3      ¥ */
    232,                       /*  4      è */
    233,                       /*  5      é */
    249,                       /*  6      ù */
    236,                       /*  7      ì */
    242,                       /*  8      ò */
    199,                       /*  9      Ç */
    10,                        /*  10       */
    216,                       /*  11     Ø */
    248,                       /*  12     ø */
    13,                        /*  13       */
    197,                       /*  14     Å */
    229,                       /*  15     å */
    NON_PRINTABLE_8_BIT,       /*  16       */
    95,                        /*  17     _ */
    NON_PRINTABLE_8_BIT,       /*  18       */
    NON_PRINTABLE_8_BIT,       /*  19       */
    NON_PRINTABLE_8_BIT,       /*  20       */
    NON_PRINTABLE_8_BIT,       /*  21       */
    NON_PRINTABLE_8_BIT,       /*  22       */
    NON_PRINTABLE_8_BIT,       /*  23       */
    NON_PRINTABLE_8_BIT,       /*  24       */
    NON_PRINTABLE_8_BIT,       /*  25       */
    NON_PRINTABLE_8_BIT,       /*  26       */
    27,                        /*  27       */
    198,                       /*  28     Æ */
    230,                       /*  29     æ */
    223,                       /*  30     ß */
    201,                       /*  31     É */
    32,                        /*  32       */
    33,                        /*  33     ! */
    34,                        /*  34     " */
    35,                        /*  35     # */
    164,                       /*  36     € */
    37,                        /*  37     % */
    38,                        /*  38     & */
    39,                        /*  39     ' */
    40,                        /*  40     ( */
    41,                        /*  41     ) */
    42,                        /*  42     * */
    43,                        /*  43     + */
    44,                        /*  44     , */
    45,                        /*  45     - */
    46,                        /*  46     . */
    47,                        /*  47     / */
    48,                        /*  48     0 */
    49,                        /*  49     1 */
    50,                        /*  50     2 */
    51,                        /*  51     3 */
    52,                        /*  52     4 */
    53,                        /*  53     5 */
    54,                        /*  54     6 */
    55,                        /*  55     7 */
    56,                        /*  56     8 */
    57,                        /*  57     9 */
    58,                        /*  58     : */
    59,                        /*  59     ; */
    60,                        /*  60     < */
    61,                        /*  61     = */
    62,                        /*  62     > */
    63,                        /*  63     ? */
    161,                       /*  64     ¡ */
    65,                        /*  65     A */
    66,                        /*  66     B */
    67,                        /*  67     C */
    68,                        /*  68     D */
    69,                        /*  69     E */
    70,                        /*  70     F */
    71,                        /*  71     G */
    72,                        /*  72     H */
    73,                        /*  73     I */
    74,                        /*  74     J */
    75,                        /*  75     K */
    76,                        /*  76     L */
    77,                        /*  77     M */
    78,                        /*  78     N */
    79,                        /*  79     O */
    80,                        /*  80     P */
    81,                        /*  81     Q */
    82,                        /*  82     R */
    83,                        /*  83     S */
    84,                        /*  84     T */
    85,                        /*  85     U */
    86,                        /*  86     V */
    87,                        /*  87     W */
    88,                        /*  88     X */
    89,                        /*  89     Y */
    90,                        /*  90     Z */
    196,                       /*  91     Ä */
    214,                       /*  92     Ö */
    209,                       /*  93     Ñ */
    220,                       /*  94     Ü */
    167,                       /*  95     § */
    191,                       /*  96     ¿ */
    97,                        /*  97     a */
    98,                        /*  98     b */
    99,                        /*  99     c */
    100,                       /*  100    d */
    101,                       /*  101    e */
    102,                       /*  102    f */
    103,                       /*  103    g */
    104,                       /*  104    h */
    105,                       /*  105    i */
    106,                       /*  106    j */
    107,                       /*  107    k */
    108,                       /*  108    l */
    109,                       /*  109    m */
    110,                       /*  110    n */
    111,                       /*  111    o */
    112,                       /*  112    p */
    113,                       /*  113    q */
    114,                       /*  114    r */
    115,                       /*  115    s */
    116,                       /*  116    t */
    117,                       /*  117    u */
    118,                       /*  118    v */
    119,                       /*  119    w */
    120,                       /*  120    x */
    121,                       /*  121    y */
    122,                       /*  122    z */
    228,                       /*  123    ä */
    246,                       /*  124    ö */
    241,                       /*  125    ñ */
    252,                       /*  126    ü */
    224                        /*  127    à */
};

uint8_t Get7BitsAtPos
(
    const uint8_t* buffer,
    uint32_t       pos
)
{
    uint8_t index = pos / 8;
    uint8_t offset = (pos % 8);

    uint8_t low_Byte = buffer[index] >> offset;
    uint8_t high_Byte = 0;

    if (offset > 0) {
        high_Byte = buffer[index + 1] << (8 - offset);
    }

    return (low_Byte | high_Byte) & BITMASK_7BITS;
}

void Set7BitsAtPos
(
    uint8_t* buffer,
    uint8_t  val,
    uint32_t pos
)
{
    val &= BITMASK_7BITS;

    uint8_t index = pos / 8;
    uint8_t offset = (pos % 8);

    if (offset == 0)
    {
        buffer[index] = val;
    }
    else if (offset == 1)
    {
        buffer[index] = buffer[index] | (val << 1);
    }
    else
    {
        buffer[index] = buffer[index] | (val << offset);
        buffer[index + 1] = (val >> (8 - offset));
    }
}

void SetByteAtPos
(
    uint8_t* buf,
    uint8_t  pos,
    uint8_t  val
)
{
    buf[pos] = val;
}

uint8_t GetByteAtPos
(
    const uint8_t* buf,
    uint8_t        pos
)
{
    return buf[pos];
}

uint8_t pduDecodeAddr(const unsigned char* buffer, uint8_t addrLen, char* outputAddr)
{
    LE_DEBUG("pduDecodeAddr");

    uint idx = 0;

    LE_DEBUG("buffer[0]: 0x%.2X", (int)buffer[0]);

    if(buffer[0] == 0x91)
    {
        outputAddr[idx++] = '+';
    }

    for (uint8_t i = 0; i < addrLen; ++i)
    {
        uint8_t byte = buffer[(i / 2) + 1];
        char nibble;

        if (i % 2 == 0)
        {
            nibble = (byte & BITMASK_LOW_4BITS) + '0';
        }
        else
        {
            nibble = ((byte & BITMASK_HIGH_4BITS) >> 4) + '0';
        }

        if (nibble > '9')
        {
            nibble += 'A' - '0' - 10;
        }

        if (nibble == 'F')
        {
            outputAddr[idx++] = '\0';
            break;
        }
        else
        {
            outputAddr[idx++] = nibble;
        }
        LE_DEBUG("outputAddr: %s", outputAddr);
    }

    outputAddr[idx] = '\0';

    LE_INFO("outputAddr: %s", outputAddr);

    return addrLen;
}

int32_t pduDecode7BitsTo8Bits
(
    const uint8_t *bufferIn_7bit,
    uint8_t       len,
    uint8_t       *bufferOut_8bit,
    size_t        bufferOut_Size
)
{
    uint r_indx = 0;
    uint w_indx = 0;

    for (r_indx = 0; r_indx < len; r_indx++)
    {
        uint8_t byte = Get7BitsAtPos(bufferIn_7bit, r_indx * 7);
        byte = ascii_7_to_8[byte];

        if (byte != ESCAPE_7BIT)
        {
            TAF_ERROR_IF_RET_VAL(w_indx >= bufferOut_Size, LE_OVERFLOW, "Decode 7BitsTo8Bits overflow");

            bufferOut_8bit[w_indx] = byte;
            w_indx++;

        }
        else    //escape symbol for next byte
        {
            r_indx++;

            byte = Get7BitsAtPos(bufferIn_7bit, r_indx * 7);

            TAF_ERROR_IF_RET_VAL(w_indx >= bufferOut_Size, LE_OVERFLOW, "Decode 7BitsTo8Bits overflow");

            switch (byte)
            {
                case FORM_FEED_7BIT_ESC:
                    bufferOut_8bit[w_indx] = FORM_FEED_8BIT;
                    break;
                case CIRCUMFLEX_ACCENT_7BIT_ESC:
                    bufferOut_8bit[w_indx] = '^';
                    break;
                case LEFT_CURLY_BRACKET_7BIT_ESC:
                    bufferOut_8bit[w_indx] = '{';
                    break;
                case RIGHT_CURLY_BRACKET_7BIT_ESC:
                    bufferOut_8bit[w_indx] = '}';
                    break;
                case REVERSE_SOLIDUS_7BIT_ESC:
                    bufferOut_8bit[w_indx] = '\\';
                    break;
                case LEFT_SQUARE_BRACKET_7BIT_ESC:
                    bufferOut_8bit[w_indx] = '[';
                    break;
                case TILDE_7BIT_ESC:
                    bufferOut_8bit[w_indx] = '~';
                    break;
                case RIGHT_SQUARE_BRACKET_7BIT_ESC:
                    bufferOut_8bit[w_indx] = ']';
                    break;
                case VERTICAL_BAR_ESC:
                    bufferOut_8bit[w_indx] = '|';
                    break;
                default:
                    bufferOut_8bit[w_indx] = NON_PRINTABLE_8_BIT;
                    break;
            }
            w_indx++;

        }
    }

    return w_indx;
}

static int16_t pduDecodeUserData
(
    pdu_Encoding_t encoding,
    const unsigned char* buffer,
    char* userData,
    uint8_t userDataLen
)
{
    LE_DEBUG("pduDecodeUserData");

    uint8_t outputDataLen = 0;

    switch(encoding)
    {
        case PDU_ENCODING_7_BITS:
        {
            outputDataLen = pduDecode7BitsTo8Bits((uint8_t*)buffer, userDataLen, (uint8_t*)userData, TAF_SMS_TEXT_BYTES);

            break;
        }

        case PDU_ENCODING_8_BITS:
        {
            outputDataLen = userDataLen;
            memcpy(userData, buffer, userDataLen);

            break;
        }

        case PDU_ENCODING_16_BITS:
        {
            outputDataLen = userDataLen;
            memcpy(userData, buffer, userDataLen);

            break;
        }

        default:
            return 0;

    }

    return outputDataLen;
}

static pdu_Encoding_t getDataCodingScheme
(
    uint8_t smsDcs
)
{
    LE_DEBUG("getDataCodingScheme");

    pdu_Encoding_t encoding = PDU_ENCODING_UNKNOWN;

    if ((smsDcs >> 4) == 0xF)
    {
        encoding = (pdu_Encoding_t)((smsDcs >> 2) & 1);
    }
    else if ((smsDcs >> 6) == 0)
    {
        encoding = (pdu_Encoding_t)((smsDcs >> 2) & 0x3);
    }
    else
    {
        LE_DEBUG("encoding is not supported");
        return PDU_ENCODING_UNKNOWN;
    }

    return encoding;
}

le_result_t sms_DecodeDeliver
(
    const uint8_t*    dataPtr,
    sms_PduMsg_t*     smsPduPtr
)
{
    LE_DEBUG("sms_DecodeDeliver");

    const uint8_t pos_smsDeliver = 1 + GetByteAtPos(dataPtr, 0);

    const uint8_t smsAddrLen =  GetByteAtPos(dataPtr, pos_smsDeliver + 1);
    const uint8_t pos_smsAddr = pos_smsDeliver + 2;

 TAF_ERROR_IF_RET_VAL((size_t)smsAddrLen + TAF_SMS_ADDR_PLUS_CHARS + TAF_SMS_ADDR_NULL_CHARS > sizeof(smsPduPtr->addr),
                         LE_OVERFLOW, "addr size overflow");

    pduDecodeAddr(dataPtr + pos_smsAddr, smsAddrLen, smsPduPtr->addr);

    const uint8_t pos_smsPid = pos_smsDeliver + 3 + (dataPtr[pos_smsDeliver + 1] + 1) / 2;

    const uint8_t pos_smsDcs = pos_smsPid + 1;

    pdu_Encoding_t encoding = getDataCodingScheme((uint8_t)dataPtr[pos_smsDcs]);

    TAF_ERROR_IF_RET_VAL(encoding == PDU_ENCODING_UNKNOWN, LE_UNSUPPORTED, "unsupported encoding");

    smsPduPtr->encoding = encoding;

    const uint8_t pos_dataLen = pos_smsDcs + 7 + 1;

    const uint8_t smsUdl = GetByteAtPos(dataPtr, pos_dataLen);

    LE_DEBUG("smsUdl: %d", smsUdl);

    TAF_ERROR_IF_RET_VAL(sizeof(smsPduPtr->data) < smsUdl, LE_FAULT, "data size is not enough for decoding");

    const uint8_t pos_Data = pos_dataLen + 1;

    const int16_t decodedContentSize = pduDecodeUserData(encoding,
                                                         dataPtr + pos_Data,
                                                         smsPduPtr->data,
                                                         smsUdl);

    LE_DEBUG("decodedContentSize: %d", decodedContentSize);
    LE_DEBUG("smsPduPtr->data: %s", smsPduPtr->data);

    /*TAF_ERROR_IF_RET_VAL(decodedContentSize != smsUdl, LE_FAULT,
                         "decoded content length(%d) doesn't match smsUdl(%d)", decodedContentSize, smsUdl);*/

    smsPduPtr->dataLen = decodedContentSize;

    return LE_OK;
}

le_result_t sms_DecodeGsm
(
    const uint8_t*    dataPtr,
    sms_PduMsg_t*     smsPduPtr
)
{
    le_result_t result;

    memset(smsPduPtr, 0, sizeof(sms_PduMsg_t));

    const uint8_t pos_smsDeliver = 1 + GetByteAtPos(dataPtr, 0);
    const uint8_t smsType = GetByteAtPos(dataPtr, pos_smsDeliver);

    LE_DEBUG("smsType: 0x%.2x", smsType);

    if((smsType & TP_TYPE_MASK) == TP_TYPE_SMS_DELIVER)
    {
        LE_DEBUG("SMS_TYPE_DELIVER");
        smsPduPtr->type = SMS_TYPE_DELIVER;
        result = sms_DecodeDeliver(dataPtr, smsPduPtr);
    }
    else
    {
        result = LE_UNSUPPORTED;
    }

    return result;
}

le_result_t smsPdu_Decode
(
    sms_Protocol_t    protocol,
    const uint8_t*    dataPtr,
    sms_PduMsg_t*     smsPduPtr
)
{
    le_result_t result = LE_OK;

    LE_DEBUG("protocol: %d", protocol);

    switch(protocol)
    {
        case SMS_PROTOCOL_GSM:
            LE_DEBUG("SMS_PROTOCOL_GSM");
            result = sms_DecodeGsm(dataPtr, smsPduPtr);
            break;

        case SMS_PROTOCOL_CDMA:
            LE_DEBUG("SMS_PROTOCOL_CDMA not supported");
            result = LE_UNSUPPORTED;
            break;

        case SMS_PROTOCOL_GW_CB:
            LE_DEBUG("SMS_PROTOCOL_GW_CB not supported");
            result = LE_UNSUPPORTED;
            break;

        default:
            result = LE_UNSUPPORTED;
            break;
    }

    return result;
}

uint8_t pduEncode8BitsTo7Bits
(
    const uint8_t *bufferIn_8bit,
    uint8_t       pos,
    uint8_t       len,
    uint8_t       *bufferOut_7bit,
    size_t        bufferOut_Size,
    uint8_t       *bufferOut_Write
)
{
    LE_DEBUG("pduEncode8BitsTo7Bits");

    uint8_t numOfwriteChar = 0;
    uint8_t numOf8bitChar = 0;

    for (uint8_t i = pos; i < len + pos; i++)
    {
        LE_DEBUG("encode char: '%c'", bufferIn_8bit[i]);

        uint8_t byte = ascii_8_to_7[bufferIn_8bit[i]];

        /* escape symbol for next byte */
        if (byte >= ESCAPE_SHIFT_INDEX)
        {
            TAF_ERROR_IF_RET_VAL(numOf8bitChar > bufferOut_Size, LE_OVERFLOW, "encode msg overflow");

            Set7BitsAtPos(bufferOut_7bit, ESCAPE_7BIT, numOfwriteChar * 7);
            numOfwriteChar++;
            byte -= ESCAPE_SHIFT_INDEX;
        }

        TAF_ERROR_IF_RET_VAL(numOf8bitChar > bufferOut_Size, LE_OVERFLOW, "encode msg overflow");

        Set7BitsAtPos(bufferOut_7bit, byte, numOfwriteChar * 7);
        numOfwriteChar++;

        uint32_t numOfbit = (numOfwriteChar * 7);
        numOf8bitChar = (numOfbit / 8);

        if(numOfbit % 8 != 0)
        {
            numOf8bitChar++;
        }
    }

    TAF_ERROR_IF_RET_VAL(numOf8bitChar > bufferOut_Size, LE_OVERFLOW, "overflow: numOf8bitChar > bufferOut_Size");

    *bufferOut_Write = numOfwriteChar;
    LE_DEBUG("numOfwriteChar = %u", numOfwriteChar);

    return numOf8bitChar;
}

size_t pduEncodeAddr
(
    const char* addrPtr,
    uint8_t*    bufferOut,
    size_t      size
)
{

    char number[TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES] = {0};
    size_t length = strlen(addrPtr);

    if (addrPtr[0] == '+')
    {
        length--;
        addrPtr++;
    }

    uint numLen = 0;
    for(uint i = 0; i < length; i += 2)
    {
        number[i + 1] = addrPtr[i];

        if((i + 1) < length)
        {
            number[i] = addrPtr[i + 1];
        }
        else
        {
            number[i] = 'F';
        }
        numLen +=2;
    }

    le_hex_StringToBinary(number, numLen, bufferOut, size);

    return length;
}

le_result_t sms_EncodeGsm
(
    smsPdu_EncodeMsg_t*    sms,
    taf_sms_Pdu_t*         smsPdu
)
{
    TAF_ERROR_IF_RET_VAL(sms->msgDataLen > TAF_SMS_TEXT_LEN, LE_FAULT, "msg length > %d", TAF_SMS_TEXT_LEN);

    memset(smsPdu->data, 0x00, sizeof(smsPdu->data));

    uint8_t pduType = 0x00;

    switch (sms->type)
    {
        case TP_TYPE_SMS_SUBMIT:

            pduType = PDU_TYPE_TP_MTI_SMS_SUBMIT | PDU_TYPE_TP_VP_A_INTEGER;
            if (sms->statusReport)
            {
                pduType |= PDU_TYPE_TP_SRR;
            }
            break;

        default:
            TAF_ERROR_IF_RET_VAL(true, LE_UNSUPPORTED, "Msg type is not supported");
    }

    uint addrLen = strlen(sms->addrData);

    TAF_ERROR_IF_RET_VAL(addrLen > TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES, LE_FAULT,
                         "Address length (%d) > %d", addrLen, TAF_TYPES_REMOTE_PARTY_NUM_MAX_BYTES);

    uint8_t addrToa = TYPE_ADDRESS_UNKNOWN;

    if ('+' == sms->addrData[0])
    {
        addrLen--;
        addrToa = TYPE_ADDRESS_INTERNATIONAL;
    }

    uint8_t tpDcs = 0x00;

    switch (sms->encoding)
    {
        case PDU_ENCODING_7_BITS:

            tpDcs = TP_DCS_7_BIT;
            break;

        case PDU_ENCODING_8_BITS:

            tpDcs = TP_DCS_8_BIT;
            break;

        case PDU_ENCODING_16_BITS:

            tpDcs = TP_DCS_16_BIT;
            break;

        default:
            TAF_ERROR_IF_RET_VAL(true, LE_FAULT, "Invalid encoding");
    }

    uint8_t pos = 0;

    /* SMSC */
    SetByteAtPos(smsPdu->data, pos++, 0x00);

    /* PDU type */
    SetByteAtPos(smsPdu->data, pos++, pduType);

    if (sms->type == TP_TYPE_SMS_SUBMIT)
    {
        SetByteAtPos(smsPdu->data, pos++, 0x00);
    }

    /* First Byte */
    SetByteAtPos(smsPdu->data, pos++, addrLen);

    /* TP-DA */
    SetByteAtPos(smsPdu->data, pos++, addrToa);
    pos += (pduEncodeAddr(sms->addrData, &smsPdu->data[pos], TAF_SMS_PDU_BYTES - pos) + 1) / 2;

    /* TP-PID */
    SetByteAtPos(smsPdu->data, pos++, 0x00);

    /* TP-DCS */
    SetByteAtPos(smsPdu->data, pos++, tpDcs);

    if (sms->type == TP_TYPE_SMS_SUBMIT)
    {
        /* TP-VP: default set to 7 days, it can be changed */
        SetByteAtPos(smsPdu->data, pos++, 0xAD);
    }

    /* TP-UDL */
    uint msgLen = sms->msgDataLen;

    /* TP-UD */
    switch (sms->encoding)
    {
        case PDU_ENCODING_7_BITS:
        {
            uint8_t newmsgLen;
            uint8_t size = pduEncode8BitsTo7Bits(sms->msgData,
                                                0,
                                                msgLen,
                                                &smsPdu->data[pos + 1],
                                                TAF_SMS_PDU_PAYLOAD,
                                                &newmsgLen);

            TAF_ERROR_IF_RET_VAL(size == LE_OVERFLOW, LE_OVERFLOW, "Overflow - encoding 7-bit PDU");

            SetByteAtPos(smsPdu->data, pos++, newmsgLen);
            pos += size;

            break;
        }
        case PDU_ENCODING_8_BITS:
        {
            TAF_ERROR_IF_RET_VAL(msgLen > TAF_SMS_PDU_PAYLOAD, LE_OVERFLOW, "Overflow - encoding 8-bit PDU");

            SetByteAtPos(smsPdu->data, pos++, msgLen);
            memcpy(&smsPdu->data[pos], sms->msgData, msgLen);
            pos += msgLen;

            break;
        }
        case PDU_ENCODING_16_BITS:
        {
            TAF_ERROR_IF_RET_VAL(msgLen > TAF_SMS_PDU_PAYLOAD, LE_OVERFLOW, "Overflow - encoding 16-bit PDU");

            SetByteAtPos(smsPdu->data, pos++, msgLen);
            memcpy(&smsPdu->data[pos], sms->msgData, msgLen);
            pos += msgLen;

            break;
        }

        default:
            TAF_ERROR_IF_RET_VAL(true, LE_UNSUPPORTED, "Invalid encoding");
    }

    smsPdu->length = pos;
    LE_DEBUG("smsPdu->length = %u", smsPdu->length);

    return LE_OK;
}

le_result_t smsPdu_Encode
(
    smsPdu_EncodeMsg_t*    sms,
    taf_sms_Pdu_t*         smsPdu
)
{
    le_result_t result;

    switch (sms->protocol)
    {
        case SMS_PROTOCOL_GSM:
            LE_DEBUG("SMS_PROTOCOL_GSM");
            result = sms_EncodeGsm(sms, smsPdu);
            break;

        case SMS_PROTOCOL_CDMA:
            LE_DEBUG("SMS_PROTOCOL_CDMA not supported");
            result = LE_UNSUPPORTED;
            break;

        default:
            LE_WARN("Protocol %d not supported", sms->protocol);
            result = LE_UNSUPPORTED;
            break;
    }

    return result;
}

