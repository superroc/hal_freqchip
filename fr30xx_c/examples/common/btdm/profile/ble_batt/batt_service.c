/**
 * Copyright (c) 2019, Freqchip
 *
 * All rights reserved.
 *
 *
 */

/*
 * INCLUDES (����ͷ�ļ�)
 */
#include <stdio.h>
#include <string.h>

#include "gap_api.h"
#include "gatt_api.h"
#include "gatt_sig_uuid.h"
#include "batt_service.h"


/*
 * MACROS (�궨��)
 */
#define CFG_CON                     20

/*
 * CONSTANTS (��������)
 */


// Device Information Service UUID: 0x180A
static const uint8_t batt_svc_uuid[UUID_SIZE_2] = UUID16_ARR(BATT_SERV_UUID);

/******************************* Battery level defination *******************************/
// Battery level data
static uint8_t battery_level;



/*
 * TYPEDEFS (���Ͷ���)
 */

/*
 * GLOBAL VARIABLES (ȫ�ֱ���)
 */


/*
 * LOCAL VARIABLES (���ر���)
 */
static uint8_t batt_svc_id;
static uint8_t batt_link_ntf_enable[CFG_CON] = {0};


/*********************************************************************
 * Profile Attributes - Table
 * ÿһ���һ��attribute�Ķ��塣
 * ��һ��attributeΪService �ĵĶ��塣
 * ÿһ������ֵ(characteristic)�Ķ��壬�����ٰ�������attribute�Ķ��壻
 * 1. ����ֵ����(Characteristic Declaration)
 * 2. ����ֵ��ֵ(Characteristic value)
 * 3. ����ֵ������(Characteristic description)
 * �����notification ����indication �Ĺ��ܣ��������ĸ�attribute�Ķ��壬����ǰ�涨���������������һ������ֵ�ͻ�������(client characteristic configuration)��
 *
 */

static const gatt_attribute_t batt_att_table[] =
{
    [IDX_BATT_SERVICE] = { { UUID_SIZE_2, UUID16_ARR(GATT_PRIMARY_SERVICE_UUID)},
        GATT_PROP_READ,  UUID_SIZE_2, (uint8_t *)batt_svc_uuid,
    },

    [IDX_BATT_LEVEL_CHAR_DECLARATION] = { { UUID_SIZE_2, UUID16_ARR(GATT_CHARACTER_UUID)},
        GATT_PROP_READ,0, NULL,
    },

    [IDX_BATT_LEVEL_CHAR_VALUE] = { { UUID_SIZE_2, UUID16_ARR(BATT_LEVEL_UUID)},
        GATT_PROP_READ | GATT_PROP_NOTI, sizeof(uint8_t),NULL,
    },

    [IDX_BATT_LEVEL_CCCD] = { {UUID_SIZE_2, UUID16_ARR(GATT_CLIENT_CHAR_CFG_UUID)},
        GATT_PROP_READ | GATT_PROP_WRITE_CMD | GATT_PROP_WRITE_REQ,sizeof(uint16_t),NULL,
    },
};


static void show_reg(uint8_t *data,uint32_t len,uint8_t dbg_on)
{
    uint32_t i=0;
    if(len == 0 || (dbg_on==0)) return;
    for(; i<len; i++)
    {
        printf("0x%02X,",data[i]);
    }
    printf("\r\n");
}

/*********************************************************************
 * @fn      batt_gatt_op_cmp_handler
 *
 * @brief   Gatt operation complete handler.
 *
 *
 * @param   p_operation  - operation that has compeleted
 *
 * @return  none.
 */
void batt_gatt_op_cmp_handler(void *p_operation)
{
//    if (p_operation->status == 0)
//    {}
}


/*********************************************************************
 * @fn      dis_gatt_msg_handler
 *
 * @brief   Device information gatt message handler.
 *
 *
 * @param   p_msg  - messages from GATT layer.
 *
 * @return  none.
 */
static uint16_t batt_gatt_msg_handler(struct gatt_msg *p_msg)
{
    switch(p_msg->msg_evt)
    {
        case GATTS_MSG_READ_REQ:
            if(p_msg->att_idx == IDX_BATT_LEVEL_CHAR_VALUE)
            {
                printf("batt_levle request\r\n");
                memcpy(p_msg->param.gatt_data.p_msg_data, &battery_level, sizeof(battery_level));
                return sizeof(battery_level);
            }
            else if(p_msg->att_idx == IDX_BATT_LEVEL_CCCD)
            {
                *(uint16_t *)(p_msg->param.gatt_data.p_msg_data) = batt_link_ntf_enable[p_msg->conn_idx];
                return sizeof(uint16_t);
            }
            break;

        case GATTS_MSG_WRITE_REQ:
            printf("batt_write:%d\r\n",p_msg->att_idx);
            show_reg(p_msg->param.gatt_data.p_msg_data,p_msg->param.gatt_data.msg_len,1);
            if(p_msg->att_idx == IDX_BATT_LEVEL_CCCD)
            {
                batt_link_ntf_enable[p_msg->conn_idx] = *(uint16_t *)(p_msg->param.gatt_data.p_msg_data);
            }
            break;

        case GATTC_MSG_CMP_EVT:
            batt_gatt_op_cmp_handler((struct gatt_op_cmp*)&(p_msg->param.gatt_op_cmp));
            break;
        case GATTC_MSG_LINK_CREATE:
            //co_printf("batt svc link[%d] create\r\n",p_msg->conn_idx);

            break;
        case GATTC_MSG_LINK_LOST:
            //co_printf("batt svc link[%d] lost\r\n",p_msg->conn_idx);
            batt_link_ntf_enable[p_msg->conn_idx] = 0x0;
            break;

        default:
            break;
    }
    return 0;
}


/*********************************************************************
 * @fn      batt_gatt_notify
 *
 * @brief   Send batt level notification to peer.
 *
 *
 * @param   conidx  - link idx.
 *          batt_level  - battery energy percentage.
 *
 * @return  none.
 */
void batt_gatt_notify(uint8_t conidx,uint8_t batt_level)
{
    battery_level = batt_level;
    if(batt_link_ntf_enable[conidx])
    {
        struct gatt_send_event ntf;
        ntf.conidx = conidx;
        ntf.svc_id = batt_svc_id;
        ntf.att_idx = IDX_BATT_LEVEL_CHAR_VALUE;
        ntf.data_len = 1;
        ntf.p_data = &battery_level;
        gatt_notification(&ntf);
    }
}

/*********************************************************************
 * @fn      batt_gatt_add_service
 *
 * @brief   Simple Profile add GATT service function.
 *          ���GATT service��ATT�����ݿ����档
 *
 * @param   None.
 *
 *
 * @return  None.
 */
void batt_gatt_add_service(void)
{
    gatt_service_t batt_profie_svc;

    batt_profie_svc.p_att_tb = batt_att_table;
    batt_profie_svc.att_nb = IDX_BATT_NB;
    batt_profie_svc.gatt_msg_handler = batt_gatt_msg_handler;

    battery_level = 100;
    batt_svc_id = gatt_add_service(&batt_profie_svc);
}




