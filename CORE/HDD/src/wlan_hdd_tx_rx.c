/*
 * Copyright (c) 2012-2014 The Linux Foundation. All rights reserved.
 *
 * Previously licensed under the ISC license by Qualcomm Atheros, Inc.
 *
 *
 * Permission to use, copy, modify, and/or distribute this software for
 * any purpose with or without fee is hereby granted, provided that the
 * above copyright notice and this permission notice appear in all
 * copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL
 * WARRANTIES WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED
 * WARRANTIES OF MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE
 * AUTHOR BE LIABLE FOR ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL
 * DAMAGES OR ANY DAMAGES WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR
 * PROFITS, WHETHER IN AN ACTION OF CONTRACT, NEGLIGENCE OR OTHER
 * TORTIOUS ACTION, ARISING OUT OF OR IN CONNECTION WITH THE USE OR
 * PERFORMANCE OF THIS SOFTWARE.
 */

/*
 * This file was originally distributed by Qualcomm Atheros, Inc.
 * under proprietary terms before Copyright ownership was assigned
 * to the Linux Foundation.
 */

/**===========================================================================

  \file  wlan_hdd_tx_rx.c

  \brief Linux HDD Tx/RX APIs

  ==========================================================================*/

/*---------------------------------------------------------------------------
  Include files
  -------------------------------------------------------------------------*/
#include <wlan_hdd_tx_rx.h>
#include <wlan_hdd_softap_tx_rx.h>
#include <wlan_hdd_dp_utils.h>
#include <wlan_qct_tl.h>
#include <linux/netdevice.h>
#include <linux/skbuff.h>
#include <linux/etherdevice.h>

#include <wlan_hdd_p2p.h>
#include <linux/wireless.h>
#include <net/cfg80211.h>
#include <net/ieee80211_radiotap.h>
#include "sapApi.h"

#ifdef FEATURE_WLAN_TDLS
#include "wlan_hdd_tdls.h"
#endif

#ifdef IPA_OFFLOAD
#include <wlan_hdd_ipa.h>
#endif

/*---------------------------------------------------------------------------
  Preprocessor definitions and constants
  -------------------------------------------------------------------------*/
const v_U8_t hddWmmAcToHighestUp[] = {
   SME_QOS_WMM_UP_RESV,
   SME_QOS_WMM_UP_EE,
   SME_QOS_WMM_UP_VI,
   SME_QOS_WMM_UP_NC
};

//Mapping Linux AC interpretation to TL AC.
const v_U8_t hdd_QdiscAcToTlAC[] = {
   WLANTL_AC_VO,
   WLANTL_AC_VI,
   WLANTL_AC_BE,
   WLANTL_AC_BK,
};

static struct sk_buff* hdd_mon_tx_fetch_pkt(hdd_adapter_t* pAdapter);

/*---------------------------------------------------------------------------
  Type declarations
  -------------------------------------------------------------------------*/

/*---------------------------------------------------------------------------
  Function definitions and documenation
  -------------------------------------------------------------------------*/

#ifdef DATA_PATH_UNIT_TEST
//Utility function to dump an sk_buff
static void dump_sk_buff(struct sk_buff * skb)
{
  VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,"%s: head = %p", __func__, skb->head);
  VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,"%s: data = %p", __func__, skb->data);
  VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,"%s: tail = %p", __func__, skb->tail);
  VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,"%s: end = %p", __func__, skb->end);
  VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,"%s: len = %d", __func__, skb->len);
  VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,"%s: data_len = %d", __func__, skb->data_len);
  VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,"%s: mac_len = %d", __func__, skb->mac_len);

  VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,"0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
     skb->data[0], skb->data[1], skb->data[2], skb->data[3], skb->data[4],
     skb->data[5], skb->data[6], skb->data[7]);
  VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,"0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x 0x%x",
     skb->data[8], skb->data[9], skb->data[10], skb->data[11], skb->data[12],
     skb->data[13], skb->data[14], skb->data[15]);
}

//Function for Unit Test only
static void transport_thread(hdd_adapter_t *pAdapter)
{
   v_U8_t staId;
   WLANTL_ACEnumType ac = WLANTL_AC_BE;
   vos_pkt_t *pVosPacket = NULL ;
   vos_pkt_t dummyPacket;
   WLANTL_MetaInfoType pktMetaInfo;
   WLANTL_RxMetaInfoType pktRxMetaInfo;
   VOS_STATUS status = VOS_STATUS_E_FAILURE;

   if (NULL == pAdapter)
   {
       VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
              FL("pAdapter is NULL"));
       VOS_ASSERT(0);
       return;
   }
   status = hdd_tx_fetch_packet_cbk( pAdapter->pvosContext,
                                     &staId,
                                     &ac,
                                     &pVosPacket,
                                     &pktMetaInfo );
  if (status != VOS_STATUS_SUCCESS && status != VOS_STATUS_E_EMPTY)
     VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                "%s: Test FAIL hdd_tx_fetch_packet_cbk", __func__);
  else
     VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                "%s: Test PASS hdd_tx_fetch_packet_cbk", __func__);

  status = hdd_tx_complete_cbk(pAdapter->pvosContext, &dummyPacket, VOS_STATUS_SUCCESS);
  if (status != VOS_STATUS_SUCCESS)
     VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                "%s: Test FAIL hdd_tx_complete_cbk", __func__);
  else
     VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                "%s: Test PASS hdd_tx_complete_cbk", __func__);

  status = hdd_tx_low_resource_cbk(pVosPacket, pAdapter);
  if (status != VOS_STATUS_SUCCESS)
     VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                "%s: Test FAIL hdd_tx_low_resource_cbk", __func__);
  else
     VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                "%s: Test PASS hdd_tx_low_resource_cbk", __func__);

  status = hdd_rx_packet_cbk( pAdapter->pvosContext,
                              &dummyPacket,
                              staId,
                              &pktRxMetaInfo);
  if (status != VOS_STATUS_SUCCESS)
     VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                "%s: Test FAIL hdd_rx_packet_cbk", __func__);
  else
     VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                "%s: Test PASS hdd_rx_packet_cbk", __func__);

}
#endif


/**============================================================================
  @brief hdd_flush_tx_queues() - Utility function to flush the TX queues

  @param pAdapter : [in] pointer to adapter context
  @return         : VOS_STATUS_E_FAILURE if any errors encountered
                  : VOS_STATUS_SUCCESS otherwise
  ===========================================================================*/
static VOS_STATUS hdd_flush_tx_queues( hdd_adapter_t *pAdapter )
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   v_SINT_t i = -1;
   hdd_list_node_t *anchor = NULL;
   skb_list_node_t *pktNode = NULL;
   struct sk_buff *skb = NULL;

   pAdapter->isVosLowResource = VOS_FALSE;

   while (++i != NUM_TX_QUEUES)
   {
      //Free up any packets in the Tx queue
      spin_lock_bh(&pAdapter->wmm_tx_queue[i].lock);
      while (true)
      {
         status = hdd_list_remove_front( &pAdapter->wmm_tx_queue[i], &anchor );
         if(VOS_STATUS_E_EMPTY != status)
         {
            pktNode = list_entry(anchor, skb_list_node_t, anchor);
            skb = pktNode->skb;
            //TODO
            //++pAdapter->stats.tx_dropped;
            ++pAdapter->hdd_stats.hddTxRxStats.txFlushed;
            ++pAdapter->hdd_stats.hddTxRxStats.txFlushedAC[i];
            kfree_skb(skb);
            continue;
         }
         break;
      }
      spin_unlock_bh(&pAdapter->wmm_tx_queue[i].lock);
      // backpressure is no longer in effect
      pAdapter->isTxSuspended[i] = VOS_FALSE;
   }

   return status;
}

/**============================================================================
  @brief hdd_flush_ibss_tx_queues() - Utility function to flush the TX queues
                                      in IBSS mode

  @param pAdapter : [in] pointer to adapter context
                  : [in] Staion Id
  @return         : VOS_STATUS_E_FAILURE if any errors encountered
                  : VOS_STATUS_SUCCESS otherwise
  ===========================================================================*/
void hdd_flush_ibss_tx_queues( hdd_adapter_t *pAdapter, v_U8_t STAId)
{
   v_U8_t i;
   v_SIZE_t size = 0;
   v_U8_t skbStaIdx;
   skb_list_node_t *pktNode = NULL;
   hdd_list_node_t *tmp = NULL, *next = NULL;
   struct netdev_queue *txq;
   struct sk_buff *skb = NULL;

   if (NULL == pAdapter)
   {
       VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
              FL("pAdapter is NULL %u"), STAId);
       VOS_ASSERT(0);
       return;
   }

   for (i = 0; i < NUM_TX_QUEUES; i++)
   {
      spin_lock_bh(&pAdapter->wmm_tx_queue[i].lock);

      if ( list_empty( &pAdapter->wmm_tx_queue[i].anchor ) )
      {
         spin_unlock_bh(&pAdapter->wmm_tx_queue[i].lock);
         continue;
      }

      /* Iterate through the queue and identify the data for STAId */
      list_for_each_safe(tmp, next, &pAdapter->wmm_tx_queue[i].anchor)
      {
         pktNode = list_entry(tmp, skb_list_node_t, anchor);
         if (pktNode != NULL)
         {
            skb = pktNode->skb;

            /* Get the STAId from data */
            skbStaIdx = *(v_U8_t *)(((v_U8_t *)(skb->data)) - 1);
            if (skbStaIdx == STAId)
            {
               /* Data for STAId is freed along with the queue node */

               list_del(tmp);
               kfree_skb(skb);

               ++pAdapter->hdd_stats.hddTxRxStats.txFlushed;
               ++pAdapter->hdd_stats.hddTxRxStats.txFlushedAC[i];
               pAdapter->wmm_tx_queue[i].count--;
            }
         }
      }

      /* Restart the queue only-if suspend and the queue was flushed */
      hdd_list_size( &pAdapter->wmm_tx_queue[i], &size );
      txq = netdev_get_tx_queue(pAdapter->dev, i);

      if (VOS_TRUE == pAdapter->isTxSuspended[i] &&
          size <= HDD_TX_QUEUE_LOW_WATER_MARK &&
          netif_tx_queue_stopped(txq) )
      {
         netif_tx_start_queue(txq);
         pAdapter->isTxSuspended[i] = VOS_FALSE;
         ++pAdapter->hdd_stats.hddTxRxStats.txDequeDePressured;
         ++pAdapter->hdd_stats.hddTxRxStats.txDequeDePressuredAC[i];
      }

      spin_unlock_bh(&pAdapter->wmm_tx_queue[i].lock);
   }
}

static struct sk_buff* hdd_mon_tx_fetch_pkt(hdd_adapter_t* pAdapter)
{
   skb_list_node_t *pktNode = NULL;
   struct sk_buff *skb = NULL;
   v_SIZE_t size = 0;
   WLANTL_ACEnumType ac = 0;
   VOS_STATUS status = VOS_STATUS_E_FAILURE;
   hdd_list_node_t *anchor = NULL;

   if (NULL == pAdapter)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
       FL("pAdapter is NULL"));
      VOS_ASSERT(0);
      return NULL;
   }

   // do we have any packets pending in this AC?
   hdd_list_size( &pAdapter->wmm_tx_queue[ac], &size );
   if( size == 0 )
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                 "%s: NO Packet Pending", __func__);
      return NULL;
   }

   //Remove the packet from the queue
   spin_lock_bh(&pAdapter->wmm_tx_queue[ac].lock);
   status = hdd_list_remove_front( &pAdapter->wmm_tx_queue[ac], &anchor );
   spin_unlock_bh(&pAdapter->wmm_tx_queue[ac].lock);

   if(VOS_STATUS_SUCCESS == status)
   {
      //If success then we got a valid packet from some AC
      pktNode = list_entry(anchor, skb_list_node_t, anchor);
      skb = pktNode->skb;
   }
   else
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                "%s: Not able to remove Packet from the list",
                  __func__);

      return NULL;
   }

   // if we are in a backpressure situation see if we can turn the hose back on
   if ( (pAdapter->isTxSuspended[ac]) &&
        (size <= HDD_TX_QUEUE_LOW_WATER_MARK) )
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_WARN,
                 "%s: TX queue[%d] re-enabled", __func__, ac);
      pAdapter->isTxSuspended[ac] = VOS_FALSE;
      /* Enable Queues which we have disabled earlier */
      netif_tx_start_all_queues( pAdapter->dev );
   }

   return skb;
}

void hdd_mon_tx_mgmt_pkt(hdd_adapter_t* pAdapter)
{
   hdd_cfg80211_state_t *cfgState;
   struct sk_buff* skb;
   hdd_adapter_t* pMonAdapter = NULL;
   struct ieee80211_hdr *hdr;

   if (pAdapter == NULL)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
       FL("pAdapter is NULL"));
      VOS_ASSERT(0);
      return;
   }

   pMonAdapter = hdd_get_adapter( pAdapter->pHddCtx, WLAN_HDD_MONITOR );
   if (pMonAdapter == NULL)
   {
       VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
       "%s: pMonAdapter is NULL", __func__);
       return;
   }

   cfgState = WLAN_HDD_GET_CFG_STATE_PTR( pAdapter );

   if( NULL != cfgState->buf )
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
          "%s: Already one MGMT packet Tx going on", __func__);
      return;
   }

   skb = hdd_mon_tx_fetch_pkt(pMonAdapter);

   if (NULL == skb)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
       "%s: No Packet Pending", __func__);
      return;
   }

   cfgState->buf = vos_mem_malloc( skb->len ); //buf;
   if( cfgState->buf == NULL )
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
          "%s: Failed to Allocate memory", __func__);
      goto fail;
   }

   cfgState->len = skb->len;

   vos_mem_copy( cfgState->buf, skb->data, skb->len);

   cfgState->skb = skb; //buf;
   cfgState->action_cookie = (uintptr_t)cfgState->buf;

   hdr = (struct ieee80211_hdr *)skb->data;
   if( (hdr->frame_control & HDD_FRAME_TYPE_MASK)
                                       == HDD_FRAME_TYPE_MGMT )
   {
       if( (hdr->frame_control & HDD_FRAME_SUBTYPE_MASK)
                                       == HDD_FRAME_SUBTYPE_DEAUTH )
       {
          hdd_softap_sta_deauth( pAdapter, hdr->addr1 );
          goto mgmt_handled;
       }
       else if( (hdr->frame_control & HDD_FRAME_SUBTYPE_MASK)
                                      == HDD_FRAME_SUBTYPE_DISASSOC )
       {
          hdd_softap_sta_disassoc( pAdapter, hdr->addr1 );
          goto mgmt_handled;
       }
   }
   VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO,
      "%s: Sending action frame to SAP to TX, Len %d", __func__, skb->len);

   if (VOS_STATUS_SUCCESS !=
#ifdef WLAN_FEATURE_MBSSID
      WLANSAP_SendAction( pAdapter->sessionCtx.ap.sapContext,
#else
      WLANSAP_SendAction( (WLAN_HDD_GET_CTX(pAdapter))->pvosContext,
#endif
                           skb->data, skb->len, 0) )

   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
          "%s: WLANSAP_SendAction returned fail", __func__);
      hdd_sendActionCnf( pAdapter, FALSE );
   }
   return;

mgmt_handled:
   hdd_sendActionCnf( pAdapter, TRUE );
   return;
fail:
   kfree_skb(pAdapter->skb_to_tx);
   pAdapter->skb_to_tx = NULL;
   return;
}

void hdd_mon_tx_work_queue(struct work_struct *work)
{
   hdd_adapter_t* pAdapter = container_of(work, hdd_adapter_t, monTxWorkQueue);
   hdd_mon_tx_mgmt_pkt(pAdapter);
}

int hdd_mon_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
   v_U16_t rt_hdr_len;
   struct ieee80211_hdr *hdr;
   hdd_adapter_t *pPgBkAdapter, *pAdapter =  WLAN_HDD_GET_PRIV_PTR(dev);
   struct ieee80211_radiotap_header *rtap_hdr =
                        (struct ieee80211_radiotap_header *)skb->data;

   /*Supplicant sends the EAPOL packet on monitor interface*/
   pPgBkAdapter = pAdapter->sessionCtx.monitor.pAdapterForTx;
   if(pPgBkAdapter == NULL)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_FATAL,
           "%s: No Adapter to piggy back. Dropping the pkt on monitor inf",
                                                                 __func__);
      goto fail; /* too short to be possibly valid */
   }

   /* check if toal skb length is greater then radio tab header length of not */
   if (unlikely(skb->len < sizeof(struct ieee80211_radiotap_header)))
      goto fail; /* too short to be possibly valid */

   /* check if radio tap header version is correct or not */
   if (unlikely(rtap_hdr->it_version))
      goto fail; /* only version 0 is supported */

   /*Strip off the radio tap header*/
   rt_hdr_len = ieee80211_get_radiotap_len(skb->data);

   /* check if skb length if greator then total radio tap header length ot not*/
   if (unlikely(skb->len < rt_hdr_len))
      goto fail;

   /* Update the trans_start for this netdev */
   dev->trans_start = jiffies;
   /*
    * fix up the pointers accounting for the radiotap
    * header still being in there.
    */
   skb_set_mac_header(skb, rt_hdr_len);
   skb_set_network_header(skb, rt_hdr_len);
   skb_set_transport_header(skb, rt_hdr_len);

   /* Pull rtap header out of the skb */
   skb_pull(skb, rt_hdr_len);

   /*Supplicant adds: radiotap Hdr + radiotap data + 80211 Header. So after
    * radio tap header and 802.11 header starts
    */
   hdr = (struct ieee80211_hdr *)skb->data;

   /* Send data frames through the normal Data path. In this path we will
    * conver rcvd 802.11 packet to 802.3 packet */
   if ( (hdr->frame_control & HDD_FRAME_TYPE_MASK)  == HDD_FRAME_TYPE_DATA)
   {
      v_U8_t da[6];
      v_U8_t sa[6];

      memcpy (da, hdr->addr1, VOS_MAC_ADDR_SIZE);
      memcpy (sa, hdr->addr2, VOS_MAC_ADDR_SIZE);

      /* Pull 802.11 MAC header */
      skb_pull(skb, HDD_80211_HEADER_LEN);

      if ( HDD_FRAME_SUBTYPE_QOSDATA ==
          (hdr->frame_control & HDD_FRAME_SUBTYPE_MASK))
      {
         skb_pull(skb, HDD_80211_HEADER_QOS_CTL);
      }

      /* Pull LLC header */
      skb_pull(skb, HDD_LLC_HDR_LEN);

      /* Create space for Ethernet header */
      skb_push(skb, HDD_MAC_HDR_SIZE*2);
      memcpy(&skb->data[0], da, HDD_MAC_HDR_SIZE);
      memcpy(&skb->data[HDD_DEST_ADDR_OFFSET], sa, HDD_MAC_HDR_SIZE);

      /* Only EAPOL Data packets are allowed through monitor interface */
      if (vos_be16_to_cpu(
         (*(unsigned short*)&skb->data[HDD_ETHERTYPE_802_1_X_FRAME_OFFSET]) )
                                                     != HDD_ETHERTYPE_802_1_X)
      {
         VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_FATAL,
           "%s: Not a Eapol packet. Drop this frame", __func__);
         //If not EAPOL frames, drop them.
         kfree_skb(skb);
         return NETDEV_TX_OK;
      }

      skb->protocol = htons(HDD_ETHERTYPE_802_1_X);

      hdd_hostapd_select_queue(pPgBkAdapter->dev, skb);
      return hdd_softap_hard_start_xmit( skb, pPgBkAdapter->dev );
   }
   else
   {
      VOS_STATUS status;
      WLANTL_ACEnumType ac = 0;
      skb_list_node_t *pktNode = NULL;
      v_SIZE_t pktListSize = 0;

      spin_lock(&pAdapter->wmm_tx_queue[ac].lock);
      //If we have already reached the max queue size, disable the TX queue
      if ( pAdapter->wmm_tx_queue[ac].count == pAdapter->wmm_tx_queue[ac].max_size)
      {
         /* We want to process one packet at a time, so lets disable all TX queues
           * and re-enable the queues once we get TX feedback for this packet */
         netif_tx_stop_all_queues(pAdapter->dev);
         pAdapter->isTxSuspended[ac] = VOS_TRUE;
         spin_unlock(&pAdapter->wmm_tx_queue[ac].lock);
         return NETDEV_TX_BUSY;
      }
      spin_unlock(&pAdapter->wmm_tx_queue[ac].lock);

      //Use the skb->cb field to hold the list node information
      pktNode = (skb_list_node_t *)&skb->cb;

      //Stick the OS packet inside this node.
      pktNode->skb = skb;

      INIT_LIST_HEAD(&pktNode->anchor);

      //Insert the OS packet into the appropriate AC queue
      spin_lock(&pAdapter->wmm_tx_queue[ac].lock);
      status = hdd_list_insert_back_size( &pAdapter->wmm_tx_queue[ac],
                                          &pktNode->anchor, &pktListSize );
      spin_unlock(&pAdapter->wmm_tx_queue[ac].lock);

      if ( !VOS_IS_STATUS_SUCCESS( status ) )
      {
         VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                "%s:Insert Tx queue failed. Pkt dropped", __func__);
         kfree_skb(skb);
         return NETDEV_TX_OK;
      }

      if ( pktListSize == 1 )
      {
         /* In this context we cannot acquire any mutex etc. And to transmit
          * this packet we need to call SME API. So to take care of this we will
          * schedule a workqueue
          */
         schedule_work(&pPgBkAdapter->monTxWorkQueue);
      }
      return NETDEV_TX_OK;
   }

fail:
   VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_WARN,
           "%s: Packet Rcvd at Monitor interface is not proper,"
           " Dropping the packet",
            __func__);
   kfree_skb(skb);
   return NETDEV_TX_OK;
}

#ifdef QCA_LL_TX_FLOW_CT
/**============================================================================
  @brief hdd_tx_resume_timer_expired_handler() - Resume OS TX Q timer expired
      handler.
      If Blocked OS Q is not resumed during timeout period, to prevent
      permanent stall, resume OS Q forcefully.

  @param adapter_context : [in] pointer to vdev apdapter

  @return         : NONE
  ===========================================================================*/
void hdd_tx_resume_timer_expired_handler(void *adapter_context)
{
   hdd_adapter_t *pAdapter = (hdd_adapter_t *)adapter_context;

   if (!pAdapter)
   {
      /* INVALID ARG */
      return;
   }

   netif_tx_wake_all_queues(pAdapter->dev);
   return;
}

/**============================================================================
  @brief hdd_tx_resume_cb() - Resume OS TX Q.
      Q was stopped due to WLAN TX path low resource condition

  @param adapter_context : [in] pointer to vdev apdapter
  @param tx_resume       : [in] TX Q resume trigger

  @return         : NONE
  ===========================================================================*/
void hdd_tx_resume_cb(void *adapter_context,
                        v_BOOL_t tx_resume)
{
   hdd_adapter_t *pAdapter = (hdd_adapter_t *)adapter_context;

   if (!pAdapter)
   {
      /* INVALID ARG */
      return;
   }

   /* Resume TX  */
   if (VOS_TRUE == tx_resume)
   {
       if (VOS_TIMER_STATE_STOPPED !=
          vos_timer_getCurrentState(&pAdapter->tx_flow_control_timer))
       {
          vos_timer_stop(&pAdapter->tx_flow_control_timer);
       }
      netif_tx_wake_all_queues(pAdapter->dev);
   }
#if defined(CONFIG_PER_VDEV_TX_DESC_POOL)
    else if (VOS_FALSE == tx_resume)  /* Pause TX  */
    {
        netif_tx_stop_all_queues(pAdapter->dev);
        if (VOS_TIMER_STATE_STOPPED ==
            vos_timer_getCurrentState(&pAdapter->tx_flow_control_timer))
        {
            VOS_STATUS status;
            status = vos_timer_start(&pAdapter->tx_flow_control_timer,
                          WLAN_HDD_TX_FLOW_CONTROL_OS_Q_BLOCK_TIME);
            if ( !VOS_IS_STATUS_SUCCESS(status) )
            {
                VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s: Failed to initialize tx_flow_control_timer", __func__);
            }
        }
    }
#endif

   return;
}
#endif /* QCA_LL_TX_FLOW_CT */

/**============================================================================
  @brief hdd_hard_start_xmit() - Function registered with the Linux OS for
  transmitting packets. This version of the function directly passes the packet
  to Transport Layer.

  @param skb      : [in]  pointer to OS packet (sk_buff)
  @param dev      : [in] pointer to network device

  @return         : NET_XMIT_DROP if packets are dropped
                  : NET_XMIT_SUCCESS if packet is enqueued succesfully
  ===========================================================================*/
int hdd_hard_start_xmit(struct sk_buff *skb, struct net_device *dev)
{
   VOS_STATUS status;
   WLANTL_ACEnumType ac;
   sme_QosWmmUpType up;
   hdd_adapter_t *pAdapter =  WLAN_HDD_GET_PRIV_PTR(dev);
   v_BOOL_t granted;
   v_U8_t STAId = WLAN_MAX_STA_COUNT;
   hdd_station_ctx_t *pHddStaCtx = &pAdapter->sessionCtx.station;
#ifdef QCA_PKT_PROTO_TRACE
   hdd_context_t *hddCtxt = WLAN_HDD_GET_CTX(pAdapter);
   v_U8_t proto_type = 0;
#endif /* QCA_PKT_PROTO_TRACE */

#ifdef QCA_WIFI_FTM
   if (hdd_get_conparam() == VOS_FTM_MODE) {
       kfree_skb(skb);
       return NETDEV_TX_OK;
   }
#endif

   ++pAdapter->hdd_stats.hddTxRxStats.txXmitCalled;

   if (WLAN_HDD_IBSS == pAdapter->device_mode)
   {
      v_MACADDR_t *pDestMacAddress = (v_MACADDR_t*)skb->data;

      STAId = *(v_U8_t *)(((v_U8_t *)(skb->data)) - 1);

      if ((STAId == HDD_WLAN_INVALID_STA_ID) &&
          (vos_is_macaddr_broadcast( pDestMacAddress ) ||
           vos_is_macaddr_group(pDestMacAddress)))
      {
         STAId = IBSS_BROADCAST_STAID;
         VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO_LOW,
                 "%s: BC/MC packet", __func__);
      }
      else if (STAId == HDD_WLAN_INVALID_STA_ID)
      {
         VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_WARN,
                   "%s: Received Unicast frame with invalid staID", __func__);
         ++pAdapter->stats.tx_dropped;
         ++pAdapter->hdd_stats.hddTxRxStats.txXmitDropped;
         kfree_skb(skb);
         return NETDEV_TX_OK;
      }
   }
   else
   {
      STAId = pHddStaCtx->conn_info.staId[0];
   }

#ifdef QCA_LL_TX_FLOW_CT
   if (VOS_FALSE == WLANTL_GetTxResource((WLAN_HDD_GET_CTX(pAdapter))->pvosContext,
                                         pAdapter->sessionId,
                                         pAdapter->tx_flow_low_watermark,
                                         pAdapter->tx_flow_high_watermark_offset))
   {
       netif_tx_stop_all_queues(dev);
       if (VOS_TIMER_STATE_STOPPED ==
           vos_timer_getCurrentState(&pAdapter->tx_flow_control_timer))
       {
          vos_timer_start(&pAdapter->tx_flow_control_timer,
                          WLAN_HDD_TX_FLOW_CONTROL_OS_Q_BLOCK_TIME);
       }
   }
#endif /* QCA_LL_TX_FLOW_CT */

   //Get TL AC corresponding to Qdisc queue index/AC.
   ac = hdd_QdiscAcToTlAC[skb->queue_mapping];

#ifdef IPA_OFFLOAD
   if(!(NBUF_OWNER_ID(skb) == IPA_NBUF_OWNER_ID)) {
#endif

   /* Check if the buffer has enough header room */
   skb = skb_unshare(skb, GFP_ATOMIC);
   if (!skb)
       goto drop_pkt;

   if (skb_headroom(skb) < dev->hard_header_len) {
       struct sk_buff *tmp;
       tmp = skb;
       skb = skb_realloc_headroom(tmp, dev->hard_header_len);
       dev_kfree_skb(tmp);
       if (!skb)
           goto drop_pkt;
   }
#ifdef IPA_OFFLOAD
   }
#endif
   //user priority from IP header, which is already extracted and set from
   //select_queue call back function
   up = skb->priority;

   ++pAdapter->hdd_stats.hddTxRxStats.txXmitClassifiedAC[ac];
#ifdef HDD_WMM_DEBUG
   VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_FATAL,
              "%s: Classified as ac %d up %d", __func__, ac, up);
#endif // HDD_WMM_DEBUG

   if (HDD_PSB_CHANGED == pAdapter->psbChanged)
   {
      /* Function which will determine acquire admittance for a
       * WMM AC is required or not based on psb configuration done
       * in the framework
       */
       hdd_wmm_acquire_access_required(pAdapter, ac);
   }

   //Make sure we have access to this access category
   if (((pAdapter->psbChanged & (1 << ac)) && likely(pAdapter->hddWmmStatus.wmmAcStatus[ac].wmmAcAccessAllowed)) ||
           (pHddStaCtx->conn_info.uIsAuthenticated == VOS_FALSE))
   {
      granted = VOS_TRUE;
   }
   else
   {
      status = hdd_wmm_acquire_access( pAdapter, ac, &granted );
      pAdapter->psbChanged |= (1 << ac);
   }

   if (!granted) {
      bool isDefaultAc = VOS_FALSE;
      /* ADDTS request for this AC is sent, for now
       * send this packet through next avaiable lower
       * Access category until ADDTS negotiation completes.
       */
      while (!likely(pAdapter->hddWmmStatus.wmmAcStatus[ac].wmmAcAccessAllowed)) {
         switch(ac) {
            case WLANTL_AC_VO:
               ac = WLANTL_AC_VI;
               up = SME_QOS_WMM_UP_VI;
               break;
            case WLANTL_AC_VI:
               ac = WLANTL_AC_BE;
               up = SME_QOS_WMM_UP_BE;
               break;
            case WLANTL_AC_BE:
               ac = WLANTL_AC_BK;
               up = SME_QOS_WMM_UP_BK;
               break;
            default:
               ac = WLANTL_AC_BK;
               up = SME_QOS_WMM_UP_BK;
               isDefaultAc = VOS_TRUE;
               break;
         }
         if (isDefaultAc)
             break;
      }
      skb->priority = up;
      skb->queue_mapping = hddLinuxUpToAcMap[up];
   }

#ifdef QCA_PKT_PROTO_TRACE
   if ((hddCtxt->cfg_ini->gEnableDebugLog & VOS_PKT_TRAC_TYPE_EAPOL) ||
       (hddCtxt->cfg_ini->gEnableDebugLog & VOS_PKT_TRAC_TYPE_DHCP))
   {
      proto_type = vos_pkt_get_proto_type(skb,
                      hddCtxt->cfg_ini->gEnableDebugLog, 0);
      if (VOS_PKT_TRAC_TYPE_EAPOL & proto_type)
      {
         vos_pkt_trace_buf_update("ST:T:EPL");
      }
      else if (VOS_PKT_TRAC_TYPE_DHCP & proto_type)
      {
         vos_pkt_trace_buf_update("ST:T:DHC");
      }
   }
#endif /* QCA_PKT_PROTO_TRACE */

   pAdapter->stats.tx_bytes += skb->len;
   ++pAdapter->stats.tx_packets;
   ++pAdapter->hdd_stats.hddTxRxStats.pkt_tx_count;

   /*
    * TODO: Should we stop net queues when txrx returns non-NULL?.
    */
   if (WLANTL_SendSTA_DataFrame((WLAN_HDD_GET_CTX(pAdapter))->pvosContext,
				STAId, (adf_nbuf_t) skb
#ifdef QCA_PKT_PROTO_TRACE
                               , proto_type
#endif /* QCA_PKT_PROTO_TRACE */
                                ) != NULL) {
	VOS_TRACE(VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_WARN,
		  "%s: Failed to send packet to txrx for staid:%d",
		  __func__, STAId);
	goto drop_pkt;
   }

   dev->trans_start = jiffies;

   return NETDEV_TX_OK;

drop_pkt:
   ++pAdapter->stats.tx_dropped;
   ++pAdapter->hdd_stats.hddTxRxStats.txXmitDropped;
   ++pAdapter->hdd_stats.hddTxRxStats.txXmitDroppedAC[ac];
   kfree_skb(skb);
   return NETDEV_TX_OK;
}

/**============================================================================
  @brief hdd_Ibss_GetStaId() - Get the StationID using the Peer Mac address

  @param pHddStaCtx : [in] pointer to HDD Station Context
  pMacAddress [in]  pointer to Peer Mac address
  staID [out]  pointer to Station Index
  @return    : VOS_STATUS_SUCCESS/VOS_STATUS_E_FAILURE
  ===========================================================================*/

VOS_STATUS hdd_Ibss_GetStaId(hdd_station_ctx_t *pHddStaCtx, v_MACADDR_t *pMacAddress, v_U8_t *staId)
{
    v_U8_t idx;

    for (idx = 0; idx < HDD_MAX_NUM_IBSS_STA; idx++)
    {
        if (vos_mem_compare(&pHddStaCtx->conn_info.peerMacAddress[ idx ],
                pMacAddress, sizeof(v_MACADDR_t)))
        {
            *staId = pHddStaCtx->conn_info.staId[idx];
            return VOS_STATUS_SUCCESS;
        }
    }

    return VOS_STATUS_E_FAILURE;
}

/**============================================================================
  @brief hdd_tx_timeout() - Function called by OS if there is any
  timeout during transmission. Since HDD simply enqueues packet
  and returns control to OS right away, this would never be invoked

  @param dev : [in] pointer to Libra network device
  @return    : None
  ===========================================================================*/
void hdd_tx_timeout(struct net_device *dev)
{
   hdd_adapter_t *pAdapter =  WLAN_HDD_GET_PRIV_PTR(dev);
   struct netdev_queue *txq;
   int i = 0;

   VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
      "%s: Transmission timeout occurred", __func__);
   //Getting here implies we disabled the TX queues for too long. Queues are
   //disabled either because of disassociation or low resource scenarios. In
   //case of disassociation it is ok to ignore this. But if associated, we have
   //do possible recovery here

   VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO,
              "num_bytes AC0: %d AC1: %d AC2: %d AC3: %d",
              pAdapter->wmm_tx_queue[0].count,
              pAdapter->wmm_tx_queue[1].count,
              pAdapter->wmm_tx_queue[2].count,
              pAdapter->wmm_tx_queue[3].count);

   VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO,
              "tx_suspend AC0: %d AC1: %d AC2: %d AC3: %d",
              pAdapter->isTxSuspended[0],
              pAdapter->isTxSuspended[1],
              pAdapter->isTxSuspended[2],
              pAdapter->isTxSuspended[3]);

   for (i = 0; i < 8; i++)
   {
      txq = netdev_get_tx_queue(dev, i);
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO,
                "Queue%d status: %d", i, netif_tx_queue_stopped(txq));
   }

   VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO,
              "carrier state: %d", netif_carrier_ok(dev));
}


/**============================================================================
  @brief hdd_stats() - Function registered with the Linux OS for
  device TX/RX statistic

  @param dev      : [in] pointer to Libra network device

  @return         : pointer to net_device_stats structure
  ===========================================================================*/
struct net_device_stats* hdd_stats(struct net_device *dev)
{
   hdd_adapter_t *pAdapter =  WLAN_HDD_GET_PRIV_PTR(dev);

   return &pAdapter->stats;
}


/**============================================================================
  @brief hdd_init_tx_rx() - Init function to initialize Tx/RX
  modules in HDD

  @param pAdapter : [in] pointer to adapter context
  @return         : VOS_STATUS_E_FAILURE if any errors encountered
                  : VOS_STATUS_SUCCESS otherwise
  ===========================================================================*/
VOS_STATUS hdd_init_tx_rx( hdd_adapter_t *pAdapter )
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   v_SINT_t i = -1;

   if ( NULL == pAdapter )
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
              FL("pAdapter is NULL"));
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }

   pAdapter->isVosOutOfResource = VOS_FALSE;
   pAdapter->isVosLowResource = VOS_FALSE;

   //vos_mem_zero(&pAdapter->stats, sizeof(struct net_device_stats));
   //Will be zeroed out during alloc

   while (++i != NUM_TX_QUEUES)
   {
      pAdapter->isTxSuspended[i] = VOS_FALSE;
      hdd_list_init( &pAdapter->wmm_tx_queue[i], HDD_TX_QUEUE_MAX_LEN);
   }

   return status;
}


/**============================================================================
  @brief hdd_deinit_tx_rx() - Deinit function to clean up Tx/RX
  modules in HDD

  @param pAdapter : [in] pointer to adapter context
  @return         : VOS_STATUS_E_FAILURE if any errors encountered
                  : VOS_STATUS_SUCCESS otherwise
  ===========================================================================*/
VOS_STATUS hdd_deinit_tx_rx( hdd_adapter_t *pAdapter )
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   v_SINT_t i = -1;

   if ( NULL == pAdapter )
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
              FL("pAdapter is NULL"));
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }

   status = hdd_flush_tx_queues(pAdapter);
   if (VOS_STATUS_SUCCESS != status)
       VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_WARN,
          FL("failed to flush tx queues"));

   while (++i != NUM_TX_QUEUES)
   {
      //Free up actual list elements in the Tx queue
      hdd_list_destroy( &pAdapter->wmm_tx_queue[i] );
   }

   return status;
}


/**============================================================================
  @brief hdd_disconnect_tx_rx() - Disconnect function to clean up Tx/RX
  modules in HDD

  @param pAdapter : [in] pointer to adapter context
  @return         : VOS_STATUS_E_FAILURE if any errors encountered
                  : VOS_STATUS_SUCCESS otherwise
  ===========================================================================*/
VOS_STATUS hdd_disconnect_tx_rx( hdd_adapter_t *pAdapter )
{
   return hdd_flush_tx_queues(pAdapter);
}


/**============================================================================
  @brief hdd_IsEAPOLPacket() - Checks the packet is EAPOL or not.

  @param pVosPacket : [in] pointer to vos packet
  @return         : VOS_TRUE if the packet is EAPOL
                  : VOS_FALSE otherwise
  ===========================================================================*/

v_BOOL_t hdd_IsEAPOLPacket( vos_pkt_t *pVosPacket )
{
    VOS_STATUS vosStatus  = VOS_STATUS_SUCCESS;
    v_BOOL_t   fEAPOL     = VOS_FALSE;
    void       *pBuffer   = NULL;


    vosStatus = vos_pkt_peek_data( pVosPacket, (v_SIZE_t)HDD_ETHERTYPE_802_1_X_FRAME_OFFSET,
                          &pBuffer, HDD_ETHERTYPE_802_1_X_SIZE );
    if (VOS_IS_STATUS_SUCCESS( vosStatus ) )
    {
       if (pBuffer && vos_be16_to_cpu( *(unsigned short*)pBuffer) == HDD_ETHERTYPE_802_1_X )
       {
          fEAPOL = VOS_TRUE;
       }
    }

   return fEAPOL;
}


#ifdef FEATURE_WLAN_WAPI // Need to update this function
/**============================================================================
  @brief hdd_IsWAIPacket() - Checks the packet is WAI or not.

  @param pVosPacket : [in] pointer to vos packet
  @return         : VOS_TRUE if the packet is WAI
                  : VOS_FALSE otherwise
  ===========================================================================*/

v_BOOL_t hdd_IsWAIPacket( vos_pkt_t *pVosPacket )
{
    VOS_STATUS vosStatus  = VOS_STATUS_SUCCESS;
    v_BOOL_t   fIsWAI     = VOS_FALSE;
    void       *pBuffer   = NULL;

    // Need to update this function
    vosStatus = vos_pkt_peek_data( pVosPacket, (v_SIZE_t)HDD_ETHERTYPE_802_1_X_FRAME_OFFSET,
                          &pBuffer, HDD_ETHERTYPE_802_1_X_SIZE );

    if (VOS_IS_STATUS_SUCCESS( vosStatus ) )
    {
       if (pBuffer && vos_be16_to_cpu( *((unsigned short*)pBuffer)) == HDD_ETHERTYPE_WAI)
       {
          fIsWAI = VOS_TRUE;
       }
    }

   return fIsWAI;
}
#endif /* FEATURE_WLAN_WAPI */

/**============================================================================
  @brief hdd_tx_complete_cbk() - Callback function invoked by TL
  to indicate that a packet has been transmitted across the SDIO bus
  succesfully. OS packet resources can be released after this cbk.

  @param vosContext   : [in] pointer to VOS context
  @param pVosPacket   : [in] pointer to VOS packet (containing skb)
  @param vosStatusIn  : [in] status of the transmission

  @return             : VOS_STATUS_E_FAILURE if any errors encountered
                      : VOS_STATUS_SUCCESS otherwise
  ===========================================================================*/
VOS_STATUS hdd_tx_complete_cbk( v_VOID_t *vosContext,
                                vos_pkt_t *pVosPacket,
                                VOS_STATUS vosStatusIn )
{
   VOS_STATUS status = VOS_STATUS_SUCCESS;
   hdd_adapter_t *pAdapter = NULL;
   hdd_context_t *pHddCtx = NULL;
   void* pOsPkt = NULL;

   if( ( NULL == vosContext ) || ( NULL == pVosPacket )  )
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s: Null params being passed", __func__);
      return VOS_STATUS_E_FAILURE;
   }

   //Return the skb to the OS
   status = vos_pkt_get_os_packet( pVosPacket, &pOsPkt, VOS_TRUE );
   if (!VOS_IS_STATUS_SUCCESS( status ))
   {
      //This is bad but still try to free the VOSS resources if we can
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s: Failure extracting skb from vos pkt", __func__);
      vos_pkt_return_packet( pVosPacket );
      return VOS_STATUS_E_FAILURE;
   }

   //Get the HDD context.
   pHddCtx = (hdd_context_t *)vos_get_context( VOS_MODULE_ID_HDD, vosContext );
   //Get the Adapter context.
   pAdapter = hdd_get_adapter(pHddCtx,WLAN_HDD_INFRA_STATION);
   if(pAdapter == NULL)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_INFO,
                "%s: HDD adapter context is Null", __func__);
   }
   else
   {
      ++pAdapter->hdd_stats.hddTxRxStats.txCompleted;
   }

   kfree_skb((struct sk_buff *)pOsPkt);

   //Return the VOS packet resources.
   status = vos_pkt_return_packet( pVosPacket );
   if (!VOS_IS_STATUS_SUCCESS( status ))
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s: Could not return VOS packet to the pool", __func__);
   }

   return status;
}


/**============================================================================
  @brief hdd_tx_fetch_packet_cbk() - Callback function invoked by TL to
  fetch a packet for transmission.

  @param vosContext   : [in] pointer to VOS context
  @param staId        : [in] Station for which TL is requesting a pkt
  @param ac           : [in] access category requested by TL
  @param pVosPacket   : [out] pointer to VOS packet packet pointer
  @param pPktMetaInfo : [out] pointer to meta info for the pkt

  @return             : VOS_STATUS_E_EMPTY if no packets to transmit
                      : VOS_STATUS_E_FAILURE if any errors encountered
                      : VOS_STATUS_SUCCESS otherwise
  ===========================================================================*/
VOS_STATUS hdd_tx_fetch_packet_cbk( v_VOID_t *vosContext,
                                    v_U8_t *pStaId,
                                    WLANTL_ACEnumType  ac,
                                    vos_pkt_t **ppVosPacket,
                                    WLANTL_MetaInfoType *pPktMetaInfo )
{
   VOS_STATUS status = VOS_STATUS_E_FAILURE;
   hdd_adapter_t *pAdapter = NULL;
   hdd_context_t *pHddCtx = NULL;
   hdd_list_node_t *anchor = NULL;
   skb_list_node_t *pktNode = NULL;
   struct sk_buff *skb = NULL;
   vos_pkt_t *pVosPacket = NULL;
   v_MACADDR_t* pDestMacAddress = NULL;
   v_TIME_t timestamp;
   WLANTL_ACEnumType newAc;
   v_SIZE_t size = 0;
   tANI_U8   acAdmitted, i;

   //Sanity check on inputs
   if ( ( NULL == vosContext ) ||
        ( NULL == pStaId ) ||
        ( NULL == ppVosPacket ) ||
        ( NULL == pPktMetaInfo ) )
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s: Null Params being passed", __func__);
      return VOS_STATUS_E_FAILURE;
   }

   //Get the HDD context.
   pHddCtx = (hdd_context_t *)vos_get_context( VOS_MODULE_ID_HDD, vosContext );
   if(pHddCtx == NULL)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s: HDD adapter context is Null", __func__);
      return VOS_STATUS_E_FAILURE;
   }

   pAdapter = pHddCtx->sta_to_adapter[*pStaId];
   if ((NULL == pAdapter) || (WLAN_HDD_ADAPTER_MAGIC != pAdapter->magic))
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
              FL("pAdapter is NULL %u"), *pStaId);
      VOS_ASSERT(0);
      return VOS_STATUS_E_FAILURE;
   }

   ++pAdapter->hdd_stats.hddTxRxStats.txFetched;

   *ppVosPacket = NULL;

   //Make sure the AC being asked for is sane
   if( ac >= WLANTL_MAX_AC || ac < 0)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s: Invalid AC %d passed by TL", __func__, ac);
      return VOS_STATUS_E_FAILURE;
   }

   ++pAdapter->hdd_stats.hddTxRxStats.txFetchedAC[ac];

#ifdef HDD_WMM_DEBUG
   VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_FATAL,
             "%s: AC %d passed by TL", __func__, ac);
#endif // HDD_WMM_DEBUG

   // We find an AC with packets
   // or we determine we have no more packets to send
   // HDD is not allowed to change AC.

   // has this AC been admitted? or
   // To allow EAPOL packets when not authenticated
   if (unlikely((0==pAdapter->hddWmmStatus.wmmAcStatus[ac].wmmAcAccessAllowed) &&
                (WLAN_HDD_GET_STATION_CTX_PTR(pAdapter))->conn_info.uIsAuthenticated))
   {
      ++pAdapter->hdd_stats.hddTxRxStats.txFetchEmpty;
#ifdef HDD_WMM_DEBUG
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_FATAL,
                 "%s: no packets pending", __func__);
#endif // HDD_WMM_DEBUG
      return VOS_STATUS_E_FAILURE;
   }

   // do we have any packets pending in this AC?
   hdd_list_size( &pAdapter->wmm_tx_queue[ac], &size );
   if( size >  0 )
   {
       // yes, so process it
#ifdef HDD_WMM_DEBUG
       VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_FATAL,
                       "%s: AC %d has packets pending", __func__, ac);
#endif // HDD_WMM_DEBUG
   }
   else
   {
      ++pAdapter->hdd_stats.hddTxRxStats.txFetchEmpty;
#ifdef HDD_WMM_DEBUG
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_FATAL,
                   "%s: no packets pending", __func__);
#endif // HDD_WMM_DEBUG
      return VOS_STATUS_E_FAILURE;
   }

   //Get the vos packet. I don't want to dequeue and enqueue again if we are out of VOS resources
   //This simplifies the locking and unlocking of Tx queue
   status = vos_pkt_wrap_data_packet( &pVosPacket,
                                      VOS_PKT_TYPE_TX_802_3_DATA,
                                      NULL, //OS Pkt is not being passed
                                      hdd_tx_low_resource_cbk,
                                      pAdapter );

   if (status == VOS_STATUS_E_ALREADY || status == VOS_STATUS_E_RESOURCES)
   {
      //Remember VOS is in a low resource situation
      pAdapter->isVosOutOfResource = VOS_TRUE;
      ++pAdapter->hdd_stats.hddTxRxStats.txFetchLowResources;
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_WARN,"%s: VOSS in Low Resource scenario", __func__);
      //TL will now think we have no more packets in this AC
      return VOS_STATUS_E_FAILURE;
   }

   //Remove the packet from the queue
   spin_lock_bh(&pAdapter->wmm_tx_queue[ac].lock);
   status = hdd_list_remove_front( &pAdapter->wmm_tx_queue[ac], &anchor );
   spin_unlock_bh(&pAdapter->wmm_tx_queue[ac].lock);

   if(VOS_STATUS_SUCCESS == status)
   {
      //If success then we got a valid packet from some AC
      pktNode = list_entry(anchor, skb_list_node_t, anchor);
      skb = pktNode->skb;
   }
   else
   {
      ++pAdapter->hdd_stats.hddTxRxStats.txFetchDequeueError;
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_WARN, "%s: Error in de-queuing "
         "skb from Tx queue status = %d", __func__, status );
      vos_pkt_return_packet(pVosPacket);
      return VOS_STATUS_E_FAILURE;
   }

   //Attach skb to VOS packet.
   status = vos_pkt_set_os_packet( pVosPacket, skb );
   if (status != VOS_STATUS_SUCCESS)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_WARN,"%s: Error attaching skb", __func__);
      vos_pkt_return_packet(pVosPacket);
      ++pAdapter->stats.tx_dropped;
      ++pAdapter->hdd_stats.hddTxRxStats.txFetchDequeueError;
      kfree_skb(skb);
      return VOS_STATUS_E_FAILURE;
   }

   //Just being paranoid. To be removed later
   if(pVosPacket == NULL)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_WARN,"%s: VOS packet returned by VOSS is NULL", __func__);
      ++pAdapter->stats.tx_dropped;
      ++pAdapter->hdd_stats.hddTxRxStats.txFetchDequeueError;
      kfree_skb(skb);
      return VOS_STATUS_E_FAILURE;
   }

#ifdef FEATURE_WLAN_TDLS
    if (eTDLS_SUPPORT_ENABLED == pHddCtx->tdls_mode)
    {
        hdd_station_ctx_t *pHddStaCtx = &pAdapter->sessionCtx.station;
        u8 mac[6];

        wlan_hdd_tdls_extract_da(skb, mac);

        if (vos_is_macaddr_group((v_MACADDR_t *)mac)) {
            VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO_MED,
                      "broadcast packet, not adding to peer list");
        } else if (memcmp(pHddStaCtx->conn_info.bssId,
                            mac, 6) != 0) {
            VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO_MED,
                      "extract mac: " MAC_ADDRESS_STR,
                      MAC_ADDR_ARRAY(mac) );

            wlan_hdd_tdls_increment_pkt_count(pAdapter, mac, 1);
        } else {
            VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO_MED,
                       "packet da is bssid, not adding to peer list");
        }
    }
#endif

   //Return VOS packet to TL;
   *ppVosPacket = pVosPacket;

   //Fill out the meta information needed by TL
   //FIXME This timestamp is really the time stamp of wrap_data_packet
   vos_pkt_get_timestamp( pVosPacket, &timestamp );
   pPktMetaInfo->usTimeStamp = (v_U16_t)timestamp;

   if(pAdapter->sessionCtx.station.conn_info.uIsAuthenticated == VOS_TRUE)
      pPktMetaInfo->ucIsEapol = 0;
   else
      pPktMetaInfo->ucIsEapol = hdd_IsEAPOLPacket( pVosPacket ) ? 1 : 0;

#ifdef FEATURE_WLAN_WAPI
   // Override usIsEapol value when its zero for WAPI case
      pPktMetaInfo->ucIsWai = hdd_IsWAIPacket( pVosPacket ) ? 1 : 0;
#endif /* FEATURE_WLAN_WAPI */

   if ((HDD_WMM_USER_MODE_NO_QOS == pHddCtx->cfg_ini->WmmMode) ||
       (!pAdapter->hddWmmStatus.wmmQap))
   {
      // either we don't want QoS or the AP doesn't support QoS
      pPktMetaInfo->ucUP = 0;
      pPktMetaInfo->ucTID = 0;
   }
   else
   {
      /* 1. Check if ACM is set for this AC
       * 2. If set, check if this AC had already admitted
       * 3. If not already admitted, downgrade the UP to next best UP */
      if(!pAdapter->hddWmmStatus.wmmAcStatus[ac].wmmAcAccessRequired ||
         pAdapter->hddWmmStatus.wmmAcStatus[ac].wmmAcTspecValid)
      {
        pPktMetaInfo->ucUP = pktNode->userPriority;
        pPktMetaInfo->ucTID = pPktMetaInfo->ucUP;
      }
      else
      {
        //Downgrade the UP
        acAdmitted = pAdapter->hddWmmStatus.wmmAcStatus[ac].wmmAcTspecValid;
        newAc = WLANTL_AC_BK;
        for (i=ac-1; i>0; i--)
        {
            if (pAdapter->hddWmmStatus.wmmAcStatus[i].wmmAcAccessRequired == 0)
            {
                newAc = i;
                break;
            }
        }
        pPktMetaInfo->ucUP = hddWmmAcToHighestUp[newAc];
        pPktMetaInfo->ucTID = pPktMetaInfo->ucUP;
        VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO_LOW,"Downgrading UP %d to UP %d ", pktNode->userPriority, pPktMetaInfo->ucUP);
      }
   }

   pPktMetaInfo->ucType = 0;          //FIXME Don't know what this is
   pPktMetaInfo->ucDisableFrmXtl = 0; //802.3 frame so we need to xlate
   if ( 1 < size )
   {
       pPktMetaInfo->bMorePackets = 1; //HDD has more packets to send
   }
   else
   {
       pPktMetaInfo->bMorePackets = 0;
   }

   //Extract the destination address from ethernet frame
   pDestMacAddress = (v_MACADDR_t*)skb->data;
   pPktMetaInfo->ucBcast = vos_is_macaddr_broadcast( pDestMacAddress ) ? 1 : 0;
   pPktMetaInfo->ucMcast = vos_is_macaddr_group( pDestMacAddress ) ? 1 : 0;



   // if we are in a backpressure situation see if we can turn the hose back on
   if ( (pAdapter->isTxSuspended[ac]) &&
        (size <= HDD_TX_QUEUE_LOW_WATER_MARK) )
   {
      ++pAdapter->hdd_stats.hddTxRxStats.txFetchDePressured;
      ++pAdapter->hdd_stats.hddTxRxStats.txFetchDePressuredAC[ac];
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO,
                 "%s: TX queue[%d] re-enabled", __func__, ac);
      pAdapter->isTxSuspended[ac] = VOS_FALSE;
      netif_tx_wake_queue(netdev_get_tx_queue(pAdapter->dev,
                                        skb_get_queue_mapping(skb) ));
   }


   // We're giving the packet to TL so consider it transmitted from
   // a statistics perspective.  We account for it here instead of
   // when the packet is returned for two reasons.  First, TL will
   // manipulate the skb to the point where the len field is not
   // accurate, leading to inaccurate byte counts if we account for
   // it later.  Second, TL does not provide any feedback as to
   // whether or not the packet was successfully sent over the air,
   // so the packet counts will be the same regardless of where we
   // account for them
   pAdapter->stats.tx_bytes += skb->len;
   ++pAdapter->stats.tx_packets;
   ++pAdapter->hdd_stats.hddTxRxStats.txFetchDequeued;
   ++pAdapter->hdd_stats.hddTxRxStats.txFetchDequeuedAC[ac];

   if((pHddCtx->cfg_ini->thermalMitigationEnable) &&
      (WLAN_HDD_INFRA_STATION == pAdapter->device_mode))
   {
      if(mutex_lock_interruptible(&pHddCtx->tmInfo.tmOperationLock))
      {
         VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                    "%s: Tm Lock fail", __func__);
         return VOS_STATUS_E_FAILURE;
      }
      if(WLAN_HDD_TM_LEVEL_1 < pHddCtx->tmInfo.currentTmLevel)
      {
         if(0 == pHddCtx->tmInfo.txFrameCount)
         {
            /* Just recovered from sleep timeout */
            pHddCtx->tmInfo.lastOpenTs = timestamp;
         }

         if((VOS_FALSE == pHddCtx->tmInfo.qBlocked) &&
            ((timestamp - pHddCtx->tmInfo.lastOpenTs) > (pHddCtx->tmInfo.tmAction.txOperationDuration / 10)) &&
            (pHddCtx->tmInfo.txFrameCount >= pHddCtx->tmInfo.tmAction.txBlockFrameCountThreshold))
         {
            /* During TX open duration, TX frame count is larger than threshold
             * Block TX during Sleep time */
            netif_tx_stop_all_queues(pAdapter->dev);
            pHddCtx->tmInfo.qBlocked = VOS_TRUE;
            pHddCtx->tmInfo.lastblockTs = timestamp;
            if(VOS_TIMER_STATE_STOPPED == vos_timer_getCurrentState(&pHddCtx->tmInfo.txSleepTimer))
            {
               vos_timer_start(&pHddCtx->tmInfo.txSleepTimer, pHddCtx->tmInfo.tmAction.txSleepDuration);
            }
         }
         else if(((timestamp - pHddCtx->tmInfo.lastOpenTs) > (pHddCtx->tmInfo.tmAction.txOperationDuration / 10)) &&
                 (pHddCtx->tmInfo.txFrameCount < pHddCtx->tmInfo.tmAction.txBlockFrameCountThreshold))
         {
            /* During TX open duration, TX frame count is less than threshold
             * Reset count and timestamp to prepare next cycle */
            pHddCtx->tmInfo.lastOpenTs = timestamp;
            pHddCtx->tmInfo.txFrameCount = 0;
         }
         else
         {
            /* Do Nothing */
         }
         pHddCtx->tmInfo.txFrameCount++;
      }
      mutex_unlock(&pHddCtx->tmInfo.tmOperationLock);
   }


#ifdef HDD_WMM_DEBUG
   VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_FATAL,"%s: Valid VOS PKT returned to TL", __func__);
#endif // HDD_WMM_DEBUG

   return status;
}


/**============================================================================
  @brief hdd_tx_low_resource_cbk() - Callback function invoked in the
  case where VOS packets are not available at the time of the call to get
  packets. This callback function is invoked by VOS when packets are
  available.

  @param pVosPacket : [in]  pointer to VOS packet
  @param userData   : [in]  opaque user data that was passed initially

  @return           : VOS_STATUS_E_FAILURE if any errors encountered,
                    : VOS_STATUS_SUCCESS otherwise
  =============================================================================*/
VOS_STATUS hdd_tx_low_resource_cbk( vos_pkt_t *pVosPacket,
                                    v_VOID_t *userData )
{
   VOS_STATUS status;
   v_SINT_t i = 0;
   v_SIZE_t size = 0;
   hdd_adapter_t* pAdapter = (hdd_adapter_t *)userData;

   if (NULL == pAdapter)
   {
      VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                "%s: HDD adapter context is Null", __func__);
      return VOS_STATUS_E_FAILURE;
   }

   //Return the packet to VOS. We just needed to know that VOS is out of low resource
   //situation. Here we will only signal TL that there is a pending data for a STA.
   //VOS packet will be requested (if needed) when TL comes back to fetch data.
   vos_pkt_return_packet( pVosPacket );

   pAdapter->isVosOutOfResource = VOS_FALSE;

   //Indicate to TL that there is pending data if a queue is non empty
   for( i=NUM_TX_QUEUES-1; i>=0; --i )
   {
      size = 0;
      hdd_list_size( &pAdapter->wmm_tx_queue[i], &size );
      if ( size > 0 )
      {
         status = WLANTL_STAPktPending( (WLAN_HDD_GET_CTX(pAdapter))->pvosContext,
                                        (WLAN_HDD_GET_STATION_CTX_PTR(pAdapter))->conn_info.staId [0],
                                        (WLANTL_ACEnumType)i );
         if( !VOS_IS_STATUS_SUCCESS( status ) )
         {
            VOS_TRACE( VOS_MODULE_ID_HDD, VOS_TRACE_LEVEL_ERROR,
                      "%s: Failure in indicating pkt to TL for ac=%d", __func__,i);
         }
      }
   }

   return VOS_STATUS_SUCCESS;
}



/**============================================================================
  @brief hdd_rx_packet_cbk() - Receive callback registered with TL.
  TL will call this to notify the HDD when one or more packets were
  received for a registered STA.

  @param vosContext      : [in] pointer to VOS context
  @param staId           : [in] Station Id
  @param rxBuf           : [in] pointer to rx adf_nbuf

  @return                : VOS_STATUS_E_FAILURE if any errors encountered,
                         : VOS_STATUS_SUCCESS otherwise
  ===========================================================================*/
VOS_STATUS hdd_rx_packet_cbk(v_VOID_t *vosContext,
                             adf_nbuf_t rxBuf, v_U8_t staId)
{
   hdd_adapter_t *pAdapter = NULL;
   hdd_context_t *pHddCtx = NULL;
   int rxstat;
   struct sk_buff *skb = NULL;
#ifdef QCA_PKT_PROTO_TRACE
   v_U8_t proto_type;
#endif /* QCA_PKT_PROTO_TRACE */
   hdd_station_ctx_t *pHddStaCtx = NULL;

   //Sanity check on inputs
   if ((NULL == vosContext) || (NULL == rxBuf))
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,"%s: Null params being passed", __func__);
      return VOS_STATUS_E_FAILURE;
   }

   pHddCtx = (hdd_context_t *)vos_get_context( VOS_MODULE_ID_HDD, vosContext );
   if ( NULL == pHddCtx )
   {
      VOS_TRACE( VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,"%s: HDD adapter context is Null", __func__);
      return VOS_STATUS_E_FAILURE;
   }

   pAdapter = pHddCtx->sta_to_adapter[staId];
   if( NULL == pAdapter )
   {
      return VOS_STATUS_E_FAILURE;
   }
   ++pAdapter->hdd_stats.hddTxRxStats.rxChains;

   // walk the chain until all are processed
   skb = (struct sk_buff *) rxBuf;

   if (WLAN_HDD_ADAPTER_MAGIC != pAdapter->magic)
   {
       VOS_TRACE(VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_FATAL,
       "Magic cookie(%x) for adapter sanity verification is invalid", pAdapter->magic);
       return eHAL_STATUS_FAILURE;
   }

   pHddStaCtx = WLAN_HDD_GET_STATION_CTX_PTR(pAdapter);
   if ((pHddStaCtx->conn_info.proxyARPService) &&
         cfg80211_is_gratuitous_arp_unsolicited_na(skb))
   {
         ++pAdapter->hdd_stats.hddTxRxStats.rxDropped;
         VOS_TRACE(VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_INFO,
            "%s: Dropping HS 2.0 Gratuitous ARP or Unsolicited NA", __func__);
        kfree_skb(skb);
        return VOS_STATUS_SUCCESS;
   }

#ifdef FEATURE_WLAN_TDLS
#endif

#ifdef QCA_PKT_PROTO_TRACE
   if ((pHddCtx->cfg_ini->gEnableDebugLog & VOS_PKT_TRAC_TYPE_EAPOL) ||
       (pHddCtx->cfg_ini->gEnableDebugLog & VOS_PKT_TRAC_TYPE_DHCP))
   {
      proto_type = vos_pkt_get_proto_type(skb,
                        pHddCtx->cfg_ini->gEnableDebugLog, 0);
      if (VOS_PKT_TRAC_TYPE_EAPOL & proto_type)
      {
         vos_pkt_trace_buf_update("ST:R:EPL");
      }
      else if (VOS_PKT_TRAC_TYPE_DHCP & proto_type)
      {
         vos_pkt_trace_buf_update("ST:R:DHC");
      }
   }
#endif /* QCA_PKT_PROTO_TRACE */

   skb->dev = pAdapter->dev;
   skb->protocol = eth_type_trans(skb, skb->dev);
   ++pAdapter->hdd_stats.hddTxRxStats.rxPackets;
   ++pAdapter->stats.rx_packets;
   pAdapter->stats.rx_bytes += skb->len;
#ifdef WLAN_FEATURE_HOLD_RX_WAKELOCK
   vos_wake_lock_timeout_acquire(&pHddCtx->rx_wake_lock,
                                 HDD_WAKE_LOCK_DURATION);
#endif
   rxstat = netif_rx_ni(skb);
   if (NET_RX_SUCCESS == rxstat)
   {
       ++pAdapter->hdd_stats.hddTxRxStats.rxDelivered;
   }
   else
   {
       ++pAdapter->hdd_stats.hddTxRxStats.rxRefused;
   }

   pAdapter->dev->last_rx = jiffies;

   return VOS_STATUS_SUCCESS;
}

/**============================================================================
  @brief hdd_tx_rx_pkt_cnt_stat_timer_handler() -
               Enable/Disable split scan based on TX and RX traffic.
  @param HddContext      : [in] pointer to Hdd context
  @return                : None
  ===========================================================================*/
void hdd_tx_rx_pkt_cnt_stat_timer_handler( void *phddctx)
{

}

#ifdef IPA_OFFLOAD
/**============================================================================
  @brief hdd_rx_mul_packet_cbk() - Receive callback registered with TL.
  IPA integrated platform, TL Shim will give multiple RX frames with NETBUF
  link. Linked frames should be un-link and send to NETDEV.

  @param vosContext      : [in] pointer to VOS context
  @param rx_buf_list     : [in] pointer to rx adf_nbuf linked list
  @param staId           : [in] Station Id (Adress 1 Index)

  @return                : VOS_STATUS_E_FAILURE if any errors encountered,
                         : VOS_STATUS_SUCCESS otherwise
  ===========================================================================*/
VOS_STATUS hdd_rx_mul_packet_cbk(v_VOID_t *vosContext,
                                    adf_nbuf_t rx_buf_list, v_U8_t staId)
{
   adf_nbuf_t buf, next_buf;
   VOS_STATUS status;

   buf = rx_buf_list;
   while(buf)
   {
      next_buf = adf_nbuf_queue_next(buf);
      adf_nbuf_set_next(buf, NULL); /* Add NULL terminator */
      status = hdd_rx_packet_cbk(vosContext, buf, staId);
      if(!VOS_IS_STATUS_SUCCESS(status))
      {
         VOS_TRACE(VOS_MODULE_ID_HDD_DATA, VOS_TRACE_LEVEL_ERROR,
                   "%s: RX fail, status %d", __func__, status);
         return status;
      }
      buf = next_buf;
   }
   return VOS_STATUS_SUCCESS;
}
#endif /* IPA_OFFLOAD */

