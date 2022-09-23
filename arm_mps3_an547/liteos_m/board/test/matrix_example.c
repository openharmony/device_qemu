/*
 * Copyright (c) 2022-2022 Huawei Device Co., Ltd. All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without modification,
 * are permitted provided that the following conditions are met:
 *
 * 1. Redistributions of source code must retain the above copyright notice, this list of
 *    conditions and the following disclaimer.
 *
 * 2. Redistributions in binary form must reproduce the above copyright notice, this list
 *    of conditions and the following disclaimer in the documentation and/or other materials
 *    provided with the distribution.
 *
 * 3. Neither the name of the copyright holder nor the names of its contributors may be used
 *    to endorse or promote products derived from this software without specific prior written
 *    permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
 * "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT HOLDER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
 * ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#include <stdio.h>
#include "arm_math.h"
#include "sys/time.h"

#define ROW             4
#define COW             4
#define LOOP_TIMES      1000
#define MS_PER_S        1000

float32_t F32MatrixTransA[ROW * COW];
float32_t F32MatrixATMA[ROW * COW];
float32_t F32MatrixATMAI[ROW * COW];

const float32_t F32MatrixA[ROW * COW] = {1.0, 32.0, 16.0,  64.0, 1.0, 32.0, 256.0, 256.0, 1.0, 64.0, 16.0, 512.0, 1.0, 64.0, 128.0, 1024.0};

int32_t test_dsp(void)
{
    int index = LOOP_TIMES;
    uint32_t srcRows = ROW;
    uint32_t srcColumns = COW;
    struct timeval start, end;
    uint32_t timeCost;
    arm_matrix_instance_f32 MatrixA;
    arm_matrix_instance_f32 MatrixAT;
    arm_matrix_instance_f32 MatrixATMA;
    arm_matrix_instance_f32 MatrixATMAI;

    gettimeofday(&start, NULL);
    while (index--) {
        arm_mat_init_f32(&MatrixA, srcRows, srcColumns, (float32_t *)F32MatrixA);
        arm_mat_init_f32(&MatrixAT, srcRows, srcColumns, F32MatrixTransA);
        arm_mat_trans_f32(&MatrixA, &MatrixAT);
        arm_mat_init_f32(&MatrixATMA, srcRows, srcColumns, F32MatrixATMA);
        arm_mat_mult_f32(&MatrixAT, &MatrixA, &MatrixATMA);
        arm_mat_init_f32(&MatrixATMAI, srcRows, srcColumns, F32MatrixATMAI);
        arm_mat_inverse_f32(&MatrixATMA, &MatrixATMAI);
        arm_mat_mult_f32(&MatrixATMAI, &MatrixAT, &MatrixATMA);
    }

    gettimeofday(&end, NULL);
    timeCost = (unsigned int)(end.tv_sec - start.tv_sec) * MS_PER_S + (unsigned int)(end.tv_usec - start.tv_usec) /
               MS_PER_S;
    printf("time cost: %d\n", timeCost);
    return 0;
}

