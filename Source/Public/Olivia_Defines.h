#pragma once

#define ARRAY_SIZE(x) sizeof(x) / sizeof(x[0])

#define KILOBYTES(x) ((x) * 1024ull)
#define MEGABYTES(x) (KILOBYTES(x) * 1024ull)
#define GIGABYTES(x) (MEGABYTES(x) * 1024ull)
