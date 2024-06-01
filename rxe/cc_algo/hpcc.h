#ifndef RXE_HPCC
#define RXE_HPCC
// static const uint64_t hpcc_rdtsc_mul = 2200; // 1ns = 2.2 cpu cycles

#define HPCC_SNT_NXT 10
#define HPCC_T (30)
#define HPCC_WAI 16
#define HPCC_MAXSTAGE 5
#define HPCC_N 9500


static inline int hpcc_check_credit(struct rxe_qp *qp, int payload) {
    return qp->hpcc_window - qp->hpcc_flying_bytes < payload ? 0 : 1;
}

static inline void hpcc_send_check(struct rxe_qp *qp, struct rxe_pkt_info *pkt, int payload) {
    uint32_t *hpcc_header;
    // don't need hpcc send check!
    if (rxe_opcode[pkt->opcode].offset[RXE_HPCC_HEADER] == 0) {
        return;
    }
    while (!hpcc_check_credit(qp, payload)) {}
    qp->hpcc_flying_bytes += payload;
    hpcc_header = (uint32_t *)(pkt->hdr + rxe_opcode[pkt->opcode].offset[RXE_HPCC_HEADER]);
    hpcc_header[0] = qp->hpcc_seq;
    hpcc_header[4] = qp->hpcc_B;
    hpcc_header[5] = payload;
    qp->hpcc_seq++;
}


static inline void hpcc_recv_pkt(struct rxe_qp *qp, struct rxe_pkt_info *pkt, struct rxe_pkt_info *ack_pkt) {
    uint32_t *hpcc_header, *ack_hpcc_header;
    int index;
    // just copy all hpcc header to ack_pkt
    if (rxe_opcode[pkt->opcode].offset[RXE_HPCC_HEADER] == 0) {
        return;
    }
    hpcc_header = (uint32_t *)(pkt->hdr + rxe_opcode[pkt->opcode].offset[RXE_HPCC_HEADER]);
    ack_hpcc_header = (uint32_t *)(ack_pkt->hdr + rxe_opcode[ack_pkt->opcode].offset[RXE_HPCC_HEADER]);
    for (index = 0; index < 6; index++) {
        ack_hpcc_header[index] = hpcc_header[index];
    }
}

static inline void hpcc_recv_ack(struct rxe_qp *qp, struct rxe_pkt_info *pkt) {
    uint64_t hpcc_u, hpcc_tx_rate, hpcc_diff;
    uint32_t *hpcc_header;
    if (rxe_opcode[pkt->opcode].offset[RXE_HPCC_HEADER] == 0) {
        return;
    }

    hpcc_header = (uint32_t *)(pkt->hdr + rxe_opcode[pkt->opcode].offset[RXE_HPCC_HEADER]);
    if (hpcc_header[0] >= qp->hpcc_last_update_seq) {
        hpcc_u = 0;
        hpcc_tx_rate = (hpcc_header[2] - qp->hpcc_txbyte) / (hpcc_header[3] - qp->hpcc_ts);
        if (hpcc_header[1] < qp->hpcc_qlen) {
            hpcc_u = hpcc_header[1] / (HPCC_T * qp->hpcc_B) + hpcc_tx_rate / qp->hpcc_B;
        } else {
            hpcc_u = qp->hpcc_qlen / (HPCC_T * qp->hpcc_B) + hpcc_tx_rate / qp->hpcc_B;
        }
        hpcc_diff = hpcc_header[3] - qp->hpcc_ts;
        if (hpcc_diff > HPCC_T) {
            hpcc_diff = HPCC_T;
        }
        qp->hpcc_U = qp->hpcc_U - qp->hpcc_U * hpcc_diff / HPCC_T + 10000 * hpcc_diff * hpcc_u / HPCC_T;
        if (qp->hpcc_U >= HPCC_N || qp->hpcc_inc_stage >= HPCC_MAXSTAGE) {
            qp->hpcc_window = qp->hpcc_window * HPCC_N / qp->hpcc_U + HPCC_WAI;
            qp->hpcc_inc_stage = 0;
        } else {
            qp->hpcc_window = qp->hpcc_window + HPCC_WAI;
            qp->hpcc_inc_stage++;
        }
        qp->hpcc_flying_bytes -= hpcc_header[5];
        qp->hpcc_last_update_seq += HPCC_SNT_NXT;
    } else {
        qp->hpcc_flying_bytes -= hpcc_header[5];
    }
    qp->hpcc_txbyte = hpcc_header[2];
    qp->hpcc_qlen = hpcc_header[1];
    qp->hpcc_ts = hpcc_header[3];
    qp->hpcc_B = hpcc_header[4];
}
#endif