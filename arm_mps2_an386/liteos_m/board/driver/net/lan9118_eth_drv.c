/*
 * Copyright (c) 2016-2019 Arm Limited
 * SPDX-License-Identifier: Apache-2.0
 *
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

#include "lan9118_eth_drv.h"
#include "los_debug.h"

#if LWIP_IPV6
#include "lwip/ethip6.h"
#endif

#define DSB                        __DSB()

#define LAN9118_NETIF_NAME         "lan9118"
#define LAN9118_NETIF_NICK         "eth0"
#define LAN9118_NETIF_TEST_IP      "10.0.2.15"
#define LAN9118_NETIF_TEST_GW      "10.0.2.2"
#define LAN9118_NETIF_TEST_MASK    "255.255.255.0"

#define LAN9118_ETH_MAX_FRAME_SIZE 1522U
#define LAN9118_BUFF_ALIGNMENT     4U
#define UINT32_MAX                 ((uint32_t)-1)
#define SLEEP_TIME_MS              60
#define NETIF_SETUP_OVERTIME       100

static struct netif g_NetIf;
static const struct lan9118_eth_dev_cfg_t LAN9118_ETH_DEV_CFG = {.base = LAN9118_BASE};
static struct lan9118_eth_dev_data_t LAN9118_ETH_DEV_DATA = {.state = 0};
struct lan9118_eth_dev_t LAN9118_ETH_DEV = {&(LAN9118_ETH_DEV_CFG), &(LAN9118_ETH_DEV_DATA)};
struct lan9118_eth_dev_t* g_dev = &LAN9118_ETH_DEV;

extern void tcpip_init(tcpip_init_done_fn initfunc, void* arg);

/** Setter bit manipulation macro */
#define SET_BIT(WORD, BIT_INDEX)       \
    ({                                 \
        DSB;                           \
        ((WORD)) |= ((1U) << (BIT_INDEX)); \
    })

/** Clearing bit manipulation macro */
#define CLR_BIT(WORD, BIT_INDEX)        \
    ({                                  \
        DSB;                            \
        (WORD) &= ~((1U) << (BIT_INDEX)); \
    })

/** Getter bit manipulation macro */
#define GET_BIT(WORD, BIT_INDEX)                            \
    ({                                                      \                                   
        UINT32 r = (bool)(((WORD) & ((1U) << (BIT_INDEX))));  \
        DSB;                                                \
        r;                                                  \
    })

/** Setter bit-field manipulation macro */
#define SET_BIT_FIELD(WORD, BIT_MASK, BIT_OFFSET, VALUE) \
    ({                                                   \
        DSB;                                             \
        ((WORD) |= (((VALUE) & (BIT_MASK)) << (BIT_OFFSET)));    \
    })

/** Clearing bit-field manipulation macro */
#define CLR_BIT_FIELD(WORD, BIT_MASK, BIT_OFFSET, VALUE) \
    ({                                                   \
        DSB;                                             \
        ((WORD) &= ~(((VALUE) & (BIT_MASK)) << (BIT_OFFSET)));   \
    })

/** Getter bit-field manipulation macro */
#define GET_BIT_FIELD(WORD, BIT_MASK, BIT_OFFSET) \
    ({                                            \
        UINT32 r = (WORD);                          \
        DSB;                                      \
        r = (((r) >> (BIT_OFFSET)) & (BIT_MASK));       \
        r;                                        \
    })

/** Millisec timeout macros */
#define RESET_TIME_OUT_MS     10U
#define REG_WRITE_TIME_OUT_MS 50U
#define PHY_RESET_TIME_OUT_MS 100U
#define INIT_FINISH_DELAY     2000U

struct lan9118_eth_reg_map_t {
    uint32_t rx_data_port; /**< Receive FIFO Ports (offset 0x0) */
    uint32_t reserved1[0x7];
    uint32_t tx_data_port; /**< Transmit FIFO Ports (offset 0x20) */
    uint32_t reserved2[0x7];

    uint32_t rx_status_port; /**< Receive FIFO status port (offset 0x40) */
    uint32_t rx_status_peek; /**< Receive FIFO status peek (offset 0x44) */
    uint32_t tx_status_port; /**< Transmit FIFO status port (offset 0x48) */
    uint32_t tx_status_peek; /**< Transmit FIFO status peek (offset 0x4C) */

    uint32_t id_revision;       /**< Chip ID and Revision (offset 0x50) */
    uint32_t irq_cfg;           /**< Main Interrupt Config (offset 0x54) */
    uint32_t irq_status;        /**< Interrupt Status (offset 0x58) */
    uint32_t irq_enable;        /**< Interrupt Enable Register (offset 0x5C) */
    uint32_t reserved3;         /**< Reserved for future use (offset 0x60) */
    uint32_t byte_test;         /**< Byte order test 87654321h (offset 0x64) */
    uint32_t fifo_level_irq;    /**< FIFO Level Interrupts (offset 0x68) */
    uint32_t rx_cfg;            /**< Receive Configuration (offset 0x6C) */
    uint32_t tx_cfg;            /**< Transmit Configuration (offset 0x70) */
    uint32_t hw_cfg;            /**< Hardware Configuration (offset 0x74) */
    uint32_t rx_datapath_ctrl;  /**< RX Datapath Control (offset 0x78) */
    uint32_t rx_fifo_inf;       /**< Receive FIFO Information (offset 0x7C) */
    uint32_t tx_fifo_inf;       /**< Transmit FIFO Information (offset 0x80) */
    uint32_t pmt_ctrl;          /**< Power Management Control (offset 0x84) */
    uint32_t gpio_cfg;          /**< GPIO Configuration (offset 0x88) */
    uint32_t gptimer_cfg;       /**< GP Timer Configuration (offset 0x8C) */
    uint32_t gptimer_count;     /**< GP Timer Count (offset 0x90) */
    uint32_t reserved4;         /**< Reserved for future use (offset 0x94) */
    uint32_t word_swap;         /**< WORD SWAP Register (offset 0x98) */
    uint32_t free_run_counter;  /**< Free Run Counter (offset 0x9C) */
    uint32_t rx_dropped_frames; /**< RX Dropped Frames Counter (offset 0xA0) */
    uint32_t mac_csr_cmd;       /**< MAC CSR Synchronizer Cmd (offset 0xA4) */
    uint32_t mac_csr_data;      /**< MAC CSR Synchronizer Data (offset 0xA8) */
    uint32_t afc_cfg;           /**< AutomaticFlow Ctrl Config (offset 0xAC) */
    uint32_t eeprom_cmd;        /**< EEPROM Command (offset 0xB0) */
    uint32_t eeprom_data;       /**< EEPROM Data (offset 0xB4) */
};

/**
 * \brief FIFO Info definitions
 *
 */
#define FIFO_USED_SPACE_MASK          0xFFFFU
#define DATA_FIFO_USED_SPACE_POS      0U

/**
 * \brief MAC CSR Synchronizer Command bit definitions
 *
 */
enum mac_csr_cmd_bits_t {
    MAC_CSR_CMD_RW_INDEX = 30U,
    MAC_CSR_CMD_BUSY_INDEX = 31U,
};

#define MAC_CSR_CMD_ADDRESS_MASK 0x0FU

/**
 * \brief MAC Control register bit definitions
 *
 */
enum mac_reg_cr_bits_t { MAC_REG_CR_RXEN_INDEX = 2U, MAC_REG_CR_TXEN_INDEX = 3U };

/**
 * \brief MII Access register bit definitions
 *
 */
enum mac_reg_mii_acc_bits_t {
    MAC_REG_MII_ACC_BUSY_INDEX = 0U,
    MAC_REG_MII_ACC_WRITE_INDEX = 1U,
    MAC_REG_MII_ACC_PHYADDR_INDEX = 11U
};
#define MAC_REG_MII_ACC_MII_REG_MASK   0x1FU
#define MAC_REG_MII_ACC_MII_REG_OFFSET 6U

/**
 * \brief Hardware config register bit definitions
 *
 */
enum hw_cfg_reg_bits_t {
    HW_CFG_REG_SRST_INDEX = 0U,
    HW_CFG_REG_SRST_TIMEOUT_INDEX = 1U,
    HW_CFG_REG_MUST_BE_ONE_INDEX = 20U,
};
#define HW_CFG_REG_TX_FIFO_SIZE_POS 16U
#define HW_CFG_REG_TX_FIFO_SIZE_MIN 2U  /*< Min Tx fifo size in KB */
#define HW_CFG_REG_TX_FIFO_SIZE_MAX 14U /*< Max Tx fifo size in KB */
#define HW_CFG_REG_TX_FIFO_SIZE     5U  /*< Tx fifo size in KB */

/**
 * \brief EEPROM command register bit definitions
 *
 */
enum eeprom_cmd_reg_bits_t {
    EEPROM_CMD_REG_BUSY_INDEX = 31U,
};

/**
 * \brief PHY Basic Control register bit definitions
 *
 */
enum phy_reg_bctrl_reg_bits_t {
    PHY_REG_BCTRL_RST_AUTO_NEG_INDEX = 9U,
    PHY_REG_BCTRL_AUTO_NEG_EN_INDEX = 12U,
    PHY_REG_BCTRL_RESET_INDEX = 15U
};

/**
 * \brief TX Command A bit definitions
 *
 */
#define TX_CMD_DATA_START_OFFSET_BYTES_POS  16U
#define TX_CMD_DATA_START_OFFSET_BYTES_MASK 0x1FU

enum tx_command_a_bits_t { TX_COMMAND_A_LAST_SEGMENT_INDEX = 12U, TX_COMMAND_A_FIRST_SEGMENT_INDEX = 13U };

#define TX_CMD_PKT_LEN_BYTES_MASK 0x7FFU
#define TX_CMD_PKT_TAG_MASK       0xFFFFU
#define TX_CMD_PKT_TAG_POS        16U

#define RX_FIFO_STATUS_PKT_LENGTH_POS  16U
#define RX_FIFO_STATUS_PKT_LENGTH_MASK 0x3FFFU

/**
 * \brief Interrupt Configuration register bit definitions
 *
 */
enum irq_cfg_bits_t { IRQ_CFG_IRQ_EN_INDEX = 8U };

#define AFC_BACK_DUR_MASK   0x0FU
#define AFC_BACK_DUR_POS    4U
#define AFC_BACK_DUR        4U /**< equal to 50us */

#define AFC_LOW_LEVEL_MASK  0xFFU
#define AFC_LOW_LEVEL_POS   8U
#define AFC_LOW_LEVEL       55U /**< specifies in multiple of 64 bytes */

#define AFC_HIGH_LEVEL_MASK 0xFFU
#define AFC_HIGH_LEVEL_POS  16U
#define AFC_HIGH_LEVEL      110U /**< specifies in multiple of 64 bytes */

#define BYTE_LEN            4
/**
 * \brief Auto-Negotiation Advertisement register bit definitions
 *
 */
enum aneg_bits_t {
    ANEG_10_BASE_T_INDEX = 5U,             /**< 10Mbps able */
    ANEG_10_BASE_T_FULL_DUPL_INDEX = 6U,   /**< 10Mbps with full duplex */
    ANEG_100_BASE_TX_INDEX = 7U,           /**< 100Mbps Tx able */
    ANEG_100_BASE_TX_FULL_DUPL_INDEX = 8U, /**< 100Mbps with full duplex */
    ANEG_SYMM_PAUSE_INDEX = 10U,           /**< Symmetric Pause */
    ANEG_ASYMM_PAUSE_INDEX = 11U           /**< Asymmetric Pause */
};

/**
 * \brief Transmit Configuration register bit definitions
 *
 */
enum tx_cfg_bits_t {
    TX_CFG_STOP_INDEX = 0U,      /*< stop */
    TX_CFG_ON_INDEX = 1U,        /*< on */
    TX_CFG_AO_INDEX = 2U,        /*< allow overrun */
    TX_CFG_TXD_DUMP_INDEX = 14U, /*< Data FIFO dump */
    TX_CFG_TXS_DUMP_INDEX = 15U  /*< Status FIFO dump */
};

/**
 * \brief Chip ID definitions
 *
 */
#define CHIP_ID 0x1180001

/**
 * Helper struct to hold private data used to operate your ethernet interface.
 * Keeping the ethernet address of the MAC in this struct is not necessary
 * as it is already kept in the struct netif.
 */
struct EtherNetif {
    struct eth_addr* ethaddr;
};

static void DelayMs(uint32_t ms)
{
    uint32_t delayMs = 1000;
    delayMs = delayMs * ms;
    usleep(delayMs);
}

static void* GetPayloadAddr(const struct pbuf* buf)
{
    return buf->payload;
}

static void SetTotalLen(struct pbuf* pbuf)
{
    if (!pbuf->next) {
        pbuf->tot_len = pbuf->len;
        return;
    }

    uint32_t total_len;
    struct pbuf* pbuf_tailing;

    while (pbuf) {
        total_len = pbuf->len;

        pbuf_tailing = pbuf->next;
        while (pbuf_tailing) {
            total_len += pbuf_tailing->len;
            pbuf_tailing = pbuf_tailing->next;
        }

        pbuf->tot_len = total_len;
        pbuf = pbuf->next;
    }
}

static void AlignMemory(struct pbuf* pbuf, uint32_t const align)
{
    if (!align) {
        return;
    }

    struct pbuf* pbuf_start = pbuf;

    while (pbuf) {
        uint32_t remainder = ((uint32_t)pbuf->payload) % align;
        if (remainder) {
            uint32_t offset = align - remainder;
            if (offset >= align) {
                offset = align;
            }
            pbuf->payload = ((char*)pbuf->payload) + offset;
        }
        pbuf->len -= align;
        pbuf = pbuf->next;
    }

    // Correct total lengths
    SetTotalLen(pbuf_start);
}

static struct pbuf*  AllocHeap(const uint32_t size, uint32_t const align)
{
    struct pbuf* pbuf = pbuf_alloc(PBUF_RAW, size + align, PBUF_RAM);
    if (pbuf == NULL) {
        return NULL;
    }

    AlignMemory(pbuf, align);

    return pbuf;
}

static void SetLen(const struct pbuf* buf, const uint32_t len)
{
    struct pbuf* pbuf = buf;
    pbuf->len = len;
    SetTotalLen(pbuf);
}

static uint32_t GetLen(const struct pbuf* buf)
{
    return buf->len;
}

static uint32_t GetTotalLen(const struct pbuf* buf)
{
    return buf->tot_len;
}

static void fill_tx_fifo(const struct lan9118_eth_dev_t* dev, uint8_t* data, uint32_t size_bytes)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    uint32_t tx_data_port_tmp = 0;
    uint8_t* tx_data_port_tmp_ptr = (uint8_t*)&tx_data_port_tmp;

#ifdef  ETH_PAD_SIZE
    data += ETH_PAD_SIZE;
#endif

    uint32_t remainder_bytes = (size_bytes % 4);

    if (remainder_bytes > 0) {
        uint32_t filler_bytes = (4 - remainder_bytes);

        for (uint32_t i = 0; i < 4; i++) {
            if (i < filler_bytes) {
                tx_data_port_tmp_ptr[i] = 0;
            } else {
                tx_data_port_tmp_ptr[i] = data[i - filler_bytes];
            }
        }

        SET_BIT_FIELD(register_map->tx_data_port, 0xFFFFFFFF, 0, tx_data_port_tmp);

        size_bytes -= remainder_bytes;
        data += remainder_bytes;
    }

    while (size_bytes > 0) {
        /* Keep the same endianness in data as in the temp variable */
        tx_data_port_tmp_ptr[0] = data[0];
        tx_data_port_tmp_ptr[1] = data[1];
        tx_data_port_tmp_ptr[2] = data[2];
        tx_data_port_tmp_ptr[3] = data[3];

        SET_BIT_FIELD(register_map->tx_data_port, 0xFFFFFFFF, 0, tx_data_port_tmp);

        data += BYTE_LEN;
        size_bytes -= BYTE_LEN;
    }
}

static void empty_rx_fifo(const struct lan9118_eth_dev_t* dev, uint8_t* data, uint32_t size_bytes)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    uint32_t rx_data_port_tmp = 0;
    uint8_t* rx_data_port_tmp_ptr = (uint8_t*)&rx_data_port_tmp;

#ifdef  ETH_PAD_SIZE
    data += ETH_PAD_SIZE;
#endif

    uint32_t remainder_bytes = (size_bytes % 4);
    size_bytes -= remainder_bytes;

    while (size_bytes > 0) {
        /* Keep the same endianness in data as in the temp variable */
        rx_data_port_tmp = GET_BIT_FIELD(register_map->rx_data_port, 0xFFFFFFFF, 0);

        data[0] = rx_data_port_tmp_ptr[0];
        data[1] = rx_data_port_tmp_ptr[1];
        data[2] = rx_data_port_tmp_ptr[2];
        data[3] = rx_data_port_tmp_ptr[3];

        data += BYTE_LEN;
        size_bytes -= BYTE_LEN;
    }

    if (remainder_bytes > 0) {
        rx_data_port_tmp = GET_BIT_FIELD(register_map->rx_data_port, 0xFFFFFFFF, 0);

        for (uint32_t i = 0; i < remainder_bytes; i++) {
            data[i] = rx_data_port_tmp_ptr[i];
        }
    }
}

enum lan9118_error_t lan9118_mac_regread(const struct lan9118_eth_dev_t* dev, enum lan9118_mac_reg_offsets_t regoffset,
                                         uint32_t* data)
{
    volatile uint32_t val;
    uint32_t maccmd = GET_BIT_FIELD(regoffset, MAC_CSR_CMD_ADDRESS_MASK, 0);
    uint32_t time_out = REG_WRITE_TIME_OUT_MS;

    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    /* Make sure there's no pending operation */
    if (!(GET_BIT(register_map->mac_csr_cmd, MAC_CSR_CMD_BUSY_INDEX))) {
        SET_BIT(maccmd, MAC_CSR_CMD_RW_INDEX);
        SET_BIT(maccmd, MAC_CSR_CMD_BUSY_INDEX);
        register_map->mac_csr_cmd = maccmd; /* Start operation */

        do {
            val = register_map->byte_test; /* A no-op read. */
            (void)val;
            if (dev->data->wait_ms) {
                dev->data->wait_ms(1);
            }
            time_out--;
        } while (time_out && GET_BIT(register_map->mac_csr_cmd, MAC_CSR_CMD_BUSY_INDEX));

        if (!time_out) {
            return LAN9118_ERROR_TIMEOUT;
        } else {
            *data = register_map->mac_csr_data;
        }
    } else {
        return LAN9118_ERROR_BUSY;
    }
    return LAN9118_ERROR_NONE;
}

enum lan9118_error_t lan9118_mac_regwrite(const struct lan9118_eth_dev_t* dev, enum lan9118_mac_reg_offsets_t regoffset,
                                          uint32_t data)
{
    volatile uint32_t read = 0;
    uint32_t maccmd = GET_BIT_FIELD(regoffset, MAC_CSR_CMD_ADDRESS_MASK, 0);
    uint32_t time_out = REG_WRITE_TIME_OUT_MS;

    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    /* Make sure there's no pending operation */
    if (!GET_BIT(register_map->mac_csr_cmd, MAC_CSR_CMD_BUSY_INDEX)) {
        register_map->mac_csr_data = data; /* Store data. */

        CLR_BIT(maccmd, MAC_CSR_CMD_RW_INDEX);
        SET_BIT(maccmd, MAC_CSR_CMD_BUSY_INDEX);

        register_map->mac_csr_cmd = maccmd;

        do {
            read = register_map->byte_test; /* A no-op read. */
            (void)read;
            if (dev->data->wait_ms) {
                dev->data->wait_ms(1);
            }
            time_out--;
        } while (time_out && (register_map->mac_csr_cmd & GET_BIT(register_map->mac_csr_cmd, MAC_CSR_CMD_BUSY_INDEX)));
        if (!time_out) {
            return LAN9118_ERROR_TIMEOUT;
        }
    } else {
        return LAN9118_ERROR_BUSY;
    }
    return LAN9118_ERROR_NONE;
}

enum lan9118_error_t lan9118_phy_regread(const struct lan9118_eth_dev_t* dev, enum phy_reg_offsets_t regoffset,
                                         uint32_t* data)
{
    uint32_t val = 0;
    uint32_t phycmd = 0;
    uint32_t time_out = REG_WRITE_TIME_OUT_MS;

    if (lan9118_mac_regread(dev, LAN9118_MAC_REG_OFFSET_MII_ACC, &val)) {
        return LAN9118_ERROR_INTERNAL;
    }

    if (!GET_BIT(val, MAC_REG_MII_ACC_BUSY_INDEX)) {
        phycmd = 0;
        SET_BIT(phycmd, MAC_REG_MII_ACC_PHYADDR_INDEX);
        SET_BIT_FIELD(phycmd, MAC_REG_MII_ACC_MII_REG_MASK, MAC_REG_MII_ACC_MII_REG_OFFSET, regoffset);
        CLR_BIT(phycmd, MAC_REG_MII_ACC_WRITE_INDEX);
        SET_BIT(phycmd, MAC_REG_MII_ACC_BUSY_INDEX);

        if (lan9118_mac_regwrite(dev, LAN9118_MAC_REG_OFFSET_MII_ACC, phycmd)) {
            return LAN9118_ERROR_INTERNAL;
        }

        val = 0;
        do {
            if (dev->data->wait_ms) {
                dev->data->wait_ms(1);
            }
            time_out--;
            if (lan9118_mac_regread(dev, LAN9118_MAC_REG_OFFSET_MII_ACC, &val)) {
                return LAN9118_ERROR_INTERNAL;
            }
        } while (time_out && (GET_BIT(val, MAC_REG_MII_ACC_BUSY_INDEX)));

        if (!time_out) {
            return LAN9118_ERROR_TIMEOUT;
        } else if (lan9118_mac_regread(dev, LAN9118_MAC_REG_OFFSET_MII_DATA, data)) {
            return LAN9118_ERROR_INTERNAL;
        }
    } else {
        return LAN9118_ERROR_BUSY;
    }
    return LAN9118_ERROR_NONE;
}

enum lan9118_error_t lan9118_phy_regwrite(const struct lan9118_eth_dev_t* dev, enum phy_reg_offsets_t regoffset,
                                          uint32_t data)
{
    uint32_t val = 0;
    uint32_t phycmd = 0;
    uint32_t time_out = REG_WRITE_TIME_OUT_MS;

    if (lan9118_mac_regread(dev, LAN9118_MAC_REG_OFFSET_MII_ACC, &val)) {
        return LAN9118_ERROR_INTERNAL;
    }

    if (!GET_BIT(val, MAC_REG_MII_ACC_BUSY_INDEX)) {
        /* Load the data */
        if (lan9118_mac_regwrite(dev, LAN9118_MAC_REG_OFFSET_MII_DATA, (data & 0xFFFF))) {
            return LAN9118_ERROR_INTERNAL;
        }
        phycmd = 0;
        SET_BIT(phycmd, MAC_REG_MII_ACC_PHYADDR_INDEX);
        SET_BIT_FIELD(phycmd, MAC_REG_MII_ACC_MII_REG_MASK, MAC_REG_MII_ACC_MII_REG_OFFSET, regoffset);
        SET_BIT(phycmd, MAC_REG_MII_ACC_WRITE_INDEX);
        SET_BIT(phycmd, MAC_REG_MII_ACC_BUSY_INDEX);
        /* Start operation */
        if (lan9118_mac_regwrite(dev, LAN9118_MAC_REG_OFFSET_MII_ACC, phycmd)) {
            return LAN9118_ERROR_INTERNAL;
        }

        phycmd = 0;

        do {
            if (dev->data->wait_ms) {
                dev->data->wait_ms(1);
            }
            time_out--;
            if (lan9118_mac_regread(dev, LAN9118_MAC_REG_OFFSET_MII_ACC, &phycmd)) {
                return LAN9118_ERROR_INTERNAL;
            }
        } while (time_out && GET_BIT(phycmd, 0));

        if (!time_out) {
            return LAN9118_ERROR_TIMEOUT;
        }

    } else {
        return LAN9118_ERROR_BUSY;
    }
    return LAN9118_ERROR_NONE;
}

uint32_t lan9118_read_id(const struct lan9118_eth_dev_t* dev)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    uint32_t lan9118Id = 0;

    lan9118Id = register_map->id_revision;

    return lan9118Id;
}

enum lan9118_error_t lan9118_soft_reset(const struct lan9118_eth_dev_t* dev)
{
    uint32_t time_out = RESET_TIME_OUT_MS;

    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    /* Soft reset */
    SET_BIT(register_map->hw_cfg, HW_CFG_REG_SRST_INDEX);

    do {
        if (dev->data->wait_ms) {
            dev->data->wait_ms(1);
        }
        time_out--;
    } while (time_out && GET_BIT(register_map->hw_cfg, HW_CFG_REG_SRST_TIMEOUT_INDEX));

    if (!time_out) {
        return LAN9118_ERROR_TIMEOUT;
    }

    return LAN9118_ERROR_NONE;
}

void lan9118_set_txfifo(const struct lan9118_eth_dev_t* dev, uint32_t val)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    if (val >= HW_CFG_REG_TX_FIFO_SIZE_MIN && val <= HW_CFG_REG_TX_FIFO_SIZE_MAX) {
        register_map->hw_cfg = val << HW_CFG_REG_TX_FIFO_SIZE_POS;
    }
}

enum lan9118_error_t lan9118_set_fifo_level_irq(const struct lan9118_eth_dev_t* dev,
                                                enum lan9118_fifo_level_irq_pos_t irq_level_pos, uint32_t level)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    if (level < LAN9118_FIFO_LEVEL_IRQ_LEVEL_MIN || level > LAN9118_FIFO_LEVEL_IRQ_LEVEL_MAX) {
        return LAN9118_ERROR_PARAM;
    }

    CLR_BIT_FIELD(register_map->fifo_level_irq, LAN9118_FIFO_LEVEL_IRQ_MASK, irq_level_pos,
                  LAN9118_FIFO_LEVEL_IRQ_MASK);
    SET_BIT_FIELD(register_map->fifo_level_irq, LAN9118_FIFO_LEVEL_IRQ_MASK, irq_level_pos, level);
    return LAN9118_ERROR_NONE;
}

enum lan9118_error_t lan9118_wait_eeprom(const struct lan9118_eth_dev_t* dev)
{
    uint32_t time_out = REG_WRITE_TIME_OUT_MS;

    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    do {
        if (dev->data->wait_ms) {
            dev->data->wait_ms(1);
        }
        time_out--;
    } while (time_out && GET_BIT(register_map->eeprom_cmd, EEPROM_CMD_REG_BUSY_INDEX));

    if (!time_out) {
        return LAN9118_ERROR_TIMEOUT;
    }

    return LAN9118_ERROR_NONE;
}

void lan9118_init_irqs(const struct lan9118_eth_dev_t* dev)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    lan9118_disable_all_interrupts(dev);
    lan9118_clear_all_interrupts(dev);

    /* Set IRQ deassertion interval */
    SET_BIT_FIELD(register_map->irq_cfg, 0xFFFFFFFF, 0, 0x11);

    /* enable interrupts */
    SET_BIT(register_map->irq_cfg, IRQ_CFG_IRQ_EN_INDEX);
}

enum lan9118_error_t lan9118_check_phy(const struct lan9118_eth_dev_t* dev)
{
    uint32_t phyid1 = 0;
    uint32_t phyid2 = 0;

    if (lan9118_phy_regread(dev, LAN9118_PHY_REG_OFFSET_ID1, &phyid1)) {
        return LAN9118_ERROR_INTERNAL;
    }
    if (lan9118_phy_regread(dev, LAN9118_PHY_REG_OFFSET_ID2, &phyid2)) {
        return LAN9118_ERROR_INTERNAL;
    }
    if ((phyid1 == 0xFFFF && phyid2 == 0xFFFF) || (phyid1 == 0x0 && phyid2 == 0x0)) {
        return LAN9118_ERROR_INTERNAL;
    }
    return LAN9118_ERROR_NONE;
}

enum lan9118_error_t lan9118_reset_phy(const struct lan9118_eth_dev_t* dev)
{
    uint32_t read = 0;

    if (lan9118_phy_regread(dev, LAN9118_PHY_REG_OFFSET_BCTRL, &read)) {
        return LAN9118_ERROR_INTERNAL;
    }

    SET_BIT(read, PHY_REG_BCTRL_RESET_INDEX);
    if (lan9118_phy_regwrite(dev, LAN9118_PHY_REG_OFFSET_BCTRL, read)) {
        return LAN9118_ERROR_INTERNAL;
    }

    return LAN9118_ERROR_NONE;
}

void lan9118_advertise_cap(const struct lan9118_eth_dev_t* dev)
{
    uint32_t aneg_adv = 0;
    lan9118_phy_regread(dev, LAN9118_PHY_REG_OFFSET_ANEG_ADV, &aneg_adv);

    SET_BIT(aneg_adv, ANEG_10_BASE_T_INDEX);
    SET_BIT(aneg_adv, ANEG_10_BASE_T_FULL_DUPL_INDEX);
    SET_BIT(aneg_adv, ANEG_100_BASE_TX_INDEX);
    SET_BIT(aneg_adv, ANEG_100_BASE_TX_FULL_DUPL_INDEX);
    SET_BIT(aneg_adv, ANEG_SYMM_PAUSE_INDEX);
    SET_BIT(aneg_adv, ANEG_ASYMM_PAUSE_INDEX);

    lan9118_phy_regwrite(dev, LAN9118_PHY_REG_OFFSET_ANEG_ADV, aneg_adv);
}

void lan9118_enable_xmit(const struct lan9118_eth_dev_t* dev)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    SET_BIT(register_map->tx_cfg, TX_CFG_ON_INDEX);
}

void lan9118_enable_mac_xmit(const struct lan9118_eth_dev_t* dev)
{
    uint32_t mac_cr = 0;
    lan9118_mac_regread(dev, LAN9118_MAC_REG_OFFSET_CR, &mac_cr);

    SET_BIT(mac_cr, MAC_REG_CR_TXEN_INDEX);

    lan9118_mac_regwrite(dev, LAN9118_MAC_REG_OFFSET_CR, mac_cr);
}

void lan9118_enable_mac_recv(const struct lan9118_eth_dev_t* dev)
{
    uint32_t mac_cr = 0;
    lan9118_mac_regread(dev, LAN9118_MAC_REG_OFFSET_CR, &mac_cr);

    SET_BIT(mac_cr, MAC_REG_CR_RXEN_INDEX);

    lan9118_mac_regwrite(dev, LAN9118_MAC_REG_OFFSET_CR, mac_cr);
}

int lan9118_check_id(const struct lan9118_eth_dev_t* dev)
{
    uint32_t id = lan9118_read_id(dev);

    return ((id == CHIP_ID) ? 0 : 1);
}

void lan9118_enable_interrupt(const struct lan9118_eth_dev_t* dev, enum lan9118_interrupt_source source)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    SET_BIT(register_map->irq_enable, source);
}

void lan9118_disable_interrupt(const struct lan9118_eth_dev_t* dev, enum lan9118_interrupt_source source)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    CLR_BIT(register_map->irq_enable, source);
}

void lan9118_disable_all_interrupts(const struct lan9118_eth_dev_t* dev)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    register_map->irq_enable = 0;
}

void lan9118_clear_interrupt(const struct lan9118_eth_dev_t* dev, enum lan9118_interrupt_source source)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    SET_BIT(register_map->irq_status, source);
}

void lan9118_clear_all_interrupts(const struct lan9118_eth_dev_t* dev)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    register_map->irq_status = UINT32_MAX;
}

int lan9118_get_interrupt(const struct lan9118_eth_dev_t* dev, enum lan9118_interrupt_source source)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    return GET_BIT(register_map->irq_status, source);
}

void lan9118_establish_link(const struct lan9118_eth_dev_t* dev)
{
    uint32_t bcr = 0;
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    lan9118_phy_regread(dev, LAN9118_PHY_REG_OFFSET_BCTRL, &bcr);
    SET_BIT(bcr, PHY_REG_BCTRL_AUTO_NEG_EN_INDEX);
    SET_BIT(bcr, PHY_REG_BCTRL_RST_AUTO_NEG_INDEX);
    lan9118_phy_regwrite(dev, LAN9118_PHY_REG_OFFSET_BCTRL, bcr);

    SET_BIT(register_map->hw_cfg, HW_CFG_REG_MUST_BE_ONE_INDEX);
}

enum lan9118_error_t lan9118_read_mac_address(const struct lan9118_eth_dev_t* dev, char* mac)
{
    uint32_t mac_low = 0;
    uint32_t mac_high = 0;

    if (!mac) {
        return LAN9118_ERROR_PARAM;
    }

    if (lan9118_mac_regread(dev, LAN9118_MAC_REG_OFFSET_ADDRH, &mac_high)) {
        return LAN9118_ERROR_INTERNAL;
    }
    if (lan9118_mac_regread(dev, LAN9118_MAC_REG_OFFSET_ADDRL, &mac_low)) {
        return LAN9118_ERROR_INTERNAL;
    }
    mac[0] = mac_low & 0xFF;
    mac[1] = (mac_low >> 8) & 0xFF;
    mac[2] = (mac_low >> 16) & 0xFF;
    mac[3] = (mac_low >> 24) & 0xFF;
    mac[4] = mac_high & 0xFF;
    mac[5] = (mac_high >> 8) & 0xFF;

    return LAN9118_ERROR_NONE;
}

enum lan9118_error_t lan9118_init(const struct lan9118_eth_dev_t* dev, void (*wait_ms_function)(uint32_t))
{
    uint32_t phyreset = 0;
    enum lan9118_error_t error = LAN9118_ERROR_NONE;
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    if (!wait_ms_function) {
        return LAN9118_ERROR_PARAM;
    }
    dev->data->wait_ms = wait_ms_function;

    error = lan9118_check_id(dev);
    if (error != LAN9118_ERROR_NONE) {
        return error;
    }

    error = lan9118_soft_reset(dev);
    if (error != LAN9118_ERROR_NONE) {
        return error;
    }

    lan9118_set_txfifo(dev, HW_CFG_REG_TX_FIFO_SIZE);

    SET_BIT_FIELD(register_map->afc_cfg, AFC_BACK_DUR_MASK, AFC_BACK_DUR_POS, AFC_BACK_DUR);
    SET_BIT_FIELD(register_map->afc_cfg, AFC_LOW_LEVEL_MASK, AFC_LOW_LEVEL_POS, AFC_LOW_LEVEL);
    SET_BIT_FIELD(register_map->afc_cfg, AFC_HIGH_LEVEL_MASK, AFC_HIGH_LEVEL_POS, AFC_HIGH_LEVEL);

    error = lan9118_wait_eeprom(dev);
    if (error != LAN9118_ERROR_NONE) {
        return error;
    }

    lan9118_init_irqs(dev);

    /* Configure MAC addresses here if needed. */
    error = lan9118_check_phy(dev);
    if (error != LAN9118_ERROR_NONE) {
        return error;
    }

    error = lan9118_reset_phy(dev);
    if (error != LAN9118_ERROR_NONE) {
        return error;
    }

    if (dev->data->wait_ms) {
        dev->data->wait_ms(PHY_RESET_TIME_OUT_MS);
    }
    /* Checking whether phy reset completed successfully.*/
    error = lan9118_phy_regread(dev, LAN9118_PHY_REG_OFFSET_BCTRL, &phyreset);
    if (error != LAN9118_ERROR_NONE) {
        return error;
    }

    if (GET_BIT(phyreset, PHY_REG_BCTRL_RESET_INDEX)) {
        return LAN9118_ERROR_INTERNAL;
    }

    lan9118_advertise_cap(dev);
    lan9118_establish_link(dev);
    lan9118_enable_mac_xmit(dev);
    lan9118_enable_xmit(dev);
    lan9118_enable_mac_recv(dev);

    /* This sleep is compulsory otherwise txmit/receive will fail. */
    if (dev->data->wait_ms) {
        dev->data->wait_ms(INIT_FINISH_DELAY);
    }
    dev->data->state = 1;

    return LAN9118_ERROR_NONE;
}

enum lan9118_error_t lan9118_send_by_chunks(const struct lan9118_eth_dev_t* dev, uint32_t total_payload_length,
                                            bool is_new_packet, const char* data, uint32_t current_size)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;
    bool is_first_segment = false;
    bool is_last_segment = false;
    uint32_t txcmd_a, txcmd_b = 0;
    uint32_t tx_buffer_free_space = 0;
    volatile uint32_t xmit_stat = 0;

    if (!data) {
        return LAN9118_ERROR_PARAM;
    }

    if (is_new_packet) {
        is_first_segment = true;
        dev->data->ongoing_packet_length = total_payload_length;
        dev->data->ongoing_packet_length_sent = 0;
    } else if (dev->data->ongoing_packet_length != total_payload_length ||
               dev->data->ongoing_packet_length_sent >= total_payload_length) {
        return LAN9118_ERROR_PARAM;
    }

    /* Would next chunk fit into buffer? */
    tx_buffer_free_space = GET_BIT_FIELD(register_map->tx_fifo_inf, FIFO_USED_SPACE_MASK, DATA_FIFO_USED_SPACE_POS);

    if (current_size > tx_buffer_free_space) {
        return LAN9118_ERROR_INTERNAL; /* Not enough space in FIFO */
    }
    if ((dev->data->ongoing_packet_length_sent + current_size) == total_payload_length) {
        is_last_segment = true;
    }

    txcmd_a = 0;
    txcmd_b = 0;

    if (is_last_segment) {
        SET_BIT(txcmd_a, TX_COMMAND_A_LAST_SEGMENT_INDEX);
    }
    if (is_first_segment) {
        SET_BIT(txcmd_a, TX_COMMAND_A_FIRST_SEGMENT_INDEX);
    }

    uint32_t data_start_offset_bytes = (4 - (current_size % 4));

    SET_BIT_FIELD(txcmd_a, TX_CMD_PKT_LEN_BYTES_MASK, 0, current_size);
    SET_BIT_FIELD(txcmd_a, TX_CMD_DATA_START_OFFSET_BYTES_MASK, TX_CMD_DATA_START_OFFSET_BYTES_POS,
                  data_start_offset_bytes);

    SET_BIT_FIELD(txcmd_b, TX_CMD_PKT_LEN_BYTES_MASK, 0, current_size);
    SET_BIT_FIELD(txcmd_b, TX_CMD_PKT_TAG_MASK, TX_CMD_PKT_TAG_POS, current_size);

    SET_BIT_FIELD(register_map->tx_data_port, 0xFFFFFFFF, 0, txcmd_a);
    SET_BIT_FIELD(register_map->tx_data_port, 0xFFFFFFFF, 0, txcmd_b);

    fill_tx_fifo(dev, (uint8_t*)data, current_size);

    if (is_last_segment) {
        /* Pop status port for error check */
        xmit_stat = register_map->tx_status_port;
        (void)xmit_stat;
    }
    dev->data->ongoing_packet_length_sent += current_size;
    return LAN9118_ERROR_NONE;
}

uint32_t lan9118_get_rxfifo_data_used_space(const struct lan9118_eth_dev_t* dev)
{
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    return GET_BIT_FIELD(register_map->rx_fifo_inf, FIFO_USED_SPACE_MASK, DATA_FIFO_USED_SPACE_POS);
}

uint32_t lan9118_receive_by_chunks(const struct lan9118_eth_dev_t* dev, char* data, uint32_t dlen)
{
    uint32_t rxfifo_inf = 0;
    uint32_t rxfifo_stat = 0;
    uint32_t packet_length_byte = 0;
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    if (!data) {
        return 0; /* Invalid input parameter, cannot read */
    }

    rxfifo_inf = GET_BIT_FIELD(register_map->rx_fifo_inf, 0xFFFFFFFF, 0);

    if (rxfifo_inf & 0xFFFF) { /* If there's data */
        rxfifo_stat = GET_BIT_FIELD(register_map->rx_status_port, 0xFFFFFFFF, 0);
        if (rxfifo_stat != 0) { /* Fetch status of this packet */
            /* Ethernet controller is padding to 32bit aligned data */
            packet_length_byte =
                GET_BIT_FIELD(rxfifo_stat, RX_FIFO_STATUS_PKT_LENGTH_MASK, RX_FIFO_STATUS_PKT_LENGTH_POS);

            dev->data->current_rx_size_words = packet_length_byte;
        }
    }

    empty_rx_fifo(dev, (uint8_t*)data, packet_length_byte);
    dev->data->current_rx_size_words = 0;

    return packet_length_byte;
}

uint32_t lan9118_peek_next_packet_size(const struct lan9118_eth_dev_t* dev)
{
    uint32_t packet_size = 0;
    struct lan9118_eth_reg_map_t* register_map = (struct lan9118_eth_reg_map_t*)dev->cfg->base;

    if (lan9118_get_rxfifo_data_used_space(dev)) {
        packet_size =
            GET_BIT_FIELD(register_map->rx_status_peek, RX_FIFO_STATUS_PKT_LENGTH_MASK, RX_FIFO_STATUS_PKT_LENGTH_POS);
    }
    return packet_size;
}

struct pbuf* LowLevelInput(void)
{
    struct pbuf* p = NULL;
    uint32_t messageLength = 0;
    uint32_t receivedBytes = 0;

    messageLength = lan9118_peek_next_packet_size(g_dev);
    if (messageLength == 0) {
        return p;
    }

    p = AllocHeap(LAN9118_ETH_MAX_FRAME_SIZE, LAN9118_BUFF_ALIGNMENT);
    if (p != NULL) {
        LOS_TaskLock();

        receivedBytes = lan9118_receive_by_chunks(g_dev, (char*)GetPayloadAddr(p), GetLen(p));
        if (receivedBytes == 0) {
            pbuf_free(p);
            p = NULL;
        } else {
            receivedBytes += 2;
            SetLen(p, receivedBytes);
        }

        LOS_TaskUnlock();
    }

    return p;
}

err_t Lan9118LinkOut(struct netif* netif, struct pbuf* buf)
{
    uint32_t bufferLength = 0;

    if (buf == NULL) {
        return ERR_BUF;
    } else {
        bufferLength = GetTotalLen(buf) - ETH_PAD_SIZE;

        LOS_TaskLock();

        lan9118_send_by_chunks(g_dev, bufferLength, true, (const char*)GetPayloadAddr(buf), bufferLength);

        LOS_TaskUnlock();

        return ERR_OK;
    }
}

void Lan9118PacketRx(void)
{
    struct pbuf* buf;

    buf = LowLevelInput();

    if (buf != NULL) {

        LOS_TaskLock();
        if (g_NetIf.input(buf, &g_NetIf) != ERR_OK) {
            PRINT_ERR("Emac LWIP: IP input error\n");
            pbuf_free(buf);
        }
        LOS_TaskUnlock();
    }
}

void EthernetReceiveHandler(void)
{
    if (lan9118_get_interrupt(g_dev, LAN9118_INTERRUPT_RX_STATUS_FIFO_LEVEL)) {
        lan9118_clear_interrupt(g_dev, LAN9118_INTERRUPT_RX_STATUS_FIFO_LEVEL);

        lan9118_disable_interrupt(g_dev, LAN9118_INTERRUPT_RX_STATUS_FIFO_LEVEL);

        Lan9118PacketRx();

        lan9118_enable_interrupt(g_dev, LAN9118_INTERRUPT_RX_STATUS_FIFO_LEVEL);
    }

    return;
}

void LowLevelInit(struct netif* netif)
{
    enum lan9118_error_t ret = LAN9118_ERROR_NONE;

    /* set MAC hardware address length */
    netif->hwaddr_len = ETHARP_HWADDR_LEN;

    /* maximum transfer unit */
    netif->mtu = LAN9118_ETH_MTU_SIZE;

    /* set MAC hardware address */
    ret = lan9118_read_mac_address(g_dev, (char*)netif->hwaddr);
    if (ret != LAN9118_ERROR_NONE) {
        PRINT_ERR("get mac addr error\n");
        return;
    }

    /* Interface capabilities */
    netif->flags = NETIF_FLAG_BROADCAST | NETIF_FLAG_ETHARP | NETIF_FLAG_ETHERNET | NETIF_FLAG_LINK_UP;

#if LWIP_IPV6 && LWIP_IPV6_MLD
    /*
     * For hardware/netifs that implement MAC filtering.
     * All-nodes link-local is handled by default, so we must let the hardware
     * know to allow multicast packets in. Should set mld_mac_filter previously.
     */
    if (netif->mld_mac_filter != NULL) {
        ip6_addr_t ip6_allnodes_ll;
        ip6_addr_set_allnodes_linklocal(&ip6_allnodes_ll);
        netif->mld_mac_filter(netif, &ip6_allnodes_ll, NETIF_ADD_MAC_FILTER);
    }
#endif /* LWIP_IPV6 && LWIP_IPV6_MLD */
}

err_t EthernetifInit(struct netif* netif)
{
    struct EtherNetif* ethernetif;

    LWIP_ASSERT("netif != NULL", (netif != NULL));

    ethernetif = mem_malloc(sizeof(struct EtherNetif));
    if (ethernetif == NULL) {
        PRINT_ERR("EthernetifInit: out of memory\n");
        return ERR_MEM;
    }

    netif->state = ethernetif;
    netif->link_layer_type = ETHERNET_DRIVER_IF;

#if LWIP_NETIF_HOSTNAME
    netif->hostname = LAN9118_NETIF_NAME;
#endif

    memcpy(netif->name, LAN9118_NETIF_NICK, (sizeof(netif->name) < sizeof(LAN9118_NETIF_NICK)) ? sizeof(netif->name) : sizeof(LAN9118_NETIF_NICK));
    memcpy(netif->full_name, LAN9118_NETIF_NICK, (IFNAMSIZ < sizeof(LAN9118_NETIF_NICK)) ? IFNAMSIZ : sizeof(LAN9118_NETIF_NICK));

    /* Initialize the hardware */
    enum lan9118_error_t init_successful = lan9118_init(g_dev, &DelayMs);
    if (init_successful != LAN9118_ERROR_NONE) {
        return false;
    }

    /* Init FIFO level interrupts: use Rx status level irq to trigger
     * interrupts for any non-processed packets, while Tx is not irq driven */
    lan9118_set_fifo_level_irq(g_dev, LAN9118_FIFO_LEVEL_IRQ_RX_STATUS_POS, LAN9118_FIFO_LEVEL_IRQ_LEVEL_MIN);
    lan9118_set_fifo_level_irq(g_dev, LAN9118_FIFO_LEVEL_IRQ_TX_STATUS_POS, LAN9118_FIFO_LEVEL_IRQ_LEVEL_MIN);
    lan9118_set_fifo_level_irq(g_dev, LAN9118_FIFO_LEVEL_IRQ_TX_DATA_POS, LAN9118_FIFO_LEVEL_IRQ_LEVEL_MAX);

    /* Enable Ethernet interrupts */
    lan9118_enable_interrupt(g_dev, LAN9118_INTERRUPT_RX_STATUS_FIFO_LEVEL);
    (void)LOS_HwiCreate(ETHERNET_IRQn, 0, 0, (HWI_PROC_FUNC)EthernetReceiveHandler, 0);

#if LWIP_IPV4
    netif->output = etharp_output;
#endif /* LWIP_IPV4 */

#if LWIP_IPV6
    netif->output_ip6 = ethip6_output;
#endif /* LWIP_IPV6 */

    netif->linkoutput = Lan9118LinkOut;

    /* initialize mac */
    LowLevelInit(netif);

    return ERR_OK;
}

void Lan9118NetInit(void)
{
    ip4_addr_t ip;
    ip4_addr_t mask;
    ip4_addr_t gw;
    static uint32_t overtime = 0;
    struct netif* NetifInitRet;

    ip.addr = ipaddr_addr(LAN9118_NETIF_TEST_IP);
    mask.addr = ipaddr_addr(LAN9118_NETIF_TEST_MASK);
    gw.addr = ipaddr_addr(LAN9118_NETIF_TEST_GW);

    NetifInitRet = netif_add(&g_NetIf, &ip, &mask, &gw, g_NetIf.state, EthernetifInit, tcpip_input);
    if (NetifInitRet == NULL) {
        PRINT_ERR("NetifInit error!\n");
        return;
    }

    netif_set_default(&g_NetIf);
    netifapi_netif_set_up(&g_NetIf);
    do {
        DelayMs(SLEEP_TIME_MS);
        overtime++;
        if (overtime > NETIF_SETUP_OVERTIME) {
            PRINT_ERR("netif_is_link_up overtime!\n");
            break;
        }
    } while (netif_is_link_up(&g_NetIf) == 0);
    if (overtime <= NETIF_SETUP_OVERTIME) {
        printf("netif init succeed!\n");
    }
}

void NetInit(void)
{

    printf("tcpip_init start\n");

    tcpip_init(NULL, NULL);

    printf("tcpip_init end\n");

    Lan9118NetInit();
}
