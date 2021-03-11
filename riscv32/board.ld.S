/*
 * Copyright (c) 2020-2021 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

RAM_ADDR_START            = 0x80000000;
RAM_ADDR_SIZE             = 128M;
FLASH_LOAD_START          = RAM_ADDR_START;
FLASH_LOAD_SIZE           = RAM_ADDR_SIZE;
EXCEPT_STACK_SIZE         = 2K;
NMI_STACK_SIZE            = 1K;
START_AND_IRQ_STACK_SIZE  = 8K;

