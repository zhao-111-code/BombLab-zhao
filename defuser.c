#include "defuser.h"

#include <inttypes.h>
#include <stdio.h>
#include <string.h>

#define MAX_FIELDS 8

/*
 * 把 values 中的 count 个整数写成一行“空格分隔的无符号十进制整数”，存入 out。
 * 成功返回 0；count 为 0（还没有实现）、snprintf 出错或结果被截断时返回非零。
 */
static int write_answer(char *out, size_t cap, const uint32_t *values, size_t count) {
    size_t used = 0;
    for (size_t i = 0; i < count; ++i) {
        int size = snprintf(out + used, cap - used, "%s%" PRIu32, i ? " " : "", values[i]);
        if (size < 0 || (size_t)size >= cap - used) return 1;
        used += (size_t)size;
    }
    return count == 0;
}

/* ---------- 通用位运算 ---------- */
static uint32_t ror32(uint32_t x, unsigned n) {
    n &= 31u;
    return (x >> n) | (x << (32u - n));
}

static uint32_t rol32(uint32_t x, unsigned n) {
    n &= 31u;
    return (x << n) | (x >> (32u - n));
}

/* ================= Phase 1 ================= */
int solve_phase_1(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t hi = (uint32_t)(challenge >> 32);
    uint32_t mid = (uint32_t)(challenge >> 16);
    uint32_t lo = (uint32_t)challenge;
    uint32_t r1 = (mid + 0xEFF0u) ^ hi;
    uint32_t r0 = (hi ^ 0xAE7u) + 2u * lo;
    values[0] = 5u * (r0 - r1) + 0xE9u;
    return write_answer(out, cap, values, 1);
}

/* ================= Phase 2 ================= */
int solve_phase_2(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    values[0] = (uint32_t)((2u * (challenge >> 17)) & 0xFFu) + 1u;
    for (uint32_t k = 1; k < 6u; ++k) {
        uint32_t g = (uint32_t)((challenge >> (20u + 5u * k)) & 0x3Fu);
        values[k] = (g + k) ^ (3u * values[k - 1]);
    }
    return write_answer(out, cap, values, 6);
}

/* ================= Phase 3 ================= */
static uint32_t p3_c0(uint32_t x) { return 5u * x + 0x13FDu; }
static uint32_t p3_c1(uint32_t x) { return (4u * x) ^ 0x368Bu; }
static uint32_t p3_c2(uint32_t x) { return (x ^ 0x837Fu) + (x >> 2); }
static uint32_t p3_c3(uint32_t x) { return 5u * x + 0x32CF8u; }
static uint32_t p3_c4(uint32_t x) { return (x ^ 0x4D8u) + (x >> 1); }
static uint32_t p3_c5(uint32_t x) { return (4u * x) ^ 0xC764u; }
static uint32_t p3_c6(uint32_t x) { return 3u * x + 0x6A84u; }
static uint32_t p3_c7(uint32_t x) { return 5u * x + 0x3156Au; }

typedef uint32_t (*p3_fn)(uint32_t);
static const p3_fn p3_cases[8] = {
    p3_c3, p3_c6, p3_c7, p3_c4, p3_c0, p3_c2, p3_c5, p3_c1
};

int solve_phase_3(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t v0 = (uint32_t)(challenge & 0x7u);
    uint32_t x = (uint32_t)(challenge >> 32) ^ (uint32_t)(challenge >> 3);
    values[0] = v0;
    values[1] = p3_cases[v0](x);
    return write_answer(out, cap, values, 2);
}

/* ================= Phase 4 ================= */
static uint32_t p4_walk(uint32_t t, uint32_t lo, uint32_t hi, uint32_t rcx) {
    uint32_t mid = lo + ((hi - lo) >> 1);
    if (t == mid) {
        return (mid + 0xF8u) ^ rcx;
    }
    uint32_t pivot = mid + rcx;
    if (t < mid) {
        uint32_t child = p4_walk(t, lo, mid - 1u, rcx);
        return (3u * child) ^ pivot ^ 0x3u;
    }
    uint32_t child = p4_walk(t, mid + 1u, hi, rcx);
    return (3u * child) ^ pivot ^ 0x1Du;
}

int solve_phase_4(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t target = (uint32_t)(challenge & 0x7Fu);
    uint32_t rcx = (uint32_t)(challenge >> 7) ^ (uint32_t)(challenge >> 32);
    values[0] = p4_walk(target, 0u, 0x7Fu, rcx);
    return write_answer(out, cap, values, 1);
}

/* ================= Phase 5 ================= */
struct p5_node {
    int next;        /* 下一个节点索引，-1 表示空 */
    uint32_t a;
    uint32_t b;
};

static const struct p5_node p5_nodes[12] = {
    {11, 0x351u, 0xD003u},   /* 0  0x405ba0 */
    { 8, 0x885u, 0x40F1u},   /* 1  0x405bb0 */
    {10, 0x166u, 0x7B25u},   /* 2  0x405bc0 */
    { 1, 0x472u, 0xC718u},   /* 3  0x405bd0 */
    { 9, 0x61Bu, 0xF0C2u},   /* 4  0x405be0 */
    { 7, 0x51Du, 0xEEA0u},   /* 5  0x405bf0 */
    { 3, 0x003u, 0xC6B4u},   /* 6  0x405c00 */
    { 4, 0xA15u, 0x22B2u},   /* 7  0x405c10 */
    { 5, 0xB30u, 0x2494u},   /* 8  0x405c20 */
    { 0, 0x73Bu, 0xF511u},   /* 9  0x405c30 */
    {-1, 0x23Cu, 0xD896u},   /* 10 0x405c40 */
    { 2, 0x938u, 0x476Bu},   /* 11 0x405c50 */
};

static const uint8_t p5_heads[8] = {6, 3, 1, 8, 5, 7, 4, 9};

int solve_phase_5(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t depth = (uint32_t)(challenge >> 3) & 0x3u;
    uint32_t acc = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);

    uint32_t ni = p5_heads[challenge & 0x7u];
    int next = p5_nodes[ni].next;
    uint32_t a = p5_nodes[ni].a;
    uint32_t b = p5_nodes[ni].b;

    acc ^= 0x3E4Eu;
    acc += b;
    acc *= 5u;
    uint32_t outv = a;
    acc ^= a;

    for (uint32_t i = 0; i < depth; ++i) {
        ni = (uint32_t)next;
        next = p5_nodes[ni].next;
        a = p5_nodes[ni].a;
        b = p5_nodes[ni].b;
        acc += b;
        acc *= 5u;
        outv = a;
        acc ^= a;
    }

    values[0] = acc;
    values[1] = outv;
    return write_answer(out, cap, values, 2);
}

/* ================= Phase 6 ================= */
static uint32_t p6_op_0(uint32_t x) { return (x ^ 0xD09u) + 4u * x; }
static uint32_t p6_op_1(uint32_t x) { return (4u * x + 0x990Bu) ^ (3u * x); }
static uint32_t p6_op_2(uint32_t x) { return ror32(x, 1); }
static uint32_t p6_op_3(uint32_t x) { return ((x >> 1) ^ x) + 0x1335u; }
static uint32_t p6_op_4(uint32_t x) { return rol32(x, 2) ^ 0xBF77u; }
static uint32_t p6_op_5(uint32_t x) { return (x + 0x4995u) ^ (x >> 3); }

typedef uint32_t (*p6_op_fn)(uint32_t);
static const p6_op_fn p6_ops[8] = {
    p6_op_1, p6_op_5, p6_op_4, p6_op_0, p6_op_5, p6_op_2, p6_op_1, p6_op_3
};

int solve_phase_6(uint64_t challenge, char *out, size_t cap) {
    uint32_t values[MAX_FIELDS] = {0};
    uint32_t x0 = (uint32_t)(challenge >> 6) ^ (uint32_t)(challenge >> 32);
    uint32_t i1 = (uint32_t)(challenge & 0x7u);
    uint32_t i2 = (uint32_t)((challenge & 0x38u) >> 3);
    uint32_t x1 = p6_ops[i1](x0) ^ 0xF56Bu;
    uint32_t x2 = p6_ops[i2](x1);
    values[0] = x2 + 0x71C7u;
    return write_answer(out, cap, values, 1);
}

/* ================= Secret (Honor) ================= */
int solve_secret(uint64_t challenge, uint64_t context, char *out, size_t cap) {
    uint32_t C_lo = (uint32_t)challenge;
    uint32_t C_hi = (uint32_t)(challenge >> 32);
    uint32_t X_lo = (uint32_t)context;
    uint32_t X_hi = (uint32_t)(context >> 32);
    uint32_t X_16 = (uint32_t)(context >> 16);
    uint32_t x3 = (uint32_t)(context & 3u);

    uint32_t S = C_hi ^ C_lo;
    uint32_t R = X_hi ^ X_lo;
    uint32_t T = (x3 * 0x42F7u) ^ X_16;
    uint32_t U = ror32(S ^ R, 5) ^ 0x5B85u;
    uint32_t v0 = x3;
    uint32_t v1 = U ^ T;
    uint32_t v2 = (U ^ T) ^ (R + x3 + 0x1972u + (S >> 5));

    /* secret_gate 布局：前 32 字节任意前缀，[32..35]="OPEN"，[36..]=3 个数字 */
    if (cap <= 32u) return 1;
    memset(out, ' ', 32);
    int size = snprintf(out + 32, cap - 32,
                        "OPEN %" PRIu32 " %" PRIu32 " %" PRIu32, v0, v1, v2);
    if (size < 0 || (size_t)size >= cap - 32) return 1;
    return 0;
}
