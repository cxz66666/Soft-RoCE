#ifndef RXE_HPCC
#define RXE_HPCC

static const uint64_t hpcc_rdtsc_mul = 2200; // 1ns = 2.2 cpu cycles

#define HPCC_SNT_NXT 10
#define HPCC_T (30 * hpcc_rdtsc_mul)
#define HPCC_WAI 16
#define HPCC_MAXSTAGE 5
#define HPCC_N 0.95


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
    for (index = 0; index < 5; index++) {
        ack_hpcc_header[index] = hpcc_header[index];
    }
}

static inline void hpcc_recv_ack(struct rxe_qp *qp, struct rxe_pkt_info *pkt) {
    //TODO
    uint32_t *hpcc_header;
    if (rxe_opcode[pkt->opcode].offset[RXE_HPCC_HEADER] == 0) {
        return;
    }

    hpcc_header = (uint32_t *)(pkt->hdr + rxe_opcode[pkt->opcode].offset[RXE_HPCC_HEADER]);
    if (hpcc_header[0] >= qp->hpcc_last_update_seq) {

    } else {

    }
}
#endif