/* huffman.c */
 
#include "types.h"
     
HUFFBITS dmask = 1 << (((sizeof(HUFFBITS))<<3)-1);
unsigned int hs = sizeof(HUFFBITS)<<3;

static const HUFFBITS      t1HB[]   = {1, 1, 1, 0}; 
static const HUFFBITS      t2HB[]   = {1, 2, 1, 3, 1, 1, 3, 2, 0};
static const HUFFBITS      t3HB[]   = {3, 2, 1, 1, 1, 1, 3, 2, 0};
static const HUFFBITS      t5HB[]   = {1, 2, 6, 5, 3, 1, 4, 4, 7, 5, 7, 1, 6, 1, 1, 0};
static const HUFFBITS      t6HB[]   = {7, 3, 5, 1, 6, 2, 3, 2, 5, 4, 4, 1, 3, 3, 2, 0};
static const HUFFBITS      t7HB[]   = {1, 2, 10, 19, 16, 10, 3, 3, 7, 10, 5, 3, 11, 4, 13, 17, 8, 4, 12, 11, 18, 15, 11, 2, 7, 6, 9, 14, 3, 1, 6, 4, 5, 3, 2, 0};
static const HUFFBITS      t8HB[]   = {3, 4, 6, 18, 12, 5, 5, 1, 2, 16, 9, 3, 7, 3, 5, 14, 7, 3, 19, 17, 15, 13, 10, 4, 13, 5, 8, 11, 5, 1, 12, 4, 4, 1, 1, 0};
static const HUFFBITS      t9HB[]   = {7, 5, 9, 14, 15, 7, 6, 4, 5, 5, 6, 7, 7, 6, 8, 8, 8, 5, 15, 6, 9, 10, 5, 1, 11, 7, 9, 6, 4, 1, 14, 4, 6, 2, 6, 0};
static const HUFFBITS      t10HB[]   = {1, 2, 10, 23, 35, 30, 12, 17, 3, 3, 8, 12, 18, 21, 12, 7, 11, 9, 15, 21, 32, 40, 19, 6, 14, 13, 22, 34, 46, 23, 18, 7, 20, 19, 33, 47, 27, 22, 9, 3, 31, 22, 41, 26, 21, 20, 5, 3, 14, 13, 10, 11, 16, 6, 5, 1, 9, 8, 7, 8, 4 , 4, 2, 0};
static const HUFFBITS      t11HB[]   = {3, 4, 10, 24, 34, 33, 21, 15, 5, 3, 4, 10, 32, 17, 11, 10, 11, 7, 13, 18, 30, 31, 20, 5, 25, 11, 19, 59, 27, 18, 12, 5, 35, 33, 31, 58, 30, 16, 7, 5, 28, 26, 32, 19, 17, 15, 8, 14, 14, 12, 9, 13, 14, 9, 4, 1, 11, 4, 6, 6, 6, 3, 2, 0};
static const HUFFBITS      t12HB[]   = {9, 6, 16, 33, 41, 39, 38, 26, 7, 5, 6, 9, 23, 16, 26, 11, 17, 7, 11, 14, 21, 30, 10, 7, 17, 10, 15, 12, 18, 28, 14, 5, 32, 13, 22, 19, 18, 16, 9, 5, 40, 17, 31, 29, 17, 13, 4, 2, 27, 12, 11, 15, 10, 7, 4, 1, 27, 12, 8, 12 , 6, 3, 1, 0}; 
static const HUFFBITS      t13HB[]   = {1, 5, 14, 21, 34, 51, 46, 71, 42, 52, 68, 52, 67, 44, 43, 19, 3, 4, 12, 19, 31, 26, 44, 33, 31, 24, 32, 24, 31, 35, 22, 14, 15, 13, 23, 36, 59, 49, 77, 65, 29, 40, 30, 40, 27, 33, 42, 16, 22,
                                 20, 37, 61, 56, 79, 73, 64, 43, 76, 56, 37, 26, 31, 25, 14, 35, 16, 60, 57, 97, 75, 114, 91, 54, 73, 55, 41, 48, 53, 23, 24, 58, 27, 50, 96, 76, 70, 93, 84, 77, 58, 79, 29, 74, 49, 41, 17, 47, 
                                 45, 78, 74, 115, 94, 90, 79, 69, 83, 71, 50, 59, 38, 36, 15, 72, 34, 56, 95, 92, 85, 91, 90, 86, 73, 77, 65, 51, 44, 43, 42, 43, 20, 30, 44, 55, 78, 72, 87, 78, 61, 46, 54, 37, 30, 20, 16, 53, 
                                 25, 41, 37, 44, 59, 54, 81, 66, 76, 57, 54, 37, 18, 39, 11, 35, 33, 31, 57, 42, 82, 72, 80, 47, 58, 55, 21, 22, 26, 38, 22, 53, 25, 23, 38, 70, 60, 51, 36, 55, 26, 34, 23, 27, 14, 9, 7, 34, 32, 
                                 28, 39, 49, 75, 30, 52, 48, 40, 52, 28, 18, 17, 9, 5, 45, 21, 34, 64, 56, 50, 49, 45, 31, 19, 12, 15, 10, 7, 6, 3, 48, 23, 20, 39, 36, 35, 53, 21, 16, 23, 13, 10, 6, 1, 4, 2, 16, 15, 17, 27, 25, 
                                 20, 29, 11, 17, 12, 16, 8, 1, 1, 0, 1};
static const HUFFBITS      t15HB[]   = {7, 12, 18, 53, 47, 76, 124, 108, 89, 123, 108, 119, 107, 81, 122, 63, 13, 5, 16, 27, 46, 36, 61, 51, 42, 70, 52, 83, 65, 41, 59, 36, 19, 17, 15, 24, 41, 34, 59, 48, 40, 64, 50, 78, 62, 80, 56, 
                                  33, 29, 28, 25, 43, 39, 63, 55, 93, 76, 59, 93, 72, 54, 75, 50, 29, 52, 22, 42, 40, 67, 57, 95, 79, 72, 57, 89, 69, 49, 66, 46, 27, 77, 37, 35, 66, 58, 52, 91, 74, 62, 48, 79, 63, 90, 62, 40, 38,
                                  125, 32, 60, 56, 50, 92, 78, 65, 55, 87, 71, 51, 73, 51, 70, 30, 109, 53, 49, 94, 88, 75, 66, 122, 91, 73, 56, 42, 64, 44, 21, 25, 90, 43, 41, 77, 73, 63, 56, 92, 77, 66, 47, 67, 48, 53, 36, 20,
                                  71, 34, 67, 60, 58, 49, 88, 76, 67, 106, 71, 54, 38, 39, 23, 15, 109, 53, 51, 47, 90, 82, 58, 57, 48, 72, 57, 41, 23, 27, 62, 9, 86, 42, 40, 37, 70, 64, 52, 43, 70, 55, 42, 25, 29, 18, 11, 11, 
                                  118, 68, 30, 55, 50, 46, 74, 65, 49, 39, 24, 16, 22, 13, 14, 7, 91, 44, 39, 38, 34, 63, 52, 45, 31, 52, 28, 19, 14, 8, 9, 3, 123, 60, 58, 53, 47, 43, 32, 22, 37, 24, 17, 12, 15, 10, 2, 1, 71, 
                                  37, 34, 30, 28, 20, 17, 26, 21, 16, 10, 6, 8, 6, 2, 0};
static const HUFFBITS      t16HB[]   = {1, 5, 14, 44, 74, 63, 110, 93, 172, 149, 138, 242, 225, 195, 376, 17, 3, 4, 12, 20, 35, 62, 53, 47, 83, 75, 68, 119, 201, 107, 207, 9, 15, 13, 23, 38, 67, 58, 103, 90, 161, 72, 127, 117, 
                                  110, 209, 206, 16, 45, 21, 39, 69, 64, 114, 99, 87, 158, 140, 252, 212, 199, 387, 365, 26, 75, 36, 68, 65, 115, 101, 179, 164, 155, 264, 246, 226, 395, 382, 362, 9, 66, 30, 59, 56, 102,
                                  185, 173, 265, 142, 253, 232, 400, 388, 378, 445, 16, 111, 54, 52, 100, 184, 178, 160, 133, 257, 244, 228, 217, 385, 366, 715, 10, 98, 48, 91, 88, 165, 157, 148, 261, 248, 407, 397, 372,
                                  380, 889, 884, 8, 85, 84, 81, 159, 156, 143, 260, 249, 427, 401, 392, 383, 727, 713, 708, 7, 154, 76, 73, 141, 131, 256, 245, 426, 406, 394, 384, 735, 359, 710, 352, 11, 139, 129, 67, 125,
                                  247, 233, 229, 219, 393, 743, 737, 720, 885, 882, 439, 4, 243, 120, 118, 115, 227, 223, 396, 746, 742, 736, 721, 712, 706, 223, 436, 6, 202, 224, 222, 218, 216, 389, 386, 381, 364, 888, 
                                  443, 707, 440, 437, 1728, 4, 747, 211, 210, 208, 370, 379, 734, 723, 714, 1735, 883, 877, 876, 3459, 865, 2, 377, 369, 102, 187, 726, 722, 358, 711, 709, 866, 1734, 871, 3458, 870, 434, 
                                  0, 12, 10, 7, 11, 10, 17, 11, 9, 13, 12, 10, 7, 5, 3, 1, 3};
static const HUFFBITS      t24HB[]   = {15, 13, 46, 80, 146, 262, 248, 434, 426, 669, 653, 649, 621, 517, 1032, 88, 14, 12, 21, 38, 71, 130, 122, 216, 209, 198, 327, 345, 319, 297, 279, 42, 47, 22, 41, 74, 68, 128, 120, 221,
                                  207, 194, 182, 340, 315, 295, 541, 18, 81, 39, 75, 70, 134, 125, 116, 220, 204, 190, 178, 325, 311, 293, 271, 16, 147, 72, 69, 135, 127, 118, 112, 210, 200, 188, 352, 323, 306, 285, 
                                  540, 14, 263, 66, 129, 126, 119, 114, 214, 202, 192, 180, 341, 317, 301, 281, 262, 12, 249, 123, 121, 117, 113, 215, 206, 195, 185, 347, 330, 308, 291, 272, 520, 10, 435, 115, 111,
                                  109, 211, 203, 196, 187, 353, 332, 313, 298, 283, 531, 381, 17, 427, 212, 208, 205, 201, 193, 186, 177, 169, 320, 303, 286, 268, 514, 377, 16, 335, 199, 197, 191, 189, 181, 174, 333, 
                                  321, 305, 289, 275, 521, 379, 371, 11, 668, 184, 183, 179, 175, 344, 331, 314, 304, 290, 277, 530, 383, 373, 366, 10, 652, 346, 171, 168, 164, 318, 309, 299, 287, 276, 263, 513, 375, 
                                  368, 362, 6, 648, 322, 316, 312, 307, 302, 292, 284, 269, 261, 512, 376, 370, 364, 359, 4, 620, 300, 296, 294, 288, 282, 273, 266, 515, 380, 374, 369, 365, 361, 357, 2, 1033, 280, 278,
                                  274, 267, 264, 259, 382, 378, 372, 367, 363, 360, 358, 356, 0, 43, 20, 19, 17, 15, 13, 11, 9, 7, 6, 4, 7, 5, 3, 1, 3};
static const HUFFBITS      t32HB[]   = {1, 5, 4, 5, 6, 5, 4, 4, 7, 3, 6, 0, 7, 2, 3, 1};
static const HUFFBITS      t33HB[]   = {15, 14, 13, 12, 11, 10, 9, 8, 7, 6, 5, 4, 3, 2, 1, 0};

static const unsigned char t1l[]  = {1, 3, 2, 3}; 
static const unsigned char t2l[]  = {1, 3, 6, 3, 3, 5, 5, 5, 6}; 
static const unsigned char t3l[]  = {2, 2, 6, 3, 2, 5, 5, 5, 6}; 
static const unsigned char t5l[]  = {1, 3, 6, 7, 3, 3, 6, 7, 6, 6, 7, 8, 7, 6, 7, 8}; 
static const unsigned char t6l[]  = {3, 3, 5, 7, 3, 2, 4, 5, 4, 4, 5, 6, 6, 5, 6, 7}; 
static const unsigned char t7l[]  = {1, 3, 6, 8, 8, 9, 3, 4, 6, 7, 7, 8, 6, 5, 7, 8, 8, 9, 7, 7, 8, 9, 9, 9, 7, 7, 8, 9, 9, 10, 8, 8, 9, 10, 10, 10}; 
static const unsigned char t8l[]  = {2, 3, 6, 8, 8, 9, 3, 2, 4, 8, 8, 8, 6, 4, 6, 8, 8, 9, 8, 8, 8, 9, 9, 10, 8, 7, 8, 9, 10, 10, 9, 8, 9, 9, 11, 11}; 
static const unsigned char t9l[]  = {3, 3, 5, 6, 8, 9, 3, 3, 4, 5, 6, 8, 4, 4, 5, 6, 7, 8, 6, 5, 6, 7, 7, 8, 7, 6, 7, 7, 8, 9, 8, 7, 8, 8, 9, 9}; 
static const unsigned char t10l[]  = {1, 3, 6, 8, 9, 9, 9, 10, 3, 4, 6, 7, 8, 9, 8, 8, 6, 6, 7, 8, 9, 10, 9, 9, 7, 7, 8, 9, 10, 10, 9, 10, 8, 8, 9, 10, 10, 10, 10, 10, 9, 9, 10, 10, 11, 11, 10, 11, 8, 8, 9, 10, 10, 10, 11, 11, 9, 8, 9, 10, 10, 11, 11, 11}; 
static const unsigned char t11l[]  = {2, 3, 5, 7, 8, 9, 8, 9, 3, 3, 4, 6, 8, 8, 7, 8, 5, 5, 6, 7, 8, 9, 8, 8, 7, 6, 7, 9, 8, 10, 8, 9, 8, 8, 8, 9, 9, 10, 9, 10, 8, 8, 9, 10, 10, 11, 10, 11, 8, 7, 7, 8, 9, 10, 10, 10, 8, 7, 8, 9, 10, 10, 10, 10}; 
static const unsigned char t12l[]  = {4, 3, 5, 7, 8, 9, 9, 9, 3, 3, 4, 5, 7, 7, 8, 8, 5, 4, 5, 6, 7, 8, 7, 8, 6, 5, 6, 6, 7, 8, 8, 8, 7, 6, 7, 7, 8, 8, 8, 9, 8, 7, 8, 8, 8, 9, 8, 9, 8, 7, 7, 8, 8, 9, 9, 10, 9, 8, 8, 9, 9, 9, 9, 10}; 
static const unsigned char t13l[]  = {1, 4, 6, 7, 8, 9, 9, 10, 9, 10, 11, 11, 12, 12, 13, 13, 3, 4, 6, 7, 8, 8, 9, 9, 9, 9, 10, 10, 11, 12, 12, 12, 6, 6, 7, 8, 9, 9, 10, 10, 9, 10, 10, 11, 11, 12, 13, 13, 7, 7, 8, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11, 12, 13, 13, 
8, 7, 9, 9, 10, 10, 11, 11, 10, 11, 11, 12, 12, 13, 13, 14, 9, 8, 9, 10, 10, 10, 11, 11, 11, 11, 12, 11, 13, 13, 14, 14, 9, 9, 10, 10, 11, 11, 11, 11, 11, 12, 12, 12, 13, 13, 14, 14, 10, 9, 10, 11, 11, 11, 12, 12, 12, 12, 13, 13, 13, 14, 16, 16, 9, 8, 9, 10,
10, 11, 11, 12, 12, 12, 12, 13, 13, 14, 15, 15, 10, 9, 10, 10, 11, 11, 11, 13, 12, 13, 13, 14, 14, 14, 16, 15, 10, 10, 10, 11, 11, 12, 12, 13, 12, 13, 14, 13, 14, 15, 16, 17, 11, 10, 10, 11, 12, 12, 12, 12, 13, 13, 13, 14, 15, 15, 15, 16, 11, 11, 11, 12, 12, 
13, 12, 13, 14, 14, 15, 15, 15, 16, 16, 16, 12, 11, 12, 13, 13, 13, 14, 14, 14, 14, 14, 15, 16, 15, 16, 16, 13, 12, 12, 13, 13, 13, 15, 14, 14, 17, 15, 15, 15, 17, 16, 16, 12, 12, 13, 14, 14, 14, 15, 14, 15, 15, 16, 16, 19, 18, 19, 16}; 
static const unsigned char t15l[]  = {3, 4, 5, 7, 7, 8, 9, 9, 9, 10, 10, 11, 11, 11, 12, 13, 4, 3, 5, 6, 7, 7, 8, 8, 8, 9, 9, 10, 10, 10, 11, 11, 5, 5, 5, 6, 7, 7, 8, 8, 8, 9, 9, 10, 10, 11, 11, 11, 6, 6, 6, 7, 7, 8, 8, 9, 9, 9, 10, 10, 10, 11, 11, 11, 7, 6, 7,
 7, 8, 8, 9, 9, 9, 9, 10, 10, 10, 11, 11, 11, 8, 7, 7, 8, 8, 8, 9, 9, 9, 9, 10, 10, 11, 11, 11, 12, 9, 7, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 11, 11, 12, 12, 9, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 10, 11, 11, 11, 12, 9, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11,
 12, 12, 12, 9, 8, 9, 9, 9, 9, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 10, 9, 9, 9, 10, 10, 10, 10, 10, 11, 11, 11, 11, 12, 13, 12, 10, 9, 9, 9, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 13, 11, 10, 9, 10, 10, 10, 11, 11, 11, 11, 11, 11, 12, 12, 13, 13, 
11, 10, 10, 10, 10, 11, 11, 11, 11, 12, 12, 12, 12, 12, 13, 13, 12, 11, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 12, 13, 12, 11, 11, 11, 11, 11, 11, 12, 12, 12, 12, 12, 13, 13, 13, 13}; 
static const unsigned char t16l[]  = {1, 4, 6, 8, 9, 9, 10, 10, 11, 11, 11, 12, 12, 12, 13, 9, 3, 4, 6, 7, 8, 9, 9, 9, 10, 10, 10, 11, 12, 11, 12, 8, 6, 6, 7, 8, 9, 9, 10, 10, 11, 10, 11, 11, 11, 12, 12, 9, 8, 7, 8, 9, 9, 10, 10, 10, 11, 11, 12, 12, 12, 13, 13,
 10, 9, 8, 9, 9, 10, 10, 11, 11, 11, 12, 12, 12, 13, 13, 13, 9, 9, 8, 9, 9, 10, 11, 11, 12, 11, 12, 12, 13, 13, 13, 14, 10, 10, 9, 9, 10, 11, 11, 11, 11, 12, 12, 12, 12, 13, 13, 14, 10, 10, 9, 10, 10, 11, 11, 11, 12, 12, 13, 13, 13, 13, 15, 15, 10, 10, 10,
 10, 11, 11, 11, 12, 12, 13, 13, 13, 13, 14, 14, 14, 10, 11, 10, 10, 11, 11, 12, 12, 13, 13, 13, 13, 14, 13, 14, 13, 11, 11, 11, 10, 11, 12, 12, 12, 12, 13, 14, 14, 14, 15, 15, 14, 10, 12, 11, 11, 11, 12, 12, 13, 14, 14, 14, 14, 14, 14, 13, 14, 11, 12, 12,
 12, 12, 12, 13, 13, 13, 13, 15, 14, 14, 14, 14, 16, 11, 14, 12, 12, 12, 13, 13, 14, 14, 14, 16, 15, 15, 15, 17, 15, 11, 13, 13, 11, 12, 14, 14, 13, 14, 14, 15, 16, 15, 17, 15, 14, 11, 9, 8, 8, 9, 9, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 8}; 
static const unsigned char t24l[]  = {4, 4, 6, 7, 8, 9, 9, 10, 10, 11, 11, 11, 11, 11, 12, 9, 4, 4, 5, 6, 7, 8, 8, 9, 9, 9, 10, 10, 10, 10, 10, 8, 6, 5, 6, 7, 7, 8, 8, 9, 9, 9, 9, 10, 10, 10, 11, 7, 7, 6, 7, 7, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 7, 8, 7, 7, 8,
 8, 8, 8, 9, 9, 9, 10, 10, 10, 10, 11, 7, 9, 7, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 10, 7, 9, 8, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 10, 11, 7, 10, 8, 8, 8, 9, 9, 9, 9, 10, 10, 10, 10, 10, 11, 11, 8, 10, 9, 9, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 11, 11, 
8, 10, 9, 9, 9, 9, 9, 9, 10, 10, 10, 10, 10, 11, 11, 11, 8, 11, 9, 9, 9, 9, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 8, 11, 10, 9, 9, 9, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 8, 11, 10, 10, 10, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 8, 11, 10, 10,
 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 8, 12, 10, 10, 10, 10, 10, 10, 11, 11, 11, 11, 11, 11, 11, 11, 8, 8, 7, 7, 7, 7, 7, 7, 7, 7, 7, 7, 8, 8, 8, 8, 4}; 
static const unsigned char t32l[]  = {1, 4, 4, 5, 4, 6, 5, 6, 4, 5, 5, 6, 5, 6, 6, 6}; 
static const unsigned char t33l[]  = {4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4, 4}; 

#define NOREF -1
struct huffcodetab ht[HTN] =
{
{ 0, 0, 0,   0,NULL,NULL},
{ 2, 2, 0,   0,t1HB, t1l},
{ 3, 3, 0,   0,t2HB, t2l},
{ 3, 3, 0,   0,t3HB, t3l},
{ 0, 0, 0,   0,NULL,NULL},/* Apparently not used*/
{ 4, 4, 0,   0,t5HB, t5l},
{ 4, 4, 0,   0,t6HB, t6l},
{ 6, 6, 0,   0,t7HB, t7l},
{ 6, 6, 0,   0,t8HB, t8l},
{ 6, 6, 0,   0,t9HB, t9l},
{ 8, 8, 0,   0,t10HB, t10l},
{ 8, 8, 0,   0,t11HB, t11l},
{ 8, 8, 0,   0,t12HB, t12l},
{16,16, 0,   0,t13HB, t13l},
{ 0, 0, 0,   0,NULL,NULL},/* Apparently not used*/
{16,16, 0,   0,t15HB, t15l},
{16,16, 1,   1,t16HB, t16l},
{16,16, 2,   3,t16HB, t16l},
{16,16, 3,   7,t16HB, t16l},
{16,16, 4,  15,t16HB, t16l},
{16,16, 6,  63,t16HB, t16l},
{16,16, 8, 255,t16HB, t16l},
{16,16,10,1023,t16HB, t16l},
{16,16,13,8191,t16HB, t16l},
{16,16, 4,  15,t24HB, t24l},
{16,16, 5,  31,t24HB, t24l},
{16,16, 6,  63,t24HB, t24l},
{16,16, 7, 127,t24HB, t24l},
{16,16, 8, 255,t24HB, t24l},
{16,16, 9, 511,t24HB, t24l},
{16,16,11,2047,t24HB, t24l},
{16,16,13,8191,t24HB, t24l},
{ 1,16, 0,   0,t32HB, t32l},
{ 1,16, 0,   0,t33HB, t33l},
};      


